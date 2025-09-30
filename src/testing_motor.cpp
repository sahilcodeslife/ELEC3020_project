#include <Arduino.h>
#include <TFT_eSPI.h>
// Motor A pins (MX1508 IN1 / IN2)
#define AIN1 44   // PWM pin
#define AIN2 43   // Direction pin

// PWM settings
#define PWM_CHANNEL 0
#define PWM_FREQ 200      // Hz (low freq so you can hear it changing duty)
#define PWM_RESOLUTION 8  // 8-bit (0–255)

void setup() {
  pinMode(AIN2, OUTPUT);
  digitalWrite(AIN2, LOW);  // hold IN2 low → direction fixed

  // Setup PWM on IN1
  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(AIN1, PWM_CHANNEL);
}

void loop() {
  // Ramp up speed
  for (int duty = 0; duty <= 255; duty += 5) {
    ledcWrite(PWM_CHANNEL, duty);
    delay(50);
  }

  // Ramp down speed
  for (int duty = 255; duty >= 0; duty -= 5) {
    ledcWrite(PWM_CHANNEL, duty);
    delay(50);
  }

  // Stop for a moment
  ledcWrite(PWM_CHANNEL, 0);
  delay(1000);
}