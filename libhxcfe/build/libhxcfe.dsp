# Microsoft Developer Studio Project File - Name="libhxcfe" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=libhxcfe - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libhxcfe.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libhxcfe.mak" CFG="libhxcfe - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libhxcfe - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "libhxcfe - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libhxcfe - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "..\..\..\build\"
# PROP BASE Intermediate_Dir "..\..\..\build\Release_libhxcfe"
# PROP BASE Target_Dir "..\..\..\build\"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\..\build\"
# PROP Intermediate_Dir "..\..\..\build\Release_libhxcfe"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir "..\..\..\build\"
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBHXCFE_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\..\..\libhxcadaptor\trunk\sources" /I "..\sources\\" /I "..\sources\win32" /I "..\sources\thirdpartylibs\expat\expat-2.1.0\lib" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBHXCFE_EXPORTS" /D "IPF_SUPPORT" /D "XML_STATIC" /FAs /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG"
# ADD RSC /l 0x40c /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib libhxcadaptor.lib /nologo /dll /machine:I386 /libpath:"..\..\..\build\\"

!ELSEIF  "$(CFG)" == "libhxcfe - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "..\..\..\build\"
# PROP BASE Intermediate_Dir "..\..\..\build\Debug_libhxcfe"
# PROP BASE Target_Dir "..\..\..\build\"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\..\build\"
# PROP Intermediate_Dir "..\..\..\build\Debug_libhxcfe"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir "..\..\..\build\"
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBHXCFE_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\..\..\libhxcadaptor\trunk\sources" /I "sources" /I "..\sources" /I "..\sources\win32" /I "..\sources\thirdpartylibs\expat\expat-2.1.0\lib" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBHXCFE_EXPORTS" /D "IPF_SUPPORT" /D "XML_STATIC" /FR /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG"
# ADD RSC /l 0x40c /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib libhxcadaptor.lib /nologo /dll /debug /machine:I386 /pdbtype:sept /libpath:"..\..\..\build\\"

!ENDIF 

# Begin Target

# Name "libhxcfe - Win32 Release"
# Name "libhxcfe - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "libs"

# PROP Default_Filter ""
# Begin Group "ZLIB"

# PROP Default_Filter ""
# Begin Group "MINIZIP"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\thirdpartylibs\zlib\contrib\minizip\crypt.h
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\zlib\contrib\minizip\ioapi.c
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\zlib\contrib\minizip\ioapi.h
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\zlib\contrib\minizip\iowin32.c
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\zlib\contrib\minizip\iowin32.h
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\zlib\contrib\minizip\mztools.c
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\zlib\contrib\minizip\mztools.h
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\zlib\contrib\minizip\unzip.c
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\zlib\contrib\minizip\unzip.h
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\zlib\contrib\minizip\zip.c
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\zlib\contrib\minizip\zip.h
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\zlib\contrib\minizip\zlib.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\sources\thirdpartylibs\zlib\adler32.c
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\zlib\compress.c
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\zlib\crc32.c
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\zlib\crc32.h
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\zlib\deflate.c
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\zlib\deflate.h
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\zlib\gzclose.c
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\zlib\gzguts.h
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\zlib\gzlib.c
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\zlib\gzread.c
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\zlib\gzwrite.c
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\zlib\infback.c
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\zlib\inffast.c
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\zlib\inffast.h
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\zlib\inffixed.h
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\zlib\inflate.c
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\zlib\inflate.h
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\zlib\inftrees.c
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\zlib\inftrees.h
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\zlib\trees.c
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\zlib\trees.h
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\zlib\uncompr.c
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\zlib\zconf.h
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\zlib\zlib.h
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\zlib\zutil.c
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\zlib\zutil.h
# End Source File
# End Group
# Begin Group "ADFLIB"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\thirdpartylibs\adflib\Lib\adf_bitm.c
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\adflib\Lib\adf_bitm.h
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\adflib\Lib\adf_blk.h
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\adflib\Lib\adf_cache.c
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\adflib\Lib\adf_cache.h
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\adflib\Lib\adf_defs.h
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\adflib\Lib\adf_dir.c
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\adflib\Lib\adf_dir.h
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\adflib\Lib\adf_disk.c
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\adflib\Lib\adf_disk.h
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\adflib\Lib\adf_dump.c
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\adflib\Lib\adf_dump.h
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\adflib\Lib\adf_env.c
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\adflib\Lib\adf_env.h
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\adflib\Lib\adf_err.h
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\adflib\Lib\adf_file.c
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\adflib\Lib\adf_file.h
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\adflib\Lib\adf_hd.c
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\adflib\Lib\adf_hd.h
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\adflib\Lib\adf_link.c
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\adflib\Lib\adf_link.h
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\adflib\Lib\adf_nativ.c
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\adflib\Lib\adf_nativ.h
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\adflib\Lib\adf_raw.c
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\adflib\Lib\adf_raw.h
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\adflib\Lib\adf_salv.c
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\adflib\Lib\adf_salv.h
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\adflib\Lib\adf_str.h
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\adflib\Lib\adf_util.c
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\adflib\Lib\adf_util.h
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\adflib\Lib\adflib.h
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\adflib\Lib\defendian.h
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\adflib\Lib\hd_blk.h
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\adflib\Lib\Win32\nt4_dev.c
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\adflib\Lib\Win32\nt4_dev.h
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\adflib\Lib\prefix.h
# End Source File
# End Group
# Begin Group "XDMS"

