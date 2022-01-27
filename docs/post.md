[back](documentation.md)

Version 4.06<br>
January 2022<br>
Chris Martin<br>

## Post processing

The LConfig package includes a post-processing suite written in Python in the `py` directory.  This is a package with tools for loading LConfig data files, plotting, calibrating, and manipulating the data.  For full functionality, the NumPy and Matplotlib packages must be installed.  If you are using the Python package index (pip), 
```bash
$ sudo -H pip install --upgrade numpy
$ sudo -H pip install --upgrade matplotlib
```

If you are not using pip, or if you have difficulty getting pip to work correctly, there are a number of options.  The simplest is to try your system's native package manager.
```bash
$ sudo apt install python-numpy python-matplotlib
```



To get started, load the `lconfig` python package.
```bash
$ cd /path/to/lconfig/py
$ python
```
As of version 4.06, the python interface has been heavily re-designed to take better advantage of Python's inate object oriented design.  I constantly update the post-processing to do the jobs I need in my research.  Rather than trying to keep this documentation up-to-date, I document its behavior using Python's in-line help system.

To get started, use
```python
import lconfig as lc
help(lc)
```
