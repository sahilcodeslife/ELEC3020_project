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
#define MAX_DISTANCE 60
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
unsigned long StartTime1;
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
  //backlight on ttgo
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
#define MOTOR_FORWARDS 0
#define MOTOR_BACKWARDS 1

#define ATTACK_THRESHOLD 55 // Distance to trigger ATTACK_MODE (cm)
#define SEARCH_TURN_DURATION 400 // ms for each turn (~15-20 degree sweep, tune as needed)
#define SEARCH_PAUSE_DURATION 100 // ms pause to allow US sensor to update
#define SEARCH_BRAKE_DURATION 50 // ms for brief braking before pause
#define SEARCH_FULL_SPIN_DURATION 900 // ms for full 360-degree spin (tune based on bot)
#define SPIN_DURATION 450  // ms for 180-degree turn in DEFENCE_MODE (tune based on bot)
#define ESCAPE_DURATION 800

// Search mode states
#define SEARCH_LEFT_TURN 0
#define SEARCH_RIGHT_TURN 1
#define SEARCH_PAUSE 2
#define SEARCH_BRAKE 3
#define SEARCH_FULL_SPIN 4

int mode = IDLE_MODE; // Start in IDLE_MODE
unsigned long lastAttackCheck = 0; // Timer for periodic distance checks in ATTACK_MODE
unsigned long searchStateStartTime = 0; // Timer for search state transitions
int searchState = SEARCH_PAUSE; // Start with pause to ensure stop
int lastSearchTurnState = SEARCH_RIGHT_TURN; // Initialize to ensure first turn is left
bool swayCycleComplete = false; // Track if both left and right turns are done
unsigned long defenceStartTime = 0; // Timer for non-blocking defense actions
int defenceSubMode = 0; // 0: idle, 1: spin, 2: escape forward, 3: stop/brake
//-------------------------------------------------------------------------------------

