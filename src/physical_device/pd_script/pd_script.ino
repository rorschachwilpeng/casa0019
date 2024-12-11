/********************************************  Header  **************************************************/
#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "arduino_secrets.h" 
#include <Ticker.h>
#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
/********************************************  Header  **************************************************/

/********************************************  Variables  **************************************************/

#define LED_PIN 0        // Data pin connected to D2 (GPIO4 on ESP8266)
#define NUMPIXELS 16     // Number of NeoPixels in the strip
#define LED_BRIGHTNESS 30 // Brightness level (0-255)
#define SDA_PIN 2  // GPIO2，D4
#define SCL_PIN 14 // GPIO14，D5
#define MAXANGLE 140
#define MINANGLE 40

// Create NeoPixel object
Adafruit_NeoPixel pixels(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);
// Create LCD object
LiquidCrystal_I2C lcd(0x27, 16, 2);  

// Ticker instances for non-blocking tasks -- avoid any potential issue to the MQTT heartbeat mechnisim
Ticker buttonTicker;
Ticker knobTicker;
Ticker servoTicker; 
Ticker ledTicker; 
Ticker lcdTicker;

Servo myServo;
const char* mqtt_topic = "student/ucfnyyp/linesinfo/";
const char* mqtt_topic_all = "student/CASA0014/light/2/all/";

// Pins for button and rotary angle sensor
#define ROTARY_ANGLE_SENSOR A0  // The only analog pin on ESP8266
#define BUTTON_ANGLE_SENSOR 4   // GPIO4 (D2 pin)
#define SERVO_PIN 5  // GPIO5 (D1 pin)

// Voltage and angle configuration for rotary sensor
#define ADC_REF 3.3       // ESP8266 ADC reference voltage is 3.3V
#define FULL_ANGLE 400   // Maximum angle of the rotary sensor
#define GROVE_VCC 3.3     // Working voltage of the Grove interface is 3.3V

// Number of metro lines
#define NUM_LINES 4  

// Arrays to store westbound and eastbound times
int westBoundTimes[NUM_LINES];
int eastBoundTimes[NUM_LINES];
float inboundServiceLevel[NUM_LINES];
float outboundSerivceLevel[NUM_LINES];

// Array to map line names
//const char* lineNames[NUM_LINES] = { "Elizabeth", "Central","DLR", "Jubilee"};
const char* lineNames[NUM_LINES] = { "Jubilee", "DLR","Central","Elizabeth"};

// Variables to track selected line and direction
int selectedLineNum = 0; // Current selected line index
bool isEastbound = true; // true = eastbound, false = westbound
// 当前选中路线的 TimeToStation
int TimeToStation = -1;
bool newDataReceived = false;


int ledStates[NUMPIXELS] = {0}; // LED condition cache(0:off, 1:on)
int currentLEDIndex = 0;        // current light index

// WiFi and MQTT credentials
const char* ssid          = SECRET_SSID;
const char* password      = SECRET_PASS;
const char* mqtt_username = SECRET_MQTTUSER;
const char* mqtt_password = SECRET_MQTTPASS;
const char* mqtt_server = "mqtt.cetools.org";
const int mqtt_port = 1884;

// Variables for Servo control
static int currentServoAngle = 0; // Servo inital angle
static float currentServiceLevel = 0.0;

