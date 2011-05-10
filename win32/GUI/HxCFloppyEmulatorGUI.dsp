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
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "..\d30104\\" /I "..\..\common\includes" /I "..\..\win32" /I "..\..\common\ftdi_floppyemulator" /I "..\..\common\\" /I "..\..\common\dms_loader\xdms-1.3.2\src" /I "..\..\common\plugins\amigedosfs_loader\adflib\Lib\Win32\\" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "IPF_SUPPORT" /FR /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG"
# ADD RSC /l 0x40c /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib Comdlg32.lib advapi32.lib mod32.lib /nologo /subsystem:windows /machine:I386 /out:"Release/HxCFloppyEmulator.exe"

!ELSEIF  "$(CFG)" == "HxCFloppyEmulatorGUI - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "..\d30104\\" /I "..\..\common\includes" /I "..\..\win32" /I "..\..\common\ftdi_floppyemulator" /I "..\..\common\\" /I "..\..\common\dms_loader\xdms-1.3.2\src" /I "..\..\common\plugins\amigadosfs_loader\adflib\Lib\Win32\\" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /FR /FD /GZ /c
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
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib Comdlg32.lib advapi32.lib mod32.lib /nologo /stack:0x186a0,0x186a0 /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "HxCFloppyEmulatorGUI - Win32 Release"
# Name "HxCFloppyEmulatorGUI - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "Plugins"

# PROP Default_Filter ""
# Begin Group "ADF"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\plugins\adf_loader\adf_loader.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\adf_loader\adf_loader.h
# End Source File
# End Group
# Begin Group "DMS"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\plugins\dms_loader\dms_loader.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\dms_loader\dms_loader.h
# End Source File
# End Group
# Begin Group "amigadosfs_loader"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\plugins\amigadosfs_loader\amigadosfs_loader.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\amigadosfs_loader\amigadosfs_loader.h
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\amigadosfs_loader\stdboot3.h
# End Source File
# End Group
# Begin Group "IPF"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\plugins\ipf_loader\ipf_loader.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\ipf_loader\ipf_loader.h
# End Source File
# End Group
# Begin Group "ST"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\plugins\st_loader\st_loader.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\st_loader\st_loader.h
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\st_loader\stfileformat.h
# End Source File
# End Group
# Begin Group "STX"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\plugins\stx_loader\pasti_format.h
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\stx_loader\stx_loader.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\stx_loader\stx_loader.h
# End Source File
# End Group
# Begin Group "CPCDSK"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\plugins\cpcdsk_loader\cpcdsk_format.h
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\cpcdsk_loader\cpcdsk_loader.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\cpcdsk_loader\cpcdsk_loader.h
# End Source File
# End Group
# Begin Group "IMGPC"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\plugins\img_loader\img_loader.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\img_loader\img_loader.h
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\img_loader\pcimgfileformat.h
# End Source File
# End Group
# Begin Group "CopyQM"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\plugins\copyqm_loader\copyqm_loader.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\copyqm_loader\copyqm_loader.h
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\copyqm_loader\crctable.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\copyqm_loader\crctable.h
# End Source File
# End Group
# Begin Group "MSA"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\plugins\msa_loader\msa_loader.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\msa_loader\msa_loader.h
# End Source File
# End Group
# Begin Group "OricDSK"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\plugins\oricdsk_loader\oricdsk_format.h
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\oricdsk_loader\oricdsk_loader.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\oricdsk_loader\oricdsk_loader.h
# End Source File
# End Group
# Begin Group "MFM"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\plugins\mfm_loader\mfm_loader.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\mfm_loader\mfm_loader.h
# End Source File
# End Group
# Begin Group "FATDK"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\plugins\fat12floppy_loader\fat12.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\fat12floppy_loader\fat12.h
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\fat12floppy_loader\fat12floppy_loader.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\fat12floppy_loader\fat12floppy_loader.h
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\fat12floppy_loader\fat12formats.h
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\fat12floppy_loader\fatlib.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\fat12floppy_loader\fatlib.h
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\fat12floppy_loader\pcbootsector.h
# End Source File
# End Group
# Begin Group "SMC"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\plugins\smc_loader\snes_smc_loader.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\smc_loader\snes_smc_loader.h
# End Source File
# End Group
# Begin Group "ADZ"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\plugins\adz_loader\adz_loader.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\adz_loader\adz_loader.h
# End Source File
# End Group
# Begin Group "MSX"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\plugins\msx_loader\msx_loader.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\msx_loader\msx_loader.h
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\msx_loader\msxfileformat.h
# End Source File
# End Group
# Begin Group "HFE"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\plugins\hfe_loader\hfe_loader.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\hfe_loader\hfe_loader.h
# End Source File
# End Group
# Begin Group "IMD"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\plugins\imd_loader\imd_format.h
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\imd_loader\imd_loader.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\imd_loader\imd_loader.h
# End Source File
# End Group
# Begin Group "AFI"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\plugins\afi_loader\afi_loader.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\afi_loader\afi_loader.h
# End Source File
# End Group
# Begin Group "D64"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\plugins\d64_loader\d64_loader.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\d64_loader\d64_loader.h
# End Source File
# End Group
# Begin Group "CPCFS"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\plugins\cpcfs_loader\cpcfs_loader.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\cpcfs_loader\cpcfs_loader.h
# End Source File
# End Group
# Begin Group "TRD"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\plugins\trd_loader\trd_loader.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\trd_loader\trd_loader.h
# End Source File
# End Group
# Begin Group "SCL"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\plugins\scl_loader\scl_loader.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\scl_loader\scl_loader.h
# End Source File
# End Group
# Begin Group "D88"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\plugins\d88_loader\d88_format.h
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\d88_loader\d88_loader.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\d88_loader\d88_loader.h
# End Source File
# End Group
# Begin Group "JV1"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\plugins\jv1_loader\jv1_loader.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\jv1_loader\jv1_loader.h
# End Source File
# End Group
# Begin Group "RAW"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\plugins\raw_loader\raw_loader.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\raw_loader\raw_loader.h
# End Source File
# End Group
# Begin Group "SAP"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\plugins\sap_loader\sap_loader.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\sap_loader\sap_loader.h
# End Source File
# End Group
# Begin Group "VTR"

