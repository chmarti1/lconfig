[back](../readme.md)

Version 3.06<br>
June 2019<br>
Chris Martin<br>

##  <a name='top'></a> The LConfig API
If you want to use the LConfig system, but you don't just want to use the canned drun or dburst codes, you'll need to know the LConfig API.  The lconfig.h header exposes functions for 
- [Interacting with configuration files](#config)
- [Interacting with devices](#dev)
- [Performing diagnostics](#diag)
- [Data collection](#data)
- [Meta configuration](#meta)

It is important to remember that LConfig is just a layer built on top of LabJack's interface to automate the device configuration, to handle software triggering, and to handle data buffering.

The typical steps for writing an applicaiton that uses LConfig are: <br>
** (1) Load a configuration file ** with `load_config()`.  The configuratino file will completely define the data acquisition operation by specifying which channels should be configured, how much data should be streamed at what rate, and how software triggering should be done.
** (2) Open the device ** with `open_config()`.  After loading the configuration file, the device configuration struct will contain all the information needed to find the correct device.
** (3) Upload the configuration ** with `upload_config()`.  Most (but not all) of the directives in the configuration file are enforced in this step.  AI resolution, analog output streaming, and trigger settings are not enforced until a stream is started.
** (4) Start a data stream ** with `start_data_stream()`.  This enforces the last of the configuration parameters and initiates the stream of data.
** (5) Service the data stream ** with `service_data_stream()`.  The data service function moved the newly available data into a ring buffer and (if so configured) scans for a trigger event.  This funciton is responsible for maintaining the trigger state registers in the device configuration struct.
** (6) Read data ** with `read_data_stream()`.  When data are available, this funciton returns a pointer into the ring buffer, where the next block of data may be read.  If no data are available, either because a trigger has not occurred or because the service function has not yet completed, the pointer is NULL.  Separating service and read operations permits applications to quckly stream lots of data and read it later for speed.
** (7) Stop the stream ** with `stop_data_stream()`.  This halts the data collection process, but does NOT free the data buffer.  Some applications (like `dburst`) may want to quickly stream data to memory and read it later when the acquisition process is done.  This allows steps 7 and 6 to be reversed without consequence.
** (8) Close the device connection ** with `close_config()`.  Closing a connection automatically calls `clear_buffer()`, which frees the ring buffer memory and dumps any data not read.

### <a name='config'></a> Interacting with configuration files

```C
int load_config(          DEVCONF* dconf, 
                const unsigned int devmax, 
                       const char* filename)
```
`load_config` loads a configuration file into an array of DEVCONF structures.  Each instance of the `connection` parameter signals the configuration a new device.  `dconf` is an array of DEVCONF structs with `devmax` elements that will contain the configuration parameters found in the data file, `filename`.  The function returns either `LCONF_ERROR` or `LCONF_NOERR` depending on whether an error was raised during execution.

```C
void write_config(        DEVCONF* dconf, 
                const unsigned int devnum, 
                             FILE* ff)
```
`write_config` writes a configuration stanza to an open file, `ff`, for the parameters of device number `devnum` in the array, `dconf`.  A DEVCONF structure written by `write_config` should result in an identical structure after being read by `load_config`.

Rather than accept file names, `write_config` accepts an open file pointer so it is convenient for creating headers for data files (which don't need to be closed after the configuration is written).  It also means that the write operation doesn't need to return an error status.

[top](#top)

### <a name='dev'></a> Interacting with devices

```C
int open_config(          DEVCONF* dconf, 
                const unsigned int devnum)
```
`open_config` opens a connection to the device described in the `devnum` element of the `dconf` array.  The function returns either `LCONF_ERROR` or `LCONF_NOERR` depending on whether an error was raised during execution.

```C
int close_config(         DEVCONF* dconf, 
                const unsigned int devnum)
```
`close_config` closes a connection to the device described in the `devnum` element of the `dconf` array.  If a ringbuffer is still allocated to the device, it is freed, so it is important NOT to call `close_config` before data is read from the buffer.  The function returns either `LCONF_ERROR` or `LCONF_NOERR` depending on whether an error was raised during execution.

```C
int upload_config(        DEVCONF* dconf, 
                const unsigned int devnum)
```
`upload_config` writes to the relevant modbus registers to assert the parameters in the `devnum` element of the `dconf` array.  The function returns either `LCONF_ERROR` or `LCONF_NOERR` depending on whether an error was raised during execution.

```C
int download_config(      DEVCONF* dconf, 
                const unsigned int devnum, 
                          DEVCONF* out)
```
`download_config` pulls values from the modbus registers of the open device connection in the `devnum` element of the `dconf` array to populate the parameters of the `out` DEVCONF struct.  This is a way to verify that the device parameters were successfully written or to see what operation was performed last.

It is important to note that some of the DEVCONF members configure parameters that are not used until a data operation is executed (like `nsample` or `samplehz`).  These parameters can never be downloaded because they do not reside persistently on the device.

[top](#top)

### <a name='diag'></a> Performing diagnostics

```C
int ndev_config(          DEVCONF* dconf, 
                const unsigned int devmax)
                
int nistream_config(      DEVCONF* dconf, 
                const unsigned int devnum)
                
int nostream_config(      DEVCONF* dconf, 
                const unsigned int devnum)
```
The `ndev_config` returns the number of devices configured in the `dconf` array, returning a number no greater than `devmax`.  `nistream_config` and `nostream_config` return the number of input and ouptut stream channels configured.

```C
void show_config(         DEVCONF* dconf, 
                const unsigned int devnum)
```
The `show_config` function calls `download_config` and prints a detailed color-coded comparison of live parameters against the configuration parameters in the `devnum` element of the `dconf` array.  Parameters that do not match are printed in red while parameters that agree with the configuration parameters are printed in green.

[top](#top)

### <a name='data'></a> Data collection

```C
int start_data_stream(    DEVCONF* dconf, 
                const unsigned int devnum,
                               int samples_per_read);

int service_data_stream(  DEVCONF* dconf, 
                const unsigned int devnum);

int read_data_stream(     DEVCONF* dconf, 
                const unsigned int devnum, 
                            double **data, 
                      unsigned int *channels, 
                      unsigned int *samples_per_read);

int stop_data_stream(     DEVCONF* dconf, 
                const unsigned int devnum);
                
```
There are four steps to a data acquisition process; start, service, read, and stop.  Version 3 of LCONFIG uses an automatically configured ring buffer, so the application never needs to perform memory management.  The addition of a service function lets the back end deal with moving data into the ring buffer, testing for trigger events, and it allows the application to distinguish between moving data on to the local machine and retrieving it for use in the application.

`start_data_stream` configures the LCONFIG buffer and starts the device's data collection process on the device `devnum` in the `dconf` array.  For `start_data_stream` to work correctly, the device should already be open and configured using the `open_config` and `upload_config` functions.  The device will be configured to transfer packets with `samples_per_read` samples per channel.  If `samples_per_read` is negative, then the LCONF_SAMPLES_PER_READ constant will be used instead.  This information is recorded in the `RB` ringbuffer struct.  The buffer can be substantial since RAM checks are performed prior to allocating memory.

All data transfers are done in *blocks*.  A *block* of data is `samples_per_read` x `channels` of individual measurements.

The `service_data_stream` function retrieves a single new block of data into the ring buffer.  If pre-triggering is configured, trigger events will be ignored until enough samples have built up in the buffer to meet the pre-trigger requirements.  Once a trigger event occurs (or if no trigger is configured), blocks of data will become available for reading.  

The `read_data_stream` function returns a double precision array, `data` representing a 2D array with  a single block of data `samples_per_read` x `channels` long.  The `s` sample of `c` channel can be retrieved by `data[s*channels + c]`.  If there are no data available, then `data` will be `NULL`.

The data acquisition process is halted by the `stop_data_stream` function, but the buffer is left intact.  As a result, data can be slowly read from the buffer long after the data collection is complete.  Before a new stream process can be started, the `clean_data_stream` function should be called to free the buffer.  In applications where only one stream operation will be executed, it may be easier to allow `close_config` to clean up the buffer instead.

```C
void status_data_stream( DEVCONF* dconf, 
               const unsigned int devnum,
                     unsigned int *samples_streamed, 
                     unsigned int *samples_read,
                     unsigned int *samples_waiting);
                     
int iscomplete_data_stream(DEVCONF* dconf, 
                 const unsigned int devnum);
                 
int isempty_data_stream( DEVCONF* dconf, 
               const unsigned int devnum);
```
These functions are handy tools for monitoring the progress of a data collection process.  The `status_data_stream` function returns the per-channel stream counts streamed into, read out of, and waiting in the ring buffer.  Authors should keep in mind that the `service_data_stream` function adjusts the `samples_streamed` value to exclude data that was thrown away in the triggering process.

`iscomplete_data_stream` returns a 1 or 0 to indicate whether the total number of `samples_streamed` per channel has exceeded the `nsample` parameter found in the configuration file.

`isempty_data_stream` returns a 1 or 0 to indicate whether the ring buffer has been exhausted by read operations.

```C
int init_file_stream(    DEVCONF* dconf, 
                const unsigned int devnum,
                             FILE* FF);
                        
int write_file_stream(     DEVCONF* dconf, 
                const unsigned int devnum,
                             FILE* FF);

```
These are tools for automatically writing data files from the stream data.  They accept a file pointer from an open `iostream` file.  In versions 2.03 and older, LCONFIG was responsible for managing the file opening and closing process, but as of version 3.00, it is up the application to provide an open file.

`init_file_stream` writes a configuration file header to the data file.  It also adds a timestamp indicating the date and time that `init_file_stream` was executed.  It should be emphasized that (especially where triggers are involved) substantial time can pass between this timestamp and the availability of data.  Care should be taken if precise absolute time values are needed.

`write_file_stream` calls `read_data_stream` and prints an ascii formatted data array into the open file provided.  

Note that the applicaiton still needs to call `start_data_stream` to begin the data acquisition process, and `service_data_stream`.  Only `read_data_stream` is replaced from the typical streaming process with `write_file_stream`.


```C
int update_fio(DEVCONF* dconf, const unsigned int devnum);
```
The FIO channels represent the only features in LCONFIG that require users to interact manually with the DEVCONF struct member variables.  `update_fio` is called to command the T7 to react to changes in the FIO settings or to obtain new FIO measurements.  For example, the following code might be used to adjust flexible I/O channel 0's duty cycle to 25%.
```
dconf[devnum].fioch[0].duty = .25;
update_fio(dconf,devnum);
```
The `update_fio()` function also downloads the latest measurements into the relevant member variables.  The following code might be used to measure a pulse frequency from a flow sensor.
```C
double frequency;
int update_fio(dconf, devnum);
frequency = 1e6 / dconf[devnum].fioch[0].time;
```

[top](#top)

### <a name='meta'></a> Meta configuration

Experiments are about more than just data rates and analog input ranges.  What were the parameters that mattered most to you?  What did you change in today's experiment?  Was there anything you wanted to note in the data file?  Are there standard searchable fields you want associated with your data?  Meta parameters are custom configuraiton parameters that are embedded in the data file just like device parameters, but they are ignored when configuring the T4/T7.  They are only there to help you keep track of your data.

These have been absolutely essential for me.

```C
int get_meta_int(          DEVCONF* dconf, 
                 const unsigned int devnum,
                        const char* param, 
                               int* value);
                               
int get_meta_flt(          DEVCONF* dconf, 
                 const unsigned int devnum,
                        const char* param, 
                            double* value);

int get_meta_str(          DEVCONF* dconf, 
                 const unsigned int devnum,
                        const char* param, 
                              char* value);
```
The `get_meta_XXX` functions retrieve meta parameters from the `devnum` element of the `dconf` array by their name, `param`.  If the parameter does not exist or if it is of the incorrect type, 
The values are written to target of the `value` pointer, and the function returns the `LCONF_ERROR` or `LCONF_NOERR` error status based on whether the parameter was found.

```C
int put_meta_int(          DEVCONF* dconf, 
                 const unsigned int devnum, 
                        const char* param, 
                                int value);
                                
int put_meta_flt(          DEVCONF* dconf, 
                 const unsigned int devnum, 
                        const char* param, 
                             double value);

int put_meta_str(          DEVCONF* dconf, 
                 const unsigned int devnum, 
                        const char* param, 
                              char* value);
```
The `put_meta_XXX` functions retrieve meta parameters from the `devnum` element of the `dconf` array by their name, `param`.  The values are written to target of the `value` pointer, and the function returns the `LCONF_ERROR` or `LCONF_NOERR` error status based on whether the parameter was found.

[top](#top)