# PROP Default_Filter ""
# Begin Source File

SOURCE="..\sources\thirdpartylibs\xdms\xdms-1.3.2\src\cdata.h"
# End Source File
# Begin Source File

SOURCE="..\sources\thirdpartylibs\xdms\xdms-1.3.2\src\crc_csum.c"
# End Source File
# Begin Source File

SOURCE="..\sources\thirdpartylibs\xdms\xdms-1.3.2\src\crc_csum.h"
# End Source File
# Begin Source File

SOURCE="..\sources\thirdpartylibs\xdms\xdms-1.3.2\src\getbits.c"
# End Source File
# Begin Source File

SOURCE="..\sources\thirdpartylibs\xdms\xdms-1.3.2\src\getbits.h"
# End Source File
# Begin Source File

SOURCE="..\sources\thirdpartylibs\xdms\xdms-1.3.2\src\maketbl.c"
# End Source File
# Begin Source File

SOURCE="..\sources\thirdpartylibs\xdms\xdms-1.3.2\src\maketbl.h"
# End Source File
# Begin Source File

SOURCE="..\sources\thirdpartylibs\xdms\xdms-1.3.2\src\pfile.c"
# End Source File
# Begin Source File

SOURCE="..\sources\thirdpartylibs\xdms\xdms-1.3.2\src\pfile.h"
# End Source File
# Begin Source File

SOURCE="..\sources\thirdpartylibs\xdms\xdms-1.3.2\src\tables.c"
# End Source File
# Begin Source File

SOURCE="..\sources\thirdpartylibs\xdms\xdms-1.3.2\src\tables.h"
# End Source File
# Begin Source File

SOURCE="..\sources\thirdpartylibs\xdms\xdms-1.3.2\src\u_deep.c"
# End Source File
# Begin Source File

SOURCE="..\sources\thirdpartylibs\xdms\xdms-1.3.2\src\u_deep.h"
# End Source File
# Begin Source File

SOURCE="..\sources\thirdpartylibs\xdms\xdms-1.3.2\src\u_heavy.c"
# End Source File
# Begin Source File

SOURCE="..\sources\thirdpartylibs\xdms\xdms-1.3.2\src\u_heavy.h"
# End Source File
# Begin Source File

SOURCE="..\sources\thirdpartylibs\xdms\xdms-1.3.2\src\u_init.c"
# End Source File
# Begin Source File

SOURCE="..\sources\thirdpartylibs\xdms\xdms-1.3.2\src\u_init.h"
# End Source File
# Begin Source File

SOURCE="..\sources\thirdpartylibs\xdms\xdms-1.3.2\src\u_medium.c"
# End Source File
# Begin Source File

SOURCE="..\sources\thirdpartylibs\xdms\xdms-1.3.2\src\u_medium.h"
# End Source File
# Begin Source File

SOURCE="..\sources\thirdpartylibs\xdms\xdms-1.3.2\src\u_quick.c"
# End Source File
# Begin Source File

