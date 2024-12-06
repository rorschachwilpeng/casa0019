#include <WiFiNINA.h>   
#include <PubSubClient.h>
#include <utility/wifi_drv.h>   // library to drive to RGB LED on the MKR1010
#include "arduino_secrets.h" 

const char* ssid          = SECRET_SSID;
const char* password      = SECRET_PASS;
const char* mqtt_username = SECRET_MQTTUSER;
const char* mqtt_password = SECRET_MQTTPASS;
const char* mqtt_server = "mqtt.cetools.org";
const int mqtt_port = 1884;
int status = WL_IDLE_STATUS;     // the Wifi radio's status


WiFiServer server(80);
WiFiClient wificlient;

WiFiClient mkrClient;
PubSubClient client(mkrClient);

// edit this for the light you are connecting to
char mqtt_topic_demo[] = "student/CASA0014/light/53/pixel/";
//const char* mqtt_topic_subscribe = "student/ucfnyyp/dlr_inbound/";
const char* mqtt_topic_subscribe = "student/CASA0014/light/53/pixel/";


/********* ---------------------------------------------------------------------------------------------------------------------------------------------------------------- ********/
/* Macro definitions of Rotary angle sensor */
#define ROTARY_ANGLE_SENSOR A0
#define ADC_REF 5       // Reference voltage of ADC is 5V
#define FULL_ANGLE 300  // Full value of the rotary angle is 300 degrees
#define GROVE_VCC 5     // VCC of the Grove interface is normally 5V
/* Macro definitions of Button angle sensor */
#define BUTTON_ANGLE_SENSOR A2

#define MAX_MESSAGE_LENGTH 256 // 限制消息最大长度
/********* ---------------------------------------------------------------------------------------------------------------------------------------------------------------- ********/
// 地铁线路数据
String lineNames[4] = {"Central", "DLR", "Elizabeth", "Jubilee"};
//int displayIndex = 0; // Display current data index
static int selectedLineNum=0;//当前所选的地铁路线索引
bool isEastbound = true;  // 地铁方向：true = 东向, false = 西向
int lastSelectedLine = -1;  // 上一次选择的线路编号，初始值为无效线路编号

/* Component variables */
static bool isDataSelected = false; 


////Button
int buttonState = 0; //initializing state to zero

// Timing variables for non-blocking logic
unsigned long lastMqttPublishTime = 0;
unsigned long lastButtonCheckTime = 0;
unsigned long lastEncoderCheckTime = 0;

String receivedMessage = ""; // 全局变量存储消息

/********* ---------------------------------------------------------------------------------------------------------------------------------------------------------------- ********/
void setup() {
  Serial.begin(115200);
  delay(1000);

  WiFi.setHostname("Lumina ucjtdjw");
  startWifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback); // 注册回调函数
  reconnectMQTT();
  
  Serial.println("setup complete");
  client.subscribe(mqtt_topic_subscribe); // 订阅主题
  Serial.println("Subscribed to topic: student/ucfnyyp/dlr_inbound/");

  // Encoder 
  pinMode(ROTARY_ANGLE_SENSOR, INPUT); // Set the rotary angle sensor pin as input
  //Button
  pinMode(BUTTON_ANGLE_SENSOR, INPUT);

}

void loop() {
  // 确保 WiFi 连接
  if (WiFi.status() != WL_CONNECTED) {
    startWifi();  // 重新尝试连接 WiFi
  }

  // 确保 MQTT 连接
  if (!client.connected()) {
    reconnectMQTT();  // 重新尝试连接 MQTT
  }

  client.loop();  // 处理 MQTT 消息

  // 处理 MQTT 消息发布
  if (millis() - lastMqttPublishTime > 10000) {  // 每 10 秒发布一次
    sendmqtt();
    lastMqttPublishTime = millis();
  }

  // 处理按钮逻辑
  if (millis() - lastButtonCheckTime > 50) {  // 每 50 毫秒检查一次按钮
    checkButtonPress();
    lastButtonCheckTime = millis();
  }

  // 处理旋钮逻辑
  if (millis() - lastEncoderCheckTime > 50) {  // 每 50 毫秒检查一次旋钮
    handleUserSelection();
    lastEncoderCheckTime = millis();
  }


  //delay(100); // Short delay to reduce CPU load
}

void sendmqtt(){

  // send a message to update the light
  char mqtt_message[100];
  sprintf(mqtt_message, "{\"pixelid\": %d, \"R\": 0, \"G\": 255, \"B\": 128, \"W\": 200}", 2);
  Serial.println(mqtt_topic_demo);
  Serial.println(mqtt_message);
  

  if (client.publish(mqtt_topic_demo, mqtt_message)) {
    Serial.println("Message published");
  } else {
    Serial.println("Failed to publish message");
  }

}

void startWifi(){
    
  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }
  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }

  // Function for connecting to a WiFi network
  // is looking for UCL_IoT and a back up network (usually a home one!)
  int n = WiFi.scanNetworks();
  Serial.println("Scan done");
  if (n == 0) {
    Serial.println("no networks found");
  } else {
    Serial.print(n);
    Serial.println(" networks found");
    // loop through all the networks and if you find UCL_IoT or the backup - ssid1
    // then connect to wifi
    Serial.print("Trying to connect to: ");
    Serial.println(ssid);
    for (int i = 0; i < n; ++i){
      String availablessid = WiFi.SSID(i);
      // Primary network
      if (availablessid.equals(ssid)) {
        Serial.print("Connecting to ");
        Serial.println(ssid);
        WiFi.begin(ssid, password);
        while (WiFi.status() != WL_CONNECTED) {
          delay(600);
          Serial.print(".");
        }
        if (WiFi.status() == WL_CONNECTED) {
          Serial.println("Connected to " + String(ssid));
          break; // Exit the loop if connected
        } else {
          Serial.println("Failed to connect to " + String(ssid));
        }
      } else {
        Serial.print(availablessid);
        Serial.println(" - this network is not in my list");
      }

    }
  }


  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

}

