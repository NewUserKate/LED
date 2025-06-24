#pragma once
#include <Arduino.h>

//uint8_t crc8(const uint8_t *data, size_t len);
uint16_t crc16(const uint8_t *data, size_t len);

// struct Packet
// { // Структура того, что будет шифроватся и отправляться
//   int value;
//   uint8_t crc; // Контрольная сума(CRC), которая будет отправлятся вместе со значением для проверки целостности данных
// };


