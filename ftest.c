#include <stdio.h>
#include "lcfilter.h"

int main(){
    int ii;
    tf_t a,b,c;
    tf_init(&a);
    tf_init(&b);
    tf_init(&c);
    tf_construct(&a, 2);
    
    a.a[0] = 1;
    a.a[1] = -1.1;
    a.a[2] = 1;
    
    a.b[0] = 0;
    a.b[1] = 0;
    a.b[2] = 1;
    
    tf_copy(&a, &b);
    
    b.b[2] = 0.1;
    b.b[1] = -0.1;
    b.b[0] = 0;
    
    tf_add_common(&a,&b,&c);

    for(ii=0; ii<=c.order; ii++)
        printf(" %.4e", c.b[ii]);
    printf("\n");
    for(ii=0; ii<=c.order; ii++)
        printf(" %.4e", c.a[ii]);
    printf("\n");
    
    tf_add(&a,&b,&c);

    for(ii=0; ii<=c.order; ii++)
        printf(" %.4e", c.b[ii]);
    printf("\n");
    for(ii=0; ii<=c.order; ii++)
        printf(" %.4e", c.a[ii]);
    printf("\n");
    
    
    tf_destruct(&a);
    tf_destruct(&b);
    tf_destruct(&c);
}
