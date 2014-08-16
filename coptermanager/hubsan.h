#ifndef HUBSAN_H
#define HUBSAN_H

#include "session.h"

void hubsan_initialize();
HubsanSession* hubsan_bind();
u16 hubsan_cb(HubsanSession* session);

#endif
