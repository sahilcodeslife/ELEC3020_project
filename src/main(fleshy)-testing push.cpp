#include <Arduino.h>
#include <TFT_eSPI.h>

#define LEFT_BUTTON 0
#define RIGHT_BUTTON 14

//----------------DEFINING_LINE_SENSORS--------------------
#define SENSOR_LINE_1_PIN 1  // Front
#define SENSOR_LINE_2_PIN 2  // Right
#define SENSOR_LINE_3_PIN 3  // Back right
#define SENSOR_LINE_4_PIN 10 // Back left
#define SENSOR_LINE_5_PIN 11 // Left
//---------------------------------------------------------

//-----------------DEFINING_ULTRASONIC_SENSORS--------------
#define SONIC_OUT_PIN 12
#define SONIC_IN_PIN1 43
#define SONIC_REFRESH_RATE 12000 // max distance ~2m
#define ARENA_DIAMETER 50 // Test arena diameter in cm
//----------------------------------------------------------

//-----------DEFINING_MOTORS (TB6612FNG)-------------------
#define AIN1 18
#define AIN2 17
#define PWMA 44
#define A_LEDC_CHANNEL 0

#define BIN1 21
#define BIN2 16
#define PWMB 13
#define B_LEDC_CHANNEL 1
//----------------------------------------------------------

TFT_eSPI tft = TFT_eSPI();

//----------VARIABLES_FOR_ULTRASONIC_SENSORS----------------
bool USready = true;
int StartTime1;
float dist1;
unsigned long lastTrigger = 0;
//----------------------------------------------------------

//------------VARIABLES_FOR_LINE_SENSORS-------------------
bool SituationChange = false;

bool Line_Sensor_1;
void IRAM_ATTR ChangeSensor1() {
  SituationChange = true;
  Line_Sensor_1 = digitalRead(SENSOR_LINE_1_PIN);
}
bool Line_Sensor_2;
void IRAM_ATTR ChangeSensor2() {
  SituationChange = true;
  Line_Sensor_2 = digitalRead(SENSOR_LINE_2_PIN);
}
bool Line_Sensor_3;
void IRAM_ATTR ChangeSensor3() {
  SituationChange = true;
  Line_Sensor_3 = digitalRead(SENSOR_LINE_3_PIN);
}
bool Line_Sensor_4;
void IRAM_ATTR ChangeSensor4() {
  SituationChange = true;
  Line_Sensor_4 = digitalRead(SENSOR_LINE_4_PIN);
}
bool Line_Sensor_5;
void IRAM_ATTR ChangeSensor5() {
  SituationChange = true;
  Line_Sensor_5 = digitalRead(SENSOR_LINE_5_PIN);
}
//------------------------------------------------------------

//---------------VARIABLES_FOR_MOTORS-------------------------
int AMotorDuty = 0;
int BMotorDuty = 0;
int AMotorDutyCurrent = 0;
int BMotorDutyCurrent = 0;
int ADirection = 0;
int BDirection = 0;
//------------------------------------------------------------

//---------------VARIABLES_FOR_PUSH_MANEUVER------------------
unsigned long pushStartTime = 0;
bool inPushManeuver = false;
#define PUSH_BACK_DURATION 500 // ms to back up straight
#define SWEEP_BACK_DURATION 1500 // ms to sweep back in circle
#define PUSH_THRESHOLD 10 // cm for "being pushed"
//------------------------------------------------------------

//-------------------------DEFINING_FUNCTIONS-----------------
void IRAM_ATTR SonicDistance1();
void USTrigger();
void drawUI(int Line_Sensor_1, int Line_Sensor_2, int Line_Sensor_3, int Line_Sensor_4, int Line_Sensor_5,
            int dist1, int modeA, int modeB, int mode);

void setMotor(int motor, int direction, int duty) {
  if (motor == 0) { // A
    if (direction == 0) { // forwards
      digitalWrite(AIN1, HIGH);
      digitalWrite(AIN2, LOW);
    } else {
      digitalWrite(AIN1, LOW);
      digitalWrite(AIN2, HIGH);
    }
    ledcWrite(A_LEDC_CHANNEL, duty);
  }
  if (motor == 1) { // B
    if (direction == 0) { // forwards
      digitalWrite(BIN1, HIGH);
      digitalWrite(BIN2, LOW);
    } else {
      digitalWrite(BIN1, LOW);
      digitalWrite(BIN2, HIGH);
    }
    ledcWrite(B_LEDC_CHANNEL, duty);
  }
}
//------------------------------------------------------------

