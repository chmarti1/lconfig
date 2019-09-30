
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

#include "lctools.h"
#include "lconfig.h"
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>   // For forming file names from timestamps

#define CONFIG_FILE "lcrun.conf"
#define MAXLOOP     "-1"
#define NBUFFER     65535
#define NDEV        3
#define MAXSTR      128


/*...................
.   Global Options
...................*/
char    pre_file[MAXSTR] = "",   // The pre-stream data file name
        post_file[MAXSTR] = "",  // The post-stream data file
        data_file[MAXSTR] = "",  // The data file
        config_file[MAXSTR] = CONFIG_FILE,  //  The configuration file
        maxloop[MAXSTR] = MAXLOOP,  // maximum number of reads
        metastage[LCONF_MAX_META][MAXSTR+1];

int     maxloop_i, metacount;

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
"lcrun [-h] [-r PREFILE] [-p POSTFILE] [-d DATAFILE] [-c CONFIGFILE] \n"\
"     [-f|i|s param=value]\n"\
"  Runs a data acquisition job until the user exists with a keystroke.\n"\
"\n"\
"PRE and POST data collection\n"\
"  When LCRUN first starts, it loads the selected configuration file\n"\
"  (see -c option).  If only one device is found, it will be used for\n"\
"  the continuous DAQ job.  However, if multiple device configurations\n"\
"  are found, they will be used to take bursts of data before (pre-)\n"\
"  after (post-) the continuous DAQ job.\n"\
"\n"\
"  If two devices are found, the first device will be used for a single\n"\
"  burst of data acquisition prior to starting the continuous data \n"\
"  collection with the second device.  The NSAMPLE parameter is useful\n"\
"  for setting the size of these bursts.\n"\
"\n"\
"  If three devices are found, the first and second devices will still\n"\
"  be used for the pre-continous and continuous data sets, and the third\n"\
"  will be used for a post-continuous data set.\n"\
"\n"\
"  In all cases, each data set will be written to its own file so that\n"\
"  LCRUN creates one to three files each time it is run.\n"\
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
"  \"YYYYMMDDHHmmSS_lcrun.dat\"\n"\
"     $ lcrun -d mydatafile.dat\n"\
"\n"\
"-r PREFILE\n"\
"  This option overrides the default pre-continuous data file name\n"\
"  \"YYYYMMDDHHmmSS_lcrun_pre.dat\"\n"\
"     $ lcrun -r myprefile.dat\n"\
"\n"\
"-p POSTFILE\n"\
"  This option overrides the default post-continuous data file name\n"\
"  \"YYYYMMDDHHmmSS_lcrun_post.dat\"\n"\
"     $ lcrun -p mypostfile.dat\n"\
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
"(c)2017-2019 C.Martin\n";


