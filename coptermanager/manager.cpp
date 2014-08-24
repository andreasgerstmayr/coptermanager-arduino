#include <Arduino.h>
#include "manager.h"
#include "common.h"
#include "a7105.h"
#include "hubsan.h"
#include "session.h"

static Session* sessions[NUM_COPTERS] = {NULL};
static int active_sessions_count = 0;

void manager_init()
{
    Serial.begin(BAUDRATE);
    while (!Serial);
    A7105_Setup();
    hubsan_initialize();
    DEBUG_MSG("initialization successful");
}

static int copter_bind(int type, int *response)
{
    for (int copterid=1; copterid <= NUM_COPTERS; copterid++) {
        if (sessions[copterid-1] == NULL) {
            switch(type) {
                case HUBSAN_X4: {
                    Session* session = (Session*)malloc(sizeof(Session));
                    session->copterType = HUBSAN_X4;
                    session->nextRunAt = 0;
                    session->initTime = millis();
                    session->bindTime = 0;
                    session->emergencyFlag = 0;
                    session->copterSession = hubsan_bind(copterid, sessions, NUM_COPTERS);
                    active_sessions_count++;
                    sessions[copterid-1] = session;
                }
                    break;
                    
                default:
                    return PROTOCOL_INVALID_COPTER_TYPE;
            }
            response[0] = 1;
            response[1] = copterid;
            return PROTOCOL_OK;
        }
    }
    
    return PROTOCOL_ALL_SLOTS_FULL;
}

int manager_process_hubsan_command(Session *session, int command, int value, int* response)
{
    HubsanSession *hubsanSession = (HubsanSession*)session->copterSession;
    
    switch(command) {
        case COPTER_THROTTLE:
            if (value >= 0x00 && value <= 0xFF) {
               hubsanSession->throttle = value;
               return PROTOCOL_OK;
            }
            else {
               return PROTOCOL_VALUE_OUT_OF_RANGE;
            }
            
        case COPTER_RUDDER:
            if (value >= 0x34 && value <= 0xCC) {
               hubsanSession->rudder = value;
               return PROTOCOL_OK;
            }
            else {
               return PROTOCOL_VALUE_OUT_OF_RANGE;
            }

        case COPTER_AILERON: 
            if (value >= 0x45 && value <= 0xC3) {
               hubsanSession->aileron = value;
               return PROTOCOL_OK;
            }
            else {
               return PROTOCOL_VALUE_OUT_OF_RANGE;
            }
            
        case COPTER_ELEVATOR: 
            if (value >= 0x3e && value <= 0xBC) {
               hubsanSession->elevator = value;
               return PROTOCOL_OK;
            }
            else {
               return PROTOCOL_VALUE_OUT_OF_RANGE;
            }
            
        case COPTER_LED: 
            if (value >= 0x00 && value <= 0x01) {
               hubsanSession->led = value;
               return PROTOCOL_OK;
            }
            else {
               return PROTOCOL_VALUE_OUT_OF_RANGE;
            }
            
        case COPTER_FLIP:
            if (value >= 0x00 && value <= 0x01) {
               hubsanSession->flip = value;
               return PROTOCOL_OK;
            }
            else {
               return PROTOCOL_VALUE_OUT_OF_RANGE;
            }
            
        case COPTER_VIDEO:
            if (value >= 0x00 && value <= 0x01) {
               hubsanSession->video = value;
               return PROTOCOL_OK;
            }
            else {
               return PROTOCOL_VALUE_OUT_OF_RANGE;
            }
        
        case COPTER_GETSTATE:
            response[0] = 1;
            response[1] = hubsan_get_binding_state(hubsanSession);
            return PROTOCOL_OK;
            
        case COPTER_TELEMETRY:
            switch(value) {
                case TELEMETRY_ALTITUDE:
                    response[0] = 2;
                    response[1] = (hubsanSession->estAltitude >> 8) & 0xFF;
                    response[2] = (hubsanSession->estAltitude >> 0) & 0xFF;
                    break;
                    
                case TELEMETRY_VOLTAGE:
                    response[0] = 1;
                    response[1] = hubsanSession->batteryVolts;
                    break;
                    
                case TELEMETRY_GYROSCOPE:
                    response[0] = 6;
                    response[1] = (hubsanSession->gyroData[0] >> 8) & 0xFF;
                    response[2] = (hubsanSession->gyroData[0] >> 0) & 0xFF;
                    response[3] = (hubsanSession->gyroData[1] >> 8) & 0xFF;
                    response[4] = (hubsanSession->gyroData[1] >> 0) & 0xFF;
                    response[5] = (hubsanSession->gyroData[2] >> 8) & 0xFF;
                    response[6] = (hubsanSession->gyroData[2] >> 0) & 0xFF;
                    break;
                    
                case TELEMETRY_ACCELEROMETER:
                    response[0] = 6;
                    response[1] = (hubsanSession->accData[0] >> 8) & 0xFF;
                    response[2] = (hubsanSession->accData[0] >> 0) & 0xFF;
                    response[3] = (hubsanSession->accData[1] >> 8) & 0xFF;
                    response[4] = (hubsanSession->accData[1] >> 0) & 0xFF;
                    response[5] = (hubsanSession->accData[2] >> 8) & 0xFF;
                    response[6] = (hubsanSession->accData[2] >> 0) & 0xFF;
                    break;
                    
                case TELEMETRY_ANGLE:
                    response[0] = 6;
                    response[1] = (hubsanSession->angle[0] >> 8) & 0xFF;
                    response[2] = (hubsanSession->angle[0] >> 0) & 0xFF;
                    response[3] = (hubsanSession->angle[1] >> 8) & 0xFF;
                    response[4] = (hubsanSession->angle[1] >> 0) & 0xFF;
                    response[5] = (hubsanSession->angle[2] >> 8) & 0xFF;
                    response[6] = (hubsanSession->angle[2] >> 0) & 0xFF;
                    break;
                    
                default:
                    return PROTOCOL_INVALID_TELEMETRY_OPTION;
            }
            return PROTOCOL_OK;
        
        case COPTER_EMERGENCY:
            hubsanSession->throttle = 0;
            hubsanSession->rudder = 0x7F;
            hubsanSession->aileron = 0x7F;
            hubsanSession->elevator = 0x7F;
            hubsanSession->led = 1;
            hubsanSession->flip = 0;
            hubsanSession->video = 0;
            return PROTOCOL_OK;

        case COPTER_DISCONNECT:
            return PROTOCOL_OK;

        default:
            return PROTOCOL_UNKNOWN_COMMAND;
    }
}

