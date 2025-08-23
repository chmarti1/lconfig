## <a name="top"></a> Documentation

Version 5.00  
August 2025  
Christopher R. Martin  


## <a name="intro"></a> Introduction
The **L**aboratory **CONFIG**uration system is a LabJack data acquisition system management suite for Linux.  It provides top-level binaries:  
- `lcrun`: Collect data for an extended period and stop on a keystroke.  
- `lcburst`: Collect a prescribed amount of high speed data.  
- `lcstat`: Provide a real-time display of calibrated sensor readings.  

These are built on back-end object files that can be used to build custom applications.  The primary header (`lconfig.h`), its corresponding c-file (`lconfig.c`), supporting headers (`lmap.h` and `lctools.h`), and their c-files (`lmap.c` and `lctools.c`), provide an API for automatically configuring the LabJack and recording data files.  

LConfig uses ASCII configuration files to simultaneously control and record how data were collected in an experiment.  The idea is that experimental parameters need to be archived in the same file as the data itself.  This includes calibrations, channel assignments, analog settings (like gain and settling time), and meta data which can describe anything not directly related to the DAQ.

### Using the binaries

- [Compiling and installation](compiling.md)  
- [The binaries](bin.md)  
- [Writing configuration files](config.md)  
- [Data files](data.md)  
- [Reference](reference.md)  

### Using LConfig data

- [Data files](data.md)  
- [Post processing](post.md)  

### Writing your own binaries

- [The LConfig API](api.md)  
	- [lconfig.h](lconfig_h.md)  
	- [lctools.h](lctools_h.md)  
	- [lcmap.h](lcmap_h.md)  
- The LCTools API (docs coming soon: see lctools.h for documentation)  
- [Compiling](compiling.md)  
- [Reference](reference.md)  

