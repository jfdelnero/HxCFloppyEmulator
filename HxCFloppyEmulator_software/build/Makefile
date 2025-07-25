#CC = i386-pc-linux-gcc
CC = gcc
BUILD_CC = gcc
CPP = g++

TARGET := $(shell uname)

BASEDIR = ../sources
INCLUDES = -I$(BASEDIR) -I ../../libhxcfe/sources -I $(BASEDIR)/gui -I $(BASEDIR)/thirdpartylibs/fltk/fltk-1.x.x/ -I ../../libusbhxcfe/sources -I ../../libhxcadaptor/sources -I ../../build -I $(BASEDIR)/gui/microintro/

FLTK1_4 ?= 0
DEBUG ?= 0
DEBUG_ASAN ?= 0
DEBUG_INFOS ?= 0

EM_PTHREAD ?= 1

EXEC = hxcfloppyemulator

ifeq ($(DEBUG), 1)
	CFLAGS = -O0 $(INCLUDES) -Wall -DDEBUG
	LDFLAGS =
	DEBUG_INFOS = 1
else
	CFLAGS = -O3 $(INCLUDES) -Wall
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

ifdef IPF_LIB_SUPPORT
	CFLAGS += -DIPF_SUPPORT
	DISKLOADERPLUGINS_IPF = ipf_loader.o libcapsimage.so.2.0
else
	DISKLOADERPLUGINS_IPF =
endif

LIBHXCFE = libhxcfe.so
LIBUSBHXCFE = libusbhxcfe.so

ifeq ($(TARGET), FreeBSD)
	CC = cc
	CPP = c++
	BUILD_CC = cc
	CFLAGS += -I/usr/local/include
	LDFLAGS += -lc -lm -ldl -lpthread -Wl,-rpath=.  -Wl,-Map,foo.map
endif

ifeq ($(TARGET), Linux)
	LDFLAGS += -lc -lm -ldl -lpthread -Wl,-rpath=.  -Wl,-Map,foo.map

	ifeq "$(shell echo '#include <pulse/pulseaudio.h>\nint main(){return 0;}' | $(CC) -xc -Wall -O -o /dev/null - 2>/dev/null && echo $$? )" "0"
		CFLAGS += -DLINUX_AUDIOSUPPORT=1
		LDFLAGS += -lpulse
	endif

endif

ifeq ($(findstring CYGWIN,$(shell uname)),CYGWIN)
	LIBHXCFE = libhxcfe.dll
	LIBUSBHXCFE = libusbhxcfe.dll
endif

ifeq ($(TARGET), mingw32)
	CC = i686-w64-mingw32-gcc
	CPP = i686-w64-mingw32-g++
	RESC = i686-w64-mingw32-windres
	LDFLAGS += -static-libgcc -static-libstdc++ -lwinmm -lws2_32
	LIBHXCFE = libhxcfe.dll
	LIBUSBHXCFE = libusbhxcfe.dll
	EXEC = HxCFloppyEmulator.exe
endif

ifeq ($(TARGET), mingw64)
	CC = x86_64-w64-mingw32-gcc
	CPP = x86_64-w64-mingw32-g++
	RESC = x86_64-w64-mingw32-windres
	LDFLAGS += -static-libgcc -static-libstdc++ -lwinmm -lws2_32
	LIBHXCFE = libhxcfe.dll
	LIBUSBHXCFE = libusbhxcfe.dll
	EXEC = HxCFloppyEmulator.exe
endif

ifeq ($(TARGET), Darwin)
	MACOSX_ARCH ?= -arch arm64 -arch x86_64
	MACOSX_MIN_VER ?= 10.9
	# For the GUI application, libraries will always be in the Frameworks directory.
	RPATHS = -Wl,-rpath,@executable_path/../Frameworks
	CFLAGS += -DOSX ${MACOSX_ARCH} -mmacosx-version-min=${MACOSX_MIN_VER}
	LDFLAGS += -lc -lm -ldl -lpthread -framework Cocoa -framework AudioToolBox -framework UniformTypeIdentifiers ${MACOSX_ARCH} -mmacosx-version-min=${MACOSX_MIN_VER} ${RPATHS}
	LIBHXCFE = libhxcfe.dylib
	LIBUSBHXCFE = libusbhxcfe.dylib
else
	LDFLAGS += $(shell $(BASEDIR)/thirdpartylibs/fltk/fltk-1.x.x/fltk-config --ldflags)
endif

