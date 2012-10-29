#CC=i386-pc-linux-gcc
CC=gcc

UNAME := $(shell uname)

CFLAGS=-O3 -I ../sources -I ../../../libhxcfe/trunk/sources -I ../../../libusbhxcfe/trunk/sources

LDFLAGS=-lc -lm  -ldl  -lpthread

ifeq ($(UNAME), Darwin)
CFLAGS += -arch i386
LDFLAGS += -arch i386
endif

EXEC=hxcfe

all: $(EXEC)

ifeq ($(UNAME), Darwin)
hxcfe:      hxcfe.o ../../../build/libhxcfe.dylib ../../../build/libhxcadaptor.a ../../../build/libusbhxcfe.a
	$(CC) -o $@    $^ $(LDFLAGS)
	cp $@ ../../../build
else
hxcfe:      hxcfe.o ../../../build/libhxcfe.o ../../../build/libhxcadaptor.a ../../../build/libusbhxcfe.a
	$(CC) -o $@    $^ $(LDFLAGS)
	cp $@ ../../../build
endif

hxcfe.o: ../sources/hxcfe.c 
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	rm -rf *.o

mrproper: clean
	rm -rf $(EXEC)

.PHONY: clean mrproper
