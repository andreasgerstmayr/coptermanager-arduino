//#include <Scheduler.h>
#include <SPI.h>
#include "manager.h"

void setup()
{
    manager_init();
}

void loop()
{
    if (Serial.available() >= 3) {
        int copterid = Serial.parseInt();
        int command = Serial.parseInt();
        int value = Serial.parseInt();
        
        int ret = manager_processcommand(copterid, command, value);
        Serial.write(ret);
    }
    
    manager_loop();
}
