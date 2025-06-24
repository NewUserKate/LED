#include <Arduino.h>
#include <GyverPower.h>
// #include "nRF24L01.h"
#include "RF24.h"
// #include "printf.h"
#include <Crypto.h>
#include <ChaCha.h>
#include "ControlSum.h"
#include <FastLED.h>
#define CH_NUM 0x60           // Канал, на якому спілкуються радіо
#define SIG_POWER RF24_PA_LOW // Рівень потужності передачі даних
#define SIG_SPEED RF24_1MBPS  // Швидкість передачі даних
#define LED_COUNT 6           // Кількість світлодіодів
#define LED_PIN 8             // Пін, на якому підключені світлодіоди
#define LED_POWER_CTRL A3     // Пін, на якому підключен ключовий транзистор для світлодіодів

RF24 radio(9, 10);
byte addresses[][6] = {"1Node", "2Node"}; // Radio pipe addresses for the 2 nodes to communicate.

uint16_t countForSignals = 1; // Змінна для забезпечення унікальності сигналу

const int numButtons = 6;                               // Кількість кнопок
const byte buttonPins[numButtons] = {2, 3, 4, 5, 6, 7}; // Масив з пінами кнопок
uint32_t T1 = 0, T2 = 0;                                // Змінни для зберігання часу отримання сигналу від кнопки
uint16_t Btn_States[numButtons] = {0};                  // Змінна для зберігання стану кнопок
unsigned long buttonCounts[numButtons] = {0};           // Змінна для зберігання кількості натискань кожної з кнопок
uint8_t TimeInterval = 3;                               // Інтервал, з яким перевіряється стан кнопок
volatile bool anyButtonFlag = false;                    // Флаг, що сигналізує натискання кнопки
volatile byte pendingIndex = 255;                       // Змінна, що зберігає, яка саме кнопка була натиснута

CRGB leds[LED_COUNT]; // Масив для роботи зі світлодіодами

void BtnScan(void) // Функція для з'ясування, чи була натиснута кнопка
{
  int SumStates = 0;
  int maybutton = 255;
  for (int i = 0; i < numButtons; i++)
  {
    Btn_States[i] <<= 1;
  }
  for (int i = 0; i < numButtons; i++)
  {
    Btn_States[i] |= !digitalRead(buttonPins[i]);
  }
  for (int i = 0; i < numButtons; i++)
  {
    if (Btn_States[i] != 0)
    {
      SumStates = __builtin_popcount(Btn_States[i]);
      maybutton = i;
      break;
    }
  }
  if (SumStates == /*16*/ 8)
  {
    pendingIndex = maybutton;
    buttonCounts[pendingIndex]++;
    // Serial.print("Button is pressed! ");
    // Serial.println(pendingIndex + 1);
    // Serial.print("Count: ");
    // Serial.println(buttonCounts[pendingIndex]);
    for (int i = 0; i < numButtons; i++)
    {
      Btn_States[i] = 0;
    }
    anyButtonFlag = false;
    // delay(2);
  }
}

void turnOnLEDs() // Функція для вмикання світлодіодів(щоб ними можна було керувати)
{
  pinMode(LED_POWER_CTRL, OUTPUT);
  digitalWrite(LED_POWER_CTRL, LOW);
  pinMode(LED_PIN, OUTPUT);
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, LED_COUNT);
}

void turnOffLEDs() // Функція для вимикання світлодіодів
{
  FastLED.clear(true);
  pinMode(LED_PIN, INPUT);
  pinMode(LED_POWER_CTRL, INPUT_PULLUP);
}

struct Packet
{                 // Структура, яка буде шифруватися і відправлятися
  int value;      // Команда
  uint16_t count; // Змінна для забезпечення унікальності сигналу
  uint16_t crc;   // Контрольна сума(CRC), яка буде відправлятися разом зі значенням для перевірки цілісності даних
};

