#ifndef HUBSAN_H
#define HUBSAN_H

#include "session.h"

static const u8 allowed_channels[] = {0x14, 0x1e, 0x28, 0x32, 0x3c, 0x46, 0x50, 0x5a, 0x64, 0x6e, 0x78, 0x82};

void hubsan_initialize();
HubsanSession* hubsan_bind(int copterid, Session* sessions[], int num_sessions);
u16 hubsan_cb(HubsanSession* session);
u16 hubsan_multiple_cb(HubsanSession* session);
int hubsan_get_binding_state(HubsanSession* session);

#endif
