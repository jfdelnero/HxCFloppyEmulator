#CC=i386-pc-linux-gcc
CC=gcc

UNAME := $(shell uname)

CFLAGS=-O3 -I ../sources -I ../../../libhxcfe/trunk/sources -I ../../../libusbhxcfe/trunk/sources

LDFLAGS=-lc -lm  -ldl  -lpthread
LIBHXCFE= ../../../build/libhxcfe.so
LIBUSBHXCFE= ../../../build/libusbhxcfe.so

ifeq ($(UNAME), Darwin)
CFLAGS += -arch i386
LDFLAGS += -arch i386
LIBHXCFE= ../../../build/libhxcfe.dylib
LIBUSBHXCFE= ../../../build/libusbhxcfe.dylib
endif

ifeq ($(findstring CYGWIN,$(shell uname)),CYGWIN)
LIBHXCFE= ../../../build/libhxcfe.dll
LIBUSBHXCFE= ../../../build/libusbhxcfe.dll
endif

EXEC=hxcfe

all: $(EXEC)

hxcfe:      hxcfe.o ../../../build/libhxcadaptor.a $(LIBHXCFE) $(LIBUSBHXCFE)
	$(CC) -o $@    $^ $(LDFLAGS)
	cp $@ ../../../build

hxcfe.o: ../sources/hxcfe.c 
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	rm -rf *.o

mrproper: clean
	rm -rf $(EXEC)

.PHONY: clean mrproper
