#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H

#include "common.h"

void printpacket(u8 packet[]);
void eavesdrop();
u8 A7105_findchannel();
int A7105_sniffchannel();
void A7105_sniffchannel(u8 _channel);
void A7105_scanchannels(const u8 channels[]);

#endif
