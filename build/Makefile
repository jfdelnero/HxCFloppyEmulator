#CC=i386-pc-linux-gcc
CC=gcc
AR=ar

BASEDIR=../sources
INCLUDES = -I$(BASEDIR)/ -I ../../../libhxcfe/trunk/sources -I ../../../libusbhxcfe/trunk/sources

CFLAGS=-O3 $(INCLUDES)

LDFLAGS= -g -shared -lc -lm  -ldl
EXEC=libhxcadaptor.a

all: $(EXEC)

libhxcadaptor.a: libhxcadaptor.o fs.o
	$(AR) r $@    $^
	cp $@ ../../../build

libhxcadaptor.o: $(BASEDIR)/libhxcadaptor.c
	$(CC) -o $@ -c $< $(CFLAGS)

fs.o: $(BASEDIR)/fs.c
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	rm -rf *.o

mrproper: clean
	rm -rf $(EXEC)

.PHONY: clean mrproper
