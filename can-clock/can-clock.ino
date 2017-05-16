#include <Arduino.h>
#include <SPI.h>
#include <avr/pgmspace.h>
#include <Wire.h>
#include <SoftwareSerial.h>
#include "mcp_can.h"
#include "mcp_can_dfs.h"
#include "RTClib.h"
#include <EEPROM.h>

#include "CANMessage.h"
#include "FordMessages.h"
#include "FormattedString.h"
#include "Service.h"
#include "Settings.h"

#define DEBUG 0
#define IS_HYBRID false

#define TIMER_STEP 25

#undef MQ135_CONNECTED

byte second, minute, hour;

#if !defined(__AVR_ATmega32U4__) // not Arduino Pro Micro
  SoftwareSerial mySerial(8, 9); // RX, TX
#endif

FormattedString fl, fr, rl, rr, message, rpm, carSpeed, temperature, tireTemperature;

byte pressurePadding;
String rpmMessage;
byte textMsgLength;

String pressureLow = "LO";

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

byte currentTpmsRequest = TPMS_INIT;

MCP_CAN CAN(SPI_CS_PIN);

void MCP2515_ISR()
{
    rcvFlag = true;
}


void printDebug(unsigned int timer, CANMessage msg)
{
  if (DEBUG) {
    Serial.print(F("Time: "));
    Serial.print(timer);
    Serial.print(F(" Message: "));
    msg.print();
  }
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

  #if !defined(__AVR_ATmega32U4__) // Arduino Pro Micro - use hw serial for input, others - software serial
    mySerial.begin(9600);
  #endif

  initStartMessages();
  initCycleMessages();
  initTextMessages();
  initTpmsMessages();

  readSettings();

  delay(2000);
  for (int i=5;i>0;i--) {
    Serial.println("Press any key to enter settings menu... " + String(i));
    delay(1000);
    if (Serial.available() > 0) {
      Serial.read();
      Serial.println();
      settingsMenu();
      break;
    }
  }

  if (currentSettings.pressurePsi) {
    fl = "  ";
    fr = "  ";
    rl = "  ";
    rr = "  ";
  } else {
    fl = "   ";
    fr = "   ";
    rl = "   ";
    rr = "   ";
  }

  if (DEBUG) {
    delay(5000);
    printCurrentSettings();
  }

#if defined(MQ135_CONNECTED)
  pinMode(A4, INPUT);
#endif


START_INIT:
  if(CAN_OK == CAN.begin(MCP_STDEXT, CAN_125KBPS, MCP_8MHZ)) {
    Serial.println(F("CAN ok!"));
  } else {
    Serial.println(F("CAN fail"));
    delay(100);
    goto START_INIT;
  }

  #if defined(__AVR_ATmega32U4__) // Arduino Pro Micro
    pinMode(7, INPUT);
    attachInterrupt(digitalPinToInterrupt(7), MCP2515_ISR, FALLING); // start interrupt
  #else // Other Arduinos (Nano in my case)
    pinMode(2, INPUT);
    attachInterrupt(digitalPinToInterrupt(2), MCP2515_ISR, FALLING); // start interrupt
  #endif

  //CAN.setMode(MCP_LOOPBACK);

  byte filtNo = 0;

  CAN.init_Mask(0, CAN_STDID, 0x07FF0000);   // there are 2 mask in mcp2515, you need to set both of them
  CAN.init_Mask(1, CAN_STDID, 0x07FF0000);
  for (int i=0;i<5;i++)
    CAN.init_Filt(i, CAN_STDID, 0x04230000);   // Speed data

  CAN.init_Filt(filtNo++, CAN_STDID, 0x04230000);   // Speed data

  if (currentSettings.displayPressure) {
    if (currentSettings.tpmsRequest) {
      CAN.init_Filt(filtNo++, CAN_STDID, 0x072E0000);    // TPMS response
      CAN.sendMsgBuf(tpms[TPMS_INIT].header, 0, tpms[TPMS_INIT].len, tpms[TPMS_INIT].data);
    } else {
      CAN.init_Filt(filtNo++, CAN_STDID, 0x03B50000);   // TPMS broadcast
    }
  }


  if (currentSettings.useRTC) {
    if (!rtc.begin()) {
      currentSettings.useRTC = false;
      CAN.init_Filt(filtNo++, CAN_STDID, 0x04660000);   // GPS
    }
  } else {
    CAN.init_Filt(filtNo++, CAN_STDID, 0x04660000);   // GPS
  }

  CAN.setMode(MCP_NORMAL);

  timer = 0;
  delay(500);

  //CAN.sendMsgBuf(metric.header, 0, metric.len, metric.data);
}

