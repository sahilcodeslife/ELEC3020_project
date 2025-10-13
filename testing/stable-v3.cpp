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
#define ARENA_DIAMETER 55 // Test arena diameter in cm
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
int BMotorDuty = 0; // UNDERPOWERED add 20 to offset
int AMotorDutyCurrent = 0;
int BMotorDutyCurrent = 0;
int ADirection = 0;
int BDirection = 0;
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
  ledcSetup(B_LEDC_CHANNEL, 5000, 8);
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
#define FULL_SPIN_MODE 6  // Mode for 360-degree spin after sway search fails
#define MOTOR_FORWARDS 0
#define MOTOR_BACKWARDS 1

#define ATTACK_THRESHOLD 50 // Lowered from 40 to require better alignment (centering)
#define ATTACK_CHECK_INTERVAL 2000
#define SCAN_TURN_DURATION 450  // ms for each small turn (tune for ~15-20 degree sweep)
#define SCAN_PAUSE_DURATION 100  // ms pause to allow US sensor to update
#define MAX_SWAY_CYCLES 1  // Number of left/right oscillations in SEARCH_MODE before switching to FULL_SPIN_MODE

int mode = IDLE_MODE; // Start in IDLE_MODE
int timeofaction;
unsigned long lastAttackCheck = 0; // Timer for periodic distance checks in ATTACK_MODE
bool crisisaverted = true;

