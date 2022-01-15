
            self.data = []
            
            # Read in the ##
            if not ff.readline().startswith('##'):
                sys.stderr.write('LCONF expected ## before data\n')
                return
                
            # Read in the date/timestamp
            self.timestamp = ff.readline()
            # If the data are in binary format
            if(self._devconf[-1]['dataformat'].getvalue()):
                # We need to figure out how many input channels there are
                naich = len(self._devconf[-1]['aich']) + (self._devconf[-1]['distream'] != 0)
                
            else:
                # Read in the data as ascii
                thisline = ff.readline()
                while thisline:
                    self.data.append([float(this) for this in thisline.split()])
                    thisline = ff.readline()
                self.data = np.array(self.data)
            
            # Was digital input streaming active?
            if self.get(0,'distream'):
                # Convert the data to an integer and remove the distream from data
                temp = np.asarray(self.data[:,-1], dtype=int)
                self.data = self.data[:,:-1]
                # If the load is configured to isolate bits
                if dibits:
                    self.didata = np.ndarray((self.data.shape[0],16), dtype=bool)
                    for index in range(0,16):
                        self.didata[:,index] = temp & (1<<index)
                else:
                    self.didata = temp.reshape(self.data.shape[0],1)
                    
            
            # Apply the calibrations?
            if cal:
                # Calculate the calibrated data
                for aich in range(len(self._devconf[0]['aich'])):
                    temp = self.get(0, 'aicalzero', aich=aich)
                    if temp != 0.:
                        self.data[:,aich] -= temp
                    
                    temp = self.get(0,'aicalslope', aich=aich)
                    if temp != 1.:
                        self.data[:,aich] *= temp
                
            T = 1./self.get(0, 'samplehz')
            N = self.data.shape[0]
            self.time = np.arange(0., (N-0.5)*T, T) 

    def __str__(self, width=80):
        out = ''
        for devnum in range(len(self._devconf)):
            out += '*** Device ' + str(devnum) + ' ***\n'
            device = self._devconf[devnum]
            for this in DEF_DEV:
                out += '  %14s : %-14s\n'%(this, repr(self.get(devnum, this)))
            out += '  * Analog Inputs\n'
            for aich in range(len(self._devconf[devnum]['aich'])):
                out += '    * AICH %d\n'%aich
                for this in DEF_AICH:
                    out += '    %14s : %-14s\n'%(this, repr(self.get(devnum, this, aich=aich)))
            out += '  * Analog Outputs\n'
            for aoch in range(len(self._devconf[devnum]['aoch'])):
                out += '    * AOCH %d\n'%aoch
                for this in DEF_AOCH:
                    out += '    %14s : %-14s\n'%(this, repr(self.get(devnum, this, aoch=aoch)))
            out += '  * Flexible Digital IO\n'
            for efch in range(len(self._devconf[devnum]['efch'])):
                out += '    * EFCH %d *'%efch
                for this in DEF_EFCH:
                    out += '    %14s : %-14s\n'%(this, repr(self.get(devnum, this, efch=efch)))
            out += '  * Meta Parameters\n'
            for param,value in self._devconf[devnum]['meta'].items():
                out += '    %14s : %-14s\n'%(param, value)
        return out

    def _get_label(self, devnum, source, label):
        """Return the index of the aich, aoch, or efch member with the label matching label.
    """
        lkey = {'aich':'ailabel', 'aoch':'aolabel', 'efch':'eflabel'}[source]
        for index in range(len(self._devconf[devnum][source])):
            this = self._devconf[devnum][source][index]
            if lkey in this and this[lkey] == label:
                return index
        raise Exception('Failed to find key %s with value %s'%(lkey, repr(label)))
        
    def _get_index(self, time):
        """Get the index closest to the time specified"""
        index = int(np.round(time*self.get(0,'samplehz')))
        # Clamp the values based on the data size
        return min(max(index, 0), self.ndata()-1)

    def ndev(self):
        """Return the number of device configurations loaded"""
        return len(self._devconf)


    def nistream(self, devnum):
        """Return the number of input stream channels.  This is usually equal to the
number of analog input channels unless digital input streaming is also enabled."""
        if 'distream' in self._devconf[devnum]:
            return self.naich(self.devnum) + (1 if self._devconf[devnum] else 0)
        return self.naich(self.devnum)
        
    def naich(self, devnum):
        """Return the number of analog input channels in device devnum"""
        return len(self._devconf[devnum]['aich'])
        
    def naoch(self, devnum):
        """Return the number of analog output channels in device devnum"""
        return len(self._devconf[devnum]['aoch'])
        
    def nefch(self, devnum):
        """Return the number of extended feature IO channels in device devnum"""
        return len(self._devconf[devnum]['efch'])
        
    def ncomch(self, devnum):
        """Return the number of digital communication channels in device devnum"""
        return len(self._devconf[devnum]['comch'])
        
    def ndata(self):
        """Return the number of data samples in the data set.  If no 
data are available, ndata() raises an exception"""
        if self.data is not None:
            return self.data.shape[0]
        raise Exception('NDATA: The LConf object has no data loaded')

    def get_labels(self, devnum, source='aich'):
        """Return an ordered list of channel labels
    [...] = get_labels(devnum, source='aistream')

The default source is 'aich', but the labels for 'aoch' and 'efch' can
also be retrieved.
"""
        lkey = {'aich':'ailabel', 'aoch':'aolabel', 'efch':'eflabel'}[source]
        out = []
        for this in self._devconf[devnum][source]:
            if lkey in this:
                out.append( this[lkey] )
            else:
                out.append( '' )
        return out
        

    def get(self, devnum, param, aich=None, efch=None, aoch=None, comch=None):
        """Retrieve a parameter value
    get(devnum, param, aich=None, efch=None, aoch=None, comch=None)

** Global Parameters **
To return a global parameter from device number devnum.  For example,
to retrieve the sample rate from device 0,
    D.get(0, 'samplehz')
    
To access multiple parameters at a time from the same device, specify 
the parameter as an iterable like a tuple or a list.
    D.get(0, ('samplehz', 'ip'))

To return a parameter belonging to one of the nested configuration 
systems (analog inputs, analog outputs, or ef channels) use the 
optional keywords to identify the channel index.

** Analog Inputs **
To return the entire analog input list, simply request 'aich' directly.
    D.get(0, 'aich')
    
To access the individual channel parameters, use the aich keyword
    D.get(0, 'airange', aich=0)

The aich can also be used to call out a channel by its label.  Channels
without a label can never be matched, even if the string is empty.
    D.get(0, 'airange', aich='Ambient Temperature')
    
** COM, EF, and AO configuration **
The same rules apply for the analog output, com, and ef channels.
    D.get(0, 'aosignal', aoch=0)
    
"""
        # Define the local and default dictionaries
        source = self._devconf[devnum]
        default = DEF_DEV
        
        # Override the source and default if aich, aoch, or efch are 
        # specified.
        if aich is not None:
            # If the reference is by label, search for the correct label
            flag = False
            if isinstance(aich,str):
                aich = self._get_label(devnum, 'aich', aich)
            source = source['aich'][aich]
            default = DEF_AICH
        elif aoch is not None:
            # If the reference is by label, search for the correct label
            flag = False
            if isinstance(aoch,str):
                aoch = self._get_label(devnum, 'aoch', aoch)
            source = source['aoch'][aoch]
            default = DEF_AOCH
        elif efch is not None:
            # If the reference is by label, search for the correct label
            flag = False
            if isinstance(efch,str):
                efch = self._get_label(devnum, 'efch', efch)
            source = source['efch'][efch]
            default = DEF_EFCH
        elif comch is not None:
            # If the reference is by label, search for the correct label
            flag = False
            if isinstance(comch,str):
                comch = self._get_label(devnum, 'comch', comch)
            source = source['comch'][efch]
            default = DEF_COMCH
            
        # If the recall is multiple    
        if hasattr(param, '__iter__'):
            out = []
            for pp in param:
                if pp in source:
                    out.append(source[pp])
                elif pp in default:
                    out.append(default[pp])
                else:
                    raise Exception('Unrecognized parameter: %s'%pp)
            return tuple(out)
            
        # If the recall is single
        if param in source:
            return source[param]
        elif param in default:
            return default[param]
        else:
            raise Exception('Unrecognized parameter: %s'%param)


    def get_meta(self, devnum, param):
        """Retrieve a meta parameter by its name
    value = get_meta(param)

If param is a list or tuple, then the result will be returned as a tuple.
"""
        if isinstance(param, (tuple, list)):
            return tuple([self.get_meta(devnum, this) for this in param])
        return self._devconf[devnum]['meta'][param]


    def get_channel(self, aich, downsample=None, start=None, stop=None):
        """Retrieve data from channel aich
    x = get_channel(aich)

AICH can be the integer index for the channel in the first device's 
analog input channels, or it can be the string channel label.  The first
channel with a matching label will be returned.  If the digital input stream is
configured, then AICH may be set to -1 or NAICH() to recover the  raw 16-bit
EIO/FIO values.

X is the numpy array containing data for the requested channel.

Optional keyword parameters are

DOWNSAMPLE
The downsample key indicates an integer number of samples to reject per
sample returned.  The first example below returns every other sample.  
The second example returns every third sample.
    x = get_channel(aich, downsample=1)
    x = get_channel(aich, downsample=2)
    
START, STOP
If they are not left as None, these specify alternate time values at 
which to start and stop the data.  get_channel() can be executed with 
neither, one, or both of these parameters.
    x = get_channel(aich, start=1.5)    # From 1.5 seconds to end-of-test
    x = get_channel(aich, stop=2)       # From 0 to 2 seconds
    x = get_channel(aich, start=1.5, stop=2) # Between 1.5 and 2 seconds
"""
        if self.data is None:
            raise Exception('GET_CHANNEL: This LConf object does not have channel data.')
            
        if isinstance(aich,str):
            aich = self._get_label(0, 'aich', aich)
        
        if downsample or start or stop:
            # Initialize slice indices
            I0 = 0
            I1 = -1
            I2 = 1
            if start is not None:
                I0 = self._get_index(start)
            if stop is not None:
                I1 = self._get_index(stop)
            if downsample is not None:
                I2 = int(downsample+1)
            return self.data[I0:I1:I2, aich]
            
        return self.data[:,aich]
        
    def get_dichannel(self, dich=None, downsample=None, start=None, stop=None):
        """Retrieve data from a digital input stream
    x = get_dichannel()
    x = get_dichannel(dich)

When the data were loaded with the DIBITS keyword set, DICH is the integer index
for the digital input stream bit to return.  Otherwise, DICH is ignored, and the
raw digital input stream values are returned as an integer array.

X is the numpy array containing a boolean array of the bit in question.

Optional keyword parameters are

DOWNSAMPLE
The downsample key indicates an integer number of samples to reject per
sample returned.  The first example below returns every other sample.  
The second example returns every third sample.
    x = get_channel(aich, downsample=1)
    x = get_channel(aich, downsample=2)
    
START, STOP
If they are not left as None, these specify alternate time values at 
which to start and stop the data.  get_channel() can be executed with 
neither, one, or both of these parameters.
    x = get_channel(aich, start=1.5)    # From 1.5 seconds to end-of-test
    x = get_channel(aich, stop=2)       # From 0 to 2 seconds
    x = get_channel(aich, start=1.5, stop=2) # Between 1.5 and 2 seconds
"""
        if not self.get(0,'distream'):
            raise Exception('GET_DICHANNEL: The data does not seem to include a digital input stream.')
        if self.didata.shape[1]==1:
            dich = 0
        elif dich is None:
            raise Exception('GET_DICHANNEL: The DICH channel number is mandatory when data are loaded bit-wise.')
            
        if downsample or start or stop:
            # Initialize slice indices
            I0 = 0
            I1 = -1
            I2 = 1
            if start is not None:
                I0 = self._get_index(start)
            if stop is not None:
                I1 = self._get_index(stop)
            if downsample is not None:
                I2 = int(downsample+1)
            return self.didata[I0:I1:I2, dich]
        return self.didata[:,dich]

    def get_time(self, downsample=None, start=None, stop=None):
        """Retrieve a time vector corresponding to the channel data
    t = get_time()
    
This funciton merely returns a to the "time" member array if data were
loaded when the LConf object was defined.  Otherwise, get_time() raises
an exception
"""
        if self.time is None:
            raise Exception('GET_TIME: This LConf object does not have channel data.')
            
        if downsample or start or stop:
            # Initialize slice indices
            I0 = 0
            I1 = -1
            I2 = 1
            if start is not None:
                I0 = self._get_index(start)
            if stop is not None:
                I1 = self._get_index(stop)
            if downsample is not None:
                I2 = int(downsample+1)
            return self.time[I0:I1:I2]
        return self.time


    def show_channel(self, aich, ax=None, fig=None, downsample=None, 
            show=True, ylabel=None, xlabel=None, fs=16,
            start=None, stop=None,
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

DOWNSAMPLE
This parameter is passed to get_time() and get_channel() to reduce the 
size of the dataset shown.

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
        
        if isinstance(aich,str):
            aich = self._get_label(0,'aich',aich)
            
        # Get the y-axis label
        if 'ailabel' in self._devconf[0]['aich'][aich]:
            ailabel = self._devconf[0]['aich'][aich]['ailabel']
        else:
            ailabel = 'AI%d'%self.get(0, 'aichannel', aich=aich)
            
        # Get the y-axis units
        if 'aicalunits' in self._devconf[0]['aich'][aich]:
            aicalunits = self._devconf[0]['aich'][aich]['aicalunits']
        else:
            aicalunits = 'V'
            
        # Get data and time
        t = self.get_time(downsample=downsample, start=start, stop=stop)
        y = self.get_channel(aich, downsample=downsample, start=start, stop=stop)
        
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
        
    def show_dichannel(self, dich=None, ax=None, fig=None, downsample=None, 
            show=True, ylabel=None, xlabel=None, fs=16,
            start=None, stop=None,
            plot_param={}):
        """Plot the data from a digital input channel
    mpll = show_dichannel(dich)
    
Returns the handle to the matplotlib line object created by the plot
command.  The dich is the same index used by the get_dichannel method.  
Optional parameters are:

AX
An optional matplotlib axes object pointing to an existing axes to which
the line should be added.  This method can be used to show multiple data
sets on a single plot.

FIG
The figure can be specified either with a matplotlib figure object or an
integer figure number.  If it exists, the figure will be cleared and a
new axes will be created for the plot.  If it does not exist, a new one
will be created.

DOWNSAMPLE
This parameter is passed to get_time() and get_channel() to reduce the 
size of the dataset shown.

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
            
        # Build the y-label
        # If the data were loaded as "raw" instead of bits
        if self.didata.shape[1] == 1:
            dilabel = "DI Stream"
            dicalunits = 'uint16'
        elif dich is not None:
            dilabel = "DI%d"%dich
            dicalunits = 'bit'
        else:
            raise Exception('SHOW_DICHANNEL: The DICH input channel is required when data were loaded with the DIBITS set')
            
        # Get data and time
        t = self.get_time(downsample=downsample, start=start, stop=stop)
        y = self.get_dichannel(dich, downsample=downsample, start=start, stop=stop)
        
        ll = ax.plot(t, y, label=dilabel, **plot_param)
        
        if xlabel:
            ax.set_xlabel(xlabel, fontsize=fs)
        else:
            ax.set_xlabel('Time (s)', fontsize=fs)
        if ylabel:
            ax.set_ylabel(ylabel, fontsize=fs)
        else:
            ax.set_ylabel('%s (%s)'%(dilabel, dicalunits), fontsize=fs)
        ax.grid('on')
        if show:
            plt.show(block=False)

        return ll

    def get_events(self, aich, level=0., edge='any', start=None, 
            stop=None, count=None, debounce=1, diff=0):
        """Detect edge crossings returns a list of indexes corresponding to data 
where the crossings occur.

AICH
The channel to search for edge crossings

LEVEL
The level of the crossing

EDGE
can be rising, falling, or any.  Defaults to any

START
The time (in seconds) to start looking for events.  Starts at t=0 if 
unspecified.

STOP
The time to stop looking for events.  Defaults to the end of data.

COUNT
The integer maximum number of events to return.  If unspecified, there 
is no limit to the number of events.

DEBOUNCE
The debounce filter requires that DEBOUNCE (integer) samples before and
after a transition remain high/low.  Redundant transitions within that
range are ignored.  For example, if debounce=3, let "l" indicate a 
sample less than the level, and "g" indicate greater than: 
The following would indicate a single rising edge
    lllggg
    lllglggg
    lllggllggllggg
The following would not be identified as any kind of edge
    lllgglll
    lllgllgllglll
    
In this way, a rapid series of transitions are all grouped as a single 
edge event.  The window in which these transitions are conflated is 
determined by the debounce integer.  If none is specified, then debounce
is 1 (no filter).

DIFF
Specifies the number of derivatives to take prior to scanning for events
This is done by y.
"""

        edge = edge.lower()
        edge_mode = 0
        if edge == 'rising':
            edge_mode = 1
        elif edge == 'falling':
            edge_mode = -1
        
        i0 = 0
        i1 = self.ndata()-1
        if start:
            i0 = self._get_index(start)
        if stop:
            i1 = self._get_index(stop)
            
        indices = []
        
        # Get the channel data
        y = self.get_channel(aich)
        if diff:
            y = np.diff(y, diff)
            y *= self.get(0, 'samplehz')**diff
        
        # State machine variables
        rising_index = None
        falling_index = None
        series_count = 1
        test_last = (y[i0] > level)
        
        for index in range(i0+1, i1):
            test = (y[index] > level)
            
            if test == test_last:
                series_count += 1
            # If there has been a value change
            else:
                series_count = 1
            
            # Check the sample count
            if series_count >= debounce:
                # If the sample is greater than
                if test:
                    falling_index = index
                    if rising_index and edge_mode >= 0:
                        indices.append(rising_index+diff)
                        rising_index = None
                # If the sample is less than
                else:
                    rising_index = index
                    if falling_index and edge_mode <= 0:
                        indices.append(falling_index+diff)
                        falling_index = None
                
            if count and len(indices) >= count:
                break
                
            test_last = test
        return indices
        

    def get_dievents(self, dich=None, level=0., edge='any', start=None, 
            stop=None, count=None, debounce=1):
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

START
The time (in seconds) to start looking for events.  Starts at t=0 if 
unspecified.

STOP
The time to stop looking for events.  Defaults to the end of data.

COUNT
The integer maximum number of events to return.  If unspecified, there 
is no limit to the number of events.

DEBOUNCE
The debounce filter requires that DEBOUNCE (integer) samples before and
after a transition remain high/low.  Redundant transitions within that
range are ignored.  For example, if debounce=3, let "l" indicate a 
sample less than the level, and "g" indicate greater than: 
The following would indicate a single rising edge
    lllggg
    lllglggg
    lllggllggllggg
The following would not be identified as any kind of edge
    lllgglll
    lllgllgllglll
    
In this way, a rapid series of transitions are all grouped as a single 
edge event.  The window in which these transitions are conflated is 
determined by the debounce integer.  If none is specified, then debounce
is 1 (no filter).
"""

        edge = edge.lower()
        edge_mode = 0
        if edge == 'rising':
            edge_mode = 1
        elif edge == 'falling':
            edge_mode = -1
        
        i0 = 0
        i1 = self.ndata()-1
        if start:
            i0 = self._get_index(start)
        if stop:
            i1 = self._get_index(stop)
            
        indices = []
        
        # Get the channel data
        y = self.get_dichannel(dich)
        
        # State machine variables
        rising_index = None
        falling_index = None
        series_count = 1
        test_last = (y[i0] > level)
        
        for index in range(i0+1, i1):
            if self.didata.shape[1] == 1:
                test = (y[index] >= level)
            else:
                test = y[index]
            
            if test == test_last:
                series_count += 1
            # If there has been a value change
            else:
                series_count = 1
            
            # Check the sample count
            if series_count >= debounce:
                # If the sample is greater than
                if test:
                    falling_index = index
                    if rising_index and edge_mode >= 0:
                        indices.append(rising_index)
                        rising_index = None
                # If the sample is less than
                else:
                    rising_index = index
                    if falling_index and edge_mode <= 0:
                        indices.append(falling_index)
                        falling_index = None
                
            if count and len(indices) >= count:
                break
                
            test_last = test
        return indices
        
