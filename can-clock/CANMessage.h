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

  CANMessage( const uint16_t _started,
              const uint16_t _delayed,
              const uint16_t _repeated,
              const uint16_t _header,
              const uint8_t _len,
              const uint8_t _d0,
              const uint8_t _d1,
              const uint8_t _d2,
              const uint8_t _d3,
              const uint8_t _d4,
              const uint8_t _d5,
              const uint8_t _d6,
              const uint8_t _d7
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

  void set( const uint16_t _started,
            const uint16_t _delayed,
            const uint16_t _repeated,
            const uint16_t _header,
            const uint8_t _len,
            const uint8_t _d0,
            const uint8_t _d1,
            const uint8_t _d2,
            const uint8_t _d3,
            const uint8_t _d4,
            const uint8_t _d5,
            const uint8_t _d6,
            const uint8_t _d7
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

  void print() const
  {
    Serial.print(started);
    Serial.print(F("| "));
    Serial.print(delayed);
    Serial.print(F(" "));
    Serial.print(repeated);
    Serial.print(F(" | "));
    Serial.print(header, HEX);
    Serial.print(F(" | "));
    Serial.print(len);
    Serial.print(F(" > "));

    for (uint8_t i=0;i<len;i++) {
      Serial.print(F(" "));
      Serial.print(data[i], HEX);
    }
    Serial.println();
  }

};

#endif // __CAN_MESSAGE_H_
