#
#   Load LCONF configuration and data
#
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
            elif name.startsith('ao'):
                setattr(self.aoch[-1], name, value)
            elif name == 'efchannel':
                self.efch.append(EfConf())
                setattr(self.efch[-1], name, value)
            elif name.startswith('ef'):
                setattr(self.efch[-1], name, value)
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

    def naich(self):
        return len(self.aich) + (self.distream != 0)


        
class AiConf(Conf):
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

class AoConf(Conf):
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

class EfConf(Conf):
    def __init__(self):
        self.__dict__.update({
            'efchannel':-1,
            'efsignal':LEnum(['pwm', 'count', 'frequency', 'phase', 'quadrature']),
            'efedge':LEnum(['rising', 'falling', 'all']),
            'efdebounce':LEnum(['none', 'fixed', 'reset', 'minimum']),
            'efdirection':LEnum(['input', 'output']),
            'efusec':0.,
            'efdegrees':0.,
            'efduty':0.5,
            'efcount':0,
            'eflabel':''
        })
        
class ComConf(Conf):
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

.dbits          digital input stream bit array
If the distream was configured and the `dbits` keyword was set to `True`
this array mimics the `data` member; each column is one of the sixteen
available digital bits, and each row is one of the streamed samples.

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
.apply_cal()        Applies the channel calibrations
.get_channel()      Returns a 1D array of the channel data
.get_time()         Returns a 1D array of times since collection started

Interacting with data
==========================
In addition to the `get_channel()` method, `LData` instances can be 
indexed directly using [] notation.  This interface supports rerencing
channels by their labels and slicing.

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
    def __init__(self, config, data, cal=True, dbits=True):
        self.data = None
        self.dbits = None
        self.timestamp = None
        self.filename = ''
        self.cal = False
        self.config = None
        # Private members
        self._time = None
        self._bylabel = {}
        self._byainum = {}
        
        self.config = config
        # format the data array
        self.data = np.array(data, dtype=float)
        # Check for correct shape
        nch = config.naich()
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
            ndata = self.data.shape[0]
            self._bylabel['distream'] = nch-1
            self.dbits = np.zeros((ndata,16), dtype=bool)
            if dbits:
                for ii in range(ndata):
                    for jj in range(16):
                        self.dbits[ii, jj] = bool(int(self.data[ii, nch-1]) & 1<<jj)


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

    def get_channel(self, target=None, ainum=None, tstart=None, tstop=None, downselect=0):
        """get_channel( 'channel label' )
    OR
get_channel( number )
    OR
get_channel(ainum = number)

Returns a 1D array of samples for a single channel.  This is similar to
the functionality offered by the item interface [] notation, but it
includes more advanced options for specifying the channel and for down-
selecting the data in time.

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

:: Specifying a channel by its hardware number ::
When the `ainum` keyword is used instead, the channel is identified by 
its corresponding positive hardware channel.  For example, a channel 
configured to accept `AIN2` as the positive input would be identified
by `ainum = 2`.  It would be strange to configure an analog input stream
with the same hardware channel appearing twice, but if that occurs, 
whichever is configured last will be returned.

:: Specifying a start and stop time ::
Optional keyword parameters, tstart and tstop, allow the user to specify
a time window by which to down-select the data.  These are specified in
seconds.  When start (stop) is not specified, then the beginning (end) 
of data is used.
>>> ldata_instance.get_channel(0, tstart = 1.5, tstop = 2.5)

:: Downselection filter ::
Optionally, the `downselect` keyword can be used to specify a number of
data points to skip.  For every datum returned, `downselect` specifies
the number of points to skip.  By default, it is zero.
"""
        ai = None
        ti = slice(0,-1)
        # Deal with slicing
        if tstart or tstop or downselect:
            start = 0
            stop = -1
            step = 1
            if tstart:
                start = int(tstart * self.config.samplehz)
            if tstop:
                stop = int(tstop * self.config.samplehz)
            if downselect:
                step = int(downselect + 1)
            ti = slice(start,stop,step)
            
        
        # If target was not specified
        if target is None:
            # The input must have been through ainum.
            if ainum is None:
                raise Exception('GET_CHANNEL: Missing mandatory argument.')
            ai = self._byainum.get(ainum)
            if ai is None:
                raise Exception('GET_CHANNEL: Analog input number %d not found.'%ainum)
        # If the target was specified
        else:
            # Make sure both weren't specified
            if ainum is not None:
                raise Exception('GET_CHANNEL: Accepts only one argument.')
            # If the channel is specified by label
            if isinstance(target, str):
                ai = self._bylabel.get(target)
                # If the channel was not found
                if ai is None:
                    raise Exception('GET_CHANNEL: Label not recognized: ' + target)
            # If the channel is specified by index
            elif isinstance(target, int):
                nch = self.config.naich()
                # Handle integer wrapping
                ai = target
                if target < 0:
                    ai += nch
                if ai < 0 or ai >= self.config.naich():
                    raise Exception('GET_CHANNEL: Index, %d, is out of range with %d channels.'%(target, nch)) 
        
        if ai is None:
            raise Exception('GET_CHANNEL: Unhandled exception!')
        
        return self.data[ti,ai]
            
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


def load(filename, data=False, cal=True, dbits=False):
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
            DATA = LData(dconf, data_temp, cal=cal, dbits=dbits)
            DATA.timestamp = timestamp
            out.append(DATA)
    return out
        
