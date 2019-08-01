#ifndef __FORD_MESSAGES_H_
#define __FORD_MESSAGES_H_

#include "CANMessage.h"
#include "Settings.h"

const uint8_t MAX_START_COUNT = 8;
const uint8_t MAX_MSG_COUNT   = 4;
const uint8_t MAX_TEXT_COUNT  = 12;

const uint8_t TPMS_COUNT      = 4;
const uint8_t TPMS_INIT_COUNT = 1;

const uint8_t TPMS_FRONT      = 0;
const uint8_t TPMS_REAR       = 1;
const uint8_t TPMS_TEMP       = 2;
const uint8_t TPMS_INIT       = 3;

const uint8_t TIRE_FL         = 0;
const uint8_t TIRE_FR         = 1;
const uint8_t TIRE_RL         = 3;
const uint8_t TIRE_RR         = 2;
const uint8_t TIRES           = 4;

uint8_t START_COUNT           = 0;
uint8_t MSG_COUNT             = 0;
uint8_t TEXT_COUNT            = 0;

CANMessage start[ MAX_START_COUNT ];
CANMessage cycle[ MAX_MSG_COUNT ];
CANMessage text[ MAX_TEXT_COUNT ];
//CANMessage metric;

CANMessage tpms[ TPMS_COUNT + TPMS_INIT_COUNT ];

void initStartMessages()
{
  START_COUNT = 0;

  start[START_COUNT++].set( 0, 100, 0, 0x50c, 3, 0x0C, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 );

  // We only need to send 3e8 messages on aftermarket HU, because both stock and chinese send it for us
  if (  (currentSettings.huType == HU_CHINESE_WITH_CAN_SIMPLE) ||
        (currentSettings.huType == HU_CHINESE_WITH_CAN_EXTENDED) ) {
    start[START_COUNT++].set( 0, 250, 0, 0x3e8, 8, 0x00, 0x00, 0x04, 0x24, 0x00, 0x00, 0x00, 0x00 );
  } else if (currentSettings.huType == HU_AFTERMARKET) {
    start[START_COUNT++].set( 0, 250, 0, 0x3e8, 8, 0x00, 0x00, 0x29, 0x00, 0x00, 0x00, 0x00, 0x00 );
  }

  start[START_COUNT++].set( 0,  50, 0, 0x3ef, 8, 0x32, 0x32, 0x32, 0x32, 0x03, 0x00, 0x00, 0x00 );

  if ( wantClock() )
    start[START_COUNT++].set( 0,  50, 0, 0x3f2, 8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x60, 0x00, 0x00 );
  else if (currentSettings.huType != HU_STOCK)
    start[START_COUNT++].set( 0, 50, 0, 0x3f2, 8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00 ); // empty clock

  start[START_COUNT++].set( 0, 100, 0, 0x50c, 3, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 );

  if ( wantClock() )
    start[START_COUNT++].set( 0,  50, 0, 0x3f2, 8, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xF0, 0x00, 0x00 );
  else if (currentSettings.huType != HU_STOCK)
    start[START_COUNT++].set( 0,  50, 0, 0x3f2, 8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00 ); // empty clock

  start[START_COUNT++].set( 0,  50, 0, 0x3f1, 8, 0xF5, 0x90, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 );
  start[START_COUNT++].set( 0, 100, 0, 0x50c, 3, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 );

}

