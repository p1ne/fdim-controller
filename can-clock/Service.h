#ifndef __SERVICE_H_
#define __SERVICE_H_

#include <Arduino.h>
#include <stdarg.h>

uint8_t decToBcd(const uint8_t val)
{
  return( (val/10*16) + (val%10) );
}

uint8_t bcdToDec(const uint8_t val)
{
  return( (val/16*10) + (val%16) );
}

String readSerialString()
{
  String inSerialData;
  char received = ' ';
  Serial.print(F("\n> "));
  while (received != '\r') {
      if (Serial.available() > 0) {
        received = Serial.read();
        inSerialData += received;
        Serial.print(received);
      }
  }
  inSerialData.trim();
  return(inSerialData);
}

uint8_t Day_of_Week(const uint8_t yr, const uint8_t m, const uint8_t d)
{
  uint8_t t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  uint8_t y = yr - m < 3;
  return( (y + y/4 - y/100 + y/400 + t[m-1] + d) % 7 );
}

#endif // __SERVICE_H_
