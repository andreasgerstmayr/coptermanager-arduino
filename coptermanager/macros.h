#ifndef MACROS_H
#define MACROS_H

#include <Arduino.h>

#define CLOCK_StartTimer(x, y) (void)0
#define CLOCK_StopTimer() (void)0
#define CLOCK_ResetWatchdog() (void)0
#define CLOCK_getms() millis()
#define rand32_r(x, y) rand()
#define usleep(x) delayMicroseconds(x)

#endif
