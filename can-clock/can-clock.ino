#include <Arduino.h>
#include <SPI.h>
#include <avr/pgmspace.h>
#include <Wire.h>
#include <SoftwareSerial.h>
#include <mcp_can.h>
#include <mcp_can_dfs.h>
#include "RTClib.h"
#include <EEPROM.h>

#include "CANMessage.h"
#include "FordMessages.h"
#include "FormattedString.h"
#include "Service.h"
#include "Settings.h"

#define DEBUG 0

#define TIMER_STEP 25

#undef MQ135_CONNECTED

byte second, minute, hour;

SoftwareSerial mySerial(8, 9); // RX, TX

FormattedString fl, fr, rl, rr, message, rpm, carSpeed, temperature;

byte pressurePadding;
String rpmMessage;
byte textMsgLength;

unsigned int timer;

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

  readSettings();

#if defined(MQ135_CONNECTED)
  pinMode(A4, INPUT);
#endif


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

  if (currentSettings.displayPressure) {
    CAN.init_Filt(1, CAN_STDID, 0x3B5);   // TPMS data
  }

  if (currentSettings.useRTC) {
    if (! rtc.begin()) {
      Serial.println("Couldn't find RTC");
      while (1);
    }
  } else {
    CAN.init_Filt(2, CAN_STDID, 0x466);   // GPS
  }

  timer = 0;
  delay(500);

  CAN.sendMsgBuf(metric.header, 0, metric.len, metric.data);
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
            if (currentSettings.displayPressure) {
              if (currentSettings.pressurePsi) {
                fl = rcvBuf[0] > 25 ? String(rcvBuf[0]) : "LO";
                fr = rcvBuf[1] > 25 ? String(rcvBuf[1]) : "LO";
                rr = rcvBuf[2] > 25 ? String(rcvBuf[2]) : "LO";
                rl = rcvBuf[3] > 25 ? String(rcvBuf[3]) : "LO";
              } else {
                fl = rcvBuf[0] > 25 ? String(round(rcvBuf[0] * 0.0689476 * 10) / 10) : "LO";
                fr = rcvBuf[1] > 25 ? String(round(rcvBuf[1] * 0.0689476 * 10) / 10) : "LO";
                rr = rcvBuf[2] > 25 ? String(round(rcvBuf[2] * 0.0689476 * 10) / 10) : "LO";
                rl = rcvBuf[3] > 25 ? String(round(rcvBuf[3] * 0.0689476 * 10) / 10) : "LO";
              }
            } else {
              fl = "  ";
              fr = "  ";
              rr = "  ";
              rl = "  ";
            }
        }
          break;
        case 0x423: { // Speed, RPM
          rpm = String(( ( rcvBuf[2] << 8 ) + rcvBuf[3] ) / 4);
          carSpeed = String(round(((((rcvBuf[0] << 8) + rcvBuf[1])/100) - 100) * (currentSettings.unitsMetric ? 1 : 0.621371)), 0);
          temperature = String((rcvBuf[4]-40) * (currentSettings.unitsMetric ? 1 : 1.8) + (currentSettings.unitsMetric ? 0 : 32));

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
            if (!currentSettings.useRTC) {
              hour = (((rcvBuf[0] & 0xF8) >> 3) + currentSettings.tz ) % (currentSettings.hours24 ? 24 : 12);
              minute = (rcvBuf[1] & 0xFC) >> 2;
              second = (rcvBuf[2] & 0xFC) >> 2;
              gotClock = true;
            }
        }
          break;
      }
    }
  }

  if (sendingNow || DEBUG) {
    for (int currentCycle = 0; currentCycle < MSG_COUNT; currentCycle ++ ) {
      if ( ( (timer >= cycle[currentCycle].started ) || (!firstCycle) ) && ((timer % cycle[currentCycle].repeated) - cycle[currentCycle].delayed) == 0) {
        if (cycle[currentCycle].header == 0x3f2) {
          if (currentSettings.useRTC) {
            DateTime now = rtc.now();
            hour = now.hour();
            minute = now.minute();
            gotClock = true;
          }
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
        if (currentSettings.pressurePsi) {
          pressurePadding = 2;
          rpmMessage = " RPM:";
          textMsgLength = 14;
        } else {
          pressurePadding = 3;
          rpmMessage = " R:";
          textMsgLength = 12;
        }
        displayText(0, carSpeed.padRight(3));
        displayText(1, fl.padRight(pressurePadding) + rpmMessage + rpm.padRight(4) + " T:" + temperature.padRight(3) + " " + fr.padRight(pressurePadding));
        displayText(2, rl.padRight(pressurePadding) + " " + message.padCenter(textMsgLength) + " " + rr.padRight(pressurePadding));
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
  } else if (millis() > 10000) {
    settingsMenu();
  }
}
