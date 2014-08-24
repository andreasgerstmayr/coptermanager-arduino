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
#include "hubsan.h"
#include "common.h"
#include "macros.h"
#include "a7105.h"

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
    if (CLOCK_getms() - ms >= 500) {
        DEBUG_MSG("calibration failed");
        return 0;
    }
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
    if (CLOCK_getms() - ms >= 500) {
        return 0;
    }
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

static void update_crc(HubsanSession *session)
{
    int sum = 0;
    for(int i = 0; i < 15; i++)
        sum += session->packet[i];
    session->packet[15] = (256 - (sum % 256)) & 0xff;
}

static void hubsan_build_bind_packet(HubsanSession *session, u8 state)
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
    session->packet[11] = (session->txid >> 24) & 0xff;
    session->packet[12] = (session->txid >> 16) & 0xff;
    session->packet[13] = (session->txid >>  8) & 0xff;
    session->packet[14] = (session->txid >>  0) & 0xff;
    update_crc(session);
}

static void hubsan_build_packet(HubsanSession *session)
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
    session->packet[11] = (session->txid >> 24) & 0xff;
    session->packet[12] = (session->txid >> 16) & 0xff;
    session->packet[13] = (session->txid >>  8) & 0xff;
    session->packet[14] = (session->txid >>  0) & 0xff;
    update_crc(session);
}

static u8 hubsan_check_integrity(HubsanSession *session) 
{
    int sum = 0;
    for(int i = 0; i < 15; i++)
        sum += session->packet[i];
    return session->packet[15] == ((256 - (sum % 256)) & 0xff);
}

enum Tag0xe0 {
    TAG, 
    ROC_MSB,
    ROC_LSB,
    XUNK3,
    XUNK4, 
    XUNK5,
    XUNK6,
    XUNK7,
    XUNK8, 
    Z_ACC_MSB, 
    Z_ACC_LSB, 
    YAW_GYRO_MSB, 
    YAW_GYRO_LSB, 
    VBAT,
    CRC0,
    CRC1
};

enum Tag0xe1 {
    TAG1, 
    PITCH_ACC_MSB,
    PITCH_ACC_LSB,
    ROLL_ACC_MSB,
    ROLL_ACC_LSB, 
    UNK5,
    UNK6,
    PITCH_GYRO_MSB, 
    PITCH_GYRO_LSB, 
    ROLL_GYRO_MSB, 
    ROLL_GYRO_LSB, 
    UNK11,
    UNK12,  
    //VBAT,
    //CRC0,
    //CRC1
};

enum rc { // must be in this order
    ROLL,
    PITCH,
    YAW,
    THROTTLE,
    AUX1,
    AUX2,
    AUX3,
    AUX4
};

