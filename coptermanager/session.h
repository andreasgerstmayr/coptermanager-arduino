#ifndef SESSION_H
#define SESSION_H

#include "common.h"
#include "manager.h"

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
    unsigned long nextRunAt; // us
    unsigned long initTime; // ms
    unsigned long bindTime; // ms
    int emergencyFlag;
    void* copterSession;
};

#endif
