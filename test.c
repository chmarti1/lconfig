#include "lconfig.h"
#include <string.h>

void main(){
    DEVCONF dconf[1];
    unsigned char txbuffer[4];
    unsigned char rxbuffer[4];
    int ii;
    
    memset(rxbuffer, 0x00, 4);
    txbuffer[0] = 0x00;
    txbuffer[1] = 0xAA;
    txbuffer[2] = 0x00;
    txbuffer[3] = 0x55;
    
    load_config(dconf, 1, "test.conf");
    open_config(dconf,0);
    for(ii=0; ii<4; ii++)
        printf("TX %d: 0x%02x\n", ii, txbuffer[ii]);
    communicate(dconf, 0, 0, txbuffer, 4, rxbuffer, 4, 4000);
    close_config(dconf,0);
    for(ii=0; ii<4; ii++)
        printf("RX %d: 0x%02x\n", ii, rxbuffer[ii]);
}
