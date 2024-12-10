#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "arduino_secrets.h" 
#include <Ticker.h>
#include <Servo.h>


// NeoPixel configuration (参考脚本2)
#define LED_PIN 0        // Data pin connected to D2 (GPIO4 on ESP8266)
#define NUMPIXELS 16     // Number of NeoPixels in the strip
#define LED_BRIGHTNESS 50 // Brightness level (0-255)
// Create NeoPixel object
Adafruit_NeoPixel pixels(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Ticker instances for non-blocking tasks
Ticker buttonTicker;
Ticker knobTicker;
Ticker servoTicker; // 定时更新 Servo 的任务
Ticker ledTicker; // 定时器用于更新LED状态

Servo myServo;
// USER SET VARIABLES FOR EACH DEPLOYMENT START
// Each device should connect to its corresponding feed
// e.g., light/1/ or light/2/ etc.
const char* mqtt_topic = "student/ucfnyyp/linesinfo/";
const char* mqtt_topic_all = "student/CASA0014/light/2/all/";
// USER SET VARIABLES FOR EACH DEPLOYMENT END

// Pins for button and rotary angle sensor
#define ROTARY_ANGLE_SENSOR A0  // The only analog pin on ESP8266
#define BUTTON_ANGLE_SENSOR 4   // GPIO4 (D2 pin)
#define SERVO_PIN 5  // GPIO5 (D1 pin)

// Voltage and angle configuration for rotary sensor
#define ADC_REF 3.3       // ESP8266 ADC reference voltage is 3.3V
#define FULL_ANGLE 300    // Maximum angle of the rotary sensor
#define GROVE_VCC 3.3     // Working voltage of the Grove interface is 3.3V

// Number of metro lines
#define NUM_LINES 4  

// Arrays to store westbound and eastbound times
int westBoundTimes[NUM_LINES];
int eastBoundTimes[NUM_LINES];
float inboundServiceLevel[NUM_LINES];
float outboundSerivceLevel[NUM_LINES];

// Array to map line names
const char* lineNames[NUM_LINES] = {"Central", "DLR", "Elizabeth", "Jubilee"};

// Variables to track selected line and direction
int selectedLineNum = 0; // Current selected line index
bool isEastbound = true; // true = eastbound, false = westbound
// 当前选中路线的 TimeToStation
int TimeToStation = -1;
bool newDataReceived = false;

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

// Variables for Servo control
static int currentServoAngle = 0; // Servo 初始角度
static float currentServiceLevel = 0.0; // 当前服务水平值

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200);
  delay(10);

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
  servoTicker.attach_ms(100, updateServo); // 每 100ms 检查是否需要更新 Servo
  //ledTicker.attach_ms(100, updateLED); // 每 100ms 检查并更新 LED 状态
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
    //newDataReceived = true;
  } else {
    Serial.println("Unknown topic");
  }
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

  // Ensure valid array values
  for (int i = 0; i < NUM_LINES; i++) {
    westBoundTimes[i] = -1;
    eastBoundTimes[i] = -1;
    inboundServiceLevel[i] = 0.0;
    outboundSerivceLevel[i] = 0.0;
  }

  westBoundTimes[0] = parseJsonValue<int>(doc, "central_inbound_timeToStation", -1, "int");
  westBoundTimes[1] = parseJsonValue<int>(doc, "dlr_inbound_timeToStation", -1, "int");
  westBoundTimes[2] = parseJsonValue<int>(doc, "elizabeth_inbound_timeToStation", -1, "int");
  westBoundTimes[3] = parseJsonValue<int>(doc, "jubilee_inbound_timeToStation", -1, "int");

  eastBoundTimes[0] = parseJsonValue<int>(doc, "central_outbound_timeToStation", -1, "int");
  eastBoundTimes[1] = parseJsonValue<int>(doc, "dlr_outbound_timeToStation", -1, "int");
  eastBoundTimes[2] = parseJsonValue<int>(doc, "elizabeth_outbound_timeToStation", -1, "int");
  eastBoundTimes[3] = -1;

  inboundServiceLevel[0] = parseJsonValue<float>(doc, "central_inbound_service_level", 0.0f, "float");
  inboundServiceLevel[1] = parseJsonValue<float>(doc, "dlr_inbound_service_level", 0.0f, "float");
  inboundServiceLevel[2] = parseJsonValue<float>(doc, "elizabeth_inbound_service_level", 0.0f, "float");
  inboundServiceLevel[3] = parseJsonValue<float>(doc, "jubilee_inbound_service_level", 0.0f, "float");

  outboundSerivceLevel[0] = parseJsonValue<float>(doc, "central_outbound_service_level", 0.0f, "float");
  outboundSerivceLevel[1] = parseJsonValue<float>(doc, "dlr_outbound_service_level", 0.0f, "float");
  outboundSerivceLevel[2] = parseJsonValue<float>(doc, "elizabeth_outbound_service_level", 0.0f, "float");
  // 打印当前所选路线和方向的值
  int idx = selectedLineNum - 1; // 获取当前所选路线的索引
  if (idx >= 0 && idx < NUM_LINES) {
    int timeToStation = isEastbound ? eastBoundTimes[idx] : westBoundTimes[idx];
    float serviceLevel = isEastbound ? inboundServiceLevel[idx] : outboundSerivceLevel[idx];

    Serial.println("---- Current Selected Route ----");
    Serial.print("Selected Line: ");
    Serial.println(lineNames[idx]);
    Serial.print("Direction: ");
    Serial.println(isEastbound ? "Eastbound" : "Westbound");
    Serial.print("Time to Station: ");
    Serial.println(timeToStation);
    Serial.print("Service Level: ");
    Serial.println(serviceLevel);
    Serial.println("--------------------------------");
  } else {
    Serial.println("Invalid selected line index.");
  }
}

