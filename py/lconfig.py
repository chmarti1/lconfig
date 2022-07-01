"""lconfig.py

The Laboratory CONFIGuration system post-processing tools for python.
For more information on this c-language-based toolset for data 
acquisition in Linux using LabJack hardware, see the github project
page:
    https://github.com/chmarti1/lconfig
    
Once you create a datafile using the binaries or with lc_datafile_init()
and lc_datafile_write(), you will probably want to do something with 
the data.  You COULD open the whitespace separated variable files with 
excel, but why volunteer for pain and suffering?

The load() function is the primary tool for opening configuration and
data files.

>>> c = load('/path/to/file.conf')

By default 

"""

import os, sys
import numpy as np
import json
import matplotlib.pyplot as plt
import struct
import time

__version__ = '4.06'






# Helper funcitons
def _read_param(ff):
    """Read in a single word
word = read_pair(ff)
"""
    # This algorithm very closely mirrors the one used by LCONFIG
    ws = ' \t\n\r'
    
    LCONF_MAX_STR = 80
    
    param = ''
    quote = False
    
    # so long as the file is not empty, keep reading
    # Read in text one character at a time
    charin = ff.read(1).decode('utf-8')
    while charin and len(param) < LCONF_MAX_STR-1:
        # Toggle the quote state
        if charin == '"':
            if quote:
                quote = False
                break
            else:
                quote = True
        # If this is a comment
        elif not quote and charin == '#':
            # Kill off the rest of the line
            charin = ff.read(1).decode('utf-8')
            # If the next character is another #, then we're done 
            # with the configuration part of the file
            if charin == '#':
                # Rewind by two characters so the read param algorithm
                # will stick on the ## character combination
                ff.seek(-2,1)
                return param
            # Kill off the remainder of the line
            while charin and charin!='\n':
                charin = ff.read(1).decode('utf-8')
            # If the parameter is not empty, the # was in the middle
            # of a word.  Go ahead and return that word.
            if(param):
                return param
        elif charin in ws:
            if quote:
                param = param + charin
            elif param:
                return param
        elif quote:
            param = param + charin
        else:
            param = param + charin.lower()
        charin = ff.read(1).decode('utf-8')
    return param


class LEnum:
    """Enumerated value class
    
LE = LEnum(strings, values=None, state=0)

The enumerated class defines a discrete set of legal states defined by
a list of strings.  Each string is intended to "name" the state.  The
LEnum class remembers the current state by the index in the string list.
Optionally, a set of values can be specified to indicate integer values
corresponding to each of the states.  If values are not specified, then
the values will be treated as equal to the index in the strings list.

This class is intended to mimick the behavior of a C-style enum type.

LE.set(value)
LE.set('state')
    The set method sets the enumerated state by name or by value 
    depending on whether an integer or string is found.
    
LE.setstate(state)
    The setstate method sets the enumerated state directly rather than
    using the integer value or string name.  This is the index that 
    should be an index in the strings and values lists.
    
LE.get()
    Return the string value for the current state

LE.getvalue()
    Return the integer value for the current state
    
LE.getstate()
    Return the current state
    
LE2 = LEnum(LE)
    Builds a new enumerated instance using LE as a prototype.  The values
    and strings will NOT be copied, but are instead accessed by reference.
"""
    def __init__(self, strings, values=None, state=0):
        # If this is a copy operation
        if isinstance(strings, LEnum):
            self._values = strings._values
            self._state = strings._state
            self._strings = strings._strings
            return
        
        self._strings = list(strings)
        self._values = None
        
        if not self._strings:
            raise Exception('LEnum __INIT__: The strings list cannot be empty.')
        for this in self._strings:
            if not isinstance(this, str):
                raise Exception('LEnum __INIT__: Found a state name that was not a string: %s'%repr(this))
                
        if values:
            self._values = list(values)
            for this in self._values:
                if not isinstance(this, int):
                    raise Exception('LEnum __INIT__: Found a state value that was not an integer: %s'%repr(this))
            if len(self._values) != len(self._strings):
                raise Exception('LEnum __INIT__: The values and strings lists MUST be the same length.')

        self._state = state
        
    def __str__(self):
        return self.get()
        
    def __repr__(self):
        out = '{'
        for ind in range(len(self._strings)):
            if ind == self._state:
                out += '('
            if self._values:
                out += '%s:%d'%(
                        self._strings[ind], 
                        self._values[ind])
            else:
                out += self._strings[ind]
            if ind == self._state:
                out += ')'
            if ind < len(self._strings)-1:
                out += ', '
        out += '}'
        return out

    def get(self):
        """return the string name of the current state"""
        return self._strings[self._state]
        
    def getvalue(self):
        if self._values:
            return self._values[self._state]
        return self._state
        
    def getstate(self):
        return self._state
        
    def set(self,ind):
        # If this is a string
        if isinstance(ind,str):
            # Is the string one of the enumerated values?
            if ind in self._strings:
                self._state = self._strings.index(ind)
                return
            # Can the string be converted into an integer?
            else:
                try:
                    self.set(int(ind))
                    return
                except:
                    raise Exception('LEnum set: Value not recognized: ' + ind)
        # If this is an integer value
        elif isinstance(ind, int):
            if ind in self._values:
                self._state = self._values.index(ind)
                return
            else:
                raise Exception('LEnum set: Integer value not recongized: ' + str(ind))
            
        raise Exception('LEnum set: Unsupported value type: ' + str(ind))
        
        
    def setstate(self,ind):
        if ind < len(self._strings) and ind>=0:
            self._state = ind
            return
        raise Exception('LE setstate: State is out-of-range: %d'%ind)


