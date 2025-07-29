#include <stdio.h>
#include <stdint.h>

int main(){
    uint8_t data[] ={0x55 , 0x81 ,0x20 ,0x6D ,0x60, 0x00};
    
    uint16_t voltage = (data[3] << 8 ) | data[4];
    
    printf("Voltage in volt: %d Volts",voltage/1000);
    return 0;
}
