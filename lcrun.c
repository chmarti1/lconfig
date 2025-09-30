
/*
  This file is part of the LCONFIG laboratory configuration system.

    LCONFIG is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    LCONFIG is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with LCONFIG.  If not, see <https://www.gnu.org/licenses/>.

    Authored by C.Martin crm28@psu.edu
*/

#include "lctools.h"
#include "lconfig.h"
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>   // For forming file names from timestamps

#define CONFIG_FILE "lcrun.conf"
#define MAXLOOP     "-1"
#define NBUFFER     65535
#define MAX_DEV        8
#define MAXSTR      128


#define halt(){\
    for(devnum=0; devnum<ndev; devnum++){\
        lc_stream_stop(&dconf[devnum]);\
        lc_close(&dconf[devnum]);\
        lc_clean(&dconf[devnum]);\
        if(dfile[devnum]){fclose(dfile[devnum]);dfile[devnum]=NULL;}\
    }\
};



/*....................
. Help text
.....................*/
const char help_text[] = \
"lcrun [-h] [-d DATAFILE] [-c CONFIGFILE] [-n MAXREAD] [-f|i|s param=value]\n"\
"\n"\
"  Runs a data acquisition job until the user exists with a keystroke.\n"\
"\n"\
"-c CONFIGFILE\n"\
"  By default, LCRUN will look for \"lcrun.conf\" in the working\n"\
"  directory.  This should be an LCONFIG configuration file for the\n"\
"  LabJackT7 containing no more than three device configurations.\n"\
"  The -c option overrides that default.\n"\
"     $ lcrun -c myconfig.conf\n"\
"\n"\
"-d DATAFILE\n"\
"  This option overrides the default continuous data file name\n"\
"  \"YYYYMMDDHHmmSS.dat\"\n"\
"     $ lcrun -d mydatafile\n"\
"  For configurations with only one device, a .dat is automatically\n"\
"  appended.  For configurations with multiple devices, a data file\n"\
"  is created for each device, mydatafile_#.dat"\
"\n"\
"-n MAXREAD\n"\
"  This option accepts an integer number of read operations after which\n"\
"  the data collection will be halted.  The number of samples collected\n"\
"  in each read operation is determined by the NSAMPLE parameter in the\n"\
"  configuration file.  The maximum number of samples allowed per channel\n"\
"  will be MAXREAD*NSAMPLE.  By default, the MAXREAD option is disabled.\n"\
"\n"\
"-f param=value\n"\
"-i param=value\n"\
"-s param=value\n"\
"  These flags signal the creation of a meta parameter at the command\n"\
"  line.  f,i, and s signal the creation of a float, integer, or string\n"\
"  meta parameter that will be written to the data file header.\n"\
"     $ lcrun -f height=5.25 -i temperature=22 -s day=Monday\n"\
"\n"\
"GPLv3\n"\
"(c)2017-2025 C.Martin\n";


