# WSM ↔ PC communication protocol

## Overview

This protocol allows PC to receive speed from WSM module. Speed is sent every
100 ms. However, this protocol is designed universally to support future
full-duplex communication. This protocol is a binary protocol.

Standard WSM ↔ PC message format:

|  Header byte  |  Data byte 1  |  Data byte 2  | ... |  Data byte n  | XOR |
|---------------|---------------|---------------|-----|---------------|-----|
| `0b1TTT LLLL` | `0b1DDD DDDD` | `0b1DDD DDDD` | ... | `0b1DDD DDDD` | XOR |

Protocol is 7-bit, because Bluettoth module resists to transfer data like
`\0` or `\3`. This is probably due to the fact that module thinks it is in
text mode. We have not any way how to switch HC-05 module to raw data mode.

XOR is also 7-bit XOR.

## Header byte

 - `TTT` : type of message
 - `LLLL` : length of *Data byte 1 .. Data byte n*

## XOR

is XOR of *Header byte, Data byte 1 .. Data byte n*.

## WSM → PC messages

### Speed

| Header byte | Data byte 1 | Data 2 | Data 3 | Data 4 | XOR    |
|-------------|-------------|--------|--------|--------|--------|
| 0x84        | 0x81        | INTHH  | INTH   | INTL   | XOR    |

Speed measured. interval = (INTHH << 14) + (INTH << 7) + INTL

```
speed = (PI * wheelDiameter * F_CPU * 3.6 * scale) / HOLE_COUNT * PSK * interval
```

 * `F_CPU` = 3686400
 * `HOLE_COUNT` = 8
 * `PSK` = 64
 * `wheelDiameter` is in mm
 * `scale` is `120` for TT, `87` for H0 etc

## Battery voltage

| Header byte | Data byte 1 | Data 2 | XOR    |
|-------------|-------------|--------|--------|
| 0x92        | 0b1C000HHH  | L      | XOR    |

Battery voltage info.

 * voltage = (HH << 7) + L
 * critical bit = C

When critical bit is set, device is going to shutdown in a few microseconds.

Real voltage [V] = `(voltage / 1024) * 4.84`.

## PC → WSM messages

No.
