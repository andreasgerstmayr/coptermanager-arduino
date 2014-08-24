#ifndef SESSION_H
#define SESSION_H

#include "common.h"
#include "manager.h"

enum {
    doTx, 
    waitTx, 
    pollRx
};

struct HubsanSession {
    u32 txid;
    u8 packet[16];
    u8 channel;
    u32 sessionid;
    u8 state;
    u8 packet_count;
    // telemetry
    uint8_t telemetryState = doTx;
    uint8_t polls;

    u8 throttle; // ascend/descend, observed range: 0x00 - 0xff (smaller is down)
    u8 rudder; // rotate left/right, observed range: 0x34 - 0xcc (smaller is right)
    u8 aileron; // drift sideways left/right, observed range: 0x45 - 0xc3 (smaller is right)
    u8 elevator; // forward/backward, observed range: 0x3e - 0xbc (smaller is up)
    int led;
    int flip;
    int video;
    
    int16_t estAltitude;
    uint8_t batteryVolts;
    int16_t gyroData[3];
    int16_t accData[3];
    int16_t angle[3];
};

struct Session {
    CopterType copterType;
    unsigned long nextRunAt; // us
    unsigned long initTime; // ms
    unsigned long bindTime; // ms
    int emergencyFlag;
    void* copterSession;
};

#endif
