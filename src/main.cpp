#include <Arduino.h>
#include <TFT_eSPI.h>

#define LEFT_BUTTON 0
#define RIGHT_BUTTON 14

//----------------DEFINING_LINE_SENSORS--------------------
#define SENSOR_LINE_1_PIN 1
#define SENSOR_LINE_2_PIN 2
#define SENSOR_LINE_3_PIN 3
#define SENSOR_LINE_4_PIN 10
#define SENSOR_LINE_5_PIN 11
//---------------------------------------------------------

//-----------------DEFINING_ULTRASONIC_SENSORS--------------
#define SONIC_OUT_PIN 18
#define SONIC_IN_PIN1 43
#define SONIC_IN_PIN2 44
//----------------------------------------------------------

//-----------DEFINING_MOTORS-------------------------------
#define A_PWM_PIN 17
#define A_DIRECTION_PIN 16
#define B_PWM_PIN 21
#define B_DIRECTION_PIN 13
#define PWM_PERIOD 20000
//----------------------------------------------------------

TFT_eSPI tft = TFT_eSPI();

//----------VARIABLES_FOR_ULTRASONIC_SENSORS----------------
hw_timer_t *SonicTriggerTimer = NULL;
//----------------------------------------------------------

//------------VARIABLES_FOR_LINE_SENSORS-------------------
bool Line_Sensor_1;
void ChangeSensor1() {
  Line_Sensor_1 = digitalRead(SENSOR_LINE_1_PIN);
}
bool Line_Sensor_2;
void ChangeSensor2() {
  Line_Sensor_2 = digitalRead(SENSOR_LINE_2_PIN);
}
bool Line_Sensor_3;
void ChangeSensor3() {
  Line_Sensor_3 = digitalRead(SENSOR_LINE_3_PIN);
}
bool Line_Sensor_4;
void ChangeSensor4() {
  Line_Sensor_4 = digitalRead(SENSOR_LINE_4_PIN);
}
bool Line_Sensor_5;
void ChangeSensor5() {
  Line_Sensor_5 = digitalRead(SENSOR_LINE_5_PIN);
}
//------------------------------------------------------------

//---------------VARIABLES_FOR_MOTORS-------------------------
int Modes[3] = {2000,10000,20000};
// int UIModes[3] = {10,50,100};

bool completeA = true;
bool completeB = true;

int starttimeA;
int starttimeB;

int HighTimeA;
int HighTimeB;

int modeA = 1;
int modeB = 1;
//------------------------------------------------------------

//-------------------------DEFINING_FUNCTIONS-----------------
void IRAM_ATTR SonicSensorTrigger();
void PWMA();
void PWMB();
//------------------------------------------------------------
void setup() {
  // setup for pins of buttons
  pinMode(LEFT_BUTTON,INPUT_PULLUP);
  pinMode(RIGHT_BUTTON,INPUT_PULLUP);

  // initilises the screen and general setup
  tft.init(); // start screen
  tft.setRotation(1); //Set screen orientation
  tft.fillScreen(TFT_BLUE);  // Set screen to all blue

  //---------------SETUP_FOR_LINE_SENSORS-------------------------
  Line_Sensor_1 = digitalRead(SENSOR_LINE_1_PIN);
  attachInterrupt(SENSOR_LINE_1_PIN,ChangeSensor1,CHANGE);
  Line_Sensor_2 = digitalRead(SENSOR_LINE_2_PIN);
  attachInterrupt(SENSOR_LINE_2_PIN,ChangeSensor2,CHANGE);
  Line_Sensor_3 = digitalRead(SENSOR_LINE_3_PIN);
  attachInterrupt(SENSOR_LINE_3_PIN,ChangeSensor3,CHANGE);
  Line_Sensor_4 = digitalRead(SENSOR_LINE_4_PIN);
  attachInterrupt(SENSOR_LINE_4_PIN,ChangeSensor4,CHANGE);
  Line_Sensor_5 = digitalRead(SENSOR_LINE_5_PIN);
  attachInterrupt(SENSOR_LINE_5_PIN,ChangeSensor5,CHANGE);
  // to read what the sensors see, use the variables given variabes (e.g. Line_Sensor_1)
  //-------------------------------------------------------------

  //--------------SETUP_FOR_ULTRASONIC_SENSORS------------------------
  pinMode(SONIC_OUT_PIN,OUTPUT);
  pinMode(SONIC_IN_PIN1,INPUT_PULLUP);
  pinMode(SONIC_IN_PIN2,INPUT_PULLUP);

  SonicTriggerTimer = timerBegin(0,80,true);
  timerAttachInterrupt(SonicTriggerTimer,SonicSensorTrigger,true);
  timerAlarmWrite(SonicTriggerTimer,10,false);

  // use these commands to run the timer and trigger the ultrasonic sensors
  // timerRestart(SonicTriggerTimer);
  // digitalWrite(SONIC_OUT_PIN,HIGH);
  // timerAlarmEnable(SonicTriggerTimer);
  //-------------------------------------------------------------

  //-------------------SETUP_FOR_MOTORS-------------------
  pinMode(A_PWM_PIN,OUTPUT);
  pinMode(A_DIRECTION_PIN,OUTPUT);
  pinMode(B_PWM_PIN,OUTPUT);
  pinMode(B_DIRECTION_PIN,OUTPUT);
  // Change modeA and modeB for different waveforms/speeds
  // digitalWrite to A_DIRECTION_PIN and B_DIRECTION_PIN to change directions
  // PWMA and PWMB should be called endlessly in loop

  //-------------------------------------------------------------
}

void loop() {
  tft.setTextColor(TFT_RED,TFT_BLUE);
  tft.setTextSize(5);
  tft.setCursor(0,0);
  tft.printf("");

  PWMA();
  PWMB();

  
}

void IRAM_ATTR SonicSensorTrigger() {
  digitalWrite(SONIC_OUT_PIN,LOW);
}

void PWMA() {
  if(completeA == true) {
        starttimeA = micros();
        digitalWrite(A_PWM_PIN,HIGH);
        HighTimeA = Modes[modeA];
        completeA = false;
    }
    if(micros() >= starttimeA + HighTimeA) {
        digitalWrite(A_PWM_PIN,LOW);
    }
    if(micros() >= starttimeA + PWM_PERIOD) {
        completeA = true;
    }
}

void PWMB() {
  if(completeB == true) {
        starttimeB = micros();
        digitalWrite(B_PWM_PIN,HIGH);
        HighTimeB = Modes[modeB];
        completeB = false;
    }
    if(micros() >= starttimeB + HighTimeB) {
        digitalWrite(B_PWM_PIN,LOW);
    }
    if(micros() >= starttimeB + PWM_PERIOD) {
        completeB = true;
    }
}