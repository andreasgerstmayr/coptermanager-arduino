#include <SPI.h>
#include "manager.h"
#include "common.h"

void setup()
{
    pinMode(13, OUTPUT);
    digitalWrite(13, LOW);
    manager_init();
    delay(500);
    digitalWrite(13, HIGH);
    delay(500);
    digitalWrite(13, LOW);
}

#define SERIAL_ASCII

void loop()
{
    if (Serial.available() >= 3) {
        #ifdef SERIAL_ASCII
            byte data[6];
            Serial.readBytes(data, 6);
            int copterid = (data[0] - 48) * 10 + (data[1] - 48);
            int command =  (data[2] - 48) * 10 + (data[3] - 48);
            int value =    (data[4] - 48) * 10 + (data[5] - 48);
        #else
            int copterid = Serial.read();
            int command = Serial.read();
            int value = Serial.read();
        #endif
        
        DEBUG_MSG("read values "+String(copterid)+" "+String(command)+" "+String(value));
        int result_code = manager_processcommand(copterid, command, value);
        
        #ifdef SERIAL_ASCII
            Serial.println("command result "+String(result_code, HEX));
        #else
            Serial.write(result_code);
        #endif
    }
    
    manager_loop();
}
