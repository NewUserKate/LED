#include <Arduino.h>
#include <Adafruit_PWMServoDriver.h>
#include "LED_Control.h"
#include "modes.h"
#include <RF24.h>        // ещё библиотека радиомодуля
#define CH_NUM 0x60   // номер канала (должен совпадать с приёмником)
#define SIG_POWER RF24_PA_LOW
#define SIG_SPEED RF24_1MBPS

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

RF24 radio(9, 10);

bool red = false;
bool green = false;
bool yellow = false;

byte pipeNo, gotByte;
byte thissignal = -1; //Змінна, що зберігає попередній код з пульта(це потрібно для гарної роботи раідо)
bool newsignal= false;  
unsigned long newstart = 0; //Змінна, що зберігає час отримання сигналу
byte addresses[][6] = {"1Node", "2Node"}; // Radio pipe addresses for the 2 nodes to communicate.

const int possibleSignals[7]={1, 2, 3, 4, 5, 6, 255};  //Масив з кодами, що отримуються від пульта(вказуються ті, що будуть використовуватися)
const int numberOfSignals = sizeof(possibleSignals)/sizeof(possibleSignals[0]); //Кількість можливих сигналів 

void radioSetup() {             // настройка радио
  radio.begin();                // активировать модуль
  radio.setAutoAck(1);          // режим подтверждения приёма, 1 вкл 0 выкл
  radio.setRetries(0, 15);      // (время между попыткой достучаться, число попыток)
  radio.enableAckPayload();     // разрешить отсылку данных в ответ на входящий сигнал
  radio.setPayloadSize(32);     // размер пакета, байт
  radio.openReadingPipe(1, addresses[0]); // хотим слушать трубу 0
  radio.setChannel(CH_NUM);     // выбираем канал (в котором нет шумов!)
  radio.setPALevel(SIG_POWER);  // уровень мощности передатчика
  radio.setDataRate(SIG_SPEED); // скорость обмена
  // должна быть одинакова на приёмнике и передатчике!
  // при самой низкой скорости имеем самую высокую чувствительность и дальность!!

  radio.powerUp();         // начать работу
  radio.startListening();  // начинаем слушать эфир, мы приёмный модуль
}


int serial_putc( char c, FILE * ) {
  Serial.write( c );
  return c;
}

void printf_begin(void) {
  fdevopen( &serial_putc, 0 );
}


bool CheckCode(){                         //CheckCode() перевіряє, чи був отриманий сигнал з пульта та чи є він відомим(чи є він у масиві remoteKeys)
  bool checked = false;
  if(radio.available()){ 
    radio.read( &gotByte, 1 );
    radio.writeAckPayload(pipeNo, &gotByte, 1 ); // This can be commented out to send empty payloads.
    printf("Sent response %d \n\r", gotByte);
    Serial.println("this is newCheckCode speaking");
    for(int i = 0; i < numberOfSignals; i++){
      if(gotByte == possibleSignals[i] && millis()-newstart > 80){ //Якщо доводилося відправляти два сигнали підряд, режими швидко перезапустяться
        newstart = millis();                                       //Щоб цього не відбулося, відбувається перевірка, наскільки швидко був отриманий новий сигнал
        newsignal = true;
        Serial.println("checked");
        checked = true;
        break;
      }
    }
  }
  return checked;
}

void myDelay(int duration){         //myDelay створена, щоб під час паузи 
  for(int i = 0; i<duration; i++){  //можна було отримати новий сигнал з пульта та запустити інший режим
    if(i%50 == 0){
      if(CheckCode()){      
        Serial.println("skip");
        break;
      }
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
  
  printf_begin();

  radioSetup();
  radio.printDetails();                   // Dump the configuration of the rf unit for debugging

  randomSeed(analogRead(0));
  SetColorForAll(0, 0, 0);
}


void loop() {
  
  while(radio.available(&pipeNo)) {            // Read all available payloads
    radio.read( &gotByte, 1 );
    // Ack payloads are much more efficient than switching to transmit mode to respond to a call
    radio.writeAckPayload(pipeNo, &gotByte, 1 ); // This can be commented out to send empty payloads.
    newstart = millis();
    printf("Sent response %d \n\r", gotByte);
    for(int i = 0; i < numberOfSignals; i++){
      if(gotByte == possibleSignals[i]){
        newsignal = true;
      }
    }
  }
  if(newsignal && millis()-newstart > 80){
    newsignal = false;
    if(gotByte == 1){
      red = false;
      green = false;
      yellow = false;
      Start();
      while(!newsignal){
        SetColorForAll(0, 0, 0);
        CheckCode();
      }
    }
    if(gotByte == 2){
      red = false;
      green = false;
      yellow = false;
      while(!newsignal){
        traffic_light();
      } 
      }
    if(gotByte == 3){
      red = !red;
      green = false;
      yellow = false;
      OnOffRed();
    }
    if(gotByte == 4){
      green = !green;
      red = false;
      yellow = false;
      OnOffGreen();
    }
    if(gotByte == 5){
      yellow = !yellow;
      red = false;
      green = false;
      OnOffYellow();
    }
    if(gotByte == 6){
      red = false;
      green = false;
      yellow = false;
      SetColorForAll(0, 0, 0);
      while(!newsignal){
        Audio();
      }
      //SetColorForAll(0, 0, 0);
    }
    if(gotByte == 255){
      red = false;
      green = false;
      yellow = false;
      while(!newsignal){
        Test();
      }
    }
    thissignal = gotByte;
  }
}  