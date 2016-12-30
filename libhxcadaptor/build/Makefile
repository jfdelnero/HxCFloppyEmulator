#CC=i386-pc-linux-gcc
CC=gcc
AR=ar

UNAME := $(shell uname)

BASEDIR=../sources
INCLUDES = -I$(BASEDIR)/ -I ../../../libhxcfe/trunk/sources -I ../../../libusbhxcfe/trunk/sources

DEBUG ?= 0

ifeq ($(DEBUG), 1)
	CFLAGS=-O0 $(INCLUDES) -Wall -g -DDEBUG -fsanitize=address -fno-omit-frame-pointer
	LDFLAGS= -shared -lc -lm -ldl -lasan
else
	CFLAGS=-O3 $(INCLUDES) -Wall
	LDFLAGS= -shared -lc -lm -ldl -s 
endif

ifeq ($(UNAME), Linux)
CFLAGS += -fPIC -Wl,-Map,foo.map
endif

ifeq ($(UNAME), Darwin)
CFLAGS += -arch i386 -mmacosx-version-min=10.5
LDFLAGS += -arch i386 -mmacosx-version-min=10.5
endif

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