WiFiClient espClient;
PubSubClient client(espClient);
/********************************************  Variables  **************************************************/
void setup() {
  Serial.begin(115200);
  delay(10);
  Wire.begin(SDA_PIN, SCL_PIN);
  
  // Initialize LCD
  lcd.init();
  lcd.backlight();
  // Initialize rotary angle sensor and button
  pinMode(ROTARY_ANGLE_SENSOR, INPUT); // A0 doesn't require manual pinMode
  pinMode(BUTTON_ANGLE_SENSOR, INPUT);

  // Initialize NeoPixel strip
  pixels.begin();
  pixels.setBrightness(LED_BRIGHTNESS);

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
  servoTicker.attach_ms(100, updateServo); // Check Servo every 100ms
  ledTicker.attach_ms(100, updateLED); // Check LED every 100ms
  lcdTicker.attach_ms(500, updateLCD); // Check LCD every 500ms
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
/*************************************************  MQTT  *******************************************************/
// Function to handle incoming MQTT messages
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message received on topic: ");
  Serial.println(topic);

  // Check which topic the message is from
  if (strcmp(topic, mqtt_topic) == 0) {
    handleTrainData(payload, length);
    //newDataReceived = true;
  } else {
    Serial.println("Unknown topic");
  }
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
/*************************************************  MQTT  *******************************************************/


/*********************************************  Components  ****************************************************/
void updateServo() {
  int idx = selectedLineNum - 1;
  if (idx < 0 || idx >= NUM_LINES) return;

  float targetServiceLevel = isEastbound ? outboundSerivceLevel[idx] : inboundServiceLevel[idx];

  // Map the servo rotation angle to range:[40,140]
  int targetAngle = MINANGLE + ((1 - targetServiceLevel) * (MAXANGLE - MINANGLE));

  // servo init
  if (!myServo.attached()) {
    myServo.attach(SERVO_PIN, 500, 2400);
  }

  // update the angle
  static unsigned long lastUpdateTime = 0;
  unsigned long currentTime = millis();

  if (currentTime - lastUpdateTime > 10) { // update every 10ms
    lastUpdateTime = currentTime;

    if (currentServoAngle < targetAngle) {
      currentServoAngle += 5; // speed
      if (currentServoAngle > MAXANGLE) {
        currentServoAngle = MAXANGLE;//upper range
      }
    } else if (currentServoAngle > targetAngle) {
      currentServoAngle -= 5; // speed
      if (currentServoAngle < targetAngle) {
        currentServoAngle = targetAngle; //lower range
      }
    }
    myServo.write(currentServoAngle); // update servo
  }
}

void updateLED() {
  int idx = selectedLineNum - 1;
  if (idx < 0 || idx >= 4) {
    TimeToStation = -1; // invalid index
    return;
  }

  // update TimeToStation
  TimeToStation = isEastbound ? eastBoundTimes[idx] : westBoundTimes[idx];

  // to minutes
  int minutesToStation = TimeToStation / 60;

  // map the timeToStation to LED physical position
  int activeLED = -1;
  if (minutesToStation > 20) {
    activeLED = 14;
  } else if (minutesToStation == 20) {
    activeLED = 11;
  } else if (minutesToStation > 15) {
    activeLED = 10;
  } else if (minutesToStation == 15) {
    activeLED = 9;
  } else if (minutesToStation > 10) {
    activeLED = 8;
  } else if (minutesToStation == 10) {
    activeLED = 7;
  } else if (minutesToStation > 5) {
    activeLED = 6;
  } else if (minutesToStation == 5) {
    activeLED = 5;
  } else if (minutesToStation > 1) {
    activeLED = 4;
  } else {
    activeLED = 3;
  }

  // update array
  for (int i = 0; i < NUMPIXELS; i++) {
    ledStates[i] = (i <= activeLED) ? 1 : 0;
  }

  // update LED
  if (ledStates[currentLEDIndex] == 1) {
    pixels.setPixelColor(currentLEDIndex, pixels.Color(255, 255, 255)); // 白光
  } else {
    pixels.setPixelColor(currentLEDIndex, pixels.Color(0, 0, 0)); // 关闭灯
  }

  // update pixels
  pixels.show();

  // update the next index
  currentLEDIndex = (currentLEDIndex + 1) % NUMPIXELS;
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
    if (selectedLineNum < 1 || selectedLineNum > NUM_LINES) {
      Serial.println("Invalid line selection");
      return;
    }
    int idx = selectedLineNum-1;

    // Select array based on Eastbound or Westbound
    TimeToStation = isEastbound 
                        ? eastBoundTimes[idx] // Eastbound
                        : westBoundTimes[idx]; // Westbound
    // Service level value   
    float serVal = isEastbound 
                        ? inboundServiceLevel[idx] // Eastbound
                        : outboundSerivceLevel[idx]; // Westbound


    // Print the selected line and time
    Serial.print("------------------- LINE SWITCH ------------------- ");
    Serial.print("Selected Line: ");
    Serial.println(lineNames[idx]); // Print line name
    Serial.print("Direction: ");
    Serial.println(isEastbound ? "Eastbound" : "Westbound"); // Print direction
    Serial.print("Time to station: ");
    Serial.println(TimeToStation); // Print time to station
    Serial.print("Service Level Value: ");
    Serial.println(serVal); // Print time to station
    Serial.print("\n");
  }
}

// Get rotary angle
float getEncoderStage(){
  int sensorValue = analogRead(ROTARY_ANGLE_SENSOR);
  float voltage = (float)sensorValue * ADC_REF / 1023.0;
  float degrees = (voltage * FULL_ANGLE) / GROVE_VCC;
  return degrees;
}

