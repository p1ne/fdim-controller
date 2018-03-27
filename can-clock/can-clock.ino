#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>

#include <SoftwareSerial.h>
#include <avr/pgmspace.h>

#include "mcp_can.h"
#include "mcp_can_dfs.h"
#include "Sodaq_DS3231.h"
#include <EEPROM.h>

#include "CANMessage.h"
#include "FordMessages.h"
#include "FormattedString.h"
#include "Service.h"
#include "Settings.h"

#undef DEBUG
#undef MQ135_CONNECTED

const uint8_t TIMER_STEP = 25;
const uint8_t SPI_CS_PIN = 10;

uint8_t second, minute, hour;
uint8_t i,filtNo;

#if !defined(__AVR_ATmega32U4__) // not Arduino Pro Micro
  SoftwareSerial mySerial(8, 9); // RX, TX
#endif

FormattedString tirePressure[TIRES], message, rpm, carSpeed, temperature, tireTemperature, hourString, minuteString, clockMessage, temperatureMessage;

uint8_t pressurePadding;
String rpmMessage, spdMessage, tireTempMessage, pressureLow;
uint8_t textMsgLength;

uint16_t timer;

bool rcvFlag = false;
uint8_t rcvLen = 0;
INT32U rcvCanId = 0x0;
uint8_t rcvBuf[8];
bool sendingNow = false;

uint16_t sentOnTick = 0;

bool firstCycle = true;
bool gotClock = false;

uint8_t currentText = 0;

uint8_t currentTpmsRequest = TPMS_INIT;

MCP_CAN CAN(SPI_CS_PIN);

void MCP2515_ISR()
{
    rcvFlag = true;
}


void printDebug(const uint16_t timer, const CANMessage msg)
{
#if defined (DEBUG)
    Serial.print(F("Time: "));
    Serial.print(timer);
    Serial.print(F(" Message: "));
    msg.print();
#endif
}

