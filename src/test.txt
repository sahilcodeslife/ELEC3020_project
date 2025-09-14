#include <Arduino.h>
#include <TFT_eSPI.h>  // Graphics library
#include <SPI.h>

#define LEFT_BUTTON 0
#define RIGHT_BUTTON 14




TFT_eSPI tft = TFT_eSPI();  // Invoke library

void setup() {      // Display init
  pinMode(LEFT_BUTTON, INPUT_PULLUP);
  pinMode(RIGHT_BUTTON, INPUT_PULLUP);
  tft.init();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);

  delay(1000);
  tft.fillScreen(TFT_GREEN);
}

void loop() {

  bool left_button_pressed = !digitalRead(LEFT_BUTTON);
  bool right_button_pressed = !digitalRead(RIGHT_BUTTON);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setCursor(0,0);
  tft.print("SIMMI");

  // tft.print("Left Button State");
  // tft.print(left_button_pressed);
  // tft.print("right Button State");
  // tft.print(right_button_pressed);

}
