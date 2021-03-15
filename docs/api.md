[back to Documentation](documentation.md)

Version 4.05  
March 2021  
Christopher R. Martin  

##  <a name='top'></a> The LConfig API
If you want to use the LConfig system, but you don't just want to use the canned `lcrun`, `lcburst`, or `lcstat` codes, you'll need to know the LConfig API.  The API is exposed through three headers that provide hooks into their respective object files.

Follow these links for detailed documentation on each:
[Introduction](#intro)
- [lconfig.h](lconfig_h.md) The core LConfig API
- [lctools.h](lctools_h.md) High level data and UI tools
- [lcmap.h](#lcmap_h.md) Tools for configuration and message mapping

## <a name=intro></a>Introduction
It is important to remember that LConfig is just a layer built on top of LabJack's interface to automate the device configuration, calibration, triggering, data buffering, and data management on the local machine.

The back-end API is broken into three files: `lconfig.c`, `lctools.c`, and `lcmap.c`, that are compiled into their corresponding `*.o` object files.  These comprise what we are calling the "lconfig system."  Their functions are exposed by their respective headers: `lconfig.h`, `lctools.h`, and `lcmap.h`.

The headers include 