unsigned long swayStartTime = 0;  // Timer for sway turns in SEARCH_MODE
int swayDirection = 0;  // 0 = left, 1 = right for SEARCH_MODE
int swayCycleCount = 0;  // Track oscillations in SEARCH_MODE
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
      swayStartTime = millis(); // Initialize sway timer
      swayDirection = 0; // Start with left turn
      swayCycleCount = 0; // Reset sway counter
    }
    return; // Skip other logic in IDLE_MODE
  }

  // Normal operation logic
  int previous_mode = mode;  // Track for transition
  int dangerlevel = Line_Sensor_1 + Line_Sensor_2 + Line_Sensor_3 + Line_Sensor_4 + Line_Sensor_5;

  // Mode logic with DEFENCE_MODE as highest priority
  if (dangerlevel > 0) { // Any sensor black, edge detected
    mode = DEFENCE_MODE;
  } else if (dist1 >= ARENA_DIAMETER) { // Opponent far, scaled for small arena
    mode = SEARCH_MODE;
    swayStartTime = millis(); // Reset sway timer on transition
    swayDirection = 0;
    swayCycleCount = 0;
  } else if (dist1 <= ATTACK_THRESHOLD) { // Only enter ATTACK_MODE if no black lines and opponent close
    mode = ATTACK_MODE;
    crisisaverted = false;
    lastAttackCheck = millis();  // Reset timer when entering ATTACK_MODE
  } else {
    mode = SEARCH_MODE; // Default to SEARCH_MODE if no black lines and opponent not close enough
  }

  if (mode == ATTACK_MODE) {
    if (dist1 < ATTACK_THRESHOLD) {
      ADirection = MOTOR_FORWARDS;
      BDirection = MOTOR_FORWARDS;
      AMotorDuty = 230; 
      BMotorDuty = 250;
    }
  } else if (mode == DEFENCE_MODE) {
    if (millis() - timeofaction >= 1000 && crisisaverted == true) {
      crisisaverted = false; // Allow new action after 1s cooldown
    }
    if (!crisisaverted) {
      if (Line_Sensor_3 == LINE_SENSOR_BLACK && Line_Sensor_4 == LINE_SENSOR_BLACK) {
        // Both back sensors detect edge: forward
        ADirection = MOTOR_FORWARDS;
        BDirection = MOTOR_FORWARDS;
        AMotorDuty = 255;
        BMotorDuty = 255;
        timeofaction = millis();
        crisisaverted = true;
      } else if (Line_Sensor_1 == LINE_SENSOR_BLACK) {
        // Front, right, and left sensors detect edge: Sharp backwards
        ADirection = MOTOR_BACKWARDS;
        BDirection = MOTOR_BACKWARDS;
        AMotorDuty = 255;
        BMotorDuty = 255;
        timeofaction = millis();
        crisisaverted = true;
      } else if (Line_Sensor_1 == LINE_SENSOR_BLACK && Line_Sensor_2 == LINE_SENSOR_BLACK) {
        // Front and right sensors detect edge: Back left turn
        ADirection = MOTOR_BACKWARDS;
        BDirection = MOTOR_BACKWARDS;
        AMotorDuty = 250;
        BMotorDuty = 170;
        timeofaction = millis();
        crisisaverted = true;
      } else if (Line_Sensor_1 == LINE_SENSOR_BLACK && Line_Sensor_5 == LINE_SENSOR_BLACK) {
        // Front and left sensors detect edge: Back right turn
        ADirection = MOTOR_BACKWARDS;
        BDirection = MOTOR_BACKWARDS;
        AMotorDuty = 170;
        BMotorDuty = 250;
        timeofaction = millis();
        crisisaverted = true;
      } else if (Line_Sensor_2 == LINE_SENSOR_BLACK) {
        // Right sensor detects edge: Turn left
        ADirection = MOTOR_FORWARDS;
        BDirection = MOTOR_FORWARDS;
        BMotorDuty = 250;
        AMotorDuty = 170;
        timeofaction = millis();
        crisisaverted = true;
      } else if (Line_Sensor_5 == LINE_SENSOR_BLACK) {
        // Left sensor detects edge: Turn right
        ADirection = MOTOR_FORWARDS;
        BDirection = MOTOR_FORWARDS;
        BMotorDuty = 170;
        AMotorDuty = 250;
        timeofaction = millis();
        crisisaverted = true;
      } else if (Line_Sensor_3 == LINE_SENSOR_BLACK || Line_Sensor_4 == LINE_SENSOR_BLACK) {
        // Either back sensor detects edge: Push forward
        ADirection = MOTOR_FORWARDS;
        BDirection = MOTOR_FORWARDS;
        AMotorDuty = 250;
        BMotorDuty = 250;
        timeofaction = millis();
        crisisaverted = true;
      } //else if (Line_Sensor_1 == LINE_SENSOR_BLACK) {
        // Front sensor detects edge: Variable back turn
    //     ADirection = MOTOR_BACKWARDS;
    //     BDirection = MOTOR_BACKWARDS;
    //     AMotorDuty = 200 - Line_Sensor_2 * 100;
    //     BMotorDuty = 200 - Line_Sensor_5 * 100;
    //     timeofaction = millis();
    //     crisisaverted = true;
    //   }
    }
  } else if (mode == SEARCH_MODE) {
    // Sway search: small left/right turns with pauses
    unsigned long currentTime = millis();
    
    if (dist1 <= ATTACK_THRESHOLD) {
      // Opponent detected during sway: snap to ATTACK_MODE
      mode = ATTACK_MODE;
      lastAttackCheck = millis();  // Reset timer
      swayCycleCount = 0;
    } else if (swayCycleCount >= MAX_SWAY_CYCLES * 2) {  // After full cycles, switch to FULL_SPIN_MODE
      mode = FULL_SPIN_MODE;
      swayCycleCount = 0;
    } else {
      // Handle the turn/pause cycle
      int phase = (currentTime - swayStartTime) / (SCAN_TURN_DURATION + SCAN_PAUSE_DURATION);
      if (phase % 2 == 0) {  // Turn phase
        if (swayDirection == 0) {  // Left turn: left motor back, right forward
          ADirection = MOTOR_BACKWARDS;
          BDirection = MOTOR_FORWARDS;
        } else {  // Right turn: left forward, right back
          ADirection = MOTOR_FORWARDS;
          BDirection = MOTOR_BACKWARDS;
        }
        AMotorDuty = 200;
        BMotorDuty = 230;
      } else {
        AMotorDuty =0;
        BMotorDuty = 0;
      }
      
      // Advance to next direction after full turn+pause
      if (currentTime - swayStartTime >= SCAN_TURN_DURATION + SCAN_PAUSE_DURATION) {
        swayStartTime = currentTime;
        swayDirection = 1 - swayDirection;  // Toggle 0<->1
        swayCycleCount++;
      }
    }
  } else if (mode == FULL_SPIN_MODE) {
    // Full 360-degree spin
    ADirection = MOTOR_BACKWARDS;
    BDirection = MOTOR_FORWARDS;
    AMotorDuty = 200;
    BMotorDuty = 230;
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
      dist1 = ARENA_DIAMETER; // Cap to trigger SEARCH_MODE for arena
    }
    USready = true;
  }
}

