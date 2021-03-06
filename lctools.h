
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

/*  The LCTOOLS header exposes functions and macros for building a highly 
simplified user interface, applying calibrations, and referencing data using the
LCONFIG system.  The display functions leverage the console control and escape
sequences for the Linux console environment.

CHANGELOG

v1.3    3/2021
- Added idle

v1.2    9/2020
- Added stream statistics tools

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
#include <time.h>       // for idle functions
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <sys/select.h>


/****************************
 *                          *
 *       Constants          *
 *                          *
 ****************************/

#define LCT_VERSION 1.3



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
int lct_ai_bylabel(lc_devconf_t *dconf, char label[]);
int lct_ao_bylabel(lc_devconf_t *dconf, char label[]);
int lct_fio_bylabel(lc_devconf_t *dconf, char label[]);



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
                unsigned int channel, unsigned int sample);

/* LCT_CAL_INPLACE
.   Apply the channel calibrations in-place on the target array.  The contents
.   of the data array are presumed to be raw voltages as returned by the 
.   READ_DATA_STREAM function.  DCONF and DEVNUM are used to determine the 
.   calibration parameters, DATA is the array on which to operate, and DATA_SIZE
.   is its length.  
*/
void lct_cal_inplace(lc_devconf_t *dconf, 
                double data[], unsigned int data_size);


/* LCT_CAL
.   Apply the channel calibration from AI channel AINUM to a raw voltage 
.	measurement.  Returns the calibrated measurement in engineering units.
*/
int lct_cal(lc_devconf_t *dconf, unsigned int ainum, double *data);

/* LCT_CAL_UNITS
.   Return a pointer to the analog input channel units.  Returns NULL if
.   the analog input channel index is out of range.
*/
char * lct_cal_units(lc_devconf_t *dconf, unsigned int ainum);

/* LCT_STAT_T
.   Contains aggregated statistics on data
.       n : number of samples aggregated into the stat struct
.       mean : the mean value
.       max : the highest value
.       min : the lowest value
.       std : the standard deviation of the data
*/
typedef struct __lct_stat_t__ {
    unsigned int n;
    double mean;
    double max;
    double min;
    double var;
} lct_stat_t;

/* LCT_STAT_INIT
.   Initialize an LCT_STAT_T struct.  This sets most parameters to zero,
.   but the max -> -infty, min -> +infty.  The STAT input argument is taken
.   to be an array of lct_stat_t structs, each representing a channel.  
.   The CHANNELS int is interpreted as the length of that array.
*/
void lct_stat_init(lct_stat_t stat[], unsigned int channels);

/* LCT_STREAM_STAT
.   Read in a single block of data from the buffer and aggregate statistics
.   on the data.  LCT_STREAM_STAT() should be called in place of the 
.   LC_STREAM_READ() function.  LCT_STREAM_STAT calls LC_STREAM_READ()
.   to access data in the buffer directly.  If data are ready, they are
.   calibrated in place using the LCT_CAL_INPLACE() function before
.   statistics are aggregated.
.
.   The LCT_STAT_T VALUES struct contains the aggregated mean, maximum,
.   minimum, and standard deviation.  Each element of the VALUES array
.   corresponds to one of the stream channels in the order they are
.   configured.  From the four basic statistics, others can be constructed.
.   variance = std*std
.   root-mean-square amplitude = sqrt(mean*mean + var)
.   peak-to-peak amplitude = max - min
.
.   If MAXCHANNELS is greater than 0, it is interpreted as the maximum 
.   length of the VALUES array.  If the number of configured analog stream
.   channels is longer than MAXCHANNELS, LCT_STREAM_STAT will print a 
.   warning to stderr and return with LCONF_ERROR.  If MAXCHANNELS is 0,
.   it is assumed that adequate measures have already been taken to protect
.   against a memory overrun.
.
.   
*/
int lct_stream_stat(lc_devconf_t *dconf, lct_stat_t values[], unsigned int maxchannels);





typedef struct __lct_idle_t__ {
    struct timespec next;
    unsigned int interval_us;
    unsigned int resolution_us;
} lct_idle_t;


/* LCT_IDLE_INIT
.  LCT_IDLE
.   Initialize the idle struct to set up a recurring blocking function to 
.   establish idle time.  This is intended to be used along with LCT_IDLE()
.   to insert idle time so that a loop executes somewhat regularly.  The
.   LCT_IDLE_INIT() function should be called immediately before the loop is
.   started and the LCT_IDLE() function should be placed at the end of the 
.   loop.
.
.   INTERVAL_US is the approximate target loop execution period in 
.   microseconds.
.
.   RESOLUTION_US is the approximate duration for which the process should 
.   sleep between checks for timer expiration.

lct_idle_t myidle;
lct_idle_init(&myidle, 1000);
while(1){
    ... Do something with variable execution time ...
    lct_idle(&myidle);
}
*/
int lct_idle_init(lct_idle_t *idle, unsigned int interval_us, unsigned int resolution_us);
int lct_idle(lct_idle_t *idle);

#endif
