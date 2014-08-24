#ifndef MANAGER_H
#define MANAGER_H

enum CopterType {
    HUBSAN_X4 = 0x01
};

enum TelemetryData {
    TELEMETRY_ALTITUDE = 0x01,
    TELEMETRY_VOLTAGE = 0x02,
    TELEMETRY_GYROSCOPE = 0x03,
    TELEMETRY_ACCELEROMETER  = 0x04,
    TELEMETRY_ANGLE = 0x05
};

enum Command {
    COPTER_BIND        = 0x01,
    COPTER_THROTTLE    = 0x02,
    COPTER_RUDDER      = 0x03,
    COPTER_AILERON     = 0x04,
    COPTER_ELEVATOR    = 0x05,
    COPTER_LED         = 0x06,
    COPTER_FLIP        = 0x07,
    COPTER_VIDEO       = 0x08,
    COPTER_GETSTATE    = 0x09,
    COPTER_TELEMETRY   = 0x0A,
    COPTER_EMERGENCY   = 0x0B,
    COPTER_DISCONNECT  = 0x0C,
    COPTER_LISTCOPTERS = 0x0D
};

enum ResultCode {
    PROTOCOL_OK                       = 0x00,
    PROTOCOL_INVALID_CHECKSUM         = 0xF0,
    PROTOCOL_INVALID_COPTER_TYPE      = 0xF1,
    PROTOCOL_ALL_SLOTS_FULL           = 0xF2,
    PROTOCOL_INVALID_SLOT             = 0xF3,
    PROTOCOL_VALUE_OUT_OF_RANGE       = 0xF4,
    PROTOCOL_EMERGENCY_MODE_ON        = 0xF5,
    PROTOCOL_NO_TELEMETRY             = 0xF6,
    PROTOCOL_INVALID_TELEMETRY_OPTION = 0xF7,
    PROTOCOL_UNKNOWN_COMMAND          = 0xF8
};

inline int calculate_checksum(int copterid, int command, int value)
{
   int sum = copterid + command + value;
   return (256 - (sum % 256)) & 0xFF;
}

void manager_init();
int manager_processcommand(int copterid, int command, int value, int checksum, int* response);
void manager_loop();

#endif