/*....................
. Main
.....................*/
int main(int argc, char *argv[]){
    int ndev;   // actual number of devices
                // NDEV is the maximum 
    int devnum;
    char go;    // Flag for whether to continue the stream loop
    char param[MAXSTR];
    // Options
    char    data_file_base[MAXSTR],
            data_file[MAXSTR],
            config_file[MAXSTR] = CONFIG_FILE;

    int     count;     // count the number of loops for safe exit
    // Temporaries
    double ftemp;
    int itemp;
    char stemp[MAXSTR];
    // Config and file
    lc_devconf_t dconf[MAX_DEV];
    time_t start;
    FILE* dfile[MAX_DEV];
    // Streaming data
    double *data;
    unsigned int channels, samples_per_read;
    // Utility
    lct_idle_t idle;

// TO DO:
//  Rewrite option parsing to use optarg
//  Move globals into main()
//  Create a parallel file naming convention
//  Transition stream operations to parallel

    // optarg processing is split in two parts:
    // Save the meta parameters for after the configuration file has been 
    // parsed.  (see below)
    while((go = getopt(argc, argv, "hc:d:n:i:f:s:"))!=-1){
        switch(go){
        case 'c':
            strcpy(config_file, optarg);
        break;
        case 'd':
            strcpy(data_file_base, optarg);
        break;
        case 'n':
            if(sscanf(optarg, "%d", &count)!=1){
                fprintf(stderr, "LCRUN: -c requires an integer, but got: %s\n", optarg);
                return -1;
            }
        break;
        case 'h':
            printf(help_text);
            return 0;
        break;
        case 'f':
        case 'i':
        case 's':
            // do nothing for now; we'll process meta parameters after config
            // load.
        break;
        default:
            fprintf(stderr, "LCRUN: Got unsupported command line option: %c\n", go);
            return -1;
        }
    }

    // Load the configuration
    printf("Loading configuration file...");
    if(lc_load(dconf, MAX_DEV, config_file)){
        printf("FAILED\n");
        fprintf(stderr, "LCRUN failed while loading the configuration file \"%s\"\n", config_file);
        return -1;
    }else
        printf("DONE\n");

    // Detect the number of configured device connections
    ndev = lc_ndev(dconf, MAX_DEV);

    time(&start);
    // Build file names from a timestamp
    if(data_file_base[0] == '\0')
        strftime(data_file_base, MAXSTR, "%Y%m%d%H%M%S", localtime(&start));


    // go back and process meta parameters
    optind=1;
    while((go = getopt(argc, argv, "c:d:n:i:f:s:"))!=-1){
        switch(go){
        case 'c':
        case 'd':
        case 'n':
        break;
        // It's time; let's process the meta parameters
        case 'f':
            if(sscanf(optarg,"%[^=]=%lf",(char*) param, &ftemp) != 2){
                fprintf(stderr, "LCRUN: expected param=float format, but found %s\n", optarg);
                return -1;
            }
            printf("flt:%s = %lf\n",param,ftemp);
            for(devnum=0;devnum<ndev;devnum++){
                if (lc_meta_put_flt(&dconf[devnum], param, ftemp))
                    fprintf(stderr, "LCRUN: failed to set device %d parameter %s to %lf\n", devnum, param, ftemp);
            }
        break;
        case 'i':
            if(sscanf(optarg,"%[^=]=%d",(char*) param, &itemp) != 2){
                fprintf(stderr, "LCRUN: expected param=integer format, but found %s\n", optarg);
                return -1;
            }
            printf("int:%s = %d\n",param,itemp);
            for(devnum=0;devnum<ndev;devnum++){
                if (lc_meta_put_int(&dconf[devnum], param, itemp))
                    fprintf(stderr, "LCRUN: failed to set device %d parameter %s to %d\n", devnum, param, itemp);
            }
            break;
        case 's':
            if(sscanf(optarg,"%[^=]=%s",(char*) param, (char*) stemp) != 2){
                fprintf(stderr, "LCRUN: expected param=string format, but found %s\n", optarg);
                return -1;
            }
            printf("str:%s = %s\n",param,stemp);
            for(devnum=0;devnum<ndev;devnum++){
                if (lc_meta_put_str(&dconf[devnum], param, stemp))
                    fprintf(stderr, "LCRUN: failed to set device %d parameter %s to %s\n", devnum, param, stemp);
            }
            break;
        default:
            fprintf(stderr,"LCRUN: Got unsupported command line option: %c\n", go);
            return -1;
        }
    }


    // Verify that there are any configurations to execute
    if(ndev<=0){
        fprintf(stderr,"LCRUN did not detect any valid devices for data acquisition.\n");
        return -1;
    }
    
    // announce how many devices there are
    printf("Found %d device configurations\n",ndev);

    // Before we start the streaming process, setup all the devices
    for(devnum=0; devnum<ndev; devnum++){
        printf("Setting up device %d of %d...", devnum,ndev);
        fflush(stdout);
        // Open the connection
        if(lc_open(&dconf[devnum])){
            fprintf(stderr, "LCRUN: Failed while opening device %d of %d\n", devnum, ndev);
            halt();
            return -1;
        }
        // Upload the configuration
        if(lc_upload(&dconf[devnum])){
            fprintf(stderr, "LCRUN: Failed while configuring device %d of %d.\n", devnum, ndev);
            halt();
            return -1;
        }
        
        // Prepare the data file
        if(ndev == 1)
            sprintf(data_file, "%s.dat", data_file_base, devnum);
        else
            sprintf(data_file, "%s_%d.dat", data_file_base, devnum);
        dfile[devnum] = fopen(data_file,"wb");
        if(dfile[devnum] == NULL){
            fprintf(stderr, "LCRUN: Failed to open data file: %s\n", data_file);
            halt();
            return -1;
        }
        lc_datafile_init(&dconf[devnum], dfile[devnum]);
        printf("DONE.\n");
    }

    // Setup the keypress for exit conditions
    printf("Press \"Q\" to quit the process\nStreaming measurements...");
    fflush(stdout);
    lct_setup_keypress();

    // Start the stream!
    for(devnum=0;devnum<ndev;devnum++){
        // Start the data collection
        if(lc_stream_start(&dconf[devnum], -1)){
            fprintf(stderr, "LCRUN: Failed to start stream on device %d of %d.\n", devnum, ndev);
            halt();
            return -1;
        }
    }

    go = 1;
    lct_idle_init(&idle, 1000, 50);
    while(go){
        for(devnum=0; devnum<ndev; devnum++){
            if(lc_stream_service(&dconf[devnum])){
                fprintf(stderr, "LCRUN: failed while trying to service device %d of %d\n", devnum, ndev);
                lct_finish_keypress();
                halt();
                return -1;
            }
            // If data came in
            if(!lc_stream_isempty(&dconf[devnum])){
                fflush(stdout);
                lc_stream_read(&dconf[devnum], &data, &channels, &samples_per_read);
                lc_stream_downsample(&dconf[devnum], data, channels, &samples_per_read);
                lc_datafile_write(&dconf[devnum], dfile[devnum], data, channels, samples_per_read);
            }
        }
        // Test for exit conditions
        if(lct_is_keypress() && getchar() == 'Q')
            go = 0;
        fflush(stdout);
        lct_idle(&idle);
    }
    lct_finish_keypress();

    halt();
    printf("Exited successfully.\n");
    return 0;
}

