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
#define CONFIG_VERSION 6

#define PRESSURE_PSI 0
#define PRESSURE_BARS 1
#define PRESSURE_KPA 2

#define CLOCK_HIDE 1
#define CLOCK_12 12
#define CLOCK_24 24

#define HU_AFTERMARKET 1
#define HU_STOCK 2
#define HU_CHINESE_WITH_CAN_SIMPLE 3
#define HU_CHINESE_WITH_CAN_EXTENDED 4

bool isConfigured = false;

typedef struct __attribute__((__packed__)) {
  uint8_t configVersion;
  bool useRTC;
  uint8_t pressureUnits;
  bool displayPressure;
  bool unitsMetric;
  uint8_t clockMode;
  int8_t tz;
  bool tpmsRequest;
  uint8_t huType;
  bool spare2;
  bool spare3;
} Settings;

Settings currentSettings = {
  CONFIG_VERSION, // version
  false,          // useRTC
  PRESSURE_PSI,   // pressureUnits
  true,           // displayPressure
  true,           // unitsMetric
  CLOCK_24,       // clockMode
  3,              // tz
  true,           // tpmsRequest
  HU_AFTERMARKET, // huType
  false,          // spare2
  false           // spare3
};

WebUSB WebUSBSerial(255, "http://localhost:8080/fdim");

void readSettings() {
  if (EEPROM.read(CONFIG_START + 0) == CONFIG_VERSION)
   for (unsigned int t=0; t<sizeof(currentSettings); t++)
     *((char*)&currentSettings + t) = EEPROM.read(CONFIG_START + t);
}

void saveSettings() {
  for (unsigned int t=0; t<sizeof(currentSettings); t++)
   EEPROM.write(CONFIG_START + t, *((char*)&currentSettings + t));

  isConfigured = true;
}

boolean wantClock() {
  // we only need to send clock messages on aftermarket HU if it is not visible
  // because on stock HU we already have it, and on chinese we have that line empty
  return ((currentSettings.clockMode != CLOCK_HIDE) && (currentSettings.huType == HU_AFTERMARKET));
}

void printParam(int8_t param) {
  WebUSBSerial.print(char(param));
  Serial.print(char(param));
//  WebUSBSerial.print(F("|"));
}

void printCurrentRTCTime()
{
  if (currentSettings.useRTC) {
    rtc.begin();
    DateTime now = rtc.now();
    printParam(int8_t(now.hour()));
    printParam(int8_t(now.minute()));
  } else {
    printParam(int8_t(32));
    printParam(int8_t(64));
  }
}

void saveRTCTime(uint8_t hour, uint8_t minute) {
   rtc.setDateTime(DateTime(2011, 11, 10, hour, minute, 0, 5));
}

void printCurrentSettings() {
  printParam(int8_t(currentSettings.configVersion));
  printParam(int8_t(currentSettings.huType));
  printParam(int8_t(currentSettings.unitsMetric));
  printParam(int8_t(currentSettings.useRTC));
  printCurrentRTCTime();
  printParam(int8_t(currentSettings.tz));
  printParam(int8_t(currentSettings.clockMode));
  printParam(int8_t(currentSettings.displayPressure));
  printParam(int8_t(currentSettings.pressureUnits));
  printParam(int8_t(currentSettings.tpmsRequest));
  WebUSBSerial.flush();
}

void saveReceivedSettings(String settingsString) {


  currentSettings.configVersion = uint8_t(settingsString.c_str()[0]);
  currentSettings.huType = uint8_t(settingsString.c_str()[1]);
  currentSettings.unitsMetric = bool(settingsString.c_str()[2]);
  currentSettings.useRTC = bool(settingsString.c_str()[3]);
  saveRTCTime(uint8_t(settingsString.c_str()[4]), uint8_t(settingsString.c_str()[5]));
  currentSettings.tz = int8_t(settingsString.c_str()[6]);
  currentSettings.clockMode = uint8_t(settingsString.c_str()[7]);
  currentSettings.displayPressure = bool(settingsString.c_str()[8]);
  currentSettings.pressureUnits = uint8_t(settingsString.c_str()[9]);
  currentSettings.tpmsRequest = bool(settingsString.c_str()[10]);

  saveSettings();
}



#endif // __SETTINGS_H_
