#include <Arduino.h>
#include <Adafruit_PWMServoDriver.h>
#include "LED_Control.h"

Adafruit_PWMServoDriver pwm1 = Adafruit_PWMServoDriver(0x40);
Adafruit_PWMServoDriver pwm2 = Adafruit_PWMServoDriver(0x41);

LED_Control LED_1(pwm1, 0, 1, 2);
LED_Control LED_2(pwm1, 4, 5, 6);
LED_Control LED_3(pwm1, 8, 9, 10);
LED_Control LED_4(pwm1, 12, 13, 14);

LED_Control LED_5(pwm2, 0, 1, 2);
LED_Control LED_6(pwm2, 4, 5, 6);
LED_Control LED_7(pwm2, 8, 9, 10);

LED_Control AllLEDs[7]={LED_1, LED_2, LED_3, LED_4, LED_5, LED_6, LED_7};

void SetColorForAll(int brightness_of_green, int brightness_of_red, int brightness_of_blue){
  for(int i = 0; i<7; i++){
    AllLEDs[i].SetColor(brightness_of_green, brightness_of_red, brightness_of_blue);
  }
}

void setup() {
  pwm1.begin();
  pwm1.setOutputMode(false);
  pwm1.setPWMFreq(1000);
  pwm2.begin();
  pwm2.setOutputMode(false);
  pwm2.setPWMFreq(1000);
  }
  
void loop() {
  
}