void loop() {
  String inSerialData;
  if (rcvFlag) {
    rcvFlag = false;
    while (CAN_MSGAVAIL == CAN.checkReceive()) {

      CAN.readMsgBuf(&rcvCanId, &rcvLen, rcvBuf);

      switch (rcvCanId) {
        case 0x3b5: { // TPMS Broadcast
            if ( currentSettings.displayPressure && !currentSettings.tpmsRequest) {
              if (currentSettings.pressurePsi) {
                fl = rcvBuf[0] > 25 ? String(rcvBuf[0]) : pressureLow;
                fr = rcvBuf[1] > 25 ? String(rcvBuf[1]) : pressureLow;
                rr = rcvBuf[2] > 25 ? String(rcvBuf[2]) : pressureLow;
                rl = rcvBuf[3] > 25 ? String(rcvBuf[3]) : pressureLow;
              } else {
                fl = rcvBuf[0] > 25 ? String(round(rcvBuf[0] * 0.689476)) : pressureLow;
                fr = rcvBuf[1] > 25 ? String(round(rcvBuf[1] * 0.689476)) : pressureLow;
                rr = rcvBuf[2] > 25 ? String(round(rcvBuf[2] * 0.689476)) : pressureLow;
                rl = rcvBuf[3] > 25 ? String(round(rcvBuf[3] * 0.689476)) : pressureLow;
              }
            }
        }
          break;
        case 0x72e: { // TPMS response
          if ( currentSettings.displayPressure && currentSettings.tpmsRequest && (rcvBuf[0] == 7) && (rcvBuf[1] == 0x62) && (rcvBuf[2] == 0x41)) {
            switch (rcvBuf[3]) {
              case 0x40: {
                if (currentSettings.pressurePsi) {
                  fl = String(round((rcvBuf[4] * 256 + rcvBuf[5]) / 20));
                  fr = String(round((rcvBuf[6] * 256 + rcvBuf[7]) / 20));
                } else {
                  fl = String(round((rcvBuf[4] * 256 + rcvBuf[5]) * 0.34475));
                  fr = String(round((rcvBuf[6] * 256 + rcvBuf[7]) * 0.34475));
                }
              }
              currentTpmsRequest = TPMS_REAR;
                break;
              case 0x41: {
                if (currentSettings.pressurePsi) {
                  rl = String(round((rcvBuf[4] * 256 + rcvBuf[5]) / 20));
                  rr = String(round((rcvBuf[6] * 256 + rcvBuf[7]) / 20));
                } else {
                  rl = String(round((rcvBuf[4] * 256 + rcvBuf[5]) * 0.34475));
                  rr = String(round((rcvBuf[6] * 256 + rcvBuf[7]) * 0.34475));
                }
              }
              currentTpmsRequest = TPMS_TEMP;
                break;
            }
          }

          if ( currentSettings.displayPressure && currentSettings.tpmsRequest && (rcvBuf[0] == 6) && (rcvBuf[1] == 0x62) && (rcvBuf[2] == 0x41) && (rcvBuf[3] == 0x60)) {
            tireTemperature = String(rcvBuf[4] - 40);
            currentTpmsRequest = TPMS_FRONT;
          }
        }
          break;
        case 0x423: { // Speed, RPM
          rpm = String(( ( rcvBuf[2] << 8 ) + rcvBuf[3] ) / 4);
          carSpeed = String(round(((((rcvBuf[0] << 8) + rcvBuf[1])/100) - 100) * (currentSettings.unitsMetric ? 1 : 0.621371)), 0);
          temperature = String(round((rcvBuf[4]-40) * (currentSettings.unitsMetric ? 1 : 1.8) + (currentSettings.unitsMetric ? 0 : 32)));

          if ( ((rpm == "0") || (rpm == "")) && sendingNow && !IS_HYBRID) {
            carSpeed = "";
            rpm = "";
            temperature = "";
            message = "";
            sendingNow = false;
            gotClock = false;
          }

          if (( ((rpm != "0") && (rpm != "")) || IS_HYBRID ) && !sendingNow) {
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

    currentTpmsRequest = currentTpmsRequest % TPMS_COUNT;
    if ( ( (timer >= tpms[currentTpmsRequest].started ) || (!firstCycle) ) && ((timer % tpms[currentTpmsRequest].repeated) - tpms[currentTpmsRequest].delayed) == 0) {
      CAN.sendMsgBuf(tpms[currentTpmsRequest].header, 0, tpms[currentTpmsRequest].len, tpms[currentTpmsRequest].data);
      sentOnTick = timer;
      printDebug(timer, tpms[currentTpmsRequest]);
    }

    inSerialData = "";

#if defined(__AVR_ATmega32U4__) // Arduino Pro Micro - input connected to pins TX,RX
    while (Serial.available() > 0) {
        char recieved = Serial.read();
        inSerialData += recieved;

        if (recieved == '\n')
        {
          message = inSerialData;
          inSerialData = "";
        }
    }
#else // Other Arduinos (Nano in my case) - input connected to pins 8,9
    while (mySerial.available() > 0) {
        char recieved = mySerial.read();
        inSerialData += recieved;

        if (recieved == '\n')
        {
          message = inSerialData;
          inSerialData = "";
        }
    }
#endif

#if defined(MQ135_CONNECTED)    // For MQ135
    int sensorValue = analogRead(4);
    message = String(sensorValue, DEC);
#endif // MQ135_CONNECTED

    if (message == F("%MTRACK")) message = "";

    // FIXME
    message = "Tire temp: " + String(tireTemperature);

    if ( ( (timer >= text[currentText].started ) || (!firstCycle) ) && ((timer % text[currentText].repeated) - text[currentText].delayed) == 0) {
      if (currentText == 0) {
        if (currentSettings.pressurePsi) {
          pressurePadding = 2;
          rpmMessage = F(" RPM:");
          textMsgLength = 14;
        } else {
          pressurePadding = 3;
          rpmMessage = F(" R:");
          textMsgLength = 12;
        }
        displayText(0, carSpeed.padRight(3));
        displayText(1, fl.padRight(pressurePadding) + rpmMessage + rpm.padRight(4) + F(" T:") + temperature.padRight(3) + " " + fr.padRight(pressurePadding));
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
  }
}
