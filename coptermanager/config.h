#ifndef CONFIG_H
#define CONFIG_H

// SPI CS PIN
#define CS_PIN 10

// serial port baudrate
#define BAUDRATE 115200

// number of copter slots, max. 8 (otherwise COPTER_LISTCOPTERS command won't work)
#define NUM_COPTERS 8

// max time of unbound state in ms
#define MAX_UNBOUND_TIME 1000*60

// max time of bound state in ms (should be greater than akku capacity)
#define MAX_BOUND_TIME 1000*60*10

//#define DEBUG
//#define DEBUG_TELEMETRY
//#define SERIAL_ASCII
//#define FIXED_CHANNEL 0x6E

#endif
