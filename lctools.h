
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

/*  The LCTOOLS header exposes functions and macros for building a highly 
simplified user interface, applying calibrations, and referencing data using the
LCONFIG system.  The display functions leverage the console control and escape
sequences for the Linux console environment.

CHANGELOG

v1.2    9/2020
- Replaced lct_stream_mean() with lct_stream_stat()
- Added lct_stat_t type

v1.1	10/2019		
- Debugged the diter utility functions.
- Added STREAM_MEAN and CAL functions

v1.0	9/2019		ORIGINAL RELEASE
*/

#ifndef __LCTOOLS
#define __LCTOOLS


// Add some headers
#include "lconfig.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <sys/select.h>


/****************************
 *                          *
 *       Constants          *
 *                          *
 ****************************/

#define LCT_VERSION 1.2


/****************************
 *                          *
 *      Prototypes          *
 *                          *
 ****************************/
 
typedef struct _lct_stat_t {
    double mean;
    double max;
    double min;
    double std;
    double rms;
    double pkpk;
    unsigned int N;
} lct_stat_t;


/****************************************
 *                                      *
 *       Formatting the Terminal        *
 *                                      *
 ****************************************/

/* CLEAR TERMINAL
.   This will clean up the terminal display.
.   We move the cursor to home and clear the display
*/
void lct_clear_terminal(void);


/* PRINT TEXT AT A LOCATION
.   This prints text starting at a row,column location
*/
void lct_print_text(const unsigned int row,
                const unsigned int column,
                const char * text);

/* PRINT HEADER TEXT AT A LOCATION
.   This prints text starting at a row,column location
*/
void lct_print_header(const unsigned int row,
                const unsigned int column,
                const char * text);




/* PRINT PARAM and PRINT XXX
.   print_param() and print_XXX() are responsible for drawing and refreshing
.   displays in a console.  They build and refresh a display of the format
.
.       param : value
.
.   The parameter label appears to the left of a colon and its value to the 
.   right; separated by spaces.  print_param() draws the parameter string and 
.   the print_XXX() functions print over the old values with strings formatted
.   based on the data type.
.
.   The row and column indicate the location in the console where the colon 
.   should be located.  Care should be taken to provide enough space for the 
.   formatted text.  (1,1) represents the upper left-hand corner of the 
.   terminal.
*/
void lct_print_param(const unsigned int row, 
                const unsigned int column, 
                const char * param);

void lct_print_str(const unsigned int row,
                const unsigned int column,
                const char * value);

void lct_print_int(const unsigned int row,
                const unsigned int column,
                const int value);

void lct_print_flt(const unsigned int row,
                const unsigned int column,
                const double value);

/* BOLD DISPLAY
.   These are identical to their regular counterparts, but they display bold
.   characters instead of ordinary characters.
*/
void lct_print_bparam(const unsigned int row, 
                const unsigned int column, 
                const char * param);

void lct_print_bstr(const unsigned int row,
                const unsigned int column,
                const char * value);

void lct_print_bint(const unsigned int row,
                const unsigned int column,
                const int value);

void lct_print_bflt(const unsigned int row,
                const unsigned int column,
                const double value);



/************************
 *                      *
 *      User Input      *
 *                      *
 ************************/


/* IS_KEYPRESS
.   Detect whether there is new data waiting on the stdin stream.  This services
.   a non-blocking keyboard input.  Returns 1 if there is new input.  Returns
.   0 if not.  The intent is that the application will use a standard read 
.   operation only when a keypress has been logged.
.
.   To work as expected, the setup_keypress() function should be called first. 
.   The finish_keypress() will return the terminal to normal operation.  See 
.   those funcitons for more information.
*/
char lct_is_keypress(void);

/* SETUP_KEYPRESS
.   Configure the terminal to allow the KEYPRESS function to work properly.  In
.   normal (Canonical) operation, terminal input is witheld until a user presses
.   enter (EOL).  SETUP_KEYPRESS reconfigures the terminal to return individual
.   characters immediately.
*/
void lct_setup_keypress(void);

/* FINISH_KEYPRESS
.   Configure the terminal to behave normally after waiting for a KEYPRESS.  
.   While waiting for a keypress, key entry at the terminal is passed to the 
.   file stream immediately without waiting for enter (EOL character).  However,
.   in normal operation, this is not ideal as it would prohibit entry editing.
*/
void lct_finish_keypress(void);

