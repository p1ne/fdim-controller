
# Ford display module controller

## What's this all about

Long story short. This project primary goal is to create simple for DIY assembly
module that enables your Ford USA vehicle top dash display (FDIM module)
to display some useful information when stock head unit is removed.

When this module is plugged in, your dash display looks like that

![](doc/images/hu_aftermarket.jpg)

Module is tested on Ford Escapes (including Hybrid) and Mercury Mariners 2008-2012.
Should work on other Fords of the same generation, including Mustang, Fiesta, F-150,
but it's not yet verified due to lack of test vehicles.

## Current functionality

- display current speed, RPM, engine temperature, tires pressure, tires temperature, current time
- configurable display units (km/miles for speed, Celsius/Fahrenheit for temperatures, psi/bars/kPa for pressure, 12h/24h for clock)
- full functionality offered with aftermarket head unit (with no other modules using dash display)
- support for CAN-enabled aftermarket head units (may require minor additional power wiring)
- very early preliminary support for stock head units
- semi-configurable display layout for different head unit types
- configuration made on the fly via serial terminal

## Hardware versions
- no internal Real Time Clock (RTC) module, time is taken directly from your vehicle GPS module (model year 2009 and newer with 911 Assist)
- with internal Real Time Clock module

## Plugging into your vehicle options
- directly behind your dash display module using DIY pass-through connector
- into vehicle OBD connector
