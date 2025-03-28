#include <Arduino.h>                                      
/*   Данный скетч делает следующее: передатчик (TX) отправляет массив
     данных, который генерируется согласно показаниям с кнопки и с
     двух потенциомтеров. Приёмник (RX) получает массив, и записывает
     данные на реле, сервомашинку и генерирует ШИМ сигнал на транзистор.
    by AlexGyver 2016
*/

#include <RF24.h>        // ещё библиотека радиомодуля
#define CH_NUM 0x60   // номер канала (должен совпадать с приёмником)
#define SIG_POWER RF24_PA_LOW
#define SIG_SPEED RF24_1MBPS

RF24 radio(9, 10);
byte addresses[][6] = {"1Node", "2Node"}; // Radio pipe addresses for the 2 nodes to communicate.


byte counter = 1;                                                          // A single byte to keep track of the data being sent back and forth

const int buttonPins[] = {2, 3, 4, 5, 6, 7};  // Пины кнопок
const int numButtons = 6; //Кількість кнопок відома

int serial_putc( char c, FILE * ) {
  Serial.write( c );
  return c;
}

void printf_begin(void) {
  fdevopen( &serial_putc, 0 );
}

void radioSetup() {
  radio.begin();              // активировать модуль
  radio.setAutoAck(1);        // режим подтверждения приёма, 1 вкл 0 выкл
  radio.setRetries(0, 15);    // (время между попыткой достучаться, число попыток)
  radio.enableAckPayload();   // разрешить отсылку данных в ответ на входящий сигнал
  radio.setPayloadSize(32);   // размер пакета, в байтах
  radio.openWritingPipe(addresses[0]);   // мы - труба 0, открываем канал для передачи данных
  radio.setChannel(CH_NUM);            // выбираем канал (в котором нет шумов!)
  radio.setPALevel(SIG_POWER);         // уровень мощности передатчика
  radio.setDataRate(SIG_SPEED);        // скорость обмена
  // должна быть одинакова на приёмнике и передатчике!
  // при самой низкой скорости имеем самую высокую чувствительность и дальность!!

  radio.powerUp();         // начать работу
  radio.stopListening();   // не слушаем радиоэфир, мы передатчик
}

void setup() {

  Serial.begin(9600);
  printf_begin();
  radio.printDetails();                   // Dump the configuration of the rf unit for debugging
  radioSetup();

  for (int i = 0; i < numButtons; i++) {
    pinMode(buttonPins[i], INPUT);
  }
}

void loop(void) {                                        
     byte pressedButton = 0;  //Змінна, що приймає сигнал з кнопок

    // Проверяем, какие кнопки нажаты
    for (int i = 0; i < numButtons; i++) {
        if (digitalRead(buttonPins[i]) == HIGH) {  // LOW — кнопка нажата
          delay(25);
          pressedButton = i + 1; // Нумерация кнопок с 1
        }
    }
    // Якщо затиснуті дві крайні кнопки-режим Test
    if (digitalRead(buttonPins[0]) == HIGH && digitalRead(buttonPins[numButtons - 1]) == HIGH) {
        pressedButton = 255;
    }
      if(pressedButton != 0){
        byte gotByte = 0;                                           // Initialize a variable for the incoming response
    printf("Now sending %d as payload. ", pressedButton);         // Use a simple byte counter as payload
    unsigned long time = micros();                          // Record the current microsecond count
        
    if ( radio.write(&pressedButton, 1) ) {                       // Send the counter variable to the other radio
      delay(25);
      if (!radio.available()) {                           // If nothing in the buffer, we got an ack but it is blank
        printf("Got blank response. round-trip delay: %lu microseconds\n\r", micros() - time);
            Serial.println("Reading again");  //Якщо AckPayload прийшов пустим, спробувати відправити сигнал ще раз та прочитати
            if ( radio.write(&pressedButton, 1) ) { 
              while (radio.available() ) {
                radio.read( &gotByte, 1 );
            }
          printf("Got response %d, round-trip delay: %lu microseconds\n\r", gotByte, micros() - time);
          delay(500);
        }
      } else {
        while (radio.available() ) {                    // If an ack with payload was received
          radio.read( &gotByte, 1 );                  // Read it, and display the response time
          while(gotByte!=pressedButton){            //Із-за того, що другий AckPayload буде завжди приходити пустим, далі транслятор завжди отримуватиме 
            delay(25);                              // AckPayload з минулого сигналу, тому якщо сигнал підтвердження не співпадає з натиснутою кнопкою-
            Serial.println("Reading again");        //спробувати відправити сигнал ще раз та прочитати
            if ( radio.write(&pressedButton, 1) ) {
              while (radio.available() ) {
                radio.read( &gotByte, 1 );
              }
            }
          }
          printf("Got response %d, round-trip delay: %lu microseconds\n\r", gotByte, micros() - time);
          delay(300);
        }
      }

    } else {
      printf("Sending failed.\n\r");  // If no ack response, sending failed
    }

    delay(200);  
    }

  }
    