ifeq ($(TARGET), Emscripten)
	CFLAGS += -I/usr/local/include
	LDFLAGS += -lc -lm -ldl
	ifeq ($(EM_PTHREAD), 1)
		CFLAGS += -pthread
		LDFLAGS += -pthread -sPTHREAD_POOL_SIZE=8 -sTOTAL_MEMORY=64MB
	endif
	CC = emcc
	CPP = em++
	BUILD_CC = cc
	LIBHXCFE = libhxcfe.a
	LIBUSBHXCFE = libusbhxcfe.a
	LDFLAGS += -sEXPORTED_FUNCTIONS="['_upload_file','_load_file_img','_main','cwrap','ccall','_malloc','_free','allocate', 'intArrayFromString']" -sSTACK_SIZE=262144 -sDEFAULT_PTHREAD_STACK_SIZE=262144 -lpthread -s WASM=1 -sALLOW_MEMORY_GROWTH -sASYNCIFY -sOFFSCREENCANVAS_SUPPORT=1 --bind
	EXEC = hxcfloppyemulator.html
endif

ifdef STANDALONEFSBROWSER_BUILD
	CFLAGS += -DSTANDALONEFSBROWSER

	EXEC = dosdiskbrowser

ifeq ($(TARGET), mingw64)
	EXEC = DosDiskBrowser.exe
endif

ifeq ($(TARGET), mingw32)
	EXEC = DosDiskBrowser.exe
endif

endif

FTDIFLOPPYEMU=usb_hxcfloppyemulator.o variablebitrate.o

GUI = main_gui.o msg_txt.o gui_strings.o about_gui.o batch_converter_window.o floppy_dump_window.o floppy_infos_window.o license_gui.o \
	log.o parameters.o rawfile_loader_window.o sdhxcfecfg_window.o usbhxcfecfg_window.o cb_batch_converter_window.o cb_floppy_dump_window.o \
	cb_floppy_infos_window.o cb_rawfile_loader_window.o cb_sdhxcfecfg_window.o cb_usbhxcfecfg_window.o cb_filesystem_generator_window.o \
	filesystem_generator_window.o cb_edittool_window.o edittool_window.o floppy_streamer_window.o cb_floppy_streamer_window.o fl_dnd_box.o

MICROINTRO = microintro.o lzw.o pack.o rle.o hxcmod.o
FLTKLIB = $(BASEDIR)/thirdpartylibs/fltk/fltk-1.x.x/lib/libfltk.a $(BASEDIR)/thirdpartylibs/fltk/fltk-1.x.x/lib/libfltk_images.a

all:
	$(MAKE) fltk
	$(MAKE) DATA_FILES
	$(MAKE) $(EXEC)

fltk:
	cd ../sources/thirdpartylibs/fltk/ && bash ./prepare_fltk.sh $(FLTK1_4)
ifeq ($(TARGET), mingw32)
		cd ../sources/thirdpartylibs/fltk/fltk-1.x.x/ && export LDFLAGS='-static-libstdc++ -static-libgcc' && ./configure --host=i686-w64-mingw32 --prefix=/usr/share/mingw-w64
		$(MAKE) -C ../sources/thirdpartylibs/fltk/fltk-1.x.x/ DIRS=src CC=i686-w64-mingw32-gcc CPP=i686-w64-mingw32-g++
else ifeq ($(TARGET), mingw64)
		cd ../sources/thirdpartylibs/fltk/fltk-1.x.x/ && export LDFLAGS='-static-libstdc++ -static-libgcc' && ./configure --host=x86_64-w64-mingw32 --prefix=/usr/share/mingw-w64
		$(MAKE) -C ../sources/thirdpartylibs/fltk/fltk-1.x.x/ DIRS=src CC=x86_64-w64-mingw32-gcc CPP=x86_64-w64-mingw32-g++
