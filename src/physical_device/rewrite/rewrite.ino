/********* ---------------------------------------------------------------------------------------------------------------------------------------------------------------- ********/
/* Macro definitions of Rotary angle sensor */
#define ROTARY_ANGLE_SENSOR A0
#define ADC_REF 5       // Reference voltage of ADC is 5V
#define FULL_ANGLE 300  // Full value of the rotary angle is 300 degrees
#define GROVE_VCC 5     // VCC of the Grove interface is normally 5V
/* Macro definitions of Button angle sensor */
#define BUTTON_ANGLE_SENSOR A2

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

/********* ---------------------------------------------------------------------------------------------------------------------------------------------------------------- ********/
void setup() {
  Serial.begin(115200);
  // Encoder 
  pinMode(ROTARY_ANGLE_SENSOR, INPUT); // Set the rotary angle sensor pin as input
  //Button
  pinMode(BUTTON_ANGLE_SENSOR, INPUT);

}

void loop() {
  handleUserSelection();
  delay(100); // Short delay to reduce CPU load
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
//2.按下按钮后，地铁方向发生改变 
//3.在LCD上显示消息：当前所选线路，                    
//4.让Servo能够发生变化
//5.让LED灯能够发生变化
//6.能够把MQTT的数据抓下来


//Test
//1. 测试旋钮能不能用
//2. 将旋钮的角度映射在四条线路上，做到能通过调整旋钮来改变线路
//3. 加入按钮功能，按下按钮后地铁方向会发生变化
//4. 在LCD上显示信息：当前所选的线路 --> TMD LCD在哪？？？
//5. 从MQTT上抓取数据