class Conf(object):
    """Configuration object prototype"""
    def __setattr__(self,name,value):
        if name not in self.__dict__:
            raise Exception('Unsupported configuration parameter: ' + name)
        thistype = type(self.__dict__[name])
        if thistype is list or thistype is dict:
            raise Exception('Cannot overwrite parameter set: ' + name)
        elif thistype is LEnum:
            self.__dict__[name].set(value)
        else:
            self.__dict__[name] = thistype(value)

class DevConf(Conf):
    """DevConf class

The DevConf class manages device configuration parameters with class
members that roughly parallel the lc_devconf_t struct:

They are:
    connection      LEnum   Type of connection
    serial          str     Device serial number
    device          LEnum   Device type: t4, t7, etc...
    dataformat      LEnum   Binary, ASCII, etc...
    name            str     Device name
    ip              str     Device IP address
    gateway         str     Default gateway IP address
    subnet          str     Subnet mask
    samplehz        float   Sample rate in hz
    settleus        float   Settling time in microseconds
    nsample         int     Number of samples per measurement burst
    distream        int     Digital input stream mask
    domask          int     Digital output mask
    dovalue         int     Digital output value
    trigchannel     int     Analog input channel for software triggering
    triglevel       float   Voltage threshold for the software trigger
    trigpre         int     Pre-trigger samples
    trigedge        LEnum   Edge for trigger: rising, falling, any
    effrequency     float   Extended feature frequency

For the various input/output channels, there are lists that contain 
their configuration instances:
    aich[]          AiConf
    aoch[]          AoConf
    efch[]          EfConf
    comch[]         ComConf
    
Finally, there is a dictionary that contains the meta parameters found
    meta_values[]
    
This should not be confused with the "meta" member, which merely holds 
the last meta mode configured in the configuration file.

There are also methods to help with high-level jobs:
    nistream()      Returns the number of input stream channels
    get_meta()      Acts like a dictionary's get() method
    
A __str()__ method is included so print() calls on a DevConf instance 
produce a detailed printout of the parameters.
"""
    def __init__(self):
        self.__dict__.update({
            'connection':LEnum(['any', 'usb', 'eth', 'ethernet'], values=[0,1,3,3]),
            'serial':'',
            'device':LEnum(['any', 't4', 't7', 'tx', 'digit'], values=[0, 4, 7, 84, 200]),
            'dataformat':LEnum(['ascii','text','bin','binary'], values=[0,0,1,1]),
            'name':'',
            'ip':'',
            'gateway':'',
            'subnet':'',
            'samplehz':-1.,
            'settleus':1.,
            'nsample':64,
            'distream':0,
            'domask':0,
            'dovalue':0,
            'trigchannel':-1,
            'triglevel':0.,
            'trigpre':0,
            'trigedge':LEnum(['rising', 'falling', 'all'], state=0),
            'effrequency':0,
            'aich':[],
            'aoch':[],
            'efch':[],
            'comch':[],
            'meta':LEnum(['end','stop','none','int','integer','flt','float','str','string'],[0,0,0,1,1,2,2,3,3]),
            'meta_values':{},
        })

    def __setattr__(self,name,value):
        if name not in self.__dict__:
            if name.startswith('int:'):
                self.meta_values[name[4:]] = int(value)
            elif name.startswith('flt:'):
                self.meta_values[name[4:]] = float(value)
            elif name.startswith('str:'):
                self.meta_values[name[4:]] = str(value)
            elif self.meta.getvalue() == 1:
                self.meta_values[name] = int(value)
            elif self.meta.getvalue() == 2:
                self.meta_values[name] = float(value)
            elif self.meta.getvalue() == 3:
                self.meta_values[name] = str(value)
            elif name == 'aichannel':
                self.aich.append(AiConf())
                setattr(self.aich[-1], name, value)
            elif name.startswith('ai'):
                setattr(self.aich[-1], name, value)
            elif name == 'aochannel':
                self.aoch.append(AoConf())
                setattr(self.aoch[-1], name, value)
            elif name.startswith('ao'):
                setattr(self.aoch[-1], name, value)
            elif name == 'efchannel':
                self.efch.append(EfConf())
                setattr(self.efch[-1], name, value)
            elif name.startswith('ef'):
                setattr(self.efch[-1], name, value)
            elif name.startswith('do'):
                channel = int(name[2:])
                self.__dict__['domask'] |= 1<<channel
                if value:
                    self.__dict__['dovalue'] |= 1<<channel
                else:
                    self.__dict__['dovalue'] &= ~(1<<channel)
            else:
                raise Exception('Unsupported configuration parameter: ' + name)
        else:
            thistype = type(self.__dict__[name])
            if thistype is list or thistype is dict:
                raise Exception('Cannot overwrite parameter set: ' + name)
            elif thistype is LEnum:
                self.__dict__[name].set(value)
            else:
                self.__dict__[name] = thistype(value)

    def __str__(self, width=80):

        fmt = '{:>14s} : {:<14s}\n'
        out ='::Global Settings::\n'
        out += fmt.format('connection', self.connection.get())
        out += fmt.format('device', self.device.get())
        for attr in ['name', 'serial', 'ip', 'subnet', 'gateway', 
                'dataformat', 'samplehz', 'settleus', 'nsample', 'distream',
                'domask', 'dovalue']:
            
            value = getattr(self, attr)
            if isinstance(value, LEnum):
                value = value.get()
            elif isinstance(value, (int,float)):
                value = str(value)
            if value:
                out += fmt.format(attr,value)
            
        # Detect trigger settings?
        if self.trigchannel>0:
            out += '::Trigger Settings::\n'
            out += fmt.format('trigchannel', str(self.trigchannel))
            out += fmt.format('trigedge', self.trigedge.get())
            out += fmt.format('trigpre', str(self.trigpre))
            out += fmt.format('triglevel', str(self.triglevel))
        
        if self.meta_values:
            out += '::Meta Data::\n'
            for param,value in self.meta_values.items():
                fmt.format(param,value)
        
        if self.aich:
            out += '::Analog Inputs::\n'
            for ii,aich in enumerate(self.aich):
                out += 'Channel [%d]\n'%(ii)
                out += str(aich)
        
        if self.aoch:
            out += '::Analog Outputs::\n'
            for ii,aoch in enumerate(self.aoch):
                out += 'Channel [%d]\n'%(ii)
                out += str(aoch)
            
        if self.efch:
            out += '::Digital Extended Feature Channels::\n'
            for ii,efch in enumerate(self.efch):
                out += 'Channel [%d]\n'%(ii)
                out += str(efch)
                
        if self.comch:
            out += '::Digital Com Channels::\n'
            for ii,comch in enumerate(self.comch):
                out += 'Channel [%d]\n'%(ii)
                out += str(comch)
        return out
        
    def nistream(self):
        """Returns the number of configured input stream channels"""
        return len(self.aich) + (self.distream != 0)

    def get_meta(self, param):
        """Return a meta parameter or None if not found"""
        return self.meta_values.get(param)

        
