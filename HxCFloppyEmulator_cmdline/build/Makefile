#CC=i386-pc-linux-gcc
CC = gcc

TARGET := $(shell uname)

INCLUDES = -I ../sources -I ../../libhxcfe/sources -I ../../libusbhxcfe/sources -I ../../build

DEBUG ?= 0
DEBUG_ASAN ?= 0
DEBUG_INFOS ?= 0

ifeq ($(DEBUG), 1)
	CFLAGS =-O0 $(INCLUDES) -Wall -DDEBUG
	LDFLAGS =
	DEBUG_INFOS = 1
else
	CFLAGS =-O3 $(INCLUDES) -Wall
	LDFLAGS =
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

EXEC = hxcfe
LIBHXCFE = libhxcfe.so
LIBUSBHXCFE = libusbhxcfe.so

ifeq ($(TARGET), Darwin)
	MACOSX_ARCH ?= -arch arm64 -arch x86_64
	MACOSX_MIN_VER ?= 10.9
	CFLAGS += ${MACOSX_ARCH} -mmacosx-version-min=${MACOSX_MIN_VER}
	LDFLAGS += -lc -lm -ldl -lpthread ${MACOSX_ARCH} -mmacosx-version-min=${MACOSX_MIN_VER}
	LIBHXCFE = libhxcfe.dylib
	LIBUSBHXCFE = libusbhxcfe.dylib
endif

ifeq ($(TARGET), FreeBSD)
	CC = cc
	LDFLAGS += -lc -lm -ldl -lpthread -Wl,-rpath=. -Wl,-Map,foo.map
endif

ifeq ($(TARGET), Linux)
	LDFLAGS += -lc -lm -ldl -lpthread -Wl,-rpath=. -Wl,-Map,foo.map
endif

ifeq ($(TARGET), mingw32)
	CC = i686-w64-mingw32-gcc
	LDFLAGS += -static-libgcc
	EXEC = hxcfe.exe
	LIBHXCFE = libhxcfe.dll
	LIBUSBHXCFE = libusbhxcfe.dll
endif

ifeq ($(TARGET), mingw64)
	CC = x86_64-w64-mingw32-gcc
	LDFLAGS += -static-libgcc
	EXEC = hxcfe.exe
	LIBHXCFE = libhxcfe.dll
	LIBUSBHXCFE = libusbhxcfe.dll
endif

ifeq ($(findstring CYGWIN,$(shell uname)),CYGWIN)
	LIBHXCFE = libhxcfe.dll
	LIBUSBHXCFE = libusbhxcfe.dll
	LDFLAGS += -lc -lm -ldl -lpthread
endif

all: $(EXEC)

hxcfe.exe:  hxcfe.o utils.o ../../build/libhxcadaptor.a $(LIBHXCFE) $(LIBUSBHXCFE)
	$(CC) -o $@    $^ $(LDFLAGS)
	cp $@ ../../build

hxcfe:  hxcfe.o utils.o ../../build/libhxcadaptor.a $(LIBHXCFE) $(LIBUSBHXCFE)
	$(CC) -o $@    $^ $(LDFLAGS)
	cp $@ ../../build

$(LIBHXCFE): ../../build/$(LIBHXCFE)
	ln ../../build/$(LIBHXCFE)  $(LIBHXCFE)

$(LIBUSBHXCFE): ../../build/$(LIBUSBHXCFE)
	ln ../../build/$(LIBUSBHXCFE)  $(LIBUSBHXCFE)

hxcfe.o: ../sources/hxcfe.c
	$(CC) -o $@ -c $< $(CFLAGS)

utils.o: ../sources/utils.c
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	rm -rf *.o
	rm -rf *.so
	rm -rf *.dll
	rm -rf *.exe
	rm -rf *.map

mrproper: clean
	rm -rf $(EXEC)
	rm -rf *.dylib

.PHONY: clean mrproper
