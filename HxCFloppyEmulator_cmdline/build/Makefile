#CC=i386-pc-linux-gcc
CC=gcc

ifdef IPF_LIB_SUPPORT
    CFLAGS=-O3 -I ../../../libhxcfe/trunk/sources -I .
else
    CFLAGS=-O3 -I ../sources -I ../../../libhxcfe/trunk/sources
endif

LDFLAGS=-lc -lm  -ldl  -lpthread
EXEC=hxcfe

all: $(EXEC)

hxcfeconv:      hxcfe.o ../../../build/libhxcfe.a ../../../build/libhxcadaptor.a
	$(CC) -o $@    $^ $(LDFLAGS)
	cp $@ ../../../build

hxcfe:      hxcfe.o ../../../build/libhxcfe.a ../../../build/libhxcadaptor.a
	$(CC) -o $@    $^ $(LDFLAGS) -DFTDILIB
	cp $@ ../../../build

hxcfe.o: ../sources/hxcfe.c 
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	rm -rf *.o

mrproper: clean
	rm -rf $(EXEC)

.PHONY: clean mrproper