/* PROMPT ON KEYPRESS
.   On a keypress, block a loop to prompt the user for input.  When the 
.   SETUP_KEYPRESS function is called prior to a loop, the PROMPT_ON_KEYPRESS
.   function can be used to initiate a "blocking" prompt for user input only if
.   the user has pressed a particular key.  Otherwise, the loop will continue
.   unaffected.  The function returns 1 if the prompt was triggered and 0 
.   otherwise.
.
.   look_for    If look_for < 0, then any key will trigger the prompt.
.               Otherwise, look_for is interpreted as a character.  If that 
.               characteris found on stdin, the prompt will be triggered.
.   prompt      This string will be printed to stdout to prompt the user for 
.               input.
.   input       This is the string returned by the prompt.
.   length      This is the maximum acceptable string length.
*/
int lct_keypress_prompt(int look_for, const char* prompt, char* input, unsigned int length);



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
int lct_ai_bylabel(lc_devconf_t *dconf, unsigned int devnum, char label[]);
int lct_ao_bylabel(lc_devconf_t *dconf, unsigned int devnum, char label[]);
int lct_fio_bylabel(lc_devconf_t *dconf, unsigned int devnum, char label[]);


/************************************
 *                                  *
 *      Interacting with Data       *
 *                                  *
 ************************************/

/* LCT_DITER_T
.  LCT_DITER_INIT
.  LCT_DITER_NEXT
.   The LCT_DITER_T struct and supporting functions collectively form an 
.   efficient method for iterating over the data of a single channel in a data
.   set obtained from READ_DATA_STREAM().  After LCT_DITER_INIT is used to set
.   up the fast iteration, each call to LCT_DITER_NEXT returns a pointer to a
.   data point of the same channel in the order they were taken.  When the data
.   are exhausted, LCT_DITER_NEXT returns a NULL pointer.  See the example 
.   below.
.
.   For the details of each function's behavior, see the documentation below.   
.
double data[DATA_SIZE];
double *this;
lct_diter_t diter;
lc_devconf_t dconf[1];

// ...Code to read in data goes here...

lct_diter_init(dconf, 0, &diter, data, DATA_SIZE, CHANNEL);
while(this = lct_diter_next(&diter)){
    // ... Code to do something with THIS goes here ....
}
*/

typedef struct {
    double *next;
    double *last;
    unsigned int increment;
} lct_diter_t;

/* LCT_DITER_INIT
.   Initializes the DITER struct to control a fast iteration over the samples of
.   a single channel in a data set read by READ_DATA_STREAM.  DATA is the data
.   array containing the streamed data with DATA_SIZE elements.  CHANNEL 
.   indicates the index of the channel over which to iterate.
.
.Returns 0 normally.  Returns -1 if the channel is out of range.
*/
int lct_diter_init(lc_devconf_t *dconf, lct_diter_t *diter,
                    double* data, unsigned int data_size, unsigned int channel);
                    

/* LCT_DITER_NEXT
.   Returns a pointer to the next sample in a data set belonging to the channel 
.   configured by the LCT_DITER_INIT function.  Returns NULL if the data are
.   exhausted.
*/
double* lct_diter_next(lct_diter_t *diter);

/* LCT_DATA
.   A utility for indexing a data array in an application.  This function 
.   returns a pointer to a data element corresponding to the specified channel
.   and sample number.  LCT_DATA() may be used directly on the buffer data array
.   returned by the LC_STREAM_READ() funciton or on a secondary data buffer into
.   which buffer data have already been copied.  
.
.   DCONF is the device configuration that was used to read in the data.
.   The DATA array contains the samples read in the same order they were read.
.   The DATA_SIZE integer indicates the length of the DATA array, and it is used
.   as a safety check to prevent accidental overrun errors.
.   CHANNEL specifies from which input stream channel the data should be read.
.   SAMPLE specifies which measurement should be returned.
.
.   LCT_DATA() returns NULL when 
.       CHANNEL is larger than the number of channels configured for streaming
if(channel >= lc_nistream(dconf)) ...
.       or when the requested sample is beyond the end of the data array
if(lc_nistream(&dconf)*sample + channel >= data_size) ...
.   
.   Except for the error checking above, LTC_DATA() is roughly equivalent to
return &data[lc_nistream(&dconf)*sample + channel];
.
*/
double * lct_data(lc_devconf_t *dconf,  
                double data[], unsigned int data_size,
                unsigned int channel, unsigned int sample);

/* LCT_CAL_INPLACE
.   Apply the channel calibrations in-place on the target array.  The contents
.   of the data array are presumed to be raw voltages as returned by the 
.   READ_DATA_STREAM function.  DCONF and DEVNUM are used to determine the 
.   calibration parameters, DATA is the array on which to operate, and DATA_SIZE
.   is its length.  
.
.   LCT_CAL_INPLACE is intended to be used on entire data arrays that have been
.   read using lc_stream_read or it can be used directly on data in the buffer.
.   For example, 

int err;
unsigned int channels, samples_per_read, index;
double *data;
lc_devconf_t dconf;
// ... setup code ... 
err = lc_stream_service(&dconf);
err = lc_stream_read(&dconf, &data, &channels, &samples_per_read);
if(data){
    lct_cal_inplace(&dconf, data, channels*samples_per_read);
    // ... do things with data[] ...
}
*/
void lct_cal_inplace(lc_devconf_t *dconf, 
                double data[], unsigned int data_size);


