#CC=i386-pc-linux-gcc
CC=gcc
CPP=g++

UNAME := $(shell uname)

INCLUDEPATH = -I ../sources -I ../../../libhxcfe/trunk/sources -I ../sources/  -I ../sources/gui -I ../sources/thirdpartylibs/fltk/fltk-1.3.x/ -I ../../../libusbhxcfe/trunk/sources -I ../../../libhxcadaptor/trunk/sources -I ../sources/gui/microintro/

ifdef IPF_LIB_SUPPORT
    CFLAGS=-O3 $(INCLUDEPATH) -DIPF_SUPPORT
    DISKLOADERPLUGINS_IPF= ipf_loader.o libcapsimage.so.2.0
else
    CFLAGS=-O3  $(INCLUDEPATH)
    DISKLOADERPLUGINS_IPF=
endif

LDFLAGS=-lc -lm  -ldl  -lpthread -Wl,-rpath=.

LIBHXCFE = libhxcfe.so
LIBUSBHXCFE = libusbhxcfe.so

ifeq ($(findstring CYGWIN,$(shell uname)),CYGWIN)
LIBHXCFE = libhxcfe.dll
LIBUSBHXCFE = libusbhxcfe.dll
endif

ifeq ($(UNAME), Darwin)
CFLAGS += -DOSX -arch i386
LDFLAGS += -framework Cocoa -arch i386
LIBHXCFE = libhxcfe.dylib
LIBUSBHXCFE = libusbhxcfe.dylib
else
LDFLAGS += -lX11
endif

EXEC=hxcfloppyemulator

FTDIFLOPPYEMU=usb_hxcfloppyemulator.o variablebitrate.o

GUI = main_gui.o about_gui.o batch_converter_window.o floppy_dump_window.o floppy_infos_window.o license_gui.o log.o rawfile_loader_window.o sdhxcfecfg_window.o usbhxcfecfg_window.o cb_batch_converter_window.o cb_floppy_dump_window.o cb_floppy_infos_window.o cb_rawfile_loader_window.o cb_sdhxcfecfg_window.o cb_usbhxcfecfg_window.o cb_filesystem_generator_window.o filesystem_generator_window.o 

MICROINTRO = microintro.o lzw.o pack.o rle.o
FLTKLIB = ../sources/thirdpartylibs/fltk/fltk-1.3.x/lib/libfltk.a ../sources/thirdpartylibs/fltk/fltk-1.3.x/lib/libfltk_images.a

all: $(EXEC)

hxcfloppyemulator: main.o utils.o $(GUI) $(MICROINTRO) loader.o soft_cfg_file.o fileselector.o $(FLTKLIB) $(LIBHXCFE) $(LIBUSBHXCFE) ../../../build/libhxcadaptor.a
	$(CPP) -o $@    $^ $(LDFLAGS)
	cp $@ ../../../build

$(LIBHXCFE): ../../../build/$(LIBHXCFE)
	ln ../../../build/$(LIBHXCFE)  $(LIBHXCFE)

$(LIBUSBHXCFE): ../../../build/$(LIBUSBHXCFE)
	ln ../../../build/$(LIBUSBHXCFE)  $(LIBUSBHXCFE)

main.o: ../sources/main.cxx 
	$(CPP) -o $@ -c $< $(CFLAGS)

utils.o: ../sources/utils.cxx 
	$(CPP) -o $@ -c $< $(CFLAGS)

loader.o: ../sources/loader.cxx 
	$(CPP) -o $@ -c $< $(CFLAGS)

soft_cfg_file.o: ../sources/soft_cfg_file.cxx 
	$(CPP) -o $@ -c $< $(CFLAGS)

about_gui.o : ../sources/gui/about_gui.cxx
	$(CPP) -o $@ -c $< $(CFLAGS)

batch_converter_window.o : ../sources/gui/batch_converter_window.cxx
	$(CPP) -o $@ -c $< $(CFLAGS)

cb_batch_converter_window.o : ../sources/gui/cb_batch_converter_window.cxx
	$(CPP) -o $@ -c $< $(CFLAGS)

cb_floppy_dump_window.o : ../sources/gui/cb_floppy_dump_window.cxx
	$(CPP) -o $@ -c $< $(CFLAGS)

cb_floppy_infos_window.o : ../sources/gui/cb_floppy_infos_window.cxx
	$(CPP) -o $@ -c $< $(CFLAGS)

cb_rawfile_loader_window.o : ../sources/gui/cb_rawfile_loader_window.cxx
	$(CPP) -o $@ -c $< $(CFLAGS)

cb_sdhxcfecfg_window.o : ../sources/gui/cb_sdhxcfecfg_window.cxx
	$(CPP) -o $@ -c $< $(CFLAGS)

cb_usbhxcfecfg_window.o : ../sources/gui/cb_usbhxcfecfg_window.cxx
	$(CPP) -o $@ -c $< $(CFLAGS)

cb_filesystem_generator_window.o : ../sources/gui/cb_filesystem_generator_window.cxx
	$(CPP) -o $@ -c $< $(CFLAGS)

fileselector.o : ../sources/gui/fileselector.cxx
	$(CPP) -o $@ -c $< $(CFLAGS)

filesystem_generator_window.o : ../sources/gui/filesystem_generator_window.cxx
	$(CPP) -o $@ -c $< $(CFLAGS)

fl_dnd_box.o : ../sources/gui/fl_dnd_box.cxx
	$(CPP) -o $@ -c $< $(CFLAGS)

floppy_dump_window.o : ../sources/gui/floppy_dump_window.cxx
	$(CPP) -o $@ -c $< $(CFLAGS)

floppy_infos_window.o : ../sources/gui/floppy_infos_window.cxx
	$(CPP) -o $@ -c $< $(CFLAGS)

license_gui.o : ../sources/gui/license_gui.cxx
	$(CPP) -o $@ -c $< $(CFLAGS)

log.o : ../sources/gui/log.cxx
	$(CPP) -o $@ -c $< $(CFLAGS)

main_gui.o : ../sources/gui/main_gui.cxx
	$(CPP) -o $@ -c $< $(CFLAGS)

rawfile_loader_window.o : ../sources/gui/rawfile_loader_window.cxx
	$(CPP) -o $@ -c $< $(CFLAGS)

sdhxcfecfg_window.o : ../sources/gui/sdhxcfecfg_window.cxx
	$(CPP) -o $@ -c $< $(CFLAGS)

usbhxcfecfg_window.o : ../sources/gui/usbhxcfecfg_window.cxx
	$(CPP) -o $@ -c $< $(CFLAGS)

microintro.o : ../sources/gui/microintro/microintro.c
	$(CC) -o $@ -c $< $(CFLAGS)

lzw.o : ../sources/gui/microintro/packer/lzw.c
	$(CC) -o $@ -c $< $(CFLAGS)

pack.o : ../sources/gui/microintro/packer/pack.c
	$(CC) -o $@ -c $< $(CFLAGS)

rle.o : ../sources/gui/microintro/packer/rle.c
	$(CC) -o $@ -c $< $(CFLAGS)


clean:
	rm -rf *.o
	rm -rf *.so

mrproper: clean
	rm -rf $(EXEC)

.PHONY: clean mrproper
