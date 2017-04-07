#include <Arduino.h>
#include <SPI.h>
#include <avr/pgmspace.h>
#include <Wire.h>
#include <SoftwareSerial.h>
#include <mcp_can.h>
#include <mcp_can_dfs.h>
#include <uRTCLib.h>
#include <EEPROM.h>

#include "CANMessage.h"
#include "FordMessages.h"
#include "FormattedString.h"
#include "Service.h"

#define DEBUG 0

#define TIMER_STEP 25

#define TEXT_MSG_LENGTH 14

#define TZ 3

uRTCLib rtc;

#undef MQ135_CONNECTED

byte second, minute, hour;

SoftwareSerial mySerial(8, 9); // RX, TX

FormattedString fl, fr, rl, rr, message, rpm, carSpeed, temperature;

String dump[8];

bool useRTC = false;

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


struct Settings {
  int configVersion;
  bool useRTC;
  bool pressurePsi;
  bool displayPressure;
  bool speedUnitsMetric;
  bool param1;
  bool param2;
  bool param3;
  bool param4;
  bool param5;
  bool param6;
};

Settings currentSettings;
#define CONFIG_START 32
#define CONFIG_VERSION 1

void readSettings() {
  if (EEPROM.read(CONFIG_START + 0) == CONFIG_VERSION)
   for (unsigned int t=0; t<sizeof(currentSettings); t++)
     *((char*)&currentSettings + t) = EEPROM.read(CONFIG_START + t);
}

void saveSettings() {
  for (unsigned int t=0; t<sizeof(currentSettings); t++)
   EEPROM.write(CONFIG_START + t, *((char*)&currentSettings + t));
}

void printCurrentSettings() {
  Serial.println("\nCurrent settings\n\n");
  Serial.println("Time source: " + String(currentSettings.useRTC ? "RTC" : "GPS"));
  if (currentSettings.useRTC) {
    rtc.refresh();
    Serial.println("Current RTC time: " + String(rtc.hour()) + ":" + String(rtc.minute()) + ":" + String(rtc.second()));
  }
  Serial.println("Pressure units: " + String(currentSettings.pressurePsi ? "Psi" : "Bars"));
  Serial.println("Display pressure: " + String(currentSettings.displayPressure ? "Yes" : "No"));
  Serial.println("Speed units: " + String(currentSettings.speedUnitsMetric ? "Metric" : "Imperial"));
  Serial.println();
}

void settingsMenu() {

  String input;
  readSettings();
  printCurrentSettings();

  Serial.println("FORD FDIM Controller configuration\n");

  Serial.println("Select time source\n1 - 911 Assist GPS (2010+)\n2 - controller real time clock\n");

  input = readSerialString();

  if (input.equals("1")) {
    currentSettings.useRTC = false;
  } else if (input.equals("2")) {
    currentSettings.useRTC = true;
  }

  if (currentSettings.useRTC) {
    rtc.refresh();
    Serial.println("Current RTC time: " + String(rtc.hour()) + ":" + String(rtc.minute()) + ":" + String(rtc.second())+"\n");
    Serial.println("Enter current local time in 24h format (HH:MM) or Enter to keep as is");
    input = readSerialString();
    Serial.println(input);
    if (!input.equals("")) {
      minute = input.substring(3,5).toInt();
      hour = input.substring(0,2).toInt();
      rtc.set(0, minute, hour, 0, 1, 1, 0);
      delay(1000);
      rtc.refresh();
      Serial.println("Time set to " + String(rtc.hour()) + ":" + String(rtc.minute()) + ":" + String(rtc.second()));
    }
  }

  Serial.println("Select speed units\n1 - Metric\n2 - Imperial\n");

  input = readSerialString();

  if (input.equals("1")) {
    currentSettings.speedUnitsMetric = true;
  } else if (input.equals("2")) {
    currentSettings.speedUnitsMetric = false;
  }

  Serial.println("Display tires pressure\n1 - Yes\n2 - No\n");

  input = readSerialString();

  if (input.equals("1")) {
    currentSettings.displayPressure = true;
  } else if (input.equals("2")) {
    currentSettings.displayPressure = false;
  }

  if (currentSettings.displayPressure) {
    Serial.println("Pressure units\n1 - Psi\n2 - Bars\n");

    input = readSerialString();

    if (input.equals("1")) {
      currentSettings.pressurePsi = true;
    } else if (input.equals("2")) {
      currentSettings.pressurePsi = false;
    }
  }
  saveSettings();
}

void setup() {
  Serial.begin(115200);
  mySerial.begin(9600);

  initStartMessages();
  initCycleMessages();
  initTextMessages();

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
  CAN.init_Filt(1, CAN_STDID, 0x3B5);   // TPMS data
  if (!useRTC) {
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
            if (!useRTC) {
              hour = (((rcvBuf[0] & 0xF8) >> 3) + TZ ) % 24;
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
          if (useRTC) {
            rtc.refresh();
            hour = rtc.hour();
            minute = rtc.minute();
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
  } else if (millis() > 10000) {
    settingsMenu();
  }
}
