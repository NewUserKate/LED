#pragma once
#include <Arduino.h>
#include <Adafruit_PWMServoDriver.h>
#include "LED_Control.h"

extern LED_Control AllLEDs[];

extern bool red;
extern bool green;
extern bool yellow;
extern bool CheckCode();
extern void myDelay(int duration);

extern bool newsignal;

void SetColorForAll(int brightness_of_green, int brightness_of_red, int brightness_of_blue);

void Start();
void traffic_light();
void OnOffRed();
void OnOffGreen();
void OnOffYellow();
void Audio();
void Test();