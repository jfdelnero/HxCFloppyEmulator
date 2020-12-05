#CC=i386-pc-linux-gcc
CC=gcc
AR=ar

TARGET := $(shell uname)

BASEDIR=../sources
INCLUDES = -I$(BASEDIR)/ -I ../../../libhxcfe/trunk/sources -I ../../../libusbhxcfe/trunk/sources

DEBUG ?= 0
DEBUG_ASAN ?= 0

ifeq ($(DEBUG), 1)
	CFLAGS=-O0 $(INCLUDES) -Wall -g -DDEBUG
	LDFLAGS= -shared
else
	CFLAGS=-O3 $(INCLUDES) -Wall
	LDFLAGS= -shared -s
endif

ifeq ($(DEBUG_ASAN), 1)
	CFLAGS += -fsanitize=address -fno-omit-frame-pointer
	LDFLAGS += -static-libasan -fsanitize=address
endif

ifeq ($(TARGET), Linux)
CFLAGS += -fPIC -Wl,-Map,foo.map
LDFLAGS += -lc -lm -ldl
endif

ifeq ($(TARGET), mingw32)
CC=i686-w64-mingw32-gcc
LDFLAGS += -lc -lm -ldl
endif

ifeq ($(TARGET), mingw64)
CC=x86_64-w64-mingw32-gcc
LDFLAGS += -lc -lm -ldl
endif

ifeq ($(TARGET), Darwin)
MACOSX_MIN_VER ?= 10.13
CFLAGS += -arch x86_64 -mmacosx-version-min=${MACOSX_MIN_VER}
LDFLAGS += -lc -lm -ldl -arch x86_64 -mmacosx-version-min=${MACOSX_MIN_VER}
endif

EXEC=libhxcadaptor.a

all: $(EXEC)

libhxcadaptor.a: libhxcadaptor.o fs.o network.o
	$(AR) r $@    $^
	cp $@ ../../../build

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
