#CC=i386-pc-linux-gcc
CC=gcc

UNAME := $(shell uname)

INCLUDES = -I ../sources -I ../../../libhxcfe/trunk/sources -I ../../../libusbhxcfe/trunk/sources

DEBUG ?= 0

ifeq ($(DEBUG), 1)
	CFLAGS=-O0 $(INCLUDES) -Wall -g -DDEBUG -fsanitize=address -fno-omit-frame-pointer
	LDFLAGS= -lc -lm -ldl -lpthread -lasan
else
	CFLAGS=-O3 $(INCLUDES) -Wall
	LDFLAGS= -lc -lm -ldl -lpthread -s
endif

LIBHXCFE = libhxcfe.so
LIBUSBHXCFE = libusbhxcfe.so

ifeq ($(UNAME), Darwin)
CFLAGS += -arch i386 -mmacosx-version-min=10.5
LDFLAGS += -arch i386 -mmacosx-version-min=10.5
LIBHXCFE= libhxcfe.dylib
LIBUSBHXCFE= libusbhxcfe.dylib
endif

ifeq ($(UNAME), Linux)
LDFLAGS += -Wl,-rpath=. -Wl,-Map,foo.map
endif

ifeq ($(findstring CYGWIN,$(shell uname)),CYGWIN)
LIBHXCFE= libhxcfe.dll
LIBUSBHXCFE= libusbhxcfe.dll
endif

EXEC=hxcfe

all: $(EXEC)

hxcfe:  hxcfe.o utils.o ../../../build/libhxcadaptor.a $(LIBHXCFE) $(LIBUSBHXCFE)
	$(CC) -o $@    $^ $(LDFLAGS)
	cp $@ ../../../build

$(LIBHXCFE): ../../../build/$(LIBHXCFE)
	ln ../../../build/$(LIBHXCFE)  $(LIBHXCFE)

$(LIBUSBHXCFE): ../../../build/$(LIBUSBHXCFE)
	ln ../../../build/$(LIBUSBHXCFE)  $(LIBUSBHXCFE)

hxcfe.o: ../sources/hxcfe.c 
	$(CC) -o $@ -c $< $(CFLAGS)

utils.o: ../sources/utils.c
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	rm -rf *.o
	rm -rf *.so

mrproper: clean
	rm -rf $(EXEC)

.PHONY: clean mrproper
