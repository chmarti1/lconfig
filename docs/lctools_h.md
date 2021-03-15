[back to API](api.md)

Version 4.05  
March 2021  
Christopher R. Martin  

# <a name=top></a>lctools.h

There are enough high-level operations that needed to be performed when writing `lcburst`, `lcrun`, and `lcstat` that it made sense to split off a separate toolkit for high-level operations like these.  They aren't part of the core LConfig system because they aren't necessary for configuring or using the T-series DAQs, but they're still awfully helpful.

- [Overview](#overview)  
- [Interacting with channels](#channels)  
- [Interacting with data](#data)  
    - [Calibration](#calibration)  
    - [Iterating](#iter)  
    - [Stream Statistics](#stat)  
- [User interface tools](#ui)  
    - [Formatted output](#print)  
    - [Keystroke input](#keystroke)  

## <a name=overview></a>Overview

Since its original creation, `lctools.h` has gradually grown to a list of functions almost as long as `lconfig.h`.  These can broadly be divided into tools for retreiving information on device channels, tools for interacting with data being streamed in using the core tools in `lconfig.h`, and tools for providing a really simple user interface through a Posix terminal.

## <a name=channels></a>Interacting with Channels

There are any number of applications where the application author may not want to depend on device channels being configured in any particular order.  It may be preferable to identify a channel by its label rather than its index.

```C
int lct_ai_bylabel(lc_devconf_t *dconf, char label[]);

int lct_ao_bylabel(lc_devconf_t *dconf, char label[]);

int lct_fio_bylabel(lc_devconf_t *dconf, char label[]);
```

These tools scan the analog input, analog output, and flexible IO channels until a label that precisely matches `label` is found, and returns its index.  If no channel with a matching label is found, the functions return -1.

## <a name=data></a> Interacting with data

The core LConfig system places great emphasis on flexible configuration for both data collection and even post processing (like calibration), but it only provides tools for automating the data collection process.  Applications that require live post-processing of raw measurements need tools not provided in the core LConfig system.

### <a name=calibration></a> Calibration

LConfig allows the user to configure each analog input channel with a calibration, and it will faithfully copy that calibration into the resulting data file, but it provides no support for applying the calibration.  `lctools.h` provides a pair of functions precisely for this purpose.

```C
void lct_cal_inplace(lc_devconf_t *dconf, 
                double data[], unsigned int data_size);

int lct_cal(lc_devconf_t *dconf, unsigned int ainum, double *data);

char * lct_cal_units(lc_devconf_t *dconf, unsigned int ainum);
```

### `lct_cal_inplace()`

`lct_cal_inplace` is written for the scenario where a block of data has been read into a data array in the application.  It applies the calibration in-place so that after being called, the data will be in the calibrated units and no longer raw voltages.  This in-place calibration can also be used to calibrate data in the LConfig buffer without having to copy it into the application ram.  For example...

```C
double * data;
unsigned int channels, samples_per_read;
// ... setup code
lc_stream_read(&dconf, &data, &channels, &samples_per_read);
lct_cal_inplace(&dconf, data, channels*samples_per_read);
// ... and so on
```

### `lct_cal()`

`lct_cal()` simply applies the calibration of a single analog input channel, identified by `ainum`, to a single raw measurement value, `data`.  If `ainum` is out of the range of configured channels, the function returns `LCONF_ERROR`.  Otherwise, it returns `LCONF_NOERR` and the result is written back to `data`.

### `lct_cal_units()`

`lct_cal_units` returns a pointer to the units string inside the calibration struct.  This is identical to `dconf->aich[ainum].calunits`, except that NULL is returned if `ainum` is out of range.

[top](#top)

### <a name=iter></a>Iteration

### `lct_diter_t` Iteration

Because measurements are intermixed from multiple channels when data are read from `lc_stream_read`, setting up for loops to iterate over data from one or two individual channels can be bulky and can even lead to crashes if it is not done correctly.  The `diter` family of functions handles data iteration automatically.
```C
int lct_diter_init(lc_devconf_t *dconf, lct_diter_t *diter,
                    double* data, unsigned int data_size, unsigned int channel);

double* lct_diter_next(lct_diter_t *diter);
```

The entire process is controlled through a `diter_t` struct, which `lct_diter_init` configures to control iteration of all elements of streaming input channel index `channel` within a data block with `data_size` elements .  Each successive call to `lct_diter_next` returns a pointer to the next sample from that channel in order.  Once the dataset has been exhausted, `lct_diter_next` returns NULL.

Here is an example implementation.

```C
lc_dconf_t dconf;
lct_diter_t my_iteration;
double *data, *sample;
unsigned int channels, samples_per_read;
// Put code here that sets up, starts, and services the data acquisition 
// process.  See lc_load_config(), lc_upload_config(), lc_stream_start(), 
// and lc_stream_service().
// Then, it's time to get a pointer to a block of data in the buffer
lc_stream_read(&dconf, &data, &channels, &samples_per_read);
// data now points to the beginning of a block of data.
// To loop over all the elements of channel 2, ------------v
lct_diter_init(&my_iteration, data, channels*my_iteration, 2);
// This example will loop over all of the elements of channel 2
// contained in this data block in the LConfig buffer.
while(sample = lct_diter_next(&my_iteration))
{
	// Do things with (*sample) here.
}
```

### `lct_data()`

The `diter` approach is efficient, but it lacks the ability to hop around between channels.  To save users from needing to calculate 2D-1D array index maps themselves, the `lct_data()` returns a pointer into the `data` array corresponding to channel `channel` and sample number `sample`.  Because it is a pointer, that means this approach can be used to read or write to the array.  The `data_size` is the total length of the `data` array (true size, not number of blocks or channels).

```C
double * lct_data(lc_devconf_t *dconf, 
                double data[], unsigned int data_size,
                unsigned int channel, unsigned int sample);
```

[top](#top)

###<a name=stat></a> Stream statistics

In many applications, there is no need to move the bulk of the data from a streaming operation into the application's ram.  Instead, it is often sufficient to merely accumulate signal statistics or do some analysis on the data in-place in the buffer.  To that end, `lctools` includes the `lc_stat_t` struct-based type and supporting functions:

```C
void lct_stat_init(lct_stat_t stat[], unsigned int channels);

int lct_stream_stat(lc_devconf_t *dconf, lct_stat_t values[], unsigned int maxchannels);
```

### The `lct_stat_t` struct

The `lct_stat_t` contains relatively self-explanatory scalar statistics on a single channel.  It has member values

| Member | Type           | Description               |
|:------:|:--------------:|---------------------------|
| `n`    | `unsigned int` | Integer number of samples accumulated so far |
| `mean` | `double` 	  | Signal's mean value       |
| `max`  | `double`       | Maximum sample value      |
| `min`  | `double`	  | Minimum sample value      |
| `var`  | `double`       | Signal's variance         |

### `lct_stream_stat_init()`

This simple function initializes an array of `lct_stat_t` struct `channels` elements long.  The intent is that each element in the array corresponds to statistics accumulated on a single channel in a device's data stream.  The mean, variance, and `n` values are set to zero, and the maximum and minimum values are set to floating point negative and positive infinity respectively.  This ensures that the first value read will be both the maximum and the minimum so far.

### `lct_stream_stat()`

This function is intended to be called _instead of_ calling the `lc_stream_read()` function to read in new data.  Instead, `lct_stream_stat()` calls `lc_stream_read()` and processes data directly to modify an array of stream statistic structs.

The `maxchannels` integer must indicate the length of the `values[]` array.  Any configured stream channels beyond that value will be ignored in the stream to avoid a segfault error.

At any time between calls to `lct_stream_stat()`, the data members of the `values[]` contain valid statistics on the samples read in so far.  This can be useful for monitoring statistics even as they are still being accumulated.

[top](#top)

## <a name="ui"></a>User interface tools

Coming soon... For now, see the documentation commented into `lctools.h`.

### Formatted output

### Keystroke input

[top](#top)