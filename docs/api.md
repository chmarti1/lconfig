[back to Documentation](documentation.md)

Version 4.04<br>
October 2020<br>
Chris Martin<br>

##  <a name='top'></a> The LConfig API
If you want to use the LConfig system, but you don't just want to use the canned lcrun, lcburst, or lcstat codes, you'll need to know the LConfig API.  The API is exposed through three headers that provide hooks into their respective object files.

Follow these links for detailed documentation on each:
[Introduction](#intro)
- [lconfig.h](lconfig_h.md) The core LConfig API
- [lctools.h](lctools_h.md) High level data and UI tools
- [lcmap.h](#lcmap_h.md) Tools for configuration and message mapping

## <a name=intro></a>Introduction
It is important to remember that LConfig is just a layer built on top of LabJack's interface to automate the device configuration, to handle software triggering, and to handle data buffering.

The entire system is broken into three object files: `lconfig.o`, `lctools.o`, and `lcmap.o`, which provide the most powerful binary tools for compiled codes using the `lconfig` system.  Their functions are exposed by their respective headers: `lconfig.h`, `lctools.h`, and `lcmap.h`.

Compiling a binary that uses the `lconfig.h` header will need to be linked with the `lconfig.o` object file.

```bash
# The lconfig.o is generated in the lconfig makefile 
# with a line like this:
$ gcc -c lconfig.c -o lconfig.o
# Then, it can be used in binaries by compiling like this
$ gcc lconfig.o mycode.c -o mybinary
```