else ifeq ($(TARGET), Emscripten)
ifeq ($(EM_PTHREAD), 1)
	cd ../sources/thirdpartylibs/fltk/fltk-1.x.x/ && export LDFLAGS='-pthread -sSTACK_SIZE=262144 -sDEFAULT_PTHREAD_STACK_SIZE=262144 -sPTHREAD_POOL_SIZE=4 -sTOTAL_MEMORY=64MB -s WASM=1 -sALLOW_MEMORY_GROWTH -sASYNCIFY -sOFFSCREENCANVAS_SUPPORT=1 --bind' && emcmake cmake -DCMAKE_BUILD_TYPE=Release  -DCMAKE_VERBOSE_MAKEFILE:BOOL=OFF -DFLTK_BUILD_TEST:BOOL=OFF -DFLTK_CFLAGS="-pthread" -DCMAKE_CXX_FLAGS="-pthread" -DCMAKE_C_FLAGS="-pthread" -DFLTK_USE_PTHREADS:BOOL=ON -DFLTK_BUILD_FLUID:BOOL=OFF -DFLTK_BUILD_FLTK_OPTIONS:BOOL=OFF -DFLTK_BUILD_GL:BOOL=OFF && emmake make DIRS=src
else
	cd ../sources/thirdpartylibs/fltk/fltk-1.x.x/ && export LDFLAGS='-sSTACK_SIZE=262144 -sDEFAULT_PTHREAD_STACK_SIZE=262144 -s WASM=1 -sALLOW_MEMORY_GROWTH -sASYNCIFY -sOFFSCREENCANVAS_SUPPORT=1 --bind' && emcmake cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_VERBOSE_MAKEFILE:BOOL=OFF -DFLTK_BUILD_TEST:BOOL=OFF -DFLTK_USE_PTHREADS:BOOL=OFF -DFLTK_BUILD_FLUID:BOOL=OFF -DFLTK_BUILD_FLTK_OPTIONS:BOOL=OFF -DFLTK_BUILD_GL:BOOL=OFF && emmake make DIRS=src
endif
else ifeq ($(TARGET), Darwin)
ifneq (,$(findstring arm64,$(MACOSX_ARCH)))
		cd ../sources/thirdpartylibs/fltk && \
		cp -a fltk-1.x.x fltk-1.x.x-arm64 && \
		cd fltk-1.x.x-arm64 && \
		export CFLAGS='-mmacosx-version-min=11 -arch arm64' && export CXXFLAGS='-mmacosx-version-min=11 -arch arm64' && export LDFLAGS='-mmacosx-version-min=11 -arch arm64' && ./configure --host=arm-apple-darwin --enable-localpng=yes --enable-localzlib=yes
		$(MAKE) -C ../sources/thirdpartylibs/fltk/fltk-1.x.x-arm64/ DIRS=src
endif
ifneq (,$(findstring x86_64,$(MACOSX_ARCH)))
		cd ../sources/thirdpartylibs/fltk && \
		cp -a fltk-1.x.x fltk-1.x.x-x86_64 && \
		cd fltk-1.x.x-x86_64 && \
		export CFLAGS='-mmacosx-version-min=10.9 -arch x86_64' && export CXXFLAGS='-mmacosx-version-min=10.9 -arch x86_64' && export LDFLAGS='-mmacosx-version-min=10.9 -arch x86_64' && ./configure --host=x86_64-apple-darwin --enable-localpng=yes --enable-localzlib=yes
		$(MAKE) -C ../sources/thirdpartylibs/fltk/fltk-1.x.x-x86_64/ DIRS=src
endif
ifneq (,$(findstring x86_64,$(MACOSX_ARCH)))
ifneq (,$(findstring arm64,$(MACOSX_ARCH)))
		cd ../sources/thirdpartylibs/fltk && \
		lipo fltk-1.x.x-x86_64/lib/libfltk.a fltk-1.x.x-arm64/lib/libfltk.a -create -output fltk-1.x.x/lib/libfltk.a && \
		lipo fltk-1.x.x-x86_64/lib/libfltk_forms.a fltk-1.x.x-arm64/lib/libfltk_forms.a -create -output fltk-1.x.x/lib/libfltk_forms.a && \
		lipo fltk-1.x.x-x86_64/lib/libfltk_gl.a fltk-1.x.x-arm64/lib/libfltk_gl.a -create -output fltk-1.x.x/lib/libfltk_gl.a && \
		lipo fltk-1.x.x-x86_64/lib/libfltk_images.a fltk-1.x.x-arm64/lib/libfltk_images.a -create -output fltk-1.x.x/lib/libfltk_images.a
		-cd ../sources/thirdpartylibs/fltk && \
		cp fltk-1.x.x-x86_64/FL/abi-version.h fltk-1.x.x/FL/abi-version.h
		-cd ../sources/thirdpartylibs/fltk && \
		cp fltk-1.x.x-x86_64/FL/fl_config.h fltk-1.x.x/FL/fl_config.h
