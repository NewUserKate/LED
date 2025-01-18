#include <Arduino.h>
#include <Adafruit_PWMServoDriver.h>
#include "LED_Control.h"
#include <IRremote.hpp>
#include "modes.h"

const int IR_RECIEVE_PIN = 3;

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

// void SetColorForAll(int brightness_of_green, int brightness_of_red, int brightness_of_blue){  //Встановити колір для всіх світлодіодів
//   for(int i = 0; i<7; i++){
//     AllLEDs[i].SetColor(brightness_of_green, brightness_of_red, brightness_of_blue);  //(яскравість зеленого, яскравість червоного, яскравість синього)
//   }
// }

bool red = false;
bool green = false;
bool yellow = false;

int indexOfCode;  // Індекс елемента в масиві remoteKeys

unsigned long lastCode = 0; //Змінна, що зберігає попередній код з пульта(це потрібно для гарної роботи OnOffRed, OnOffGreen та OnOffYellow)
bool newSignal = false;

const long unsigned int remoteKeys[6]={     // Масив з кодами, що отримуються від пульта(вказуються ті, що будуть використовуватися)
  3125149440,
  3108437760,
  3091726080,
  3141861120,
  3208707840,
  3158572800
};

void resumeSearch(){
  IrReceiver.resume();
}

bool CheckCode(){                         //CheckCode() перевіряє, чи був отриманий сигнал з пульта та чи є він відомим(чи є він у масиві remoteKeys)
  bool checked = false;
  if(IrReceiver.decode()){ 
    for(int i = 0; i < 6; i++){
      if(remoteKeys[i] == IrReceiver.decodedIRData.decodedRawData){
        checked = true;
        break;
      }
    }
  }
  return checked;
}



void myDelay(int duration){               //myDelay створена, щоб під час паузи 
    for(int i = 0; i<duration; i++){      //можна було отримати новий сигнал з пульта та запустити інший режим
      if(i%50 == 0){
        if(CheckCode()){                  //Перевіряється, чи був отриманий новий сигнал та чи знаходиться він в remoteKeys
          break;
        } 
        IrReceiver.resume();
      }
      delay(1);
    } 
}

void setup() {
  pwm1.begin();
  pwm1.setOutputMode(false);
  pwm1.setPWMFreq(1000);
  pwm2.begin();
  pwm2.setOutputMode(false);
  pwm2.setPWMFreq(1000);
  Serial.begin(9600);
  IrReceiver.begin(IR_RECIEVE_PIN, ENABLE_LED_FEEDBACK);
  randomSeed(analogRead(0));
  SetColorForAll(0, 0, 0);
}

void loop() {
  if(IrReceiver.decode()){
    for(int i = 0; i < 6; i++){
      if(remoteKeys[i] == IrReceiver.decodedIRData.decodedRawData){
        indexOfCode = i;
        break;
      }
    }
    newSignal = true;
    IrReceiver.resume();
  }

  if(newSignal){
    newSignal = false;
    switch(indexOfCode){

      case 0:   //Кновка 1 на пульті
      lastCode = IrReceiver.decodedIRData.decodedRawData;
      Start();
      break;

      case 1:   //Кновка 2 на пульті
      lastCode = IrReceiver.decodedIRData.decodedRawData;
      traffic_light();
      break;

      case 2:
      if(lastCode != IrReceiver.decodedIRData.decodedRawData){      //Якщо останнім режимом не було ввімкнення/вимкнення даного кольору->
        red = true;                                                 //виставити значення кольору true, щоб одразу ввімкнути світлодіод
      }
      else{                                                         //Інакше поміняти стан світлодіоду на протилежний. щоб він ввімкнувся або 
        red = !red;                                                 //ввимкнувся в залежності від стану, в якому він був
      }
      lastCode = IrReceiver.decodedIRData.decodedRawData;
      OnOffRed();
      break;

      case 3:
      if(lastCode != IrReceiver.decodedIRData.decodedRawData){
        green = true;
      }
      else{
        green = !green;
      }
      lastCode = IrReceiver.decodedIRData.decodedRawData;
      OnOffGreen();
      break;

      case 4:
      if(lastCode != IrReceiver.decodedIRData.decodedRawData){
        yellow = true;
      }
      else{
        yellow = !yellow;
      }
      lastCode = IrReceiver.decodedIRData.decodedRawData;
      OnOffYellow();
      break;

      case 5:
      lastCode = IrReceiver.decodedIRData.decodedRawData;
      Audio();
      break;
    }
  } 
}  
    