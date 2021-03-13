
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

#include "ldisplay.h"
#include "lconfig.h"
#include "lctools.h"
#include "lcmap.h"
#include <string.h>     // duh
#include <unistd.h>     // for system calls
#include <stdlib.h>     // for malloc and free
#include <stdio.h>      // duh
#include <math.h>
#include <time.h>       // for file stream time stamps

#define DEF_CONFIGFILE  "lcstat.conf"
#define MAXSTR          128
#define MAXDEV          16
#define UPDATE_SEC      0.5
// Column widths
#define FMT_CHANNEL     "%18s"
#define FMT_CHEAD       "\x1B[4m%18s\x1B[0m"
#define FMT_NUMBER      "%18.6f"
#define FMT_NHEAD       "\x1B[4m%18s\x1B[0m"
#define FMT_UNITS       "%8s"
#define FMT_UHEAD       "\x1B[4m%8s\x1B[0m"


#define destruct(){\
    for(ii=0;ii<ndev;ii++){lc_close(&dconf[ii]);}\
    lct_finish_keypress();\
    if(working){free(working); working=NULL;}\
    if(values){free(values); values=NULL;}\
}

/*...................
.   Global Options
...................*/



/*....................
. Help text
.....................*/
const char help_text[] = \
"lcstat [-dhmpr] [-c CONFIGFILE] [-n SAMPLES] [-u UPDATE_SEC]\n"\
"  LCSTAT is a utility that shows the status of the configured channels\n"\
"  in real time.  The intent is that it be used to aid with debugging and\n"\
"  setup of experiments from the command line.\n"
"\n"\
"  Measurement results are displayed in a table with a row for each\n"\
"  analog input and DIO extended feature channel configured and columns for\n"\
"  signal statistics, specified with switches at the command line.\n"\
"  Measurements are streamed for at least the number of samples specified\n"\
"  by the NSAMPLE configuration parameter or by the number specified by\n"\
"  the -n option.\n"\
"\n"\
"-c CONFIGFILE\n"\
"  Specifies the LCONFIG configuration file to be used to configure the\n"\
"  LabJack.  By default, LCSTAT will look for lcstat.conf\n"\
"\n"\
"-n SAMPLES\n"\
"  Specifies the minimum integer number of samples per channel to be \n"\
"  included in the statistics on each channel.  \n"\
"\n"\
"  For example, the following is true\n"\
"    $ lcburst -n 32   # collects 64 samples per channel\n"\
"    $ lcburst -n 64   # collects 64 samples per channel\n"\
"    $ lcburst -n 65   # collects 128 samples per channel\n"\
"    $ lcburst -n 190  # collects 192 samples per channel\n"\
"\n"\
"  Suffixes M (for mega or million) and K or k (for kilo or thousand)\n"\
"  are recognized.\n"\
"    $ lcburst -n 12k   # requests 12000 samples per channel\n"\
"\n"\
"  If both the test duration and the number of samples are specified,\n"\
"  which ever results in the longest test will be used.  If neither is\n"\
"  specified, then LCSTAT will collect one packet worth of data.\n"\
"\n"\
"-d\n"\
"  Display standard deviation of the signal in the results table.\n"\
"\n"\
"-m\n"\
"  Display the maximum and minimum of each signal in the results table.\n"\
"\n"\
"-p\n"\
"  Display peak-to-peak values in the results table.\n"\
"\n"\
"-r\n"\
"  Display rms values in the results table.\n"\
"\n"\
"-u UPDATE_SEC\n"\
"  Accepts a floating point indicating the approximate time in seconds between\n"\
"  display updates.\n"\
"\n"\
"GPLv3\n"\
"(c)2020 C.Martin\n";


