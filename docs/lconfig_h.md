[back to API](api.md)

Version 5.01  
October 2025  
Christopher R. Martin  


# <a name=top></a> lconfig.h  

- [Overview](#overview)
- [Interacting with configuration files](#config)
- [Interacting with devices](#dev)
- [Performing diagnostics](#diag)
- [Data collection](#data)
- [Writing data files](#datafile)
- [Stream diagnostics](#datadiag)
- [DIO Extended features](#ef)
- [Meta configuration](#meta)
- [Digital communication](#digital)

## <a name=overview></a>Overview

Many useful binaries can be written without ever using `lctools.h` or `lcmap.h`.  The core functionality is all contained in `lconfig.h`.

The most complicated (and the most common) application for the LConfig system is streaming data.  The typical steps for writing an applicaiton that uses LConfig to stream data are:  

**(1) Load a configuration file** with `lc_load_config()`.  The configuration file will completely define the data acquisition operation by specifying which channels should be configured, how much data should be streamed at what rate, and how software triggering should be done.  

**(2) Open the device** with `lc_open()`.  After loading the configuration file, the device configuration struct will contain all the information needed to find the correct device. The connection should be closed by `lc_close()` at the end of the operation.  

**(3) Upload the configuration** with `lc_upload_config()`.  Most (but not all) of the directives in the configuration file are enforced in this step.  AI resolution, analog output streaming, and trigger settings are not enforced until a stream is started.  

**(4) Start a data stream** with `lc_stream_start()`.  This enforces the last of the configuration parameters and initiates the stream of data. This function also allocates memory to a ring buffer that will be used by the `lc_stream_service()` and `lc_stream_read()` functions to manage the data stream.  This memory needs to be freed by `lc_clean()` at the end of the operation.  

**(5) Service the data stream** with `lc_stream_service()`.  This is a non-blocking function that checks for and retrieves data from a device.  When it is called, available data (if any) are moved into a ring buffer and (if so configured) scanned for a software trigger event.  This function is responsible for maintaining the trigger state registers in the device configuration struct.  

**(6) Check stream status** with `lc_stream_status()`. Some applications will not need this step, but it is extremely useful for checking the progress of data acquisition. The function returns the number of samples that have been streamed, read, and the number waiting to be read. In particular, this can be useful to determine whether a trigger has occurred.

**(7) Read data** with `lc_stream_read()`.  When data are available in the local buffer (thanks to a call to `lc_stream_service()`), this function returns a pointer into the ring buffer, where the next block of data may be read.  If no data are available, either because a trigger has not occurred or because the service function has not yet completed, the pointer is NULL.  Separating service and read operations permits applications to quickly stream lots of data and read it later for speed.  

**(8) Downsample data** by calling `lc_stream_downsample()`.  Calls to `lc_stream_read()` return all raw data prior to downsampling so the application has access prior to filtering and discarding.  If the application does not require access to the raw data, `lc_stream_downsample()` can be called immediately after every call to `lc_stream_read()`.

**(9) Repeat** by returning to step 5 (servicing the data stream).  

**(10) Stop the stream** with `lc_stream_stop()`.  This halts the data collection process, but does NOT free the data buffer.  Some applications (like `lcburst`) may want to quickly stream data to memory and read it later when the acquisition process is done.  This allows steps 7 and 8 to be reversed without consequence.  

**(11) Clean up** with `lc_close()` and `lc_clean()`.  These close the connection to the device and free dynamically allocated memory respectively.  In addition to freeing the stream buffer, `lc_clean()` also frees memory allocated to meta data parameters read from the configuration file.

## <a name='config'></a> Interacting with configuration files

```C
int lc_load(       lc_devconf_t* dconf, 
                const unsigned int devmax, 
                       const char* filename);
```
`lc_load_config` loads a configuration file into an array of `lc_devconf_t` structures.  Each instance of the `connection` parameter signals the configuration a new device.  `dconf` is an array of `lc_devconf_t` structs with `devmax` elements that will contain the configuration parameters found in the data file, `filename`.  The function returns either `LC_ERROR` or `LCONF_NOERR` depending on whether an error was raised during execution.

```C
void lc_write(     lc_devconf_t* dconf, 
                             FILE* ff);
```
`write_config` writes a configuration stanza to an open file, `ff`, for the parameters of device configuration struct pointed to by `dconf`.  A `lc_devconf_t` structure written by `lc_write_config` should result in an identical structure after being read by `lc_load_config`.

Rather than accept file names, `lc_write_config` accepts an open file pointer so it is convenient for creating headers for data files (which don't need to be closed after the configuration is written).  It also means that the write operation doesn't need to return an error status.

[top](#top)

## <a name='dev'></a> Interacting with devices

```C
int lc_open( lc_devconf_t* dconf );
```
`lc_open_config` opens a connection to the device pointed to by `dconf`.  The function returns either `LC_ERROR` or `LCONF_NOERR` depending on whether an error was raised during execution.

```C
int lc_close( lc_devconf_t* dconf );
```
`lc_close_config` closes a connection to the device pointed to by `dconf`.  If a ringbuffer is still allocated to the device, it is freed, so it is important NOT to call `lc_close_config` before data is read from the buffer.  The function returns either `LC_ERROR` or `LCONF_NOERR` depending on whether an error was raised during execution.

```C
int lc_upload( lc_devconf_t* dconf );
```
`lc_upload_config` writes to the relevant modbus registers to assert the parameters in the `dconf` configuration struct.  The function returns either `LC_ERROR` or `LCONF_NOERR` depending on whether an error was raised during execution.

[top](#top)

## <a name='diag'></a> Performing diagnostics

```C
int lc_ndev(          lc_devconf_t* dconf, 
                const unsigned int devmax);
                
int lc_nistream( lc_devconf_t* dconf );
                
int lc_nostream( lc_devconf_t* dconf );
```
The `lc_ndev` returns the number of devices configured in the `dconf` array, returning a number no greater than `devmax`.  `lc_nistream` and `lc_nostream` return the number of input and ouptut stream channels configured.  The input stream count is especially helpful since it includes non-analog input streams (like digital input streams).

```C
void lc_show( lc_devconf_t* dconf );
```
The `lc_show` function prints a detailed list of parameters specified by the `dconf` configuraiton struct.  The format of the printout is varied based on what is configured so that unconfigured features are not summarized.

Once the T4 was introduced to LabJack's product offerings, it became necessary to be able to write code that was sensitive to the different valid channel ranges of the different devices.

```C
void lc_aichannels(const lc_devconf_t* dconf, int *min, int *max);

void lc_aochannels(const lc_devconf_t* dconf, int *min, int *max);

void lc_efchannels(const lc_devconf_t* dconf, int *min, int *max);

void lc_diochannels(const lc_devconf_t* dconf, int *min, int *max);
```

The `lc_aichannels`, `lc_aochannels`, `lc_efchannels`, and `lc_diochannels` functions return minimum and maximum valid channel numbers for the device type configured in `dconf`.  The _actual_ device type and NOT the configured device type is used.

[top](#top)

## <a name='data'></a> Data collection

There are five steps to a data acquisition process: start, service, read, stop, and clean.  In a data stream, samples are accumulated in the LabJack's hardware buffer until they are transmitted to the host computer in _blocks_ of data, containing a number of samples from each channel.

```C
int lc_stream_start(    lc_devconf_t* dconf, 
                                  int samples_per_read);

int lc_stream_service(  lc_devconf_t* dconf);

int lc_stream_read(     lc_devconf_t* dconf, 
                               double **data, 
                         unsigned int *channels, 
                         unsigned int *samples_per_read);

int lc_stream_downsample(lc_devconf_t *dconf, 
                               double *data, 
                   const unsigned int channels,
                         unsigned int *samples_per_read);

int lc_stream_stop(     lc_devconf_t* dconf);
```
### `lc_stream_start()`
`lc_stream_start` initializes a local buffer for the stream and starts the data acquisition process on the device identified by the `dconf` pointer.  When `samples_per_read` is positive, it sets the number of samples per channel in each _block_.  Otherwise, the default of 64 is used.  The buffer size is set to contain more than `nsample` (set in the configuration file) samples per channel.  

**Example:** If `samples_per_read` were 64, two channels were configured, and `nsample` were also 64, the buffer would be set to contain two blocks for a total of 2blocks x 64samples = 128 samples per channel or 2channels x 2blocks x 64samples = 256 total samples.  Even though a single block would be enough buffer, the buffer is always "rounded up" so that there will always be a minimum of two blocks.  In LabJack's documentation a sample per channel is more elegantly called a "scan."  We use "samples per channel" to be especially clear about the distinction between individual samples and groups of samples form all configured channels.

**Example:** If `nsample` were increased to 100, the answer would be the same; 128 samples per channel represents the smallest number of blocks that will provide the required number of samples in the buffer.  

**Example:** If `nsample` were set to 1000, the buffer would be set to contain 16blocks x 64samples = 1024samples per channel.

Samples are stored as double precision floating point values in volts.  So, for each sample that needs to be stored, 8 bytes are required on most systems.  In the last example with 1024 samples per channel and two channels, 2channels x 1024samples per channel x 8bytes = 16.4KB.  This amount of memory is irrelevant on most modern computing systems.

### `lc_stream_service()`

Once a stream on a device has been started, the `lc_stream_service` function is used to check for a new _block_ of data waiting on the LabJack and reads it in if it is.  If no data are ready, then the service operation does nothing and returns immediately.  That means that calls to `lc_stream_service()` can return very quickly or they can last some time while the data are transferred.  It is very poor practice to write code that does this:

```C
// This is NOT good practice!
lc_stream_start(&dconf,-1);
// This loop stupidly occupies the processor at 100%
// and uselessly bombards the LJM library with requests 
// for data that isn't there.
while(True){
    lc_stream_service(&dconf);
    ... Do stuff with the data ...
    ... establish some break condition ...
}
lc_stream_stop(&dconf);
lc_clean(&dconf);
```

In this example, the service function is called as frequently as the processor can execute the loop.  It has nothing to do with how long it is likely to be between available data packets.

```C
// This is much better
lc_stream_start(&dconf,-1);
lct_idle_init(&idle);	// see the lctools documentation
while(True){
    lc_stream_start(&dconf);
    ... Do stuff with the data ...
    ... establish some break condition ...
    lct_idle(&idle);
}
lc_stream_stop(&dconf);
lc_clean(&dconf);
```
This example deliberately inserts idle time that frees the processor.  Even greater gains can be made by introducing a long idle time (see the [lctools](lctools_h.md) documentation) once per loop and then a series of shorter idle times in an inner loop.  This has the effect of causing the service function to be called much more frequently when data are more likely to be available.

### `lc_stream_read()`

Without an attempt to read data out of the LCONFIG device buffer, data accumulate until new samples begin to overwrite older ones.  Calling the `lc_stream_read` function returns a pointer into the buffer and assumes that the application is making good use of them.  From that moment forward, those data are considered "consumed."  

The pointer, `data`, should be interpreted as a 2D double precision array with  a single block of data (`samples_per_read` x `channels` array elements).  The `s` sample of `c` channel can be retrieved by `data[s*channels + c]`.  

If there are no data available, then `data` will be returned as `NULL` and the function returns `LC_ERROR`.  Alternately, see the [stream diagnostic](#datadiag) functions to query the state of the buffer.  

Once data are returned by `lc_stream_read()`, they must be copied into a safe location or otherwise used immediately, because the data at that location will eventually be overwritten.  The data are safe until the next call to `lc_stream_service()`, after which it will be possible for the data to be overwritten.  

See the [data file](#datafile) functions below or the `lct_stat` functions included in [lctools](lctools_h.md) for ways to quickly handle data for common tasks.

A read operation is not required between each stream operation.  In fact, the `lcburst` binary streams its data directly to the buffer and makes no attempt to read it until the streaming process is complete.  This decision is made for performance, so the process of handling the data does not slow down the application.

### `lc_stream_downsample()`

Downsampling is performed in a separate step after reading raw data.  In this way, the application has an opportunity to access all data before they are filtered and discarded.  The `lc_stream_downsample()` function is responsible for applying anti-aliasing filters to each of the analog input channels and then discarding the number of samples indicated by the `downsample` configuration parameter.  Streamed digital input and extended feature channels are not filtered, and samples are simply discarded.  As a result, momentary transitions can be lost if they are not analyzed prior to downsampling.

The `samples_per_read` parameter is modified to indicate the number of samples actually remaining in the data after downsampling is complete.  The number of samples remaining in each data block can change depending on how they are distributed in the data set, so applications should always be sensitive to the possibility that `samples_per_read` can change with every read cycle.

### `lc_stream_stop()`

The data acquisition process is halted by the `lc_stream_stop()` function, but the buffer is left intact.  Once this function is called, no new samples will become available to the `lc_stream_service()` and it _should_ raise an error if it is called on an inactive stream.  However, the same is not true of `lc_stream_read()`.  Even though new data are no longer being added to the buffer, the data already there can still be read by successive calls to `lc_stream_read()`.

## <a name="datafile"></a> Writing data files

LCONFIG includes tools for automatically writing data files from the data stream.  They accept a file pointer from an open `iostream` file.  

```C
int lc_datafile_init(    lc_devconf_t* dconf, 
                        FILE* FF);
                        
int lc_datafile_write(lc_devconf_t *dconf, 
			FILE *FF, 
			double *data, 
        		unsigned int channels, 
        		unsigned int samples_per_read);

```

### `lc_datafile_init()`

`lc_datafile_init()` calls `lc_write()` to writes a configuration file header to the data file.  It also adds a timestamp indicating the date and time that `lc_datafile_init()` was executed.  It should be emphasized that (especially where triggers are involved) substantial time can pass between this timestamp and the availability of data.  When authoring applications where the timestamp is intended to mark the time of the first row, `lc_datafile_init()` should be called immediately before the first call to `lc_datafile_write()`.

### `lc_datafile_write()`

`lc_datafile_write()` accepts the values provided by `lc_stream_read()` and writes data to the file provided.  `lc_datafile_write()` honors the `dataformat` parameter, automatically writing in ASCII or binary as directed.

Note that the application still needs to call `lc_stream_start()` to begin the data acquisition process and `lc_stream_service()` to stream in data, but in this mode of operation, `lc_datafile_write()` takes the place of the `lc_stream_read()` function.


##<a name="datadiag"></a> Stream diagnostic functions

```C
void lc_stream_status( lc_devconf_t* dconf, 
               const unsigned int devnum,
                     unsigned int *samples_streamed, 
                     unsigned int *samples_read,
                     unsigned int *samples_waiting);
                     
int lc_stream_iscomplete(lc_devconf_t* dconf, 
                 const unsigned int devnum);
                 
int lc_stream_isempty( lc_devconf_t* dconf, 
               const unsigned int devnum);
```

### `lc_stream_status()`

These functions are handy tools for monitoring the progress of a data collection process.  The `lc_stream_status` function returns the per-channel stream counts streamed into, read out of, and waiting in the ring buffer.  Authors should keep in mind that the `lc_stream_service` function adjusts the `samples_streamed` value to exclude data that was thrown away in the triggering process.

### `lc_stream_iscomplete()`

`lc_stream_iscomplete` returns a 1 or 0 to indicate whether the total number of `samples_streamed` per channel has exceeded the `nsample` parameter found in the configuration file.

### `lc_stream_isempty()`

`lc_stream_isempty` returns a 1 or 0 to indicate whether the ring buffer has been exhausted by read operations.

## <a name="ef"></a> DIO Extended features

Extended feature channels can be configured by `lc_upload_config()` just like analog input and output channels.  However, if they are not involved in a streaming operation, their data need to be retrieved explicitly.

### `lc_update_ef()`

```C
int lc_update_ef( lc_devconf_t* dconf );
```
For now, the extended feature channels represent the only features in LCONFIG that require users to interact directly with the lc_devconf_t struct member variables.  `lc_update_ef` is called to command the LabJack to react to changes in the EF settings and to obtain new EF measurements.  For example, the following code might be used to adjust flexible I/O channel 0's duty cycle to 25%.
```C
dconf.fioch[0].duty = .25;
lc_update_ef(&dconf);
```
The `lc_update_ef()` function also downloads the latest measurements into the relevant member variables.  The following code might be used to measure a pulse frequency from a flow sensor.
```C
lc_update_ef(&dconf);
frequency = 1e6 / dconf.efch[0].time;
```

See the [Extended Features](config.md#ef) section of the [configuration](config.md) documentation for more information.

[top](#top)

## <a name='meta'></a> Meta configuration

Experiments are about more than just data rates and analog input ranges.  What were the parameters that mattered most to you?  What did you change in today's experiment?  Was there anything you wanted to note in the data file?  Are there standard searchable fields you want associated with your data?  Meta parameters are custom configuraiton parameters that are embedded in the data file just like device parameters, but they are ignored when configuring the T4/T7.  They are only there to help you keep track of your data.

These have been absolutely essential for me.

```C
int lc_meta_get_int(          lc_devconf_t* dconf, 
                        const char* param, 
                               int* value);
                               
int lc_meta_get_flt(          lc_devconf_t* dconf, 
                        const char* param, 
                            double* value);

int lc_meta_get_str(          lc_devconf_t* dconf, 
                        const char* param, 
                              char* value);
```
The `lc_meta_get_XXX` functions retrieve meta parameters from `dconf` by their name, `param`. The values are written to `value`. If the parameter does not exist or if it is of the incorrect type, these functions return `LC_ERROR`, and value is not changed.  If the type is incorrect, a warning is also printed, so the user is made aware of the reason for the problem.  If the value does not exist, the function exits quietly, so it is up to the application to decide whether an error is warranted.

```C
int lc_meta_get_num(          lc_devconf_t* dconf, 
                        const char* param, 
                              double* value);
```

Often, the behavior of an application may depend on a number, and we don't really care how the user specified it in the configuration file.  `lc_meta_get_num()` attempts to convert integer or string values to a double precision number.  If the parameter does not exist, `lc_meta_get_num()` returns `LC_ERROR`.  If conversion from string to float fails, a warning is printed, and `-LC_ERROR` is returned.

```c
lc_metatype_t lc_meta_get_type( lc_devconf_t* dconf,
							const char* param);
```
In cases where it is unclear which data type a meta parameter might have, the `lc_meta_get_type` function checks it by returning the enumerated meta types: `LC_MT_INT`, `LC_MT_FLT`, or `LC_MT_STR`.  This function is also a convenient way to silently (without error messages) check for existance of a parameter.  If the parameter does not exist, it returns `LC_MT_NONE`. 


```C
int lc_meta_put_int(          lc_devconf_t* dconf, 
                        const char* param, 
                                int value);
                                
int lc_meta_put_flt(          lc_devconf_t* dconf, 
                        const char* param, 
                             double value);

int lc_meta_put_str(          lc_devconf_t* dconf, 
                        const char* param, 
                              char* value);
```
The `lc_meta_put_XXX` functions write to the meta parameters.  If the meta parameter does not already exist, a new one is created with the appropriate type.  If a parameter with the same name already exists, its value will be overwritten and (if necessary) its type will be changed appropriately.  `lc_meta_put_XXX` only fails if the memory allocated to meta data is full.

```c
int lc_meta_del(			lc_devconf_t *dconf,
						const char* param);
```

The list of meta parameters are in a c-string style list, so the appearance of the first empty parameter terminates the list.  As a result, deletion of a parameter requires shifting all subsequent parameters forward in the list.  This function manages that process and also checks for illegal parameter types and other sources of list corruption.  For example, if non-empty meta parameters are found beyond the end of the list (a "zombie" element), a warning is printed to `stderr` and they are cleared.

[top](#top)

## <a name=digital></a>Digial communication

The T-series supports a number of different communication protocols.  These can be configured in a configuration file using the `COM` family of configuration directives.  See [reference](reference.md) for a detailed list.  Once configured, communications interfaces may be controlled using the `lc_com` functions:

```C
int lc_com_start(lc_devconf_t* dconf, const unsigned int comchannel,
        const unsigned int rxlength);
        
int lc_com_stop(lc_devconf_t* dconf, const unsigned int comchannel);
        
int lc_com_write(lc_devconf_t* dconf, const unsigned int comchannel,
        const char *txbuffer, const unsigned int txlength);
        
int lc_com_read(lc_devconf_t* dconf, const unsigned int comchannel,
        char *rxbuffer, const unsigned int rxlength, int timeout_ms);
        
int lc_communicate(lc_devconf_t* dconf, const unsigned int comchannel,
        const char *txbuffer, const unsigned int txlength, 
        char *rxbuffer, const unsigned int rxlength,
        const int timeout_ms);
```

### `lc_com_start()`

The `lc_com_start` function uploads the configuration for the COM channel and enables the feature.  The LJM receive buffer is configured to have the size `rxlength` in bytes.  For UART, this should be twice the number of bytes the application plans to transmit because the LJM UART interface uses 16-bit-wide buffers.  The other COM interfaces use byte buffers, so the same deliberate oversizing is not needed.  Returns -1 on failure and 0 on success.

### `lc_com_stop()`

The `lc_com_stop` funciton disables the COM interface without reading or writing to it.  Returns a -1 on failure and 0 on success.

### `lc_com_write()`

The `lc_com_write` function transmits the contents of the `txbuffer` over an active COM interface.  `txlength` is the number of bytes in the `txbuffer` array.  For UART interfaces, only odd indices of the `txbuffer` should be used since the LabJack LJM Library UART interface uses 16-bit wide buffers, and the most significant 8-bits are ignored. Returns -1 on failure and 0 on success.

### `lc_com_read()`

The `lc_com_read` function reads from the COM receive buffer into the `rxbuffer` array up to a maximum of `rxlength` bytes.  Reading can be done in one of three modes depending on the value passed to `timeout_ms`.  When `timeout_ms` is positive, `lc_com_read` will wait until exactly `rxlength` bytes of data are available or until `timeout_ms` milliseconds have passed.  When `timeout_ms` is negative, `lc_com_read` will read any data available and return immediately.  When `timeout_ms` is zero, `lc_com_read` will wait indefinitely for the data to become available.  Unlike the WRITE, START, and STOP funcitons, the `lc_com_read` funciton returns the number of bytes read on success, and -1 on failure. 

### `lc_com_stop()`

A session of digital communication requires at least the use of `lc_com_start`, `lc_com_stop`, and either `lc_com_read` or `lc_com_write` or both.  The `lc_communicate` function implements all of these steps in a single function.  In this order, it calls `lc_com_start`, `lc_com_write`, `lc_com_read`, and `lc_com_stop`.  The write and read steps may be skipped by setting the `txlength` or `rxlength` arguments to zero.

[top](#top)

***