# PROP Default_Filter "VTR"
# Begin Source File

SOURCE=..\..\common\plugins\vtr_loader\vtr_loader.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\vtr_loader\vtr_loader.h
# End Source File
# End Group
# Begin Group "HDM"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\plugins\hdm_loader\hdm_loader.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\hdm_loader\hdm_loader.h
# End Source File
# End Group
# Begin Group "TI99PC99"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\plugins\ti99pc99_loader\ti99pc99_loader.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\ti99pc99_loader\ti99pc99_loader.h
# End Source File
# End Group
# Begin Group "ApriDisk"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\plugins\apridisk_loader\apridisk_format.h
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\apridisk_loader\apridisk_loader.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\apridisk_loader\apridisk_loader.h
# End Source File
# End Group
# Begin Group "EDE"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\plugins\ede_loader\ede_loader.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\ede_loader\ede_loader.h
# End Source File
# End Group
# Begin Group "FD"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\plugins\fd_loader\fd_loader.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\fd_loader\fd_loader.h
# End Source File
# End Group
# Begin Group "VDK"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\plugins\vdk_loader\vdk_format.h
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\vdk_loader\vdk_loader.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\vdk_loader\vdk_loader.h
# End Source File
# End Group
# Begin Group "DPX"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\plugins\dpx_loader\dpx_loader.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\dpx_loader\dpx_loader.h
# End Source File
# End Group
# Begin Group "Ensoniq_Mirage"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\plugins\ensoniq_mirage_loader\ensoniq_mirage_loader.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\ensoniq_mirage_loader\ensoniq_mirage_loader.h
# End Source File
# End Group
# Begin Group "Emax"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\plugins\emax_loader\emax_loader.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\emax_loader\emax_loader.h
# End Source File
# End Group
# Begin Group "MGT"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\plugins\mgt_loader\mgt_loader.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\mgt_loader\mgt_loader.h
# End Source File
# End Group
# Begin Group "SAD"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\plugins\sad_loader\sad_fileformat.h
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\sad_loader\sad_loader.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\sad_loader\sad_loader.h
# End Source File
# End Group
# Begin Group "JV3"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\plugins\jv3_loader\jv3_loader.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\jv3_loader\jv3_loader.h
# End Source File
# End Group
# Begin Group "STT"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\plugins\stt_loader\stt_loader.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\stt_loader\stt_loader.h
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\stt_loader\sttfileformat.h
# End Source File
# End Group
# Begin Group "Prophet"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\plugins\prophet_loader\prophet_loader.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\prophet_loader\prophet_loader.h
# End Source File
# End Group
# Begin Group "TeleDisk"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\plugins\teledisk_loader\td0_lzss.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\teledisk_loader\td0_lzss.h
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\teledisk_loader\teledisk_format.h
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\teledisk_loader\teledisk_loader.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\teledisk_loader\teledisk_loader.h
# End Source File
# End Group
# Begin Group "EmuII_RAW"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\plugins\emuii_raw_loader\emuii_raw_loader.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\emuii_raw_loader\emuii_raw_loader.h
# End Source File
# End Group
# Begin Group "EmuII"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\plugins\emuii_loader\emuii_loader.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\emuii_loader\emuii_loader.h
# End Source File
# End Group
# Begin Group "EmuI_RAW"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\plugins\emui_raw_loader\emui_raw_loader.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\emui_raw_loader\emui_raw_loader.h
# End Source File
# End Group
# Begin Group "JVC"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\plugins\jvc_loader\jvc_loader.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\jvc_loader\jvc_loader.h
# End Source File
# End Group
# Begin Group "DIM"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\plugins\dim_loader\dim_loader.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\dim_loader\dim_loader.h
# End Source File
# End Group
# Begin Group "TI99V9T9"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\plugins\ti99v9t9_loader\ti99v9t9_loader.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\ti99v9t9_loader\ti99v9t9_loader.h
# End Source File
# End Group
# Begin Group "DMK"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\plugins\dmk_loader\dmk_loader.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\dmk_loader\dmk_loader.h
# End Source File
# End Group
# Begin Group "D81"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\plugins\d81_loader\d81_loader.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\d81_loader\d81_loader.h
# End Source File
# End Group
# Begin Group "ACORNADF"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\plugins\acornadf_loader\acornadf_loader.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\acornadf_loader\acornadf_loader.h
# End Source File
# End Group
# Begin Group "VEGASDSK"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\plugins\vegasdsk_loader\vegasdsk_loader.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\vegasdsk_loader\vegasdsk_loader.h
# End Source File
# End Group
# Begin Group "CamputersLynxLDF"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\plugins\camputerslynxldf_loader\camputerslynxldf_loader.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\camputerslynxldf_loader\camputerslynxldf_loader.h
# End Source File
# End Group
# Begin Group "EXTADF"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\plugins\extadf_loader\extadf_loader.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\extadf_loader\extadf_loader.h
# End Source File
# End Group
# Begin Group "OLDEXTADF"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\plugins\oldextadf_loader\oldextadf_loader.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\oldextadf_loader\oldextadf_loader.h
# End Source File
# End Group
# Begin Group "FDI"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\plugins\fdi_loader\fdi_format.h
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\fdi_loader\fdi_loader.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\fdi_loader\fdi_loader.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\common\plugins\common\crc.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\common\crc.h
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\common\emuii_track.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\common\emuii_track.h
# End Source File
# Begin Source File

