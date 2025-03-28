#include "LED_Control.h"
#include <Arduino.h>
#include <Adafruit_PWMServoDriver.h>

LED_Control::LED_Control(Adafruit_PWMServoDriver& indicatedpwm, int portGreen, int portRed, int portBlue){
    pwm = &indicatedpwm;
    green = portGreen;
    red = portRed;
    blue = portBlue;
}
// Adafruit_PWMServoDriver* LED_Control::getPWM(){
//     return pwm;
// }
// int LED_Control::getGreen(){
//     return green;
// }
// int LED_Control::getRed(){
//     return red;
// }
// int LED_Control::getBlue(){
//     return blue;
// }
void LED_Control::SetColor(int brightness_of_green, int brightness_of_red, int brightness_of_blue){
    (*pwm).setPin(green, brightness_of_green, true);
    (*pwm).setPin(red, brightness_of_red, true);
    (*pwm).setPin(blue, brightness_of_blue, true);
}