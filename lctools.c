#include "lctools.h"
#include "lconfig.h"
#include "lcmap.h"

#include <math.h>


/*
.   Macros for moving the cursor around
*/


/*
Help on Linux terminal control characters

$man console_codes

gets the job done.
*/


// Move the cursor to home (1,1)
#define LDISP_CHOME         printf("\x1B[H")
// Move the cursor to (x,y)
#define LDISP_CGO(x,y)      printf("\x1B[%d;%dH",x,y)
// The value format specifiers
// Default to 15 character width to overwrite previous long outputs
// Use 3 decimal places on floating precision numbers
#define LDISP_VALUE_LEN     "15"
#define LDISP_FLT_PREC      "3"
#define LDISP_FMT_TXT       "\x1B[%d;%dH%s"
#define LDISP_FMT_HDR       "\x1B[%d;%dH\x1B[4m%s\x1B[0m"
#define LDISP_FMT_PRM       "\x1B[%d;%dH%s :"
#define LDISP_FMT_STR       "\x1B[%d;%dH%-" LDISP_VALUE_LEN "s"
#define LDISP_FMT_INT       "\x1B[%d;%dH%-" LDISP_VALUE_LEN "d"
#define LDISP_FMT_FLT       "\x1B[%d;%dH%-" LDISP_VALUE_LEN "." LDISP_FLT_PREC "f"
#define LDISP_FMT_BPRM      "\x1B[%d;%dH\x1B[1m%s :\x1B[0m"
#define LDISP_FMT_BSTR      "\x1B[%d;%dH\x1B[1m%-" LDISP_VALUE_LEN "s\x1B[0m"
#define LDISP_FMT_BINT      "\x1B[%d;%dH\x1B[1m%-" LDISP_VALUE_LEN "d\x1B[0m"
#define LDISP_FMT_BFLT      "\x1B[%d;%dH\x1B[1m%-" LDISP_VALUE_LEN "." LDISP_FLT_PREC "f\x1B[0m"

#define LDISP_STDIN_FD      STDIN_FILENO

// Return the maximum integer
#define LDISP_MAX(a,b)      ((a)>(b)?(a):(b))
// Return the minimum integer
#define LDISP_MIN(a,b)      ((a)>(b)?(b):(a))
// Clamp an integer between two extrema
// assumes a > b and returns c clamped between a and b
#define LDISP_CLAMP(a,b,c)  LDISP_MAX(LDISP_MIN((a),(c)), (b))

/****************************
 *                          *
 *       Algorithm          *
 *                          *
 ****************************/

//******************************************************************************
void lct_clear_terminal(void){
    // Move the cursor to home and clear from the cursor to the end
    printf("\x1B[H\x1B[J");
}

//******************************************************************************
void lct_print_text(const unsigned int row,
                const unsigned int column,
                const char * text){
    printf(LDISP_FMT_TXT,row,column,text);
}

//******************************************************************************
void lct_print_header(const unsigned int row,
                const unsigned int column,
                const char * text){
    printf(LDISP_FMT_HDR,row,column,text);
}

//******************************************************************************
void lct_print_param(const unsigned int row, 
                const unsigned int column, 
                const char * param){
    int x;
    // Offset the starting location of the parameter by the length of the string
    // Include an extra character for the space
    x = column - strlen(param)-1;
    // Keep things from running off the edge
    // If there is a string overrun, the space and colon will be offset
    x = LDISP_MAX(1,x);
    printf(LDISP_FMT_PRM,row,x,param);
}

//******************************************************************************
void lct_print_str(const unsigned int row, 
                const unsigned int column, 
                const char * value){

    // Offset the column by two to allow for the colon and a space
    printf(LDISP_FMT_STR,row,column+2,value);
}

//******************************************************************************
void lct_print_int(const unsigned int row, 
                const unsigned int column, 
                const int value){

    // Offset the column by two to allow for the colon and a space
    printf(LDISP_FMT_INT,row,column+2,value);
}

//******************************************************************************
void lct_print_flt(const unsigned int row, 
                const unsigned int column, 
                const double value){

    // Offset the column by two to allow for the colon and a space
    printf(LDISP_FMT_FLT,row,column+2,value);
}

