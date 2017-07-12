
# FoDiMoCo - Ford display module (FDIM) controller

## What's this all about

Long story short. FoDiMoCo is easy-to-build DIY controller for your Ford USA vehicle that
enables top dash display to display some useful information when stock head unit is removed.

When this controller is plugged in, your dash display looks like that

![](doc/images/hu_aftermarket.jpg)

Module itself looks like that (one of hardware variants)

![](doc/images/module_no_case.jpg)

FoDiMoCo is tested on Ford Escapes (including Hybrid) and Mercury Mariners 2008-2012.
Should work on other Fords of the same generation, including Mustang, Fiesta, F-150,
but it's not yet verified due to lack of test vehicles.

FoDiMoCo is built on Arduino and components that could be found on AliExpress and/or your local
DIY electronics store, so it should be easy to reproduce. PCB is available for order at Dirt Cheap PCBs.

If you don't want to read this long text, there are steps you need to build your own FoDiMoCo:

1. [Buy parts](#parts-list)
2. Order or make PCB
3. Solder
4. Flash
5. Configure
6. Plug into the car

## Overview

### Current functionality

- display current speed, RPM, engine temperature, tires pressure, tires temperature, current time
- configurable display units (km/miles for speed, Celsius/Fahrenheit for temperatures, psi/bars/kPa for pressure, 12h/24h for clock)
- full functionality offered with aftermarket head unit (with no other modules using dash display)
- support for CAN-enabled aftermarket head units (may require minor additional power wiring)
- very early preliminary support for stock head units
- semi-configurable display layout for different head unit types
- configuration made on the fly via serial terminal

### Hardware versions
- no internal Real Time Clock (RTC) module, time is taken directly from your vehicle GPS module (model year 2009 and newer with 911 Assist)
- with internal Real Time Clock module

### Plugging into your vehicle options
- directly behind your dash display module using DIY pass-through connector
- into vehicle OBD connector

## Hardware part

[Parts list](#parts-list)
### Parts list

### Schematics

### PCB

### Soldering step-by-step

### Plugging into car

#### Behind FDIM

#### Into OBD

#### With separate ACC power wire

## CAN bus structure part

### Ford CAN buses: MS CAN and HS CAN

### Messages for FDIM

#### 0x50c - heartbeat

#### 0x3e8 - top line of FDIM module

#### 0x3ef

#### 0x3f2 - clock

#### 0x3f1

#### 0x336 - text message

#### 0x324 - text message

#### 0x337 - text message

#### 0x324 - text message

#### 0x726 - TPMS request

#### 0x72e - TPMS response

#### 0x3b5 - TPMS broadcast

#### 0x423 - Speed, RPM, on/off status

#### 0x466 - GPS time data

### Message sequences

#### Initialization

#### Heartbeat

#### Text output

#### TPMS request

## Software part

### Source code structure

### Flashing from Arduino IDE

### Flashing from Platform IO/Atom

### Flashing pre-built firmwares

#### On Windows

#### On Mac

#### On Linux

## Module configuration

### Serial communication setup

### Options