void initCycleMessages()
{
  MSG_COUNT = 0;

  cycle[MSG_COUNT++].set( 0,   0,  500, 0x50c, 3, 0x11, 0x02, 0x00, 0xBE, 0xBE, 0xBE, 0xBE, 0xBE );

  if (  (currentSettings.huType == HU_CHINESE_WITH_CAN_EXTENDED) ||
        (currentSettings.huType == HU_CHINESE_WITH_CAN_EXTENDED) ) {
    cycle[MSG_COUNT++].set( 0, 250, 0, 0x3e8, 8, 0x00, 0x00, 0x04, 0x24, 0x00, 0x00, 0x00, 0x00 );
  } else if (currentSettings.huType == HU_AFTERMARKET) {
    cycle[MSG_COUNT++].set( 0, 400, 1000, 0x3e8, 8, 0x0F, 0x00, 0x29, 0x04, 0x00, 0x00, 0x00, 0x00 );
  }

  cycle[MSG_COUNT++].set( 0, 450, 1000, 0x3ef, 8, 0x32, 0x32, 0x32, 0x32, 0x03, 0x00, 0x00, 0x20 );

  if (currentSettings.huType != HU_STOCK) {
    if ( wantClock() && ( currentSettings.huType == HU_AFTERMARKET) )
      cycle[MSG_COUNT++].set( 0, 500, 1000, 0x3f2, 8, 0x12, 0x01, 0xFF, 0xFF, 0xFF, 0xF0, 0x00, 0x00 );
    else
      cycle[MSG_COUNT++].set( 0, 500, 1000, 0x3f2, 8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00 ); // empty clock
  }
}

void initTextMessages()
{
  TEXT_COUNT = 0;

  text[TEXT_COUNT++].set( 1000,   0, 500, 0x336, 8, 0x03, 0x01, 0x0A, 0x01, 0xFE, 0x00, 0x00, 0x00 );
  text[TEXT_COUNT++].set( 1000,  50, 500, 0x324, 8, 0x01, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00 );
  text[TEXT_COUNT++].set( 1000, 100, 500, 0x337, 8, 0x06, 0x20,  ' ',  '@',  ' ',  '@',  ' ', 0x00 );
  text[TEXT_COUNT++].set( 1000, 200, 500, 0x336, 8, 0x03, 0x01, 0x05, 0x03, 0x03, 0x00, 0x00, 0x00 );
  text[TEXT_COUNT++].set( 1000, 225, 500, 0x324, 8, 0x03, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00 );
  text[TEXT_COUNT++].set( 1000, 250, 500, 0x337, 8, 0x10, 0x2B,  ' ',  ' ',  ' ',  '@',  ' ',  ' ' );
  text[TEXT_COUNT++].set( 1000, 275, 500, 0x337, 8, 0x21,  ' ',  'M',  'e',  'r',  'c',  '@',  'u' );
  text[TEXT_COUNT++].set( 1000, 300, 500, 0x337, 8, 0x22,  'r',  'y',  ' ',  ' ',  ' ',  ' ',  ' ' );
  text[TEXT_COUNT++].set( 1000, 325, 500, 0x337, 8, 0x23,  ' ',  ' ',  ' ',  ' ',  ' ',  '@',  ' ' );
  text[TEXT_COUNT++].set( 1000, 350, 500, 0x337, 8, 0x24,  ' ',  ' ',  'M',  'a',  'r',  'i',  'n' );
  text[TEXT_COUNT++].set( 1000, 375, 500, 0x337, 8, 0x25,  'e',  'r',  ' ',  ' ',  ' ',  ' ',  ' ' );
  text[TEXT_COUNT++].set( 1000, 400, 500, 0x337, 8, 0x26,  ' ',  ' ', 0x00, 0x00, 0x00, 0x00, 0x00 );
}

/*
void initMetricMessage()
{
  metric.set( 0, 0, 0, 0x129, 8, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
}
*/

void initTpmsMessages()
{
  tpms[0].set( 0,   0,  5000, 0x726, 8, 3, 0x22, 0x41, 0x40, 0x00, 0x00, 0x00, 0x00 );  // Front
  tpms[1].set( 0, 200,  5000, 0x726, 8, 3, 0x22, 0x41, 0x41, 0x00, 0x00, 0x00, 0x00 );  // Rear
  tpms[2].set( 0, 400, 10000, 0x726, 8, 3, 0x22, 0x41, 0x60, 0x00, 0x00, 0x00, 0x00 );  // Temperature
  tpms[3].set( 0,   0,     0, 0x726, 8, 2, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 );  // Initialization
}

#endif // __FORD_MESSAGES_H_
