#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <Adafruit_NeoPixel.h>

// 硬件定义
#define ENCODER_PIN_A 2
#define ENCODER_PIN_B 3
#define ENCODER_BUTTON_PIN 4
#define NEOPIXEL_PIN 9  // 灯条信号引脚 D9
#define SERVO_PIN 6     // 舵机信号引脚 D6

// LCD屏幕 (接线为 SDA -> A4, SCL -> A5)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// 舵机和灯条
Servo myServo;
Adafruit_NeoPixel strip = Adafruit_NeoPixel(8, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

// 地铁线路数据
String lineNames[5] = {"Central", "DLR", "Elizabeth", "Jubilee","Midmay"};
int currentLine = 0;  // 当前线路索引
bool isEastbound = true;  // 地铁方向：true = 东向, false = 西向

// 模拟服务质量和地铁距离（为每条线路设置）
int serviceQuality[5] = {45, 90, 135, 180, 60};  // 每条线路的服务质量角度
int distances[5] = {2, 5, 8, 4, 6};             // 每条线路的地铁距离（灯珠亮起数量）

// 舵机当前角度变量，避免重复发送相同角度的命令
int currentServoAngle = 0;


// 旋钮变量
int encoderPosition = 0;     // 记录旋钮的增量位置
const int stepsPerLine = 4;   // 每条线路对应的步进数量
const int maxSteps = 16;      // 360°对应的总步进数

void setup() {
  Serial.begin(9600);
  Wire.begin();  // 初始化I2C，不需要指定SDA和SCL引脚

  // 初始化LCD屏幕
  lcd.begin(16, 2);  // 初始化LCD，指定列数和行数
  lcd.backlight();   // 打开背光
  lcd.print("System Start!");

  // 初始化舵机和灯条
  myServo.attach(SERVO_PIN);
  myServo.write(currentServoAngle);  // 初始化舵机到 0 度
  strip.begin();
  strip.show();

  // 初始化旋钮
  pinMode(ENCODER_PIN_A, INPUT);
  pinMode(ENCODER_PIN_B, INPUT);
  pinMode(ENCODER_BUTTON_PIN, INPUT_PULLUP);

  updateLCD();  // 初始化显示
  updateServoAndNeoPixel();  // 初始化舵机和灯条
}

void loop() {
  // 检测按钮按下，切换方向
  checkButtonPress();
  // 读取旋钮值并更新线路
  updateLine();
}


//******------------------------------------------------- Encoder -------------------------------------------------******//
// 按钮按下时切换方向
void checkButtonPress() {
  static bool lastButtonState = HIGH;
  bool currentButtonState = digitalRead(ENCODER_BUTTON_PIN);

  if (lastButtonState == HIGH && currentButtonState == LOW) {
    isEastbound = !isEastbound;  // 切换方向
    Serial.print("Direction: ");
    Serial.println(isEastbound ? "Eastbound" : "Westbound");

    // 在LCD上显示方向
    lcd.setCursor(0, 1);  // 设置光标到第2行
    lcd.print("Dir: ");
    lcd.print(isEastbound ? "East" : "West");  // 显示 East 或 West
    lcd.print("          "); // 清空多余字符
  }
  lastButtonState = currentButtonState;
}

// 旋钮旋转时切换线路
void updateLine() {
  static int lastStateA = HIGH;
  int currentStateA = digitalRead(ENCODER_PIN_A);

  if (lastStateA == HIGH && currentStateA == LOW) {
      Serial.print("Position Changed");
      if (digitalRead(ENCODER_PIN_B) == HIGH) {
        currentLine = (currentLine + 1) % 5;  // 顺时针旋转：切换到下一条线路
      } else {
        currentLine = (currentLine - 1 + 5) % 5;  // 逆时针旋转：切换到上一条线路
      }
      Serial.print(" -> Current Line: ");
      Serial.println(lineNames[currentLine]);
      updateLCD();
      updateServoAndNeoPixel();  // 更新舵机和灯条
  }
  lastStateA = currentStateA;
}


// // 更新旋钮状态并计算线路
// void updateLine() {
//     static int lastStateA = HIGH;
//     static unsigned long lastDebounceTime = 0;  // 去抖动时间戳
//     const unsigned long debounceDelay = 5;     // 去抖动延迟时间（单位：毫秒）

//     int currentStateA = digitalRead(ENCODER_PIN_A);
//     unsigned long currentTime = millis();

//     if (lastStateA != currentStateA && (currentTime - lastDebounceTime) > debounceDelay) {
//         // 更新去抖动时间
//         lastDebounceTime = currentTime;

//         // 判断旋转方向
//         if (currentStateA == LOW) {
//             if (digitalRead(ENCODER_PIN_B) == HIGH) {
//                Serial.print("Position Changed");
//                 encoderPosition++;  // 顺时针
//             } else {
//               Serial.print("Position Changed");
//                 encoderPosition--;  // 逆时针
//             }

//             // 将步进限制在 [0, maxSteps) 范围内
//             encoderPosition = (encoderPosition + maxSteps) % maxSteps;

//             // 根据步进值计算当前线路
//             int newLine = encoderPosition / stepsPerLine;
//             if (newLine != currentLine) {
//                 currentLine = newLine;

//                 // 打印当前线路信息到串口监视器
//                 Serial.print("Encoder Position: ");
//                 Serial.print(encoderPosition);
//                 Serial.print(" -> Current Line: ");
//                 Serial.println(lineNames[currentLine]);

//                 updateLCD();
//                 updateServoAndNeoPixel();
//             }
//         }
//     }

//     lastStateA = currentStateA;
// }

//******------------------------------------------------- Encoder -------------------------------------------------******//

//******------------------------------------------------- LCD -------------------------------------------------******//
// 更新LCD显示
void updateLCD() {
  lcd.clear();
  lcd.print("Line: ");
  lcd.print(lineNames[currentLine]);  // 显示当前线路名称

  lcd.setCursor(0, 1);  // 设置光标到第2行
  lcd.print("Dir: ");
  lcd.print(isEastbound ? "East" : "West");  // 显示当前方向
}

//******------------------------------------------------- LCD -------------------------------------------------******//


//******------------------------------------------------- Servo -------------------------------------------------******//
// 更新舵机和灯条显示
void updateServoAndNeoPixel() {
  // 更新舵机角度（服务质量）
  int targetAngle = serviceQuality[currentLine];
  smoothServoMovement(targetAngle);  // 平滑移动舵机
  Serial.print("Line: ");
  Serial.print(lineNames[currentLine]);
  Serial.print(", Service Quality Angle: ");
  Serial.println(targetAngle);

  // 更新灯条（地铁距离）
  int lights = distances[currentLine];
  for (int i = 0; i < strip.numPixels(); i++) {
    if (i < lights) {
      strip.setPixelColor(i, strip.Color(0, 255, 0));  // 绿色
    } else {
      strip.setPixelColor(i, strip.Color(0, 0, 0));  // 关闭
    }
  }
  strip.show();
  Serial.print("NeoPixel Lights: ");
  Serial.println(lights);
}

// 平滑移动舵机到目标角度
void smoothServoMovement(int targetAngle) {
  if (currentServoAngle != targetAngle) {
    if (currentServoAngle < targetAngle) {
      for (int angle = currentServoAngle; angle <= targetAngle; angle++) {
        myServo.write(angle);
        delay(15);  // 每次移动 15ms
      }
    } else {
      for (int angle = currentServoAngle; angle >= targetAngle; angle--) {
        myServo.write(angle);
        delay(15);  // 每次移动 15ms
      }
    }
    currentServoAngle = targetAngle;  // 更新当前角度
  }
}
//******------------------------------------------------- Servo -------------------------------------------------******//