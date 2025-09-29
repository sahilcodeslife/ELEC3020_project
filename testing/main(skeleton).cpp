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
#define SONIC_IN_PIN1 44
// #define SONIC_IN_PIN2 44
#define SONIC_REFRESH_RATE 12000 // max distance of about 2 meters
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
//hw_timer_t *SonicTriggerTimer = NULL;
//hw_timer_t *SonicRefreshTimer = NULL;
bool USready = true;
int StartTime1;
int EchoTime1;
float dist1;
unsigned long lastTrigger = 0;
//int StartTime2;
//float dist2;
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
// read sensors variables
// if the interupts are triggered it will also raise a flag SituationChange, which just tells us that a sensor changed.
// please reset SituationChange to false, after the new situation has been handled, otherwise it is a cause of bugs
//------------------------------------------------------------

//---------------VARIABLES_FOR_MOTORS-------------------------
int Modes[3] = {0,10000,20000};
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
void SonicDistance1();
void SonicDistance2();
void SonicSense();
void drawUI(int Line_Sensor_1, int Line_Sensor_2, int Line_Sensor_3, int Line_Sensor_4, int Line_Sensor_5,
            int dist1, /*int dist2,*/ int modeA, int modeB);

//------------------------------------------------------------
void setup() {
  //debugging
  Serial.begin(115200);

  // setup for pins of buttons
  pinMode(LEFT_BUTTON,INPUT_PULLUP);
  pinMode(RIGHT_BUTTON,INPUT_PULLUP);

  // initilises the screen and general setup
  tft.init(); // start screen
  tft.setRotation(1); //Set screen orientation
  tft.fillScreen(TFT_WHITE);  // Set screen to all blue

  //---------------SETUP_FOR_LINE_SENSORS-------------------------
  ////declare each as input
  pinMode(SENSOR_LINE_2_PIN,INPUT);
  pinMode(SENSOR_LINE_3_PIN,INPUT);
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
  //pinMode(SONIC_IN_PIN2,INPUT_PULLUP);

  //SonicTriggerTimer = timerBegin(0,80,true);
  //SonicRefreshTimer = timerBegin(0,80,true);
  //timerAttachInterrupt(SonicTriggerTimer,SonicSensorTrigger,true);
  //timerAttachInterrupt(SonicRefreshTimer,SonicSense,true);
  //timerAlarmWrite(SonicTriggerTimer,10,false);
  //timerAlarmWrite(SonicRefreshTimer,SONIC_REFRESH_RATE,true);
  attachInterrupt(SONIC_IN_PIN1,SonicDistance1,CHANGE);
  //attachInterrupt(SONIC_IN_PIN2,SonicDistance2,CHANGE);

  //timerRestart(SonicRefreshTimer);
  //timerAlarmEnable(SonicRefreshTimer);

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
void USTrigger() {
  digitalWrite(SONIC_OUT_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(SONIC_OUT_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(SONIC_OUT_PIN, LOW);
}


void loop() {
  if (USready || (micros()-lastTrigger > 50000)) {
    USready = false;
    lastTrigger = micros();
    USTrigger();
  }

  Serial.print("Ultrasonic distance: ");
  Serial.print(dist1);
  Serial.println(" cm");
  /*tft.setTextColor(TFT_GREEN,TFT_BLACK);
  tft.setTextSize(1.2);
  tft.setCursor(0,0);
  tft.printf("Line: 1:%d, 2:%d, 3:%d, 4:%d, 5:%d     ",Line_Sensor_1,Line_Sensor_2,Line_Sensor_3,Line_Sensor_4,Line_Sensor_5);
  tft.setCursor(0,40);
  tft.printf("Sonic Distance: 1-%i, 2-%i    ",dist1,dist2);
  tft.setCursor(0,70);
  tft.printf("Motor Mode: A-%i, B-%i     ",modeA,modeB);
  */
  drawUI(Line_Sensor_1, Line_Sensor_2, Line_Sensor_3, Line_Sensor_4, Line_Sensor_5,
       dist1, /*dist2,*/ modeA, modeB);
  
  PWMA();
  PWMB();

  if(dist1 < 30 /*||  dist2 < 30*/) {
    modeA = 2;
    modeB = 2;
  } else {
    modeA = 0;
    modeB = 0;
  }

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

void IRAM_ATTR SonicDistance1() {
  if(digitalRead(SONIC_IN_PIN1) == HIGH) {
    StartTime1 = micros();
  } else {
    int EchoTime1 = micros() - StartTime1;
    dist1 = (EchoTime1 * 0.0343)/2;
    USready = true;
  }
  
}

/*void SonicDistance2() {
  if(digitalRead(SONIC_IN_PIN2) == HIGH) {
    StartTime2 = micros();
  } else {
    int EchoTime2 = micros() - StartTime2;
    dist2 = (EchoTime2 * 0.0343)/2;
  }
}
*/
/*void IRAM_ATTR SonicSensorTrigger() {
  digitalWrite(SONIC_OUT_PIN,LOW);
}*/

/*void SonicSense() {
  timerRestart(SonicTriggerTimer);
  digitalWrite(SONIC_OUT_PIN,HIGH);
  timerAlarmEnable(SonicTriggerTimer);
}*/

//beautiful GUI helper. 
void drawUI(int Line_Sensor_1, int Line_Sensor_2, int Line_Sensor_3, int Line_Sensor_4, int Line_Sensor_5,
            int dist1, /*int dist2,*/ int modeA, int modeB) {
  
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);

  // --- Line Sensors (top row as circles) ---
  int y = 20;
  int spacing = 60;
  int r = 12;
  uint16_t active = TFT_BLACK, inactive = TFT_WHITE;
  
  tft.setTextDatum(MC_DATUM);
   
  tft.fillCircle(20 + 0*spacing, y, r, Line_Sensor_1 ? active : inactive);
  tft.drawCircle(20+ 0*spacing, y,r,TFT_BLACK);
  tft.setTextColor(Line_Sensor_1 ? TFT_WHITE : TFT_BLACK);
  tft.drawNumber(1, 20 + 0*spacing, y);

  tft.fillCircle(20 + 1*spacing, y, r, Line_Sensor_2 ? active : inactive);
  tft.drawCircle(20+ 1*spacing, y,r,TFT_BLACK);
  tft.setTextColor(Line_Sensor_2 ? TFT_WHITE : TFT_BLACK);
  tft.drawNumber(2, 20 + 1*spacing, y);

  tft.fillCircle(20 + 2*spacing, y, r, Line_Sensor_3 ? active : inactive);
  tft.drawCircle(20+ 2*spacing, y,r,TFT_BLACK);
  tft.setTextColor(Line_Sensor_3 ? TFT_WHITE : TFT_BLACK);
  tft.drawNumber(3, 20 + 2*spacing, y);

  tft.fillCircle(20 + 3*spacing, y, r, Line_Sensor_4 ? active : inactive);
  tft.drawCircle(20+ 3*spacing, y,r,TFT_BLACK);
  tft.setTextColor(Line_Sensor_4 ? TFT_WHITE : TFT_BLACK);
  tft.drawNumber(4, 20 + 3*spacing, y);

  tft.fillCircle(20 + 4*spacing, y, r, Line_Sensor_5 ? active : inactive);
  tft.drawCircle(20+ 4*spacing, y,r,TFT_BLACK);
  tft.setTextColor(Line_Sensor_5 ? TFT_WHITE : TFT_BLACK);
  tft.drawNumber(5, 20 + 4*spacing, y);
  
  tft.setCursor(10, y + 20);
  tft.print("Line Sensors");

  // --- Ultrasonic Distance (new red bar) ---
  int maxDist = 100;   // max distance in cm for scale
  int barX = 20;
  int barY = 60;
  int barW = 200;
  int barH = 40;

  // Closer = more fill
  int fillW = map(constrain(dist1, 0, maxDist), maxDist, 0, 0, barW);

  // Draw outline
  tft.drawRect(barX, barY, barW, barH, TFT_BLACK);

  // Fill proportional red
  tft.fillRect(barX, barY, fillW, barH, TFT_RED);

  // Distance text in center
  tft.setTextDatum(MC_DATUM);
  tft.setTextFont(4);
  tft.setTextSize(1);

  uint16_t textColor = (fillW > barW / 2) ? TFT_WHITE : TFT_BLACK;
  tft.setTextColor(textColor, TFT_WHITE);

  char buf[16];
  snprintf(buf, sizeof(buf), "%icm", (int)dist1);
  tft.drawString(buf, barX + barW / 2, barY + barH / 2);

  // --- Old ultrasonic bars (commented out) ---
  /*
  int maxDist = 100;  // cm scale
  int barWidth = 100;
  int barHeight = 12;
  
  // Dist1 bar
  int w1 = map(dist1, 0, maxDist, 0, barWidth);
  tft.drawRect(20, 60, barWidth, barHeight, TFT_WHITE);
  tft.fillRect(20, 60, w1, barHeight, TFT_BLUE);
  tft.setCursor(130, 60);
  tft.printf("D1:%icm", dist1);
  */

  // --- Motor Modes (colored boxes) ---
  int boxW = 60, boxH = 30;
  
  tft.fillRoundRect(20, 120, boxW, boxH, 5, (modeA) ? TFT_GREEN : TFT_DARKGREY);
  tft.setCursor(30, 130);
  tft.setTextColor(TFT_BLACK);
  tft.printf("A:%i", modeA);

  tft.fillRoundRect(100, 120, boxW, boxH, 5, (modeB) ? TFT_GREEN : TFT_DARKGREY);
  tft.setCursor(110, 130);
  tft.printf("B:%i", modeB);
}
