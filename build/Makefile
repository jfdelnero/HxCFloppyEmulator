TARGET := $(shell uname)

all: HxCFloppyEmulator_cmdline HxCFloppyEmulator_software

libhxcadaptor:
	$(MAKE) -C ../libhxcadaptor/build/

libhxcfe: libhxcadaptor
	$(MAKE) -C ../libhxcfe/build/

libusbhxcfe: libhxcadaptor libhxcfe
	$(MAKE) -C ../libusbhxcfe/build/

HxCFloppyEmulator_cmdline: libhxcfe libusbhxcfe
	$(MAKE) -C ../HxCFloppyEmulator_cmdline/build/

fltk:
	chmod +x ../HxCFloppyEmulator_software/sources/thirdpartylibs/fltk/prepare_fltk.sh
	cd ../HxCFloppyEmulator_software/sources/thirdpartylibs/fltk/ && ./prepare_fltk.sh
ifeq ($(TARGET), mingw32)
		cd ../HxCFloppyEmulator_software/sources/thirdpartylibs/fltk/fltk-1.x.x/ && export LDFLAGS='-static-libstdc++ -static-libgcc' && ./configure --host=i686-w64-mingw32 --prefix=/usr/share/mingw-w64
		$(MAKE) -C ../HxCFloppyEmulator_software/sources/thirdpartylibs/fltk/fltk-1.x.x/ DIRS=src CC=i686-w64-mingw32-gcc CPP=i686-w64-mingw32-g++
else ifeq ($(TARGET), mingw64)
		cd ../HxCFloppyEmulator_software/sources/thirdpartylibs/fltk/fltk-1.x.x/ && export LDFLAGS='-static-libstdc++ -static-libgcc' && ./configure --host=x86_64-w64-mingw32 --prefix=/usr/share/mingw-w64
		$(MAKE) -C ../HxCFloppyEmulator_software/sources/thirdpartylibs/fltk/fltk-1.x.x/ DIRS=src CC=x86_64-w64-mingw32-gcc CPP=x86_64-w64-mingw32-g++
else ifeq ($(TARGET), Darwin)
		cd ../HxCFloppyEmulator_software/sources/thirdpartylibs/fltk && \
		cp -a fltk-1.x.x fltk-1.x.x-arm64 && \
		cd fltk-1.x.x-arm64 && \
		export CFLAGS='-mmacosx-version-min=10.13 -arch arm64' && export CXXFLAGS='-mmacosx-version-min=10.13 -arch arm64' && export LDFLAGS='-mmacosx-version-min=10.13 -arch arm64' && ./configure --host=arm-apple-darwin
		$(MAKE) -C ../HxCFloppyEmulator_software/sources/thirdpartylibs/fltk/fltk-1.x.x-arm64/ DIRS=src
		cd ../HxCFloppyEmulator_software/sources/thirdpartylibs/fltk && \
		cp -a fltk-1.x.x fltk-1.x.x-x86_64 && \
		cd fltk-1.x.x-x86_64 && \
		export CFLAGS='-mmacosx-version-min=10.13 -arch x86_64' && export CXXFLAGS='-mmacosx-version-min=10.13 -arch x86_64' && export LDFLAGS='-mmacosx-version-min=10.13 -arch x86_64' && ./configure --host=x86_64-apple-darwin
		$(MAKE) -C ../HxCFloppyEmulator_software/sources/thirdpartylibs/fltk/fltk-1.x.x-x86_64/ DIRS=src
		cd ../HxCFloppyEmulator_software/sources/thirdpartylibs/fltk && \
		lipo fltk-1.x.x-x86_64/lib/libfltk.a fltk-1.x.x-arm64/lib/libfltk.a -create -output fltk-1.x.x/lib/libfltk.a && \
		lipo fltk-1.x.x-x86_64/lib/libfltk_forms.a fltk-1.x.x-arm64/lib/libfltk_forms.a -create -output fltk-1.x.x/lib/libfltk_forms.a && \
		lipo fltk-1.x.x-x86_64/lib/libfltk_gl.a fltk-1.x.x-arm64/lib/libfltk_gl.a -create -output fltk-1.x.x/lib/libfltk_gl.a && \
		lipo fltk-1.x.x-x86_64/lib/libfltk_images.a fltk-1.x.x-arm64/lib/libfltk_images.a -create -output fltk-1.x.x/lib/libfltk_images.a
		cd ../HxCFloppyEmulator_software/sources/thirdpartylibs/fltk && \
		cp fltk-1.x.x-x86_64/FL/abi-version.h fltk-1.x.x/FL/abi-version.h
else
		$(MAKE) -C ../HxCFloppyEmulator_software/sources/thirdpartylibs/fltk/fltk-1.x.x/ DIRS=src
endif

HxCFloppyEmulator_software: libhxcfe libusbhxcfe fltk
	$(MAKE) -C ../HxCFloppyEmulator_software/build/

clean_fltk:
ifneq ("$(wildcard ../HxCFloppyEmulator_software/sources/thirdpartylibs/fltk/fltk-1.x.x/Makefile)","")
	$(MAKE) -C ../HxCFloppyEmulator_software/sources/thirdpartylibs/fltk/fltk-1.x.x/ clean
endif

clean: clean_fltk
	$(MAKE) -C ../libhxcadaptor/build/ clean
	$(MAKE) -C ../libhxcfe/build/ clean
	$(MAKE) -C ../libusbhxcfe/build/ clean
	$(MAKE) -C ../HxCFloppyEmulator_cmdline/build/ clean
	$(MAKE) -C ../HxCFloppyEmulator_software/build/ clean

mrproper:
	$(MAKE) -C ../libhxcadaptor/build/ mrproper
	$(MAKE) -C ../libhxcfe/build/ mrproper
	$(MAKE) -C ../libusbhxcfe/build/ mrproper
	$(MAKE) -C ../HxCFloppyEmulator_cmdline/build/ mrproper
	$(MAKE) -C ../HxCFloppyEmulator_software/build/ mrproper
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
	-rm -rf ../HxCFloppyEmulator_software/sources/thirdpartylibs/fltk/fltk-1.x.x
	-rm -rf ../HxCFloppyEmulator_software/sources/thirdpartylibs/fltk/fltk-1.x.x-arm64
	-rm -rf ../HxCFloppyEmulator_software/sources/thirdpartylibs/fltk/fltk-1.x.x-x86_64

.PHONY: libhxcadaptor libhxcfe libusbhxcfe HxCFloppyEmulator_cmdline HxCFloppyEmulator_software fltk mrproper clean
