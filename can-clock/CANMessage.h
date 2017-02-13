#include <Arduino.h>

class CANMessage {

public:
  uint32_t started;
  uint32_t delayed;
  uint32_t repeated;
  uint32_t header;

  byte len;
  byte data[8];

  CANMessage()
  {
  }

  CANMessage( uint32_t _started,
              uint32_t _delayed,
              uint32_t _repeated,
              uint32_t _header,
              byte _len,
              byte _d0,
              byte _d1,
              byte _d2,
              byte _d3,
              byte _d4,
              byte _d5,
              byte _d6,
              byte _d7
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
            uint32_t _started,
            uint32_t _delayed,
            uint32_t _repeated,
            uint32_t _header,
            byte _len,
            byte _d0,
            byte _d1,
            byte _d2,
            byte _d3,
            byte _d4,
            byte _d5,
            byte _d6,
            byte _d7)
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
    Serial.println("****");
    Serial.print(started);
    Serial.print(" ");
    Serial.print(delayed);
    Serial.print(" ");
    Serial.print(repeated);
    Serial.print(" ");
    Serial.print(header);
    Serial.print(" ");
    Serial.print(len);
    Serial.print(" ");
    Serial.print(data[0]);
    Serial.print(" ");
    Serial.print(data[1]);
    Serial.print(" ");
    Serial.print(data[2]);
    Serial.print(" ");
    Serial.print(data[3]);
    Serial.print(" ");
    Serial.print(data[4]);
    Serial.print(" ");
    Serial.print(data[5]);
    Serial.print(" ");
    Serial.print(data[6]);
    Serial.print(" ");
    Serial.print(data[7]);
    Serial.println(" ");
  }

};
