# Microsoft Developer Studio Project File - Name="HxCFloppyEmulatorGUI" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=HxCFloppyEmulatorGUI - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "HxCFloppyEmulatorGUI.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "HxCFloppyEmulatorGUI.mak" CFG="HxCFloppyEmulatorGUI - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "HxCFloppyEmulatorGUI - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "HxCFloppyEmulatorGUI - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "HxCFloppyEmulatorGUI - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "..\..\..\build\"
# PROP BASE Intermediate_Dir "..\..\..\build\Release_gui"
# PROP BASE Target_Dir "..\..\..\build\"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\..\build\"
# PROP Intermediate_Dir "..\..\..\build\Release_gui"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir "..\..\..\build\"
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MD /GX /Os /Ob2 /I "..\sources" /I "..\sources\usb_floppyemulator\win32" /I "..\sources\gui" /I "..\sources\thirdpartylibs\fltk\fltk-1.3.x\\" /I "..\..\..\libhxcfe\trunk\sources" /I "..\..\..\libusbhxcfe\trunk\sources" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /FR /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG"
# ADD RSC /l 0x40c /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 winmm.lib fltk.lib comctl32.lib wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib libhxcfe.lib libusbhxcfe.lib /nologo /subsystem:windows /machine:I386 /nodefaultlib:"libcd" /libpath:"..\sources\thirdpartylibs\fltk\fltk-1.3.x\lib" /libpath:"..\..\..\build"
# SUBTRACT LINK32 /nodefaultlib

!ELSEIF  "$(CFG)" == "HxCFloppyEmulatorGUI - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "..\..\..\build\"
# PROP BASE Intermediate_Dir "..\..\..\build\Debug_gui"
# PROP BASE Target_Dir "..\..\..\build\"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\..\build\"
# PROP Intermediate_Dir "..\..\..\build\Debug_gui"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir "..\..\..\build\"
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\sources" /I "..\sources\usb_floppyemulator\win32" /I "..\sources\gui" /I "..\sources\thirdpartylibs\fltk\fltk-1.3.x\\" /I "..\..\..\libhxcfe\trunk\sources" /I "..\..\..\libusbhxcfe\trunk\sources" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /FR /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG"
# ADD RSC /l 0x40c /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 winmm.lib fltkd.lib comctl32.lib wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib libhxcfe.lib libusbhxcfe.lib /nologo /stack:0x186a0,0x186a0 /subsystem:windows /debug /machine:I386 /nodefaultlib:"libcd" /pdbtype:sept /libpath:"..\sources\thirdpartylibs\fltk\fltk-1.3.x\lib" /libpath:"..\..\..\build"

!ENDIF 

# Begin Target

# Name "HxCFloppyEmulatorGUI - Win32 Release"
# Name "HxCFloppyEmulatorGUI - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "GUI"

# PROP Default_Filter ""
# Begin Group "windows"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\gui\batch_converter_window.cxx
# End Source File
# Begin Source File

SOURCE=..\sources\gui\batch_converter_window.h
# End Source File
# Begin Source File

SOURCE=..\sources\gui\filesystem_generator_window.cxx
# End Source File
# Begin Source File

SOURCE=..\sources\gui\filesystem_generator_window.h
# End Source File
# Begin Source File

SOURCE=..\sources\gui\floppy_dump_window.cxx
# End Source File
# Begin Source File

SOURCE=..\sources\gui\floppy_dump_window.h
# End Source File
# Begin Source File

SOURCE=..\sources\gui\floppy_infos_window.cxx
# End Source File
# Begin Source File

SOURCE=..\sources\gui\floppy_infos_window.h
# End Source File
# Begin Source File

SOURCE=..\sources\gui\rawfile_loader_window.cxx
# End Source File
# Begin Source File

SOURCE=..\sources\gui\rawfile_loader_window.h
# End Source File
# Begin Source File

SOURCE=..\sources\gui\sdhxcfecfg_window.cxx
# End Source File
# Begin Source File

SOURCE=..\sources\gui\sdhxcfecfg_window.h
# End Source File
# Begin Source File

SOURCE=..\sources\gui\usbhxcfecfg_window.cxx
# End Source File
# Begin Source File

SOURCE=..\sources\gui\usbhxcfecfg_window.h
# End Source File
# End Group
# Begin Group "microintro"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\gui\microintro\packer\lzw.c
# End Source File
# Begin Source File

SOURCE=..\sources\gui\microintro\packer\lzw.h
# End Source File
# Begin Source File