else
		cd ../sources/thirdpartylibs/fltk && \
		cp fltk-1.x.x-x86_64/lib/libfltk.a fltk-1.x.x/lib/libfltk.a && \
		cp fltk-1.x.x-x86_64/lib/libfltk_forms.a fltk-1.x.x/lib/libfltk_forms.a && \
		cp fltk-1.x.x-x86_64/lib/libfltk_gl.a fltk-1.x.x/lib/libfltk_gl.a && \
		cp fltk-1.x.x-x86_64/lib/libfltk_images.a fltk-1.x.x/lib/libfltk_images.a
		-cd ../sources/thirdpartylibs/fltk && \
		cp fltk-1.x.x-x86_64/FL/abi-version.h fltk-1.x.x/FL/abi-version.h
		-cd ../sources/thirdpartylibs/fltk && \
		cp fltk-1.x.x-x86_64/FL/fl_config.h fltk-1.x.x/FL/fl_config.h
endif
else
ifneq (,$(findstring arm64,$(MACOSX_ARCH)))
		cd ../sources/thirdpartylibs/fltk && \
		cp fltk-1.x.x-arm64/lib/libfltk.a fltk-1.x.x/lib/libfltk.a && \
		cp fltk-1.x.x-arm64/lib/libfltk_forms.a fltk-1.x.x/lib/libfltk_forms.a && \
		cp fltk-1.x.x-arm64/lib/libfltk_gl.a fltk-1.x.x/lib/libfltk_gl.a && \
		cp fltk-1.x.x-arm64/lib/libfltk_images.a fltk-1.x.x/lib/libfltk_images.a
		-cd ../sources/thirdpartylibs/fltk && \
		cp fltk-1.x.x-arm64/FL/abi-version.h fltk-1.x.x/FL/abi-version.h
		-cd ../sources/thirdpartylibs/fltk && \
		cp fltk-1.x.x-arm64/FL/fl_config.h fltk-1.x.x/FL/fl_config.h
endif
endif
else
	cd ../sources/thirdpartylibs/fltk/fltk-1.x.x/ && ./autogen.sh && ./configure && $(MAKE)
endif

DATA_FILES: bmptoh
	cd $(BASEDIR)/gui/microintro/data/ && ../converttools/bmptob8/bmptoh sob.bmp -BMP8P
	cd $(BASEDIR)/gui/microintro/data/ && ../converttools/bmptob8/bmptoh hxc2001.bmp -BMP8P
	cd $(BASEDIR)/gui/microintro/data/ && ../converttools/bmptob8/bmptoh hxc2001_2.bmp -BMP8P
	cd $(BASEDIR)/gui/microintro/data/ && ../converttools/bmptob8/bmptoh hxc2001_backgnd.bmp -BMP8P
	cd $(BASEDIR)/gui/microintro/data/ && ../converttools/bmptob8/bmptoh pauline.bmp -BMP8P
	cd $(BASEDIR)/gui/microintro/data/ && ../converttools/bmptob8/bmptoh zandax_supplydas_booze.mod -DATA
	cd $(BASEDIR)/gui/microintro/data/ && ../converttools/bmptob8/bmptoh maktone_class_cracktro15.mod -DATA
	cd $(BASEDIR)/gui/microintro/data/ && ../converttools/bmptob8/bmptoh jozz_cognition.mod -DATA
	cd $(BASEDIR)/gui/microintro/data/ && ../converttools/bmptob8/bmptoh vim_not_again.mod -DATA
	cd $(BASEDIR)/gui/microintro/data/ && ../converttools/bmptob8/bmptoh meo_sleeping_waste.mod -DATA
	cd $(BASEDIR)/gui/microintro/data/ && ../converttools/bmptob8/bmptoh physical_presence.mod -DATA
	cd $(BASEDIR)/gui/microintro/data/ && ../converttools/bmptob8/bmptoh jason_shortness.mod -DATA
	cd $(BASEDIR)/gui/microintro/data/ && ../converttools/bmptob8/bmptoh nao_gabber_astro.mod -DATA
	cd $(BASEDIR)/gui/microintro/data/ && ../converttools/bmptob8/bmptoh ../../../../COPYING_FULL -DATA

HxCFloppyEmulator.exe: win32_hxcfloppy_res.o main.o utils.o $(GUI) $(MICROINTRO) loader.o soft_cfg_file.o fileselector.o $(FLTKLIB) $(LIBHXCFE) $(LIBUSBHXCFE) ../../build/libhxcadaptor.a
	$(CPP) -o $@    $^ $(LDFLAGS)
	cp $@ ../../build