/* LCT_CAL
.   Apply the channel calibration from AI channel AINUM to a raw voltage 
.   measurement.  Returns the calibrated measurement in engineering units.
*/
int lct_cal(lc_devconf_t *dconf, unsigned int ainum, double *data);

/* LCT_STREAM_STAT
.   Calculate statistics on the data[] array with a total length data_size.
.   The results are accumulated in the result[] struct array, which is no longer
.   than maxchannels.  Just like LCT_CAL_INPLACE() and LCT_DATA(), 
.   LCT_STREAM_STAT() can be used directly on data in the ring buffer returned
.   by LC_STREAM_READ() or it can be used on data that have already been copied
.   into the application's memory.
.
.   DCONF is the device configuration struct that was used to read the data.
.   The DATA array contains the data in the order read, and is total length
.   specified by DATA_SIZE.
.   RESULT is an array of LCT_STAT_T structs that will contain the analysis
.   results when LCT_STREAM_STAT is done.  The length of the RESULT array is
.   passed through MAXCHANNELS.
.
.   When DATA is NULL, the RESULT structs are initialized and the function 
.   exits.  This can be a useful mode of operation for initializing total and
.   working stat structs (see the example in the LCT_STAT_JOIN() documentation).
.
.   An error condition can occur if the number of configured device channels
.   (see LC_NISTREAM()) exceeds the length of the RESULT array specified by 
.   MAXCHANNELS.  In this case, the statistics will still be correctly 
.   calculated on all channels up to (MAXCHANNELS-1).
.
.   LCT_STREAM_STAT() returns LC_ERROR in this case, and LC_NOERR otherwise.
.
.   In the example below, LCT_CAL_INPLACE() is used on the buffer memory prior 
.   to calling LCT_STREAM_STAT().  Note that 16 is passed to MAXCHANNELS since
.   that was the size of the STAT array.

int err;
unsigned int channels, samples_per_read, index;
double *data;
lc_devconf_t dconf;
lct_stat_t stat[16];
// ... setup code ... 
err = lc_stream_service(&dconf);
err = lc_stream_read(&dconf, &data, &channels, &samples_per_read);
if(data){
    lct_cal_inplace(&dconf, data, channels*samples_per_read);
    lct_stream_stat(&dconf, data, channels*samples_per_read, stat, 16);
    // ... do stuff with stat ...
    // ... see LCT_STAT_JOIN ...
}
*/
int lct_stream_stat(lc_devconf_t *dconf, 
                double data[], unsigned int data_size,
                lct_stat_t result[], unsigned int maxchannels);
                
/* LCT_STAT_JOIN
.   Join an array of LCT_STAT_T structs representing the statistics of two 
.   segments of a single data set.  LCT_STAT_JOIN() is intended for applications
.   where data are streamed and discarded while signal statistics are maintained.
.   When NEXT is an array of LCT_STAT_T structs returned by a call to
.   LCT_STREAM_STAT() on the most recently streamed data, and TARGET is an array
.   of LCT_STAT_T structs representing the signal statistics of all previous
.   data.
.
.   The example below declares WORKING and TOTAL LCT_STAT_T struct arrays.  The
.   former is used to contain statistics from the most recent segement of data
.   streamed from the experiment.  The latter is a running total of all data
.   streamed.  Note that TOTAL must be initialized using a call to 
.   LCT_STREAM_STAT() with a NULL pointer instead of genuine data.

int err;
unsigned int channels, samples_per_read, index;
double *data;
lc_devconf_t dconf;
lct_stat_t working[16], total[16];
// Initialize the total stat structs
lct_stream_stat(&dconf, NULL, 0, total, 16);
// ... setup code ... 
err = lc_stream_service(&dconf);
err = lc_stream_read(&dconf, &data, &channels, &samples_per_read);
if(data){
    lct_cal_inplace(&dconf, data, channels*samples_per_read);
    lct_stream_stat(&dconf, data, channels*samples_per_read, working, 16);
    lct_stream_join(total, working, 16);
    // the "total" array now contains running statistics that includes the most
    // recent results returned in "working."
}
*/
void lct_stat_join(lct_stat_t *target, lct_stat_t *next);

#endif
