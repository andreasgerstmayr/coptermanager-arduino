/*
  Thanks to Cass May, https://github.com/cassm/a7105
*/

#include <Arduino.h>
#include "diagnostics.h"
#include "a7105.h"
#include "hubsan.h"

u8 receivedpacket[16];
u8 no_allowed_channels = sizeof(allowed_channels)/sizeof(allowed_channels[0]);

// print the contents of packet in a human-readable way
void printpacket(u8 packet[]) {
  int j;
  Serial.print("Packet received: ");
  for (j = 0 ; j < 16 ; j++) {
      if (packet[j] < 10)
          Serial.print("0");
      Serial.print(packet[j], HEX);
      Serial.print(" ");
  }
  Serial.println("");
}

// Eavesdrop on a hubsan exchange. This must be started prior to the binding exchange.
void eavesdrop() {
    u8 prebind_packet[16];
    int wait_start, wait_end;
    
    Serial.println("Eavesdropping...");
    
    // use findchannel to locate the channel which is currently being broadcast on
    u8 sess_channel = A7105_findchannel();
    
    // strobe to receiver mode, intercept a packet    
    A7105_Strobe(A7105_RX);  
    while(A7105_ReadReg(A7105_00_MODE) & 0x01)
        delayMicroseconds(1);
    A7105_ReadData(prebind_packet, 16);
    
    // use the data from the packet to switch the chip over to the transactions session ID
    A7105_WriteID((prebind_packet[2] << 24) | (prebind_packet[3] << 16) | (prebind_packet[4] << 8) | prebind_packet[5]);
    
    // It is now acceptable to allow the Tx to bind with an Rx
    
    // measure haw long it takes for the next packet to arrive
    wait_start = micros();
    while(true) {
        A7105_Strobe(A7105_RX);  
        while(A7105_ReadReg(A7105_00_MODE) & 0x01) {
            delayMicroseconds(1);
            
            // if it takes more than 5 seconds, we can assume that the session has been terminated
            if ((micros()-wait_start) > 5000000) {
                Serial.println("Session terminated. Rescanning...");
                // ...and therefore start listening for pre-bind packets again
                return;
            }
        }
        
        // record the end time, retrieve the packet
        wait_end = micros();
        A7105_ReadData(receivedpacket, 16);
        
        // print the packet along with the time interval between it and the previous packet
        int timeinterval = (wait_end - wait_start)/1000;
        if (timeinterval < 10)
            Serial.print("0");
        Serial.print(timeinterval);
        
        // start timing for the next packet  
        wait_start = micros();
        Serial.print("us: ");
        printpacket(receivedpacket);
    }
}

// find which channel the binding exchange is taking place on
u8 A7105_findchannel() { 
    int pack_count;
    while (true) {
      // scan all allowed channels
       for (int i = 0 ; i < no_allowed_channels ; i++) {
          pack_count = 0;
          A7105_WriteReg(A7105_0F_CHANNEL, allowed_channels[i]);

          // Listen on channel
          for (int j = 0 ; j < 20 ; j++)
              pack_count += A7105_sniffchannel();

          // if channel has more than 3 packets, assume channel is correct
          if (pack_count > 3) {
              Serial.println("Channel found: "+String(allowed_channels[i], HEX));
              return allowed_channels[i];
          }
       }
    }
}

// sniffs the currently set channel, prints packets to serial
int A7105_sniffchannel() {
       // put chip in receiver mode
       A7105_Strobe(A7105_RX);  

       // wait for packet to arrive
       delayMicroseconds(3000);

       // if flag is high, buffer has filled, meaning a packet has been intercepted
       if(!(A7105_ReadReg(A7105_00_MODE) & 0x01)) {

           // accept and print packet
           A7105_ReadData(receivedpacket, 16);
           printpacket(receivedpacket);
           return 1;
       }
       else
         return 0;
}

// version of sniffchannel which sniffs a channel other than the one which is currently set.
void A7105_sniffchannel(u8 _channel) {
      Serial.print("Switching to channel ");
      Serial.println(_channel);
      Serial.println("");
      A7105_WriteReg(A7105_0F_CHANNEL, _channel);
      while (1) {
          A7105_sniffchannel();
      }
}

// function to sniff a list of channels and see what is being broadcast on them
// attempts sniffing 20 times on each channel before looping, print any results to serial
void A7105_scanchannels(const u8 channels[]) {
    int packetsreceived;
    for (int i = 0 ; i < no_allowed_channels ; i++) {
          packetsreceived = 0;
          Serial.println("");
          Serial.print("Switching to channel ");
          Serial.println(channels[i], HEX);
          Serial.println("");
          A7105_WriteReg(A7105_0F_CHANNEL, channels[i]);
          for (int j = 0 ; j < 20 ; j++) {
              packetsreceived += A7105_sniffchannel();
          }
          if (packetsreceived) {
              Serial.print(packetsreceived);
              Serial.print(" packets received on channel ");
              Serial.println(channels[i], HEX);
          }
          else {
              Serial.print("Channel ");
              Serial.print(channels[i], HEX);
              Serial.println(" is clear.");
          }
      }
}
