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

#include <Arduino.h>
#include "common.h"
#include "macros.h"
#include "a7105.h"
#include "protocol.h"

static struct Model Model = {
    {0, 0, 0, 0},
    TXPOWER_10mW
};

#define TELEM_ON 0
#define TELEM_OFF 1

enum{
    FLAG_VIDEO= 0x01,   // record video
    FLAG_FLIP = 0x08,   // enable flips
    FLAG_LED  = 0x04    // enable LEDs
};

#define VTX_STEP_SIZE "5"

enum {
    PROTOOPTS_VTX_FREQ = 0,
    PROTOOPTS_TELEMETRY,
    LAST_PROTO_OPT,
};

static const u8 allowed_ch[] = {0x14, 0x1e, 0x28, 0x32, 0x3c, 0x46, 0x50, 0x5a, 0x64, 0x6e, 0x78, 0x82};
static const u32 txid = 0xdb042679;

enum {
    BIND_1,
    BIND_2,
    BIND_3,
    BIND_4,
    BIND_5,
    BIND_6,
    BIND_7,
    BIND_8,
    DATA_1,
    DATA_2,
    DATA_3,
    DATA_4,
    DATA_5,
};
#define WAIT_WRITE 0x80

static int hubsan_init()
{
    u8 if_calibration1;
    u8 vco_calibration0;
    u8 vco_calibration1;
    //u8 vco_current;

    A7105_WriteID(0x55201041);
    A7105_WriteReg(A7105_01_MODE_CONTROL, 0x63);
    A7105_WriteReg(A7105_03_FIFOI, 0x0f);
    A7105_WriteReg(A7105_0D_CLOCK, 0x05);
    A7105_WriteReg(A7105_0E_DATA_RATE, 0x04);
    A7105_WriteReg(A7105_15_TX_II, 0x2b);
    A7105_WriteReg(A7105_18_RX, 0x62);
    A7105_WriteReg(A7105_19_RX_GAIN_I, 0x80);
    A7105_WriteReg(A7105_1C_RX_GAIN_IV, 0x0A);
    A7105_WriteReg(A7105_1F_CODE_I, 0x07);
    A7105_WriteReg(A7105_20_CODE_II, 0x17);
    A7105_WriteReg(A7105_29_RX_DEM_TEST_I, 0x47);

    A7105_Strobe(A7105_STANDBY);

    //IF Filter Bank Calibration
    A7105_WriteReg(0x02, 1);
    //vco_current =
    A7105_ReadReg(0x02);
    u32 ms = CLOCK_getms();
    CLOCK_ResetWatchdog();
    while(CLOCK_getms()  - ms < 500) {
        if(! A7105_ReadReg(0x02))
            break;
    }
    if (CLOCK_getms() - ms >= 500)
        return 0;
    if_calibration1 = A7105_ReadReg(A7105_22_IF_CALIB_I);
    A7105_ReadReg(A7105_24_VCO_CURCAL);
    if(if_calibration1 & A7105_MASK_FBCF) {
        //Calibration failed...what do we do?
        return 0;
    }

    //VCO Current Calibration
    //A7105_WriteReg(0x24, 0x13); //Recomended calibration from A7105 Datasheet

    //VCO Bank Calibration
    //A7105_WriteReg(0x26, 0x3b); //Recomended limits from A7105 Datasheet

    //VCO Bank Calibrate channel 0?
    //Set Channel
    A7105_WriteReg(A7105_0F_CHANNEL, 0);
    //VCO Calibration
    A7105_WriteReg(0x02, 2);
    ms = CLOCK_getms();
    CLOCK_ResetWatchdog();
    while(CLOCK_getms()  - ms < 500) {
        if(! A7105_ReadReg(0x02))
            break;
    }
    if (CLOCK_getms() - ms >= 500)
        return 0;
    vco_calibration0 = A7105_ReadReg(A7105_25_VCO_SBCAL_I);
    if (vco_calibration0 & A7105_MASK_VBCF) {
        //Calibration failed...what do we do?
        return 0;
    }

    //Calibrate channel 0xa0?
    //Set Channel
    A7105_WriteReg(A7105_0F_CHANNEL, 0xa0);
    //VCO Calibration
    A7105_WriteReg(A7105_02_CALC, 2);
    ms = CLOCK_getms();
    CLOCK_ResetWatchdog();
    while(CLOCK_getms()  - ms < 500) {
        if(! A7105_ReadReg(A7105_02_CALC))
            break;
    }
    if (CLOCK_getms() - ms >= 500)
        return 0;
    vco_calibration1 = A7105_ReadReg(A7105_25_VCO_SBCAL_I);
    if (vco_calibration1 & A7105_MASK_VBCF) {
        //Calibration failed...what do we do?
    }

    //Reset VCO Band calibration
    //A7105_WriteReg(0x25, 0x08);
    A7105_SetTxRxMode(TX_EN);

    A7105_SetPower(Model.tx_power);


    A7105_Strobe(A7105_STANDBY);
    return 1;
}