SOURCE=..\..\common\floppy_utils.c
# End Source File
# Begin Source File

SOURCE=..\..\common\includes\floppy_utils.h
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\common\gcr_track.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\common\gcr_track.h
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\common\iso_ibm_track.c
# End Source File
# Begin Source File

SOURCE=..\..\common\plugins\common\ISO_IBM_track.h
# End Source File
# End Group
# Begin Group "GUI"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dialog_about.c
# End Source File
# Begin Source File

SOURCE=.\Dialog_about.h
# End Source File
# Begin Source File

SOURCE=.\Dialog_BatchConvert.c
# End Source File
# Begin Source File

SOURCE=.\Dialog_BatchConvert.h
# End Source File
# Begin Source File

SOURCE=.\Dialog_config.c
# End Source File
# Begin Source File

SOURCE=.\Dialog_config.h
# End Source File
# Begin Source File

SOURCE=.\Dialog_CreateFileSystem.c
# End Source File
# Begin Source File

SOURCE=.\Dialog_CreateFileSystem.h
# End Source File
# Begin Source File

SOURCE=.\Dialog_floppydump.c
# End Source File
# Begin Source File

SOURCE=.\Dialog_floppydump.h
# End Source File
# Begin Source File

SOURCE=.\Dialog_license.c
# End Source File
# Begin Source File