//******************************************************************************
void lct_print_bparam(const unsigned int row, 
                const unsigned int column, 
                const char * param){
    int x;
    // Offset the starting location of the parameter by the length of the string
    // Include an extra character for the space
    x = column - strlen(param)-1;
    // Keep things from running off the edge
    // If there is a string overrun, the space and colon will be offset
    x = LDISP_MAX(1,x);
    printf(LDISP_FMT_BPRM,row,x,param);
}

//******************************************************************************
void lct_print_bstr(const unsigned int row, 
                const unsigned int column, 
                const char * value){

    // Offset the column by two to allow for the colon and a space
    printf(LDISP_FMT_BSTR,row,column+2,value);
}

//******************************************************************************
void lct_print_bint(const unsigned int row, 
                const unsigned int column, 
                const int value){

    // Offset the column by two to allow for the colon and a space
    printf(LDISP_FMT_BINT,row,column+2,value);
}

//******************************************************************************
void lct_print_bflt(const unsigned int row, 
                const unsigned int column, 
                const double value){

    // Offset the column by two to allow for the colon and a space
    printf(LDISP_FMT_BFLT,row,column+2,value);
}

//******************************************************************************
char lct_is_keypress(void){
// Credit for this code goes to
// http://cc.byexamples.com/2007/04/08/non-blocking-user-input-in-loop-without-ncurses/
    struct timeval tv;
    fd_set fds;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&fds);
    FD_SET(LDISP_STDIN_FD, &fds); //STDIN_FILENO is 0
    select(LDISP_STDIN_FD+1, &fds, NULL, NULL, &tv);
    return FD_ISSET(LDISP_STDIN_FD, &fds);
}

//******************************************************************************
void lct_setup_keypress(void){
    struct termios terminal_state;
    tcgetattr(LDISP_STDIN_FD, &terminal_state);     // Get the current state
    terminal_state.c_lflag &= ~(ICANON | ECHO);   // Clear the canonical bit
                                                  // Turn off the echo
    terminal_state.c_cc[VMIN] = 1;      // Set the minimum number of characters
    tcsetattr(LDISP_STDIN_FD, TCSANOW, &terminal_state); // Apply the changes
    // TCSANOW = Terminal Control Set Attribute Now; controls timing
}

//******************************************************************************
void lct_finish_keypress(void){
    struct termios terminal_state;
    tcgetattr(LDISP_STDIN_FD, &terminal_state);     // Get the current state
    terminal_state.c_lflag |= (ICANON | ECHO);  // Set the canonical bit
                                                // Turn on the echo
    tcsetattr(LDISP_STDIN_FD, TCSANOW, &terminal_state); // Apply the changes
    // TCSANOW = Terminal Control Set Attribute Now; controls timing
}

//******************************************************************************
int lct_keypress_prompt(int look_for, const char* prompt, char* input, const unsigned int length){
    if(     lct_is_keypress() && \
            (getchar()==(char)look_for || look_for<0)){
        lct_finish_keypress();
        fputs(prompt,stdout);
        fgets(input,length,stdin);
        lct_setup_keypress();
        return 1;
    }
    return 0;
}




/************************************
 *                                  *
 *      Interacting with Channels   *
 *                                  *
 ************************************/
/* LCT_AI_BYLABEL
.  LCT_AO_BYLABEL
.  LCT_FIO_BYLABEL
.   These functions return the integer index of the appropriate channel with the
.   label that matches LABEL.  If no matching channel is found, the functions
.   return -1.
*/
int lct_ai_bylabel(lc_devconf_t *dconf, char label[]){
    int ii;
    for(ii=0; ii<dconf->naich; ii++){
        if(strcmp(label, dconf->aich[ii].label)==0)
            return ii;
    }
    return -1;
}

int lct_ao_bylabel(lc_devconf_t *dconf, char label[]){
    int ii;
    for(ii=0; ii<dconf->naoch; ii++){
        if(strcmp(label, dconf->aoch[ii].label)==0)
            return ii;
    }
    return -1;
}

int lct_ef_bylabel(lc_devconf_t *dconf, char label[]){
    int ii;
    for(ii=0; ii<dconf->nefch; ii++){
        if(strcmp(label, dconf->efch[ii].label)==0)
            return ii;
    }
    return -1;
}



/************************************
 *                                  *
 *      Interacting with Data       *
 *                                  *
 ************************************/



