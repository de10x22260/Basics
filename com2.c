//Using macros and not using hardcode values

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define FRAME_SIZE 6
#define START_SIZE 0x55
#define HEADER_BYTE 0x81
#define VOLTAGE_MSB 3
#define VOLTAGE_LSB 4


int main(){
    uint8_t data[] ={0x55 , 0x81 ,0x20 ,0x6D ,0x60, 0x00};
    
    uint16_t voltage = (data[3] << 8 ) | data[4];
    
    printf("Voltage in volt: %d Volts",voltage/1000);
    return 0;
}
