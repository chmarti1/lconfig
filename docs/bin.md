## Binaries

There are two binaries included with LConfig: `drun` and `dburst`.  These are inteded to be sufficiently generic to do most data acquisition tasks.  `drun` is intended for long slow sample-rate tests where the test duration may not be known in advance.  `dburst` is intended for short high-speed tests.

On a Linux system, following the steps in [Compiling](compiling.bin), the binaries will be installed in `/usr/local/bin`.  For help text, use

```bash
$ drun -h
... prints help text ...
$ dburst -h
... prints help text ...
```
If you're using a terminal window and want to be able to scoll comfortably, use
```bash
$ drun -h | less
$ dburst -h | less
```

### drun

The **D**ata **RUN** utility, `drun`, streams data directly to a [data file](data.md).  This means it is well suited to long tests that involve big data sets, but the maximum data rate will be limited by the write speed to the hard drive.  For high-speed applications, look at `dburst`.

`drun` ignores the `nsample` parameter.

```bash
$ drun [-h] [-r PREFILE] [-p POSTFILE] [-d DATAFILE] [-c CONFIGFILE] 
     [-f|i|s param=value]
  Runs a data acquisition job until the user exists with a keystroke.

PRE and POST data collection
  When DRUN first starts, it loads the selected configuration file
  (see -c option).  If only one device is found, it will be used for
  the continuous DAQ job.  However, if multiple device configurations
  are found, they will be used to take bursts of data before (pre-)
  after (post-) the continuous DAQ job.

  If two devices are found, the first device will be used for a single
  burst of data acquisition prior to starting the continuous data 
  collection with the second device.  The NSAMPLE parameter is useful
  for setting the size of these bursts.

  If three devices are found, the first and second devices will still
  be used for the pre-continous and continuous data sets, and the third
  will be used for a post-continuous data set.

  In all cases, each data set will be written to its own file so that
  DRUN creates one to three files each time it is run.

-c CONFIGFILE
  By default, DRUN will look for "drun.conf" in the working
  directory.  This should be an LCONFIG configuration file for the
  LabJackT7 containing no more than three device configurations.
  The -c option overrides that default.
     $ drun -c myconfig.conf

-d DATAFILE
  This option overrides the default continuous data file name
  "drun.dat"
     $ drun -d mydatafile.dat

-r PREFILE
  This option overrides the default pre-continuous data file name
  "drun_pre.dat"
     $ drun -r myprefile.dat

-p POSTFILE
  This option overrides the default post-continuous data file name
  "drun_post.dat"
     $ drun -p mypostfile.dat

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
     $ drun -f height=5.25 -i temperature=22 -s day=Monday

GPLv3
(c)2017 C.Martin

```

### dburst

The **D**ata **BURST** utility is intended for high speed streaming for a known duration.  The command-line options can be used to override the configured `nsample`, and otherwise, `nsample` determines the number of samples per channel that should be collected.

```bash
dburst [-h] [-c CONFIGFILE] [-n SAMPLES] [-t DURATION] [-d DATAFILE]
     [-f|i|s param=value]
  Runs a single high-speed burst data colleciton operation. Data are
  streamed directly into ram and then saved to a file after collection
  is complete.  This allows higher data rates than streaming to the hard
  drive.

-c CONFIGFILE
  Specifies the LCONFIG configuration file to be used to configure the
  LabJack T7.  By default, DBURST will look for dburst.conf

-f param=value
-i param=value
-s param=value
  These flags signal the creation of a meta parameter at the command
  line.  f,i, and s signal the creation of a float, integer, or string
  meta parameter that will be written to the data file header.
     $ DBURST -f height=5.25 -i temperature=22 -s day=Monday

-n SAMPLES
  Specifies the integer number of samples per channel desired.  This is
  treated as a minimum, since DBURST will collect samples in packets
  of LCONF_SAMPLES_PER_READ (64) per channel.  LCONFIG will collect the
  number of packets required to collect at least this many samples.

  For example, the following is true
    $ dburst -n 32   # collects 64 samples per channel
    $ dburst -n 64   # collects 64 samples per channel
    $ dburst -n 65   # collects 128 samples per channel
    $ dburst -n 190  # collects 192 samples per channel

  Suffixes M (for mega or million) and K or k (for kilo or thousand)
  are recognized.
    $ dburst -n 12k   # requests 12000 samples per channel

  If both the test duration and the number of samples are specified,
  which ever results in the longest test will be used.  If neither is
  specified, then DBURST will collect one packet worth of data.

-t DURATION
  Specifies the test duration with an integer.  By default, DURATION
  should be in seconds.
    $ dburst -t 10   # configures a 10 second test

  Short or long test durations can be specified by a unit suffix: m for
  milliseconds, M for minutes, and H for hours.  s for seconds is also
  recognized.
    $ dburst -t 500m  # configures a 0.5 second test
    $ dburst -t 1M    # configures a 60 second test
    $ dburst -t 1H    # configures a 3600 second test

  If both the test duration and the number of samples are specified,
  which ever results in the longest test will be used.  If neither is
  specified, then DBURST will collect one packet worth of data.

GPLv3
(c)2017 C.Martin
```