void loop() {
  // Trigger ultrasonic sensor
  if (USready || (micros() - lastTrigger > 100000)) {
    USready = false;
    lastTrigger = micros();
    USTrigger();
  }

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
      searchStateStartTime = millis(); // Initialize search timer
      searchState = SEARCH_PAUSE; // Start with pause to ensure stop
      lastSearchTurnState = SEARCH_RIGHT_TURN; // Ensure first turn is left
      swayCycleComplete = false; // Reset sway cycle
      AMotorDuty = 0; // Stop motors on entry
      BMotorDuty = 0;
    }
    return; // Skip other logic in IDLE_MODE
  }

  // Normal operation logic
  int previous_mode = mode; // Track for transition
  int dangerlevel = Line_Sensor_1 + Line_Sensor_2 + Line_Sensor_3 + Line_Sensor_4 + Line_Sensor_5;

  // Mode logic with DEFENCE_MODE as highest priority
  if (dangerlevel > 0 || defenceSubMode != 0 || (previous_mode == DEFENCE_MODE 
      && millis() - defenceStartTime < (200 + SPIN_DURATION + ESCAPE_DURATION))) {
    mode = DEFENCE_MODE;
  } else if (dist1 >= 60) { // Opponent far
    if (mode != SEARCH_MODE) { // Transition to SEARCH_MODE
      mode = SEARCH_MODE;
      searchStateStartTime = millis(); // Reset search timer
      searchState = SEARCH_PAUSE; // Start with pause to ensure stop
      lastSearchTurnState = SEARCH_RIGHT_TURN; // Ensure first turn is left
      swayCycleComplete = false; // Reset sway cycle
      AMotorDuty = 0; // Stop motors on entry
      BMotorDuty = 0;
    }
  } else if (dist1 < ATTACK_THRESHOLD) { // Opponent close
    mode = ATTACK_MODE;
    lastAttackCheck = millis(); // Reset timer when entering ATTACK_MODE
  } else {
    if (mode != SEARCH_MODE) { // Transition to SEARCH_MODE
      mode = SEARCH_MODE;
      searchStateStartTime = millis(); // Reset search timer
      searchState = SEARCH_PAUSE; // Start with pause to ensure stop
      lastSearchTurnState = SEARCH_RIGHT_TURN; // Ensure first turn is left
      swayCycleComplete = false; // Reset sway cycle
      AMotorDuty = 0; // Stop motors on entry
      BMotorDuty = 0;
    }
  }

  // Mode-specific actions
  if (mode == ATTACK_MODE) {
    if (dist1 < ATTACK_THRESHOLD) {

      ADirection = MOTOR_FORWARDS;
      BDirection = MOTOR_FORWARDS;
      AMotorDuty = 230;
      BMotorDuty = 255;
    }
  } else if (mode == DEFENCE_MODE) {
    unsigned long currentTime = millis();

    // Prioritize front sensor: brief stop/brake, then 180 spin, then forward
    if (Line_Sensor_1 == LINE_SENSOR_BLACK || defenceSubMode != 0) {
      if (defenceSubMode == 0) { // Start action
        defenceSubMode = 3; // Stop/brake phase
        defenceStartTime = currentTime;
        ADirection = MOTOR_BACKWARDS;
        BDirection = MOTOR_BACKWARDS;
        AMotorDuty = 150;
        BMotorDuty = 150;
      } else if (defenceSubMode == 3 && currentTime - defenceStartTime >= 200) {
        defenceSubMode = 1; // Spin phase
        defenceStartTime = currentTime;
        ADirection = MOTOR_FORWARDS;
        BDirection = MOTOR_BACKWARDS;
        AMotorDuty = 255;
        BMotorDuty = 255;
      } else if (defenceSubMode == 1 && currentTime - defenceStartTime >= SPIN_DURATION) {
        defenceSubMode = 2; // Escape forward phase
        defenceStartTime = currentTime;
        ADirection = MOTOR_FORWARDS;
        BDirection = MOTOR_FORWARDS;
        AMotorDuty = 255;
        BMotorDuty = 255;
      } else if (defenceSubMode == 2 && currentTime - defenceStartTime >= ESCAPE_DURATION) {
        defenceSubMode = 0; // Action complete
        AMotorDuty = 0;
        BMotorDuty = 0;
      }
    }
    // Sides: Differential turns
    else if (Line_Sensor_5 == LINE_SENSOR_BLACK) {
      defenceSubMode = 0;
      ADirection = MOTOR_FORWARDS;
      BDirection = MOTOR_FORWARDS;
      AMotorDuty = 250;
      BMotorDuty = 150;
    }
    else if (Line_Sensor_2 == LINE_SENSOR_BLACK) {
      defenceSubMode = 0;
      ADirection = MOTOR_FORWARDS;
      BDirection = MOTOR_FORWARDS;
      AMotorDuty = 150;
      BMotorDuty = 250;
    }
    // Backs: Push forward
    else if (Line_Sensor_3 == LINE_SENSOR_BLACK || Line_Sensor_4 == LINE_SENSOR_BLACK) {
      defenceSubMode = 0;
      ADirection = MOTOR_FORWARDS;
      BDirection = MOTOR_FORWARDS;
      AMotorDuty = 250;
      BMotorDuty = 250;
    }
    // Default: Stop if no sensors triggered
    else {
      defenceSubMode = 0;
      AMotorDuty = 0;
      BMotorDuty = 0;
    }
  } else if (mode == SEARCH_MODE) {
    unsigned long currentTime = millis();

    // State machine for search: PAUSE -> LEFT_TURN -> BRAKE -> PAUSE -> RIGHT_TURN -> BRAKE -> FULL_SPIN
    if (dist1 < ATTACK_THRESHOLD) {
      // Opponent detected: switch to ATTACK_MODE
      mode = ATTACK_MODE;
      lastAttackCheck = millis();
      swayCycleComplete = false; // Reset sway cycle
    } else {
      if (searchState == SEARCH_LEFT_TURN) {
        ADirection = MOTOR_BACKWARDS; // Left turn: left motor back, right forward
        BDirection = MOTOR_FORWARDS;
        AMotorDuty = 200; // Balanced for smaller turns
        BMotorDuty = 240;
        if (currentTime - searchStateStartTime >= SEARCH_TURN_DURATION) {
          searchState = SEARCH_BRAKE;
          lastSearchTurnState = SEARCH_LEFT_TURN; // Track last turn
          searchStateStartTime = currentTime;
        }
      } else if (searchState == SEARCH_RIGHT_TURN) {
        ADirection = MOTOR_FORWARDS; // Right turn: left forward, right back
        BDirection = MOTOR_BACKWARDS;
        AMotorDuty = 150; // Balanced for smaller turns
        BMotorDuty = 150;
        if (currentTime - searchStateStartTime >= SEARCH_TURN_DURATION) {
          searchState = SEARCH_BRAKE;
          lastSearchTurnState = SEARCH_RIGHT_TURN; // Track last turn
          swayCycleComplete = true; // Mark sway cycle complete
          searchStateStartTime = currentTime;
        }
      } else if (searchState == SEARCH_BRAKE) {
        ADirection = MOTOR_BACKWARDS; // Brief brake: both motors back
        BDirection = MOTOR_BACKWARDS;
        AMotorDuty = 100; // Low duty for gentle braking
        BMotorDuty = 100;
        if (currentTime - searchStateStartTime >= SEARCH_BRAKE_DURATION) {
          searchState = (swayCycleComplete && lastSearchTurnState == SEARCH_RIGHT_TURN) 
                        ? SEARCH_FULL_SPIN : SEARCH_PAUSE;
          searchStateStartTime = currentTime;
        }
      } else if (searchState == SEARCH_PAUSE) {
        AMotorDuty = 0;
        BMotorDuty = 0;
        if (currentTime - searchStateStartTime >= SEARCH_PAUSE_DURATION) {
          // Toggle between left and right turns based on last turn
          searchState = (lastSearchTurnState == SEARCH_LEFT_TURN) ? SEARCH_RIGHT_TURN : SEARCH_LEFT_TURN;
          searchStateStartTime = currentTime;
        }
      } else if (searchState == SEARCH_FULL_SPIN) {
        ADirection = MOTOR_FORWARDS; // Full 360-degree spin: left forward, right back
        BDirection = MOTOR_BACKWARDS;
        AMotorDuty = 255; // High duty for fast spin
        BMotorDuty = 255;
        if (currentTime - searchStateStartTime >= SEARCH_FULL_SPIN_DURATION) {
          searchState = SEARCH_PAUSE;
          swayCycleComplete = false; // Reset for next sway cycle
          searchStateStartTime = currentTime;
        }
      }
    }
  } // BLIND_MODE remains empty

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
    if (dist1 > MAX_DISTANCE) {
      dist1 = MAX_DISTANCE; // Cap distance at 60 cm
    }
    USready = true;
  }
}

void drawUI(int Line_Sensor_1, int Line_Sensor_2, int Line_Sensor_3, int Line_Sensor_4, int Line_Sensor_5, int dist1, int modeA, int modeB, int mode) {
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
    default: modeStr = "UNKNOWN"; break;
  }
  tft.setCursor(160, 100);
  tft.print("Mode: ");
  tft.print(modeStr);
}