uint8_t key[32] = {
    0xA3, 0xF1, 0x94, 0x23, 0x78, 0x99, 0xEF, 0x00,
    0x12, 0x88, 0x42, 0x11, 0xC3, 0x7A, 0x56, 0xD4,
    0x01, 0x98, 0x4F, 0xA6, 0xB1, 0x5C, 0x2A, 0xF3,
    0x34, 0x09, 0xBF, 0xDD, 0x75, 0x6B, 0x8D, 0xE2}; // ключ(один унікальний)
uint8_t nonce[12] = {0};                             // ще один "ключ"; в ідеалі повинен змінюватися

ChaCha chacha;

ISR((PCINT2_vect)) // Функція, яка викликаєтсья у випадку події на пінах PD(з D2 до D7)
{
  anyButtonFlag = true;
}

void radioSetup()
{
  // Setup and configure radio
  radio.begin();
  radio.setAutoAck(1);                    // Ensure autoACK is enabled
  radio.enableAckPayload();               // Allow optional ack payloads
  radio.setRetries(0, 15);                // Smallest time between retries, max no. of retries
  radio.setPayloadSize(1);                // Here we are sending 1-byte payloads to test the call-response speed
  radio.openWritingPipe(addresses[0]);    // Open different pipes when writing. Write on pipe 0, address 0
  radio.openReadingPipe(1, addresses[1]); // Read on pipe 1, as address 1
  radio.setChannel(CH_NUM);               // выбираем канал (в котором нет шумов!)
  radio.setPALevel(SIG_POWER);            // уровень мощности передатчика
  radio.setDataRate(SIG_SPEED);           // скорость обмена
  radio.stopListening();
  // radio.powerUp();
  // radio.printDetails(); // Dump the configuration of the rf unit for debugging
}

void toEncrypt(Packet &pkt) // Шифрування структури
{
  chacha.clear();                                                // очищає внутрішній об'єкт chacha, щоб не плутати нові та старі ключі та nonce
  chacha.setKey(key, sizeof(key));                               // встановлення ключа шифрування (масив із 32 байт (256 біт)); ключ, його розмір
  chacha.setIV(nonce, sizeof(nonce));                            // Встановлює ініціалізаційний вектор; nonce, його розмір (12 байт); для надійності він повинен змінюватися кожне повідомлення
  chacha.encrypt((uint8_t *)&pkt, (uint8_t *)&pkt, sizeof(pkt)); // шифруємо
}

void toDecrypt(Packet &pkt) // Дешифрування структури
{
  chacha.clear();
  chacha.setKey(key, sizeof(key));
  chacha.setIV(nonce, sizeof(nonce));
  chacha.decrypt((uint8_t *)&pkt, (uint8_t *)&pkt, sizeof(pkt));
}

bool checkCRC(Packet pkt, uint16_t check) // Перевірка контрольної суми
{
  if (check == pkt.crc)
  {
    // Serial.println("I've recognised him!");
    // Serial.print("Received: ");
    // Serial.println(pkt.value);
    // delay(2);
    return true;
  }
  else
  {
    // Serial.println("CRC mismatch! Packet dropped.");
    return false;
  }
}

void setup()
{
  power.setSystemPrescaler(PRESCALER_2); // Встановлюємо частоту на Arduino Nano 8 МГц
  // Serial.begin(19200);
  // Serial.println("It's 8");
  //  printf_begin();
  // Serial.println("ROLE: Transmitter");
  radioSetup();
  radio.powerDown();                   // Преводимо радіо у енергозберігаючий режим
  power.autoCalibrate();               // Калібруємо WDT
  power.setSleepMode(POWERDOWN_SLEEP); // Встановлюємо режим сна, який будемо використовувати(найбільш глибокий)
  for (byte i = 0; i < 6; i++)         // Встановлюємо INPUT_PULLUP для всіх пінів з кнопками
  {
    pinMode(buttonPins[i], INPUT_PULLUP);
  }
  turnOnLEDs(); // Вмикаємо та вимикаємо світлодіодм, щоб "проініціалізувати" їх на всякий випадок
  turnOffLEDs();

  PCICR |= B00000100;  // activate PD
  PCMSK2 |= B11111100; // activate PD pins(2, 3, 4, 5, 6, 7)
  delay(30);
}

