#CC=i386-pc-linux-gcc
CC=gcc
AR=ar

BASEDIR=../sources
INCLUDES = -I$(BASEDIR)/ -I ../../../libhxcfe/trunk/sources -I ../sources/win32 -I ../../../libhxcadaptor/trunk/sources

CFLAGS=-O3 $(INCLUDES)
LDFLAGS= -g -shared -lc -lm  -ldl

ifdef __APPLE__
CFLAGS=-O3 $(INCLUDES) -arch i386
LDFLAGS= -g -shared -lc -lm  -ldl -arch i386
endif

EXEC=libusbhxcfe.a

all: $(EXEC)

libusbhxcfe.a: usb_hxcfloppyemulator.o variablebitrate.o ftdi.o
	$(AR) r $@    $^ 
	cp $@ ../../../build

libusbhxcfe.so: usb_hxcfloppyemulator.c variablebitrate.o ftdi.o
	$(CC) -o $@    $^ $(LDFLAGS)
		
usb_hxcfloppyemulator.o: $(BASEDIR)/usb_hxcfloppyemulator.c
	$(CC) -o $@ -c $< $(CFLAGS)

variablebitrate.o: $(BASEDIR)/variablebitrate.c
	$(CC) -o $@ -c $< $(CFLAGS)

ftdi.o: $(BASEDIR)/ftdi.c
	$(CC) -o $@ -c $< $(CFLAGS)

ftdi_d2xxmode.o : ftdi.c ftdi.h ftd2xx.h WinTypes.h
	$(CC) -o $@ -c $< $(CFLAGS)

ftdi_libftdimode.o : ftdi.c ftdi.h ftd2xx.h WinTypes.h
	$(CC) -o $@ -c $< $(CFLAGS) -DFTDILIB

clean:
	rm -rf *.o

mrproper: clean
	rm -rf $(EXEC)

.PHONY: clean mrproper