void setup() {
  pinMode(15, OUTPUT);
  digitalWrite(15, HIGH);

  Serial.begin(115200);

  pinMode(LEFT_BUTTON, INPUT_PULLUP);
  pinMode(RIGHT_BUTTON, INPUT_PULLUP);

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_WHITE);

  //---------------SETUP_FOR_LINE_SENSORS-------------------------
  Line_Sensor_1 = digitalRead(SENSOR_LINE_1_PIN);
  attachInterrupt(SENSOR_LINE_1_PIN, ChangeSensor1, CHANGE);
  Line_Sensor_2 = digitalRead(SENSOR_LINE_2_PIN);
  attachInterrupt(SENSOR_LINE_2_PIN, ChangeSensor2, CHANGE);
  Line_Sensor_3 = digitalRead(SENSOR_LINE_3_PIN);
  attachInterrupt(SENSOR_LINE_3_PIN, ChangeSensor3, CHANGE);
  Line_Sensor_4 = digitalRead(SENSOR_LINE_4_PIN);
  attachInterrupt(SENSOR_LINE_4_PIN, ChangeSensor4, CHANGE);
  Line_Sensor_5 = digitalRead(SENSOR_LINE_5_PIN);
  attachInterrupt(SENSOR_LINE_5_PIN, ChangeSensor5, CHANGE);
  //-------------------------------------------------------------

  //--------------SETUP_FOR_ULTRASONIC_SENSORS------------------------
  pinMode(SONIC_OUT_PIN, OUTPUT);
  pinMode(SONIC_IN_PIN1, INPUT_PULLUP);
  attachInterrupt(SONIC_IN_PIN1, SonicDistance1, CHANGE);
  //-------------------------------------------------------------

  //-------------------SETUP_FOR_MOTORS (TB6612FNG)-------------------
  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(PWMA, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);
  pinMode(PWMB, OUTPUT);

  ledcSetup(A_LEDC_CHANNEL, 5000, 8); // Lowered to 5kHz for motor reliability
  ledcAttachPin(PWMA, A_LEDC_CHANNEL);
  ledcSetup(B_LEDC_CHANNEL, 20000, 8);
  ledcAttachPin(PWMB, B_LEDC_CHANNEL);
  //-------------------------------------------------------------
}

//-----------------------variables for combat logic------------------------------------
#define LINE_SENSOR_WHITE 0
#define LINE_SENSOR_BLACK 1
#define IDLE_MODE 0
#define ATTACK_MODE 1
#define DEFENCE_MODE 2
#define SEARCH_MODE 3
#define BLIND_MODE 4
#define MOTOR_FORWARDS 0
#define MOTOR_BACKWARDS 1

int mode = IDLE_MODE; // Start in IDLE_MODE
int timeofaction;
bool crisisaverted = true;
//-------------------------------------------------------------------------------------

