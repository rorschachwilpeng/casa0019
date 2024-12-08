#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "arduino_secrets.h" 
#include <Ticker.h>

Ticker buttonTicker;
Ticker knobTicker;
// USER SET VARIABLE FOR EACH DEPLOYMENT START
// Each device should connect to the feed that belongs to it
// e.g. light/1/ or light/2/ etc.
const char* mqtt_topic = "student/ucfnyyp/linesinfo/";
const char* mqtt_topic_all = "student/CASA0014/light/2/all/";
// USER SET VARIABLE FOR EACH DEPLOYMENT END

// 按钮和旋钮的引脚
#define ROTARY_ANGLE_SENSOR A0  // ESP8266上的唯一模拟引脚
#define BUTTON_ANGLE_SENSOR 4   // GPIO4（D2引脚）

#define ADC_REF 3.3       // ESP8266的ADC参考电压为3.3V
#define FULL_ANGLE 300    // 旋钮的最大角度
#define GROVE_VCC 3.3     // ESP8266的工作电压
#define NUM_LINES 4  // 地铁线路数量

int inboundTimes[NUM_LINES];  // inbound方向数据
int outboundTimes[NUM_LINES]; // outbound方向数据

//String lineNames[4] = {"Central", "DLR", "Elizabeth", "Jubilee"};
const char* lineNames[NUM_LINES] = {"Central", "DLR", "Elizabeth", "Jubilee"};
int selectedLineNum = 0; // 当前所选线路索引
bool isEastbound = true; // true = inbound, false = outbound

/*
**** please enter your sensitive data in the Secret tab/arduino_secrets.h
**** using format below

#define SECRET_SSID "ssid name"
#define SECRET_PASS "ssid password"
#define SECRET_MQTTUSER "user name - eg student"
#define SECRET_MQTTPASS "password";
 */

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

  // 初始化旋钮和按钮 configure encoder and button
  pinMode(ROTARY_ANGLE_SENSOR, INPUT); // A0 不需要手动 pinMode
  pinMode(BUTTON_ANGLE_SENSOR, INPUT);

  // Connect to WiFi
  startWifi();
  
  // Connect to MQTT broker
  client.setServer(mqtt_server, mqtt_port);
  client.setBufferSize(2000);
  client.setCallback(callback);

  Serial.println("Set-up complete");
  // 定时器任务
  buttonTicker.attach_ms(50, checkButtonPress); // 每 50ms 检查按钮
  knobTicker.attach_ms(50, handleUserSelection); // 每 50ms 检查旋钮
}
 
void loop() {
  // Reconnect if necessary
  if (!client.connected()) {
    reconnectMQTT();
  }
  if (WiFi.status() != WL_CONNECTED){
    startWifi();
  }
  // keep mqtt alive
  client.loop();
}

// Function to handle incoming MQTT messages
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message received on topic: ");
  Serial.println(topic);

  // Check which topic the message is received from
  if (strcmp(topic, mqtt_topic) == 0) {
    handleTrainData(payload, length);
  }  else {
    Serial.println("Unknown topic");
  }
}

void handleTrainData(byte* payload, unsigned int length) {
  // 创建一个 JSON 文档对象来存储解析后的数据
  DynamicJsonDocument doc(1024);  // 为 JSON 文档分配足够大的空间
  DeserializationError error = deserializeJson(doc, payload, length);

  // 检查 JSON 解析是否成功
  if (error) {
    Serial.print("JSON parsing failed: ");
    Serial.println(error.c_str());
    return;
  }

  // 打印接收到的 JSON 数据（调试用）
  //Serial.println("Parsed JSON:");
  //serializeJsonPretty(doc, Serial);

  // 初始化地铁线路的时间数组
  inboundTimes[0] = doc["central_inbound_timeToStation"];    // Central
  inboundTimes[1] = doc["dlr_inbound_timeToStation"];        // DLR
  inboundTimes[2] = doc["elizabeth_inbound_timeToStation"];  // Elizabeth
  inboundTimes[3] = doc["jubilee_inbound_timeToStation"];    // Jubilee

  outboundTimes[0] = -1;  // Central没有outbound数据，填充为-1
  outboundTimes[1] = doc["dlr_outbound_timeToStation"];  // DLR
  outboundTimes[2] = -1;  // Elizabeth没有outbound数据，填充为-1
  outboundTimes[3] = -1;  // Jubilee没有outbound数据，填充为-1

  // 打印解析后的数组内容
  Serial.println("\nInbound Times:");
  for (int i = 0; i < NUM_LINES; i++) {
    Serial.print("Line ");
    Serial.print(lineNames[i]);  // 映射线路名称
    Serial.print(": ");
    Serial.println(inboundTimes[i]);
  }

  Serial.println("\nOutbound Times:");
  for (int i = 0; i < NUM_LINES; i++) {
    Serial.print("Line ");
    Serial.print(lineNames[i]);  // 映射线路名称
    Serial.print(": ");
    Serial.println(outboundTimes[i]);
  }
}

void startWifi(){
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  // check to see if connected and wait until you are
  int counter = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(600);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

}

void reconnectMQTT() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect("ESP8266Client-neo2-ucjtdjw", mqtt_username, mqtt_password)) {
      Serial.println("connected");
      // Subscribe to the topic you want to listen to
      client.subscribe(mqtt_topic);
      //client.subscribe(mqtt_topic_all);
      Serial.println("Subscribed to MQTT topics");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

// 检查按钮按下
void checkButtonPress() {
  static unsigned long lastCheckTime = 0;
  if (millis() - lastCheckTime < 500) return; // 每隔50ms检查一次
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

// 处理旋钮输入
void handleUserSelection() {
  static unsigned long lastCheckTime = 0;
  if (millis() - lastCheckTime < 500) return; // 每隔50ms检查一次
  lastCheckTime = millis();

  float currentAngle = getEncoderStage();

  int newSelectedLine = mapAngleToDataType(currentAngle);
  if (newSelectedLine != selectedLineNum) {
    selectedLineNum = newSelectedLine;
    Serial.print("Selected Line: ");
    Serial.println(lineNames[selectedLineNum - 1]);
  }
}

// 读取旋钮角度
float getEncoderStage() {
  int sensorValue = analogRead(ROTARY_ANGLE_SENSOR);
  float voltage = (float)sensorValue * ADC_REF / 1023.0;
  float degrees = (voltage * FULL_ANGLE) / GROVE_VCC;
  return degrees;
}

// 映射旋钮角度到线路索引
int mapAngleToDataType(float angle) {
  if (angle >= 0 && angle <= 75) return 1;
  if (angle > 75 && angle <= 150) return 2;
  if (angle > 150 && angle <= 225) return 3;
  if (angle > 225 && angle <= 300) return 4;
  return 1;
}