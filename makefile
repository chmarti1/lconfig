# Install destinations
TODIR=/usr/local/bin
LCBURST="$(TODIR)/lcburst"
LCRUN="$(TODIR)/lcrun"
LCSTAT="$(TODIR)/lcstat"
# Build destinations
BUILD=build
LCONFIG_O=$(BUILD)/lconfig.o
LCTOOLS_O=$(BUILD)/lctools.o
LCMAP_O=$(BUILD)/lcmap.o
LCFILTER_O=$(BUILD)/lcfilter.o
ALL_O=$(LCONFIG_O) $(LCTOOLS_O) $(LCMAP_O) $(LCFILTER_O)
LCSTAT_B=$(BUILD)/lcstat.bin
LCRUN_B=$(BUILD)/lcrun.bin
LCBURST_B=$(BUILD)/lcburst.bin
ALL_B=$(LCSTAT_B) $(LCRUN_B) $(LCBURST_B)
# Binary CHMOD settings
BIN_CHMOD=755
# Linked libraries
LINK=-lLabJackM -lm
# Compiler options
OPT=-Wall

# The build directory
$(BUILD):
	mkdir $(BUILD)

# The LCONFIG object file
$(LCONFIG_O): $(BUILD) lconfig.c lconfig.h lcmap.h lcfilter.h
	gcc $(OPT) -c lconfig.c -o $(LCONFIG_O)

# The LCTOOLS object file
$(LCTOOLS_O): $(BUILD) lctools.c lctools.h lconfig.h
	gcc $(OPT) -c lctools.c -o $(LCTOOLS_O)

# The LCMAP object file
$(LCMAP_O): $(BUILD) lcmap.c lcmap.h lconfig.h
	gcc $(OPT) -c lcmap.c -o $(LCMAP_O)

# The LCFILTER object file
$(LCFILTER_O): $(BUILD) lcfilter.c lcfilter.h
	gcc $(OPT) -c lcfilter.c -o $(LCFILTER_O)

# The Binaries...
#
$(LCSTAT_B): $(ALL_O) lcstat.c
	gcc $(ALL_O) lcstat.c $(LINK) -o $(LCSTAT_B)
	chmod $(BIN_CHMOD) $(LCSTAT_B)

$(LCRUN_B): $(ALL_O) lcrun.c
	gcc $(ALL_O) lcrun.c $(LINK) -o $(LCRUN_B)
	chmod $(BIN_CHMOD) $(LCRUN_B)

$(LCBURST_B): $(ALL_O) lcburst.c
	gcc $(ALL_O) lcburst.c $(LINK) -o $(LCBURST_B)
	chmod $(BIN_CHMOD) $(LCBURST_B)

binaries: $(ALL_B)

clean:
	rm -rf $(BUILD)

install: $(ALL_B)
	cp -f $(LCRUN_B) $(LCRUN)
	chmod $(BIN_CHMOD) $(LCRUN)
	cp -f $(LCBURST_B) $(LCBURST)
	chmod $(BIN_CHMOD) $(LCBURST)
	cp -f $(LCSTAT_B) $(LCSTAT)
	chmod $(BIN_CHMOD) $(LCSTAT)

uninstall:
	rm $(LCRUN)
	rm $(LCBURST)
	rm $(LCSTAT)