void loop() {
  // Trigger ultrasonic sensor
  if (USready || (micros() - lastTrigger > 50000)) {
    USready = false;
    lastTrigger = micros();
    USTrigger();
  }

  // Print distance for debugging
  Serial.print("Ultrasonic distance: ");
  Serial.print(dist1);
  Serial.print(" cm, Mode: ");
  Serial.println(mode);

  // Update GUI
  drawUI(Line_Sensor_1, Line_Sensor_2, Line_Sensor_3, Line_Sensor_4, Line_Sensor_5,
         dist1, AMotorDuty, BMotorDuty, mode);

  // Update motors if duty changed
  if (AMotorDutyCurrent != AMotorDuty || BMotorDutyCurrent != BMotorDuty) {
    AMotorDutyCurrent = AMotorDuty;
    BMotorDutyCurrent = BMotorDuty;
    setMotor(0, ADirection, AMotorDuty);
    setMotor(1, BDirection, BMotorDuty);
  }

  // Check for button press to exit IDLE_MODE
  if (mode == IDLE_MODE) {
    AMotorDuty = 0;
    BMotorDuty = 0; // Motors off
    if (digitalRead(LEFT_BUTTON) == LOW) { // Button pressed (INPUT_PULLUP, so LOW = pressed)
      mode = SEARCH_MODE; // Start in SEARCH_MODE
    }
    return; // Skip other logic in IDLE_MODE
  }

  // Normal operation logic
  int dangerlevel = Line_Sensor_1 + Line_Sensor_2 + Line_Sensor_3 + Line_Sensor_4 + Line_Sensor_5;

  // Mode logic
  if (dangerlevel == 0) { // All sensors white, safe
    mode = ATTACK_MODE;
    crisisaverted = false;
  } else if (dangerlevel > 0) { // Any sensor black, edge detected
    mode = DEFENCE_MODE;
  }
  if (dist1 >= 60) { // Opponent far, scaled for small arena
    mode = SEARCH_MODE;
  }
  if (mode == SEARCH_MODE && dangerlevel == 0 && dist1 <= 30) {
    mode = ATTACK_MODE;
  }

  if (mode == ATTACK_MODE) {
    if (dist1 < PUSH_THRESHOLD) { // Being pushed (<10 cm)
      inPushManeuver = true;
      pushStartTime = millis();
    }
    if (inPushManeuver) {
      if (millis() - pushStartTime < PUSH_BACK_DURATION) {
        // Back up straight
        ADirection = MOTOR_BACKWARDS;
        BDirection = MOTOR_BACKWARDS;
        AMotorDuty = 200;
        BMotorDuty = 200;
      } else if (millis() - pushStartTime < PUSH_BACK_DURATION + SWEEP_BACK_DURATION) {
        // Backwards sweeping turn (circular path, e.g., left motor slower)
        ADirection = MOTOR_BACKWARDS;
        BDirection = MOTOR_BACKWARDS;
        AMotorDuty = 200; // Left full
        BMotorDuty = 100; // Right slower for curve
      } else {
        // End maneuver, go to SEARCH_MODE
        inPushManeuver = false;
        mode = SEARCH_MODE;
      }
    } else if (dist1 < 40) {
      ADirection = MOTOR_FORWARDS;
      BDirection = MOTOR_FORWARDS;
      AMotorDuty = 200;
      BMotorDuty = 200;
    }
  } else if (mode == DEFENCE_MODE) {
    if (millis() - timeofaction >= 2000 && crisisaverted == true) {
      crisisaverted = false; // Allow new action after 2s cooldown
    }
    if (!crisisaverted) { // Fixed comparison
      if (Line_Sensor_3 == LINE_SENSOR_BLACK && Line_Sensor_4 == LINE_SENSOR_BLACK) {
        ADirection = MOTOR_BACKWARDS;
        BDirection = MOTOR_BACKWARDS;
        AMotorDuty = 255;
        BMotorDuty = 20; // Pivot turn
        timeofaction = millis();
        crisisaverted = true;
      } else if (Line_Sensor_2 == LINE_SENSOR_BLACK) {
        ADirection = MOTOR_FORWARDS;
        BDirection = MOTOR_FORWARDS;
        BMotorDuty = 200;
        AMotorDuty = 100; // Turn left
        timeofaction = millis();
        crisisaverted = true;
      } else if (Line_Sensor_5 == LINE_SENSOR_BLACK) {
        ADirection = MOTOR_FORWARDS;
        BDirection = MOTOR_FORWARDS;
        BMotorDuty = 100;
        AMotorDuty = 200; // Turn right
        timeofaction = millis();
        crisisaverted = true;
      } else if (Line_Sensor_3 == LINE_SENSOR_BLACK || Line_Sensor_4 == LINE_SENSOR_BLACK) {
        ADirection = MOTOR_FORWARDS;
        BDirection = MOTOR_FORWARDS;
        AMotorDuty = 200;
        BMotorDuty = 200; // Push forward
        timeofaction = millis();
        crisisaverted = true;
      } else if (Line_Sensor_1 == LINE_SENSOR_BLACK) {
        ADirection = MOTOR_BACKWARDS;
        BDirection = MOTOR_BACKWARDS;
        AMotorDuty = 200 - Line_Sensor_2 * 100;
        BMotorDuty = 200 - Line_Sensor_5 * 100; // Variable back turn
        timeofaction = millis();
        crisisaverted = true;
      }
    }
  } else if (mode == SEARCH_MODE) {
    ADirection = MOTOR_BACKWARDS;
    BDirection = MOTOR_FORWARDS;
    AMotorDuty = 80; // Increased for motor reliability
    BMotorDuty = 80;
  }
  // BLIND_MODE remains empty

  // Enforce minimum duty for motor reliability
  if (AMotorDuty > 0 && AMotorDuty < 80) AMotorDuty = 80;
  if (BMotorDuty > 0 && BMotorDuty < 80) BMotorDuty = 80;
}

