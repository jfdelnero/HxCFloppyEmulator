/*
//
// Copyright (C) 2006-2023 Jean-François DEL NERO
//
// This file is part of the HxCFloppyEmulator library
//
// HxCFloppyEmulator may be used and distributed without restriction provided
// that this copyright statement is not removed from the file and that any
// derivative work contains the original copyright notice and the associated
// disclaimer.
//
// HxCFloppyEmulator is free software; you can redistribute it
// and/or modify  it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// HxCFloppyEmulator is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//   See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with HxCFloppyEmulator; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//
*/
///////////////////////////////////////////////////////////////////////////////////
//-------------------------------------------------------------------------------//
//-------------------------------------------------------------------------------//
//-----------H----H--X----X-----CCCCC----22222----0000-----0000------11----------//
//----------H----H----X-X-----C--------------2---0----0---0----0--1--1-----------//
//---------HHHHHH-----X------C----------22222---0----0---0----0-----1------------//
//--------H----H----X--X----C----------2-------0----0---0----0-----1-------------//
//-------H----H---X-----X---CCCCC-----222222----0000-----0000----1111------------//
//-------------------------------------------------------------------------------//
//----------------------------------------------------- http://hxc2001.free.fr --//
///////////////////////////////////////////////////////////////////////////////////
// File : capslibloader.c
// Contains: Windows, Linux, mac OS SPS CAPS library loader
//
// Written by: Jean-François DEL NERO, Riku Anttila
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#ifdef WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include <stdint.h>

#include "internal_libhxcfe.h"
#include "libhxcfe.h"

#ifndef WIN32
typedef uint8_t BYTE;
typedef uint32_t DWORD;
typedef uint16_t WORD;
typedef void* HMODULE;
#endif

#include "thirdpartylibs/capslib/Comtype.h"
#include "thirdpartylibs/capslib/CapsAPI.h"

typedef SDWORD (* CAPSINIT)(void);
typedef SDWORD (* CAPSADDIMAGE)(void);
typedef SDWORD (* CAPSLOCKIMAGEMEMORY)(SDWORD,PUBYTE,UDWORD,UDWORD);
typedef SDWORD (* CAPSUNLOCKIMAGE)(SDWORD);
typedef SDWORD (* CAPSLOADIMAGE)(SDWORD,UDWORD);
typedef SDWORD (* CAPSGETIMAGEINFO)(PCAPSIMAGEINFO,SDWORD);
typedef SDWORD (* CAPSLOCKTRACK)(PCAPSTRACKINFO,SDWORD,UDWORD,UDWORD,UDWORD);
typedef SDWORD (* CAPSUNLOCKTRACK)(SDWORD id, UDWORD cylinder, UDWORD head);
typedef SDWORD (* CAPSUNLOCKALLTRACKS)(SDWORD);
typedef SDWORD (* CAPSGETVERSIONINFO)(PCAPSVERSIONINFO,UDWORD);
typedef SDWORD (* CAPSREMIMAGE)(SDWORD id);

CAPSINIT pCAPSInit = 0;
CAPSADDIMAGE pCAPSAddImage = 0;
CAPSLOCKIMAGEMEMORY pCAPSLockImageMemory = 0;
CAPSUNLOCKIMAGE pCAPSUnlockImage = 0;
CAPSLOADIMAGE pCAPSLoadImage = 0;
CAPSGETIMAGEINFO pCAPSGetImageInfo = 0;
CAPSLOCKTRACK pCAPSLockTrack = 0;
CAPSUNLOCKTRACK pCAPSUnlockTrack = 0;
CAPSUNLOCKALLTRACKS pCAPSUnlockAllTracks = 0;
CAPSGETVERSIONINFO pCAPSGetVersionInfo = 0;
CAPSREMIMAGE pCAPSRemImage = 0;

typedef int (*func_ptr)(void);

typedef struct _lib_funcs_def
{
	const char * name;
	func_ptr *ptr;
}lib_funcs_def;

lib_funcs_def capslib_lib_funcs_def[]=
{
	{"CAPSInit",            (func_ptr *)&pCAPSInit},
	{"CAPSAddImage",        (func_ptr *)&pCAPSAddImage},
	{"CAPSLockImageMemory", (func_ptr *)&pCAPSLockImageMemory},
	{"CAPSUnlockImage",     (func_ptr *)&pCAPSUnlockImage},
	{"CAPSLoadImage",       (func_ptr *)&pCAPSLoadImage},
	{"CAPSGetImageInfo",    (func_ptr *)&pCAPSGetImageInfo},
	{"CAPSLockTrack",       (func_ptr *)&pCAPSLockTrack},
	{"CAPSUnlockTrack",     (func_ptr *)&pCAPSUnlockTrack},
	{"CAPSUnlockAllTracks", (func_ptr *)&pCAPSUnlockAllTracks},
	{"CAPSGetVersionInfo",  (func_ptr *)&pCAPSGetVersionInfo},
	{"CAPSRemImage",        (func_ptr *)&pCAPSRemImage},
	{ 0, 0 }
};

int init_caps_lib(HXCFE* hxcfe)
{
	int i;
	HMODULE h = 0;
	void* ret;
	char * libname;

	i = 0;
	while( capslib_lib_funcs_def[i].name )
	{
		if( !(*capslib_lib_funcs_def[i].ptr) )
			break; // Function not loaded.
		i++;
	}

	// End of array reach -> All functions already loaded.
	if( !capslib_lib_funcs_def[i].name )
		return 1;

	libname = hxcfe_getEnvVar( hxcfe, "SPSCAPS_LIB_NAME", 0 );
	if(!libname)
	{
		hxcfe->hxc_printf(MSG_ERROR,"init_caps_lib : Library name not defined !",libname);

		return 0;
	}

#ifdef WIN32
	h = LoadLibrary (libname);
#else
	h = dlopen(libname, RTLD_LAZY);
#endif
	if(!h)
	{
		hxcfe->hxc_printf(MSG_ERROR,"init_caps_lib : Can't load %s ! Library not found ?",libname);
		return 0;
	}

	// Load symbols...
	i = 0;
	while( capslib_lib_funcs_def[i].name )
	{
		#ifdef WIN32
			ret = (void*)GetProcAddress (h, capslib_lib_funcs_def[i].name);
		#else
			ret = (void*)dlsym (h, capslib_lib_funcs_def[i].name);
		#endif

		if( ret )
			*capslib_lib_funcs_def[i].ptr = (void*)ret;
		else
			break; // Missing entry ?

		i++;
	}

	// End of array reached -> All functions loaded.
	if( !capslib_lib_funcs_def[i].name )
		return 1;

	// One or more function missing...
	hxcfe->hxc_printf(MSG_ERROR,"init_caps_lib : Can't load the needed entries from %s ! Bad library ?",libname);

#ifdef WIN32
	FreeLibrary(h);
#else
	dlclose(h);
#endif

	i = 0;
	while( capslib_lib_funcs_def[i].name )
	{
		capslib_lib_funcs_def[i].ptr = 0;
		i++;
	}

	return 0;
};
