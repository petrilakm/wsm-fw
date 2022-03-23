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

Fuses:
```
-U lfuse:w:0xed:m -U hfuse:w:0xd9:m -U efuse:w:0xfe:m
```

## Resources

 * [Communication protocol](protocol.md)
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

## Function overview

The main core of this FW is measuring of speed. Speed is measured via special
*ICP capture* feature of Atmega328 processor. The processor measures period
between two rising edges at optosensor.

The processor remembers count and length of all ticks in last second separated
into parts of 100 ms. The speed is sent to PC each 100 ms and is calculated
as average tick length over last 1 s. This method is known as *sliding window*
with total length of 1 s and each frame of length 100 ms.

Plus, there is another mechnism, which sets speed to *0* when there is no
pulse in last 500 ms.

## Authors

 * Jan Horacek ([jan.horacek@kmz-brno.cz](mailto:jan.horacek@kmz-brno.cz))

## License

This application is released under the [Apache License v2.0
](https://www.apache.org/licenses/LICENSE-2.0).
