#CC=i386-pc-linux-gcc
CC = gcc
AR = ar

TARGET := $(shell uname)

BASEDIR = ../sources
INCLUDES = -I$(BASEDIR)/ -I ../../libhxcfe/sources -I ../sources/win32 -I ../../libhxcadaptor/sources -I ../../build

DEBUG ?= 0
DEBUG_ASAN ?= 0
DEBUG_INFOS ?= 0

ifeq ($(DEBUG), 1)
	CFLAGS = -O0 $(INCLUDES) -Wall -DDEBUG
	LDFLAGS = -shared
	DEBUG_INFOS = 1
else
	CFLAGS = -O3 $(INCLUDES) -Wall
	LDFLAGS = -shared
endif

ifeq ($(DEBUG_ASAN), 1)
	CFLAGS += -fsanitize=address -fno-omit-frame-pointer
	LDFLAGS += -static-libasan -fsanitize=address
endif

ifeq ($(DEBUG_INFOS), 1)
	CFLAGS += -g
else
	LDFLAGS += -s
endif

EXEC=libusbhxcfe.so

ifeq ($(TARGET), FreeBSD)
	CC = cc
	CFLAGS += -fPIC -Wl,-Map,foo.map
	LDFLAGS += -lc -lm  -ldl
endif

ifeq ($(TARGET), Linux)
	CFLAGS += -fPIC -Wl,-Map,foo.map
	LDFLAGS += -lc -lm  -ldl
endif

ifeq ($(TARGET), mingw32)
	CC = i686-w64-mingw32-gcc
	AR = i686-w64-mingw32-ar
	RESC = i686-w64-mingw32-windres
	LDFLAGS += -static-libgcc ../sources/win32/libusbhxcfe.def
	EXEC = libusbhxcfe.dll
endif

ifeq ($(TARGET), mingw64)
	CC = x86_64-w64-mingw32-gcc
	AR = x86_64-w64-mingw32-ar
	RESC = x86_64-w64-mingw32-windres
	LDFLAGS += -static-libgcc ../sources/win32/libusbhxcfe.def
	EXEC = libusbhxcfe.dll
endif

ifeq ($(TARGET), Darwin)
	MACOSX_ARCH ?= -arch arm64 -arch x86_64
	MACOSX_MIN_VER ?= 10.9
	CFLAGS += ${MACOSX_ARCH} -mmacosx-version-min=${MACOSX_MIN_VER}
	LDFLAGS +=  -lc -lm -ldl ${MACOSX_ARCH} -dynamiclib -current_version 2.0 -install_name @executable_path/../Frameworks/libusbhxcfe.dylib -mmacosx-version-min=${MACOSX_MIN_VER}
	EXEC = libusbhxcfe.dylib
endif

ifeq ($(findstring CYGWIN,$(shell uname)),CYGWIN)
	EXEC = libusbhxcfe.dll
endif

all: $(EXEC)

libusbhxcfe.dll: win32_libusbhxcfe_res.o usb_hxcfloppyemulator.o variablebitrate.o ftdi.o ../../build/libhxcadaptor.a ../../build/libhxcfe.dll
	$(CC) -o $@    $^ $(LDFLAGS)
	cp $@ ../../build

libusbhxcfe.dylib: usb_hxcfloppyemulator.o variablebitrate.o ftdi.o ../../build/libhxcadaptor.a ../../build/libhxcfe.dylib
	$(CC) -o $@    $^ $(LDFLAGS)
	cp $@ ../../build

libusbhxcfe.a: usb_hxcfloppyemulator.o variablebitrate.o ftdi.o
ifeq ($(TARGET), Darwin)
	libtool -o $@     $^
else
	$(AR) r $@    $^
endif
	cp $@ ../../build

libusbhxcfe.so: usb_hxcfloppyemulator.o variablebitrate.o ftdi.o
	$(CC) -o $@    $^ $(LDFLAGS)
	cp $@ ../../build

win32_libusbhxcfe_res.o: $(BASEDIR)/win32/libusbhxcfe.rc
	$(RESC) $< $@

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
	rm -rf *.dll
	rm -rf *.dylib

.PHONY: clean mrproper
