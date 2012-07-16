#CC=i386-pc-linux-gcc
CC=gcc

ifdef IPF_LIB_SUPPORT
    CFLAGS=-O3 -I ../../../libhxcfe/trunk/sources	-I . -I ../sources/ftdi/ -DIPF_SUPPORT
    DISKLOADERPLUGINS_IPF= ipf_loader.o libcapsimage.so.2.0
else
    CFLAGS=-O3 -I ../sources -I ../../../libhxcfe/trunk/sources -I ../sources/ftdi/
    DISKLOADERPLUGINS_IPF=
endif

LDFLAGS=-lc -lm  -ldl  -lpthread
EXEC=hxcfe

FTDIFLOPPYEMU=usb_hxcfloppyemulator.o variablebitrate.o

all: $(EXEC)

hxcfloppyemulator_convert: hxcfloppyemu_convert.o floppy_loader.o floppy_utils.o linux_api.o emuii_track.o crc.o track_generator.o gcr_track.o sector_extractor.o  $(ADFLIBOBJ)  $(DISKLOADERPLUGINS_IPF) 
	$(CC) -o $@    $^ $(LDFLAGS) -DFTDILIB

hxcfloppyemulator_d2xx: hxcfloppyemu.o floppy_loader.o floppy_utils.o linux_api.o ftdi_d2xxmode.o emuii_track.o crc.o track_generator.o gcr_track.o sector_extractor.o  $(ADFLIBOBJ)  $(DISKLOADERPLUGINS_IPF) $(FTDIFLOPPYEMU) libftd2xx.a.0.4.13
	$(CC) -o $@    $^ $(LDFLAGS)

#hxcfe:      hxcfe.o linux_api.o  $(FTDIFLOPPYEMU) $(DISKLOADERPLUGINS_IPF) /usr/local/lib/libftdi.so.1

hxcfeconv:      hxcfe.o linux_api.o ../../../build/libhxcfe.a
	$(CC) -o $@    $^ $(LDFLAGS)
	cp $@ ../../../build

hxcfe:      hxcfe.o linux_api.o ../../../build/libhxcfe.a
	$(CC) -o $@    $^ $(LDFLAGS) -DFTDILIB
	cp $@ ../../../build

hxcfe.o: ../sources/hxcfe.c 
	$(CC) -o $@ -c $< $(CFLAGS)

linux_api.o: ../sources/linux_api.c
	$(CC) -o $@ -c $< $(CFLAGS)

usb_hxcfloppyemulator.o : ../sources/usb_floppyemulator/usb_hxcfloppyemulator.c
	$(CC) -o $@ -c $< $(CFLAGS)

variablebitrate.o : ../sources/usb_floppyemulator/variablebitrate.c
	$(CC) -o $@ -c $< $(CFLAGS)

ftdi_d2xxmode.o : ../sources/ftdi/ftdi.c ../sources/ftdi/ftdi.h ../sources/ftdi/ftd2xx.h ../sources/ftdi/WinTypes.h
	$(CC) -o $@ -c $< $(CFLAGS)

ftdi_libftdimode.o : ../sources/ftdi/ftdi.c ../sources/ftdi/ftdi.h ../sources/ftdi/ftd2xx.h ../sources/ftdi/WinTypes.h
	$(CC) -o $@ -c $< $(CFLAGS) -DFTDILIB

clean:
	rm -rf *.o

mrproper: clean
	rm -rf $(EXEC)

.PHONY: clean mrproper
