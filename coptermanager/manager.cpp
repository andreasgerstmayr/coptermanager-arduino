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

// copterid: 1..NUM_COPTERS
int manager_processcommand(int copterid, int command, int value)
{
    // TODO: other copter types
    HubsanSession *copterSession = NULL;
    
    if (command != COPTER_BIND) {
        if (copterid < 1 || copterid > NUM_COPTERS || session[copterid-1] == NULL)
            return PROTOCOL_INVALID_SLOT;

        // if emergency flag is set only allow disconnect command
        if (session[copterid-1]->emergencyFlag == 1 && command != COPTER_DISCONNECT)
            return PROTOCOL_EMERGENCY_MODE_ON;
            
        copterSession = (HubsanSession*)session[copterid-1]->copterSession;
    }
    
    switch(command) {
        case COPTER_BIND:
            return copter_bind(value);
            
        case COPTER_THROTTLE: copterSession->throttle = value; break;
        case COPTER_RUDDER: copterSession->rudder = value; break;
        case COPTER_AILERON: copterSession->aileron = value; break;
        case COPTER_ELEVATOR: copterSession->elevator = value; break;
        case COPTER_LED: copterSession->led = value; break;
        case COPTER_FLIP: copterSession->flip = value; break;
        case COPTER_VIDEO: copterSession->video = value; break;
        
        case COPTER_EMERGENCY:
            session[copterid-1]->emergencyFlag = 1;
            copterSession->throttle = 0;
            copterSession->rudder = 0;
            copterSession->aileron = 0;
            copterSession->elevator = 0;
            copterSession->led = 0;
            copterSession->flip = 0;
            copterSession->video = 0;
            break;

        case COPTER_DISCONNECT:
            free(session[copterid-1]->copterSession);
            free(session[copterid-1]);
            session[copterid-1] = NULL;
            break;

        default:
            return PROTOCOL_UNKNOWN_COMMAND;
    }
    
    return PROTOCOL_OK;
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