SOURCE="..\sources\thirdpartylibs\xdms\xdms-1.3.2\src\u_quick.h"
# End Source File
# Begin Source File

SOURCE="..\sources\thirdpartylibs\xdms\xdms-1.3.2\src\u_rle.c"
# End Source File
# Begin Source File

SOURCE="..\sources\thirdpartylibs\xdms\xdms-1.3.2\src\u_rle.h"
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\xdms\vfile.c
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\xdms\vfile.h
# End Source File
# End Group
# Begin Group "CAPS"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\win32\ipf\capslibloader.c
# End Source File
# Begin Source File

SOURCE=..\sources\win32\ipf\capslibloader.h
# End Source File
# End Group
# Begin Group "LIBSAP"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\thirdpartylibs\libsap\libsap.c
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\libsap\libsap.h
# End Source File
# End Group
# Begin Group "expat"

# PROP Default_Filter ""
# Begin Source File

SOURCE="..\sources\thirdpartylibs\expat\expat-2.1.0\win32\bin\Release\libexpatMT.lib"
# End Source File
# End Group
# Begin Group "FATIOLib"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\thirdpartylibs\FATIOlib\fat_access.c
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\FATIOlib\fat_access.h
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\FATIOlib\fat_cache.c
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\FATIOlib\fat_cache.h
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\FATIOlib\fat_defs.h
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\FATIOlib\fat_filelib.c
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\FATIOlib\fat_filelib.h
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\FATIOlib\fat_format.c
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\FATIOlib\fat_format.h
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\FATIOlib\fat_list.h
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\FATIOlib\fat_misc.c
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\FATIOlib\fat_misc.h
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\FATIOlib\fat_opts.h
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\FATIOlib\fat_string.c
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\FATIOlib\fat_string.h
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\FATIOlib\fat_table.c
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\FATIOlib\fat_table.h
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\FATIOlib\fat_types.h
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\FATIOlib\fat_write.c
# End Source File
# Begin Source File

SOURCE=..\sources\thirdpartylibs\FATIOlib\fat_write.h
# End Source File
# End Group
# End Group
# Begin Group "loaders"

