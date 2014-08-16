/*
 This project is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Deviation is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Deviation.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "protocol.h"
#include "macros.h"

#define PROTOCOL_SticksMoved(x) (void)0

static u8 proto_state;
static u32 bind_time;

#define PROTO_READY     0x02
#define PROTO_BINDING   0x04
#define PROTO_BINDDLG   0x08

void PROTOCOL_SetBindState(u32 msec)
{
    if (msec) {
        if (msec == 0xFFFFFFFF)
            bind_time = msec;
        else
            bind_time = CLOCK_getms() + msec;
        proto_state |= PROTO_BINDING;
        PROTOCOL_SticksMoved(1);  //Initialize Stick position
    } else {
        proto_state &= ~PROTO_BINDING;
    }
}
