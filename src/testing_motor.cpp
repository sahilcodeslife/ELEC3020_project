#include <Arduino.h>
#include <TFT_eSPI.h>
TFT_eSPI tft = TFT_eSPI();
// Motor A pins (TB6612FNG)
#define PWMA 43
#define AIN1 44
#define AIN2 18

// Motor B pins (TB6612FNG)
#define PWMB 17
#define BIN1 21
#define BIN2 16

// PWM settings
#define PWM_CHANNEL_A 2
#define PWM_CHANNEL_B 3
#define PWM_FREQ 1000    
#define PWM_RESOLUTION 8  // 8-bit (0–255)

void setup() {
  pinMode(15, OUTPUT);
  digitalWrite(15, HIGH);
  // Direction pins
  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);

  // Setup PWM channels
  ledcSetup(PWM_CHANNEL_A, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(PWM_CHANNEL_B, PWM_FREQ, PWM_RESOLUTION);

  // Attach PWM pins
  ledcAttachPin(PWMA, PWM_CHANNEL_A);
  ledcAttachPin(PWMB, PWM_CHANNEL_B);
  // initilises the screen and general setup
  tft.init(); // start screen
  tft.setRotation(1); //Set screen orientation
  tft.fillScreen(TFT_BLUE);  // Set screen to all blue

}
/* void rampMotor(int channel, int in1, int in2) {
  // Forward
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
  for (int duty = 0; duty <= 255; duty += 5) {
    ledcWrite(channel, duty);
    delay(30);
  }
  delay(500);

  // Stop
  ledcWrite(channel, 0);
  delay(500);

  // Reverse
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  for (int duty = 0; duty <= 255; duty += 5) {
    ledcWrite(channel, duty);
    delay(30);
  }
  delay(500);

  // Stop
  ledcWrite(channel, 0);
  delay(1000);
}
*/
bool off = false;

void loop() {
  tft.fillScreen(TFT_BLUE);

  // Example: sweep Motor A and B speed up and down
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, HIGH);
  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, HIGH);

  // Sweep up
  for (int duty = 0; duty <= 255; duty += 50) {
    ledcWrite(PWM_CHANNEL_A, duty);  // Motor A
    ledcWrite(PWM_CHANNEL_B, duty);  // Motor B
    delay(20);
  }

  // Sweep down
  for (int duty = 255; duty >= 0; duty -= 50) {
    ledcWrite(PWM_CHANNEL_A, duty);
    ledcWrite(PWM_CHANNEL_B, duty);
    delay(20);
  }
      
    
  }