void displayText(const uint8_t strNo, const String str)
{
  uint8_t curLine, curChar, numChars;

#if defined (DEBUG)
    Serial.println(String(strNo) + ": " + str);
#endif

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

  for (i=0;i<numChars;i++) {
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

FormattedString getPressure(const uint8_t pressure)
{
  return (pressure > 25) ? String(round(pressure * (currentSettings.pressureUnits == PRESSURE_PSI ? 1 : 6.89476 ) ) / ( currentSettings.pressureUnits == PRESSURE_BARS ? 100.0 : 1 )) : pressureLow;
}

FormattedString getTemperature(const int8_t t)
{
  return String((int8_t)round((t-40) * (currentSettings.unitsMetric ? 1 : 1.8) + (currentSettings.unitsMetric ? 0 : 32)));
}

void sendStartSequence()
{

  detachCAN();

#if defined DEBUG
  Serial.println("Start sequence");
#endif

  delay(1000);
  timer = 0;

  for (i=0;i<START_COUNT;i++) {
    CAN.sendMsgBuf(start[i].header, 0, start[i].len, start[i].data);
    printDebug(timer, start[i]);
    delay(start[i].delayed);
    timer = timer + start[i].delayed;
  }
  timer = 0;
  firstCycle = true;
  currentText = 0;

  if (currentSettings.tpmsRequest) {
    CAN.sendMsgBuf(tpms[TPMS_INIT].header, 0, tpms[TPMS_INIT].len, tpms[TPMS_INIT].data);
    currentTpmsRequest = TPMS_FRONT;
  }
  attachCAN();
  delay(500);
}

void attachCAN()
{
  #if defined(__AVR_ATmega32U4__) // Arduino Pro Micro
    pinMode(7, INPUT);
    attachInterrupt(digitalPinToInterrupt(7), MCP2515_ISR, FALLING); // start interrupt
  #else // Other Arduinos (Nano in my case)
    pinMode(2, INPUT);
    attachInterrupt(digitalPinToInterrupt(2), MCP2515_ISR, FALLING); // start interrupt
  #endif
}

void detachCAN()
{
  #if defined(__AVR_ATmega32U4__) // Arduino Pro Micro
    pinMode(7, INPUT);
    detachInterrupt(digitalPinToInterrupt(7));
  #else // Other Arduinos (Nano in my case)
    pinMode(2, INPUT);
    detachInterrupt(digitalPinToInterrupt(2));
  #endif
}

void setup() {
  Serial.begin(115200);

  #if !defined(__AVR_ATmega32U4__) // Arduino Pro Micro - use hw serial for input, others - software serial
    mySerial.begin(9600);
  #endif

  readSettings();

  delay(2000);
  for (i=5;i>0;i--) {
    Serial.print(F("Press any key to enter settings menu... "));
    Serial.println(String(i));
    delay(1000);
    if (Serial.available() > 0) {
      Serial.read();
      Serial.println();
      settingsMenu();
      readSettings();
      break;
    }
  }

  if (currentSettings.useRTC) {
    Wire.begin();
  }

  initStartMessages();
  initCycleMessages();
  initTextMessages();
  initTpmsMessages();

  if (currentSettings.pressureUnits == PRESSURE_PSI) {
    pressurePadding = 2;
    pressureLow = F("LO");
    rpmMessage = F(" RPM:");
    spdMessage = F(" SPD:");
    tireTempMessage = F("          T:");
    textMsgLength = 14;
    for (i=TIRE_FL;i<TIRES;i++)
      tirePressure[i] = F("  ");
  } else {
    pressurePadding = 3;
    pressureLow = F("LOW");
    rpmMessage = F(" R:");
    spdMessage = F(" S:");
    tireTempMessage = F("        T:");
    textMsgLength = 12;
    for (i=TIRE_FL;i<TIRES;i++)
      tirePressure[i] = F("   ");
  }

#if defined (DEBUG)
    delay(5000);
    printCurrentSettings();
#endif

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

  attachCAN();

  CAN.setMode(MCP_LOOPBACK);

  filtNo = 0;

  CAN.init_Mask(0, CAN_STDID, 0x07FF0000);   // there are 2 mask in mcp2515, you need to set both of them
  CAN.init_Mask(1, CAN_STDID, 0x07FF0000);
  for (i=0;i<5;i++)
    CAN.init_Filt(i, CAN_STDID, 0x04230000);   // Speed data

  CAN.init_Filt(filtNo++, CAN_STDID, 0x04230000);   // Speed data

  if (currentSettings.displayPressure) {
    if (currentSettings.tpmsRequest) {
      CAN.init_Filt(filtNo++, CAN_STDID, 0x072E0000);    // TPMS response
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
              for (i=TIRE_FL;i<TIRES;i++)
                tirePressure[i] = getPressure(rcvBuf[i]);
            }
        }
          break;
        case 0x72e: { // TPMS response
          if ( currentSettings.displayPressure && currentSettings.tpmsRequest && (rcvBuf[0] == 7) && (rcvBuf[1] == 0x62) && (rcvBuf[2] == 0x41)) {

            switch (rcvBuf[3]) {  // checking front are rear tires response
              case 0x40: {  // front tires
                for (i = TIRE_FL;i <= TIRE_FR; i++)
                  tirePressure[i] = getPressure((rcvBuf[4+(i-TIRE_FL)*2] * 256 + rcvBuf[5+(i-TIRE_FL)*2]) * 0.05);
                  currentTpmsRequest = TPMS_REAR;
              }
                break;
              case 0x41: {  // rear tires
                for (i = TIRE_RL;i <= TIRE_RR; i++)
                  tirePressure[i] = getPressure((rcvBuf[4+(i-TIRE_RL)*2] * 256 + rcvBuf[5+(i-TIRE_RL)*2]) * 0.05);
                  currentTpmsRequest = TPMS_TEMP;
              }
                break;
            }
          }

          // Tires temperature
          if ( currentSettings.displayPressure && currentSettings.tpmsRequest && (rcvBuf[0] == 6) && (rcvBuf[1] == 0x62) && (rcvBuf[2] == 0x41) && (rcvBuf[3] == 0x60)) {
            if (rcvBuf[4] != 0 ) {  // cut-off for zero tires temperature
              tireTemperature = getTemperature(rcvBuf[4]);
            } else {
              tireTemperature = F("--");
            }
            currentTpmsRequest = TPMS_FRONT;
          }
        }
          break;
        case 0x423: { // Speed, RPM
          rpm = String((unsigned int)round(( ( rcvBuf[2] << 8 ) + rcvBuf[3] ) / 4));
          carSpeed = String(round(((((rcvBuf[0] << 8) + rcvBuf[1])/100) - 100) * (currentSettings.unitsMetric ? 1 : 0.621371)), 0);
          temperature = getTemperature(rcvBuf[4]);

          //Serial.println("sendingNow: " + String(sendingNow) + " buf0 " + String(rcvBuf[0],HEX));

          if ( sendingNow && (rcvBuf[0] == 0xFF)) { // got 423 message for ignition off
            sendingNow = false;
            gotClock = false;
            carSpeed = F("");
            rpm = F("");
            temperature = F("");
            tireTemperature = F("");
            message = F("");
          }

          if ( !sendingNow && (rcvBuf[0] != 0xFF)) { // got 423 message for ignition on
            sendingNow = true;
            sendStartSequence();
          }
        }
          break;
        case 0x466: {  // GPS clock
            if (!currentSettings.useRTC && (currentSettings.clockMode != CLOCK_HIDE)) {
              hour = (((rcvBuf[0] & 0xF8) >> 3) + currentSettings.tz ) % currentSettings.clockMode;
              minute = (rcvBuf[1] & 0xFC) >> 2;
              second = (rcvBuf[2] & 0xFC) >> 2;
              gotClock = true;
            }
        }
          break;
      }
    }
  }

