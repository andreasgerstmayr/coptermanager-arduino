# Coptermanager

A set of applications to program your quadrocopters. Contains code for the transmitter station (arduino board with A7105 chip), a server application to control your drones in the browser and execute custom code using a JavaScript API, and a node.js package for connecting to a local or remote transmitter station.

## Overview

  * [coptermanager-arduino](https://github.com/andihit/coptermanager-arduino): arduino application which communicates with the quadrocopters
  * [coptermanager_server](https://github.com/andihit/coptermanager_server): web interface and HTTP API for controlling multiple quadrocopters
  * [coptermanager-client](https://github.com/andihit/coptermanager-client): client library to control quadrocopters with javascript (node.js)

## Possible setups

### Variant 1: full stack solution

This setup is recommended. It allows you to control multiple quadrocopters with just a single arduino board and transmitter chip. You can open the webinterface and start programming right away, inside the browser. Furthermore you can connect other apps to the HTTP API (e.g. apps for mobile devices).

### Variant 2: coptermanager-arduino and coptermanager-client

It is also possible to talk directly from the client to the arduino board. The JavaScript API is identical to variant 1.

## Setup instructions

1. Follow the [instructions to build the transmitter station](http://www.instructables.com/id/Easy-Android-controllable-PC-Interfaceable-Relati/step5/Building-the-Arduino-driven-radio/) with a small modification:
  * instead of 3 wire SPI use 4 wire SPI:
  * skip step 10 "Put an additional wire from the 'SDIO' pin of the A7105 to the 'MISO' pin of the due."
  * **instead wire 'gio1' of the A7105 chip to the 'MISO' port of the arduino due**
  * test some resistor values (as written in the tutorial) - mine didn't work with 22K Ohm, but it works with 10K Ohm

2. Open the source code of this project and send it to your arduino board

### Pictures

![](http://andihit.github.io/coptermanager-arduino/images/board1.jpg)
![](http://andihit.github.io/coptermanager-arduino/images/board2.jpg)
(Note: the floppy cable isn't required - I didn't have any male-to-female cables for the SPI port of the arduino due so I had to improvise...)

## Documentation

### Protocol

**coptermanager-arduino** listens on the serial port for new commands. The protocol is very simple: each command contains 3 bytes - `copterid`, `command` and `value`. The response is always a one byte.

#### List of commands:

| Name               | Code | Command value                      | Description                                                                                                         |
| ------------------ | ---- | ---------------------------------- | ------------------------------------------------------------------------------------------------------------------- |
| COPTER_BIND        | 0x01 | copter type (see below)            | Initiate binding process. Timing is very important here, so during this time (max. 500ms) the processor is blocked. Return values the copterid |
| COPTER_THROTTLE    | 0x02 | throttle value (range 0x00 - 0xFF) | set throttle (top/down) value                                                                                       |
| COPTER_RUDDER      | 0x03 | rudder value (range 0x34 - 0xCC)   | set rudder (rotate left/right) value                                                                                |
| COPTER_AILERON     | 0x04 | aileron value (range 0x45 - 0xC3)  | set aileron (drift left/right) value                                                                                |
| COPTER_ELEVATOR    | 0x05 | elevator value (range 0x3E - 0xBC) | set elevator (forward/backward) value                                                                               |
| COPTER_LED         | 0x06 | enable or disable (1 or 0)         | enable/disable leds                                                                                                 |
| COPTER_FLIP        | 0x07 | enable or disable (1 or 0)         | enable/disable flips                                                                                                |
| COPTER_VIDEO       | 0x08 | enable or disable (1 or 0)         | enable/disable video (if the copter has a camera)                                                                   |
| COPTER_GETSTATE    | 0x09 | -                                  | get binding state                                                                                                   |
| COPTER_EMERGENCY   | 0x0A | -                                  | set emergency mode on: in this mode all values are set to default (the copter will fall down like a rock) and no further commands will be accepted (except COPTER_DISCONNECT) |
| COPTER_DISCONNECT  | 0x0B | -                                  | disconnect copter                                                                                                   |
| COPTER_LISTCOPTERS | 0x0C | -                                  | get bitmask of connected copters (LSB = copterid 1, bit 1 = copterid 2, ...)                                        |

#### List of copter types:

| Name      | Code | Description                                                                   |
| --------- | ---- | ----------------------------------------------------------------------------- |
| HUBSAN_X4 | 0x01 | The Hubsan X4 (tested with the V2, but should work with V1 and maybe FPV too) |

#### List of response codes:

| Name                         | Code | Description                                                           |
| ---------------------------- | ---- | --------------------------------------------------------------------- |
| PROTOCOL_OK                  | 0x00 | Success                                                               |
| PROTOCOL_UNBOUND             | 0xE0 | Copter is unbound                                                     |
| PROTOCOL_BOUND               | 0xE1 | Copter is bound                                                       |
| PROTOCOL_INVALID_COPTER_TYPE | 0xF0 | Invalid copter type. See table above on valid copter types            |
| PROTOCOL_ALL_SLOTS_FULL      | 0xF1 | All copter slots are full. Configurable inside config.h `NUM_COPTERS` |
| PROTOCOL_INVALID_SLOT        | 0xF2 | Invalid copterid. Range 1 - `NUM_COPTERS`                             |
| PROTOCOL_VALUE_OUT_OF_RANGE  | 0xF3 | Value out of range                                                    |
| PROTOCOL_EMERGENCY_MODE_ON   | 0xF4 | If emergency mode is on, only the disconnect command is supported     |
| PROTOCOL_UNKNOWN_COMMAND     | 0xF5 | Unkown command                                                        |

#### Example session:

```
> 0x00 0x01 0x01 # bind a hubsan x4
< 0x01           # assigned copterid is 0x01
> 0x01 0x02 0x0F # set throttle of copter #1 to 0x0F
< 0x00           # success
> 0x01 0x06 0x00 # set of copter #1 off
< 0x00           # success
> 0x01 0x0B 0x00 # disconnect copter #1
< 0x00           # success
```

In this session `>` indicates a request and `<` the response. The protocol is binary: spaces are just for readability, and values must be transmitted as their value and *not* as ASCII strings.

#### Debugging:

In `config.h` there are 2 debug modes available: `DEBUG` and `SERIAL_ASCII`. The first one enables debug messages and the last one enables write and read operations as ASCII strings. With `SERIAL_ASCII` turned on the requests are in the format `AABBCC` where `AA` is the copterid, `BB` the command and `CC` the command value in hexadecimal format with ASCII characters (don't prefix them with `0x`!). Example request: `000101` (transmitted as ASCII characters, e.g. wrote in the arduino IDE's serial monitor, with no line ending) to bind the hubsan.