static void hubsan_update_telemetry(HubsanSession *session)
{
    if(hubsan_check_integrity(session)) {
        switch (session->packet[0]) {
        case 0xE0:
            session->estAltitude = -(session->packet[ROC_MSB] << 8 | session->packet[ROC_LSB]);// 1,2
            //session->debug[0] = session->packet[3] << 8 | session->packet[4];
            //session->debug[1] = session->packet[5] << 8 | session->packet[6]; 
            //session->debug[2] = session->packet[7] << 8 | session->packet[8]; 
            session->accData[YAW] = session->packet[Z_ACC_MSB] << 8 | session->packet[Z_ACC_LSB]; // OK
            session->gyroData[YAW] = session->packet[YAW_GYRO_MSB] << 8 | session->packet[YAW_GYRO_LSB]; 
            // batteryVolts = session->packet[VBAT];    // only in 0xe1 Packet! (http://www.deviationtx.com/forum/protocol-development/1848-new-hubsan-upgraded-version-on-the-way?start=340#17668)
            break;
        case 0xE1:
            session->accData[PITCH] = session->packet[PITCH_ACC_MSB] << 8 | session->packet[PITCH_ACC_LSB];  
            session->accData[ROLL] = session->packet[ROLL_ACC_MSB] << 8 | session->packet[ROLL_ACC_LSB]; 

            session->gyroData[PITCH] = session->packet[PITCH_GYRO_MSB] << 8 | session->packet[PITCH_GYRO_LSB]; 
            session->gyroData[ROLL] = session->packet[ROLL_GYRO_MSB] << 8 | session->packet[ROLL_GYRO_LSB]; 

            // use acc as angle alias
            session->angle[PITCH] = session->accData[PITCH];
            session->angle[ROLL] = -session->accData[ROLL];
            session->batteryVolts = session->packet[VBAT];
            break;
        default:
            break;
        }
        
        #ifdef DEBUG_TELEMETRY
            Serial.print ("Telemetry data: ");
            for(int i=0; i<16;i++) {
                if (session->packet[i] <= 15)
                    Serial.print("0");
                Serial.print(session->packet[i], HEX);
                Serial.print(" ");
            }
            
            Serial.print ("  Alt: " + String(session->estAltitude));
            Serial.print (", Battery: " + String(session->batteryVolts));
            Serial.print (", Gyro:");
            for(int i=0; i<3; i++) {
                Serial.print(" "+String(session->gyroData[i]));
            }
            Serial.print (", Acc:");
            for(int i=0; i<3; i++) {
                Serial.print(" "+String(session->accData[i]));
            }
            Serial.print (", Angle:");
            for(int i=0; i<3; i++) {
                Serial.print(" "+String(session->angle[i]));
            }
            Serial.println();
        #endif
    }
}

inline boolean A7105_busy(void) {
    return A7105_ReadReg(A7105_00_MODE) & 0x01;
}

u16 hubsan_cb(HubsanSession *session)
{
    // odd stages: TX -> RX (sending)
    // even stages: RX -> TX (receiving)

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
        if (i == 20)
            DEBUG_MSG("Failed to complete write\n");
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
            //DEBUG_MSG("No signal, restart binding procedure");
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
            //DEBUG_MSG("Transmittion in progress, waiting");
            return 15000; //22.5msec elapsed since last write
        }
        A7105_ReadData(session->packet, 16);
        if(session->packet[1] == 9) {
            session->state = DATA_1;
            A7105_WriteReg(A7105_1F_CODE_I, 0x0F);
            //PROTOCOL_SetBindState(0);
            DEBUG_MSG("copter bound.");
            return 28000; //35.5msec elapsed since last write
        } else {
            session->state = BIND_7;
            DEBUG_MSG("Got unknown packet, first 2 bytes: "+String(session->packet[0], HEX) + " " + String(session->packet[1], HEX));
            return 15000; //22.5 msec elapsed since last write
        }
    case DATA_1:
        A7105_SetPower(Model.tx_power); // keep transmit power in sync
    case DATA_2:
    case DATA_3:
    case DATA_4:
    case DATA_5:
        uint16_t d;
        switch (session->telemetryState) { // Goebish - telemetry is every ~0.1S r 10 Tx packets
        case doTx:
            hubsan_build_packet(session);
            A7105_Strobe(A7105_STANDBY);
            A7105_WriteData(session->packet, 16, session->state == DATA_5 ? session->channel + 0x23 : session->channel);
            d = 3000; // nominal tx time
            session->telemetryState = waitTx;
            break;
        case waitTx: 
            if(A7105_busy())
                d = 0;
            else { // wait for tx completion
                A7105_Strobe(A7105_RX);
                session->polls = 0; 
                session->telemetryState = pollRx;
                d = 3000; // nominal rx time
            } 
            break;
        case pollRx: // check for telemetry
            if(A7105_busy()) 
                d = 1000;
            else { 
                A7105_ReadData(session->packet, 16);
                hubsan_update_telemetry(session);
                A7105_Strobe(A7105_RX);
                d = 1000; 
            }

            if (++session->polls >= 7) { // 3ms + 3mS + 4*1ms
                if (session->state == DATA_5) 
                    session->state = DATA_1;
                else 
                    session->state++;  
                session->telemetryState = doTx;   
            }
            break;
        }
        return d;
    }
    return 0;
}

