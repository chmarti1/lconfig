
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
#define COLWIDTH        20
#define HEADERROW       2
#define FMT_NUMBER      "%20.6f"
#define MAX_CHANNELS    (LCONF_MAX_AICH + 1)



/*....................
. Prototypes
.....................*/
struct args_t {
    int:1 maxmin;
    int:1 std;
    int:1 pkpk;
};

/*....................
. Help text
.....................*/
const char help_text[] = \
"lcstat [-hmps] [-c CONFIGFILE] [-n SAMPLES]\n"\
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
"  If a trigger is configured, it will be ignored.  Instead, the acquisition\n"\
"  starts as normal and streams directly to the display.\n"\
"\n"\
"-c CONFIGFILE\n"\
"  Specifies the LCONFIG configuration file to be used to configure the\n"\
"  LabJack.  By default, LCSTAT will look for lcstat.conf\n"\
"\n"\
"-m\n"\
"  Display the maximum and minimum values in the results table.\n"\
"\n"\
"-n SAMPLES\n"\
"  Specifies the minimum integer number of samples per channel to be \n"\
"  included in the statistics on each channel.  Since samples are read in\n"\
"  bursts of LCONF_SAMPLES_PER_READ (64) samples per channel, the actual \n"\
"  samples read will be rounded up to the nearest multiple of 64.\n"\
"\n"\
"-p\n"\
"  Display peak-to-peak values in the results table.\n"\
"\n"\
"-s\n"\
"  Display standard deviation of the signal in the results table.\n"\
"\n"\
"-t DURATION\n"\
"  Specifies the test duration with an integer.  By default, DURATION\n"\
"  should be in seconds.\n"\
"    $ lcburst -t 10   # configures a 10 second test\n"\
"\n"\
"  Short or long test durations can be specified by a unit suffix: m for\n"\
"  milliseconds, M for minutes, and H for hours.  s for seconds is also\n"\
"  recognized.\n"\
"    $ lcburst -t 500m  # configures a 0.5 second test\n"\
"    $ lcburst -t 1M    # configures a 60 second test\n"\
"    $ lcburst -t 1H    # configures a 3600 second test\n"\
"\n"\
"  If both the test duration and the number of samples are specified,\n"\
"  which ever results in the longest test will be used.  If neither is\n"\
"  specified, then LCBURST will collect one packet worth of data.\n"\
"\n"\
"GPLv3\n"\
"(c)2017-2019 C.Martin\n";


void print_ouptut(lct_stat_t results[], struct args_t *mode);


/*....................
. Main
.....................*/
int main(int argc, char *argv[]){
    // DCONF parameters
    int     nich;           // number of channels in the operation
    unsigned int channels, samples_per_read;
    // Temporary variables
    double  ftemp;          // Temporary float
    int     itemp;          // Temporary integer
    char    stemp[MAXSTR],  // Temporary string
            param[MAXSTR];  // Parameter
    char    optchar;
    // Configuration results
    char    config_file[MAXSTR] = DEF_CONFIGFILE;
    int     nsample = 0;
    struct args_t mode;
    char    go_b = 1;
    // Finally, the essentials; device configuration
    lc_devconf_t dconf;
    lct_stat_t working[MAX_CHANNELS], total[MAX_CHANNELS];

    // Parse the command-line options
    // use an outer foor loop as a catch-all safety
    for(count=0; count<argc; count++){
        switch(getopt(argc, argv, "hmpsc:n:")){
        // Help text
        case 'h':
            printf(help_text);
            return 0;
        // Config file
        case 'c':
            strcpy(config_file, optarg);
            break;
        case 'm':
            maxmin_b = 1;
            break;
        case 'p':
            peak_b = 1;
            break;
        case 's':
            std_b = 1;
            break;
        // Sample count
        case 'n':
            if(sscanf(optarg, "%d", &samples) < 1){
                fprintf(stderr,
                        "The sample count was not a number: %s\n", optarg);
                return -1;
            }
            break;
        // Data file
        case '?':
            fprintf(stderr, "Unexpected option %s\n", argv[optind]);
            return -1;
        case -1:    // Deliberately combine -1 and default
        default:
            count = argc;
            break;
        }
    }

    // Load the configuration
    printf("Loading configuration file...");
    fflush(stdout);
    if(lc_load_config(&dconf, 1, config_file)){
        fprintf(stderr, "LCSTAT failed while loading the configuration file \"%s\"\n", config_file);
        return -1;
    }else
        printf("DONE\n");

    // Detect the number of configured device connections
    if(lc_ndev(&dconf,1)<=0){
        fprintf(stderr,"LCSTAT did not detect any valid devices for data acquisition.\n");
        return -1;
    }
    // Detect the number of input columns
    nich = lc_nistream(&dconf);
    // Determine whether the sample count was set at the command line
    if(nsample > 0) 
        dconf.nsample = nsample;
    else
        nsample = dconf.nsample;

    printf("Setting up measurement...");
    fflush(stdout);
    if(lc_open(&dconf)){
        fprintf(stderr, "LCSTAT failed to open the device.\n");
        return -1;
    }
    if(lc_open(&dconf)){
        fprintf(stderr, "LCSTAT failed while configuring the device.\n");
        lc_close(&dconf);
        return -1;
    }
    printf("DONE\n");

    // Setup the keypress interface
    lct_setup_keypress();

    // Start the data stream
    if(lc_stream_start(&dconf, -1)){
        fprintf(stderr, "\nLCSTAT failed to start data collection.\n");
        lc_close(&dconf);
        return -1;
    }

    // Initialize the total struct array
    lct_stream_stat(&dconf, NULL, 0, total, MAX_CHANNELS);

    fprintf(stdout, "Waiting for data.\n");

    // Go
    go_b = 1;
    while(go_b){
        lc_stream_service(&dconf);
        lc_stream_read(&dconf, &data, &channels, &samples_per_read);
        // Process the stats
        if(data){
            lct_stream_stat(&dconf, data, samples_per_read*channels,\
                    working, MAX_CHANNELS);
            lct_stat_join(total, working, MAX_CHANNELS);
        }
        
        // Detect a keypress
        if(lct_is_keypress() && getchar() == 'Q')
            go_b = 0;
        // detect whether the sample is complete
        else if(total[0].N >= nsample){
            // Print the results to the screen
            print_output(total, &mode);
            // Reset the total values
            lct_stream_stat(&dconf, NULL, 0, total, MAX_CHANNELS);
        }
    }
    // Halt data collection
    if(lc_stream_stop(&dconf)){
        fprintf(stderr, "\nLCSTAT failed to halt data collection!?\n");
        lc_close(&dconf);
        return -1;
    }
    lc_close(&dconf);

    return 0;
}