static void update_crc(Session *session)
{
    int sum = 0;
    for(int i = 0; i < 15; i++)
        sum += session->packet[i];
    session->packet[15] = (256 - (sum % 256)) & 0xff;
}
static void hubsan_build_bind_packet(Session *session, u8 state)
{
    session->packet[0] = state;
    session->packet[1] = session->channel;
    session->packet[2] = (session->sessionid >> 24) & 0xff;
    session->packet[3] = (session->sessionid >> 16) & 0xff;
    session->packet[4] = (session->sessionid >>  8) & 0xff;
    session->packet[5] = (session->sessionid >>  0) & 0xff;
    session->packet[6] = 0x08;
    session->packet[7] = 0xe4; //???
    session->packet[8] = 0xea;
    session->packet[9] = 0x9e;
    session->packet[10] = 0x50;
    session->packet[11] = (txid >> 24) & 0xff;
    session->packet[12] = (txid >> 16) & 0xff;
    session->packet[13] = (txid >>  8) & 0xff;
    session->packet[14] = (txid >>  0) & 0xff;
    update_crc(session);
}

static void hubsan_build_packet(Session *session)
{
    static s16 vtx_freq = 0; 
    memset(session->packet, 0, 16);
    if(vtx_freq != Model.proto_opts[PROTOOPTS_VTX_FREQ] || session->packet_count==100) // set vTX frequency (H107D)
    {
        vtx_freq = Model.proto_opts[PROTOOPTS_VTX_FREQ];
        session->packet[0] = 0x40;
        session->packet[1] = (vtx_freq >> 8) & 0xff;
        session->packet[2] = vtx_freq & 0xff;
        session->packet[3] = 0x82;
        session->packet_count++;      
    }
    else //20 00 00 00 80 00 7d 00 84 02 64 db 04 26 79 7b
    {
        session->packet[0] = 0x20;
        session->packet[2] = session->throttle; //Throttle
    }
    session->packet[4] = 0xff - session->rudder; //Rudder is reversed
    session->packet[6] = 0xff - session->elevator; //Elevator is reversed
    session->packet[8] = session->aileron; //Aileron 
    if(session->packet_count < 100)
    {
        session->packet[9] = 0x02 | FLAG_LED | FLAG_FLIP; // sends default value for the 100 first packets
        session->packet_count++;
    }
    else
    {
        session->packet[9] = 0x02;
        // Channel 5
        if(session->led)
            session->packet[9] |= FLAG_LED;
        // Channel 6
        if(session->flip >= 0)
            session->packet[9] |= FLAG_FLIP;
        // Channel 7
        if(session->video >0) // off by default
            session->packet[9] |= FLAG_VIDEO;
    }
    session->packet[10] = 0x64;
    session->packet[11] = (txid >> 24) & 0xff;
    session->packet[12] = (txid >> 16) & 0xff;
    session->packet[13] = (txid >>  8) & 0xff;
    session->packet[14] = (txid >>  0) & 0xff;
    update_crc(session);
}

static u8 hubsan_check_integrity(Session *session) 
{
    int sum = 0;
    for(int i = 0; i < 15; i++)
        sum += session->packet[i];
    return session->packet[15] == ((256 - (sum % 256)) & 0xff);
}

static void hubsan_update_telemetry()
{
}

