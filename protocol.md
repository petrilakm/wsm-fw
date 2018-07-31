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

| Header byte | Data byte 1 | Data 2     | Data 3 | Data 4 |                 Description                     |
|-------------|-------------|------------|--------|--------|-------------------------------------------------|
| 0x03        | 0x01        | INTH       | INTL   | XOR    | Speed measured. interval = (INTH << 8) + INTL   |
| 0x03        | 0x10        | 0bC00000HH | L      | XOR    | Battery info. value = (HH << 8) + L, C=critical |

## PC → WSM messages

No.