#if defined (DEBUG)
  sendingNow = true;
#endif

  if (sendingNow) {

    if (currentSettings.useRTC && (currentSettings.clockMode != CLOCK_HIDE)) {
      DateTime now = rtc.now();
      hour = now.hour() % currentSettings.clockMode;
      minute = now.minute();
      hourString = String(hour);
      minuteString = String(minute);
      clockMessage = hourString.padZeros(2) + F(":") + minuteString.padZeros(2);
      gotClock = true;
    }

    for (uint16_t currentCycle = 0; currentCycle < MSG_COUNT; currentCycle++ ) {
      if ( ( (timer >= cycle[currentCycle].started ) || (!firstCycle) ) && ((timer % cycle[currentCycle].repeated) - cycle[currentCycle].delayed) == 0) {

        if ((currentSettings.huType == HU_AFTERMARKET) &&
            wantClock() &&
            (cycle[currentCycle].header == 0x3f2) &&
            gotClock
           ) { // Special case - add current clock values to clock message
               // Potential FIXME: We assume that there should not be 0x3f2 messages set in
               // init functions if stock head unit is used, so no extra checks
          cycle[currentCycle].data[0] = decToBcd(hour);
          cycle[currentCycle].data[1] = decToBcd(minute);
        }

        if (sentOnTick == timer) delay(TIMER_STEP/2);
        CAN.sendMsgBuf(cycle[currentCycle].header, 0, cycle[currentCycle].len, cycle[currentCycle].data);
        sentOnTick = timer;
        printDebug(timer, cycle[currentCycle]);
      }
    }

    currentTpmsRequest = currentTpmsRequest % TPMS_COUNT;
    if ( ( (timer >= tpms[currentTpmsRequest].started ) || (!firstCycle) ) && ((timer % tpms[currentTpmsRequest].repeated) - tpms[currentTpmsRequest].delayed) == 0) {
      if (sentOnTick == timer) delay(TIMER_STEP/2);
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
          inSerialData = F("");
        }
    }