SOURCE=.\Dialog_license.h
# End Source File
# Begin Source File

SOURCE=.\Dialog_logs.c
# End Source File
# Begin Source File

SOURCE=.\Dialog_logs.h
# End Source File
# Begin Source File

SOURCE=.\Dialog_MainDialog.c
# End Source File
# Begin Source File

SOURCE=.\Dialog_MainDialog.h
# End Source File
# Begin Source File

SOURCE=.\Dialog_RAWFileSettings.c
# End Source File
# Begin Source File

SOURCE=.\Dialog_RAWFileSettings.h
# End Source File
# Begin Source File

SOURCE=.\Dialog_stats.c
# End Source File
# Begin Source File

SOURCE=.\Dialog_stats.h
# End Source File
# Begin Source File

SOURCE=.\fileselector.c
# End Source File
# Begin Source File

SOURCE=.\fileselector.h
# End Source File
# Begin Source File

SOURCE=.\hxc2001bmp.h
# End Source File
# Begin Source File

SOURCE=.\HxCFloppyEmulatorGUI.c
# End Source File
# Begin Source File

SOURCE=.\HxCFloppyEmulatorGUI.h
# End Source File
# Begin Source File

SOURCE=.\loader.c
# End Source File
# Begin Source File

SOURCE=.\loader.h
# End Source File
# Begin Source File

SOURCE=.\microintro.c
# End Source File
# Begin Source File

SOURCE=.\microintro.h
# End Source File
# Begin Source File

SOURCE=.\plateforms.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\Resource_HxCFloppyEmu.RC
# End Source File
# Begin Source File

SOURCE=.\rle.c
# End Source File
# Begin Source File

SOURCE=.\rle.h
# End Source File
# Begin Source File

SOURCE=.\soft_cfg_file.c
# End Source File
# Begin Source File

SOURCE=.\soft_cfg_file.h
# End Source File
# End Group
# Begin Group "FTDI_FloppyEmulator"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\usb_floppyemulator\cpld_opcodes.h
# End Source File
# Begin Source File

SOURCE=..\..\win32\ftdi.c
# End Source File
# Begin Source File

SOURCE=..\..\win32\ftdi.h
# End Source File
# Begin Source File

SOURCE=..\..\common\usb_floppyemulator\usb_hxcfloppyemulator.c
# End Source File
# Begin Source File

SOURCE=..\..\common\usb_floppyemulator\usb_hxcfloppyemulator.h
# End Source File
# Begin Source File

SOURCE=..\..\common\usb_floppyemulator\variablebitrate.c
# End Source File
# Begin Source File

SOURCE=..\..\common\usb_floppyemulator\variablebitrate.h
# End Source File
# End Group
# Begin Group "libs"

