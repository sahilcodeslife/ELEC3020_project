#include <Arduino.h>
#include <TFT_eSPI.h>

#define LEFT_BUTTON 0
#define RIGHT_BUTTON 14

TFT_eSPI tft = TFT_eSPI();

//--------------------------------------
#define US_SENSOR_TRIGGER_PIN 10 // an output
#define US_ECHO_RECEIVER_PIN 11 // an input

#define TIME_UNTIL_RESET 12000 // in microseconds the equation is distance(cm) = (time(us) * 0.0343)/2, 12000 is about 2 metres
//--------------------------------------

void USTrigger();
void HighTime();

bool USready = true;

int StartTime = 0;
int EchoTime;
float dist;


void setup() {
  // setup for pins of buttons
  pinMode(LEFT_BUTTON,INPUT_PULLUP);
  pinMode(RIGHT_BUTTON,INPUT_PULLUP);

  // pin setup for ultrasonic sensor
  pinMode(US_SENSOR_TRIGGER_PIN,OUTPUT);
  pinMode(US_ECHO_RECEIVER_PIN,INPUT_PULLUP);

  attachInterrupt(US_ECHO_RECEIVER_PIN,HighTime,CHANGE);

  // initilises the screen and general setup
  tft.init(); // start screen
  tft.setRotation(1); //Set screen orientation
  tft.fillScreen(TFT_BLUE);  // Set screen to all blue

  digitalWrite(US_SENSOR_TRIGGER_PIN,LOW);
  USready = true;
}

void loop() {
  tft.setTextColor(TFT_RED,TFT_BLUE);
  tft.setTextSize(7);
  tft.setCursor(0,0);
  tft.printf("%.2f    ",dist);
  if(USready == true) {
    USTrigger();
    USready = false;
  } 
}

void USTrigger() {
    digitalWrite(US_SENSOR_TRIGGER_PIN,LOW);
    delayMicroseconds(2);

    digitalWrite(US_SENSOR_TRIGGER_PIN,HIGH);
    delayMicroseconds(10);
    digitalWrite(US_SENSOR_TRIGGER_PIN,LOW);
}

void HighTime() {
  if(digitalRead(US_ECHO_RECEIVER_PIN) == HIGH) {
    StartTime = micros();
  } else {
    EchoTime = micros() - StartTime;
    dist = (EchoTime * 0.0343)/2;
    USready = true;
  }
}