int lct_diter_init(lc_devconf_t *dconf, lct_diter_t *diter,
                    double* data, unsigned int data_size, unsigned int channel){
    unsigned int ii;
    ii = lc_nistream(dconf);
    if(channel >= ii){
        diter->last = NULL;
        diter->next = NULL;
        return -1;
    }
    diter->increment = &data[channel+ii] - &data[channel];
    diter->next = &data[channel];
    diter->last = &data[data_size-1];
    return 0;
}


double* lct_diter_next(lct_diter_t *diter){
    double *done;
    if(diter->next > diter->last){
        diter->last = NULL;
        return NULL;
    }
    done = diter->next;
    diter->next += diter->increment;
    return done;
}

/* LCT_DATA
.   A utility for indexing a data array in an application.  Presuming that an 
.   applicaiton has defined a double array and has streamed data into it using
.   the READ_DATA_STREAM function, the LCT_DATA function provides a pointer to
.   an element in the array corresponding to a given channel number and sample
.   number.  DCONF and DEVNUM are used to determine the number of channels being
.   streamed.  DATA is the data array being indexed, and DATA_SIZE is its total
.   length.  CHANNEL and SAMPLE determine which data point is to be returned.
.   
.   LTC_DATA is roughly equivalent to
.       &data[nistream_config(dconf,devnum)*sample + channel]
.
.   When CHANNEL or SAMPLE are out of range, LTC_DATA returns a NULL pointer.
*/
double * lct_data(lc_devconf_t *dconf, 
                double data[], unsigned int data_size,
                unsigned int channel, unsigned int sample){
    unsigned int ii;
    
    ii = lc_nistream(dconf);
    if(channel >= ii)
        return NULL;
    ii = sample*ii + channel;
    if(ii >= data_size)
        return NULL;
    return &data[ii];
}

/* LCT_CAL_INPLACE
.   Apply the channel calibrations in-place on the target array.  The contents
.   of the data array are presumed to be raw voltages as returned by the 
.   READ_DATA_STREAM function.  DCONF and DEVNUM are used to determine the 
.   calibration parameters, DATA is the array on which to operate, and DATA_SIZE
.   is its length.  
*/
void lct_cal_inplace(lc_devconf_t *dconf, 
                double data[], unsigned int data_size){
    lct_diter_t diter;
    double *this;
    unsigned int ii;
    // Loop through the analog channels
    for(ii = 0; ii<dconf->naich; ii++){
        // Initialize the iteration
        if(lct_diter_init(dconf,&diter,data,data_size,ii)){
            fprintf(stderr, "LCT_CAL_INPLACE: This error should never happen!  Calibration failed.\n"\
                    "    We sincerely apologize.\n");
            return;
        }
        while(this = lct_diter_next(&diter))
            *this = dconf->aich[ii].calslope * ((*this) - dconf->aich[ii].calzero);
    }
    return;
}


/* LCT_CAL
.   Apply the channel calibration from AI channel AINUM to a raw voltage 
.	measurement.  Returns the calibrated measurement in engineering units.
.	If the AINUM value is not a valid analog input channel, LCT_CAL returns
.	-1.  Otherwise, the value in DATA is adjusted in-place.
*/
int lct_cal(lc_devconf_t *dconf, unsigned int ainum, double *data){
	if(ainum >= dconf->naich){
		fprintf(stderr, "LCT_CAL: Analog input channel %d is out of range.  Only %d are configured.\n", ainum, dconf->naich);
		return LCONF_ERROR;
	}
	*data = (*data - dconf->aich[ainum].calzero) * dconf->aich[ainum].calslope;
	return LCONF_NOERR;
}

/* LCT_CAL_UNITS
.   Return a pointer to the analog input channel units.  Returns NULL if
.   the analog input channel index is out of range.
*/
char * lct_cal_units(lc_devconf_t *dconf, unsigned int ainum){
    if(ainum >= dconf->naich){
        fprintf(stderr, "LCT_CAL_UNITS: Analog input channel %d is out of range.  Only %d are configured.\n", ainum, dconf->naich);
        return NULL;
    }
    return dconf->aich[ainum].calunits;
}


