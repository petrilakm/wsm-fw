# WSM ↔ PC communication protocol

## Overview

This protocol allows PC to receive speed from WSM module. Speed is sent every
100 ms. However, this protocol is designed universally to support future
full-duplex communication. This protocol is a binary protocol.

Standard WSM ↔ PC message format:

|  Header byte  |  Data byte 1  |  Data byte 2  | ... |  Data byte n  | XOR |
|---------------|---------------|---------------|-----|---------------|-----|
| `0bTTTT LLLL` | `0bDDDD DDDD` | `0bDDDD DDDD` | ... | `0bDDDD DDDD` | XOR |

## Header byte

 - `TTTT` : type of message
 - `LLLL` : length of *Data byte 1 .. Data byte n*

## XOR

is XOR of *Header byte, Data byte 1 .. Data byte n*.

## WSM → PC messages

### Speed

| Header byte | Data byte 1 | Data 2     | Data 3 | Data 4 |
|-------------|-------------|------------|--------|--------|
| 0x03        | 0x01        | INTH       | INTL   | XOR    |

Speed measured. interval = (INTH << 8) + INTL

```
speed = (PI * wheelDiameter * F_CPU * 3.6 * scale) / HOLE_COUNT * PSK * interval
```

 * `F_CPU` = 3686400
 * `HOLE_COUNT` = 8
 * `PSK` = 64
 * `wheelDiameter` is in mm
 * `scale` is `120` for TT, `87` for H0 etc

## Battery voltage

| Header byte | Data byte 1 | Data 2     | Data 3 | Data 4 |
|-------------|-------------|------------|--------|--------|
| 0x03        | 0x10        | 0bC00000HH | L      | XOR    |

Battery voltage info.

 * voltage = (HH << 8) + L
 * critical bit = C

When critical bit is set, device is going to shutdown in a few microseconds.

## PC → WSM messages

No.
