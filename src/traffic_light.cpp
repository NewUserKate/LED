#include <Arduino.h>
#include <Adafruit_PWMServoDriver.h>
#include "LED_Control.h"
#include "modes.h"
#include <RF24.h> // ещё библиотека радиомодуля
#include <Crypto.h>
#include <ChaCha.h>
#include "ControlSum.h"
//#include "printf.h"
#define CH_NUM 0x60 // Канал, на якому спілкуються радіо
#define SIG_POWER RF24_PA_LOW // Рівень потужності передачі даних
#define SIG_SPEED RF24_1MBPS  // Швидкість передачі даних
RF24 radio(9, 10);
byte addresses[][6] = {"1Node", "2Node"}; // Radio pipe addresses for the 2 nodes to communicate.

Adafruit_PWMServoDriver pwm1 = Adafruit_PWMServoDriver(0x40);
Adafruit_PWMServoDriver pwm2 = Adafruit_PWMServoDriver(0x41);

LED_Control LED_1(pwm1, 0, 1, 2);
LED_Control LED_2(pwm1, 4, 5, 6);
LED_Control LED_3(pwm1, 8, 9, 10);
LED_Control LED_4(pwm1, 12, 13, 14);

LED_Control LED_5(pwm2, 0, 1, 2);
LED_Control LED_6(pwm2, 4, 5, 6);
LED_Control LED_7(pwm2, 8, 9, 10);

LED_Control AllLEDs[7] = {LED_1, LED_2, LED_3, LED_4, LED_5, LED_6, LED_7};

bool red = false;
bool green = false;
bool yellow = false;

int incoming = 255; //Змінна, що зберігає отриману команду
uint16_t lastCount = 0; //Змінна, що зберігає минулий count
bool lesserCount = false; //Змінна, що свідчить про те, чи є отриманий або минулий сигнал меншим(або дорівнює) тому, що здерігається в змінній lastCount
int rememberLast = 255; //Для зберігання минулої змінної на випадок, якщо count не є новим
bool newsignal = false; //Змінна, що повідомляє про те, чи був отриманий новий сигнал, на який потрібно діяти
unsigned long newstart = 0; // Змінна, що зберігає час отримання сигналу

const int possibleSignals[6] = {1, 2, 3, 4, 5, 6};                                // Масив з кодами, що отримуються від пульта(вказуються ті, що будуть використовуватися)
const int numberOfSignals = sizeof(possibleSignals) / sizeof(possibleSignals[0]); // Кількість можливих сигналів

struct Packet
{ // Структура, яка буде шифруватися і відправлятися
  int value;  //Команда
  uint16_t count; // Змінна для забезпечення унікальності сигналу
  uint16_t crc; // Контрольна сума(CRC), яка буде відправлятися разом зі значенням для перевірки цілісності даних
};

// ChaCha key/nonce
uint8_t key[32] = {
    0xA3, 0xF1, 0x94, 0x23, 0x78, 0x99, 0xEF, 0x00,
    0x12, 0x88, 0x42, 0x11, 0xC3, 0x7A, 0x56, 0xD4,
    0x01, 0x98, 0x4F, 0xA6, 0xB1, 0x5C, 0x2A, 0xF3,
    0x34, 0x09, 0xBF, 0xDD, 0x75, 0x6B, 0x8D, 0xE2}; // ключ(один унікальний)
uint8_t nonce[12] = {0};                             // ще один "ключ"; в ідеалі повинен змінюватися

ChaCha chacha;

void radioSetup()
{ // настройка радио
  radio.begin();
  radio.setAutoAck(1);                    // Ensure autoACK is enabled
  radio.enableAckPayload();               // Allow optional ack payloads
  radio.setRetries(0, 15);                // Smallest time between retries, max no. of retries
  radio.setPayloadSize(1);                // Here we are sending 1-byte payloads to test the call-response speed
  radio.openWritingPipe(addresses[1]);    // Since only two radios involved, both listen on the same addresses and pipe numbers in RX mode
  radio.openReadingPipe(1, addresses[0]); // then switch pipes & addresses to transmit.
  radio.setChannel(CH_NUM);               // выбираем канал (в котором нет шумов!)
  radio.setPALevel(SIG_POWER);            // уровень мощности передатчика
  radio.setDataRate(SIG_SPEED);           // скорость обмена
  radio.powerUp();
  radio.printDetails();   // Dump the configuration of the rf unit for debugging
  radio.startListening(); // Need to start listening after opening new reading pipes
}

void toEncrypt(Packet &pkt) //Шифрування структури
{
  chacha.clear();                                                // очищає внутрішній об'єкт chacha, щоб не плутати нові та старі ключі та nonce
  chacha.setKey(key, sizeof(key));                               // встановлення ключа шифрування (масив із 32 байт (256 біт)); ключ, його розмір
  chacha.setIV(nonce, sizeof(nonce));                            // Встановлює ініціалізаційний вектор; nonce, його розмір (12 байт); для надійності він повинен змінюватися кожне повідомлення
  chacha.encrypt((uint8_t *)&pkt, (uint8_t *)&pkt, sizeof(pkt)); // шифруємо
}

void toDecrypt(Packet &pkt) //Дешифрування структури
{
  chacha.clear();
  chacha.setKey(key, sizeof(key));
  chacha.setIV(nonce, sizeof(nonce));
  chacha.decrypt((uint8_t *)&pkt, (uint8_t *)&pkt, sizeof(pkt));
}

