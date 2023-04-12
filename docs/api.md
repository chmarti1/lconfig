[back to Documentation](documentation.md)

Version 4.08  
April 2023  
Christopher R. Martin  

##  <a name='top'></a> The LConfig API
If you want to use the LConfig system, but you don't just want to use the canned `lcrun`, `lcburst`, or `lcstat` codes, you'll need to know the LConfig API.  The API is exposed through three headers that provide hooks into their respective object files.

Follow these links for detailed documentation on each:
[Introduction](#intro)
- [lconfig.h](lconfig_h.md) The core LConfig API
- [lctools.h](lctools_h.md) High level data and UI tools
- [lcmap.h](#lcmap_h.md) Tools for configuration and message mapping
The authoritative documentation for each element of the interface is ALWAYS found in the comments of each header file, while these doc files are intended to help users understand the top-level design of the lconfig system.

## <a name=intro></a>Introduction
It is important to remember that LConfig is just a layer built on top of LabJack's interface to automate the device configuration, calibration, triggering, data buffering, and data management on the local machine.

The back-end API is broken into three files: `lconfig.c`, `lctools.c`, and `lcmap.c`, that are compiled into their corresponding `*.o` object files.  These comprise what we are calling the "lconfig system."  Their functions are exposed by their respective headers: `lconfig.h`, `lctools.h`, and `lcmap.h`.

The `lconfig.h` header includes the functions that most users will ever need to write their application that interacts with the lconfig configuration system.  It is documented in [lconfig_h.md](lconfig_h.md) and with extensive comments in the lconfig header itself.

The `lctools.h` header provides some handy tools for interacting with data and generating rudimentary UIs with terminals (without importing some other library).  It is primarily written to assist with the `lcstat` binary, but it's useful enough that users may find their own uses for it.  It is documented in [lctools_h.md](lctools_h.md) and with comments in the header itself.

Finally, the `lcmap.h` header provides data structures that map enumerated configuration parameters from their string values in configuration files to their enumerated values.  Many users may not need these tools, but they are extremely helpful to the internals of `lconfig.c`.