/*....................
. Main
.....................*/
int main(int argc, char *argv[]){
    // DCONF parameters
    int     ndev;           // number of devices found in configuration
    // Temporary variables
    int     ii, jj;         // counters for loops
    unsigned int row, col;  // indices for the terminal display
    double  ftemp;          // Temporary float
    int     itemp;          // Temporary integer
    char    stemp[MAXSTR],  // Temporary string
            param[MAXSTR];  // Parameter
    char    optchar;
    // Configuration results
    char    config_file[MAXSTR] = DEF_CONFIGFILE;
    unsigned int     samples = 0;
    // State struct used to define the operation of the state machine
    // that prints to the screen
    struct {
        unsigned int peak:1;
        unsigned int rms:1;
        unsigned int std:1;
        unsigned int maxmin:1;
        unsigned int run:1;
        unsigned int redraw:1;
    } state;
    time_t now, then;
    // Command-line options
    double update_sec = UPDATE_SEC;

    // Finally, the essentials
    lc_devconf_t dconf[MAXDEV];     // device configuration array
    lct_stat_t  * values = NULL,    // Live arrays of channel statistics
                * working = NULL;   // working arrays of channel statistics
    lct_idle_t  idle;
    

    // Initialize the state
    state.peak = 0;
    state.rms = 0;
    state.std = 0;
    state.maxmin = 0;
    state.run = 1;
    
    
    
    // Parse the command-line options
    // use an outer foor loop as a catch-all safety
    for(ii=0; ii<argc; ii++){
        switch(getopt(argc, argv, "hprdmc:n:u:")){
        // Help text
        case 'h':
            printf(help_text);
            return 0;
        case 'p':
            state.peak = 1;
            break;
        case 'm':
            state.maxmin = 1;
            break;
        case 'd':
            state.std = 1;
            break;
        case 'r':
            state.rms = 1;
            break;
        // Config file
        case 'c':
            strcpy(config_file, optarg);
            break;
        // Sample count
        case 'n':
            optchar = 0;
            if(sscanf(optarg, "%d%c", &samples, &optchar) < 1){
                fprintf(stderr,
                        "The sample count was not a number: %s\n", optarg);
                return -1;
            }
            switch(optchar){
                case 'M':
                    samples *= 1000;
                case 'k':
                case 'K':
                    samples *= 1000;
                case 0:
                    break;
                default:
                    fprintf(stderr,
                            "Unexpected sample count unit: %c\n", optchar);
                    return -1;
            }
            break;
        case 'u':
            if(sscanf(optarg, "%lf", &update_sec) != 1){
                printf("LCSTAT: -u expects a number, but got: %s\n", optarg);
                return -1;
            }
        break;
        case -1:    // What if we're out of switch options?
            // Force the loop to exit.
            ii = argc;
            break;
        // Unrecognized characters
        case '?':
        default:
            fprintf(stderr, "Unexpected option %s\n", argv[optind]);
            return -1;
        }
    }

    // Load the configuration
    // This will also enforce that no more than MAXDEV devices are configured
    printf("Loading configuration file...\n");
    if(lc_load_config(dconf, MAXDEV, config_file)){
        fprintf(stderr, "LCSTAT failed while loading the configuration file \"%s\"\n", config_file);
        return -1;
    }else
        printf("DONE\n");

    // Detect the number of configured device connections
    ndev = lc_ndev(dconf, MAXDEV);
    if(!ndev){
        fprintf(stderr,"LCSTAT did not detect any valid devices for data acquisition.\n");
        return -1;
    }
    
    // Declare memory for the working and active channel statistics
    values = malloc(ndev * LCONF_MAX_NAICH * sizeof(lct_stat_t));
    working = malloc(ndev * LCONF_MAX_NAICH * sizeof(lct_stat_t));

    // Initialize the terminal
    lct_clear_terminal();
    lct_setup_keypress();

    // Open the device connections and upload the configuration
    for(ii=0; ii<ndev; ii++){
        // Override the nsample parameter?
        if(samples) 
            dconf[ii].nsample = samples;

        // Open the device connection
        if(lc_open(&dconf[ii])){
            fprintf(stderr, "LCSTAT failed to open the device %d in configuration file %s\n", ii, config_file);
            destruct();
            return -1;
        // Upload the configurations
        }else if(lc_upload_config(&dconf[ii])){
            fprintf(stderr, "LCSTAT failed to configure device %d in configuration file %s\n", ii, config_file);
            destruct();
            return -1;
        // Start the data streams
        }else if(lc_stream_start(&dconf[ii], -1)){
            fprintf(stderr, "LCSTAT failed to start data collection on device %d in configuration file %s\n", ii, config_file);
            destruct();
            return -1;
        }
        // initialize the statistics
        lct_stat_init(&values[ii*LCONF_MAX_NAICH], LCONF_MAX_NAICH);
        lct_stat_init(&working[ii*LCONF_MAX_NAICH], LCONF_MAX_NAICH);
    }
    
    then = time(NULL);
    lct_idle_init(&idle, 100, 5);
    while(state.run){
        now = time(NULL);
        // When it's time to redraw the screen
        if(difftime(now,then) > update_sec){
            then = now;
            // REFRESH CODE
            
            // EXTENDED FEATURE DIO CHANNELS
            // If there are any extended feature channels, update them before
            // beginning the redraw
            if(dconf[ii].nefch)
                lc_update_ef(&dconf[ii]);
            
            // Start fresh
            lct_clear_terminal();
            // Loop through the devices
            for(ii=0; ii<ndev; ii++){
                // Print the device header
                printf("Device %d: \x1B[1m%s\x1B[0m (%s)\n", 
                        ii, dconf[ii].name, 
                        lcm_get_message(lcm_connection, dconf[ii].connection_act));
                // Print the header
                // Start by printing a header
                printf(FMT_CHEAD, "Channel");
                printf(FMT_UHEAD, "Units");
                printf(FMT_NHEAD, "Mean");
                if(state.rms)
                    printf(FMT_NHEAD, "RMS");
                if(state.std)
                    printf(FMT_NHEAD, "Std.Dev.");
                if(state.peak)
                    printf(FMT_NHEAD, "Pk-Pk");
                if(state.maxmin){
                    printf(FMT_NHEAD, "Max.");
                    printf(FMT_NHEAD, "Min.");
                }
                printf("\n");

                // Loop through the channels
                for(jj=0; jj<dconf[ii].naich; jj++){
                    // CHANNEL LABEL
                    // If there is a channel label, print that
                    if(dconf[ii].aich[jj].label[0])
                        printf(FMT_CHANNEL, dconf[ii].aich[jj].label);
                    // If the channel is not differential
                    else if(dconf[ii].aich[jj].nchannel == LCONF_SE_NCH){
                        sprintf(stemp, "+AI%02d -GND", dconf[ii].aich[jj].channel);
                        printf(FMT_CHANNEL, stemp);
                    }else{
                        printf(stemp, "+AI%02d -AI%02d", dconf[ii].aich[jj].channel, dconf[ii].aich[jj].nchannel);
                        printf(FMT_CHANNEL, stemp);
                    }
                    
                    
                    // CHANNEL UNITS
                    // If a units string is defined, use that
                    if(dconf[ii].aich[jj].calunits[0])
                        printf(FMT_UNITS, dconf[ii].aich[jj].calunits);
                    else 
                        printf(FMT_UNITS, "V");
                    
                    // CHANNEL MEAN
                    printf(FMT_NUMBER, values[ii*LCONF_MAX_NAICH+jj].mean);
                    
                    if(state.rms)
                        printf(FMT_NUMBER, sqrt(values[ii*LCONF_MAX_NAICH+jj].mean*values[ii*LCONF_MAX_NAICH+jj].mean + values[ii*LCONF_MAX_NAICH+jj].var));   
                    if(state.std)
                        printf(FMT_NUMBER, sqrt(values[ii*LCONF_MAX_NAICH+jj].var));
                    if(state.peak)
                        printf(FMT_NUMBER, values[ii*LCONF_MAX_NAICH+jj].max - values[ii*LCONF_MAX_NAICH+jj].min);
                    if(state.maxmin){
                        printf(FMT_NUMBER, values[ii*LCONF_MAX_NAICH+jj].max);
                        printf(FMT_NUMBER, values[ii*LCONF_MAX_NAICH+jj].min);   
                    }
                    printf("\n");
                }
                
                // Draw the EF channels
                // Then display them
                for(jj=0; jj<dconf[ii].nefch; jj++){
                    // CHANNEL LABEL
                    // If the labe is defined, print it verbatim
                    if(dconf[ii].efch[jj].label[0])
                        printf(FMT_CHANNEL, dconf[ii].efch[jj].label);
                    else{
                        sprintf(stemp, "DIO%d", dconf[ii].efch[jj].channel);
                        printf(FMT_CHANNEL, stemp);
                    }
                    
                    switch(dconf[ii].efch[jj].signal){
                    case LC_EF_PWM:
                        printf(FMT_UNITS, "PWM");
                        printf(FMT_NUMBER, dconf[ii].efch[jj].duty);
                        break;
                    case LC_EF_COUNT:
                        printf(FMT_UNITS, "Count");
                        printf(FMT_NUMBER, (double) dconf[ii].efch[jj].counts);
                        break;
                    case LC_EF_FREQUENCY:
                        printf(FMT_UNITS, "Freq(kHz)");
                        printf(FMT_NUMBER, 1000./dconf[ii].efch[jj].time);
                        break;
                    case LC_EF_PHASE:
                        printf(FMT_UNITS, "Phase(deg)");
                        printf(FMT_NUMBER, dconf[ii].efch[jj].phase);
                        break;
                    case LC_EF_QUADRATURE:
                        printf(FMT_UNITS, "Quad.");
                        printf(FMT_NUMBER, (double) dconf[ii].efch[jj].counts);
                        break;
                    default:
                        printf(FMT_UNITS, "Uns.");
                        break;
                    }
                    printf("\n");
                }
            }
            printf("\nPress \"Q\" to exit.\n");
        }
        
        // Service the data connections
        for(ii=0;ii<ndev;ii++){
            lc_stream_service(&dconf[ii]);
            lct_stream_stat(&dconf[ii], &working[ii*LCONF_MAX_NAICH], 0);
            // If the working array has accumulated enough samples
            if(working[ii*LCONF_MAX_NAICH].n >= dconf[ii].nsample){
                // Copy the result and clear the worker
                for(jj=0; jj<dconf[ii].naich; jj++) 
                    values[ii*LCONF_MAX_NAICH + jj] = working[ii*LCONF_MAX_NAICH + jj];
                lct_stat_init(&working[ii*LCONF_MAX_NAICH], dconf[ii].naich);
            }
        }
        
        // Time for a little downtime
        lct_idle(&idle);
        
        // Check for the escape keypress
        if(lct_is_keypress() && getchar()=='Q')
            state.run = 0;
            
    }
    destruct();
    return 0;
}
