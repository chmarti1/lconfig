
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
    along with Foobar.  If not, see <https://www.gnu.org/licenses/>.

    Authored by C.Martin crm28@psu.edu
*/

#include "ldisplay.h"
#include "lconfig.h"
#include <string.h>     // duh
#include <unistd.h>     // for system calls
#include <stdlib.h>     // for malloc and free
#include <stdio.h>      // duh
#include <time.h>       // for file stream time stamps

#define DEF_CONFIGFILE  "lcstat.conf"
#define DEF_DATAFILE    "lcstat.dat"
#define DEF_SAMPLES     "-1"
#define DEF_DURATION    "-1"
#define MAXSTR          128


/*...................
.   Global Options
...................*/


/*....................
. Prototypes
.....................*/
// Parse the command-line options strings
// Modifies the globals appropriately
int parse_options(int argc, char*argv[]);

/*....................
. Help text
.....................*/
const char help_text[] = \
"lcstat [-hps] [-c CONFIGFILE] [-n SAMPLES]\n"\
"  LCSTAT is a utility that shows the status of the configured channels\n"\
"  in real time.  The intent is that it be used to aid with debugging and\n"\
"  setup of experiments from the command line.\n"
"\n"\
"  Measurement results are displayed in a table with a row for each\n"\
"  channel configured and a column for various statistics on the signal.\n"\
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
"-p\n"\
"  Display peak-to-peak values in the results table.\n"\
"\n"\
"-s\n"\
"  Display standard deviation of the signal in the results table.\n"\
"\n"\
"GPLv3\n"\
"(c)2017-2019 C.Martin\n";


/*....................
. Main
.....................*/
int main(int argc, char *argv[]){
    // DCONF parameters
    int     nsample,        // number of samples to collect
            nich;           // number of channels in the operation
    // Temporary variables
    int     count;          // a counter for loops
    double  ftemp;          // Temporary float
    int     itemp;          // Temporary integer
    char    stemp[MAXSTR],  // Temporary string
            param[MAXSTR];  // Parameter
    char    optchar;
    // Configuration results
    char    config_file[MAXSTR] = DEF_CONFIGFILE,
            data_file[MAXSTR] = "\0";
    int     samples = 0, 
            duration = 0;
    time_t  start;

    // Finally, the essentials; a data file and the device configuration
    FILE *dfile;
    lc_devconf_t dconf;

    // Parse the command-line options
    // use an outer foor loop as a catch-all safety
    for(count=0; count<argc; count++){
        switch(getopt(argc, argv, "hpsc:n:")){
        // Help text
        case 'h':
            printf(help_text);
            return 0;
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
        case '?':
            fprintf(stderr, "Unexpected option %s\n", argv[optind]);
            return -1;
        case -1:    // Deliberately combine -1 and default
        default:
            fprintf(stderr, "Unhandled command line argument %s\n", argv[optind]);
        }
    }

    // Load the configuration
    printf("Loading configuration file...");
    fflush(stdout);
    if(lc_load_config(&dconf, 1, config_file)){
        fprintf(stderr, "LCBURST failed while loading the configuration file \"%s\"\n", config_file);
        return -1;
    }else
        printf("DONE\n");

    // Detect the number of configured device connections
    if(lc_ndev(&dconf,1)<=0){
        fprintf(stderr,"LCBURST did not detect any valid devices for data acquisition.\n");
        return -1;
    }
    // Detect the number of input columns
    nich = lc_nistream(&dconf);

    // Check to see if nsample was specified
    if(samples)
        dconf.nsample = samples;
    nsample = dconf.nsample;


    printf("Setting up measurement...");
    fflush(stdout);
    if(lc_open(&dconf)){
        fprintf(stderr, "LCBURST failed to open the device.\n");
        return -1;
    }
    if(lc_open(&dconf)){
        fprintf(stderr, "LCBURST failed while configuring the device.\n");
        lc_close(&dconf);
        return -1;
    }
    printf("DONE\n");

    // Start the data stream
    if(lc_stream_start(&dconf, -1)){
        fprintf(stderr, "\nLCBURST failed to start data collection.\n");
        lc_close(&dconf);
        return -1;
    }

    // Stream data
    printf("Streaming data\n");
    if(dconf.trigstate == LC_TRIG_PRE)
        printf("Waiting for trigger\n");

    while(!lc_stream_iscomplete(&dconf)){
        if(lc_stream_service(&dconf)){
            fprintf(stderr, "\nLCBURST failed while servicing the T7 connection!\n");
            lc_stream_stop(&dconf);
            lc_close(&dconf);
            return -1;            
        }

        if(dconf.trigstate == LC_TRIG_IDLE || dconf.trigstate == LC_TRIG_ACTIVE){
            printf(".");
            fflush(stdout);
        }
    }
    // Halt data collection
    if(lc_stream_stop(&dconf)){
        fprintf(stderr, "\nLCBURST failed to halt preliminary data collection!\n");
        lc_close(&dconf);
        return -1;
    }
    printf("DONE\n");

    // Open the output file
    printf("Writing the data file");
    fflush(stdout);
    dfile = fopen(data_file,"w");
    if(dfile == NULL){
        printf("FAILED\n");
        fprintf(stderr, "LCBURST failed to open the data file \"%s\"\n", data_file);
        return -1;
    }

    // Write the configuration header
    lc_datafile_init(&dconf,dfile);
    // Write the samples
    while(!lc_stream_isempty(&dconf)){
        lc_datafile_write(&dconf,dfile);
        printf(".");
        fflush(stdout);
    }
    fclose(dfile);
    lc_close(&dconf);
    printf("DONE\n");

    printf("Exited successfully.\n");
    return 0;
}