SOURCE=..\sources\gui\microintro\microintro.c
# End Source File
# Begin Source File

SOURCE=..\sources\gui\microintro\microintro.h
# End Source File
# Begin Source File

SOURCE=..\sources\gui\microintro\mod32.h
# End Source File
# Begin Source File

SOURCE=..\sources\gui\microintro\packer\pack.c
# End Source File
# Begin Source File

SOURCE=..\sources\gui\microintro\packer\pack.h
# End Source File
# Begin Source File

SOURCE=..\sources\gui\microintro\packer\rle.c
# End Source File
# Begin Source File

SOURCE=..\sources\gui\microintro\packer\rle.h
# End Source File
# Begin Source File

SOURCE=..\sources\gui\microintro\mod32.lib
# End Source File
# End Group
# Begin Source File

SOURCE=..\sources\gui\about_gui.cxx
# End Source File
# Begin Source File

SOURCE=..\sources\gui\about_gui.h
# End Source File
# Begin Source File

SOURCE=..\sources\gui\basename.cxx
# End Source File
# Begin Source File

SOURCE=..\sources\gui\cb_batch_converter_window.cxx
# End Source File
# Begin Source File

SOURCE=..\sources\gui\cb_batch_converter_window.h
# End Source File
# Begin Source File

SOURCE=..\sources\gui\cb_floppy_dump_window.cxx
# End Source File
# Begin Source File

SOURCE=..\sources\gui\cb_floppy_dump_window.h
# End Source File
# Begin Source File

SOURCE=..\sources\gui\cb_floppy_infos_window.cxx
# End Source File
# Begin Source File

SOURCE=..\sources\gui\cb_floppy_infos_window.h
# End Source File
# Begin Source File

SOURCE=..\sources\gui\cb_rawfile_loader_window.cxx
# End Source File
# Begin Source File

SOURCE=..\sources\gui\cb_rawfile_loader_window.h
# End Source File
# Begin Source File

SOURCE=..\sources\gui\cb_sdhxcfecfg_window.cxx
# End Source File
# Begin Source File

SOURCE=..\sources\gui\cb_sdhxcfecfg_window.h
# End Source File
# Begin Source File

SOURCE=..\sources\gui\cb_usbhxcfecfg_window.cxx
# End Source File
# Begin Source File

SOURCE=..\sources\gui\cb_usbhxcfecfg_window.h
# End Source File
# Begin Source File

SOURCE=..\sources\gui\fileselector.cxx
# End Source File
# Begin Source File

SOURCE=..\sources\gui\fileselector.h
# End Source File
# Begin Source File

SOURCE=..\sources\gui\fl_dnd_box.cxx
# End Source File
# Begin Source File

SOURCE=..\sources\gui\fl_dnd_box.h
# End Source File
# Begin Source File

SOURCE=..\sources\gui\fl_mouse_box.h
# End Source File
# Begin Source File

SOURCE=..\sources\gui\gui_struct.h
# End Source File
# Begin Source File

SOURCE=..\sources\gui\license_gui.cxx
# End Source File
# Begin Source File

SOURCE=..\sources\gui\license_gui.h
# End Source File
# Begin Source File

SOURCE=..\sources\loader.cxx
# End Source File
# Begin Source File

SOURCE=..\sources\loader.h
# End Source File
# Begin Source File

SOURCE=..\sources\gui\log.cxx
# End Source File
# Begin Source File

SOURCE=..\sources\gui\log_gui.h
# End Source File
# Begin Source File

SOURCE=..\sources\main.cxx
# End Source File
# Begin Source File

SOURCE=..\sources\main.h
# End Source File
# Begin Source File

SOURCE=..\sources\gui\main_gui.cxx
# End Source File
# Begin Source File

SOURCE=..\sources\gui\main_gui.h
# End Source File
# Begin Source File

SOURCE=..\sources\gui\msg_txt.h
# End Source File
# Begin Source File

SOURCE=..\sources\soft_cfg_file.cxx
# End Source File
# Begin Source File

SOURCE=..\sources\win32\win32_api.c
# End Source File
# Begin Source File

SOURCE=..\sources\win32_api.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\sources\win32\hxcfloppy.rc
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\sources\version.h
# End Source File
# Begin Source File

SOURCE=..\sources\win32\win32_api.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\sources\win32\icon2.ico
# End Source File
# Begin Source File

SOURCE=..\sources\win32\mainicon.ico
# End Source File
# End Group
# End Target
# End Project
