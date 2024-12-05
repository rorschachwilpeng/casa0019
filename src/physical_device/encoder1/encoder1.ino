#include <LiquidCrystal_I2C.h>
#include <Encoder.h>

// 初始化LCD，设置地址为0x27
LiquidCrystal_I2C lcd(0x27, 16, 2);

// 定义编码器的引脚
Encoder myEncoder(2, 3);
long oldPosition = 0; // 从0开始计数
long newPosition;

// 地铁线数据
const char* lines[] = {"DLR(in)", "Jubilee(in)", "Mildmay(in)", "Elizabeth(in)", "Central(in)"};
const char* boolValues[] = {"DLR(out)", "Jubilee(out)", "Mildmay(out)", "Elizabeth(out)", "Central(out)"};

// 地铁线索引
int lineIndex = 0; // 从0开始索引

// 地铁线状态
bool lineStatus = false;

// 按钮引脚
const int buttonPin = 4;

// 初始化LCD和串口
void setup() {
  Serial.begin(9600); // 初始化串口通信
  lcd.begin(16, 2); // 初始化LCD，设置列数和行数
  lcd.backlight();
  pinMode(buttonPin, INPUT_PULLUP); // 编码器按钮
  attachInterrupt(digitalPinToInterrupt(buttonPin), updateLineStatus, FALLING);
}

// 更新地铁线状态
void updateLineStatus() {
  static unsigned long lastButtonTime = 0;
  unsigned long currentMillis = millis();
  if (currentMillis - lastButtonTime > 50) { // 防抖动
    lineStatus = !lineStatus;
    Serial.println("Changed");
    lastButtonTime = currentMillis;
    displayLine();
  }
}

// 主循环
void loop() {
  newPosition = myEncoder.read();
  if (newPosition != oldPosition) {
    oldPosition = newPosition;
    lineIndex = newPosition % 20; // 确保索引在0-19之间
    Serial.print("New index: ");
    Serial.println(lineIndex); // 打印当前索引值
    displayLine();
  }
}

// 显示地铁线信息
void displayLine() {
  if (lineIndex >= 2 && lineIndex <= 4) {
    lcd.setCursor(0, 0);
    lcd.print(lineStatus ? boolValues[(lineIndex - 2) / 2] : lines[(lineIndex - 2) / 2]);
  } else if (lineIndex >= 5 && lineIndex <= 7) {
    lcd.setCursor(0, 0);
    lcd.print(lineStatus ? boolValues[1] : lines[1]);
  } else if (lineIndex >= 13 && lineIndex <= 15) {
    lcd.setCursor(0, 0);
    lcd.print(lineStatus ? boolValues[2] : lines[2]);
  } else if (lineIndex >= 16 && lineIndex <= 18) {
    lcd.setCursor(0, 0);
    lcd.print(lineStatus ? boolValues[3] : lines[3]);
  } else if (lineIndex == 19 || lineIndex == 20 || lineIndex == 1) {
    lcd.setCursor(0, 0);
    lcd.print(lineStatus ? boolValues[4] : lines[4]);
  } else {
    lcd.clear();
  }
}