# PROP Default_Filter ""
# Begin Group "ADF"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\adf_loader\adf_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\adf_loader\adf_loader.h
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\adf_loader\adf_writer.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\adf_loader\adf_writer.h
# End Source File
# End Group
# Begin Group "DMS"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\dms_loader\dms_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\dms_loader\dms_loader.h
# End Source File
# End Group
# Begin Group "amigadosfs_loader"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\amigadosfs_loader\amigadosfs_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\amigadosfs_loader\amigadosfs_loader.h
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\amigadosfs_loader\stdboot3.h
# End Source File
# End Group
# Begin Group "IPF"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\ipf_loader\ipf_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\ipf_loader\ipf_loader.h
# End Source File
# End Group
# Begin Group "ST"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\st_loader\st_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\st_loader\st_loader.h
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\st_loader\stfileformat.h
# End Source File
# End Group
# Begin Group "STX"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\stx_loader\pasti_format.h
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\stx_loader\stx_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\stx_loader\stx_loader.h
# End Source File
# End Group
# Begin Group "CPCDSK"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\cpcdsk_loader\cpcdsk_format.h
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\cpcdsk_loader\cpcdsk_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\cpcdsk_loader\cpcdsk_loader.h
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\cpcdsk_loader\cpcdsk_writer.c
# End Source File
# End Group
# Begin Group "IMGPC"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\img_loader\img_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\img_loader\img_loader.h
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\img_loader\pcimgfileformat.h
# End Source File
# End Group
# Begin Group "CopyQM"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\copyqm_loader\copyqm_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\copyqm_loader\copyqm_loader.h
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\copyqm_loader\crctable.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\copyqm_loader\crctable.h
# End Source File
# End Group
# Begin Group "MSA"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\msa_loader\msa_format.h
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\msa_loader\msa_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\msa_loader\msa_loader.h
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\msa_loader\msa_writer.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\msa_loader\msa_writer.h
# End Source File
# End Group
# Begin Group "OricDSK"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\oricdsk_loader\oricdsk_format.h
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\oricdsk_loader\oricdsk_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\oricdsk_loader\oricdsk_loader.h
# End Source File
# End Group
# Begin Group "MFM"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\mfm_loader\mfm_format.h
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\mfm_loader\mfm_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\mfm_loader\mfm_loader.h
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\mfm_loader\mfm_writer.c
# End Source File
# End Group
# Begin Group "FATDK"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\fat12floppy_loader\fat12.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\fat12floppy_loader\fat12.h
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\fat12floppy_loader\fat12floppy_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\fat12floppy_loader\fat12floppy_loader.h
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\fat12floppy_loader\fat12formats.h
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\fat12floppy_loader\fatlib.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\fat12floppy_loader\fatlib.h
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\fat12floppy_loader\pcbootsector.h
# End Source File
# End Group
# Begin Group "SMC"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\smc_loader\snes_smc_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\smc_loader\snes_smc_loader.h
# End Source File
# End Group
# Begin Group "ADZ"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\adz_loader\adz_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\adz_loader\adz_loader.h
# End Source File
# End Group
# Begin Group "MSX"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\msx_loader\msx_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\msx_loader\msx_loader.h
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\msx_loader\msxfileformat.h
# End Source File
# End Group
# Begin Group "HFE"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\hfe_loader\exthfe_writer.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\hfe_loader\hfe_format.h
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\hfe_loader\hfe_hddd_a2_writer.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\hfe_loader\hfe_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\hfe_loader\hfe_loader.h
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\hfe_loader\hfe_writer.c
# End Source File
# End Group
# Begin Group "IMD"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\imd_loader\imd_format.h
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\imd_loader\imd_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\imd_loader\imd_loader.h
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\imd_loader\imd_writer.c
# End Source File
# End Group
# Begin Group "AFI"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\afi_loader\afi_format.h
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\afi_loader\afi_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\afi_loader\afi_loader.h
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\afi_loader\afi_writer.c
# End Source File
# End Group
# Begin Group "D64"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\d64_loader\d64_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\d64_loader\d64_loader.h
# End Source File
# End Group
# Begin Group "CPCFS"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\cpcfs_loader\cpcfs_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\cpcfs_loader\cpcfs_loader.h
# End Source File
# End Group
# Begin Group "TRD"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\trd_loader\trd_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\trd_loader\trd_loader.h
# End Source File
# End Group
# Begin Group "SCL"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\scl_loader\scl_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\scl_loader\scl_loader.h
# End Source File
# End Group
# Begin Group "D88"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\d88_loader\d88_format.h
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\d88_loader\d88_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\d88_loader\d88_loader.h
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\d88_loader\d88_writer.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\d88_loader\d88_writer.h
# End Source File
# End Group
# Begin Group "JV1"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\jv1_loader\jv1_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\jv1_loader\jv1_loader.h
# End Source File
# End Group
# Begin Group "RAW"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\raw_loader\raw_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\raw_loader\raw_loader.h
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\raw_loader\raw_writer.c
# End Source File
# End Group
# Begin Group "SAP"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\sap_loader\sap_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\sap_loader\sap_loader.h
# End Source File
# End Group
# Begin Group "VTR"

# PROP Default_Filter "VTR"
# Begin Source File