/*....................
. Main
.....................*/
int main(int argc, char *argv[]){
    int ndev;   // actual number of devices
                // NDEV is the maximum 
    int stream_dev = 0; // which device will be used for streaming?
    char go;    // Flag for whether to continue the stream loop
    char param[MAXSTR];
    double ftemp;
    int itemp;
    char stemp[MAXSTR];
    unsigned int count; // count the number of loops for safe exit
    lc_devconf_t dconf[NDEV];
    time_t start;
    FILE* dfile;

    // Parse the command-line options
    if(parse_options(argc, argv))
        return -1;

    // Load the configuration
    printf("Loading configuration file...");
    if(lc_load_config(dconf, NDEV, config_file)){
        printf("FAILED\n");
        fprintf(stderr, "LCRUN failed while loading the configuration file \"%s\"\n", config_file);
        return -1;
    }else
        printf("DONE\n");

    // Detect the number of configured device connections
    ndev = lc_ndev(dconf, NDEV);

    // Build file names from a timestamp
    time(&start);
    strftime(stemp, MAXSTR, "%Y%m%d%H%M%S", localtime(&start));
    if(data_file[0] == '\0')
        sprintf(data_file, "%s_lcrun.dat", stemp);
    if(pre_file[0] == '\0')
        sprintf(pre_file, "%s_lcrun_pre.dat", stemp);
    if(post_file[0] == '\0')
        sprintf(post_file, "%s_lcrun_post.dat", stemp);

    // Process the staged command-line meta parameters
    for(metacount--; metacount>=0; metacount--){
        if(metastage[metacount][0]=='f'){
            if(sscanf(&metastage[metacount][1],"%[^=]=%lf",(char*)&param, &ftemp) != 2){
                fprintf(stderr, "LCRUN expected param=float format, but found %s\n", &metastage[metacount][1]);
                return -1;
            }
            printf("flt:%s = %lf\n",param,ftemp);
            if (lc_put_meta_flt(&dconf[0], param, ftemp) || 
                lc_put_meta_flt(&dconf[1], param, ftemp) ||
                lc_put_meta_flt(&dconf[2], param, ftemp))
                fprintf(stderr, "LCRUN failed to set parameter %s to %lf\n", param, ftemp);
        }else if(metastage[metacount][0]=='i'){
            if(sscanf(&metastage[metacount][1],"%[^=]=%d",(char*)&param, &itemp) != 2){
                fprintf(stderr, "LCRUN expected param=integer format, but found %s\n", &metastage[metacount][1]);
                return -1;
            }
            printf("int:%s = %d\n",param,itemp);
            if (lc_put_meta_int(&dconf[0], param, itemp) || 
                lc_put_meta_int(&dconf[1], param, itemp) ||
                lc_put_meta_int(&dconf[2], param, itemp))
                fprintf(stderr, "LCRUN failed to set parameter %s to %d\n", param, itemp);
        }else if(metastage[metacount][0]=='s'){
            if(sscanf(&metastage[metacount][1],"%[^=]=%s",(char*)&param, (char*)&stemp) != 2){
                fprintf(stderr, "LCRUN expected param=string format, but found %s\n", &metastage[metacount][1]);
                return -1;
            }
            printf("flt:%s = %s\n",param,stemp);
            if (lc_put_meta_str(&dconf[0], param, stemp) || 
                lc_put_meta_str(&dconf[1], param, stemp) ||
                lc_put_meta_str(&dconf[2], param, stemp))
                fprintf(stderr, "LCRUN failed to set parameter %s to %s\n", param, stemp);
        }else{
            fprintf(stderr, "LCRUN unexpected error parsing staged command-line meta parameters!\n");
            return -1;
        }
    }

    // Verify that there are any configurations to execute
    if(ndev<=0){
        fprintf(stderr,"LCRUN did not detect any valid devices for data acquisition.\n");
        return -1;
    }else{
        // Unless additional devices are found, use the first for streaming
        stream_dev = 0;
        printf("Found %d device configurations\n",ndev);
    }

    // Perform the preliminary data collection process
    if(ndev>1){
        // set the streaming device to the second in the array
        stream_dev = 1;
        printf("Performing preliminary static measurements...");
        if(lc_open(&dconf[0])){
            printf("FAILED\n");
            fprintf(stderr, "LCRUN failed to open the preliminary data collection device.\n");
            return -1;
        }
        if(lc_upload_config(&dconf[0])){
            printf("FAILED\n");
            fprintf(stderr, "LCRUN failed while configuring preliminary data collection.\n");
            lc_close(&dconf[0]);
            return -1;
        }
        if(lc_stream_start(&dconf[0], -1)){
            printf("FAILED\n");
            fprintf(stderr, "LCRUN failed to start preliminary data collection.\n");
            lc_close(&dconf[0]);
            return -1;
        }
        // STREAM!
        while(!lc_stream_iscomplete(&dconf[0])){
            if(lc_stream_service(&dconf[0])){
                fprintf(stderr, "\nLCRUN failed while servicing the T7 connection!\n");
                lc_stream_stop(&dconf[0]);
                lc_close(&dconf[0]);
                return -1;
            }
        }
        if(lc_stream_stop(&dconf[0])){
            printf("FAILED\n");
            fprintf(stderr, "LCRUN failed to halt preliminary data collection!\n");
            lc_close(&dconf[0]);
            return -1;
        }

        dfile = fopen(pre_file,"w");
        if(dfile == NULL){
            fprintf(stderr, "LCRUN failed to open pre-data file: %s\n", pre_file);
            lc_close(&dconf[0]);
            return -1;
        }
        lc_datafile_init(&dconf[0],dfile);
        while(!lc_stream_isempty(&dconf[0]))
            lc_datafile_write(&dconf[0],dfile);
        fclose(dfile);
        dfile = NULL;
        lc_close(&dconf[0]);
        printf("DONE\n");
    }

    // Perform the streaming data collection process
    if(lc_open(&dconf[stream_dev])){
        printf("FAILED\n");
        fprintf(stderr, "LCRUN failed to open the device for streaming.\n");
        return -1;
    }
    if(lc_upload_config(&dconf[stream_dev])){
        printf("FAILED\n");
        fprintf(stderr, "LCRUN failed while configuring the data stream.\n");
        lc_close(&dconf[stream_dev]);
        return -1;
    }
    lc_show_config(&dconf[stream_dev]);

    // Prep the data file
    dfile = fopen(data_file,"w");
    if(dfile == NULL){
        fprintf(stderr, "LCRUN failed to open pre-data file: %s\n", pre_file);
        lc_close(&dconf[0]);
        return -1;
    }
    lc_datafile_init(&dconf[stream_dev], dfile);

    // Setup the keypress for exit conditions
    printf("Press \"Q\" to quit the process\nStreaming measurements...");
    fflush(stdout);
    lct_setup_keypress();

    // Start the data collection
    if(lc_stream_start(&dconf[stream_dev], -1)){
        printf("FAILED\n");
        fprintf(stderr, "LCRUN failed to start the data stream.\n");
        lc_close(&dconf[stream_dev]);
        return -1;
    }

    go = 1;
    for(count=0; go; count++){
        if(lc_stream_service(&dconf[stream_dev])){
            printf("FAILED\n");
            fprintf(stderr, "LCRUN failed while trying to service the data stream!\n");
            lc_stream_stop(&dconf[stream_dev]);
            lc_close(&dconf[stream_dev]);
            lct_finish_keypress();
            return -1;
        }
        lc_datafile_write(&dconf[stream_dev], dfile);

        // Test for exit conditions
        if(lct_is_keypress() && getchar() == 'Q')
            go = 0;
        else if(maxloop_i>0 && count>=maxloop_i)
            go = 0;
    }
    lct_finish_keypress();

    if(lc_stream_stop(&dconf[stream_dev])){
        printf("FAILED\n");
        fprintf(stderr, "LCRUN failed to halt the data stream!\n");
        lc_close(&dconf[stream_dev]);
        return -1;
    }
    lc_close(&dconf[stream_dev]);
    printf("DONE\n");

    // Perform the post data collection process
    if(ndev>2){
        printf("Performing post static measurements...");
        if(lc_open(&dconf[2])){
            printf("FAILED\n");
            fprintf(stderr, "LCRUN failed to open the post data collection device.\n");
            return -1;
        }
        if(lc_upload_config(&dconf[2])){
            printf("FAILED\n");
            fprintf(stderr, "LCRUN failed while configuring post data collection.\n");
            lc_close(&dconf[2]);
            return -1;
        }
        if(lc_stream_start(&dconf[2], -1)){
            printf("FAILED\n");
            fprintf(stderr, "LCRUN failed to start post data collection.\n");
            lc_close(&dconf[2]);
            return -1;
        }
        // STREAM!
        while(!lc_stream_iscomplete(&dconf[2])){
            if(lc_stream_service(&dconf[2])){
                fprintf(stderr, "\nLCRUN failed while servicing the T7 connection!\n");
                lc_stream_stop(&dconf[2]);
                lc_close(&dconf[2]);
                return -1;
            }
        }
        if(lc_stream_stop(&dconf[2])){
            printf("FAILED\n");
            fprintf(stderr, "LCRUN failed to halt preliminary data collection!\n");
            lc_close(&dconf[2]);
            return -1;
        }

        dfile = fopen(post_file,"w");
        if(dfile == NULL){
            fprintf(stderr, "LCRUN failed to open post-data file: %s\n", post_file);
            lc_close(&dconf[2]);
            return -1;
        }
        lc_datafile_init(&dconf[2],dfile);
        while(!lc_stream_isempty(&dconf[2]))
            lc_datafile_write(&dconf[2],dfile);
        fclose(dfile);
        dfile = NULL;
        lc_close(&dconf[2]);
        printf("DONE\n");
    }

    printf("Exited successfully.\n");
    return 0;
}