void loop(void)
{
  PCICR &= ~(1 << PCIE2); // Вимикаємо переривання
  if (anyButtonFlag)
  {
    // Serial.println("I got something");
    unsigned long waitting = millis();
    while (anyButtonFlag && millis() - waitting < 25)
    {
      T2 = millis();
      if ((T2 - T1) >= TimeInterval)
      {
        BtnScan();
        T1 = millis();
      }
    }
    anyButtonFlag = false;
    if (pendingIndex != 255)
    {
      // Serial.println("I will send something");
      int but = pendingIndex;
      pendingIndex = 255;

      radio.powerUp();
      delay(1);
      Packet pkt;
      pkt.value = but + 1;
      pkt.count = countForSignals;
      pkt.crc = crc16((uint8_t *)&pkt, sizeof(pkt) - sizeof(pkt.crc));
      // Serial.print("Sending count: ");
      // Serial.println(pkt.count);
      // Шифрование
      toEncrypt(pkt);
      bool success = false;
      int tries = 0;
      bool mightNeedNewSignal = false;
      do
      {
        if (radio.write(&pkt, sizeof(pkt)))
        {
          // Ждём ответ вручную
          radio.startListening();
          unsigned long startTime = millis();
          bool timeout = false;

          while (!radio.available())
          {
            if (millis() - startTime > /*200*/ 100)
            {
              timeout = true;
              break;
            }
          }

          if (!timeout)
          {
            radio.read(&pkt, sizeof(pkt));
            // Serial.print("I got:");
            // Serial.println(pkt.value);
            // Расшифровка
            toDecrypt(pkt);
            uint16_t check = crc16((uint8_t *)&pkt, sizeof(pkt) - sizeof(pkt.crc));
            if (checkCRC(pkt, check))
            {
              success = true;
              turnOnLEDs();
              leds[but] = CRGB(0, 64, 0); //Світлодіод горить зеленим кольором, якщо вдалося відправити повідомлення та від приймача прийшла відповідь
              FastLED.show();

              if (pkt.value == 128 && mightNeedNewSignal) //Прийняти новий count
              {
                // Serial.println("I'm accepting new count");
                mightNeedNewSignal = false;
                countForSignals = pkt.count;
              }
              countForSignals++;
            }
            // else //На випадок, якщо доведеться перевіряти, чи співпав CRC(світлодіод горить синім, якщо ні)
            // {
            //   // Serial.println("Wrong CRC");
            //   if (tries == 2)
            //   {
            //     turnOnLEDs();
            //     leds[but] = CRGB(0, 0, 64);
            //     FastLED.show();
            //   }
            // }
          }
          else
          {
            // Serial.println("No response (timeout)"); //Якщо не отримав відповідь - горить жовтим
            if (tries == 2)
            {
              turnOnLEDs();
              leds[but] = CRGB(47, 34, 0);
              FastLED.show();
            }
            mightNeedNewSignal = true;
            pkt.value = 128;  // Якщо сигнал дійшов до приймача, але відповіді не було - попросити відправити новий count
            pkt.crc = crc16((uint8_t *)&pkt, sizeof(pkt) - sizeof(pkt.crc));
            toEncrypt(pkt);
          }
        }
        else
        {
          // Serial.println("Send failed.");  //Якщо відповідь не дійшла - горить червоним
          if (tries == 2)
          {
            turnOnLEDs();
            leds[but] = CRGB(64, 0, 0);
            FastLED.show();
          }
        }
        tries++;
        radio.stopListening();
      } while (!success && tries != 3);
      radio.powerDown();
      delay(20);
      unsigned long waitbut = millis();
      while (digitalRead(buttonPins[but]) == LOW && millis() - waitbut < /*1000*/ 500)
        ;
      turnOffLEDs();
    }
  }
  PCICR |= (1 << PCIE2);      // Вмикаємо переривання
  power.sleep(SLEEP_FOREVER); // Засинаємо
}
