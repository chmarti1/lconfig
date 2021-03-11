#include <unistd.h>
#include <stdlib.h>

#define LEN 16

int main(void){
    int ii, jj;
    char * a[LEN];
    a = malloc(4 * LEN * sizeof(char));
    
    for(ii=0;ii<4;ii++){
        for(jj=0;jj<4;jj++){
            a[ii][jj] = 0;
        }
    }
    
    free(a);
}
