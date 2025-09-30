#include <Arduino.h>
#include <TFT_eSPI.h>

// Pin definitions for ESP32
const int motorPin1 = 16;  // AIN1 - Direction pin 1
const int motorPin2 = 17;  // AIN2 - Direction pin 2

const int motorPin3 = 43;
const int motorPin4 = 44; 

void setup() {
  // Set motor pins as outputs
  pinMode(motorPin1, OUTPUT);
  pinMode(motorPin2, OUTPUT);
  pinMode(motorPin3, OUTPUT);
  pinMode(motorPin4, OUTPUT);
  
  
  // Initialize serial communication for debugging
  Serial.begin(115200);
  
  // Set motor to run forward
  Serial.println("Motor Running Forward");
  digitalWrite(motorPin1, HIGH);
  digitalWrite(motorPin2, LOW);
  digitalWrite(motorPin3, HIGH);
  digitalWrite(motorPin4, LOW);
  
}

void loop() {
  // No action needed; motor runs forward continuously
}
