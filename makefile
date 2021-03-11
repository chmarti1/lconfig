TODIR=/usr/local/bin
LCBURST="$(TODIR)/lcburst"
LCRUN="$(TODIR)/lcrun"
LCSTAT="$(TODIR)/lcstat"

#LINK=-lljacklm -lLabJackM -lm
LINK=-lLabJackM -lm

# The LCONFIG object file
lconfig.o: lconfig.c lconfig.h lcmap.h
	gcc -Wall -c lconfig.c -o lconfig.o

# The LCTOOLS object file
lctools.o: lctools.c lctools.h lconfig.h lconfig.o
	gcc -Wall -c lctools.c -o lctools.o

# The LCMAP object file
lcmap.o: lcmap.c lcmap.h lconfig.h
	gcc -Wall -c lcmap.c -o lcmap.o

# The Binaries...
#
lcstat.bin: lcmap.o lconfig.o lctools.o lcstat.c
	gcc lcmap.o lconfig.o lctools.o lcstat.c $(LINK) -o lcstat.bin
	chmod +x lcstat.bin

lcrun.bin: lcmap.o lconfig.o lctools.o lcrun.c
	gcc lcmap.o lconfig.o lctools.o lcrun.c $(LINK) -o lcrun.bin
	chmod +x lcrun.bin

lcburst.bin: lcmap.o lconfig.o lctools.o lcburst.c
	gcc lcmap.o lconfig.o lctools.o lcburst.c $(LINK) -o lcburst.bin
	chmod +x lcburst.bin
clean:
	rm -f *.o
	rm -f *.bin

install: lcburst.bin lcrun.bin lcstat.bin
	cp -f lcrun.bin $(LCRUN)
	chmod 755 $(LCRUN)
	cp -f lcburst.bin $(LCBURST)
	chmod 755 $(LCBURST)
	cp -f lcstat.bin $(LCSTAT)
	chmod 755 $(LCSTAT)
