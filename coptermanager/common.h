#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include "config.h"

#ifdef DEBUG
  #define DEBUG_MSG(x) Serial.println (x)
#else
  #define DEBUG_MSG(x) (void)0
#endif

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

enum TxPower {
    TXPOWER_100uW,
    TXPOWER_300uW,
    TXPOWER_1mW,
    TXPOWER_3mW,
    TXPOWER_10mW,
    TXPOWER_30mW,
    TXPOWER_100mW,
    TXPOWER_150mW,
    TXPOWER_LAST,
};

enum TXRX_State {
    TXRX_OFF,
    TX_EN,
    RX_EN,
};

#define NUM_PROTO_OPTS 4

struct Model {
    s16 proto_opts[NUM_PROTO_OPTS];
    enum TxPower tx_power;
};

#endif
