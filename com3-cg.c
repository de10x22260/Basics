#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

//Macros
#define FRAME_SIZE           6
#define START_BYTE           0x55
#define CMD_ID_INDEX         1
#define ALARM_BYTE_INDEX     2
#define VOLTAGE_MSB_INDEX    3
#define VOLTAGE_LSB_INDEX    4
#define CHECKSUM_INDEX       5
#define NO_BATTERY_ALARM_BIT 0x01  // Bit 0 in alarm byte
#define IS_VALID_FRAME_START(f)    ((f[0] == START_BYTE))

//Checksum 
uint8_t ODM_com_checksum_calc(uint8_t CID, uint8_t p_data[], uint8_t len) {
   uint16_t tmp = CID;
   for (uint8_t i = 0; i < len; i++) {
       tmp += p_data[i];
       if (tmp >= 256u)
           tmp -= 255u;
   }
   tmp = (~tmp) & 0x00FFu;
   return (uint8_t)tmp;
}



//Driver code
int main() {
   // Example frame: {Start, CID, Alarm, Voltage MSB, Voltage LSB, Checksum}
   uint8_t frame[FRAME_SIZE] = {0x55, 0x81, 0x00, 0x6D, 0x60, 0xD2};
   if (!IS_VALID_FRAME_START(frame)) {
       printf("Invalid start byte.\n");
       return 0;
   }
   // Extract CID and data pointer (from Alarm byte onward)
   uint8_t cid = frame[CMD_ID_INDEX];
   uint8_t *data = &frame[ALARM_BYTE_INDEX];
   uint8_t checksum_calc = ODM_com_checksum_calc(cid, data, 3);  // Alarm + Voltage bytes (3 bytes)
   if (checksum_calc != frame[CHECKSUM_INDEX]) {
       printf("Checksum mismatch. Expected: 0x%02X, Found: 0x%02X\n", checksum_calc, frame[CHECKSUM_INDEX]);
       return 0;
   }

   
   // Check if battery is present
   if ((frame[ALARM_BYTE_INDEX] & NO_BATTERY_ALARM_BIT) == 0) {
       uint16_t voltage_mv = (frame[VOLTAGE_MSB_INDEX] << 8) | frame[VOLTAGE_LSB_INDEX];
       printf("Battery Voltage: %d mV\n", voltage_mv/1000);                               //voltage calculate in volts
   } else {
       printf("No Battery Alarm Active. Skipping voltage read.\n");
   }
   return 0;
}