u16 hubsan_multiple_cb(HubsanSession *session)
{
    static HubsanSession *bindingSession = NULL;
    int i;
    switch(session->state) {
    case BIND_1:
        bindingSession = session;
        A7105_WriteID(0x55201041);
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
        for(int i = 0; i < 20; i++) {
           if(! (A7105_ReadReg(A7105_00_MODE) & 0x01))
               break;
        }
        if (i == 20)
            DEBUG_MSG("Failed to complete write\n");
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
            bindingSession = NULL;
            //PROTOCOL_SetBindState(0);
            DEBUG_MSG("another copter bound.");
            return 28000; //35.5msec elapsed since last write
        } else {
            session->state = BIND_7;
            return 15000; //22.5 msec elapsed since last write
        }
    case DATA_1:
        A7105_SetPower(Model.tx_power); // keep transmit power in sync
    case DATA_2:
    case DATA_3:
    case DATA_4:
    case DATA_5:
        if (bindingSession == NULL) {
            hubsan_build_packet(session);
            A7105_WriteID(session->sessionid);
            A7105_WriteData(session->packet, 16, session->state == DATA_5 ? session->channel + 0x23 : session->channel);
            
            // wait for completion
            for(i = 0; i < 20; i++) {
                if(!(A7105_ReadReg(A7105_00_MODE) & 0x01))
                    break;
            }
            if (i == 20)
                DEBUG_MSG("Failed to complete write\n");
        }
        
        if (session->state == DATA_5)
            session->state = DATA_1;
        else
            session->state++;
        return 10000;
    }
    return 0;
}

void hubsan_initialize()
{
    CLOCK_StopTimer();
    while(1) {
        A7105_Reset();
        CLOCK_ResetWatchdog();
        DEBUG_MSG("hubsan_init()");
        if (hubsan_init())
            break;
    }
}

u32 find_free_sessionid(Session* sessions[], int num_sessions)
{
    u32 sessionid;
    int isfree = false;
    while(!isfree) {
        sessionid = rand32_r(0, 0);
        isfree = true;
        for(int i=0; i<num_sessions; i++) {
            if (sessions[i]->copterType == HUBSAN_X4) {
                if (((HubsanSession*)sessions[i]->copterSession)->sessionid == sessionid) {
                    isfree = false;
                    break;
                }
            }
        }
    }
    return sessionid;
}

u8 find_free_channel(Session* sessions[], int num_sessions)
{
    u8 channel;
    int isfree = false;
    while(!isfree) {
        channel = allowed_channels[rand32_r(0, 0) % sizeof(allowed_channels)];
        isfree = true;
        for(int i=0; i<num_sessions; i++) {
            if (sessions[i]->copterType == HUBSAN_X4) {
                if (((HubsanSession*)sessions[i]->copterSession)->channel == channel) {
                    isfree = false;
                    break;
                }
            }
        }
    }
    return channel;
}

HubsanSession* hubsan_bind(int copterid, Session* sessions[], int num_sessions)
{
    HubsanSession *session = (HubsanSession*)malloc(sizeof(HubsanSession));
    memset(session, 0, sizeof(HubsanSession));
    
    // hubsan default values
    session->rudder = session->aileron = session->elevator = 0x7F;
    session->led = 1;
   
    session->txid = 0xdb042679 + copterid;
    session->sessionid = find_free_sessionid(sessions, num_sessions);
    #ifdef FIXED_CHANNEL
        session->channel = FIXED_CHANNEL;
    #else
        session->channel = find_free_channel(sessions, num_sessions);
    #endif
    //PROTOCOL_SetBindState(0xFFFFFFFF);
    session->state = BIND_1;
    session->packet_count=0;
    //memset(&Telemetry, 0, sizeof(Telemetry));
    //TELEMETRY_SetType(TELEM_DEVO);
    if( Model.proto_opts[PROTOOPTS_VTX_FREQ] == 0)
        Model.proto_opts[PROTOOPTS_VTX_FREQ] = 5885;
    CLOCK_StartTimer(10000, hubsan_cb);
    return session;
}

int hubsan_get_binding_state(HubsanSession *session) {
    return session->state >= DATA_1 && session->state <= DATA_5;
}
