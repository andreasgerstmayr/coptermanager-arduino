#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

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

enum CopterType {
    HUBSAN_X4 = 0x01
};

struct HubsanSession {
    u8 packet[16];
    u8 channel;
    u32 sessionid;
    u8 state;
    u8 packet_count;

    u8 throttle; // ascend/descend, observed range: 0x00 - 0xff (smaller is down)
    u8 rudder; // rotate left/right, observed range: 0x34 - 0xcc (smaller is right)
    u8 aileron; // drift sideways left/right, observed range: 0x45 - 0xc3 (smaller is right)
    u8 elevator; // forward/backward, observed range: 0x3e - 0xbc (smaller is up)
    int led;
    int flip;
    int video;
};

struct Session {
    CopterType copterType;
    unsigned long nextRunAt;
    int emergencyFlag;
    void* copterSession;
};

#endif
