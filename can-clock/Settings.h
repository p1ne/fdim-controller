#ifndef __SETTINGS_H_
#define __SETTINGS_H_

#include <Arduino.h>
#include <EEPROM.h>

#include "mcp_can.h"
#include "mcp_can_dfs.h"
#include "Sodaq_DS3231.h"
#include "WebUSB.h"

#include "Service.h"

#define CONFIG_START 32
#define CONFIG_VERSION 8

#define PRESSURE_OFF 0
#define PRESSURE_PSI 1
#define PRESSURE_BARS 2
#define PRESSURE_KPA 3

#define CLOCK_HIDE 1
#define CLOCK_12 12
#define CLOCK_24 24

#define HU_AFTERMARKET 1
#define HU_STOCK 2
#define HU_CHINESE_WITH_CAN_SIMPLE 3
#define HU_CHINESE_WITH_CAN_EXTENDED 4
#define HU_AFTERMARKET_WITH_GPS_DATE 5

typedef struct __attribute__((__packed__)) {
  uint8_t configVersion;
  uint8_t huType;
  bool unitsMetric;
  bool useRTC;
  uint8_t tz;
  bool tzPositive;
  uint8_t clockMode;
  uint8_t tpmsDisplay;
  bool tpmsRequest;
  bool spare1;
  bool spare2;
} Settings;

Settings currentSettings = {
  CONFIG_VERSION, // version
  HU_AFTERMARKET, // huType
  true,           // unitsMetric
  false,          // useRTC
  3,              // tz
  true,           // tzPositive
  CLOCK_24,       // clockMode
  PRESSURE_PSI,   // tpmsDisplay
  true,           // tpmsRequest
  false,          // spare1
  false           // spare2
};

//WebUSB WebUSBSerial(255, "https://p1ne.github.io/fdim-controller/fdim-config/");

WebUSB WebUSBSerial(255, "http://localhost:8080/fdim-config/");

void readSettings() {
  if (EEPROM.read(CONFIG_START + 0) == CONFIG_VERSION)
   for (unsigned int t=0; t<sizeof(currentSettings); t++)
     *((char*)&currentSettings + t) = EEPROM.read(CONFIG_START + t);
}

void saveSettings() {
  for (unsigned int t=0; t<sizeof(currentSettings); t++)
   EEPROM.write(CONFIG_START + t, *((char*)&currentSettings + t));
}

boolean wantClock() {
  // we only need to send clock messages on aftermarket HU if it is not visible
  // because on stock HU we already have it, and on chinese we have that line empty
  return ((currentSettings.clockMode != CLOCK_HIDE) && (currentSettings.huType == HU_AFTERMARKET));
}

void printParam(int8_t param) {
  WebUSBSerial.print(char(param));

#if defined (WEBUSB_DEBUG)
  Serial.print(char(param), HEX);
  Serial.print("|");
#endif
}

void printCurrentRTCTime()
{
  if (currentSettings.useRTC) {
    rtc.begin();
    DateTime now = rtc.now();
    if ((now.hour() >= 0) && (now.hour() < 24) && (now.minute() >= 0) && (now.minute() < 60)) {
      printParam(uint8_t(now.hour()));
      printParam(uint8_t(now.minute()));
      return;
    }
  }

  printParam(uint8_t(0));
  printParam(uint8_t(0));
}

void saveRTCTime(uint8_t hour, uint8_t minute) {
  if (currentSettings.useRTC) {
   rtc.begin();
   rtc.setDateTime(DateTime(2011, 11, 10, hour, minute, 0, 5));
  }
}

void printCurrentSettings() {
  printParam(uint8_t(currentSettings.configVersion));
  printParam(uint8_t(currentSettings.huType));
  printParam(uint8_t(currentSettings.unitsMetric));
  printParam(uint8_t(currentSettings.useRTC));
  printParam(uint8_t(currentSettings.tz));
  printParam(uint8_t(currentSettings.tzPositive));
  printParam(uint8_t(currentSettings.clockMode));
  printParam(uint8_t(currentSettings.tpmsDisplay));
  printParam(uint8_t(currentSettings.tpmsRequest));
  printCurrentRTCTime();
  WebUSBSerial.flush();
}

void saveReceivedSettings(byte settingsString[], byte length) {
  currentSettings.configVersion = uint8_t(settingsString[1]);
  currentSettings.huType = uint8_t(settingsString[2]);
  currentSettings.unitsMetric = bool(settingsString[3]);
  currentSettings.useRTC = bool(settingsString[4]);
  currentSettings.tz = uint8_t(settingsString[5]);
  currentSettings.tzPositive = bool(settingsString[6]);
  currentSettings.clockMode = uint8_t(settingsString[7]);
  currentSettings.tpmsDisplay = uint8_t(settingsString[8]);
  currentSettings.tpmsRequest = bool(settingsString[9]);
  saveRTCTime(uint8_t(settingsString[10]), uint8_t(settingsString[11]));
  printCurrentSettings();
  saveSettings();
}



#endif // __SETTINGS_H_
