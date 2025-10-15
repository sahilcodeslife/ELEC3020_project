//DO NOT USE, search is horrible. reverting. 

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

#define BIN1 21 // SHIT MOTOR ALWAYS OVERPOWER. 
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
  pinMode(15, OUTPUT);
  digitalWrite(15, HIGH);

  Serial.begin(115200);

  pinMode(LEFT_BUTTON, INPUT_PULLUP);
  pinMode(RIGHT_BUTTON, INPUT_PULLUP);

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_WHITE);

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

  pinMode(SONIC_OUT_PIN, OUTPUT);
  pinMode(SONIC_IN_PIN1, INPUT_PULLUP);
  attachInterrupt(SONIC_IN_PIN1, SonicDistance1, CHANGE);

  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(PWMA, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);
  pinMode(PWMB, OUTPUT);

  ledcSetup(A_LEDC_CHANNEL, 5000, 8);
  ledcAttachPin(PWMA, A_LEDC_CHANNEL);
  ledcSetup(B_LEDC_CHANNEL, 5000, 8);
  ledcAttachPin(PWMB, B_LEDC_CHANNEL);
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
#define SEARCH_TURN_DURATION 400 // ms for each turn (~15-20 degree sweep)
#define SEARCH_PAUSE_DURATION 100 // ms pause to allow US sensor to update
#define SEARCH_BRAKE_DURATION 50 // ms for brief braking before pause
#define SEARCH_FULL_SPIN_DURATION 900 // ms for full 360-degree spin
#define SPIN_DURATION 450 // ms for 180-degree turn in DEFENCE_MODE
#define ESCAPE_DURATION 800
#define ADJUST_DURATION 400 // ms for brief slight turn adjustment
#define ALIGN_TURN_DURATION 100 // ms for brief left/right turn (~5-10 degrees)
#define ALIGN_PAUSE_DURATION 100 // ms pause to measure distance
#define FINAL_TURN_DURATION 300 // ms for on-spot turn toward opponent (~30-45 degrees)

#define ADJUST_LEFT 0
#define ADJUST_RIGHT 1

#define ATTACK_ALIGN_LEFT 1
#define ATTACK_PAUSE_LEFT 2
#define ATTACK_ALIGN_RIGHT 3
#define ATTACK_PAUSE_RIGHT 4
#define ATTACK_TURN_FINAL 5

#define SEARCH_LEFT_TURN 0
#define SEARCH_RIGHT_TURN 1
#define SEARCH_PAUSE 2
#define SEARCH_BRAKE 3
#define SEARCH_FULL_SPIN 4


int mode = IDLE_MODE;
unsigned long lastAttackCheck = 0;
unsigned long searchStateStartTime = 0;
int searchState = SEARCH_PAUSE;
int lastSearchTurnState = SEARCH_RIGHT_TURN;
bool swayCycleComplete = false;
unsigned long defenceStartTime = 0;
int defenceSubMode = 0;

unsigned long attackStartTime = 0;
int lastTurnDirection = 0;
int attackSubState = 0;
unsigned long attackSubStartTime = 0;
float dist_left = MAX_DISTANCE;
float dist_right = MAX_DISTANCE;
//-------------------------------------------------------------------------------------

