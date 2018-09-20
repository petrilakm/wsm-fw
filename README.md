# WSM-FW: A Wireless SpeedoMeter firmware

WSM is a model railroad car which measures speed of a train and transmits it
via Bluetooth. This repository contains firmware to the device`s main processor
ATmega328p.

## Build & requirements

This firmware is developed in C language, compiled via `avr-gcc` with help
of `make`. You may also find tools like `avrdude` helpful.

Hex files are available in *Releases* section.

## Programming

[WSM PCB](https://github.com/kmzbrnoI/wsm-pcb) contains programming connector,
so you may assemble it and program the processor later.

Currently, EEPROM is not used.

## Resources

 * [PCB](https://github.com/kmzbrnoI/wsm-pcb)
 * [Software to PC](https://github.com/kmzbrnoI/wsm-speed-reader)

## Toolkit

Text editor + `make`. No more, no less.

## LEDs

There are 4 LEDs on the board (red, yellow, green and blue). Blue LED indicates
charging, it is not controlled from CPU. The remaining 3 LEDs are controlled
from CPU.

Red, yellow and green LED should flash at start.

 * Green LED should blink each 5 s. It indicates normal operation and measuring
   voltage of the battery.
 * Yellow LED changes state each time a rising edge on sensor is measured.
 * Red LED should normally be off. When this LED is turned on, the battery is
   low and only a few minutes of measurement are remaining. When the battery
   level in critical, the red LED flashes 3 times and the device shuts itself
   down.

## Authors

 * Jan Horacek ([jan.horacek@kmz-brno.cz](mailto:jan.horacek@kmz-brno.cz))

## License

This application is released under the [Apache License v2.0
](https://www.apache.org/licenses/LICENSE-2.0).
