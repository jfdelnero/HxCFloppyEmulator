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

hxcfe:      hxcfe.o ../../../build/libhxcfe.a ../../../build/libhxcadaptor.a ../../../build/libusbhxcfe.a
	$(CC) -o $@    $^ $(LDFLAGS)
	cp $@ ../../../build

hxcfe.o: ../sources/hxcfe.c 
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	rm -rf *.o

mrproper: clean
	rm -rf $(EXEC)

.PHONY: clean mrproper
