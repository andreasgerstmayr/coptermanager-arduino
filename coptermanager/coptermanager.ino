#include <SPI.h>
#include "a7105.h"
#include "hubsan.h"

void setup()
{
    Serial.begin(115200);
    A7105_Setup(); //A7105_Reset();
}

void loop()
{
    int startTime, waitTime, hubsanWait, finishTime;
    //Serial.println("Preinit");
    Session *session = initialize();

    startTime = micros();
    while (1) {
        if (Serial.available() > 4) {
            if (Serial.read() != 23) {
                //throttle = rudder = aileron = elevator = 0;
            } else {
                session->throttle = Serial.read();
                session->rudder = Serial.read();
                session->aileron = Serial.read();
                session->elevator = Serial.read();
            }
        }
    
        //if (state!=0 && state!=1 & state!=128)
        //Serial.print("State: ");
        //Serial.println(state);
        hubsanWait = hubsan_cb(session);
        // finishTime=micros();
        // waitTime = hubsanWait - (micros() - startTime);
        // Serial.print("hubsanWait: " ); Serial.println(hubsanWait);
        // Serial.print("waitTime: " ); Serial.println(waitTime);
        //Serial.println(hubsanWait);
        delayMicroseconds(hubsanWait);
        startTime = micros();
    }
}

