#include <Scheduler.h>
#include <SPI.h>
#include "manager.h"

void setup()
{
    manager_init();
    Scheduler.startLoop(copter1);
    Scheduler.startLoop(copter2);
}

void loop()
{
    if (Serial.available() >= 3) {
        int copterid = Serial.read();
        int command = Serial.read();
        int value = Serial.read();
        
        int ret = manager_processcommand(copterid, command, value);
        Serial.write(ret);
    }
    
    yield(); // pass control to other loops
}

void copter1()
{
    manager_loop(1);
    yield();
}

void copter2()
{
    manager_loop(2);
    yield();
}

