TARGET := $(shell uname)

all: HxCFloppyEmulator_cmdline HxCFloppyEmulator_software

libhxcadaptor:
	$(MAKE) -C ../libhxcadaptor/trunk/build/

libhxcfe: libhxcadaptor
	$(MAKE) -C ../libhxcfe/trunk/build/

libusbhxcfe: libhxcadaptor
	$(MAKE) -C ../libusbhxcfe/trunk/build/

HxCFloppyEmulator_cmdline: libhxcfe libusbhxcfe
	$(MAKE) -C ../HxCFloppyEmulator_cmdline/trunk/build/

fltk:
	chmod +x ../HxCFloppyEmulator_software/trunk/sources/thirdpartylibs/fltk/prepare_fltk.sh
	cd ../HxCFloppyEmulator_software/trunk/sources/thirdpartylibs/fltk/ && ./prepare_fltk.sh
ifeq ($(TARGET), mingw32)
		cd ../HxCFloppyEmulator_software/trunk/sources/thirdpartylibs/fltk/fltk-1.x.x/ && export LDFLAGS='-static-libstdc++ -static-libgcc' && ./configure --host=i686-w64-mingw32 --prefix=/usr/share/mingw-w64
		$(MAKE) -C ../HxCFloppyEmulator_software/trunk/sources/thirdpartylibs/fltk/fltk-1.x.x/ DIRS=src CC=i686-w64-mingw32-gcc CPP=i686-w64-mingw32-g++
else ifeq ($(TARGET), mingw64)
		cd ../HxCFloppyEmulator_software/trunk/sources/thirdpartylibs/fltk/fltk-1.x.x/ && export LDFLAGS='-static-libstdc++ -static-libgcc' && ./configure --host=x86_64-w64-mingw32 --prefix=/usr/share/mingw-w64
		$(MAKE) -C ../HxCFloppyEmulator_software/trunk/sources/thirdpartylibs/fltk/fltk-1.x.x/ DIRS=src CC=x86_64-w64-mingw32-gcc CPP=x86_64-w64-mingw32-g++
else
		$(MAKE) -C ../HxCFloppyEmulator_software/trunk/sources/thirdpartylibs/fltk/fltk-1.x.x/ DIRS=src
endif

HxCFloppyEmulator_software: libhxcfe libusbhxcfe fltk
	$(MAKE) -C ../HxCFloppyEmulator_software/trunk/build/

clean_fltk:
ifneq ("$(wildcard ../HxCFloppyEmulator_software/trunk/sources/thirdpartylibs/fltk/fltk-1.x.x/Makefile)","")
	$(MAKE) -C ../HxCFloppyEmulator_software/trunk/sources/thirdpartylibs/fltk/fltk-1.x.x/ clean
endif

clean: clean_fltk
	$(MAKE) -C ../libhxcadaptor/trunk/build/ clean
	$(MAKE) -C ../libhxcfe/trunk/build/ clean
	$(MAKE) -C ../libusbhxcfe/trunk/build/ clean
	$(MAKE) -C ../HxCFloppyEmulator_cmdline/trunk/build/ clean
	$(MAKE) -C ../HxCFloppyEmulator_software/trunk/build/ clean

mrproper:
	$(MAKE) -C ../libhxcadaptor/trunk/build/ mrproper
	$(MAKE) -C ../libhxcfe/trunk/build/ mrproper
	$(MAKE) -C ../libusbhxcfe/trunk/build/ mrproper
	$(MAKE) -C ../HxCFloppyEmulator_cmdline/trunk/build/ mrproper
	$(MAKE) -C ../HxCFloppyEmulator_software/trunk/build/ mrproper
	-rm -rf *.so
	-rm -rf *.a
	-rm -rf *.obj
	-rm -rf *.opt
	-rm -rf *.dll
	-rm -rf *.exe
	-rm -rf *.lib
	-rm -rf *.ncb
	-rm -rf *.exp
	-rm -rf *.dylib
	-rm -rf *.dmg
	-rm -rf *.app
	-rm -rf hxcfe_cmdline
	-rm -rf hxcfloppyemulator
	-rm -f  hxcfe
	-rm -rf Debug_*
	-rm -rf Release_*
	-rm -rf ADFOpus_*
	-rm -rf ../HxCFloppyEmulator_software/trunk/sources/thirdpartylibs/fltk/fltk-1.x.x

.PHONY: libhxcadaptor libhxcfe libusbhxcfe HxCFloppyEmulator_cmdline HxCFloppyEmulator_software fltk mrproper clean