void updateServo() {
  // 获取当前服务水平值
  int idx = selectedLineNum - 1;
  if (idx < 0 || idx >= NUM_LINES) return;

  float targetServiceLevel = isEastbound ? inboundServiceLevel[idx] : outboundSerivceLevel[idx];

  // 计算目标角度
  int targetAngle = 180 - (targetServiceLevel * 180);

  // 初始化舵机
  if (!myServo.attached()) {
    myServo.attach(SERVO_PIN, 500, 2400);
  }

  // 持续调整角度
  static unsigned long lastUpdateTime = 0;
  unsigned long currentTime = millis();

  if (currentTime - lastUpdateTime > 10) { // 每 10 毫秒调整一次
    lastUpdateTime = currentTime;

    if (currentServoAngle < targetAngle) {
      currentServoAngle += 5; // 加快调整速度，每次增加 2°
      if (currentServoAngle > targetAngle) {
        currentServoAngle = targetAngle; // 防止超出目标角度
      }
    } else if (currentServoAngle > targetAngle) {
      currentServoAngle -= 5; // 加快调整速度，每次减少 2°
      if (currentServoAngle < targetAngle) {
        currentServoAngle = targetAngle; // 防止超出目标角度
      }
    }

    myServo.write(currentServoAngle); // 更新 Servo 角度
    //Serial.print("Current Servo Angle: ");
    //Serial.println(currentServoAngle);

    // 打印调试信息
    // if (currentServoAngle == targetAngle) {
    //   Serial.println("Servo reached target angle.");
    // }
  }
}

void updateLED() {
  static int rainbowOffset = 0; // 用于控制彩虹流动效果的偏移量

  // 确保选中的路线索引有效
  int idx = selectedLineNum - 1;
  if (idx < 0 || idx >= NUM_LINES) {
    TimeToStation = -1; // 无效索引，设置为默认值
    return;
  }

  // 更新全局变量 TimeToStation
  TimeToStation = isEastbound ? eastBoundTimes[idx] : westBoundTimes[idx];

  // 如果无效或负值，清除所有灯
  if (TimeToStation < 0) {
    pixels.clear();
    pixels.show();
    return;
  }

  // 将 TimeToStation 转换为分钟
  int minutesToStation = TimeToStation / 60;

  // 根据分钟设置灯光范围
  int activeLED = -1; // 活跃灯索引

  if (minutesToStation > 20) {
    activeLED = 14; // 第14颗灯表示 >20 分钟
  } else if (minutesToStation > 15) {
    activeLED = 12; // 第12颗灯表示 >15 分钟
  } else if (minutesToStation > 10) {
    activeLED = 10; // 第10颗灯表示 >10 分钟
  } else if (minutesToStation > 5) {
    activeLED = 8;  // 第8颗灯表示 >5 分钟
  } else {
    activeLED = 6;  // 第6颗灯表示 <=5 分钟
  }

  // 根据活跃灯索引设置彩虹流动效果
  // for (int i = 0; i < NUMPIXELS; i++) {
  //   if (i <= activeLED) {
  //     // 使用偏移量控制彩虹流动效果
  //     pixels.setPixelColor(i, Wheel((i * 256 / NUMPIXELS + rainbowOffset) & 255));
  //   } else {
  //     // 关闭灯
  //     pixels.setPixelColor(i, pixels.Color(0, 0, 0));
  //   }
  // }

  // 更新灯光显示
  pixels.show();

  // 增加偏移量以实现流动效果
  rainbowOffset = (rainbowOffset + 1) % 256; // 保持偏移量在 0-255 范围内循环
}


// Helper function: Generate rainbow colors across 0-255 positions
uint32_t Wheel(byte WheelPos) {
  if (WheelPos < 85) {
    return pixels.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if (WheelPos < 170) {
    WheelPos -= 85;
    return pixels.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
    WheelPos -= 170;
    return pixels.Color(0, WheelPos * 3, 255 - WheelPos * 3);
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
    int timeToStation = isEastbound 
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
    Serial.println(timeToStation); // Print time to station
    Serial.print("Service Level Value: ");
    Serial.println(serVal); // Print time to station
    Serial.print("\n");

     // Set servo angle based on service level
    //setServoAngleByServiceLevel(serVal);
  }
}

void setServoAngleByServiceLevel(float serVal) {
  if (serVal < 0.0 || serVal > 1.0) {
    Serial.println("Invalid service level value. It should be between 0 and 1.");
    return;
  }

  // Map service level (0 to 1) to servo angle (0 to 180)
  int targetAngle = 180 - (serVal * 180); // Reverse angle
  Serial.print("Target Angle: ");
  Serial.println(targetAngle);

  // Ensure servo is attached and initialized
  if (!myServo.attached()) {
    myServo.attach(SERVO_PIN, 500, 2400); // Adjust PWM range for specific servo
    Serial.println("Servo attached");
  }

  static int currentAngle = 90; // Set initial angle to 90°
  static unsigned long lastUpdateTime = 0;

  if (millis() - lastUpdateTime > 5) { // Update every 5ms for smooth motion
    lastUpdateTime = millis();

    if (currentAngle < targetAngle) {
      currentAngle++;
    } else if (currentAngle > targetAngle) {
      currentAngle--;
    }

    myServo.write(currentAngle); // Move servo
    Serial.print("Moving to angle: ");
    Serial.println(currentAngle);
  }
}


// Get rotary angle
float getEncoderStage(){
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
