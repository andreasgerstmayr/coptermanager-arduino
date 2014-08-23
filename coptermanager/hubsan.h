#ifndef HUBSAN_H
#define HUBSAN_H

#include "session.h"

void hubsan_initialize();
HubsanSession* hubsan_bind(int copterid, Session* sessions[], int num_sessions);
u16 hubsan_cb(HubsanSession* session);
u16 hubsan_multiple_cb(HubsanSession* session);
int hubsan_get_binding_state(HubsanSession* session);

#endif
