#include <Arduino.h>
#include <Adafruit_PWMServoDriver.h>
#include "LED_Control.h"
#include "modes.h"

void SetColorForAll(int brightness_of_green, int brightness_of_red, int brightness_of_blue){
    for(int i = 0; i<7; i++){
        AllLEDs[i].SetColor(brightness_of_green, brightness_of_red, brightness_of_blue);  //(яскравість зеленого, яскравість червоного, яскравість синього)
    }
}

/****************************    Перший режим (Start)     *****************************/
void Start(){
    int randDuration = random(2000, 7000);
  SetColorForAll(2650, 4095, 0);  //Жовтий колір на всіх світлодіодах
  myDelay(3000);

  if(CheckCode()){            
    return;
  }

  for(int i = 0; i<6; i++){
    AllLEDs[i].SetColor(0, 2867, 0); // Загоряються червоним на 70% по черзі з інтервалом в 1.2 секунди
    myDelay(1200);
    if(CheckCode()){            
      break;
    } 
    resumeSearch();
  }

  AllLEDs[6].SetColor(0, 2867, 0);
  if(CheckCode()){            
      return;
  } 

  for(int i = 0; i<1229; i+=8){   // Збільшується яскравість до 100% протягом 2 секунд
    if(i%8 == 0){
      if(CheckCode()){            
        break;
      } 
      resumeSearch();
    }
    for(int j = 0; j<7; j++){
      AllLEDs[j].SetColor(0, 2867+i, 0);  
    }
  } 

  if(CheckCode()){            
    return;
  } 

  myDelay(randDuration);
  if(CheckCode()){            
    return;
  } 

  SetColorForAll(4095, 0, 0);       //Всі світлодіоди змінюють свій колір на зелений
  myDelay(5000);

  if(CheckCode()){            
    return;
  } 
  for(int i = 4095; i>-1; i-=9){  //Світлодіоди повільно затухають
    if(i%27 == 0){
      if(CheckCode()){            
        break;
      } 
      resumeSearch();
    }
    for(int j = 0; j<7; j++){
      AllLEDs[j].SetColor(i, 0, 0); 
    }
    
  }
  while(!CheckCode()){          //Цикл, що забезпечує виконання режиму Start лише один раз та не допускає отримання невідомих
    SetColorForAll(0, 0, 0);    //кодів з пульта(цикл завершиться тільки після отримання коду з пульта, що є у масиві remoteKeys)
  }
}

/***************************** Другий режим (traffic_light)****************************************/
void traffic_light(){
    while(!CheckCode()){
    resumeSearch();
    for(int i = 0; i<4095; i+=21){    // Плавно збільшується яскравість до 100%(червоний колір)
      if(i%21 == 0){
        if(CheckCode()){            
          break;
        } 
        resumeSearch();
      }
      for(int j = 0; j<7; j++){
        AllLEDs[j].SetColor(0, i, 0);  
      }
    } 
    if(CheckCode()){            
      break;
    } 
  for(int i = 4095; i>-1; i-=21){   // Плавно зменшується яскравість до 0%(червоний колір)
    resumeSearch();
    if(i%21 == 0){
      if(CheckCode()){            
        break;
      } 
      resumeSearch();
    }
    for(int j = 0; j<7; j++){
      AllLEDs[j].SetColor(0, i, 0);  
    }
    if(CheckCode()){            
      break;
    } 
  } 
}
}

/************************* Третій, четвертий та п'ятий режими(Ввімкнути/вимкнути червоний/зелений/жовтий)********************************************/
void OnOffRed(){
  while(!CheckCode()){
    resumeSearch();
    if(red == true){
      SetColorForAll(0, 4095, 0);
    }
    else{
      SetColorForAll(0, 0, 0);
    }
  }
}

void OnOffGreen(){
  while(!CheckCode()){
    resumeSearch();
    if(green == true){
      SetColorForAll(4095, 0, 0);
    }
    else{
      SetColorForAll(0, 0, 0);
    }
  }
}

void OnOffYellow(){
  while(!CheckCode()){
    resumeSearch();
    if(yellow == true){
      SetColorForAll(2650, 4095, 0);
    }
    else{
      SetColorForAll(0, 0, 0);
    }
  }
}
/******************************** Шостий режим (Audio)*************************************/
void Audio(){
    SetColorForAll(0, 0, 0);
    while(true){
        AllLEDs[0].SetColor(0, 0, 0);
        AllLEDs[6].SetColor(0, 0, 0);
        AllLEDs[3].SetColor(2650, 4095, 0);
        myDelay(550);
        if(CheckCode()){
            break;
        }
        AllLEDs[3].SetColor(0, 0, 0);
        AllLEDs[2].SetColor(2650, 4095, 0);
        AllLEDs[4].SetColor(2650, 4095, 0);
        myDelay(550);
        if(CheckCode()){
            break;
        }
        AllLEDs[2].SetColor(0, 0, 0);
        AllLEDs[4].SetColor(0, 0, 0);
        AllLEDs[1].SetColor(2650, 4095, 0);
        AllLEDs[5].SetColor(2650, 4095, 0);
        myDelay(550);
        if(CheckCode()){
            break;
        }
        AllLEDs[1].SetColor(0, 0, 0);
        AllLEDs[5].SetColor(0, 0, 0);
        AllLEDs[0].SetColor(2650, 4095, 0);
        AllLEDs[6].SetColor(2650, 4095, 0);
        myDelay(550);
    }
}