SOURCE=..\sources\loaders\vtr_loader\vtr_format.h
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\vtr_loader\vtr_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\vtr_loader\vtr_loader.h
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\vtr_loader\vtr_writer.c
# End Source File
# End Group
# Begin Group "HDM"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\hdm_loader\hdm_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\hdm_loader\hdm_loader.h
# End Source File
# End Group
# Begin Group "TI99PC99"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\ti99pc99_loader\ti99pc99_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\ti99pc99_loader\ti99pc99_loader.h
# End Source File
# End Group
# Begin Group "ApriDisk"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\apridisk_loader\apridisk_format.h
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\apridisk_loader\apridisk_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\apridisk_loader\apridisk_loader.h
# End Source File
# End Group
# Begin Group "EDE"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\ede_loader\ede_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\ede_loader\ede_loader.h
# End Source File
# End Group
# Begin Group "FD"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\fd_loader\fd_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\fd_loader\fd_loader.h
# End Source File
# End Group
# Begin Group "VDK"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\vdk_loader\vdk_format.h
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\vdk_loader\vdk_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\vdk_loader\vdk_loader.h
# End Source File
# End Group
# Begin Group "DPX"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\dpx_loader\dpx_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\dpx_loader\dpx_loader.h
# End Source File
# End Group
# Begin Group "Ensoniq_Mirage"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\ensoniq_mirage_loader\ensoniq_mirage_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\ensoniq_mirage_loader\ensoniq_mirage_loader.h
# End Source File
# End Group
# Begin Group "Emax"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\emax_loader\emax_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\emax_loader\emax_loader.h
# End Source File
# End Group
# Begin Group "MGT"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\mgt_loader\mgt_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\mgt_loader\mgt_loader.h
# End Source File
# End Group
# Begin Group "SAD"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\sad_loader\sad_fileformat.h
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\sad_loader\sad_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\sad_loader\sad_loader.h
# End Source File
# End Group
# Begin Group "JV3"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\jv3_loader\jv3_format.h
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\jv3_loader\jv3_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\jv3_loader\jv3_loader.h
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\jv3_loader\jv3_writer.c
# End Source File
# End Group
# Begin Group "STT"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\stt_loader\stt_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\stt_loader\stt_loader.h
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\stt_loader\sttfileformat.h
# End Source File
# End Group
# Begin Group "TeleDisk"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\teledisk_loader\td0_lzss.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\teledisk_loader\td0_lzss.h
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\teledisk_loader\teledisk_format.h
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\teledisk_loader\teledisk_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\teledisk_loader\teledisk_loader.h
# End Source File
# End Group
# Begin Group "EmuII_RAW"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\emuii_raw_loader\emuii_raw_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\emuii_raw_loader\emuii_raw_loader.h
# End Source File
# End Group
# Begin Group "EmuII"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\emuii_loader\emuii_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\emuii_loader\emuii_loader.h
# End Source File
# End Group
# Begin Group "EmuI_RAW"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\emui_raw_loader\emui_raw_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\emui_raw_loader\emui_raw_loader.h
# End Source File
# End Group
# Begin Group "JVC"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\jvc_loader\jvc_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\jvc_loader\jvc_loader.h
# End Source File
# End Group
# Begin Group "DIM"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\dim_loader\dim_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\dim_loader\dim_loader.h
# End Source File
# End Group
# Begin Group "TI99V9T9"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\ti99v9t9_loader\ti99v9t9_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\ti99v9t9_loader\ti99v9t9_loader.h
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\ti99v9t9_loader\ti99v9t9_writer.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\ti99v9t9_loader\ti99v9t9_writer.h
# End Source File
# End Group
# Begin Group "DMK"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\dmk_loader\dmk_format.h
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\dmk_loader\dmk_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\dmk_loader\dmk_loader.h
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\dmk_loader\dmk_writer.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\dmk_loader\dmk_writer.h
# End Source File
# End Group
# Begin Group "D81"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\d81_loader\d81_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\d81_loader\d81_loader.h
# End Source File
# End Group
# Begin Group "ACORNADF"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\acornadf_loader\acornadf_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\acornadf_loader\acornadf_loader.h
# End Source File
# End Group
# Begin Group "VEGASDSK"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\vegasdsk_loader\vegasdsk_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\vegasdsk_loader\vegasdsk_loader.h
# End Source File
# End Group
# Begin Group "CamputersLynxLDF"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\camputerslynxldf_loader\camputerslynxldf_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\camputerslynxldf_loader\camputerslynxldf_loader.h
# End Source File
# End Group
# Begin Group "EXTADF"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\extadf_loader\extadf_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\extadf_loader\extadf_loader.h
# End Source File
# End Group
# Begin Group "OLDEXTADF"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\oldextadf_loader\oldextadf_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\oldextadf_loader\oldextadf_loader.h
# End Source File
# End Group
# Begin Group "FDI"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\fdi_loader\fdi_format.h
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\fdi_loader\fdi_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\fdi_loader\fdi_loader.h
# End Source File
# End Group
# Begin Group "BBC_SSD_DSD"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\ssd_dsd_loader\ssd_dsd_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\ssd_dsd_loader\ssd_dsd_loader.h
# End Source File
# End Group
# Begin Group "BBC_ADL"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\adl_loader\adl_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\adl_loader\adl_loader.h
# End Source File
# End Group
# Begin Group "KRZ"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\krz_loader\krz_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\krz_loader\krz_loader.h
# End Source File
# End Group
# Begin Group "W30"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\w30_loader\w30_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\w30_loader\w30_loader.h
# End Source File
# End Group
# Begin Group "FEI"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\fei_loader\fei_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\fei_loader\fei_loader.h
# End Source File
# End Group
# Begin Group "SVD"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\svd_loader\svd.h
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\svd_loader\svd_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\svd_loader\svd_loader.h
# End Source File
# End Group
# Begin Group "IMZ"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\imz_loader\imz_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\imz_loader\imz_loader.h
# End Source File
# End Group
# Begin Group "GKH"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\gkh_loader\gkh_format.h
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\gkh_loader\gkh_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\gkh_loader\gkh_loader.h
# End Source File
# End Group
# Begin Group "Prophet"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\prophet_loader\prophet_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\prophet_loader\prophet_loader.h
# End Source File
# End Group
# Begin Group "KryoFlux_Stream"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\kryofluxstream_loader\kryofluxstream.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\kryofluxstream_loader\kryofluxstream.h
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\kryofluxstream_loader\kryofluxstream_format.h
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\kryofluxstream_loader\kryofluxstream_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\kryofluxstream_loader\kryofluxstream_loader.h
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\kryofluxstream_loader\kryofluxstream_writer.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\kryofluxstream_loader\kryofluxstream_writer.h
# End Source File
# End Group
# Begin Group "S24"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\system24_loader\system24_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\system24_loader\system24_loader.h
# End Source File
# End Group
# Begin Group "FZF"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\fzf_loader\fzf_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\fzf_loader\fzf_loader.h
# End Source File
# End Group
# Begin Group "Apple2NIB"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\apple2_nib_loader\apple2_nib_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\apple2_nib_loader\apple2_nib_loader.h
# End Source File
# End Group
# Begin Group "SDDSpeccyDos"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\sdd_speccydos_loader\sdd_speccydos_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\sdd_speccydos_loader\sdd_speccydos_loader.h
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\sdd_speccydos_loader\sdd_speccydos_writer.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\sdd_speccydos_loader\sdd_speccydos_writer.h
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\sdd_speccydos_loader\sddfileformat.h
# End Source File
# End Group
# Begin Group "Apple2DO"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\apple2_do_loader\apple2_do_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\apple2_do_loader\apple2_do_loader.h
# End Source File
# End Group
# Begin Group "Arburg"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\arburg_raw_loader\arburg_raw_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\arburg_raw_loader\arburg_raw_loader.h
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\arburg_raw_loader\arburg_raw_writer.c
# End Source File
# End Group
# Begin Group "SCP"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\scp_loader\scp_format.h
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\scp_loader\scp_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\scp_loader\scp_loader.h
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\scp_loader\scp_writer.c
# End Source File
# End Group
# Begin Group "BMP"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\loaders\bmp_loader\bmp_file.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\bmp_loader\bmp_file.h
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\bmp_loader\bmp_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\bmp_loader\bmp_loader.h
# End Source File
# Begin Source File

