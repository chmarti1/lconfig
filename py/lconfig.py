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
    
    def __init__(self):
        self.t = None
        self.data = None
        self.timestamp = None
        self.filename = ''
        self.cal = False
        self.config = None
        


def load(filename, data=False, dibits=False, cal=True):
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
                DATA = LData()
                # Detect the number of channels
                nch = len(dconf.aich) + (dconf.distream != 0)
                # Scan for the timestamp
                thisline = ff.readline().decode('utf-8').strip()
                while not thisline.startswith('#:'):
                    thisline = ff.readline().decode('utf-8').strip()
                try:
                    DATA.timestamp = time.strptime(thisline[3:], '%a %b %d %H:%M:%S %Y')
                except:
                    print('WARNING: Failed to convert the timestamp.')
                    print(thisline)
                    
                # If text/ascii
                if dconf.dataformat.getvalue() == 0:
                    DATA.data = []
                    thisline = ff.readline().decode('utf-8')
                    while thisline:
                        samples = [float(s) for s in thisline.split()]
                        if len(samples) != nch:
                            raise Exception('LOAD: Line does not have the correct number of samples:\n' + thisline)
                        DATA.data.append(samples)
                        thisline = ff.readline().decode('utf-8')
                # If binary format
                else:
                    DATA.data = []
                    samples = []
                    s = ff.read(4)
                    while s:
                        samples.append(struct.unpack('f',s)[0])
                        if len(samples) == nch:
                            DATA.data.append(samples)
                            samples = []
                        s = ff.read(4)
                    if samples:
                        print('LOAD: WARNING: last data line was not complete.')
                DATA.data = np.array(DATA.data)
                out.append(DATA)
        return out
            
