#include <Arduino.h>
#include <SPI.h>
#include <avr/pgmspace.h>
#include <Wire.h>
#include <SoftwareSerial.h>
#include <mcp_can.h>
#include <mcp_can_dfs.h>
#include "CANMessage.h"
#include "FormattedString.h"

#define DEBUG 1

#define TIMER_STEP 25

#define START_COUNT 8
#define MSG_COUNT 4
#define TEXT_COUNT 12

#define TEXT_MSG_LENGTH 14

#define TZ 3

#undef MQ135_CONNECTED

byte second, minute, hour;

SoftwareSerial mySerial(8, 9); // RX, TX

FormattedString fl, fr, rl, rr, message, rpm, carSpeed, temperature;

String dump[8];

unsigned int timer;

CANMessage start[START_COUNT];
CANMessage cycle[MSG_COUNT];
CANMessage text[TEXT_COUNT];
CANMessage metric;

void initStartMessages()
{
  start[0].set( 0, 100, 0, 0x50c, 3, 0x0C, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 );
  start[1].set( 0, 250, 0, 0x3e8, 8, 0x00, 0x00, 0x29, 0x00, 0x00, 0x00, 0x00, 0x00 );
  start[2].set( 0,  50, 0, 0x3ef, 8, 0x32, 0x32, 0x32, 0x32, 0x03, 0x00, 0x00, 0x00 );
  start[3].set( 0,  50, 0, 0x3f2, 8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x60, 0x00, 0x00 );
  start[4].set( 0, 100, 0, 0x50c, 3, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 );
  start[5].set( 0,  50, 0, 0x3f2, 8, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xF0, 0x00, 0x00 );
  start[6].set( 0,  50, 0, 0x3f1, 8, 0xF5, 0x90, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 );
  start[7].set( 0, 100, 0, 0x50c, 3, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 );
}

void initCycleMessages()
{
  cycle[0].set( 0,   0,  500, 0x50c, 3, 0x11, 0x02, 0x00, 0xBE, 0xBE, 0xBE, 0xBE, 0xBE );
  cycle[1].set( 0, 400, 1000, 0x3e8, 8, 0x0F, 0x00, 0x29, 0x04, 0x00, 0x00, 0x00, 0x00 );
  cycle[2].set( 0, 450, 1000, 0x3ef, 8, 0x32, 0x32, 0x32, 0x32, 0x03, 0x00, 0x00, 0x20 );
  cycle[3].set( 0, 500, 1000, 0x3f2, 8, 0x12, 0x01, 0xFF, 0xFF, 0xFF, 0xF0, 0x00, 0x00 );
}

void initTextMessages()
{
  text[ 0].set( 1000,   0, 500, 0x336, 8, 0x03, 0x01, 0x0A, 0x01, 0xFE, 0x00, 0x00, 0x00 );
  text[ 1].set( 1000,  50, 500, 0x324, 8, 0x01, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00 );
  text[ 2].set( 1000, 100, 500, 0x337, 8, 0x06, 0x20,  ' ',  '@',  ' ',  '@',  ' ', 0x00 );
  text[ 3].set( 1000, 200, 500, 0x336, 8, 0x03, 0x01, 0x05, 0x03, 0x03, 0x00, 0x00, 0x00 );
  text[ 4].set( 1000, 225, 500, 0x324, 8, 0x03, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00 );
  text[ 5].set( 1000, 250, 500, 0x337, 8, 0x10, 0x2B,  ' ',  ' ',  ' ',  '@',  ' ',  ' ' );
  text[ 6].set( 1000, 275, 500, 0x337, 8, 0x21,  ' ',  'M',  'e',  'r',  'c',  '@',  'u' );
  text[ 7].set( 1000, 300, 500, 0x337, 8, 0x22,  'r',  'y',  ' ',  ' ',  ' ',  ' ',  ' ' );
  text[ 8].set( 1000, 325, 500, 0x337, 8, 0x23,  ' ',  ' ',  ' ',  ' ',  ' ',  '@',  ' ' );
  text[ 9].set( 1000, 350, 500, 0x337, 8, 0x24,  ' ',  ' ',  'M',  'a',  'r',  'i',  'n' );
  text[10].set( 1000, 375, 500, 0x337, 8, 0x25,  'e',  'r',  ' ',  ' ',  ' ',  ' ',  ' ' );
  text[11].set( 1000, 400, 500, 0x337, 8, 0x26,  ' ',  ' ', 0x00, 0x00, 0x00, 0x00, 0x00 );
}