SOURCE=..\sources\loaders\bmp_loader\bmp_writer.c
# End Source File
# End Group
# End Group
# Begin Group "tracks"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\tracks\apple2.c
# End Source File
# Begin Source File

SOURCE=..\sources\tracks\apple2.h
# End Source File
# Begin Source File

SOURCE=..\sources\tracks\arburg_track.c
# End Source File
# Begin Source File

SOURCE=..\sources\tracks\arburg_track.h
# End Source File
# Begin Source File

SOURCE=..\sources\tracks\display_track.c
# End Source File
# Begin Source File

SOURCE=..\sources\tracks\display_track.h
# End Source File
# Begin Source File

SOURCE=..\sources\tracks\emuii_track.c
# End Source File
# Begin Source File

SOURCE=..\sources\tracks\gcr_track.c
# End Source File
# Begin Source File

SOURCE=..\sources\tracks\sector_extractor.c
# End Source File
# Begin Source File

SOURCE=..\sources\tracks\track_generator.c
# End Source File
# Begin Source File

SOURCE=..\sources\tracks\trackutils.c
# End Source File
# Begin Source File

SOURCE=..\sources\tracks\trackutils.h
# End Source File
# End Group
# Begin Group "xml_disk"

# PROP Default_Filter ""
# Begin Group "Layouts"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\xml_disk\DiskLayouts\data_DiskLayout_DOS_DD_720KB_xml.h
# End Source File
# Begin Source File

