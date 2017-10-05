#ifndef __SETTINGS_H_
#define __SETTINGS_H_

#include <Arduino.h>
//#include <avr/pgmspace.h>
#include "mcp_can.h"
#include "mcp_can_dfs.h"
#include "Sodaq_DS3231.h"
#include <EEPROM.h>
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

void printCurrentRTCTime()
{
  DateTime now = rtc.now();
  Serial.println(F("Current RTC time: "));
  Serial.println(String(now.hour()) + F(":") + String(now.minute()) + F(":") + String(now.second()));
}

void printCurrentSettings() {
  String pressureUnitsString;

  Serial.println(F("\n\nCurrent settings\n"));
  Serial.print(F("Config version: "));
  Serial.println(String(currentSettings.configVersion));

  Serial.print(F("HU type: "));
  switch (currentSettings.huType) {
    case HU_AFTERMARKET:
      Serial.println(F("Aftermarket"));
      break;
    case HU_STOCK:
      Serial.println(F("Stock"));
      break;
    case HU_CHINESE_WITH_CAN_SIMPLE:
      Serial.println(F("Aftermarket with CAN (simple output)"));
      break;
    case HU_CHINESE_WITH_CAN_EXTENDED:
      Serial.println(F("Aftermarket with CAN (extended output)"));
      break;
  }

  Serial.print(F("Units: "));
  Serial.println(String(currentSettings.unitsMetric ? "Metric" : "American"));
  Serial.print(F("Time source: "));
  Serial.println(String(currentSettings.useRTC ? "RTC" : "GPS"));
  if (currentSettings.useRTC) {
    printCurrentRTCTime();
  } else {
    Serial.print(F("Time zone: "));
    Serial.println(String(currentSettings.tz));
  }

  Serial.print(F("Clock mode: "));
  switch (currentSettings.clockMode) {
    case CLOCK_HIDE:
      Serial.println(F("Hide"));
      break;
    case CLOCK_12:
      Serial.println(F("12h"));
      break;
    case CLOCK_24:
      Serial.println(F("24h"));
      break;
  }

  Serial.print(F("Pressure units: "));
  switch (currentSettings.pressureUnits) {
    case PRESSURE_BARS:
      Serial.println(F("Bars"));
      break;
    case PRESSURE_PSI:
      Serial.println(F("Psi"));
      break;
    case PRESSURE_KPA:
      Serial.println(F("kPa"));
      break;
  }

  Serial.print(F("Display pressure: "));
  Serial.println(String(currentSettings.displayPressure ? "Yes" : "No"));
  Serial.println(F("TPMS interaction mode: "));
  Serial.println(String(currentSettings.tpmsRequest ? "Request" : "Broadcast"));
  Serial.println();
}



void settingsMenu() {

  String input;
  readSettings();
  printCurrentSettings();

  Serial.println(F("FORD FDIM Controller configuration\n"));

  Serial.println(F("Select HU type\n1 - Aftermarket\n2 - Stock\n3 - Aftermarket with CAN (simple info)\n4 - Aftermarket with CAN (extended info)\n"));

  input = readSerialString();

  if (input.equals("1")) {
    currentSettings.huType = HU_AFTERMARKET;
  } else if (input.equals("2")) {
    currentSettings.huType = HU_STOCK;
  } else if (input.equals("3")) {
    currentSettings.huType = HU_CHINESE_WITH_CAN_SIMPLE;
  } else if (input.equals("4")) {
    currentSettings.huType = HU_CHINESE_WITH_CAN_EXTENDED;
  }

  Serial.println(F("Select time source\n1 - 911 Assist GPS (2010+)\n2 - controller real time clock\n"));

  input = readSerialString();

  if (input.equals("1")) {
    currentSettings.useRTC = false;
  } else if (input.equals("2")) {
    currentSettings.useRTC = true;
  }

  if (currentSettings.useRTC) {
    printCurrentRTCTime();
    Serial.println(F("Enter local time in 24h format (HH:MM) or Enter to keep as is"));
    input = readSerialString();
    Serial.println(input);
    if (!input.equals("")) {
      byte minute = input.substring(3,5).toInt();
      byte hour = input.substring(0,2).toInt();
      rtc.setDateTime(DateTime(2011, 11, 10, hour, minute, 0, 5));
      delay(1000);
      printCurrentRTCTime();
    }
  } else {
    Serial.println(F("Enter timezone (+- hours)\n"));
    input = readSerialString();
    currentSettings.tz = input.toInt();
  }

  Serial.println(F("Set clock mode\n1 - Hide\n2 - 12h\n3 - 24h"));
  input = readSerialString();
  if (input.equals("1")) {
    currentSettings.clockMode = CLOCK_HIDE;
  } else if (input.equals("2")) {
    currentSettings.clockMode = CLOCK_12;
  } else if (input.equals("3")) {
    currentSettings.clockMode = CLOCK_24;
  }

  Serial.println(F("Select speed/temperature units\n1 - Metric\n2 - American\n"));
  input = readSerialString();

  if (input.equals("1")) {
    currentSettings.unitsMetric = true;
  } else if (input.equals("2")) {
    currentSettings.unitsMetric = false;
  }

  Serial.println(F("Display tires pressure\n1 - Yes\n2 - No\n"));
  input = readSerialString();

  if (input.equals("1")) {
    currentSettings.displayPressure = true;
  } else if (input.equals("2")) {
    currentSettings.displayPressure = false;
  }

  if (currentSettings.displayPressure) {
    Serial.println(F("Pressure units\n1 - Psi\n2 - kPa\n3 - Bars\n"));

    input = readSerialString();

    if (input.equals("1")) {
      currentSettings.pressureUnits = PRESSURE_PSI;
    } else if (input.equals("2")) {
      currentSettings.pressureUnits = PRESSURE_KPA;
    } else if (input.equals("3")) {
      currentSettings.pressureUnits = PRESSURE_BARS;
    }
  }

  Serial.println(F("Set TPMS interaction mode\n1 - Request\n2 - Broadcast"));
  input = readSerialString();
  if (input.equals("1")) {
    currentSettings.tpmsRequest = true;
  } else if (input.equals("2")) {
    currentSettings.tpmsRequest = false;
  }

  Serial.println(F("Saving settings...\n\n"));
  saveSettings();
  Serial.println(F("Settings saved, getting back to operation mode"));
}

#endif // __SETTINGS_H_
