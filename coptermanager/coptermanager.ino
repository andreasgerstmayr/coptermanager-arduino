// some .ino file must include the SPI library, otherwise it's not available inside a7105.cpp
#include <SPI.h>
#include "common.h"
#include "manager.h"

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

int hex2int(byte *arr, int startpos, int endpos)
{
    int base = 1;
    int ret = 0;
    for(int i = endpos; i >= startpos; i--) {
        int val = 0;
        if (arr[i] >= 'A' && arr[i] <= 'F')
           val = 10 + (arr[i] - 'A');
        else if (arr[i] >= 'a' && arr[i] <= 'f')
           val = 10 + (arr[i] - 'a');
        else if (arr[i] >= '0' && arr[i] <= '9')
           val = arr[i] - '0';
           
        ret += val * base;
        base *= 16;
    }
    return ret;
}

void loop()
{
    if (Serial.available() >= 3) {
        #ifdef SERIAL_ASCII
            byte data[6];
            Serial.readBytes(data, 6);
            int copterid = hex2int(data, 0, 1);
            int command = hex2int(data, 2, 3);
            int value = hex2int(data, 4, 5);
        #else
            int copterid = Serial.read();
            int command = Serial.read();
            int value = Serial.read();
        #endif
        
        DEBUG_MSG("read values " + String(copterid) + " " + String(command) + " " + String(value));
        int result_code = manager_processcommand(copterid, command, value);
        
        #ifdef SERIAL_ASCII
            Serial.println("command result " + String(result_code, HEX));
        #else
            Serial.write(result_code);
        #endif
    }
    
    manager_loop();
}