void initMetricMessage()
{
  metric.set( 0, 0, 0, 0x129, 8, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
}

const int SPI_CS_PIN = 10;

bool rcvFlag = false;
unsigned char rcvLen = 0;
unsigned long rcvCanId = 0x0;
unsigned char rcvBuf[8];
bool sendingNow = false;

unsigned int sentOnTick = 0;

bool firstCycle = true;
bool gotClock = false;

unsigned char currentText = 0;

MCP_CAN CAN(SPI_CS_PIN);

void MCP2515_ISR()
{
    rcvFlag = true;
}

byte decToBcd(byte val)
{
  return( (val/10*16) + (val%10) );
}
byte bcdToDec(byte val)
{
  return( (val/16*10) + (val%16) );
}

void printDebug(unsigned int timer, CANMessage msg)
{
    Serial.print("Time: ");
    Serial.print(timer);
    Serial.print(" Message: ");
    msg.print();
}

void displayText(byte strNo, String str)
{
  byte curLine, curChar, numChars;

  if (DEBUG) {
    Serial.println(String(strNo) + ": " + str);
  }
  switch (strNo) {
    case 0:
      curLine = 2;
      curChar = 2;
      numChars = 3;
      break;
    case 1:
      curLine = 5;
      curChar = 2;
      numChars = 20;
      break;
    case 2:
      curLine = 8;
      curChar = 3;
      numChars = 20;
      break;
  }

  for (byte i=0;i<numChars;i++) {
    if (
          ( curLine == 2 && curChar == 3 ) ||
          ( curLine == 2 && curChar == 5 ) ||
          ( curLine == 5 && curChar == 5 ) ||
          ( curLine == 6 && curChar == 6 ) ||
          ( curLine == 8 && curChar == 6 )
       ) {
      text[curLine].data[curChar] = '@';
      curChar++;
    }

    text[curLine].data[curChar] = i<str.length() ? str[i]:' ';

    curChar++;

    if ( curChar == 8 ) {
      curLine++;
      curChar = 1;
    }
  }
}

void sendStartSequence()
{
  delay(1000);
  timer = 0;

  for (int i=0;i<START_COUNT;i++) {
    CAN.sendMsgBuf(start[i].header, 0, start[i].len, start[i].data);
    printDebug(timer, start[i]);
    delay(start[i].delayed);
    timer = timer + start[i].header;
  }
  timer = 0;
  firstCycle = true;
  currentText = 0;
  delay(500);
}

void setup() {
  Serial.begin(115200);
  mySerial.begin(9600);

  initStartMessages();
  initCycleMessages();
  initTextMessages();
  initMetricMessage();

  pinMode(A4, INPUT);
  pinMode(2, INPUT);

START_INIT:
if(CAN_OK == CAN.begin(CAN_125KBPS, MCP_8MHz))
    {
        Serial.println("CAN ok!");
    }
    else
    {
        Serial.println("CAN fail");
        delay(100);
        goto START_INIT;
    }

  #if defined(__AVR_ATmega32U4__) // Arduino Pro Micro
    pinMode(4, INPUT);
    attachInterrupt(digitalPinToInterrupt(4), MCP2515_ISR, FALLING); // start interrupt
  #else // Other Arduinos (Nano in my case)
    pinMode(3, INPUT);
    attachInterrupt(digitalPinToInterrupt(3), MCP2515_ISR, FALLING); // start interrupt
  #endif

  CAN.init_Mask(0, CAN_STDID, 0x7FF);   // there are 2 mask in mcp2515, you need to set both of them
  CAN.init_Mask(1, CAN_STDID, 0x7FF);
  CAN.init_Filt(0, CAN_STDID, 0x423);   // Speed data
  CAN.init_Filt(1, CAN_STDID, 0x3B5);   // TPMS data
  CAN.init_Filt(2, CAN_STDID, 0x466);   // GPS

  timer = 0;
  delay(500);

  //CAN.sendMsgBuf(metric.header, 0, metric.len, metric.data);
}

void loop() {
  String inSerialData;
  if (rcvFlag) {
    rcvFlag = false;
    while (CAN_MSGAVAIL == CAN.checkReceive()) {

      CAN.readMsgBuf(&rcvLen, rcvBuf);
      rcvCanId = CAN.getCanId();

      switch (rcvCanId) {
        case 0x3b5: { // TPMS
            fl = rcvBuf[0] > 25 ? String(rcvBuf[0]) : "LO";
            fr = rcvBuf[1] > 25 ? String(rcvBuf[1]) : "LO";
            rr = rcvBuf[2] > 25 ? String(rcvBuf[2]) : "LO";
            rl = rcvBuf[3] > 25 ? String(rcvBuf[3]) : "LO";
        }
          break;
        case 0x423: { // Speed, RPM
          rpm = String(( ( rcvBuf[2] << 8 ) + rcvBuf[3] ) / 4);
          carSpeed = String(round((( rcvBuf[0] << 8) + rcvBuf[1])/100) - 100, 0);
          temperature = String(rcvBuf[4]-40);

          if ( ((rpm == "0") || (rpm == "")) && sendingNow) {
            carSpeed = "";
            rpm = "";
            temperature = "";
            message = "";
            sendingNow = false;
            gotClock = false;
          }

          if ((rpm != "0") && (rpm != "") && !sendingNow) {
            sendingNow = true;
            sendStartSequence();
          }
        }
          break;
        case 0x466: {  // GPS clock
            hour = (((rcvBuf[0] & 0xF8) >> 3) + TZ ) % 24;
            minute = (rcvBuf[1] & 0xFC) >> 2;
            second = (rcvBuf[2] & 0xFC) >> 2;
            gotClock = true;
        }
          break;
      }
    }
  }

  if (sendingNow || DEBUG) {
    for (int currentCycle = 0; currentCycle < MSG_COUNT; currentCycle ++ ) {
      if ( ( (timer >= cycle[currentCycle].started ) || (!firstCycle) ) && ((timer % cycle[currentCycle].repeated) - cycle[currentCycle].delayed) == 0) {
        if (cycle[currentCycle].header == 0x3f2) {
          cycle[currentCycle].data[0] = decToBcd(hour);
          cycle[currentCycle].data[1] = decToBcd(minute);
          if (gotClock) {
            CAN.sendMsgBuf(cycle[currentCycle].header, 0, cycle[currentCycle].len, cycle[currentCycle].data);
            sentOnTick = timer;
            printDebug(timer, cycle[currentCycle]);
          }
        } else {
          CAN.sendMsgBuf(cycle[currentCycle].header, 0, cycle[currentCycle].len, cycle[currentCycle].data);
          sentOnTick = timer;
          printDebug(timer, cycle[currentCycle]);
        }
      }
    }
    inSerialData = "";

    while (mySerial.available() > 0) {
        char recieved = mySerial.read();
        inSerialData += recieved;

        if (recieved == '\n')
        {
          message = inSerialData;
          inSerialData = "";
        }
    }


#if defined(MQ135_CONNECTED)    // For MQ135
    int sensorValue = analogRead(4);
    message = String(sensorValue, DEC);
#endif // MQ135_CONNECTED

    if (message == "%MTRACK") message = "";
    if ( ( (timer >= text[currentText].started ) || (!firstCycle) ) && ((timer % text[currentText].repeated) - text[currentText].delayed) == 0) {
      if (currentText == 0) {
        displayText(0, carSpeed.padRight(3));
        displayText(1, fl.padRight(2) + " RPM:" + rpm.padRight(4) + " T:" + temperature.padRight(3) + " " + fr.padRight(2));
        displayText(2, rl.padRight(2) + " " + message.padCenter(TEXT_MSG_LENGTH) + " " + rr.padRight(2));
      }
      if (sentOnTick == timer) delay(TIMER_STEP/2);
      CAN.sendMsgBuf(text[currentText].header, 0, text[currentText].len, text[currentText].data);
      printDebug(timer, text[currentText]);
      currentText = (currentText + 1) % TEXT_COUNT;
    }

    delay(TIMER_STEP);
    timer += TIMER_STEP;
    if (timer >= 32000) {
      timer = 0;
      firstCycle = false;
    }
  }
}
