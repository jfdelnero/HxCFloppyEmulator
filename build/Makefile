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

HxCFloppyEmulator_software: libhxcfe libusbhxcfe
	$(MAKE) -C ../HxCFloppyEmulator_software/build/

clean_fltk:
	$(MAKE) -C ../HxCFloppyEmulator_software/build/ clean_fltk

tests: all
	cd ../tests/ && ./run.sh

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
	-rm -rf ../tests/run
	-rm -rf HxCFloppyEmulator_Software
	-rm -rf *.zip
	-rm -rf *.wasm

.PHONY: libhxcadaptor libhxcfe libusbhxcfe HxCFloppyEmulator_cmdline HxCFloppyEmulator_software mrproper clean
