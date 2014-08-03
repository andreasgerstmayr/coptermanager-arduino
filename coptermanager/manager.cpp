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
}

static int copter_init(int type)
{
    for (int i=1; i <= NUM_COPTERS; i++) {
        if (session[i-1] == NULL) {
            switch(type) {
                case HUBSAN_X4:
                    session[i-1] = initialize();
                    break;
                    
                default:
                    return -1;
            }
            return i;
        }
    }
    
    return -1;
}

// copterid: 1..NUM_COPTERS
int manager_processcommand(int copterid, int command, int value)
{
    if (copterid < 1 || copterid > NUM_COPTERS)
        return -1;
        
    switch(command) {
        case COPTER_INIT: return copter_init(value);
        case COPTER_THROTTLE: session[copterid-1]->throttle = value; break;
        case COPTER_RUDDER: session[copterid-1]->rudder = value; break;
        case COPTER_AILERON: session[copterid-1]->throttle = value; break;
        case COPTER_ELEVATOR: session[copterid-1]->aileron = value; break;
        case COPTER_LED: session[copterid-1]->led = value; break;
        case COPTER_FLIP: session[copterid-1]->flip = value; break;
        case COPTER_VIDEO: session[copterid-1]->video = value; break;
    }
    
    return 0;
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

