#ifndef __CAN_MESSAGE_H_
#define __CAN_MESSAGE_H_

#include <Arduino.h>

class CANMessage {

public:
  uint16_t started;
  uint16_t delayed;
  uint16_t repeated;
  uint16_t header;

  uint8_t len;
  uint8_t data[8];

  CANMessage()
  {
  }

  CANMessage( uint16_t _started,
              uint16_t _delayed,
              uint16_t _repeated,
              uint16_t _header,
              uint8_t _len,
              uint8_t _d0,
              uint8_t _d1,
              uint8_t _d2,
              uint8_t _d3,
              uint8_t _d4,
              uint8_t _d5,
              uint8_t _d6,
              uint8_t _d7
            )
  {
    started = _started;
    delayed = _delayed;
    repeated = _repeated;
    header = _header;
    len = _len;
    data[0] = _d0;
    data[1] = _d1;
    data[2] = _d2;
    data[3] = _d3;
    data[4] = _d4;
    data[5] = _d5;
    data[6] = _d6;
    data[7] = _d7;
  }

  void set(
            uint16_t _started,
            uint16_t _delayed,
            uint16_t _repeated,
            uint16_t _header,
            uint8_t _len,
            uint8_t _d0,
            uint8_t _d1,
            uint8_t _d2,
            uint8_t _d3,
            uint8_t _d4,
            uint8_t _d5,
            uint8_t _d6,
            uint8_t _d7)
  {
    started = _started;
    delayed = _delayed;
    repeated = _repeated;
    header = _header;
    len = _len;
    data[0] = _d0;
    data[1] = _d1;
    data[2] = _d2;
    data[3] = _d3;
    data[4] = _d4;
    data[5] = _d5;
    data[6] = _d6;
    data[7] = _d7;
  }

  void print()
  {
    Serial.print(started);
    Serial.print("| ");
    Serial.print(delayed);
    Serial.print(" ");
    Serial.print(repeated);
    Serial.print(" | ");
    Serial.print(header, HEX);
    Serial.print(" | ");
    Serial.print(len);
    Serial.print(" > ");

    for (uint8_t i=0;i<len;i++) {
      Serial.print(" ");
      Serial.print(data[i], HEX);
    }
    Serial.println();
  }

};

#endif // __CAN_MESSAGE_H_
