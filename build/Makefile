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
	$(MAKE) -C ../HxCFloppyEmulator_software/trunk/sources/thirdpartylibs/fltk/fltk-1.3.x/

HxCFloppyEmulator_software: libhxcfe libusbhxcfe fltk
	$(MAKE) -C ../HxCFloppyEmulator_software/trunk/build/

clean:
	$(MAKE) -C ../libhxcadaptor/trunk/build/ clean
	$(MAKE) -C ../libhxcfe/trunk/build/ clean
	$(MAKE) -C ../libusbhxcfe/trunk/build/ clean
	$(MAKE) -C ../HxCFloppyEmulator_cmdline/trunk/build/ clean
	$(MAKE) -C ../HxCFloppyEmulator_software/trunk/sources/thirdpartylibs/fltk/fltk-1.3.x/ clean
	$(MAKE) -C ../HxCFloppyEmulator_software/trunk/build/ clean

mrproper:
	$(MAKE) -C ../libhxcadaptor/trunk/build/ mrproper
	$(MAKE) -C ../libhxcfe/trunk/build/ mrproper
	$(MAKE) -C ../libusbhxcfe/trunk/build/ mrproper
	$(MAKE) -C ../HxCFloppyEmulator_cmdline/trunk/build/ mrproper
	$(MAKE) -C ../HxCFloppyEmulator_software/trunk/build/ mrproper
	rm -rf *.so
	rm -rf *.a
	rm hxcfloppyemulator
	rm hxcfe

.PHONY: libhxcadaptor libhxcfe libusbhxcfe HxCFloppyEmulator_cmdline HxCFloppyEmulator_software fltk mrproper clean
