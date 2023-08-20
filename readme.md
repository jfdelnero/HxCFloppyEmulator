# HxC Floppy Emulator toolkit

*The [HxC Floppy Emulators hardwares and firmwares](https://hxc2001.com/) companion.*

![CI Badge](https://github.com/jfdelnero/HxCFloppyEmulator/actions/workflows/ccpp.yml/badge.svg)

![HxCFloppyEmulator](/doc/imgs/banner.jpg?raw=true "HxCFloppyEmulator")

### Features

 - Import and convert many floppy image file formats.
 - Import, analyze and convert floppy stream files images.
 - Create/Browse DOS/FAT and AmigaDOS floppy images.
 - Create floppy disk images with custom sector layout.
 - Low level track and disk inspection.
 - Floppy disk read function.

 And many more !

### HxC Floppy Emulators main websites

https://hxc2001.com

https://hxc2001.com/floppy_drive_emulator/

### Forum

https://torlus.com/floppy/forum

### Repository content

##### Folder .\HxCFloppyEmulator_software\

HxC Floppy Emulator GUI software

(Windows, Linux, macOS)

##### Folder .\HxCFloppyEmulator_cmdline\

HxC Floppy Emulator command line software

(Windows, Linux, macOS)

##### Folder .\libhxcfe\

The main HxC Floppy Emulator library

(Windows, Linux, macOS)

##### Folder .\libusbhxcfe\

2006 USB HxC Floppy Emulator driver/library.

(Windows, Linux, macOS)

##### Folder .\libhxcadaptor\

System calls helper/wrapper library.

(Windows, Linux, macOS)

##### Folder .\build\

Build folder.

## How to build it ?

### Linux target

(Linux build environment)
```
cd build
make
```

### Windows target

(Linux or WSL2 + mingw32 build environment)

```
cd build
make TARGET=mingw32   # 32 bits build
make TARGET=mingw64   # 64 bits build
```

### macOS target

(macOS + Xcode build environment)

```
cd build
make
./maccreatebundle
```

-------------------------------------------------------------------------------

HxC Floppy Emulator project

Generic/universal floppy disk drive emulators

Copyright (C) 2006-2023  Jean-François DEL NERO / HxC2001

Email :    hxc2001(at)hxc2001.com

Website :  https://hxc2001.com
           https://hxc2001.com/floppy_drive_emulator/

Forum :    https://torlus.com/floppy/forum

Facebook : https://www.facebook.com/groups/hxc2001/

Twitter :  https://twitter.com/jfdelnero

-------------------------------------------------------------------------------