#else // Other Arduinos (Nano in my case) - input connected to pins 8,9
    while (mySerial.available() > 0) {
        char recieved = mySerial.read();
        inSerialData += recieved;

        if (recieved == '\n')
        {
          message = inSerialData;
          inSerialData = F("");
        }
    }
#endif

#if defined(MQ135_CONNECTED)    // For MQ135
    uint8_t sensorValue = analogRead(4);
    message = String(sensorValue, DEC);
#endif // MQ135_CONNECTED

    if (message == F("%MTRACK")) message = F(""); // FIXME: to mask Media Utilities variable

    if ( ( (timer >= text[currentText].started ) || (!firstCycle) ) && ((timer % text[currentText].repeated) - text[currentText].delayed) == 0) {
      if (currentText == 0) {
        switch (currentSettings.huType) {
          case HU_AFTERMARKET:
            //     80       12:00
            // 2.1 R:1234 E: 83 2.1
            // 2.1        T: 40 2.1

            //     80       12:00
            // 32 RPM:1234 E: 83 32
            // 32          T: 40 32
            displayText(0, carSpeed.padRight(3));
            displayText(1, tirePressure[TIRE_FL].padRight(pressurePadding) + rpmMessage + rpm.padRight(4) + F(" E:") + temperature.padRight(3) + " " + tirePressure[TIRE_FR].padRight(pressurePadding));
            displayText(2, tirePressure[TIRE_RL].padRight(pressurePadding) + tireTempMessage + tireTemperature.padRight(3) + " " + tirePressure[TIRE_RR].padRight(pressurePadding));
            break;
          case HU_STOCK:
            // 2.1 R:1234 E: 83 2.1
            // 2.1 S:  80 T: 40 2.1

            // 32 RPM:1234 E: 83 32
            // 32 SPD:  80 T: 40 32
            displayText(1, tirePressure[TIRE_FL].padRight(pressurePadding) + rpmMessage + rpm.padRight(4) + F(" E:") + temperature.padRight(3) + " " + tirePressure[TIRE_FR].padRight(pressurePadding));
            displayText(2, tirePressure[TIRE_RL].padRight(pressurePadding) + spdMessage + carSpeed.padRight(4) + F(" T:") + tireTemperature.padRight(3) + " " + tirePressure[TIRE_RR].padRight(pressurePadding));
            break;
          case HU_CHINESE_WITH_CAN_SIMPLE:
            // 2.1    12:00     2.1
            // 2.1    E: 83     2.1

            // 32     12:00      32
            // 32     E: 83      32
            temperatureMessage = "E:" + temperature.padRight(3);
            displayText(1, tirePressure[TIRE_FL].padRight(pressurePadding) + clockMessage.padCenter(textMsgLength+2) + tirePressure[TIRE_FR].padRight(pressurePadding));
            displayText(2, tirePressure[TIRE_FL].padRight(pressurePadding) + temperatureMessage.padCenter(textMsgLength+2) + tirePressure[TIRE_FR].padRight(pressurePadding));
            break;

          case HU_CHINESE_WITH_CAN_EXTENDED:
            // 2.1 R:1234 E: 83 2.1
            // 2.1 S:  80 12:00 2.1

            // 32 RPM:1234 E: 83 32
            // 32 SPD:  80 12:00 32
            displayText(1, tirePressure[TIRE_FL].padRight(pressurePadding) + rpmMessage + rpm.padRight(4) + F(" E:") + temperature.padRight(3) + " " + tirePressure[TIRE_FR].padRight(pressurePadding));
            displayText(2, tirePressure[TIRE_RL].padRight(pressurePadding) + spdMessage + carSpeed.padRight(4) + F(" ") + clockMessage + F(" ") + tirePressure[TIRE_RR].padRight(pressurePadding));
            break;
        }
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
