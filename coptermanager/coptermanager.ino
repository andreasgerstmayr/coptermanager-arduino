#include <SPI.h>
#include "manager.h"

void setup()
{
    manager_init();
    pinMode(13, OUTPUT);
    digitalWrite(13, HIGH);
    delay(200);
    digitalWrite(13, LOW);
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
    
    //manager_loop();
}
