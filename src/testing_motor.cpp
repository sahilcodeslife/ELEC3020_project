#include <Arduino.h>
#include <TFT_eSPI.h>

// Motor A pins
#define AIN1 18
#define AIN2 17
#define PWMA 12
#define STBY 21

// Ultrasonic sensor pins
#define TRIG 43
#define ECHO 44

// PWM settings
#define PWM_CHANNEL 0
#define PWM_FREQ 1000
#define PWM_RESOLUTION 8

// Display
TFT_eSPI tft = TFT_eSPI();

void setup() {
    pinMode(AIN1, OUTPUT);
    pinMode(AIN2, OUTPUT);
    pinMode(STBY, OUTPUT);
    pinMode(TRIG, OUTPUT);
    pinMode(ECHO, INPUT);

    digitalWrite(STBY, HIGH);
    digitalWrite(TRIG, LOW);

    ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(PWMA, PWM_CHANNEL);

    // TFT init
    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setTextSize(2);
}

long getDistanceCM() {
    digitalWrite(TRIG, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG, LOW);

    long duration = pulseIn(ECHO, HIGH, 30000); // 30 ms timeout
    if (duration == 0) return -1;               // no echo
    return duration * 0.034 / 2;
}

void loop() {
    long distance = getDistanceCM();

    tft.fillRect(0, 0, 240, 40, TFT_BLACK);  // clear text area

    if (distance > 0 && distance < 15) {
        // motor forward
        digitalWrite(AIN1, HIGH);
        digitalWrite(AIN2, LOW);
        ledcWrite(PWM_CHANNEL, 200);

        tft.setCursor(10, 10);
        tft.print("Object detected");
    } else {
        // motor stop
        digitalWrite(AIN1, LOW);
        digitalWrite(AIN2, LOW);
        ledcWrite(PWM_CHANNEL, 0);

        tft.setCursor(10, 10);
        tft.print("No object");
    }

    delay(200);
}
