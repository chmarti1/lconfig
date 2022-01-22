[back](documentation.md)

Version 4.6
January 2022
Chris Martin  

## Binaries

A binary starts its job by reading in a configuration file, which is just a plain text file written by a user.  Based on the instructions it finds there, it configures the data acquisition device(s) and executes the corresponding data acquisition operation.  The precise job that is done depends on the configuration file and the binary.

There are three binaries included with LConfig: `lcrun`, `lcburst`, and `lcstat`.  These are inteded to be sufficiently generic to do most data acquisition tasks.  `lcrun` is intended for long slow sample-rate tests where the test duration may not be known in advance.  `lcburst` is intended for short high-speed tests.  `lcstat` is intended for monitoring and debugging an experiment.

On a Linux system, following the steps in [Compiling](compiling.bin), the binaries will be installed in `/usr/local/bin`.  For help text, use

```bash
$ lcrun -h
... prints help text ...
$ lcburst -h
... prints help text ...
$ lcstat -h
... prints help text ...
```
If you're using a terminal window and want to be able to scoll comfortably, use
```bash
$ lcrun -h | less
$ lcburst -h | less
$ lcstat -h | less
```

### lcrun

The **L**aboratory **C**onfiguration **RUN** utility, `lcrun`, streams data from up to 8 devices directly to [data files](data.md).  This means it is well suited to long tests that involve big data sets, but the maximum data rate will be limited by the write speed to the hard drive.  For high-speed applications, look at `lcburst`.

`lcrun` ignores the `nsample` parameter.

```bash
$ lcrun -h
lcrun [-h] [-r PREFILE] [-p POSTFILE] [-d DATAFILE] [-c CONFIGFILE] 
     [-f|i|s param=value]
  Runs a data acquisition job until the user exists with a keystroke.

-c CONFIGFILE
  By default, LCRUN will look for "lcrun.conf" in the working
  directory.  This should be an LCONFIG configuration file for the
  LabJackT7 containing no more than three device configurations.
  The -c option overrides that default.
     $ lcrun -c myconfig.conf

-d DATAFILE
  This option overrides the default continuous data file name
  "YYYYMMDDHHmmSS_lcrun.dat"
     $ lcrun -d mydatafile.dat

-n MAXREAD
  This option accepts an integer number of read operations after which
  the data collection will be halted.  The number of samples collected
  in each read operation is determined by the NSAMPLE parameter in the
  configuration file.  The maximum number of samples allowed per channel
  will be MAXREAD*NSAMPLE.  By default, the MAXREAD option is disabled.

-f param=value
-i param=value
-s param=value
  These flags signal the creation of a meta parameter at the command
  line.  f,i, and s signal the creation of a float, integer, or string
  meta parameter that will be written to the data file header.
     $ lcrun -f height=5.25 -i temperature=22 -s day=Monday

GPLv3
(c)2017-2022 C.Martin
```

### lcburst

The **L**aboratory **C**onfiguration **BURST** utility is intended for high speed streaming for a known duration.  The command-line options can be used to override the configured `nsample`, and otherwise, `nsample` determines the number of samples per channel that should be collected.

```bash
$ lcburst -h
lcburst [-h] [-c CONFIGFILE] [-n SAMPLES] [-t DURATION] [-d DATAFILE]
     [-f|i|s param=value]
  Runs a single high-speed burst data colleciton operation. Data are
  streamed directly into ram and then saved to a file after collection
  is complete.  This allows higher data rates than streaming to the hard
  drive.

-c CONFIGFILE
  Specifies the LCONFIG configuration file to be used to configure the
  LabJack.  By default, LCBURST will look for lcburst.conf

-d DATAFILE
  Specifies the data file to output.  This overrides the default, which is
  constructed from the current date and time: "YYYYMMDDHHmmSS_lcburst.dat"

-f param=value
-i param=value
-s param=value
  These flags signal the creation of a meta parameter at the command
  line.  f,i, and s signal the creation of a float, integer, or string
  meta parameter that will be written to the data file header.
     $ LCBURST -f height=5.25 -i temperature=22 -s day=Monday

-n SAMPLES
  Specifies the integer number of samples per channel desired.  This is
  treated as a minimum, since LCBURST will collect samples in packets
  of LCONF_SAMPLES_PER_READ (64) per channel.  LCONFIG will collect the
  number of packets required to collect at least this many samples.

  For example, the following is true
    $ lcburst -n 32   # collects 64 samples per channel
    $ lcburst -n 64   # collects 64 samples per channel
    $ lcburst -n 65   # collects 128 samples per channel
    $ lcburst -n 190  # collects 192 samples per channel

  Suffixes M (for mega or million) and K or k (for kilo or thousand)
  are recognized.
    $ lcburst -n 12k   # requests 12000 samples per channel

  If both the test duration and the number of samples are specified,
  which ever results in the longest test will be used.  If neither is
  specified, then LCBURST will collect one packet worth of data.

-t DURATION
  Specifies the test duration with an integer.  By default, DURATION
  should be in seconds.
    $ lcburst -t 10   # configures a 10 second test

  Short or long test durations can be specified by a unit suffix: m for
  milliseconds, M for minutes, and H for hours.  s for seconds is also
  recognized.
    $ lcburst -t 500m  # configures a 0.5 second test
    $ lcburst -t 1M    # configures a 60 second test
    $ lcburst -t 1H    # configures a 3600 second test

  If both the test duration and the number of samples are specified,
  which ever results in the longest test will be used.  If neither is
  specified, then LCBURST will collect one packet worth of data.

GPLv3
(c)2017-2022 C.Martin

```

### lcstat

The **L**aboratory **C**onfiguration **STAT**us utility is intended to provide a quick and convenient real time display of signal statistics for the configured device.  This can be handy for monitoring an experiment or just for debugging.

`lcstat` uses the `nsample` parameter to determine the number of samples used for averaging.

```bash
$ lcstat -h
lcstat [-dhmpr] [-c CONFIGFILE] [-n SAMPLES] [-u UPDATE_SEC]
  LCSTAT is a utility that shows the status of the configured channels
  in real time.  The intent is that it be used to aid with debugging and
  setup of experiments from the command line.

  Measurement results are displayed in a table with a row for each
  analog input and DIO extended feature channel configured and columns for
  signal statistics, specified with switches at the command line.
  Measurements are streamed for at least the number of samples specified
  by the NSAMPLE configuration parameter or by the number specified by
  the -n option.

-c CONFIGFILE
  Specifies the LCONFIG configuration file to be used to configure the
  LabJack.  By default, LCSTAT will look for lcstat.conf

-n SAMPLES
  Specifies the minimum integer number of samples per channel to be 
  included in the statistics on each channel.  

  For example, the following is true
    $ lcburst -n 32   # collects 64 samples per channel
    $ lcburst -n 64   # collects 64 samples per channel
    $ lcburst -n 65   # collects 128 samples per channel
    $ lcburst -n 190  # collects 192 samples per channel

  Suffixes M (for mega or million) and K or k (for kilo or thousand)
  are recognized.
    $ lcburst -n 12k   # requests 12000 samples per channel

  If both the test duration and the number of samples are specified,
  which ever results in the longest test will be used.  If neither is
  specified, then LCSTAT will collect one packet worth of data.

-d
  Display standard deviation of the signal in the results table.

-m
  Display the maximum and minimum of each signal in the results table.

-p
  Display peak-to-peak values in the results table.

-r
  Display rms values in the results table.

-u UPDATE_SEC
  Accepts a floating point indicating the approximate time in seconds between
  display updates.

GPLv3
(c)2020-2022 C.Martin

```