void reconnectMQTT() {
  if (WiFi.status() != WL_CONNECTED) {
    startWifi();
  }

  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "LuminaSelector";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("connected");
      client.subscribe(mqtt_topic_subscribe); // 重新订阅主题
      Serial.println("Re-subscribed to topic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void callback(char* topic, uint8_t* payload, unsigned int length) {
  // Serial.print("Message arrived [");
  // Serial.print(topic);
  // Serial.print("]: ");

  // // 限制消息长度
  // unsigned int msgLength = (length > MAX_MESSAGE_LENGTH) ? MAX_MESSAGE_LENGTH : length;

  // // 清空 receivedMessage，仅保存当前消息
  // receivedMessage = "";
  // for (unsigned int i = 0; i < msgLength; i++) {
  //   receivedMessage += (char)payload[i];
  // }
  // Serial.println(receivedMessage);

  // // 处理逻辑
  // if (String(topic) == "student/ucfnyyp/dlr_inbound/") {
  //   Serial.println("Message received for student/ucfnyyp/dlr_inbound/");
  // }
  char str[length+1];
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  int i=0;
  for (i=0;i<length;i++) {
  Serial.print((char)payload[i]);
  str[i]=(char)payload[i];
  }
}


//******------------------------------------------------- General -------------------------------------------------******//
// User selects the datatype (which sensor data to use for LED control)
void handleUserSelection() {
    //Check button press states
    checkButtonPress();

    static unsigned long stableStartTime = 0; // Start timestamp for stability
    static float lastStableAngle = -1;       // Last stable angle
    float currentAngle = getEncoderStage();  // Get current rotary angle

    // Display the current selection
    //displaySelectionOnLCD(currentAngle);

    // Check if the angle is stable
    if (abs(currentAngle - lastStableAngle) <= 5) {
        // If the current angle is stable within the range of the last angle
        if (millis() - stableStartTime > 1500) {
            // If the angle is stable for more than 3 seconds, consider the selection confirmed
            selectedLineNum = mapAngleToDataType(currentAngle);
            isDataSelected = true;
            // 只有当线路发生切换时，才打印信息
            if (selectedLineNum != lastSelectedLine) {
                Serial.print("Selected Line: ");
                Serial.println(lineNames[selectedLineNum - 1]);
                lastSelectedLine = selectedLineNum;  // 更新上次选择的线路
            }
        }
    } else {
        // If the current angle changes, reset stability timing
        stableStartTime = millis();
        lastStableAngle = currentAngle;
    }
}
//******------------------------------------------------- General -------------------------------------------------******//

//******------------------------------------------------- Encoder -------------------------------------------------******//
// Encoder logic
float getEncoderStage(){
    // Read the analog value from the rotary angle sensor
    int sensor_value = analogRead(ROTARY_ANGLE_SENSOR);
    // Convert sensor value to voltage
    float voltage = (float)sensor_value * ADC_REF / 1023;
    // Convert voltage to angle (in degrees)
    float degrees = (voltage * FULL_ANGLE) / GROVE_VCC;
    return degrees;
}

// Map the Encoder rotation angle to the datatype
int mapAngleToDataType(float angle) {
    if (angle >= 0 && angle <= 75) return 1; // Central
    if (angle > 75 && angle <= 150) return 2; // DLR
    if (angle > 150 && angle <= 225) return 3; // Elizabeth
    if (angle > 225 && angle <= 300) return 4; // Jubilee
    //return 1; // Central
}                                                                                                                                 
//******------------------------------------------------- Encoder -------------------------------------------------******//

//******------------------------------------------------- Button -------------------------------------------------******//
void checkButtonPress() {
    static int lastButtonState = LOW;          // 上一次按钮状态
    int currentButtonState = digitalRead(BUTTON_ANGLE_SENSOR); // 当前按钮状态

    // 检测按钮从未按下（LOW）到按下（HIGH）的瞬间
    if (currentButtonState == HIGH && lastButtonState == LOW) {
        isEastbound = !isEastbound;  // 切换方向
        Serial.print("Direction: ");
        Serial.println(isEastbound ? "Eastbound" : "Westbound");

        // 在 LCD 上显示方向（如需要）
        // lcd.setCursor(0, 1);
        // lcd.print("Dir: ");
        // lcd.print(isEastbound ? "East" : "West");
    }

    // 更新上次按钮状态
    lastButtonState = currentButtonState;
}

//******------------------------------------------------- Button -------------------------------------------------******//



//TODO
/////General Logic
//1.通过旋转旋钮能够选择不同的线路 --> 一共四条线路，每条线路90度✅
//2.按下按钮后，地铁方向发生改变✅
//3.能够订阅，发送，拉取指定MQTT中频道的信息
//4. -- 4.1.让Servo能够发生变化; 4.2.能够让Servo根据频道中不同的服务数值进行变化
//5. -- 5.1.让LED能够发生变化; 5.2.能够让LED根据频道中列车到达时间进行显示; 5.3.需要确定一个计算公式，因为有很多辆列车，而我们每次只需要确定其中最近的一辆；
//6.在LCD上显示消息：当前所选线路，                    


//Test
//1. 测试旋钮能不能用
//2. 将旋钮的角度映射在四条线路上，做到能通过调整旋钮来改变线路
//3. 加入按钮功能，按下按钮后地铁方向会发生变化
//4. 在LCD上显示信息：当前所选的线路 --> TMD LCD在哪？？？
//5. 从MQTT上抓取数据