void USTrigger() {
  digitalWrite(SONIC_OUT_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(SONIC_OUT_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(SONIC_OUT_PIN, LOW);
}

void IRAM_ATTR SonicDistance1() {
  if (digitalRead(SONIC_IN_PIN1) == HIGH) {
    StartTime1 = micros();
  } else {
    int EchoTime1 = micros() - StartTime1;
    dist1 = (EchoTime1 * 0.0343) / 2;
    if (dist1 > ARENA_DIAMETER) {
      dist1 = 60; // Cap to trigger SEARCH_MODE for small arena
    }
    USready = true;
  }
}

//beautiful GUI helper. 
void drawUI(int Line_Sensor_1, int Line_Sensor_2, int Line_Sensor_3, int Line_Sensor_4, int Line_Sensor_5,
            int dist1, int modeA, int modeB, int mode) {
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);

  int y = 20;
  int spacing = 60;
  int r = 12;
  uint16_t active = TFT_BLACK, inactive = TFT_WHITE;

  tft.setTextDatum(MC_DATUM);

  tft.fillCircle(20 + 0*spacing, y, r, Line_Sensor_1 ? active : inactive);
  tft.drawCircle(20+ 0*spacing, y, r, TFT_BLACK);
  tft.setTextColor(Line_Sensor_1 ? TFT_WHITE : TFT_BLACK);
  tft.drawNumber(1, 20 + 0*spacing, y);

  tft.fillCircle(20 + 1*spacing, y, r, Line_Sensor_2 ? active : inactive);
  tft.drawCircle(20+ 1*spacing, y, r, TFT_BLACK);
  tft.setTextColor(Line_Sensor_2 ? TFT_WHITE : TFT_BLACK);
  tft.drawNumber(2, 20 + 1*spacing, y);

  tft.fillCircle(20 + 2*spacing, y, r, Line_Sensor_3 ? active : inactive);
  tft.drawCircle(20+ 2*spacing, y, r, TFT_BLACK);
  tft.setTextColor(Line_Sensor_3 ? TFT_WHITE : TFT_BLACK);
  tft.drawNumber(3, 20 + 2*spacing, y);

  tft.fillCircle(20 + 3*spacing, y, r, Line_Sensor_4 ? active : inactive);
  tft.drawCircle(20+ 3*spacing, y, r, TFT_BLACK);
  tft.setTextColor(Line_Sensor_4 ? TFT_WHITE : TFT_BLACK);
  tft.drawNumber(4, 20 + 3*spacing, y);

  tft.fillCircle(20 + 4*spacing, y, r, Line_Sensor_5 ? active : inactive);
  tft.drawCircle(20+ 4*spacing, y, r, TFT_BLACK);
  tft.setTextColor(Line_Sensor_5 ? TFT_WHITE : TFT_BLACK);
  tft.drawNumber(5, 20 + 4*spacing, y);

  tft.setCursor(10, y + 20);
  

  int maxDist = 100;
  int barX = 20;
  int barY = 60;
  int barW = 200;
  int barH = 40;

  int fillW = map(constrain(dist1, 0, maxDist), maxDist, 0, 0, barW);

  tft.drawRect(barX, barY, barW, barH, TFT_BLACK);
  tft.fillRect(barX, barY, fillW, barH, TFT_RED);

  tft.setTextDatum(MC_DATUM);
  tft.setTextFont(4);
  tft.setTextSize(1);

  uint16_t textColor = (fillW > barW / 2) ? TFT_WHITE : TFT_BLACK;
  tft.setTextColor(textColor, TFT_WHITE);

  char buf[16];
  snprintf(buf, sizeof(buf), "%icm", (int)dist1);
  tft.drawString(buf, barX + barW / 2, barY + barH / 2);

  int boxW = 60, boxH = 30;

  tft.fillRoundRect(20, 120, boxW, boxH, 5, (modeA) ? TFT_GREEN : TFT_DARKGREY);
  tft.setCursor(30, 130);
  tft.setTextColor(TFT_BLACK);
  tft.printf("A:%i", modeA);

  tft.fillRoundRect(100, 120, boxW, boxH, 5, (modeB) ? TFT_GREEN : TFT_DARKGREY);
  tft.setCursor(110, 130);
  tft.setTextColor(TFT_BLACK);
  tft.printf("B:%i", modeB);

  // Display current mode
  tft.setTextDatum(TL_DATUM);
  tft.setTextFont(4);
  tft.setTextSize(1);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  // Clear previous mode text to avoid overlap
  tft.fillRect(10, 160, 200, 20, TFT_WHITE);
  const char* modeStr;
  switch (mode) {
    case IDLE_MODE: modeStr = "IDLE"; break;
    case ATTACK_MODE: modeStr = "ATTACK"; break;
    case DEFENCE_MODE: modeStr = "DEFENCE"; break;
    case SEARCH_MODE: modeStr = "SEARCH"; break;
    case BLIND_MODE: modeStr = "BLIND"; break;
    default: modeStr = "UNKNOWN"; break;
  }
  tft.setCursor(160, 100);
  tft.print("Mode: ");
  tft.print(modeStr);
}