void drawUI(int Line_Sensor_1, int Line_Sensor_2, int Line_Sensor_3, int Line_Sensor_4, int Line_Sensor_5,
            int dist1, int modeA, int modeB, int mode) {
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);

  int y = 20;
  int spacing = 60;
  int r = 12;
  uint16_t active = TFT_BLACK, inactive = TFT_WHITE;

  tft.setTextDatum(MC_DATUM);

  tft.fillCircle(20 + 0 * spacing, y, r, Line_Sensor_1 ? active : inactive);
  tft.drawCircle(20 + 0 * spacing, y, r, TFT_BLACK);
  tft.setTextColor(Line_Sensor_1 ? TFT_WHITE : TFT_BLACK);
  tft.drawNumber(1, 20 + 0 * spacing, y);

  tft.fillCircle(20 + 1 * spacing, y, r, Line_Sensor_2 ? active : inactive);
  tft.drawCircle(20 + 1 * spacing, y, r, TFT_BLACK);
  tft.setTextColor(Line_Sensor_2 ? TFT_WHITE : TFT_BLACK);
  tft.drawNumber(2, 20 + 1 * spacing, y);

  tft.fillCircle(20 + 2 * spacing, y, r, Line_Sensor_3 ? active : inactive);
  tft.drawCircle(20 + 2 * spacing, y, r, TFT_BLACK);
  tft.setTextColor(Line_Sensor_3 ? TFT_WHITE : TFT_BLACK);
  tft.drawNumber(3, 20 + 2 * spacing, y);

  tft.fillCircle(20 + 3 * spacing, y, r, Line_Sensor_4 ? active : inactive);
  tft.drawCircle(20 + 3 * spacing, y, r, TFT_BLACK);
  tft.setTextColor(Line_Sensor_4 ? TFT_WHITE : TFT_BLACK);
  tft.drawNumber(4, 20 + 3 * spacing, y);

  tft.fillCircle(20 + 4 * spacing, y, r, Line_Sensor_5 ? active : inactive);
  tft.drawCircle(20 + 4 * spacing, y, r, TFT_BLACK);
  tft.setTextColor(Line_Sensor_5 ? TFT_WHITE : TFT_BLACK);
  tft.drawNumber(5, 20 + 4 * spacing, y);

  tft.setCursor(10, y + 20);
  tft.print("Line Sensors");

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
  tft.fillRect(160, 100, 200, 20, TFT_WHITE);
  const char* modeStr;
  switch (mode) {
    case IDLE_MODE: modeStr = "IDLE"; break;
    case ATTACK_MODE: modeStr = "ATTACK"; break;
    case DEFENCE_MODE: modeStr = "DEFENCE"; break;
    case SEARCH_MODE: modeStr = "SEARCH"; break;
    case BLIND_MODE: modeStr = "BLIND"; break;
    case FULL_SPIN_MODE: modeStr = "SPIN"; break;
    default: modeStr = "UNKNOWN"; break;
  }
  tft.setCursor(160, 100);
  tft.print("Mode: ");
  tft.print(modeStr);
}