// copterid: 1..NUM_COPTERS
int manager_processcommand(int copterid, int command, int value, int checksum, int* response)
{
    // no additional response
    response[0] = 0;
    
    #ifdef DEBUG
    if (command == 0xFF) {
        Serial.println("INFO:");
        HubsanSession *hubsanSession = (HubsanSession*)sessions[copterid-1]->copterSession;
        Serial.println("channel: "+String(hubsanSession->channel));
        Serial.println();
        return PROTOCOL_OK;
    }
    #endif
    
    if (checksum != calculate_checksum(copterid, command, value)) {
        return PROTOCOL_INVALID_CHECKSUM;
    }
    else if (command == COPTER_BIND) {
        return copter_bind(value, response);
    }
    else if (command == COPTER_LISTCOPTERS) {
        int bitmask = 0;
        for (int copterid=1; copterid <= NUM_COPTERS; copterid++) {
            if (sessions[copterid-1] != NULL)
                bitmask |= 1 << (copterid-1);
        }
        response[0] = 1;
        response[1] = bitmask;
        return PROTOCOL_OK;
    }
    else {
        if (copterid < 1 || copterid > NUM_COPTERS || sessions[copterid-1] == NULL)
            return PROTOCOL_INVALID_SLOT;
            
        Session *session = sessions[copterid-1];

        // if emergency flag is set only allow disconnect command
        if (session->emergencyFlag == 1 && command != COPTER_DISCONNECT)
            return PROTOCOL_EMERGENCY_MODE_ON;
        
        int resultCode;
        switch(session->copterType) {
            case HUBSAN_X4:
               resultCode = manager_process_hubsan_command(session, command, value, response);
               break;
               
            default:
               return PROTOCOL_INVALID_COPTER_TYPE;
        }
        
        if (command == COPTER_EMERGENCY) {
            session->emergencyFlag = 1;
        }
        else if (resultCode == PROTOCOL_OK && command == COPTER_DISCONNECT) {
            free(session->copterSession);
            free(session);
            sessions[copterid-1] = NULL;
            active_sessions_count--;
        }
        
        return resultCode;
    }
    
    return PROTOCOL_UNKNOWN_COMMAND;
}

int dummy_response[7];
inline void manager_copterloop(int copterid)
{
    Session* session = sessions[copterid-1];
    
    if (session->bindTime == 0) {
        if ((millis() - session->initTime) > MAX_UNBOUND_TIME) {
            DEBUG_MSG("removing inactive copter " + String(copterid) + " (max unbound time reached)");
            manager_processcommand(copterid, COPTER_DISCONNECT, 0, calculate_checksum(copterid, COPTER_DISCONNECT, 0), dummy_response);
            return;
        }
    }
    else {
        if ((millis() - session->bindTime) > MAX_BOUND_TIME) {
            DEBUG_MSG("removing inactive copter " + String(copterid) + " (max bound time reached)");
            manager_processcommand(copterid, COPTER_DISCONNECT, 0, calculate_checksum(copterid, COPTER_DISCONNECT, 0), dummy_response);
            return;
        }
    }
    
    if (micros() > session->nextRunAt) {
        switch(session->copterType) {
            case HUBSAN_X4:
                HubsanSession *hubsanSession = (HubsanSession*)session->copterSession;
                int is_bound = hubsan_get_binding_state(hubsanSession);
                int waitTime = active_sessions_count <= 1 ? hubsan_cb(hubsanSession) : hubsan_multiple_cb(hubsanSession);
                session->nextRunAt = micros() + waitTime;
                if (!is_bound && hubsan_get_binding_state(hubsanSession))
                    session->bindTime = millis();
                break;
        }
    }       
}

void manager_loop()
{
    for (int copterid=1; copterid <= NUM_COPTERS; copterid++) {
        if (sessions[copterid-1] != NULL)
            manager_copterloop(copterid);
    }
}
