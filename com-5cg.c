#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#define PACKET_LENGTH 6

// Indexes
#define SYNC_INDEX      0
#define CID_INDEX       1
#define STATUS_INDEX    2
#define VOLT_H_INDEX    3
#define VOLT_L_INDEX    4
#define CHECKSUM_INDEX  5

// Bit Masks for Battery Status
#define DISCHARGE_MASK      (1 << 0)
#define BATTERY_SUPPORTED   (1 << 5)
#define VOLTAGE_OK_MASK     (1 << 6)
#define BATTERY_ERROR_MASK  (1 << 7)

// Sample packet 

//Packet definition
    // SYNC | CID  | Status | Volt_H | Volt_L  | Checksum
    // 0x55 | 0x81 | 0x00   | 0x96   | 0xFE    | 0x3F


uint8_t packet[PACKET_LENGTH] = {0x55, 0x81, 0x00, 0x96, 0xFE, 0x3F};



// Checksum
uint8_t calculate_checksum(uint8_t cid, const uint8_t* data_buffer, uint8_t len) {
    uint16_t tmp = cid;
    uint8_t i;

    for (i = 0; i < len; i++) {
        tmp += data_buffer[i];
        
        if (tmp >= 256u) {
            tmp -= 255u; // This is an unusual overflow handling as per document
        }
    }
    tmp = (~tmp) & 0x00FFu; // Bitwise NOT and mask to 8 bits
    return (uint8_t)tmp;
}


// Decode battery status bits
void decode_battery_status(uint8_t status) {
   printf("Battery Status:\n");
   if (!(status & DISCHARGE_MASK))
       printf(" Battery can be discharged\n");
   if (!(status & BATTERY_SUPPORTED))
       printf(" Battery is supported\n");
   else
       printf(" Battery NOT supported\n");
   if (!(status & VOLTAGE_OK_MASK))
       printf(" Battery voltage is OK\n");
   else
       printf(" Battery voltage NOT OK\n");
   if (!(status & BATTERY_ERROR_MASK))
       printf(" Battery has NO error\n");
   else
       printf(" Battery has error\n");
}


int main() {
   // Step 1: Check checksum
   uint8_t calculated = calculate_checksum(  packet[1],  &packet[2], PACKET_LENGTH - 3 );
   uint8_t received = packet[CHECKSUM_INDEX];   
   if (calculated != received) {
       printf("Checksum mismatch! Calculated: 0x%X, Received: 0x%X\n", calculated, received);
       return -1;
   }
   printf("Checksum valid!\n");    //this line run if checksum is right

  
  
   // Step 2: Decode battery status  
   uint8_t status = packet[STATUS_INDEX];
   decode_battery_status(status);
   


   // Step 3: Only print voltage if supported and no error
   bool supported = !(status & BATTERY_SUPPORTED);
   bool error = status & BATTERY_ERROR_MASK;
   if (supported && !error) {
       uint16_t voltage_raw = ((uint16_t)packet[VOLT_H_INDEX] << 8) | packet[VOLT_L_INDEX];
       printf("Battery Voltage: %u V\n", voltage_raw/1000);
   } else {
       printf("Battery not supported or has error. Voltage not displayed.\n");
   }
   return 0;
};
