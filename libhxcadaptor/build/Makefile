#CC=i386-pc-linux-gcc
CC = gcc
AR = ar

TARGET := $(shell uname)

BASEDIR = ../sources
INCLUDES = -I$(BASEDIR)/ -I ../../libhxcfe/sources -I ../../libusbhxcfe/sources -I ../../build

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

ifeq ($(TARGET), FreeBSD)
	CFLAGS += -fPIC -Wl,-Map,foo.map
	LDFLAGS += -lc -lm -ldl
	CC = cc
endif

ifeq ($(TARGET), Linux)
	CFLAGS += -fPIC -Wl,-Map,foo.map
	LDFLAGS += -lc -lm -ldl
endif

ifeq ($(TARGET), mingw32)
	CC = i686-w64-mingw32-gcc
	AR = i686-w64-mingw32-ar
	LDFLAGS += -lc -lm -ldl
endif

ifeq ($(TARGET), mingw64)
	CC = x86_64-w64-mingw32-gcc
	AR = x86_64-w64-mingw32-ar
	LDFLAGS += -lc -lm -ldl
endif

ifeq ($(TARGET), Darwin)
	MACOSX_ARCH ?= -arch arm64 -arch x86_64
	MACOSX_MIN_VER ?= 10.9
	CFLAGS += ${MACOSX_ARCH} -mmacosx-version-min=${MACOSX_MIN_VER}
	LDFLAGS += -lc -lm -ldl ${MACOSX_ARCH} -mmacosx-version-min=${MACOSX_MIN_VER}
endif

EXEC = libhxcadaptor.a

all: $(EXEC)

libhxcadaptor.a: libhxcadaptor.o fs.o network.o
ifeq ($(TARGET), Darwin)
	libtool -o $@     $^
else
	$(AR) r $@    $^
endif
	cp $@ ../../build

libhxcadaptor.o: $(BASEDIR)/libhxcadaptor.c
	$(CC) -o $@ -c $< $(CFLAGS)

network.o: $(BASEDIR)/network.c
	$(CC) -o $@ -c $< $(CFLAGS)

fs.o: $(BASEDIR)/fs.c
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	rm -rf *.o

mrproper: clean
	rm -rf $(EXEC)

.PHONY: clean mrproper