# PROP Default_Filter ""
# Begin Group "ZLIB"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\libs\zlib\zlib123\adler32.c
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\zlib\zlib123\compress.c
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\zlib\zlib123\crc32.c
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\zlib\zlib123\crc32.h
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\zlib\zlib123\deflate.c
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\zlib\zlib123\deflate.h
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\zlib\zlib123\gzio.c
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\zlib\zlib123\infback.c
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\zlib\zlib123\inffast.c
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\zlib\zlib123\inffast.h
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\zlib\zlib123\inffixed.h
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\zlib\zlib123\inflate.c
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\zlib\zlib123\inflate.h
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\zlib\zlib123\inftrees.c
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\zlib\zlib123\inftrees.h
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\zlib\zlib123\trees.c
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\zlib\zlib123\trees.h
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\zlib\zlib123\uncompr.c
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\zlib\zlib123\zconf.h
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\zlib\zlib123\zconf.in.h
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\zlib\zlib123\zlib.h
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\zlib\zlib123\zutil.c
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\zlib\zlib123\zutil.h
# End Source File
# End Group
# Begin Group "ADFLIB"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\libs\adflib\Lib\adf_bitm.c
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\adflib\Lib\adf_bitm.h
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\adflib\Lib\adf_blk.h
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\adflib\Lib\adf_cache.c
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\adflib\Lib\adf_cache.h
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\adflib\Lib\adf_defs.h
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\adflib\Lib\adf_dir.c
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\adflib\Lib\adf_dir.h
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\adflib\Lib\adf_disk.c
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\adflib\Lib\adf_disk.h
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\adflib\Lib\adf_dump.c
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\adflib\Lib\adf_dump.h
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\adflib\Lib\adf_env.c
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\adflib\Lib\adf_env.h
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\adflib\Lib\adf_err.h
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\adflib\Lib\adf_file.c
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\adflib\Lib\adf_file.h
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\adflib\Lib\adf_hd.c
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\adflib\Lib\adf_hd.h
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\adflib\Lib\adf_link.c
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\adflib\Lib\adf_link.h
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\adflib\Lib\adf_nativ.c
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\adflib\Lib\adf_nativ.h
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\adflib\Lib\adf_raw.c
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\adflib\Lib\adf_raw.h
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\adflib\Lib\adf_salv.c
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\adflib\Lib\adf_salv.h
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\adflib\Lib\adf_str.h
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\adflib\Lib\adf_util.c
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\adflib\Lib\adf_util.h
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\adflib\Lib\adflib.h
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\adflib\Lib\defendian.h
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\adflib\Lib\hd_blk.h
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\adflib\Lib\Win32\nt4_dev.c
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\adflib\Lib\Win32\nt4_dev.h
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\adflib\Lib\prefix.h
# End Source File
# End Group
# Begin Group "XDMS"

# PROP Default_Filter ""
# Begin Source File

SOURCE="..\..\common\libs\xdms\xdms-1.3.2\src\cdata.h"
# End Source File
# Begin Source File

SOURCE="..\..\common\libs\xdms\xdms-1.3.2\src\crc_csum.c"
# End Source File
# Begin Source File

SOURCE="..\..\common\libs\xdms\xdms-1.3.2\src\crc_csum.h"
# End Source File
# Begin Source File

SOURCE="..\..\common\libs\xdms\xdms-1.3.2\src\getbits.c"
# End Source File
# Begin Source File

SOURCE="..\..\common\libs\xdms\xdms-1.3.2\src\getbits.h"
# End Source File
# Begin Source File

SOURCE="..\..\common\libs\xdms\xdms-1.3.2\src\maketbl.c"
# End Source File
# Begin Source File

SOURCE="..\..\common\libs\xdms\xdms-1.3.2\src\maketbl.h"
# End Source File
# Begin Source File

SOURCE="..\..\common\libs\xdms\xdms-1.3.2\src\pfile.c"
# End Source File
# Begin Source File

SOURCE="..\..\common\libs\xdms\xdms-1.3.2\src\pfile.h"
# End Source File
# Begin Source File

SOURCE="..\..\common\libs\xdms\xdms-1.3.2\src\tables.c"
# End Source File
# Begin Source File

SOURCE="..\..\common\libs\xdms\xdms-1.3.2\src\tables.h"
# End Source File
# Begin Source File

SOURCE="..\..\common\libs\xdms\xdms-1.3.2\src\u_deep.c"
# End Source File
# Begin Source File

SOURCE="..\..\common\libs\xdms\xdms-1.3.2\src\u_deep.h"
# End Source File
# Begin Source File

SOURCE="..\..\common\libs\xdms\xdms-1.3.2\src\u_heavy.c"
# End Source File
# Begin Source File

SOURCE="..\..\common\libs\xdms\xdms-1.3.2\src\u_heavy.h"
# End Source File
# Begin Source File

SOURCE="..\..\common\libs\xdms\xdms-1.3.2\src\u_init.c"
# End Source File
# Begin Source File

SOURCE="..\..\common\libs\xdms\xdms-1.3.2\src\u_init.h"
# End Source File
# Begin Source File