class AiConf(Conf):
    """AiConf class

Analog Input channel CONFiguration objects have data members that mirror
the lc_aiconf_t struct.  

They are:
    aichannel       int     The analog input channel number
    ainegative      LEnum   differential or ground
    airange         float   10., 1., 0.1, or .01
    airesolution    int     The resolution index used by the T7
    aicalslope      float   meas = (v - aicalzero) * aicalslope
    aicalzero       float
    aicalunits      str     The units for the calibrated measurement
    ailabel         str     Human-readable text label for the channel
"""
    def __init__(self):
        self.__dict__.update({
            'aichannel':-1,
            'ainegative':LEnum(
                    ['differential', 'differential', 'differential', 'differential', 'differential', 'differential', 'differential', 'ground'], 
                    values=[1, 3, 5, 7, 9, 11, 13, 199], state=7),
            'airange':10.,
            'airesolution':0,
            'aicalslope':1.,
            'aicalzero':0.,
            'ailabel':'',
            'aicalunits':''
        })
        
    def __str__(self):
        fmt = '{:>14s} : {:<14s}\n'
        out = ''
        for param in ['aichannel', 'ainegative', 'airange', 'airesolution',
                'aicalslope', 'aicalzero', 'ailabel', 'aicalunits']:
            value = getattr(self,param)
            if isinstance(value, LEnum):
                value = value.get()
            elif isinstance(value, (int,float)):
                value = str(value)
            out += fmt.format(param,value)
        return out


class AoConf(Conf):
    """AoConf class

Analog Onput channel CONFiguration objects have data members that mirror
the lc_aoconf_t struct.  

They are:
    aochannel       int     The analog output channel number
    aosignal        LEnume  constant, sine, square, triangle, or noise
    aofrequency     float   The signal frequency
    aoamplitude     float   The amplitude of the signal 0.5*(max-min)
    aooffset        float   The common mode dc offset 0.5*(max+min)
    aoduty          float   Allows asymmetrical square and triangle waves
    aolabel         str     Human readable text label for the channel
"""
    def __init__(self):
        self.__dict__.update({
            'aochannel':-1,
            'aosignal':LEnum(['constant', 'sine', 'square', 'triangle', 'noise']),
            'aofrequency':-1.,
            'aoamplitude':1.,
            'aooffset':2.5,
            'aoduty':0.5,
            'aolabel':''
        })
        
    def __str__(self):
        fmt = '{:>14s} : {:<14s}\n'
        out = ''
        for param in ['aochannel', 'aosignal', 'aofrequency', 'aoamplitude',
                'aooffset', 'aoduty', 'aolabel']:
            value = getattr(self,param)
            if isinstance(value, LEnum):
                value = value.get()
            elif isinstance(value, (int,float)):
                value = str(value)
            out += fmt.format(param,value)
        return out