// LCD update
void updateLCD() {
    int idx = selectedLineNum - 1;
    if (idx < 0 || idx >= 4) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Invalid Line");
        return;
    }

    // obtain direction and time info
    const char* direction = isEastbound ? "Eastbound" : "WestBound";
    int minutesToStation = TimeToStation / 60; // transform to minutes

    // update LCD 
    lcd.clear();

    // show current selected info
    lcd.setCursor(0, 0);
    lcd.print("Line: ");
    lcd.print(lineNames[idx]);

    //show current direction and time info
    lcd.setCursor(0, 1);
    lcd.print(direction);
    lcd.print(" T: ");
    lcd.print(minutesToStation);
    lcd.print("m");
}
/*********************************************  Components  ****************************************************/

/*********************************************  utilities ****************************************************/
// Map rotary angle to line index
int mapAngleToDataType(float angle) {
  if (angle >= 0 && angle <= 90) return 1;//Jubliee
  if (angle >90 && angle <= 310) return 2; //DLR
  if (angle > 310 && angle <= 380) return 3;//Central
  if (angle > 380) return 4; //Elizabeth
  return 4;
}

template <typename T>
T parseJsonValue(DynamicJsonDocument& doc, const char* key, T defaultValue, const char* typeName) {
  if (doc.containsKey(key) && doc[key].is<T>()) {
    return doc[key];
  } else {
    Serial.print("Missing or invalid key: ");
    Serial.println(key);
    return defaultValue;
  }
}

void handleTrainData(byte* payload, unsigned int length) {
  //Serial.println("Parsing train data...");

  DynamicJsonDocument doc(4096);
  DeserializationError error = deserializeJson(doc, payload, length);

  if (error) {
    Serial.print("JSON parsing failed: ");
    Serial.println(error.c_str());
    return;
  }
  //// Eastbound == Outbound
  //// Westbound == Inbound
  westBoundTimes[2] = parseJsonValue<int>(doc, "central_inbound_timeToStation", -1, "int");
  westBoundTimes[1] = parseJsonValue<int>(doc, "dlr_inbound_timeToStation", -1, "int");
  westBoundTimes[3] = parseJsonValue<int>(doc, "elizabeth_inbound_timeToStation", -1, "int");
  westBoundTimes[0] = parseJsonValue<int>(doc, "jubilee_inbound_timeToStation", -1, "int");

  //
  eastBoundTimes[2] = parseJsonValue<int>(doc, "central_outbound_timeToStation", -1, "int");
  eastBoundTimes[1] = parseJsonValue<int>(doc, "dlr_outbound_timeToStation", -1, "int");
  eastBoundTimes[3] = parseJsonValue<int>(doc, "elizabeth_inbound_timeToStation", -1, "int");
  eastBoundTimes[0] = -1;

  //Westbound
  inboundServiceLevel[2] = parseJsonValue<float>(doc, "central_inbound_service_level", 0.0f, "float");
  inboundServiceLevel[1] = parseJsonValue<float>(doc, "dlr_inbound_service_level", 0.0f, "float");
  inboundServiceLevel[3] = parseJsonValue<float>(doc, "elizabeth_outbound_service_level", 0.0f, "float");
  inboundServiceLevel[0] = parseJsonValue<float>(doc, "jubilee_inbound_service_level", 0.0f, "float");

  //EastBound
  outboundSerivceLevel[2] = parseJsonValue<float>(doc, "central_outbound_service_level", 0.0f, "float");
  outboundSerivceLevel[1] = parseJsonValue<float>(doc, "dlr_outbound_service_level", 0.0f, "float");
  outboundSerivceLevel[3] = parseJsonValue<float>(doc, "elizabeth_outbound_service_level", 0.0f, "float");
  outboundSerivceLevel[0] = parseJsonValue<float>(doc, "jubilee_inbound_service_level", 0.0f, "float");
  int idx = selectedLineNum - 1;
  if (idx >= 0 && idx < NUM_LINES) {
    TimeToStation = isEastbound ? eastBoundTimes[idx] : westBoundTimes[idx];
    float serviceLevel = isEastbound ?  outboundSerivceLevel[idx] : inboundServiceLevel[idx];

    Serial.println("---- Current Selected Route ----");
    Serial.print("Selected Line: ");
    Serial.println(lineNames[idx]);
    Serial.print("Direction: ");
    Serial.println(isEastbound ? "Eastbound" : "Westbound");
    Serial.print("Time to Station: ");
    Serial.println(TimeToStation);
    Serial.print("Service Level: ");
    Serial.println(serviceLevel);
    Serial.println("--------------------------------");
  } else {
    Serial.println("Invalid selected line index.");
  }
}
/*********************************************  utilities ****************************************************/