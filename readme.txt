-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
-----------H----H--X----X-----CCCCC----22222----0000-----0000------11----------
----------H----H----X-X-----C--------------2---0----0---0----0--1--1-----------
---------HHHHHH-----X------C----------22222---0----0---0----0-----1------------
--------H----H----X--X----C----------2-------0----0---0----0-----1-------------
-------H----H---X-----X---CCCCC-----222222----0000-----0000----1111------------
-------------------------------------------------------------------------------
-------------------------------------------------------- https://hxc2001.com --
-------------------------------------------------------------------------------
HxC Floppy Emulator project

Generic/universal floppy disk drive emulators

Copyright (C) 2006-2024  Jean-François DEL NERO / HxC2001

Email :    hxc2001(at)hxc2001.com

Website :  https://hxc2001.com
           https://hxc2001.com/floppy_drive_emulator/

Forum :    https://torlus.com/floppy/forum

Facebook : https://www.facebook.com/groups/hxc2001/

Twitter :  https://twitter.com/jfdelnero
-------------------------------------------------------------------------------

This software is the HxC Floppy Emulators hardwares and firmwares companion.

Features :

 - Import and convert many floppy image file formats.
 - Import, analyze and convert floppy stream files images.
 - Create/Browse DOS and AmigaDOS floppy images.
 - Create floppy disk images with custom sector layout.
 - Low level track and disk inspection.
 - Floppy disk read function.

 And many more !

-------------------------------------------------------------------------------
.\HxCFloppyEmulator_software\

HxC Floppy Emulator GUI software
(Windows, Linux, macOS)
-------------------------------------------------------------------------------
.\HxCFloppyEmulator_cmdline\

HxC Floppy Emulator command line software
(Windows, Linux, macOS)
-------------------------------------------------------------------------------
.\libhxcfe\

The main HxC Floppy Emulator library
(Windows, Linux, macOS)
-------------------------------------------------------------------------------
.\libusbhxcfe\

2006 USB HxC Floppy Emulator driver/library.
(Windows, Linux, macOS)
-------------------------------------------------------------------------------
.\libhxcadaptor\

System calls helper/wrapper library.
(Windows, Linux, macOS)
-------------------------------------------------------------------------------
.\build\

Build folder.
-------------------------------------------------------------------------------

How to build it ?
-----------------

Linux target :
-----------------

(Linux build environment)

cd build
make

Windows target :
-----------------

(Linux or WSL2 + mingw32 build environment)

cd build
make TARGET=mingw32   # 32 bits build

make TARGET=mingw64   # 64 bits build

macOS target :
-----------------

(macOS + Xcode build environment)

cd build
make
./maccreatebundle
