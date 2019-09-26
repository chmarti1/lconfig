TODIR=/usr/local/bin
DBURST="$(TODIR)/dburst"
DRUN="$(TODIR)/drun"

#LINK=-lljacklm -lLabJackM -lm
LINK=-lLabJackM -lm

# The LCONFIG object file
lconfig.o: lconfig.c lconfig.h
	gcc -Wall -c lconfig.c -o lconfig.o

# The LCTOOLS object file
lctools.o: lctools.c lctools.h lconfig.o
	gcc -Wall -c lctools.c -o lctools.o

# The Binaries...
#
drun.bin: lconfig.o lctools.o drun.c
	gcc lconfig.o lctools.o drun.c $(LINK) -o drun.bin
	chmod +x drun.bin

dburst.bin: lconfig.o dburst.c
	gcc lconfig.o dburst.c $(LINK) -o dburst.bin
	chmod +x dburst.bin

test: lconfig.o test.c
	gcc lconfig.o test.c $(LINK) -o test
	chmod +x test

clean:
	rm -f *.o
	rm -f *.bin

install: dburst.bin drun.bin
	cp -f drun.bin $(DRUN)
	chmod 755 $(DRUN)
	cp -f dburst.bin $(DBURST)
	chmod 755 $(DBURST)