class EfConf(Conf):
    """EfConf class

digital Extended Feature channel CONFiguration objects have data members
that mirror the lc_efconf_t struct.  

They are:
    efchannel   int     The hardware digital channel
    efsignal    LEnum   pwm, count, pulse, frequency, phase, quadrature
    efedge      LEnum   rising, falling, all
    efdebounce  LEnum   none, fixed, reset, minimum
    efdirection LEnum   input, output
    efusec      float   Time measurement/value for the signal
    efdegrees   float   Phase measurement/value for the signal
    efduty      float   Duty cycle measurement/value for the signal
    efcount     int     Integer edge/pulse count for the signal
    eflabel     str     Human readable text label for the channel
"""
    def __init__(self):
        self.__dict__.update({
            'efchannel':-1,
            'efsignal':LEnum(['pwm', 'count', 'pulse', 'frequency', 'phase', 'quadrature'], [0,1,1,2,3,4]),
            'efedge':LEnum(['rising', 'falling', 'all']),
            'efdebounce':LEnum(['none', 'fixed', 'reset', 'minimum']),
            'efdirection':LEnum(['input', 'output']),
            'efusec':0.,
            'efdegrees':0.,
            'efduty':0.5,
            'efcount':0,
            'eflabel':''
        })
        
    def __str__(self):
        fmt = '{:>14s} : {:<14s}\n'
        out = ''
        for param in ['efchanne', 'efsigna', 'efedge', 'efdebounce', 
                'efdirection', 'efusec', 'efduty', 'efcount', 'eflabel']:
            value = getattr(self,param)
            if isinstance(value, LEnum):
                value = value.get()
            elif isinstance(value, (int,float)):
                value = str(value)
            out += fmt.format(param,value)
        return out
        
class ComConf(Conf):
    """ComConf class

COMmunication channel CONFiguration objects have data members that mirror
the lc_comconf_t struct.  

They are:
    comchannel      LEnum   none, uart, 1wire, spi, i2c, sbus
    comrate         float   Communcation data rate
    comin           int     Byte received
    comout          int     Byte transmitted
    comclock        int     Specify the communcations clock source
    comoptions      str     mode-specific options string
    comlabel        str     Human-readable text label for the channel
"""
    def __init__(self):
        self.__dict__.update({
            'comchannel':LEnum(['none', 'uart', '1wire', 'spi', 'i2c', 'sbus']),
            'comrate':-1,
            'comin':-1,
            'comout':-1,
            'comclock':-1,
            'comoptions':'',
            'comlabel':''
        })

    def __str__(self):
        fmt = '{:>14s} : {:<14s}\n'
        out = ''
        for param in ['comchannel', 'comrate', 'comin', 'comout', 
                'comclock', 'comoptions', 'comlabel']:
            value = getattr(self,param)
            if isinstance(value, LEnum):
                value = value.get()
            elif isinstance(value, (int,float)):
                value = str(value)
            out += fmt.format(param,value)
        return out