bool isNewPacket(uint16_t counter)  //Функція, яка визначає, чи є цей пакет новим
{
  uint16_t diff = counter - lastCount;
  return (diff > 0 && diff < 10); 
}

bool workOnNewKnownSignal(int value)  //Функція, яка визначає, чи є цей сигнал відомим
{
  Serial.print("Received: ");
  Serial.println(value);
  for (int i = 0; i < numberOfSignals; i++)
  {
    if (value == possibleSignals[i] && millis() - newstart > 100)
    {
      newstart = millis();
      newsignal = true;
      incoming = value;
      return true;
    }
  }
  return false;
}

bool acceptAndReply()
{
  bool newCode = false;
  if (radio.available())
  {
    Packet pkt;
    radio.read(&pkt, sizeof(pkt));
    radio.stopListening();
    Serial.println("I got a new signal");
    // Расшифровка
    toDecrypt(pkt);
    uint16_t check = crc16((uint8_t *)&pkt, sizeof(pkt) - sizeof(pkt.crc));
    if (check == pkt.crc)
    {
      if (pkt.value == 128 && lesserCount && rememberLast != 0) //Якщо ми отримали код 128, lastCount при минулій перевірці виявився не меншим за count і им щось запам'ятали
      {
        Serial.println("I have to give a new count!");
        pkt.count = lastCount;
        lesserCount = false;
        newCode = workOnNewKnownSignal(rememberLast); //Опрацювати rememberLast та у подальшому виконати команду, яка була
        rememberLast = 0;
        goto sending; //Відправити новий count пульту
      }
      Serial.print("The lastCount is: ");
      Serial.println(lastCount);
      Serial.print("The current is: ");
      Serial.println(pkt.count);
      if (isNewPacket(pkt.count))
      {
        Serial.println("Last count is lesser than current count: correct");
        lastCount = pkt.count;
        rememberLast = 0;
        lesserCount = false;
        newCode = workOnNewKnownSignal(pkt.value);
      }
      else
      {
        Serial.println("Last count isn't lesser than current count: incorrect");
        rememberLast = pkt.value;
        lesserCount = true;
        //newsignal = false;
        radio.startListening();
        //return false;
        return newCode;
      }
      // newCode = workOnNewKnownSignal(signal);
    }
    else
    {
      Serial.println("CRC mismatch! Packet dropped.");
      radio.startListening();
      return newCode;
    }

  sending:
    // Подготовка ответа
    if(!newsignal)
    {
      Serial.println("Unknown code");
      Serial.println(pkt.value);
      Serial.println(rememberLast);
      radio.startListening();
      return newsignal;
    }
    pkt.crc = crc16((uint8_t *)&pkt, sizeof(pkt) - sizeof(pkt.crc));
    Serial.print("Sent count: ");
    Serial.println(pkt.count);
    // Шифрование
    toEncrypt(pkt);
    
    if(!radio.write(&pkt, sizeof(pkt))){
      Serial.println("Reply failed");
    }

    radio.startListening(); // Снова ждём новых пакетов
  }
  return newCode;
}

void myDelay(int duration)  // myDelay створена, щоб під час паузи
{                           // можна було отримати новий сигнал з пульта та запустити інший режим
  for (int i = 0; i < duration; i++)
  { 
    if (i % 50 == 0)
    {
      if (acceptAndReply())
      {
        Serial.println("skip");
        break;
      }
    }
    delay(1);
  }
}

void setup()
{
  pwm1.begin();
  pwm1.setOutputMode(false);
  pwm1.setPWMFreq(1000);
  pwm2.begin();
  pwm2.setOutputMode(false);
  pwm2.setPWMFreq(1000);
  Serial.begin(9600);
  //printf_begin();

  radioSetup();

  randomSeed(analogRead(0));
  SetColorForAll(0, 0, 0);
}

void loop()
{
  acceptAndReply();
  if (newsignal && millis() - newstart > 100)
  {
    newsignal = false;
    switch (incoming)
    {
    case 1:
      red = false;
      green = false;
      yellow = false;
      Start();
      while (!newsignal)
      {
        acceptAndReply();
        SetColorForAll(0, 0, 0);
      }
      break;
    case 2:
      red = false;
      green = false;
      yellow = false;
      while (!newsignal)
      {
        acceptAndReply();
        traffic_light();
      }
      break;
    case 3:
      red = !red;
      green = false;
      yellow = false;
      OnOffRed();
      break;
    case 4:
      green = !green;
      red = false;
      yellow = false;
      OnOffGreen();
      break;
    case 5:
      yellow = !yellow;
      red = false;
      green = false;
      OnOffYellow();
      break;
    case 6:
      red = false;
      green = false;
      yellow = false;
      SetColorForAll(0, 0, 0);
      while (!newsignal)
      {
        acceptAndReply();
        Audio();
      }
      break;
      // if(incoming == 255){
      //   red = false;
      //   green = false;
      //   yellow = false;
      //   SetColorForAll(4095, 0, 0); //Щоб відрізнити перший режим Test() від режиму Start()
      //   myDelay(2000);
      //   SetColorForAll(0, 0, 0);
      //   myDelay(1000);
      //   SetColorForAll(4095, 0, 0);
      //   myDelay(1000);
      //   while(!newsignal){
      //     Test();
      //   }
      // }
    }
  }
}