void loop() {

    if (digitalRead(RIGHT_BUTTON) == LOW){
        ESP.restart();
    }
  if (USready || (micros() - lastTrigger > 100000)) {
    USready = false;
    lastTrigger = micros();
    USTrigger();
  }

  drawUI(Line_Sensor_1, Line_Sensor_2, Line_Sensor_3, Line_Sensor_4, Line_Sensor_5,
         dist1, AMotorDuty, BMotorDuty, mode);

  if (AMotorDutyCurrent != AMotorDuty || BMotorDutyCurrent != BMotorDuty) {
    AMotorDutyCurrent = AMotorDuty;
    BMotorDutyCurrent = BMotorDuty;
    setMotor(0, ADirection, AMotorDuty);
    setMotor(1, BDirection, BMotorDuty);
  }

  if (mode == IDLE_MODE) {
    AMotorDuty = 0;
    BMotorDuty = 0;
    if (digitalRead(LEFT_BUTTON) == LOW) {
      mode = SEARCH_MODE;
      searchStateStartTime = millis();
      searchState = SEARCH_PAUSE;
      lastSearchTurnState = SEARCH_RIGHT_TURN;
      swayCycleComplete = false;
      AMotorDuty = 0;
      BMotorDuty = 0;
    }
    return;
  }

  int previous_mode = mode;
  int dangerlevel = Line_Sensor_1 + Line_Sensor_2 + Line_Sensor_3 + Line_Sensor_4 + Line_Sensor_5;

  if (dangerlevel > 0 || defenceSubMode != 0 || (previous_mode == DEFENCE_MODE 
      && millis() - defenceStartTime < (200 + SPIN_DURATION + ESCAPE_DURATION))) {
    mode = DEFENCE_MODE;
  } else if (dist1 >= 60) {
    if (mode != SEARCH_MODE) {
      mode = SEARCH_MODE;
      searchStateStartTime = millis();
      searchState = SEARCH_PAUSE;
      lastSearchTurnState = SEARCH_RIGHT_TURN;
      swayCycleComplete = false;
      AMotorDuty = 0;
      BMotorDuty = 0;
    }
  } else if (dist1 < ATTACK_THRESHOLD) {
    if (mode != ATTACK_MODE) {
      lastAttackCheck = millis();
      attackStartTime = millis();
      if (mode == SEARCH_MODE) {
        if (searchState == SEARCH_LEFT_TURN) {
          lastTurnDirection = ADJUST_LEFT;
        } else if (searchState == SEARCH_RIGHT_TURN) {
          lastTurnDirection = ADJUST_RIGHT;
        } else {
          lastTurnDirection = (lastSearchTurnState == SEARCH_LEFT_TURN) ? ADJUST_LEFT : ADJUST_RIGHT;
        }
      } else {
        lastTurnDirection = 0;
      }
      attackSubState = ATTACK_ALIGN_LEFT;
      attackSubStartTime = millis();
      dist_left = MAX_DISTANCE;
      dist_right = MAX_DISTANCE;
    }
    mode = ATTACK_MODE;
  } else {
    if (mode != SEARCH_MODE) {
      mode = SEARCH_MODE;
      searchStateStartTime = millis();
      searchState = SEARCH_PAUSE;
      lastSearchTurnState = SEARCH_RIGHT_TURN;
      swayCycleComplete = false;
      AMotorDuty = 0;
      BMotorDuty = 0;
    }
  }

  if (mode == ATTACK_MODE) {
    unsigned long currentTime = millis();
    if (dist1 >= ATTACK_THRESHOLD) {
      mode = SEARCH_MODE;
      searchStateStartTime = currentTime;
      searchState = SEARCH_PAUSE;
      lastSearchTurnState = SEARCH_RIGHT_TURN;
      swayCycleComplete = false;
      AMotorDuty = 0;
      BMotorDuty = 0;
      attackSubState = 0;
    } else if (attackSubState > 0) {
      switch (attackSubState) {
        case ATTACK_ALIGN_LEFT:
          ADirection = MOTOR_BACKWARDS;
          BDirection = MOTOR_FORWARDS;
          AMotorDuty = 100;
          BMotorDuty = 100;
          if (currentTime - attackSubStartTime >= ALIGN_TURN_DURATION) {
            attackSubState = ATTACK_PAUSE_LEFT;
            attackSubStartTime = currentTime;
            AMotorDuty = 0;
            BMotorDuty = 0;
          }
          break;
        case ATTACK_PAUSE_LEFT:
          AMotorDuty = 0;
          BMotorDuty = 0;
          if (currentTime - attackSubStartTime >= ALIGN_PAUSE_DURATION) {
            dist_left = dist1;
            attackSubState = ATTACK_ALIGN_RIGHT;
            attackSubStartTime = currentTime;
          }
          break;
        case ATTACK_ALIGN_RIGHT:
          ADirection = MOTOR_FORWARDS;
          BDirection = MOTOR_BACKWARDS;
          AMotorDuty = 100;
          BMotorDuty = 100;
          if (currentTime - attackSubStartTime >= 2 * ALIGN_TURN_DURATION) {
            attackSubState = ATTACK_PAUSE_RIGHT;
            attackSubStartTime = currentTime;
            AMotorDuty = 0;
            BMotorDuty = 0;
          }
          break;
        case ATTACK_PAUSE_RIGHT:
          AMotorDuty = 0;
          BMotorDuty = 0;
          if (currentTime - attackSubStartTime >= ALIGN_PAUSE_DURATION) {
            dist_right = dist1;
            attackSubState = ATTACK_TURN_FINAL;
            attackSubStartTime = currentTime;
            if (dist_left < dist_right) {
              ADirection = MOTOR_BACKWARDS; // Turn left
              BDirection = MOTOR_FORWARDS;
            } else {
              ADirection = MOTOR_FORWARDS; // Turn right
              BDirection = MOTOR_BACKWARDS;
            }
            AMotorDuty = 150;
            BMotorDuty = 150;
          }
          break;
        case ATTACK_TURN_FINAL:
          if (currentTime - attackSubStartTime >= FINAL_TURN_DURATION) {
            attackSubState = 0;
            attackStartTime = currentTime;
            AMotorDuty = 0;
            BMotorDuty = 0;
          }
          break;
      }
    } else {
      ADirection = MOTOR_FORWARDS;
      BDirection = MOTOR_FORWARDS;
      if (currentTime - attackStartTime < ADJUST_DURATION) {
        if (lastTurnDirection == ADJUST_LEFT) {
          AMotorDuty = 120;
          BMotorDuty = 255;
        } else if (lastTurnDirection == ADJUST_RIGHT) {
          AMotorDuty = 220;
          BMotorDuty = 140;
        } else {
          AMotorDuty = 250;
          BMotorDuty = 250;
        }
      } else {
        AMotorDuty = 250;
        BMotorDuty = 250;
      }
    }
  } else if (mode == DEFENCE_MODE) {
    unsigned long currentTime = millis();
    if (Line_Sensor_1 == LINE_SENSOR_BLACK || defenceSubMode != 0) {
      if (defenceSubMode == 0) {
        defenceSubMode = 3;
        defenceStartTime = currentTime;
        ADirection = MOTOR_BACKWARDS;
        BDirection = MOTOR_BACKWARDS;
        AMotorDuty = 150;
        BMotorDuty = 150;
      } else if (defenceSubMode == 3 && currentTime - defenceStartTime >= 200) {
        defenceSubMode = 1;
        defenceStartTime = currentTime;
        ADirection = MOTOR_FORWARDS;
        BDirection = MOTOR_BACKWARDS;
        AMotorDuty = 255;
        BMotorDuty = 255;
      } else if (defenceSubMode == 1 && currentTime - defenceStartTime >= SPIN_DURATION) {
        defenceSubMode = 2;
        defenceStartTime = currentTime;
        ADirection = MOTOR_FORWARDS;
        BDirection = MOTOR_FORWARDS;
        AMotorDuty = 255;
        BMotorDuty = 255;
      } else if (defenceSubMode == 2 && currentTime - defenceStartTime >= ESCAPE_DURATION) {
        defenceSubMode = 0;
        AMotorDuty = 0;
        BMotorDuty = 0;
      }
    } else if (Line_Sensor_5 == LINE_SENSOR_BLACK) { // Do the 90 degree thing for the side sensors
      defenceSubMode = 0;
      ADirection = MOTOR_FORWARDS;
      BDirection = MOTOR_FORWARDS;
      AMotorDuty = 250;
      BMotorDuty = 150;
    } else if (Line_Sensor_2 == LINE_SENSOR_BLACK) {
      defenceSubMode = 0;
      ADirection = MOTOR_FORWARDS;
      BDirection = MOTOR_FORWARDS;
      AMotorDuty = 150;
      BMotorDuty = 250;
    } else if (Line_Sensor_3 == LINE_SENSOR_BLACK || Line_Sensor_4 == LINE_SENSOR_BLACK) {
      defenceSubMode = 0;
      ADirection = MOTOR_FORWARDS;
      BDirection = MOTOR_FORWARDS;
      AMotorDuty = 250;
      BMotorDuty = 250;
    } else {
      defenceSubMode = 0;
      AMotorDuty = 0;
      BMotorDuty = 0;
    }
  } else if (mode == SEARCH_MODE) {
    unsigned long currentTime = millis();
    if (dist1 < ATTACK_THRESHOLD) {
      mode = ATTACK_MODE;
      lastAttackCheck = millis();
      swayCycleComplete = false;
    } else {
      if (searchState == SEARCH_LEFT_TURN) {
        ADirection = MOTOR_BACKWARDS;
        BDirection = MOTOR_FORWARDS;
        AMotorDuty = 220;
        BMotorDuty = 220;
        if (currentTime - searchStateStartTime >= SEARCH_TURN_DURATION) {
          searchState = SEARCH_BRAKE;
          lastSearchTurnState = SEARCH_LEFT_TURN;
          searchStateStartTime = currentTime;
        }
      } else if (searchState == SEARCH_RIGHT_TURN) {
        ADirection = MOTOR_FORWARDS;
        BDirection = MOTOR_BACKWARDS;
        AMotorDuty = 150;
        BMotorDuty = 150;
        if (currentTime - searchStateStartTime >= SEARCH_TURN_DURATION) {
          searchState = SEARCH_BRAKE;
          lastSearchTurnState = SEARCH_RIGHT_TURN;
          swayCycleComplete = true;
          searchStateStartTime = currentTime;
        }
      } else if (searchState == SEARCH_BRAKE) {
        ADirection = MOTOR_BACKWARDS;
        BDirection = MOTOR_BACKWARDS;
        AMotorDuty = 100;
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
          searchState = (lastSearchTurnState == SEARCH_LEFT_TURN) ? SEARCH_RIGHT_TURN : SEARCH_LEFT_TURN;
          searchStateStartTime = currentTime;
        }
      } else if (searchState == SEARCH_FULL_SPIN) {
        ADirection = MOTOR_FORWARDS;
        BDirection = MOTOR_BACKWARDS;
        AMotorDuty = 255;
        BMotorDuty = 255;
        if (currentTime - searchStateStartTime >= SEARCH_FULL_SPIN_DURATION) {
          searchState = SEARCH_PAUSE;
          swayCycleComplete = false;
          searchStateStartTime = currentTime;
        }
      }
    }
  }

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
      dist1 = MAX_DISTANCE;
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

  tft.setTextDatum(TL_DATUM);
  tft.setTextFont(4);
  tft.setTextSize(1);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
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