class LData:
    """LData - The lconfig data class for python
    
The LData class is populated by the `load()` function when the `data` 
keyword is set to `True`.  It is initialized empty, but offers tools
for interacting with the data.


Class members
=======================
.data           data array, numpy array
Once populated, the data array is a 2D numpy array.  Each column 
corresponds to a channel, and each row is a sample.  When distreaming
is active, the digital channel is always last.  The analog channels
appear in the same order in which they were ligsted in the configuration.

.timestamp      Time when the data collection began
Once populated, the timestamp is a `time.time_struct` instance converted
from the timestamp embedded in the data file.

.cal            T/F has the calibration been applied?
When `cal` is `True`, it indicates that the channel calibrations have 
been applied to the data.

.config         The DevConf configuration for the data collection
This is the configuration instance that describes the device 
configuration under which the data were collected.


Class methods
==========================
--- Retrieving data ---
.get_index()        Finds a channel index in the order it was configured
.get_config()       Returns the configuration class for a channel
.get_channel()      Returns a 1D array of a channel's data
.time()             Returns a 1D array of times since collection started
.dbits()            Returns digital input stream channels
  --> See also "Interacting with data" below <--
--- Getting basic information ---
.nch()              How many channels are in the data?
.ndata()            How many samples are there in each channel?
--- Manipulating data ---
.apply_cal()        Applies the channel calibrations (only once)
.ds()               Produces a slice for selecting data segments by time
--- Detecting events ---
.event_filter()     Generic tool for detecting edge crossing events
.get_events()       Find threshold crossings in analog input data
.get_dievents()     Find high/low transitions in digital input streams
--- Plotting data ---
.show_channel()     Generates a plot of a single channel versus time

Interacting with data
==========================
In addition to the `get_channel()` method, `LData` instances can be 
indexed directly using [] notation.  This interface supports rerencing
channels by their labels and slicing.  Where the get_XXX methods deal
with class data channel-by-channel, the [] notation allows an array-like
interface that still supplorts referencing channels by their labels.

When only one index is present, it is interpreted as a channel specifier
>>> ldata_instance[0]
# returns a 1D array of the first channel's data
>>> ldata_instance[0:2]
# returns a 2D array with the first two channels' data
>>> ldata_instance['Ambient Temperature (K)']
# returns a 1D array of the channel with label 'Ambient Temperature (K)'

When two indices are present, the first indicates the sample index and
the second identifies the channel.  This also supports slicing.
>>> ldata_instance[13,'Battery Voltage']
# returns the measurement at time index 13 for the channel with label
# 'Battery Voltage'
>>> ldata_instance[2:25, 1]
# Returns a 1D array of measurements from time index 2 to 24 from the
# second channel (index 1).
"""
    def __init__(self, config, data, cal=True):
        self.data = None
        self.timestamp = None
        self.filename = ''
        self.cal = False
        self.config = None
        # Private members
        self._time = None
        self._dbits = None
        self._bylabel = {}
        self._byainum = {}
        
        self.config = config
        # format the data array
        self.data = np.array(data, dtype=float)
        # Check for correct shape
        nch = config.nistream()
        if self.data.shape[1] != nch:
            raise Exception('LData: %d channels configured, but %d channels found in data'%(nch, self.data.shape[1]))
        # Apply calibration
        if cal:
            self.apply_cal()
        # Establish the label and number maps
        for ii,aiconf in enumerate(self.config.aich):
            if aiconf.ailabel:
                self._bylabel[aiconf.ailabel] = ii
            self._byainum[aiconf.aichannel] = ii
        # Add digital input stream if present
        if config.distream:
            self._bylabel['distream'] = nch-1


    def __str__(self):
        out = '<LData %d samples x %d channels>'%(self.data.shape[0], self.data.shape[1])

    def __getitem__(self, varg):
        N = len(varg)
        ch = None
        index = slice(0,-1)
        if N == 0:
            return self.data
        elif N==1:
            ch = varg[0]
        elif N == 2:
            index = varg[0]
            ch = varg[1]
        else:
            raise IndexError('LData: only 1 or 2 indices allowed.')
        
        # If it is a string, resolve the channel label
        if isinstance(ch,str):
            temp = self._bylabel.get(ch)
            if temp is None:
                raise IndexError('LData: Unrecognzied channel label: ' + ch)
            ch = temp
            
        return self.data[index,ch]
        
    def __len__(self):
        return self.data.shape[0]
        
    def ndata(self):
        """Returns the number of samples in the data set"""
        return self.data.shape[0]
        
    def nch(self):
        """Returns the number of channels in the data set"""
        return self.data.shape[1]
        
    def apply_cal(self):
        """apply_cal()  Applies calibrations to the data
    If the `cal` member is `False`, the `apply_cal()` method applies 
the appropriate calibration to each channel and sets `cal` to `True`.
"""
        if self.cal:
            return
        for ii,aich in enumerate(self.config.aich):
            self.data[:,ii] -= aich.aicalzero
            self.data[:,ii] *= aich.aicalslope
        self.cal = True


    def dbits(self, dich=None):
        """dbits()
    OR
dbits(dich)

Returns a 2D boolean array with sixteen columns corresponding to each of
the digital input stream bits.  When `dich` is an integer or a slice,
it is used to select which of the digital input channels to return in 
the array.  

If distream was not set, then this `dibits()` returns with an error.
"""
        if not self.config.distream:
            raise Exception('DBITS: The digital input stream was not configured for this data set.')
        
        # If the conversion hasn't already been performed, do it    
        if self._dbits is None:
            self._dbits = np.zeros((ndata,16), dtype=bool)
            for ii in range(ndata):
                for jj in range(16):
                    self._dbits[ii, jj] = bool(int(self.data[ii, nch-1]) & 1<<jj)
        
        if dich is None:
            return self._dbits
        else:
            return self._dbits[:,dich]


    def get_index(self, target=None, ainum=None):
        """get_index('channel label')
    OR
get_index(number)
    OR
get_index(ainum = number)

Returns the integer index corresponding to the column in the data 
corresponding to the channel indicated.  

:: Specifying a channel by its label ::
When the argument (or the `target` keyword) is set to a string, it is
interpreted as a channel label string.  If a channel is present with 
a matching label string, it will be returned.  Otherwise, `None` is 
returned.

:: Specifying a channel by its index ::
When the argument (or the `target` keyword) is set to an integer, it is
interpreted as the channel's index.  The channel index identifies it by
the order in which it was defined in the original configuration file.
If the digital input stream is present, it always appears last, 
regardless of where the `distream` directive appeared in the 
configuration file.

Negative indexes are transposed to be indexed from the last channel in
the list.  So, -1 indicates the last channel.  When nch is the number of
channels, the range of valid integer indices is [-nch, nch-1].

:: Specifying a channel by its hardware number ::
When the `ainum` keyword is used instead, the channel is identified by 
its corresponding positive hardware channel.  For example, a channel 
configured to accept `AIN2` as the positive input would be identified
by `ainum = 2`.  It would be strange to configure an analog input stream
with the same hardware channel appearing twice, but if that occurs, 
whichever is configured last will be returned.
"""
        ai = None
        nch = self.config.nistream()

        # If target was not specified
        if target is None:
            # The input must have been through ainum.
            if ainum is None:
                raise Exception('GET_AIINDEX: Missing mandatory argument.')
            ai = self._byainum.get(ainum)
            if ai is None:
                raise Exception('GET_AIINDEX: Analog input number %d not found.'%ainum)
        # If the target was specified
        else:
            # Make sure both weren't specified
            if ainum is not None:
                raise Exception('GET_AIINDEX: Accepts only one argument.')
            # If the channel is specified by label
            if isinstance(target, str):
                ai = self._bylabel.get(target)
                # If the channel was not found
                if ai is None:
                    raise Exception('GET_AIINDEX: Label not recognized: ' + target)
            # If the channel is specified by index
            elif isinstance(target, int):
                # Handle integer wrapping
                ai = target
                if target < 0:
                    ai += nch
                if ai < 0 or ai >= nch:
                    raise Exception('GET_AIINDEX: Index, %d, is out of range with %d channels.'%(target, nch)) 
        
        if ai is None:
            raise Exception('GET_CHANNEL: Unhandled exception!')
            
        return ai

    def get_channel(self, target=None, ainum=None):
        """get_channel( 'channel label' )
    OR
get_channel( number )
    OR
get_channel(ainum = number)

Returns a 1D array of samples for a single channel.  This is similar to
the functionality offered by the item interface [] notation, but it
includes the option to specify a channel by its hardware number.

See the get_index() for a detailed description of the arguments

See the class documentation for other operations that can be performed
with the item retrieval [] notation.
"""
        return self.data[:,self.get_aiindex(target=target, ainum=ainum)]
            
    
    def get_config(self, target=None, ainum=None):
        """get_config( 'channel label' )
    OR
get_config( number )
    OR
get_config(ainum = number)

Returns the analog input configuration instance for the input channel

See the get_index() for a detailed description of the arguments
"""
        ai = self.get_index(target=target,ainum=ainum)
        if ai==len(self.aich):
            raise Exception('GET_CONFIG: The DISTREAM configuration is not supported by get_config()')
        return self.aich[ai]
    
            
    def time(self):
        """time()       Return a 1D time array
        
    t = time()
    
Constructs a 1-D time array with values in seconds for each of the rows
of the data array.  Repeated calls to `time()` return the same array, so
users should make a copy of the array before editing its values unless
they want the effects to be permanent.
"""
        if self._time is None:
            T = 1./self.config.samplehz
            N = self.data.shape[0]
            self._time = np.arange(0.,N*T,T)
        return self._time

    def ds(self, tstart, tstop=None, downsample=0):
        """ds(tstart, tstop=None, downsample=0)
   
    I = ds(tstart, tstop, downsample=0)

D.S. stands for Down-Select.

Returns a slice object that can be used to select a portion of the data
beginning at time `tstart` and ending at time `tstop`.  If tstop is not
specified, then it will revert to the end of the dataset.  Optionally, 
for each datum included, `ds` data will be skipped (or downsampled).

Great care should be taken when downsampling data in this way, because
no filter has been applied to prevent aliasing.  It is solid practice to
apply a digital filter to the data before artificially reducing the 
sample rate.
"""
        start = round(tstart * self.config.samplehz)
        if tstop is not None:
            stop = round(tstop * self.config.samplehz)
        else:
            stop = -1
        step = int(downsample) + 1
        return slice(start, stop, step)


    def show_channel(self, aich, ax=None, fig=None, 
            show=True, ylabel=None, xlabel=None, fs=16,
            tstart=0., tstop=None, downsample=0, 
            plot_param={}):
        """Plot the data from a channel
    mpll = show_channel(aich)
    
Returns the handle to the matplotlib line object created by the plot
command.  The aich is the same index or string used by the get_channel
command.  Optional parameters are:

AX
An optional matplotlib axes object pointing to an existing axes to which
the line should be added.  This method can be used to show multiple data
sets on a single plot.

FIG
The figure can be specified either with a matplotlib figure object or an
integer figure number.  If it exists, the figure will be cleared and a
new axes will be created for the plot.  If it does not exist, a new one
will be created.

TSTART, TSTOP, DOWNSAMPLE
These parameters are passed to the `ds()` method to downsample the data
shown on the plot.

SHOW
If True, then a non-blocking show() command will be called after 
plotting to prompt matplotlib to display the plot.  In some interfaces,
this step is not necessary.

XLABEL, YLABEL
If either is supplied, it will be passed to the set_xlabel and 
set_ylabel functions instead of the automatic values generated from the
channel labels and units

FS
Short for "fontsize" indicates the label font size in points.

PLOT_PARAM
A dictionar of keyword, value pairs that will be passed to the plot 
command to configure the line object.
"""

        # Initialize the figure and the axes
        if ax is not None:
            fig = ax.get_figure()
        elif fig is not None:
            if isinstance(fig, int):
                fig = plt.figure(fig)
            fig.clf()
            ax = fig.add_subplot(111)
        else:
            fig = plt.figure()
            ax = fig.add_subplot(111)
        
        # Which channel is this?
        ai = self.get_index(aich)
        aiconf = self.config.aich[ai]
        
        # Get the downsampled slice
        ii = self.ds(tstart,tstop,downsample)
        
        # Get some descriptive text for the plot
        if aiconf.ailabel:
            ailabel = aiconf.ailabel
        else:
            ailabel = 'AI%d'%aiconf.aichannel
            
        aicalunits = 'V'
        if aiconf.aicalunits:
            aicalunits = aiconf.aicalunits
            
            
        # Get data and time
        t = self.time()[ii]
        y = self.data[ii,ai]
        
        ll = ax.plot(t, y, label=ailabel, **plot_param)
        
        if xlabel:
            ax.set_xlabel(xlabel, fontsize=fs)
        else:
            ax.set_xlabel('Time (s)', fontsize=fs)
        if ylabel:
            ax.set_ylabel(ylabel, fontsize=fs)
        else:
            ax.set_ylabel('%s (%s)'%(ailabel, aicalunits), fontsize=fs)
        ax.grid('on')
        if show:
            plt.show(block=False)

        return ll

    def event_filter(self, x, edge='any', debounce=1, count=None):
        """This is "private" routine builds a list of indicies where rising/falling
rising/falling edge transitions are found to occur in an array of bools.

    index_list = event_filter(x, edge='any', debounce=1, count=None)
    
x           A 1D numpy boolean array
edge        'rising', 'falling, or 'any'
debounce    Debounce filter count
count       Maximum events to return (ignore if None)

The debounce filter requires that DEBOUNCE (integer) samples before and
after a transition remain high/low.  Redundant transitions within that
range are ignored.  For example, if debounce=3, the following would 
indicate a single rising edge
    000111
    00010111
    00011001100111
The following would not be identified as any kind of edge
    00011000
    000110011000
    
In this way, a rapid series of transitions are all grouped as a single 
edge event.  The window in which these transitions are conflated is 
determined by the debounce integer.  If none is specified, then debounce
is 1 (no filter).

The index reported is the datum just prior to the first transition.  In
the successful examples above, all of them have an event index of 2; 
corresponding to the last of the three successive 0 samples.
"""
        # Translate the edge into a number.  1: rising, 0: any, -1: falling
        edge = edge.lower()
        edge_mode = 0
        if edge == 'rising':
            edge_mode = 1
        elif edge == 'falling':
            edge_mode = -1

        # The filter is applied with a state machine that looks for 
        # sequences of bits with the same value.  Each bit with the same
        # value as the last causes the series counter to be incremented
        # Once the counter is the same size (or larger) than the 
        # debounce filter, the appropriate index register is updated to
        # point to the current sample.
        series_count = 0
        # The index registers track the last potential edge index.  For
        # example, the rising_index is updated once `debounce` low bits
        # in a row have been added. It is not recorded as a transition 
        # until `debounce` high bits have also been detected.  If more
        # low bits are found first, then it will be updated before it is
        # ever recorded.
        falling_index = None
        rising_index = None
        # The output is a list of indices.  It starts empty.
        out = []
        # Pretend the last sample was the same as the first
        xlast = x[0]
        # Parse the array one element at a time
        for ii,xi in enumerate(x):
            # If this sample is the same as the last, increment the 
            # series counter.  If not, return it to 1 to begin a new
            # series.
            if xi == xlast:
                series_count += 1
            else:
                series_count = 1
            # If the series counter is high enough, update the indices
            if series_count >= debounce:
                # If this sample is high, this could be a falling edge
                if xi:
                    falling_index = ii
                    # If there was a candidate rising edge, then it is
                    # now officially an edge event. Record it if the 
                    # edge mode is appropriate
                    if rising_index is not None and edge_mode >= 0:
                        out.append(rising_index)
                        rising_index = None
                # If this sample is low, this could be a rising edge
                else:
                    rising_index = ii
                    # If there was a candidate falling edge, then it is
                    # now officially an edge event. Record it if the 
                    # edge mode is appropriate
                    if falling_index is not None and edge_mode <= 0:
                        out.append(falling_index)
                        falling_index = None
                        
                # Check for a fully populated array
                if count is not None and len(out) >= count:
                    break
        return np.array(out, dtype=int)


    def get_events(self, aich, level=0., edge='any', tstart=None, 
            tstop=None, count=None, debounce=1, diff=0):
        """Detect edge crossings returns a list of indexes corresponding to data 
where the crossings occur.

AICH
The channel to search for edge crossings

LEVEL
The level of the crossing

EDGE
can be rising, falling, or any.  Defaults to any

TSTART
The time (in seconds) to start looking for events.  Starts at t=0 if 
unspecified.

TSTOP
The time to stop looking for events.  Defaults to the end of data.

COUNT
The integer maximum number of events to return.  If unspecified, there 
is no limit to the number of events.

DEBOUNCE
See the `event_filter()` method for a detailed description of the 
debounce filter.  An edge will not be recorded unless there have been
`debounce` samples in a row with the same boolean value before and after
the candidate event.  In this way, a rapid series of transitions are all
grouped as a single edge event.  Setting `debounce` to 1 removes the 
filter; all transitions will be reported.

DIFF
Specifies how many times the signal should be differentiated prior to 
searching for edges.  This can be useful when searching for sudden 
changes when the exact level may not be known, but it can also amplify
high-frequency small signal noise, so be careful.  When `diff` is not 0,
the level is interpreted in appropriate units, e.g. V / (sec ** diff)
"""
        
        i0 = 0
        i1 = -1
        if tstart:
            i0 = int(round(tstart * self.config.samplehz))
        if tstop:
            i1 = int(round(tstop * self.config.samplehz))
            
        indices = []
        
        # Get the channel data
        y = self[i0:i1,aich]
        if diff:
            y = np.diff(y, diff)
            y *= self.config.samplehz**diff
        
        # Transpose to a boolean array
        y = (y > level)
        
        indices = self.edge_filter(y, debounce=debounce, edge=edge, count=count)
        
        # Adjust the indices for the offsets created by downselection
        # and differentiation
        if diff or i0:
            indices += diff + i0
        
        return indices
        

    def get_dievents(self, dich=None, level=0., edge='any', tstart=None, 
            tstop=None, count=None, debounce=1):
        """Detect edges on the digital input stream.  When the data were loaded with the DIBITS
keyword set, the LEVEL is ignored, and DICH indicates which bit should be tested.
When the data were loaded with the DIBITS keyword clear, an edge is detected by
the comparison operation:
    LC.get_dichannel() >= level

DICH
The digital input bit to search for edge crossings

LEVEL
The level of the crossing

EDGE
can be rising, falling, or any.  Defaults to any

TSTART
The time (in seconds) to start looking for events.  Starts at t=0 if 
unspecified.

TSTOP
The time to stop looking for events.  Defaults to the end of data.

COUNT
The integer maximum number of events to return.  If unspecified, there 
is no limit to the number of events.

DEBOUNCE
See the `event_filter()` method for a detailed description of the 
debounce filter.  An edge will not be recorded unless there have been
`debounce` samples in a row with the same boolean value before and after
the candidate event.  In this way, a rapid series of transitions are all
grouped as a single edge event.  Setting `debounce` to 1 removes the 
filter; all transitions will be reported.
"""
        
        i0 = 0
        i1 = -1
        if tstart:
            i0 = int(round(self.config.samplehz*tstart))
        if tstop:
            i1 = int(round(self.config.samplehz*tstop))
        
        # Get the channel data
        y = self[i0:i1,dich]
        # Correct for the offset imposed by the downselect
        if i0:
            indices += i0
            
        return indices
        

