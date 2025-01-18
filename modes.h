#pragma once
#include <Arduino.h>
#include "LED_Control.h"
extern LED_Control AllLEDs[];

void SetColorForAll(int brightness_of_green, int brightness_of_red, int brightness_of_blue);