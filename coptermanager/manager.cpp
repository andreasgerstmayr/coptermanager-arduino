#include <Arduino.h>
#include <limits.h>
#include "manager.h"
#include "common.h"
#include "a7105.h"
#include "hubsan.h"

static Session* session[NUM_COPTERS] = {NULL};
static unsigned long runat[NUM_COPTERS] = {ULONG_MAX};


void manager_init()
{
    Serial.begin(115200);
    A7105_Setup(); //A7105_Reset();
    hubsan_initialize();
}

static int copter_bind(int type)
{
    for (int i=1; i <= NUM_COPTERS; i++) {
        if (session[i-1] == NULL) {
            switch(type) {
                case HUBSAN_X4:
                    session[i-1] = hubsan_bind();
                    break;
                    
                default:
                    return PROTOCOL_ERROR;
            }
            return i;
        }
    }
    
    return PROTOCOL_ERROR;
}

// copterid: 1..NUM_COPTERS
int manager_processcommand(int copterid, int command, int value)
{
    if (copterid < 1 || copterid > NUM_COPTERS)
        return PROTOCOL_ERROR;
        
    switch(command) {
        case COPTER_BIND:
            return copter_bind(value);
            
        case COPTER_THROTTLE: session[copterid-1]->throttle = value; break;
        case COPTER_RUDDER: session[copterid-1]->rudder = value; break;
        case COPTER_AILERON: session[copterid-1]->aileron = value; break;
        case COPTER_ELEVATOR: session[copterid-1]->elevator = value; break;
        case COPTER_LED: session[copterid-1]->led = value; break;
        case COPTER_FLIP: session[copterid-1]->flip = value; break;
        case COPTER_VIDEO: session[copterid-1]->video = value; break;
        
        case COPTER_LAND:
            // TODO: smooth landing
            session[copterid-1]->throttle = 0;
            session[copterid-1]->rudder = 0;
            session[copterid-1]->aileron = 0;
            session[copterid-1]->elevator = 0;
            session[copterid-1]->led = 0;
            session[copterid-1]->flip = 0;
            session[copterid-1]->video = 0;
            break;

        case COPTER_EMERGENCY:
            session[copterid-1]->throttle = 0;
            session[copterid-1]->rudder = 0;
            session[copterid-1]->aileron = 0;
            session[copterid-1]->elevator = 0;
            session[copterid-1]->led = 0;
            session[copterid-1]->flip = 0;
            session[copterid-1]->video = 0;
            break;

        case COPTER_DISCONNECT:
            session[copterid-1] = NULL;
            break;

        default:
            return PROTOCOL_ERROR;
    }
    
    return PROTOCOL_OK;
}

void manager_loop(int copterid)
{
    if (session[copterid-1] == NULL)
        return;
        
    if (micros() < runat[copterid-1])
        return;
        
    runat[copterid-1] = hubsan_cb(session[copterid-1]);
    runat[copterid-1] += micros();
}

