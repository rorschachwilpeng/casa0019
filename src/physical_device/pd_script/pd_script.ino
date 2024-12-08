#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "arduino_secrets.h" 
#include <Ticker.h>

// Ticker instances for non-blocking tasks
Ticker buttonTicker;
Ticker knobTicker;

// USER SET VARIABLES FOR EACH DEPLOYMENT START
// Each device should connect to its corresponding feed
// e.g., light/1/ or light/2/ etc.
const char* mqtt_topic = "student/ucfnyyp/linesinfo/";
const char* mqtt_topic_all = "student/CASA0014/light/2/all/";
// USER SET VARIABLES FOR EACH DEPLOYMENT END

// Pins for button and rotary angle sensor
#define ROTARY_ANGLE_SENSOR A0  // The only analog pin on ESP8266
#define BUTTON_ANGLE_SENSOR 4   // GPIO4 (D2 pin)

// Voltage and angle configuration for rotary sensor
#define ADC_REF 3.3       // ESP8266 ADC reference voltage is 3.3V
#define FULL_ANGLE 300    // Maximum angle of the rotary sensor
#define GROVE_VCC 3.3     // Working voltage of the Grove interface is 3.3V

// Number of metro lines
#define NUM_LINES 4  

// Arrays to store westbound and eastbound times
int westBoundTimes[NUM_LINES];
int eastBoundTimes[NUM_LINES];

// Array to map line names
const char* lineNames[NUM_LINES] = {"Central", "DLR", "Elizabeth", "Jubilee"};

// Variables to track selected line and direction
int selectedLineNum = 0; // Current selected line index
bool isEastbound = true; // true = eastbound, false = westbound

/*
**** Enter sensitive data in the Secret tab/arduino_secrets.h
**** using the following format:

#define SECRET_SSID "ssid name"
#define SECRET_PASS "ssid password"
#define SECRET_MQTTUSER "user name - eg student"
#define SECRET_MQTTPASS "password";
*/

// WiFi and MQTT credentials
const char* ssid          = SECRET_SSID;
const char* password      = SECRET_PASS;
const char* mqtt_username = SECRET_MQTTUSER;
const char* mqtt_password = SECRET_MQTTPASS;
const char* mqtt_server = "mqtt.cetools.org";
const int mqtt_port = 1884;

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200);
  delay(10);

  // Initialize rotary angle sensor and button
  pinMode(ROTARY_ANGLE_SENSOR, INPUT); // A0 doesn't require manual pinMode
  pinMode(BUTTON_ANGLE_SENSOR, INPUT);

  // Connect to WiFi
  startWifi();
  
  // Connect to the MQTT broker
  client.setServer(mqtt_server, mqtt_port);
  client.setBufferSize(2000);
  client.setCallback(callback);

  Serial.println("Set-up complete");

  // Schedule tasks with tickers
  buttonTicker.attach_ms(50, checkButtonPress); // Check button every 50ms
  knobTicker.attach_ms(50, handleUserSelection); // Check rotary sensor every 50ms
}
 
void loop() {
  // Reconnect to MQTT if necessary
  if (!client.connected()) {
    reconnectMQTT();
  }
  // Reconnect to WiFi if disconnected
  if (WiFi.status() != WL_CONNECTED){
    startWifi();
  }
  // Keep MQTT connection alive
  client.loop();
}

// Function to handle incoming MQTT messages
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message received on topic: ");
  Serial.println(topic);

  // Check which topic the message is from
  if (strcmp(topic, mqtt_topic) == 0) {
    handleTrainData(payload, length);
  } else {
    Serial.println("Unknown topic");
  }
}

// Parse incoming train data and store in arrays
void handleTrainData(byte* payload, unsigned int length) {
  // Create a JSON document object to store parsed data
  DynamicJsonDocument doc(1024);  // Allocate sufficient space for the JSON document
  DeserializationError error = deserializeJson(doc, payload, length);

  // Check if JSON parsing was successful
  if (error) {
    Serial.print("JSON parsing failed: ");
    Serial.println(error.c_str());
    return;
  }

  // Initialize arrays for train times
  westBoundTimes[0] = doc["central_inbound_timeToStation"];    // Central
  westBoundTimes[1] = doc["dlr_inbound_timeToStation"];        // DLR
  westBoundTimes[2] = doc["elizabeth_inbound_timeToStation"];  // Elizabeth
  westBoundTimes[3] = doc["jubilee_inbound_timeToStation"];    // Jubilee

  eastBoundTimes[0] = -1;  // No outbound data for Central
  eastBoundTimes[1] = doc["dlr_outbound_timeToStation"];  // DLR
  eastBoundTimes[2] = -1;  // No outbound data for Elizabeth
  eastBoundTimes[3] = -1;  // No outbound data for Jubilee
}

void startWifi() {
  // Start connecting to the WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  // Wait until connected
  int counter = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(600);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnectMQTT() {
  // Loop until the client is reconnected
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect("ESP8266Client-neo2-ucjtdjw", mqtt_username, mqtt_password)) {
      Serial.println("connected");
      // Subscribe to the topic
      client.subscribe(mqtt_topic);
      Serial.println("Subscribed to MQTT topics");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

// Check button press
void checkButtonPress() {
  static unsigned long lastCheckTime = 0;
  if (millis() - lastCheckTime < 500) return; // Check every 50ms
  lastCheckTime = millis();

  static int lastButtonState = LOW;
  int currentButtonState = digitalRead(BUTTON_ANGLE_SENSOR);

  if (currentButtonState == HIGH && lastButtonState == LOW) {
    isEastbound = !isEastbound;
    Serial.print("Direction: ");
    Serial.println(isEastbound ? "Eastbound" : "Westbound");
  }

  lastButtonState = currentButtonState;
}

// Handle rotary angle sensor input
void handleUserSelection() {
  static unsigned long lastCheckTime = 0;
  if (millis() - lastCheckTime < 500) return; // Check every 50ms
  lastCheckTime = millis();

  float currentAngle = getEncoderStage();

  int newSelectedLine = mapAngleToDataType(currentAngle); // Map angle to line index
  if (newSelectedLine != selectedLineNum) {
    selectedLineNum = newSelectedLine;

    // Select array based on Eastbound or Westbound
    int timeToStation = isEastbound 
                        ? eastBoundTimes[selectedLineNum - 1] // Eastbound
                        : westBoundTimes[selectedLineNum - 1]; // Westbound

    // Print the selected line and time
    Serial.print("Selected Line: ");
    Serial.println(lineNames[selectedLineNum - 1]); // Print line name
    Serial.print("Direction: ");
    Serial.println(isEastbound ? "Eastbound" : "Westbound"); // Print direction
    Serial.print("Time to station: ");
    Serial.println(timeToStation); // Print time to station
    Serial.print("\n");
  }
}

// Get rotary angle
float getEncoderStage() {
  int sensorValue = analogRead(ROTARY_ANGLE_SENSOR);
  float voltage = (float)sensorValue * ADC_REF / 1023.0;
  float degrees = (voltage * FULL_ANGLE) / GROVE_VCC;
  return degrees;
}

// Map rotary angle to line index
int mapAngleToDataType(float angle) {
  if (angle >= 0 && angle <= 75) return 1;
  if (angle > 75 && angle <= 150) return 2;
  if (angle > 150 && angle <= 225) return 3;
  if (angle > 225 && angle <= 300) return 4;
  return 1;
}
