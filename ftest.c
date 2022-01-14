#include <stdio.h>
#include "lconfig.h"

int main(){
    FILE *f;
    float value;
    
    f = fopen("test.dat", "rb");
    
    fprintf(f, "%0.6f\n", value);
    fwrite(&value, sizeof(value), 1, f);
    
    fclose(f);

    return 0;
}
