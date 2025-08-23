[back to Documentation](documentation.md)

Version 5.00  
August 2025  
Chris Martin  

## <a name="compiling"></a> Compiling

The LConfig system is written in C using LabJack's LabJackM interface, and is written for a POSIX environment (Linux/Unix).  To obtain the latest version of LConfig

```bash
$ git clone http://githuh.com/chmarti1/lconfig
```

If you already have a working copy of LConfig, you can upgrade it to the latest version using
```bash
$ cd /path/to/lconfig
$ git pull origin master
```

To compile in a Debian-based system, first be certain that the GNU C Compiler is installed.

```bash
$ sudo apt update
$ sudo apt install gcc build-essential
```

Next, be certain that the [LabJackM](https://labjack.com/support/software/installers/ljm) driver is installed.  Download the appropriate release for your system and follow the instructions.

To compile the built-in `lcrun` and `lcburst` binaries and automatically place them on your system,
```bash
$ cd /path/to/lconfig
$ sudo make install
```

The output of the build process is stored in the `build` sub-directory.  This includes the following:  
- `build/lcrun.bin`  
- `build/lcburst.bin`  
- `build/lcstat.bin`  
- `build/lconfig.o`  
- `build/lctools.o`  
- `build/lcmap.o`  

These binaries and object files can be destroyed by
```bash
$ make clean
```
For users that would like to test these binaries prior to installation,
```bash
$ make binaries
```
will create the `build` directory and compile the binaries, but it will not install them in the system.  To compile from scratch, 
```bash
$ make clean
```
removes the `build` directory and its contents.  

### Installation and uninstallation

The `make install` command finishes by placing files in `/usr/local/bin`, and forcing their `chmod` permissions to 755.  To change this behavior, edit the `TODIR` and `BIN_CHMOD` variables in `makefile`.  

The `make uninstall` command uses the same `TODIR` variable to locate prior installed binaries and remove them.  

### Compiler commands

Applications that use the LConfig system should include the `lconfig.h` header.  There are also tools for interacting with data and for building simple terminal interfaces in the `lctools.h` header.  Somewhere at the top of your c-file, the line below should appear.  

```C
#include "lconfig.h"
#include "lctools.h"
```

In the current LConfig design, the configuration system is NOT installed as a system library.  You should simply copy `lconfig.h`, `lconfig.c`, `lctools.h`, `lctools.c`, `lcmap.h`, and `lcmap.c` into your project.  This design may change in the future, but for now it is quite functional.  The biggest advantage of this approach is that I have utilities that were written years ago on earlier versions of LConfig that will not be broken by pushing an update to my system libraries.  Instead, each can be built on its own version of LConfig.  If it's a utility I use regularly, it will make sense to update it.  That seems messy, but it's really quite functional for most of us that do regular laboratory work.

When you are compiling your applicaiton, you will need to compile the `lconfig.o` and `lcmap.o` object files (and `lctools.o` if you are using `lctools.h`).  Then, you will need to link your executable with `lconfig.o`, `lcmap.o`, (`lctools.o`), the LJM library, and probably the c-math library.  Take a look at the makefile for how the LConfig project does this for `lcrun`, `lcburst`, and `lcstat`.  For a user application called `mycode.c` producing an executable `mybinary`, the following compilation should work.
```
$ gcc -c lconfig.c -o lconfig.o
$ gcc -c lctools.c -o lctools.o
$ gcc -Wall -lLabJackM -lm lconfig.o lctools.o mycode.c -o mybinary
```
The `-Wall` flag is good practice for development, but can be removed once you are confident in your code.

To learn how to implement LConfig, read the docs on the [api](api.md), take a peak at the [reference](reference.md), and when in doubt, the `lconfig.h` and `lctools.h` headers are thoroughly commented to serve as their own documentation.

[top](#compiling)