dosdiskbrowser: main.o utils.o $(GUI) $(MICROINTRO) loader.o soft_cfg_file.o fileselector.o $(FLTKLIB) $(LIBHXCFE) ../../build/libhxcadaptor.a
	$(CPP) -o $@    $^ $(LDFLAGS)
	cp $@ ../../build
	
DosDiskBrowser.exe: win32_hxcfloppy_res.o main.o utils.o $(GUI) $(MICROINTRO) loader.o soft_cfg_file.o fileselector.o $(FLTKLIB) $(LIBHXCFE) $(LIBUSBHXCFE) ../../build/libhxcadaptor.a
	$(CPP) -o $@    $^ $(LDFLAGS)
	cp $@ ../../build

hxcfloppyemulator: main.o utils.o $(GUI) $(MICROINTRO) loader.o soft_cfg_file.o fileselector.o $(FLTKLIB) $(LIBHXCFE) $(LIBUSBHXCFE) ../../build/libhxcadaptor.a
	$(CPP) -o $@    $^ $(LDFLAGS)
	cp $@ ../../build

hxcfloppyemulator.html: main.o utils.o $(GUI) $(MICROINTRO) loader.o soft_cfg_file.o fileselector.o $(FLTKLIB) $(LIBHXCFE) $(LIBUSBHXCFE) ../../build/libhxcadaptor.a
	$(CPP) -o $@    $^ $(LDFLAGS)
	mkdir -p ../../build/wasm/
	cp $@ *.html *.js *.wasm ../sources/gui/wasm/*.js ../sources/gui/wasm/*.html ../../build/wasm/

$(LIBHXCFE): ../../build/$(LIBHXCFE)
	ln ../../build/$(LIBHXCFE)  $(LIBHXCFE)

$(LIBUSBHXCFE): ../../build/$(LIBUSBHXCFE)
	ln ../../build/$(LIBUSBHXCFE)  $(LIBUSBHXCFE)

win32_hxcfloppy_res.o: $(BASEDIR)/win32/hxcfloppy.rc
	$(RESC) $< $@

main.o: $(BASEDIR)/main.cxx
	$(CPP) -o $@ -c $< $(CFLAGS)

utils.o: $(BASEDIR)/utils.cxx
	$(CPP) -o $@ -c $< $(CFLAGS)

loader.o: $(BASEDIR)/loader.cxx
	$(CPP) -o $@ -c $< $(CFLAGS)

soft_cfg_file.o: $(BASEDIR)/soft_cfg_file.cxx
	$(CPP) -o $@ -c $< $(CFLAGS)

about_gui.o : $(BASEDIR)/gui/about_gui.cxx
	$(CPP) -o $@ -c $< $(CFLAGS)

batch_converter_window.o : $(BASEDIR)/gui/batch_converter_window.cxx
	$(CPP) -o $@ -c $< $(CFLAGS)

cb_batch_converter_window.o : $(BASEDIR)/gui/cb_batch_converter_window.cxx
	$(CPP) -o $@ -c $< $(CFLAGS)

cb_floppy_dump_window.o : $(BASEDIR)/gui/cb_floppy_dump_window.cxx
	$(CPP) -o $@ -c $< $(CFLAGS)

cb_floppy_infos_window.o : $(BASEDIR)/gui/cb_floppy_infos_window.cxx
	$(CPP) -o $@ -c $< $(CFLAGS)

cb_floppy_streamer_window.o : $(BASEDIR)/gui/cb_floppy_streamer_window.cxx
	$(CPP) -o $@ -c $< $(CFLAGS)

cb_edittool_window.o : $(BASEDIR)/gui/cb_edittool_window.cxx
	$(CPP) -o $@ -c $< $(CFLAGS)

cb_rawfile_loader_window.o : $(BASEDIR)/gui/cb_rawfile_loader_window.cxx
	$(CPP) -o $@ -c $< $(CFLAGS)

cb_sdhxcfecfg_window.o : $(BASEDIR)/gui/cb_sdhxcfecfg_window.cxx
	$(CPP) -o $@ -c $< $(CFLAGS)

cb_usbhxcfecfg_window.o : $(BASEDIR)/gui/cb_usbhxcfecfg_window.cxx
	$(CPP) -o $@ -c $< $(CFLAGS)

cb_filesystem_generator_window.o : $(BASEDIR)/gui/cb_filesystem_generator_window.cxx
	$(CPP) -o $@ -c $< $(CFLAGS)

fileselector.o : $(BASEDIR)/gui/fileselector.cxx
	$(CPP) -o $@ -c $< $(CFLAGS)

filesystem_generator_window.o : $(BASEDIR)/gui/filesystem_generator_window.cxx
	$(CPP) -o $@ -c $< $(CFLAGS)

fl_dnd_box.o : $(BASEDIR)/gui/fl_dnd_box.cxx
	$(CPP) -o $@ -c $< $(CFLAGS)

floppy_dump_window.o : $(BASEDIR)/gui/floppy_dump_window.cxx
	$(CPP) -o $@ -c $< $(CFLAGS)

floppy_infos_window.o : $(BASEDIR)/gui/floppy_infos_window.cxx
	$(CPP) -o $@ -c $< $(CFLAGS)

floppy_streamer_window.o : $(BASEDIR)/gui/floppy_streamer_window.cxx
	$(CPP) -o $@ -c $< $(CFLAGS)

edittool_window.o : $(BASEDIR)/gui/edittool_window.cxx
	$(CPP) -o $@ -c $< $(CFLAGS)

license_gui.o : $(BASEDIR)/gui/license_gui.cxx
	$(CPP) -o $@ -c $< $(CFLAGS)

log.o : $(BASEDIR)/gui/log.cxx
	$(CPP) -o $@ -c $< $(CFLAGS)

parameters.o : $(BASEDIR)/gui/parameters.cxx
	$(CPP) -o $@ -c $< $(CFLAGS)

main_gui.o : $(BASEDIR)/gui/main_gui.cxx
	$(CPP) -o $@ -c $< $(CFLAGS)

msg_txt.o : $(BASEDIR)/gui/msg_txt.cxx
	$(CPP) -o $@ -c $< $(CFLAGS)

gui_strings.o : $(BASEDIR)/gui/gui_strings.cxx
	$(CPP) -o $@ -c $< $(CFLAGS)

rawfile_loader_window.o : $(BASEDIR)/gui/rawfile_loader_window.cxx
	$(CPP) -o $@ -c $< $(CFLAGS)

sdhxcfecfg_window.o : $(BASEDIR)/gui/sdhxcfecfg_window.cxx
	$(CPP) -o $@ -c $< $(CFLAGS)

usbhxcfecfg_window.o : $(BASEDIR)/gui/usbhxcfecfg_window.cxx
	$(CPP) -o $@ -c $< $(CFLAGS)

microintro.o : $(BASEDIR)/gui/microintro/microintro.c
	$(CC) -o $@ -c $< $(CFLAGS)

hxcmod.o : $(BASEDIR)/gui/microintro/hxcmod.c
	$(CC) -o $@ -c $< $(CFLAGS)

lzw.o : $(BASEDIR)/gui/microintro/packer/lzw.c
	$(CC) -o $@ -c $< $(CFLAGS)

pack.o : $(BASEDIR)/gui/microintro/packer/pack.c
	$(CC) -o $@ -c $< $(CFLAGS)

rle.o : $(BASEDIR)/gui/microintro/packer/rle.c
	$(CC) -o $@ -c $< $(CFLAGS)

bmptoh:
	cd $(BASEDIR)/gui/microintro/converttools/bmptob8 && $(MAKE) CC=${BUILD_CC}

clean:
	rm -rf *.a
	rm -rf *.wasm
	rm -rf *.js
	rm -rf *.html
	rm -rf *.o
	rm -rf *.so
	rm -rf *.dll
	rm -rf *.map

clean_fltk:
ifneq ("$(wildcard ../sources/thirdpartylibs/fltk/fltk-1.x.x/Makefile)","")
	$(MAKE) -C ../sources/thirdpartylibs/fltk/fltk-1.x.x/ clean
endif

mrproper: clean
	rm -rf $(EXEC)
	rm -rf *.map *.ncb *.opt *.plg *.dll *.exe *.bak *.dylib
	$(MAKE) -C $(BASEDIR)/gui/microintro/converttools/bmptob8 mrproper
	-rm -rf ../sources/thirdpartylibs/fltk/fltk-1.x.x
	-rm -rf ../sources/thirdpartylibs/fltk/fltk-1.x.x-arm64
	-rm -rf ../sources/thirdpartylibs/fltk/fltk-1.x.x-x86_64

.PHONY: clean mrproper
