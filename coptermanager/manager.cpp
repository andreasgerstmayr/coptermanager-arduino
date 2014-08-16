#include <Arduino.h>
#include "manager.h"
#include "common.h"
#include "a7105.h"
#include "hubsan.h"
#include "session.h"

static Session* session[NUM_COPTERS] = {NULL};


void manager_init()
{
    Serial.begin(115200);
    while (!Serial);
    A7105_Setup(); //A7105_Reset();
    hubsan_initialize();
    DEBUG_MSG("initialization successful");
}

static int copter_bind(int type)
{
    for (int i=1; i <= NUM_COPTERS; i++) {
        if (session[i-1] == NULL) {
            switch(type) {
                case HUBSAN_X4:
                    session[i-1] = (Session*)malloc(sizeof(Session));
                    session[i-1]->copterType = HUBSAN_X4;
                    session[i-1]->nextRunAt = 0;
                    session[i-1]->emergencyFlag = 0;
                    session[i-1]->copterSession = hubsan_bind();
                    break;
                    
                default:
                    return PROTOCOL_INVALID_COPTER_TYPE;
            }
            return i;
        }
    }
    
    return PROTOCOL_ALL_SLOTS_FULL;
}

int manager_process_hubsan_command(Session *session, int command, int value)
{
    HubsanSession *copterSession = (HubsanSession*)session->copterSession;
    
    switch(command) {
        case COPTER_THROTTLE:
            if (value >= 0x00 && value <= 0xFF) {
               copterSession->throttle = value;
               return PROTOCOL_OK;
            }
            else {
               return PROTOCOL_VALUE_OUT_OF_RANGE;
            }
            
        case COPTER_RUDDER:
            if (value >= 0x34 && value <= 0xCC) {
               copterSession->rudder = value;
               return PROTOCOL_OK;
            }
            else {
               return PROTOCOL_VALUE_OUT_OF_RANGE;
            }

        case COPTER_AILERON: 
            if (value >= 0x45 && value <= 0xC3) {
               copterSession->aileron = value;
               return PROTOCOL_OK;
            }
            else {
               return PROTOCOL_VALUE_OUT_OF_RANGE;
            }
            
        case COPTER_ELEVATOR: 
            if (value >= 0x3e && value <= 0xBC) {
               copterSession->elevator = value;
               return PROTOCOL_OK;
            }
            else {
               return PROTOCOL_VALUE_OUT_OF_RANGE;
            }
            
        case COPTER_LED: 
            if (value >= 0x00 && value <= 0x01) {
               copterSession->led = value;
               return PROTOCOL_OK;
            }
            else {
               return PROTOCOL_VALUE_OUT_OF_RANGE;
            }
            
        case COPTER_FLIP:
            if (value >= 0x00 && value <= 0x01) {
               copterSession->flip = value;
               return PROTOCOL_OK;
            }
            else {
               return PROTOCOL_VALUE_OUT_OF_RANGE;
            }
            
        case COPTER_VIDEO:
            if (value >= 0x00 && value <= 0x01) {
               copterSession->video = value;
               return PROTOCOL_OK;
            }
            else {
               return PROTOCOL_VALUE_OUT_OF_RANGE;
            }
        
        case COPTER_GETSTATE:
            return hubsan_get_binding_state(copterSession) ? PROTOCOL_ISBOUND : PROTOCOL_ISBINDING;
        
        case COPTER_EMERGENCY:
            copterSession->throttle = 0;
            copterSession->rudder = 0;
            copterSession->aileron = 0;
            copterSession->elevator = 0;
            copterSession->led = 0;
            copterSession->flip = 0;
            copterSession->video = 0;
            return PROTOCOL_OK;

        case COPTER_DISCONNECT:
            return PROTOCOL_OK;

        default:
            return PROTOCOL_UNKNOWN_COMMAND;
    }
}

// copterid: 1..NUM_COPTERS
int manager_processcommand(int copterid, int command, int value)
{
    if (command == COPTER_BIND) {
        return copter_bind(value);
    }
    else {
        if (copterid < 1 || copterid > NUM_COPTERS || session[copterid-1] == NULL)
            return PROTOCOL_INVALID_SLOT;

        // if emergency flag is set only allow disconnect command
        if (session[copterid-1]->emergencyFlag == 1 && command != COPTER_DISCONNECT)
            return PROTOCOL_EMERGENCY_MODE_ON;
        
        int resultCode = PROTOCOL_OK;
        switch(session[copterid-1]->copterType) {
            case HUBSAN_X4:
               resultCode = manager_process_hubsan_command(session[copterid-1], command, value);
               break;
               
            default:
               return PROTOCOL_INVALID_COPTER_TYPE;
        }
               
        if (command == COPTER_EMERGENCY) {
            session[copterid-1]->emergencyFlag = 1;
        }
        else if (resultCode == PROTOCOL_OK && command == COPTER_DISCONNECT) {
            free(session[copterid-1]->copterSession);
            free(session[copterid-1]);
            session[copterid-1] = NULL;
        }
        
        return resultCode;
    }
}

void manager_loop()
{
    for (int copterid=1; copterid <= NUM_COPTERS; copterid++) {
        if (session[copterid-1] != NULL && micros() > session[copterid-1]->nextRunAt) {
            switch(session[copterid-1]->copterType) {
                case HUBSAN_X4:
                    int waitTime = hubsan_cb((HubsanSession*)session[copterid-1]->copterSession);
                    session[copterid-1]->nextRunAt = micros() + waitTime;
                    break;
            }
        }
    }
}
