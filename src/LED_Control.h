#pragma once
#include <Arduino.h>
#include <Adafruit_PWMServoDriver.h>

class LED_Control{
private:
    Adafruit_PWMServoDriver* pwm;
    int green;
    int red;
    int blue;
public:
    LED_Control(Adafruit_PWMServoDriver& indicatedpwm, int portGreen, int portRed, int portBlue);
    void SetColor(int brightness_of_green, int brightness_of_red, int brightness_of_blue);
};