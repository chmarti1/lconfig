#include "lconfig.h"
#include <string.h>

void main(){
    lc_devconf_t dconf;
    unsigned char txbuffer[4];
    unsigned char rxbuffer[4];
    int ii;
    
    memset(rxbuffer, 0x00, 4);
    txbuffer[0] = 0x00;
    txbuffer[1] = 0xAA;
    txbuffer[2] = 0x00;
    txbuffer[3] = 0x55;
    
    lc_load_config(&dconf, 1, "test.conf");
    lc_open(&dconf);
    lc_show_config(&dconf);
    for(ii=0; ii<4; ii++)
        printf("TX %d: 0x%02x\n", ii, txbuffer[ii]);
    lc_communicate(&dconf, 0, txbuffer, 4, rxbuffer, 4, 4000);
    lc_close(&dconf);
    for(ii=0; ii<4; ii++)
        printf("RX %d: 0x%02x\n", ii, rxbuffer[ii]);
}
