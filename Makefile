CC=gcc
#CFLAGS= -O3 -Wall -ggdb -m32
CFLAGS= -O2 -Wall -march=native -mmmx -msse -msse2 -m32
#CFLAGS= -ggdb

PROGNAME=main
PKGNAME=serial_proxy.pkg
THINSTATION_DIR=thinstation
THINSTATION_BINDIR=$(THINSTATION_DIR)/bin
SERIAL_PROXY_NAME=serial_proxy

OBJ= $(PROGNAME).o net_prot.o net_connect.o hw_prot.o hw_maria301.o hw_datecs.o hw_prot_privat.o hw_prot_fuib.o logs.o util.o checkpid.o

.c.o:
	$(CC) -c $(CFLAGS) $(DEFS) $<

all: $(PROGNAME) pkg

$(PROGNAME): version.h $(OBJ) Makefile Makefile.dep
	$(CC) $(CFLAGS) $(DEFS) -o $(PROGNAME) $(OBJ)
#	/bin/cp -f $(PROGNAME) $(TAREM_DIR)/bin/

Makefile.dep:
	echo \# > Makefile.dep

clean:
	rm -f *.o *.cgi *~ core *.b $(PROGNAME) version.h

version.h: .version
	@LANG=C echo \#define COMPILE_TIME \"`date "+%d-%b-%Y %H:%M:%S"`\" > version.h
	@echo \#define SVN_REVISION \"`svn info | awk '/^Revision: / {print $$2;}'`\" >> version.h

dep: clean version.h
	$(CC) -MM *.c > Makefile.dep
	
$(PKGNAME): $(PROGNAME)
	cp -f $(PROGNAME) $(THINSTATION_BINDIR)/$(SERIAL_PROXY_NAME)
	strip --target=elf32-i386 $(THINSTATION_BINDIR)/$(SERIAL_PROXY_NAME)
	tar czf $(PKGNAME) -C thinstation/ .

pkg: $(PKGNAME)

include Makefile.dep
