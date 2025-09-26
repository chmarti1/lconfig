#include <stdio.h>
#include "lcfilter.h"

int main(){
    int ii;
    tf_t g;
    tf_init(&g);
    tf_butterworth(&g, 5, .05);
    for(ii=0; ii<=5; ii++)
        printf(" %.4e", g.b[ii]);
    printf("\n");
    for(ii=0; ii<=5; ii++)
        printf(" %.4e", g.a[ii]);
    printf("\n");
    tf_destruct(&g);
}