u16 hubsan_cb(Session *session)
{
    static u8 txState = 0;
    static int delay = 0;
    static u8 rfMode=0;
    int i;
    switch(session->state) {
    case BIND_1:
    case BIND_3:
    case BIND_5:
    case BIND_7:
        hubsan_build_bind_packet(session, session->state == BIND_7 ? 9 : (session->state == BIND_5 ? 1 : session->state + 1 - BIND_1));
        A7105_Strobe(A7105_STANDBY);
        A7105_WriteData(session->packet, 16, session->channel);
        session->state |= WAIT_WRITE;
        return 3000;
    case BIND_1 | WAIT_WRITE:
    case BIND_3 | WAIT_WRITE:
    case BIND_5 | WAIT_WRITE:
    case BIND_7 | WAIT_WRITE:
        //wait for completion
        for(i = 0; i< 20; i++) {
           if(! (A7105_ReadReg(A7105_00_MODE) & 0x01))
               break;
        }
        //if (i == 20)
        //    printf("Failed to complete write\n");
        A7105_SetTxRxMode(RX_EN);
        A7105_Strobe(A7105_RX);
        session->state &= ~WAIT_WRITE;
        session->state++;
        return 4500; //7.5msec elapsed since last write
    case BIND_2:
    case BIND_4:
    case BIND_6:
        A7105_SetTxRxMode(TX_EN);
        if(A7105_ReadReg(A7105_00_MODE) & 0x01) {
            session->state = BIND_1;
            return 4500; //No signal, restart binding procedure.  12msec elapsed since last write
        }
        A7105_ReadData(session->packet, 16);
        session->state++;
        if (session->state == BIND_5)
            A7105_WriteID((session->packet[2] << 24) | (session->packet[3] << 16) | (session->packet[4] << 8) | session->packet[5]);
        
        return 500;  //8msec elapsed time since last write;
    case BIND_8:
        A7105_SetTxRxMode(TX_EN);
        if(A7105_ReadReg(A7105_00_MODE) & 0x01) {
            session->state = BIND_7;
            return 15000; //22.5msec elapsed since last write
        }
        A7105_ReadData(session->packet, 16);
        if(session->packet[1] == 9) {
            session->state = DATA_1;
            A7105_WriteReg(A7105_1F_CODE_I, 0x0F);
            PROTOCOL_SetBindState(0);
            return 28000; //35.5msec elapsed since last write
        } else {
            session->state = BIND_7;
            return 15000; //22.5 msec elapsed since last write
        }
    case DATA_1:
    case DATA_2:
    case DATA_3:
    case DATA_4:
    case DATA_5:
        if( txState == 0) { // send packet
            rfMode = A7105_TX;
            if( session->state == DATA_1)
                A7105_SetPower( Model.tx_power); //Keep transmit power in sync
            hubsan_build_packet(session);
            A7105_Strobe(A7105_STANDBY);
            A7105_WriteData( session->packet, 16, session->state == DATA_5 ? session->channel + 0x23 : session->channel);
            if (session->state == DATA_5)
                session->state = DATA_1;
            else
                session->state++;
            delay=3000;
        }
        else {
            if( Model.proto_opts[PROTOOPTS_TELEMETRY] == TELEM_ON) {
                if( rfMode == A7105_TX) {// switch to rx mode 3ms after packet sent
                    for( i=0; i<10; i++)
                    {
                        if( !(A7105_ReadReg(A7105_00_MODE) & 0x01)) {// wait for tx completion
                            A7105_SetTxRxMode(RX_EN);
                            A7105_Strobe(A7105_RX); 
                            rfMode = A7105_RX;
                            break;
                        }
                    }
                }
                if( rfMode == A7105_RX) { // check for telemetry frame
                    for( i=0; i<10; i++) {
                        if( !(A7105_ReadReg(A7105_00_MODE) & 0x01)) { // data received
                            A7105_ReadData(session->packet, 16);
                            hubsan_update_telemetry();
                            A7105_Strobe(A7105_RX);
                            break;
                        }
                    }
                }
            }
            delay=1000;
        }
        if (++txState == 8) { // 3ms + 7*1ms
            A7105_SetTxRxMode(TX_EN);
            txState = 0;
        }
        return delay;
    }
    return 0;
}

Session* initialize()
{
    Session *session = (Session*)malloc(sizeof(Session));
    *session = EmptySession;
    
    CLOCK_StopTimer();
    while(1) {
        A7105_Reset();
        CLOCK_ResetWatchdog();
        if (hubsan_init())
            break;
    }
    session->sessionid = rand32_r(0, 0);
    session->channel = allowed_ch[rand32_r(0, 0) % sizeof(allowed_ch)];
    PROTOCOL_SetBindState(0xFFFFFFFF);
    session->state = BIND_1;
    session->packet_count=0;
    //memset(&Telemetry, 0, sizeof(Telemetry));
    //TELEMETRY_SetType(TELEM_DEVO);
    if( Model.proto_opts[PROTOOPTS_VTX_FREQ] == 0)
        Model.proto_opts[PROTOOPTS_VTX_FREQ] = 5885;
    CLOCK_StartTimer(10000, hubsan_cb);
    return session;
}

