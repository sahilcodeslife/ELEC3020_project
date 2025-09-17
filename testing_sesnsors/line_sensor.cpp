#include <Arduino.h>
#include <TFT_eSPI.h>  // Graphics library
#include <SPI.h>

#define LEFT_BUTTON 0
#define RIGHT_BUTTON 14
#define LineAPin 1
#define LineDPin 2




TFT_eSPI tft = TFT_eSPI();  // Invoke library

void setup() {      // Display init
  pinMode(LEFT_BUTTON, INPUT_PULLUP);
  pinMode(RIGHT_BUTTON, INPUT_PULLUP);
  pinMode(LineDPin, INPUT); // line pin 
  pinMode(LineAPin, INPUT); // line pin 

  tft.init();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);

  delay(1000);
  tft.fillScreen(TFT_GREEN);
  Serial.begin(115200);
}

void loop() {
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(20, 60);
    tft.setTextSize(2);
    tft.print("TEsting the line sensor.");
    int Line_A = analogRead(LineAPin);
    bool Line_D = analogRead(LineDPin);
    Serial.print(" Digital Reading: ");
    Serial.println(Line_D);
    Serial.print("Analloug Reading: ");
    Serial.println(Line_A);

  // tft.print("Left Button State");
  // tft.print(left_button_pressed);
  // tft.print("right Button State");
  // tft.print(right_button_pressed);
  delay(1000);
}