SOURCE="..\..\common\libs\xdms\xdms-1.3.2\src\u_medium.c"
# End Source File
# Begin Source File

SOURCE="..\..\common\libs\xdms\xdms-1.3.2\src\u_medium.h"
# End Source File
# Begin Source File

SOURCE="..\..\common\libs\xdms\xdms-1.3.2\src\u_quick.c"
# End Source File
# Begin Source File

SOURCE="..\..\common\libs\xdms\xdms-1.3.2\src\u_quick.h"
# End Source File
# Begin Source File

SOURCE="..\..\common\libs\xdms\xdms-1.3.2\src\u_rle.c"
# End Source File
# Begin Source File

SOURCE="..\..\common\libs\xdms\xdms-1.3.2\src\u_rle.h"
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\xdms\vfile.c
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\xdms\vfile.h
# End Source File
# End Group
# Begin Group "CAPS"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\ipf\capslibloader.c
# End Source File
# Begin Source File

SOURCE=..\ipf\capslibloader.h
# End Source File
# End Group
# Begin Group "CPCFSLIB"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\libs\cpcfs\src\cpcfs.c
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\cpcfs\src\cpcfs.h
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\cpcfs\src\dos.c
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\cpcfs\src\dos.h
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\cpcfs\src\fs.c
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\cpcfs\src\keywords.h
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\cpcfs\src\makedoc.c
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\cpcfs\src\match.c
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\cpcfs\src\match.h
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\cpcfs\src\readdef.c
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\cpcfs\src\Tools.c
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\cpcfs\src\ui.c
# End Source File
# End Group
# Begin Group "LIBSAP"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\libs\libsap\libsap.c
# End Source File
# Begin Source File

SOURCE=..\..\common\libs\libsap\libsap.h
# End Source File
# End Group
# End Group
# Begin Source File

SOURCE=..\..\common\afi_file_writer.c
# End Source File
# Begin Source File

SOURCE=..\..\common\cpcdsk_file_writer.c
# End Source File
# Begin Source File

SOURCE=..\..\common\extended_hfe_file_writer.c
# End Source File
# Begin Source File

SOURCE=..\..\common\extended_hfe_file_writer.h
# End Source File
# Begin Source File

SOURCE=..\..\common\floppy_loader.c
# End Source File
# Begin Source File

SOURCE=..\..\common\hfe_file_writer.c
# End Source File
# Begin Source File

SOURCE=..\..\common\imd_file_writer.c
# End Source File
# Begin Source File

SOURCE=..\..\common\imd_file_writer.h
# End Source File
# Begin Source File

SOURCE=..\..\common\mfm_file_writer.c
# End Source File
# Begin Source File

SOURCE=..\..\common\raw_file_writer.c
# End Source File
# Begin Source File

SOURCE=..\..\common\sector_extractor.c
# End Source File
# Begin Source File

SOURCE=..\..\common\vtrucco_file_writer.c
# End Source File
# Begin Source File

SOURCE=..\..\win32\WIN32_API.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\common\afi_file_writer.h
# End Source File
# Begin Source File

SOURCE=..\..\common\hfe_file_writer.h
# End Source File
# Begin Source File

SOURCE=..\..\common\includes\hxc_floppy_emulator.h
# End Source File
# Begin Source File

SOURCE=..\..\common\includes\internal_floppy.h
# End Source File
# Begin Source File

SOURCE=..\..\common\loaders_list.h
# End Source File
# Begin Source File

SOURCE=..\..\common\mfm_file_writer.h
# End Source File
# Begin Source File

SOURCE=..\..\common\raw_file_writer.h
# End Source File
# Begin Source File

SOURCE=..\..\common\sector_extractor.h
# End Source File
# Begin Source File

SOURCE=..\..\common\types.h
# End Source File
# Begin Source File

SOURCE=..\..\common\version.h
# End Source File
# Begin Source File

SOURCE=..\..\common\vtrucco_file_writer.h
# End Source File
# Begin Source File

SOURCE=..\..\win32\WIN32_API.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\floppy.bmp
# End Source File
# Begin Source File

SOURCE=.\mainicon.ico
# End Source File
# End Group
# End Target
# End Project
