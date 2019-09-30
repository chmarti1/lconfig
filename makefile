TODIR=/usr/local/bin
LCBURST="$(TODIR)/lcburst"
LCRUN="$(TODIR)/lcrun"

#LINK=-lljacklm -lLabJackM -lm
LINK=-lLabJackM -lm

# The LCONFIG object file
lconfig.o: lconfig.c lconfig.h lcmap.h
	gcc -Wall -c lconfig.c -o lconfig.o

# The LCTOOLS object file
lctools.o: lctools.c lctools.h lconfig.o
	gcc -Wall -c lctools.c -o lctools.o

# The Binaries...
#
lcrun.bin: lconfig.o lctools.o lcrun.c
	gcc lconfig.o lctools.o lcrun.c $(LINK) -o lcrun.bin
	chmod +x lcrun.bin

lcburst.bin: lconfig.o lcburst.c
	gcc lconfig.o lctools.o lcburst.c $(LINK) -o lcburst.bin
	chmod +x lcburst.bin

test: lconfig.o test.c
	gcc lconfig.o test.c $(LINK) -o test
	chmod +x test

clean:
	rm -f *.o
	rm -f *.bin

install: lcburst.bin lcrun.bin
	cp -f lcrun.bin $(LCRUN)
	chmod 755 $(LCRUN)
	cp -f lcburst.bin $(LCBURST)
	chmod 755 $(LCBURST)
