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
#define CONFIG_VERSION 5

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
  uint8_t tz;
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

void printParam(String param) {
  WebUSBSerial.print(param);
  WebUSBSerial.print(F("|"));
}

void printCurrentRTCTime()
{
  if (currentSettings.useRTC) {
    rtc.begin();
    DateTime now = rtc.now();
    printParam(String(now.hour()) + F(":") + String(now.minute()) + F(":") + String(now.second()));
  } else {
    printParam("");
  }
}

void printCurrentSettings() {
  printParam(String(currentSettings.configVersion));
  printParam(String(currentSettings.huType));
  printParam(String(currentSettings.unitsMetric));
  printParam(String(currentSettings.useRTC));
  printCurrentRTCTime();
  printParam(String(currentSettings.tz));
  printParam(String(currentSettings.clockMode));
  printParam(String(currentSettings.displayPressure));
  printParam(String(currentSettings.pressureUnits));
  printParam(String(currentSettings.tpmsRequest));
  WebUSBSerial.flush();
}



void settingsMenu() {

  String input;
  readSettings();
  printCurrentSettings();

  input = readSerialString();

  saveSettings();
}

#endif // __SETTINGS_H_
