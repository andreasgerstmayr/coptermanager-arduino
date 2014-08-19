#include <Arduino.h>
#include "manager.h"
#include "common.h"
#include "a7105.h"
#include "hubsan.h"
#include "session.h"

static Session* sessions[NUM_COPTERS] = {NULL};


void manager_init()
{
    Serial.begin(BAUDRATE);
    while (!Serial);
    A7105_Setup(); //A7105_Reset();
    hubsan_initialize();
    DEBUG_MSG("initialization successful");
}

static int copter_bind(int type)
{
    for (int i=1; i <= NUM_COPTERS; i++) {
        if (sessions[i-1] == NULL) {
            switch(type) {
                case HUBSAN_X4: {
                    Session* session = (Session*)malloc(sizeof(Session));
                    session->copterType = HUBSAN_X4;
                    session->nextRunAt = 0;
                    session->initTime = millis();
                    session->bindTime = 0;
                    session->emergencyFlag = 0;
                    
                    hubsan_initialize();
                    HubsanSession *hubsanSession = hubsan_bind();
                    // timing is very important in binding phase, so here is a dedicated loop until binding is finished
                    while (!hubsan_get_binding_state(hubsanSession) && millis() - session->initTime < 500) {
                        int waitTime = hubsan_cb(hubsanSession);
                        delayMicroseconds(waitTime);
                    }
                    if (hubsan_get_binding_state(hubsanSession))
                        session->bindTime = millis();
                    DEBUG_MSG("bound: "+String(hubsan_get_binding_state(hubsanSession))+", time: "+String(millis() - session->initTime)+"ms");
                    
                    session->copterSession = hubsanSession;
                    sessions[i-1] = session;
                }
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
            return hubsan_get_binding_state(hubsanSession) ? PROTOCOL_BOUND : PROTOCOL_UNBOUND;
        
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
int manager_processcommand(int copterid, int command, int value)
{
    #ifdef DEBUG
    if (command == 0xFF) {
        // 300-400 ms bind time
        DEBUG_MSG("int time: "+String(sessions[copterid-1]->initTime)+", bind time: "+String(sessions[copterid-1]->bindTime));
    }
    #endif
    
    if (command == COPTER_BIND) {
        return copter_bind(value);
    }
    else if (command == COPTER_LISTCOPTERS) {
        int bitmask = 0;
        for (int copterid=1; copterid <= NUM_COPTERS; copterid++) {
            if (sessions[copterid-1] != NULL)
                bitmask |= 1 << (copterid-1);
        }
        return bitmask;
    }
    else {
        if (copterid < 1 || copterid > NUM_COPTERS || sessions[copterid-1] == NULL)
            return PROTOCOL_INVALID_SLOT;
            
        Session *session = sessions[copterid-1];

        // if emergency flag is set only allow disconnect command
        if (session->emergencyFlag == 1 && command != COPTER_DISCONNECT)
            return PROTOCOL_EMERGENCY_MODE_ON;
        
        int resultCode = PROTOCOL_OK;
        switch(session->copterType) {
            case HUBSAN_X4:
               resultCode = manager_process_hubsan_command(session, command, value);
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
        }
        
        return resultCode;
    }
}

inline void manager_copterloop(int copterid)
{
    Session* session = sessions[copterid-1];
    
    if (session->bindTime == 0) {
        if ((millis() - session->initTime) > MAX_UNBOUND_TIME) {
            DEBUG_MSG("removing inactive copter " + String(copterid) + " (max unbound time reached)");
            manager_processcommand(copterid, COPTER_DISCONNECT, 0);
            return;
        }
    }
    else {
        if ((millis() - session->bindTime) > MAX_BOUND_TIME) {
            DEBUG_MSG("removing inactive copter " + String(copterid) + " (max bound time reached)");
            manager_processcommand(copterid, COPTER_DISCONNECT, 0);
            return;
        }
    }
    
    if (micros() > session->nextRunAt) {
        switch(session->copterType) {
            case HUBSAN_X4:
                HubsanSession *hubsanSession = (HubsanSession*)session->copterSession;
                int is_bound = hubsan_get_binding_state(hubsanSession);
                int waitTime = hubsan_cb(hubsanSession);
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