/* LCT_STAT_INIT
.   Initialize an LCT_STAT_T struct.  This sets most parameters to zero,
.   but the max -> -infty, min -> +infty.  The STAT input argument is taken
.   to be an array of lct_stat_t structs, each representing a channel.  
.   The CHANNELS int is interpreted as the length of that array.
*/
void lct_stat_init(lct_stat_t stat[], unsigned int channels){
    unsigned int ii;
    for(ii=0; ii<channels; ii++){
        stat[ii].n = 0;
        stat[ii].mean = 0.;
        stat[ii].max = -INFINITY;
        stat[ii].min = INFINITY;
        stat[ii].var = 0.;
    }
}

/* LCT_STREAM_STAT
.   Read in a single block of data from the buffer and aggregate statistics
.   on the data.  
*/
int lct_stream_stat(lc_devconf_t *dconf, lct_stat_t values[], unsigned int maxchannels){
    double *data = NULL, *this = NULL;
    unsigned int channels, samples_per_read, err, ii;
    lct_diter_t diter;
    
    // Get data.  Are there any?
    // If not, return with an error.
    if(err = lc_stream_read(dconf, &data, &channels, &samples_per_read))
        return err;
    else if(!data)
        return LCONF_ERROR;
        
    // Are the number of channels legal?
    if(maxchannels > 0 && channels > maxchannels){
        fprintf(stderr, "LCT_STREAM_STAT: The device is configured with more channels than the application allows.\n");
        channels = maxchannels;
    }
        
    // First apply the calibration to the channels
    lct_cal_inplace(dconf, data, channels*samples_per_read);
    
    // Loop through the channels
    for(ii=0; ii<channels; ii++){
        // initialize an iterator for this channel
        if(lct_diter_init(dconf, &diter, data, channels*samples_per_read, ii)){
            fprintf(stderr, "LCT_STREAM_STAT: Failed to initialize the channel iterator for channel %d\n", ii);
            return LCONF_ERROR;
        }
        // Modify the prior statistics to receive in-place calculation
        // Adjust the variance to be mean squre
        values[ii].var += values[ii].mean*values[ii].mean;
        // Re-scale by the number of samples so new samples can simply be added
        values[ii].var *= values[ii].n;
        values[ii].mean *= values[ii].n;

        while(this = lct_diter_next(&diter)){
            values[ii].n ++;
            values[ii].mean += *this;
            values[ii].var += (*this) * (*this);
            values[ii].max = (*this) > values[ii].max ? (*this) : values[ii].max;
            values[ii].min = (*this) < values[ii].min ? (*this) : values[ii].min;
        }
        // Clean up the intermediate values struct
        values[ii].mean /= values[ii].n;
        values[ii].var /= values[ii].n;
        values[ii].var = values[ii].var - values[ii].mean*values[ii].mean;

    }
    return LCONF_NOERR;
}



int lct_idle_init(lct_idle_t *idle, unsigned int interval_us, unsigned int resolution_us){
    if(clock_gettime(CLOCK_REALTIME, &idle->next))
        return -1;
    idle->interval_us = interval_us;
    idle->resolution_us = resolution_us;
    idle->next.tv_nsec += interval_us * 1000;
    if(idle->next.tv_nsec > 1e9){
        idle->next.tv_nsec -= 1e9;
        idle->next.tv_sec += 1;
    }
    return 0;
}
    
int lct_idle(lct_idle_t *idle){
    struct timespec now;
    int wait_us;

    if(clock_gettime(CLOCK_REALTIME, &now))
        return -1;
    // Wait a nominal period
    wait_us = idle->next.tv_sec - now.tv_sec;
    wait_us *= 1e6;
    wait_us += (idle->next.tv_nsec - now.tv_nsec)/1000 - 5*idle->resolution_us;
    
    if(wait_us > 0)
        usleep(wait_us);
    
    while(1){
        if(clock_gettime(CLOCK_REALTIME, &now))
            return -1;
        // If the timer has expired
        if(now.tv_sec > idle->next.tv_sec || \
                (now.tv_sec == idle->next.tv_sec && now.tv_nsec > idle->next.tv_nsec)){
            // Increment the next expiration
            idle->next.tv_nsec += idle->interval_us * 1000;
            if(idle->next.tv_nsec > 1e9){
                idle->next.tv_nsec -= 1e9;
                idle->next.tv_sec += 1;
            }
            // All done
            return 0;
        }
        // If the timer has not expired, wait
        usleep(idle->resolution_us);
    }
    // This should never be executed!
    return -1;
}
