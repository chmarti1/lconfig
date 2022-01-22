## <a name="top"></a> Documentation

Version 4.06  
January 2022  
Christopher R. Martin  


## <a name="intro"></a> Introduction
The **L**aboratory **CONFIG**uration system is organized into top-level binaries (`lcrun`, `lcburst`, and `lcstat`), which get most data acquisition jobs done. For custom applications, the core header (`lconfig.h`), its corresponding c-file (`lconfig.c`), supporting headers (`lmap.h` and `lctools.h`), and their c-files (`lmap.c` and `lctools.c`), provide an API for automatically configuring the LabJack and recording data files.

Most jobs can be done by the [Thebinaries](bin.md), so users should probably start there.  Detailed documentation for the [LConfig API](api.md) describes the system functions and their use.  There is also a [Reference](reference.md) for a complete list of funcitons, constants, and the available configuration directives.  Finally, the `lconfig.h` header is commented with detailed documentation and a changelog.  If these sources ever contradict one another, the header file should be interpreted as the authoritative resource.

The entire lconfig system is based on the idea that the entire data acquisition process is spelled out in a configuration file, which is read in by the binary performing the job.  The final output is usually a data file (or data files) that include the entire configuration, some meta data to describe the experiment, and the measurements themselves.

### To get started

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
- The LCTools API (docs coming soon: see lctools.h for documentation)
- [Compiling](compiling.md)
- [Reference](reference.md)

