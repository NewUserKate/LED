#include <Arduino.h>
#include <Adafruit_PWMServoDriver.h>
#include "LED_Control.h"
#include <IRremote.hpp>
#include "modes.h"

void SetColorForAll(int brightness_of_green, int brightness_of_red, int brightness_of_blue){
    for(int i = 0; i<7; i++){
        AllLEDs[i].SetColor(brightness_of_green, brightness_of_red, brightness_of_blue);  //(яскравість зеленого, яскравість червоного, яскравість синього)
    }
}
