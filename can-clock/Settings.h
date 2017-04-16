#ifndef __SETTINGS_H_
#define __SETTINGS_H_

#include <Arduino.h>
#include <EEPROM.h>
#include "RTClib.h"
#include "Service.h"

#define CONFIG_START 32
#define CONFIG_VERSION 1

RTC_DS3231 rtc;

struct Settings {
  int configVersion;
  bool useRTC;
  bool pressurePsi;
  bool displayPressure;
  bool unitsMetric;
  bool hours24;
  byte tz;
  bool spare1;
  bool spare2;
  bool spare3;
  bool spare4;
};

Settings currentSettings = {
  1,      // version
  false,  // useRTC
  true,   // pressurePsi
  true,   // displayPressure
  true,   // unitsMetric
  true,   // hours24
  3,      // tz
  false,  // spare1
  false,  // spare2
  false,  // spare3
  false   // spare4
};

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
  Serial.println("\n\nCurrent settings\n");
  Serial.println("Time source: " + String(currentSettings.useRTC ? "RTC" : "GPS"));
  if (currentSettings.useRTC) {
    DateTime now = rtc.now();
    Serial.println("Current RTC time: " + String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second()));
  }
  Serial.println("Time format: " + String(currentSettings.hours24 ? "24 hours" : "12 hours"));
  Serial.println("Time zone: " + String(currentSettings.tz));
  Serial.println("Pressure units: " + String(currentSettings.pressurePsi ? "Psi" : "Bars"));
  Serial.println("Display pressure: " + String(currentSettings.displayPressure ? "Yes" : "No"));
  Serial.println("Units: " + String(currentSettings.unitsMetric ? "Metric" : "Imperial"));
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
    DateTime now = rtc.now();
    Serial.println("Current RTC time: " + String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second())+"\n");
    Serial.println("Enter current local time in 24h format (HH:MM) or Enter to keep as is");
    input = readSerialString();
    Serial.println(input);
    if (!input.equals("")) {
      byte minute = input.substring(3,5).toInt();
      byte hour = input.substring(0,2).toInt();
      rtc.adjust(DateTime(2017, 1, 1, hour, minute, 0));
      delay(1000);
      now = rtc.now();
      Serial.println("Time set to " + String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second()));
    }
  }

  Serial.println("Set clock format\n1 - 24 hours\n2 - 12 hours");
  input = readSerialString();
  if (input.equals("1")) {
    currentSettings.hours24 = true;
  } else if (input.equals("2")) {
    currentSettings.hours24 = false;
  }

  Serial.println("Enter timezone (+- hours)\n");
  input = readSerialString();
  currentSettings.tz = input.toInt();

  Serial.println("Select speed/temperature units\n1 - Metric\n2 - Imperial\n");

  input = readSerialString();

  if (input.equals("1")) {
    currentSettings.unitsMetric = true;
  } else if (input.equals("2")) {
    currentSettings.unitsMetric = false;
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

#endif // __SETTINGS_H_