int parse_options(int argc, char* argv[]){
    int index;
    char* target=NULL;

    metacount = 0;

    // The target pointer indicates to what global parameter string values
    // will be written.  Option flags refocus the pointer and writing a value
    // will swing the pointer back to NULL.
    for(index=1; index<argc; index++){
        // Help text
        if(strncmp(argv[index],"-h",MAXSTR)==0){
            printf(help_text);
            return -1;
        }else if(strncmp(argv[index],"-r",MAXSTR)==0)
            target = pre_file;
        else if(strncmp(argv[index],"-d",MAXSTR)==0)
            target = data_file;
        else if(strncmp(argv[index],"-c",MAXSTR)==0)
            target = config_file;
        else if(strncmp(argv[index],"-p",MAXSTR)==0)
            target = post_file;
        else if(strncmp(argv[index],"-f",MAXSTR)==0 ||
                strncmp(argv[index],"-i",MAXSTR)==0 ||
                strncmp(argv[index],"-s",MAXSTR)==0){
            if(metacount>LCONF_MAX_META){
                target=NULL;
                fprintf(stderr,"Too many meta parameters. LCONFIG only accepts %d.\n",LCONF_MAX_META);
            }
            // Write the type specifier as the first character
            metastage[metacount][0] = argv[index][1];
            // Copy the remaining text behind
            target = &metastage[metacount][1];
            metacount++;
        }else if(target){
            strncpy(target,argv[index],MAXSTR);
            target = NULL;
        }else{
            target = NULL;
            fprintf(stderr,"Unexpected parameter \"%s\"\n", argv[index]);
            return -1;
        }
    }

    // Postprocessing: convert the MAXLOOP parameter into an integer
    if(sscanf(maxloop, "%d", &maxloop_i) != 1){
        fprintf(stderr,"Illegal value for -n parameter: %s\n",maxloop);
        return -1;
    }

    return 0;
}
