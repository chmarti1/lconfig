#include <stdio.h>
#include "lconfig.h"

int main(){
    FILE *f;
    float value;
    
    value = 123.456;
    
    f = fopen("test.dat", "wb");
    
    fprintf(f, "# This is a test\n");
    fprintf(f, "%f\n", value);
    fwrite(&value, sizeof(value), 1, f);
    
    fclose(f);

    return 0;
}
