/*
//
// Copyright (C) 2006-2025 Jean-Fran�ois DEL NERO
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
// Written by: Jean-Fran�ois DEL NERO
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "types.h"

#include "internal_libhxcfe.h"
#include "libhxcfe.h"
#include "floppy_loader.h"
#include "floppy_utils.h"

typedef struct interfacemode_
{
	int32_t ifmodeid;
	const char* ifmodename;
	const char* ifmodedesc;
}interfacemode;

typedef struct trackmode_
{
	int32_t trackmodeid;
	const char* trackmodename;
	const char* trackmodedesc;
}trackmode;

trackmode trackmodelist[]=
{
	{ISOIBM_MFM_ENCODING,           "ISOIBM_MFM_ENCODING",  "" },
	{AMIGA_MFM_ENCODING,            "AMIGA_MFM_ENCODING",   "" },
	{ISOIBM_FM_ENCODING,            "ISOIBM_FM_ENCODING",   "" },
	{EMU_FM_ENCODING,               "EMU_FM_ENCODING",      "" },
	{TYCOM_FM_ENCODING,             "TYCOM_FM_ENCODING",    "" },
	{MEMBRAIN_MFM_ENCODING,         "MEMBRAIN_MFM_ENCODING","" },
	{APPLEII_GCR1_ENCODING,         "APPLEII_GCR1_ENCODING","" },
	{APPLEII_GCR2_ENCODING,         "APPLEII_GCR2_ENCODING","" },
	{APPLEMAC_GCR_ENCODING,         "APPLEMAC_GCR_ENCODING","" },
	{APPLEII_HDDD_A2_GCR1_ENCODING, "APPLEII_HDDD_A2_GCR1_ENCODING","" },
	{APPLEII_HDDD_A2_GCR2_ENCODING, "APPLEII_HDDD_A2_GCR2_ENCODING","" },
	{ARBURGDAT_ENCODING,            "ARBURG_DATA_ENCODING","" },
	{ARBURGSYS_ENCODING,            "ARBURG_SYSTEM_ENCODING","" },
	{NORTHSTAR_HS_MFM_ENCODING,     "NORTHSTAR_HS_MFM_ENCODING", "" },
	{HEATHKIT_HS_FM_ENCODING,       "HEATHKIT_HS_FM_ENCODING", "" },
	{DEC_RX02_M2FM_ENCODING,        "DEC_RX02_M2FM_ENCODING", "" },
	{QD_MO5_ENCODING,               "QD_MO5_ENCODING","" },
	{C64_GCR_ENCODING,              "C64_GCR_ENCODING","" },
	{VICTOR9K_GCR_ENCODING,         "VICTOR9K_GCR_ENCODING","" },
	{MICRALN_HS_FM_ENCODING,        "MICRALN_HS_FM_ENCODING","" },
	{UNKNOWN_ENCODING,              "UNKNOWN_ENCODING", "" },
	{-1,"", ""},
};

interfacemode interfacemodelist[]=
{
	{IBMPC_DD_FLOPPYMODE,           "IBMPC_DD_FLOPPYMODE",          "PC Interface (720KB/DD Disk)" },
	{IBMPC_HD_FLOPPYMODE,           "IBMPC_HD_FLOPPYMODE",          "PC Interface (1.44MB/HD Disk)" },
	{ATARIST_DD_FLOPPYMODE,         "ATARIST_DD_FLOPPYMODE",        "Atari Interface (720KB/DD Disk)" },
	{ATARIST_HD_FLOPPYMODE,         "ATARIST_HD_FLOPPYMODE",        "Atari Interface (1.44MB/HD Disk)" },
	{AMIGA_DD_FLOPPYMODE,           "AMIGA_DD_FLOPPYMODE",          "Amiga Interface (880KB/DD)" },
	{AMIGA_HD_FLOPPYMODE,           "AMIGA_HD_FLOPPYMODE",          "Amiga Interface (1.76MB/HD)" },
	{CPC_DD_FLOPPYMODE,             "CPC_DD_FLOPPYMODE",            "Amstrad CPC Interface" },
	{GENERIC_SHUGART_DD_FLOPPYMODE, "GENERIC_SHUGART_DD_FLOPPYMODE","Shugart Interface" },
	{IBMPC_ED_FLOPPYMODE,           "IBMPC_ED_FLOPPYMODE",          "PC Interface (2.88MB/ED Disk)" },
	{MSX2_DD_FLOPPYMODE,            "MSX2_DD_FLOPPYMODE",           "MSX Interface" },
	{C64_DD_FLOPPYMODE,             "C64_DD_FLOPPYMODE",            "C64 Interface" },
	{EMU_SHUGART_FLOPPYMODE,        "EMU_SHUGART_FLOPPYMODE",       "E-mu Interface" },
	{S950_DD_FLOPPYMODE,            "S950_DD_FLOPPYMODE",           "Akai S900/S950 Interface (800KB/DD Disk)" },
	{S950_HD_FLOPPYMODE,            "S950_HD_FLOPPYMODE",           "Akai S950 Interface (1.6MB/HD Disk)" },
	{S950_DD_HD_FLOPPYMODE,         "S950_DD_HD_FLOPPYMODE",        "Akai S950 Interface (Automatic density selection)" },
	{IBMPC_DD_HD_FLOPPYMODE,        "IBMPC_DD_HD_FLOPPYMODE",       "PC Interface (Automatic density selection)" },
	{QUICKDISK_FLOPPYMODE,          "QUICKDISK_FLOPPYMODE",         "Quickdisk Interface" },
	{-1,"", ""},
};

int32_t hxcfe_getFloppyInterfaceModeID( HXCFE* floppycontext, char * ifmode )
{
	int i;

	if(floppycontext)
	{
		i=0;

		while( strcmp( interfacemodelist[i].ifmodename , ifmode ) && ( interfacemodelist[i].ifmodeid >= 0 ) )
		{
			i++;
		}

		return interfacemodelist[i].ifmodeid;
	}

	return 0;
}

const char * hxcfe_getFloppyInterfaceModeName( HXCFE* floppycontext, int32_t ifmodeid )
{

	int i;

	if(floppycontext)
	{
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

	return 0;
}

const char * hxcfe_getFloppyInterfaceModeDesc( HXCFE* floppycontext, int32_t ifmodeid )
{
	int i;

	if(floppycontext)
	{

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

	return 0;
}

const char * hxcfe_getTrackEncodingName( HXCFE* floppycontext,int32_t trackencodingid )
{

	int i;

	if(floppycontext)
	{

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

	return 0;
}
