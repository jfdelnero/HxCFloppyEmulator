/*
//
// Copyright (C) 2006 - 2012 Jean-François DEL NERO
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
// File : floppy_ifmode.c
// Contains: Floppy Interface functions
//
// Written by:	DEL NERO Jean Francois
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <string.h>
#include <stdio.h>


#include "libhxcfe.h"
#include "floppy_loader.h"
#include "floppy_utils.h"

typedef struct interfacemode_
{
	int ifmodeid;
	const char* ifmodename;
	const char* ifmodedesc;
}interfacemode;

typedef struct trackmode_
{
	int trackmodeid;
	const char* trackmodename;
	const char* trackmodedesc;
}trackmode;



trackmode trackmodelist[]=
{
	{ISOIBM_MFM_ENCODING,		"ISOIBM_MFM_ENCODING",	"" },
	{AMIGA_MFM_ENCODING,		"AMIGA_MFM_ENCODING",	"" },
	{ISOIBM_FM_ENCODING,		"ISOIBM_FM_ENCODING",	"" },
	{EMU_FM_ENCODING,			"EMU_FM_ENCODING",		"" },
	{TYCOM_FM_ENCODING,			"TYCOM_FM_ENCODING",	"" },
	{MEMBRAIN_MFM_ENCODING,		"MEMBRAIN_MFM_ENCODING","" },
	{UNKNOWN_ENCODING,			"UNKNOWN_ENCODING",		"" },
	{-1,"",	""},
};

interfacemode interfacemodelist[]=
{
	{IBMPC_DD_FLOPPYMODE,			"IBMPC_DD_FLOPPYMODE",			"PC Interface (720KB/DD Disk)" },
	{IBMPC_HD_FLOPPYMODE,			"IBMPC_HD_FLOPPYMODE",			"PC Interface (1.44MB/HD Disk)" },
	{ATARIST_DD_FLOPPYMODE,			"ATARIST_DD_FLOPPYMODE",		"Atari Interface (720KB/DD Disk)" },
	{ATARIST_HD_FLOPPYMODE,			"ATARIST_HD_FLOPPYMODE",		"Atari Interface (1.44MB/HD Disk)" },
	{AMIGA_DD_FLOPPYMODE,			"AMIGA_DD_FLOPPYMODE",			"Amiga Interface (880KB/DD)" },
	{AMIGA_HD_FLOPPYMODE,			"AMIGA_HD_FLOPPYMODE",			"Amiga Interface (1.76MB/HD)" },
	{CPC_DD_FLOPPYMODE,				"CPC_DD_FLOPPYMODE",			"Amstrad CPC Interface" },
	{GENERIC_SHUGART_DD_FLOPPYMODE,	"GENERIC_SHUGART_DD_FLOPPYMODE","Shugart Interface" },
	{IBMPC_ED_FLOPPYMODE,			"IBMPC_ED_FLOPPYMODE",			"PC Interface (2.88MB/ED Disk)" },
	{MSX2_DD_FLOPPYMODE,			"MSX2_DD_FLOPPYMODE",			"MSX Interface" },
	{C64_DD_FLOPPYMODE,				"C64_DD_FLOPPYMODE",			"C64 Interface" },
	{EMU_SHUGART_FLOPPYMODE,		"EMU_SHUGART_FLOPPYMODE",		"E-mu Interface" },
	{S950_DD_FLOPPYMODE,			"S950_DD_FLOPPYMODE",			"Akai S900/S950 Interface (800KB/DD Disk)" },
	{S950_HD_FLOPPYMODE,			"S950_HD_FLOPPYMODE",			"Akai S950 Interface (1.6MB/HD Disk)" },
	{-1,"",	""},
};




int hxcfe_getFloppyInterfaceModeID(HXCFLOPPYEMULATOR* floppycontext,char * ifmode)
{
	int i;

	i=0;

	while( strcmp( interfacemodelist[i].ifmodename , ifmode ) && ( interfacemodelist[i].ifmodeid >= 0 ) )
	{
		i++;
	}

	return interfacemodelist[i].ifmodeid;

}

const char * hxcfe_getFloppyInterfaceModeName(HXCFLOPPYEMULATOR* floppycontext,int ifmodeid)
{
	
	int i;

	i=0;

	while( ( interfacemodelist[i].ifmodeid != ifmodeid ) && ( interfacemodelist[i].ifmodeid >= 0 ) )
	{
		i++;
	}

	if( interfacemodelist[i].ifmodeid <0 )
		return NULL;
	else
		return interfacemodelist[i].ifmodename;

}

const char * hxcfe_getFloppyInterfaceModeDesc(HXCFLOPPYEMULATOR* floppycontext,int ifmodeid)
{
	int i;

	i=0;

	while( ( interfacemodelist[i].ifmodeid != ifmodeid ) && ( interfacemodelist[i].ifmodeid >= 0 ) )
	{
		i++;
	}

	if( interfacemodelist[i].ifmodeid <0 )
		return NULL;
	else
		return interfacemodelist[i].ifmodedesc;
}


const char * hxcfe_getTrackEncodingName(HXCFLOPPYEMULATOR* floppycontext,int trackencodingid)
{
	
	int i;

	i=0;

	while( ( trackmodelist[i].trackmodeid != trackencodingid ) && ( trackmodelist[i].trackmodeid >= 0 ) )
	{
		i++;
	}

	if( trackmodelist[i].trackmodeid <0 )
		return NULL;
	else
		return trackmodelist[i].trackmodename;

}
