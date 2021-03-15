## <a name="top"></a> Documentation

Version 4.05  
March 2021  
Christopher R. Martin  


## <a name="intro"></a> Introduction
The **L**aboratory **CONFIG**uration system is organized into top-level binaries (`lcrun`, `lcburst`, and `lcstat`), the core header (`lconfig.h`), its corresponding c-file (`lconfig.c`), supporting headers (`lmap.h` and `lctools.h`), and their c-files (`lmap.c` and `lctools.c`).

Most jobs can be done by the [Thebinaries](bin.md), so users should probably start there.  Detailed documentation for the [LConfig API](api.md) describes the system functions and their use.  There is also a [Reference](reference.md) for a complete list of funcitons, constants, and the available configuration directives.  Finally, the `lconfig.h` header is commented with detailed documentation and a changelog.  If these sources ever contradict one another, the header file should be interpreted as the authoritative resource.


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

