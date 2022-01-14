
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

#include "lconfig.h"
#include <string.h>     // duh
#include <unistd.h>     // for system calls
#include <stdlib.h>     // for malloc and free
#include <stdio.h>      // duh
#include <time.h>       // for file stream time stamps

#define DEF_CONFIGFILE  "lcburst.conf"
#define DEF_DATAFILE    "lcburst.dat"
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
"lcburst [-h] [-c CONFIGFILE] [-n SAMPLES] [-t DURATION] [-d DATAFILE]\n"\
"     [-f|i|s param=value]\n"\
"  Runs a single high-speed burst data colleciton operation. Data are\n"\
"  streamed directly into ram and then saved to a file after collection\n"\
"  is complete.  This allows higher data rates than streaming to the hard\n"\
"  drive.\n"\
"\n"\
"-c CONFIGFILE\n"\
"  Specifies the LCONFIG configuration file to be used to configure the\n"\
"  LabJack.  By default, LCBURST will look for lcburst.conf\n"\
"\n"\
"-d DATAFILE\n"\
"  Specifies the data file to output.  This overrides the default, which is\n"\
"  constructed from the current date and time: \"YYYYMMDDHHmmSS_lcburst.dat\"\n"\
"\n"\
"-f param=value\n"\
"-i param=value\n"\
"-s param=value\n"\
"  These flags signal the creation of a meta parameter at the command\n"\
"  line.  f,i, and s signal the creation of a float, integer, or string\n"\
"  meta parameter that will be written to the data file header.\n"\
"     $ LCBURST -f height=5.25 -i temperature=22 -s day=Monday\n"\
"\n"\
"-n SAMPLES\n"\
"  Specifies the integer number of samples per channel desired.  This is\n"\
"  treated as a minimum, since LCBURST will collect samples in packets\n"\
"  of LCONF_SAMPLES_PER_READ (64) per channel.  LCONFIG will collect the\n"\
"  number of packets required to collect at least this many samples.\n"\
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
"  specified, then LCBURST will collect one packet worth of data.\n"\
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
"(c)2017-2021 C.Martin\n";


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
        switch(getopt(argc, argv, "hc:n:t:d:f:i:s:")){
        // Help text
        case 'h':
            printf(help_text);
            return 0;
        // Config file
        case 'c':
            strcpy(config_file, optarg);
            break;
        // Duration
        case 't':
            optchar = 0;
            if(sscanf(optarg, "%d%c", &duration, &optchar) < 1){
                fprintf(stderr,
                        "The duration was not a number: %s\n", optarg);
                return -1;
            }
            switch(optchar){
                case 'H':
                    duration *= 60;
                case 'M':
                    duration *= 60;
                case 's':
                case 0:
                    duration *= 1000;
                case 'm':
                    break;
                default:
                    fprintf(stderr,
                            "Unexpected sample duration unit %c\n", optchar);
                    return -1;
            }
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
                    duration *= 1000;
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
        // Data file
        case 'd':
            strcpy(data_file, optarg);
            break;
        // Process meta parameters later
        case 'f':
        case 'i':
        case 's':
            break;
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

    // Process the staged command-line meta parameters
    // use an outer for loop as a catch-all safety
    optind = 1;
    for(count=0; count<argc; count++){
        switch(getopt(argc, argv, "hc:n:t:d:f:i:s:")){
        // Process meta parameters later
        case 'f':
            if(sscanf(optarg,"%[^=]=%lf",(char*) param, &ftemp) != 2){
                fprintf(stderr, "LCBURST expected param=float format, but found %s\n", optarg);
                return -1;
            }
            printf("flt:%s = %lf\n",param,ftemp);
            if (lc_put_meta_flt(&dconf, param, ftemp))
                fprintf(stderr, "LCBURST failed to set parameter %s to %lf\n", param, ftemp);            
            break;
        case 'i':
            if(sscanf(optarg,"%[^=]=%d",(char*) param, &itemp) != 2){
                fprintf(stderr, "LCBURST expected param=integer format, but found %s\n", optarg);
                return -1;
            }
            printf("int:%s = %d\n",param,itemp);
            if (lc_put_meta_int(&dconf, param, itemp))
                fprintf(stderr, "LCBURST failed to set parameter %s to %d\n", param, itemp);
            break;
        case 's':
            if(sscanf(optarg,"%[^=]=%s",(char*) param, (char*) stemp) != 2){
                fprintf(stderr, "LCBURST expected param=string format, but found %s\n", optarg);
                return -1;
            }
            printf("str:%s = %s\n",param,stemp);
            if (lc_put_meta_str(&dconf, param, stemp))
                fprintf(stderr, "LCBURST failed to set parameter %s to %s\n", param, stemp);
            break;
        // Escape condition
        case -1:
            count = argc;
            break;
        // We've already done error handling
        default:
            break;
        }
    }
    // If the data file was not configured, use the timestamp to create a name
    if(data_file[0] == '\0'){
        time(&start);
        strftime(data_file, MAXSTR, "%Y%m%d%H%M%S_lcburst.dat", localtime(&start));
    }

    // Calculate the number of samples to collect
    // If neither the sample nor duration option is configured, leave 
    // configuration alone
    if(samples > 0 || duration > 0){
        // Calculate the number of samples to collect
        // Use which ever is larger: samples or duration
        nsample = (duration * dconf.samplehz) / 1000;  // duration is in ms
        nsample = nsample > samples ? nsample : samples;
        dconf.nsample = nsample;
    }

    // Print some information
    printf("  Stream channels : %d\n", nich);
    printf("      Sample rate : %.1fHz\n", dconf.samplehz);
    printf(" Samples per chan : %d (%d requested)\n", dconf.nsample, samples);
    ftemp = dconf.nsample/dconf.samplehz;
    if(ftemp>60){
        ftemp /= 60;
        if(ftemp>60){
            ftemp /= 60;
            printf("    Test duration : %fhr (%d requested)\n", (float)(ftemp), duration/3600000);
        }else{
            printf("    Test duration : %fmin (%d requested)\n", (float)(ftemp), duration/60000);
        }
    }else if(ftemp<1)
        printf("    Test duration : %fms (%d requested)\n", (float)(ftemp*1000), duration);
    else
        printf("    Test duration : %fs (%d requested)\n", (float)(ftemp), duration/1000);


    printf("Setting up measurement...");
    fflush(stdout);
    if(lc_open(&dconf)){
        fprintf(stderr, "LCBURST failed to open the device.\n");
        return -1;
    }
    if(lc_upload_config(&dconf)){
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
    fflush(stdout);
    if(dconf.trigchannel >= 0)
        printf("\nWaiting for trigger\n");

    while(!lc_stream_iscomplete(&dconf)){
        itemp = dconf.trigstate;
        if(lc_stream_service(&dconf)){
            fprintf(stderr, "\nLCBURST failed while servicing the T7 connection!\n");
            lc_stream_stop(&dconf);
            lc_close(&dconf);
            return -1;            
        }
        
        if(itemp != LC_TRIG_ACTIVE && dconf.trigstate == LC_TRIG_ACTIVE){
            printf("Streaming data.\n");
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
    dfile = fopen(data_file,"wb");
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