def load(filename, data=True, cal=True):
    """load(filename, data=True, cal=True)
    
Opens the indicated file and (1) parses the configuration header, and 
(2) if data=True, also loads the data contained therein.  The same tool
can be used to load configuration files and data files since they use
the same configuration format.

When the data keyword is False, load() will return a DevConf instance 
for each of the device configurations found in the file.  For most 
applications, there will only be one.

>>> [c] = load(filename, data=False)
    OR, when there are multiple devices
>>> [c0, c1, ...] = load(filename, data=False)

When the data keyword is True, load() will also append an LData instance
to the end of the returned list, which holds the data found in the file.
In most data files, there will only be one device in the header, so the
returned list will appear

>>> [c, d] = load(filename)

For more information on how to work with these DevConf and LData 
instances, use the in-line help on them or their methods.
"""
    out = []
    dconf = None
    ldata = None
    filename = os.path.abspath(filename)

    with open(filename,'rb') as ff:
        
        # start the parse
        # Read in the new
        param = _read_param(ff)
        value = _read_param(ff)
        # Initialize the meta type
        metatype = 'n'

        while param and value:
            
            #####
            # First, if the parameter indicates the need for a new
            # device connection or channel, create the new element.
            #####
            # Detect a new connection configuration
            if param == 'connection':
                # Appending a minimal dictionary
                # The nested configurations are the only ones that
                # need to be defined explicitly.  All other 
                # parameters are defined by their defaults in 
                # DEF_DEV
                out.append(DevConf())
                dconf = out[-1]

            setattr(dconf, param, value)
            
            param = _read_param(ff)
            value = _read_param(ff)
        
        # If this is a file with data, the next characters should be
        #  "##\n#:" and then the timestamp
        if data:
            # Initialize the result
            timestamp = None
            # Detect the number of channels
            nch = len(dconf.aich) + (dconf.distream != 0)
            # Scan for the timestamp
            thisline = ff.readline().decode('utf-8').strip()
            while not thisline.startswith('#:'):
                thisline = ff.readline().decode('utf-8').strip()
            try:
                timestamp = time.strptime(thisline, '#: %a %b %d %H:%M:%S %Y')
            except:
                print('WARNING: Failed to convert the timestamp.')
                print(thisline)
                
            # If text/ascii
            if dconf.dataformat.getvalue() == 0:
                data_temp = []
                thisline = ff.readline().decode('utf-8')
                while thisline:
                    samples = [float(s) for s in thisline.split()]
                    if len(samples) != nch:
                        raise Exception('LOAD: Line does not have the correct number of samples:\n' + thisline)
                    data_temp.append(samples)
                    thisline = ff.readline().decode('utf-8')
            # If binary format
            else:
                data_temp = []
                samples = []
                s = ff.read(4)
                while s:
                    samples.append(struct.unpack('f',s)[0])
                    if len(samples) == nch:
                        data_temp.append(samples)
                        samples = []
                    s = ff.read(4)
                if samples:
                    print('LOAD: WARNING: last data line was not complete.')
            DATA = LData(dconf, data_temp, cal=cal)
            DATA.timestamp = timestamp
            out.append(DATA)
    return out
        