SOURCE=..\sources\xml_disk\DiskLayouts\data_DiskLayout_DOS_HD_1_44MB_xml.h
# End Source File
# Begin Source File

SOURCE=..\sources\xml_disk\DiskLayouts\LayoutsIndex.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\sources\xml_disk\packer\lzw.c
# End Source File
# Begin Source File

SOURCE=..\sources\xml_disk\packer\lzw.h
# End Source File
# Begin Source File

SOURCE=..\sources\xml_disk\packer\pack.c
# End Source File
# Begin Source File

SOURCE=..\sources\xml_disk\packer\pack.h
# End Source File
# Begin Source File

SOURCE=..\sources\xml_disk\packer\rle.c
# End Source File
# Begin Source File

SOURCE=..\sources\xml_disk\packer\rle.h
# End Source File
# Begin Source File

SOURCE=..\sources\xml_disk\xml_disk.c
# End Source File
# Begin Source File

SOURCE=..\sources\xml_disk\xml_disk.h
# End Source File
# End Group
# Begin Group "fs"

# PROP Default_Filter ""
# Begin Group "fs_fat12"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\fs_manager\fs_fat12\fs_fat12.c
# End Source File
# Begin Source File

SOURCE=..\sources\fs_manager\fs_fat12\fs_fat12.h
# End Source File
# End Group
# Begin Group "fs_amigados"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\fs_manager\fs_amigados\fs_amigados.c
# End Source File
# Begin Source File

SOURCE=..\sources\fs_manager\fs_amigados\fs_amigados.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\sources\fs_manager\fs_manager.c
# End Source File
# Begin Source File

SOURCE=..\sources\fs_manager\fs_manager.h
# End Source File
# End Group
# Begin Group "fluxStreamAnalyzer"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources\stream_analyzer\fluxStreamAnalyzer.c
# End Source File
# Begin Source File

SOURCE=..\sources\stream_analyzer\fluxStreamAnalyzer.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\sources\tracks\crc.c
# End Source File
# Begin Source File

SOURCE=..\sources\floppy_ifmode.c
# End Source File
# Begin Source File

SOURCE=..\sources\floppy_loader.c
# End Source File
# Begin Source File

SOURCE=..\sources\floppy_utils.c
# End Source File
# Begin Source File

SOURCE=..\sources\win32\libhxcfe.def
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\sources\tracks\crc.h
# End Source File
# Begin Source File

SOURCE=..\sources\tracks\emuii_track.h
# End Source File
# Begin Source File

SOURCE=..\sources\floppy_loader.h
# End Source File
# Begin Source File

SOURCE=..\sources\floppy_utils.h
# End Source File
# Begin Source File

SOURCE=..\sources\tracks\gcr_track.h
# End Source File
# Begin Source File

SOURCE=..\sources\internal_floppy.h
# End Source File
# Begin Source File

SOURCE=..\sources\libhxcfe.h
# End Source File
# Begin Source File

SOURCE=..\sources\licensetxt.h
# End Source File
# Begin Source File

SOURCE=..\sources\loaders_list.h
# End Source File
# Begin Source File

SOURCE=..\sources\os_api.h
# End Source File
# Begin Source File

SOURCE=..\sources\plugins_id.h
# End Source File
# Begin Source File

SOURCE=..\sources\tracks\sector_extractor.h
# End Source File
# Begin Source File

SOURCE=..\sources\tracks\track_generator.h
# End Source File
# Begin Source File

SOURCE=..\sources\types.h
# End Source File
# Begin Source File

SOURCE=..\sources\version.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\sources\win32\libhxcfe.rc
# End Source File
# End Group
# End Target
# End Project
