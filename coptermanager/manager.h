#ifndef MANAGER_H
#define MANAGER_H

enum CopterType {
    HUBSAN_X4 = 0x01
};

enum Command {
    COPTER_BIND       = 0x01,
    COPTER_THROTTLE   = 0x02,
    COPTER_RUDDER     = 0x03,
    COPTER_AILERON    = 0x04,
    COPTER_ELEVATOR   = 0x05,
    COPTER_LED        = 0x06,
    COPTER_FLIP       = 0x07,
    COPTER_VIDEO      = 0x08,
    COPTER_GETSTATE   = 0x09,
    COPTER_EMERGENCY  = 0x0A,
    COPTER_DISCONNECT = 0x0B
};

enum ResultCode {
    PROTOCOL_OK = 0x00,
    
    PROTOCOL_ISBINDING = 0xE0,
    PROTOCOL_ISBOUND = 0xE1,
    
    PROTOCOL_INVALID_COPTER_TYPE = 0xF0,
    PROTOCOL_ALL_SLOTS_FULL = 0xF1,
    PROTOCOL_INVALID_SLOT = 0xF2,
    PROTOCOL_VALUE_OUT_OF_RANGE = 0xF3,
    PROTOCOL_EMERGENCY_MODE_ON = 0xF4,
    PROTOCOL_UNKNOWN_COMMAND = 0xF5
};

void manager_init();
int manager_processcommand(int copterid, int command, int value);
void manager_loop();

#endif
