/*
//
// Copyright (C) 2006-2024 Jean-François DEL NERO
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
//-----------H----H--X----X-----CCCCC-----22222----0000-----0000-----11----------//
//----------H----H----X-X-----C--------------2---0----0---0----0---1-1-----------//
//---------HHHHHH-----X------C----------22222---0----0---0----0-----1------------//
//--------H----H----X--X----C----------2-------0----0---0----0-----1-------------//
//-------H----H---X-----X---CCCCC-----22222----0000-----0000----11111------------//
//-------------------------------------------------------------------------------//
//----------------------------------------------------- http://hxc2001.free.fr --//
///////////////////////////////////////////////////////////////////////////////////
// File : track_types_defs.c
// Contains: Track format definition
//
// Written by: Jean-François DEL NERO
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include "types.h"

#include "libhxcfe.h"
#include "track_types_defs.h"

isoibm_config formatstab[]=
{
	{
		IBMFORMAT_SD,

		0xFF,40, 				// post index gap4 config

		0x00, 6,				// index sync config

		0x00,0x00,0,			// index mark coding
		0xFC,0xD7,1,

		0xFF,26,				// gap1 config

		0x00, 6,				// h sync config

		0x00, 6,				// d sync config

		0x00,0x00,0,			// address mark coding
		0xFE,0xC7,1,

		0xFF,11,				// gap2 config

		0x00,0x00,0,			// data mark coding
		0xFB,0xC7,1,

		0xFF,84,				// gap3 config

		0xFF,255,				// gap4 config

		0xFF,0xFF,0xFF,0xFF,	// Track - Side - Sector - Size

		0x1021,0xFFFF,			// crc, initial value

		0x00,0x00,0,			// post crc header glith
		0x00,0x00,0 			// post crc data glith
	},
	{
		IBMFORMAT_DD,

		0x4E,80,				// post index gap4 config

		0x00,12,				// index sync config

		0xC2,0x14,3,			// index mark coding
		0xFC,0xFF,1,

		0x4E,50,				// gap1 config

		0x00,12,				// h sync config

		0x00,12,				// d sync config

		0xA1,0x0A,3,			// address mark coding
		0xFE,0xFF,1,

		0x4E,22,				// gap2 config

		0xA1,0x0A,3,			// data mark coding
		0xFB,0xFF,1,

		0x4E,84,				// gap3 config

		0x4E,255,				// gap4 config

		0xFF,0xFF,0xFF,0xFF,	// Track - Side - Sector - Size

		0x1021,0xFFFF,			// crc, initial value

		0x00,0x00,0,			// post crc header glith
		0x00,0x00,0 			// post crc data glith
	},
	{
		ISOFORMAT_SD,

		0xFF,00,				// post index gap4 config

		0x00,00,				// index sync config

		0x00,0x00,0,			// index mark coding
		0xFC,0xD7,1,

		0xFF,16,				// gap1 config

		0x00,06,				// h sync config

		0x00,06,				// d sync config

		0x00,0x00,0,			// address mark coding
		0xFE,0xC7,1,

		0xFF,11,				// gap2 config

		0x00,0x00,0,			// data mark coding
		0xFB,0xC7,1,

		0xFF,84,				// gap3 config

		0xFF,255,				// gap4 config

		0xFF,0xFF,0xFF,0xFF,	// Track - Side - Sector - Size

		0x1021,0xFFFF,			// crc, initial value

		0x00,0x00,0,			// post crc header glith
		0x00,0x00,0				// post crc data glith
	},
	{
		ISOFORMAT_DD,

		0x4E,00,				// post index gap4 config

		0x00,00,				// index sync config

		0x00,0x00,0,			// index mark coding
		0x00,0x00,0,

		0x4E,32,				// gap1 config

		0x00,12,				// h sync config

		0x00,12,				// d sync config

		0xA1,0x0A,3,			// address mark coding
		0xFE,0xFF,1,

		0x4E,22,				// gap2 config

		0xA1,0x0A,3,			// data mark coding
		0xFB,0xFF,1,

		0x4E,84,				// gap3 config

		0x4E,255,				// gap4 config

		0xFF,0xFF,0xFF,0xFF,	// Track - Side - Sector - Size

		0x1021,0xFFFF,			// crc, initial value

		0x00,0x00,0,			// post crc header glith
		0x00,0x00,0				// post crc data glith
	},
	{
		ISOFORMAT_DD11S,

		0x4E,00,				// post index gap4 config

		0x00,00,				// index sync config

		0x00,0x00,0,			// index mark coding
		0x00,0x00,0,

		0x4E,00,				// gap1 config

		0x00,03,				// h sync config

		0x00,12,				// d sync config

		0xA1,0x0A,3,			// address mark coding
		0xFE,0xFF,1,

		0x4E,22,				// gap2 config

		0xA1,0x0A,3,			// data mark coding
		0xFB,0xFF,1,

		0x4E,5,					// gap3 config

		0x4E,0xFF,				// gap4 config

		0xFF,0xFF,0xFF,0xFF,	// Track - Side - Sector - Size

		0x1021,0xFFFF,			// crc, initial value

		0x00,0x00,0,			// post crc header glith
		0x00,0x00,0				// post crc data glith
	},
	{
		AMIGAFORMAT_DD,

		0x4E,00,				// post index gap4 config

		0x00,00,				// index sync config

		0x00,0x00,0,			// index mark coding
		0x00,0x00,0,

		0x4E,00,				// gap1 config

		0x00,02,				// h sync config

		0x00,12,				// d sync config

		0xA1,0x0A,2,			// address mark coding (0x4489 0x4489)
		0xFF,0xFF,0,

		0x00,16,				// gap2 config

		0xA1,0x0A,0,			// data mark coding
		0xFB,0xFF,0,

		0x00,0,					// gap3 config

		0xFF,0xFF,				// gap4 config

		0xFF,0xFF,0xFF,0xFF,	// Track - Side - Sector - Size

		0x0000,0x0000,			// crc, initial value

		0x00,0x00,0,			// post crc header glith
		0x00,0x00,0				// post crc data glith
	},
	{
		TYCOMFORMAT_SD,

		0xFF,00,				// post index gap4 config

		0x00,00,				// index sync config

		0x00,0x00,0,			// index mark coding
		0xFC,0xD7,1,

		0xFF,16,				// gap1 config

		0x00,04,				// h sync config

		0x00,04,				// d sync config

		0x00,0x00,0,			// address mark coding
		0xFE,0xC7,1,

		0xFF,6,					// gap2 config

		0x00,0x00,0,			// data mark coding
		0xFB,0xC7,1,

		0xFF,84,				// gap3 config

		0xFF,255,				// gap4 config

		0xFF,0x00,0xFF,0x00,	// Track - Side - Sector - Size

		0x1021,0xFFFF,			// crc, initial value

		0x00,0x00,0,			// post crc header glith
		0x00,0x00,0				// post crc data glith
	},
	{
		MEMBRAINFORMAT_DD,

		0x4E,00,				// post index gap4 config

		0x00,00,				// index sync config

		0x00,0x00,0,			// index mark coding
		0x00,0x00,0,

		0x4E,32,				// gap1 config

		0x00,12,				// h sync config

		0x00,12,				// d sync config

		0xA1,0x0A,1,			// address mark coding
		0xFE,0xFF,1,

		0x4E,22,				// gap2 config

		0xA1,0x0A,1,			// data mark coding
		0xF8,0xFF,1,

		0x4E,84,				// gap3 config

		0x4E,255,				// gap4 config

		0xFF,0xFF,0xFF,0xFF,	// Track - Side - Sector - Size

		0x8005,0x0000,			// crc, initial value

		0x00,0x00,0,			// post crc header glith
		0x00,0x00,0				// post crc data glith
	},
	{
		EMUFORMAT_SD,

		0x00,00,				// post index gap4 config

		0x00,00,				// index sync config

		0x00,0x00,0,			// index mark coding
		0x00,0x00,0,

		0xFF,20,				// gap1 config

		0x00,12,				// h sync config

		0x00,12,				// d sync config

		0x00,0xFF,4,			// address mark coding
		0xFA,0x00,2,

		0xFF,8,					// gap2 config

		0x00,0xFF,4,			// data mark coding
		0xFA,0x00,2,

		0xFF,84,				// gap3 config

		0xFF,255,				// gap4 config

		0xFF,0xFF,0xFF,0xFF,	// Track - Side - Sector - Size

		0x8005,0x0000,			// crc, initial value

		0x00,0x00,0,			// post crc header glith
		0x00,0x00,0				// post crc data glith
	},
	{
		APPLE2_GCR5A3,

		0xFF,00,				// post index gap4 config

		0x00,00,				// index sync config

		0x00,0x00,0,			// index mark coding
		0x00,0x00,0,

		0xFF,20,				// gap1 config

		0x00,12,				// h sync config

		0x00,12,				// d sync config

		0x00,0xFF,4,			// address mark coding
		0xFA,0x00,2,

		0xFF,8,					// gap2 config

		0x00,0xFF,4,			// data mark coding
		0xFA,0x00,2,

		0xFF,84,				// gap3 config

		0xFF,255,				// gap4 config

		0xFF,0xFF,0xFF,0xFF,	// Track - Side - Sector - Size

		0x0000,0x0000,			// crc, initial value

		0x00,0x00,0,			// post crc header glith
		0x00,0x00,0				// post crc data glith
	},
	{
		APPLE2_GCR6A2,

		0xFF,00,				// post index gap4 config

		0x00,00,				// index sync config

		0x00,0x00,0,			// index mark coding
		0x00,0x00,0,

		0xFF,20,				// gap1 config

		0x00,12,				// h sync config

		0x00,12,				// d sync config

		0x00,0xFF,4,			// address mark coding
		0xFA,0x00,2,

		0xFF,8,					// gap2 config

		0x00,0xFF,4,			// data mark coding
		0xFA,0x00,2,

		0xFF,84,				// gap3 config

		0xFF,255,				// gap4 config

		0xFF,0xFF,0xFF,0xFF,	// Track - Side - Sector - Size

		0x0000,0x0000,			// crc, initial value

		0x00,0x00,0,			// post crc header glith
		0x00,0x00,0				// post crc data glith
	},
	{
		ARBURG_DAT,

		0x00,00,				// post index gap4 config

		0x00,00,				// index sync config

		0x00,0x00,0,			// index mark coding
		0x00,0x00,0,

		0x00,56,				// gap1 config

		0x00,00,				// h sync config

		0x00,00,				// d sync config

		0x00,0x00,0,			// address mark coding
		0x00,0x00,0,

		0x00,00,				// gap2 config

		0x00,0x00,0,			// data mark coding
		0xFF,0xFF,1,

		0x00,01,				// gap3 config

		0x00,255,				// gap4 config

		0x00,0x00,0x00,0x00,	// Track - Side - Sector - Size

		0x0000,0x0000,			// crc, initial value

		0x00,0x00,0,			// post crc header glith
		0x00,0x00,0				// post crc data glith
	},
	{
		ARBURG_SYS,

		0x00,00,				// post index gap4 config

		0x00,00,				// index sync config

		0x00,0x00,0,			// index mark coding
		0x00,0x00,0,

		0x00,56,				// gap1 config

		0x00,00,				// h sync config

		0x00,00,				// d sync config

		0x00,0x00,0,			// address mark coding
		0x00,0x00,0,

		0x00,00,				// gap2 config

		0x00,0x00,0,			// data mark coding
		0xFF,0xFF,1,

		0x00,01,				// gap3 config

		0x00,255,				// gap4 config

		0x00,0x00,0x00,0x00,	// Track - Side - Sector - Size

		0x0000,0x0000,			// crc, initial value

		0x00,0x00,0,			// post crc header glith
		0x00,0x00,0				// post crc data glith
	},
	{
		UKNCFORMAT_DD,

		0x4E,00,				// post index gap4 config

		0x00,00,				// index sync config

		0x00,0x00,0,			// index mark coding
		0x00,0x00,0,

		0x4E,27,				// gap1 config

		0x00,12,				// h sync config

		0x00,12,				// d sync config

		0xA1,0x0A,3,			// address mark coding
		0xFE,0xFF,1,

		0x4E,24,				// gap2 config

		0xA1,0x0A,3,			// data mark coding
		0xFB,0xFF,1,

		0x4E,39,				// gap3 config

		0x4E,255,				// gap4 config

		0xFF,0xFF,0xFF,0xFF,	// Track - Side - Sector - Size

		0x1021,0xFFFF,			// crc, initial value

		0xA1,0x0A,1,			// post crc header glith
		0xA1,0x0A,3				// post crc data glith
	},
	{
		AED6200P_DD,

		0x00,00,				// post index gap4 config

		0x00,00,				// index sync config

		0x00,0x00,0,			// index mark coding
		0x00,0x00,0,

		0x00,32,				// gap1 config

		0x00,0,				// h sync config

		0x00,0,				// d sync config

		0xC6,0xEB,1,			// address mark coding
		0x00,0xFF,0,

		0x00,22,				// gap2 config

		0xC0,0xEB,1,			// data mark coding
		0x00,0xFF,0,

		0x00,84,				// gap3 config

		0x00,255,				// gap4 config

		0xFF,0xFF,0xFF,0xFF,	// Track - Side - Sector - Size

		0x1021,0xFFFF,			// crc, initial value

		0x00,0x00,0,			// post crc header glith
		0x00,0x00,0				// post crc data glith
	},
	{
		NORTHSTAR_HS_DD,

		0x00,00,				// post index gap4 config

		0x00,00,				// index sync config

		0x00,0x00,0,			// index mark coding
		0x00,0x00,0,

		0x00,33,				// gap1 config

		0x00,0,				// h sync config

		0x00,0,				// d sync config

		0x00,0x00,0,			// address mark coding
		0xFB,0xFF,1,

		0x00,0,				// gap2 config

		0x00,0x00,1,			// data mark coding
		0x00,0xFF,0,

		0x00,00,				// gap3 config

		0x00,00,				// gap4 config

		0xFF,0xFF,0xFF,0xFF,	// Track - Side - Sector - Size

		0x00,0x0000,			// crc, initial value

		0x00,0x00,0,			// post crc header glith
		0x00,0x00,0				// post crc data glith
	},
	{
		HEATHKIT_HS_SD,

		0x00,00,				// post index gap4 config

		0x00,00,				// index sync config

		0x00,0x00,0,			// index mark coding
		0x00,0x00,0,

		0x00,0,				// gap1 config

		0x00,0,				// h sync config

		0x00,0,				// d sync config

		0x00,0x00,0,			// address mark coding
		0xFD,0xFF,1,

		0x00,0,				// gap2 config

		0x00,0x00,1,			// data mark coding
		0xFD,0xFF,0,

		0x00,00,				// gap3 config

		0x00,00,				// gap4 config

		0xFF,0xFF,0xFF,0xFF,	// Track - Side - Sector - Size

		0x00,0x0000,			// crc, initial value

		0x00,0x00,0,			// post crc header glith
		0x00,0x00,0				// post crc data glith
	},
	{
		DECRX02_SDDD,

		0xFF,00,				// post index gap4 config

		0x00,00,				// index sync config

		0x00,0x00,0,			// index mark coding
		0xFC,0xD7,1,

		0xFF,16,				// gap1 config

		0x00,06,				// h sync config

		0x00,06,				// d sync config

		0x00,0x00,0,			// address mark coding
		0xFE,0xC7,1,

		0xFF,11,				// gap2 config

		0x00,0x00,0,			// data mark coding
		0xFD,0xC7,1,

		0xFF,84,				// gap3 config

		0xFF,255,				// gap4 config

		0xFF,0xFF,0xFF,0xFF,	// Track - Side - Sector - Size

		0x1021,0xFFFF,			// crc, initial value

		0x00,0x00,0,			// post crc header glith
		0x00,0x00,0				// post crc data glith
	},
	{
		APPLEMAC_GCR6A2,

		0xFF,00,				// post index gap4 config

		0x00,00,				// index sync config

		0x00,0x00,0,			// index mark coding
		0x00,0x00,0,

		0xFF,20,				// gap1 config

		0x00,12,				// h sync config

		0x00,12,				// d sync config

		0x00,0xFF,4,			// address mark coding
		0xFA,0x00,2,

		0xFF,8,					// gap2 config

		0x00,0xFF,4,			// data mark coding
		0xFA,0x00,2,

		0xFF,84,				// gap3 config

		0xFF,255,				// gap4 config

		0xFF,0xFF,0xFF,0xFF,	// Track - Side - Sector - Size

		0x0000,0x0000,			// crc, initial value

		0x00,0x00,0,			// post crc header glith
		0x00,0x00,0				// post crc data glith
	},
	{
		MICRALN_HS_SD,

		0x00,00,				// post index gap4 config

		0x00,00,				// index sync config

		0x00,0x00,0,			// index mark coding
		0x00,0x00,0,

		0x00,0,				// gap1 config

		0x00,0,				// h sync config

		0x00,0,				// d sync config

		0x00,0x00,0,			// address mark coding
		0xFD,0xFF,1,

		0x00,0,				// gap2 config

		0x00,0x00,1,			// data mark coding
		0xFD,0xFF,0,

		0x00,00,				// gap3 config

		0x00,00,				// gap4 config

		0xFF,0xFF,0xFF,0xFF,	// Track - Side - Sector - Size

		0x00,0x0000,			// crc, initial value

		0x00,0x00,0,			// post crc header glith
		0x00,0x00,0				// post crc data glith
	},
	{
		0,

		0x4E,00,				// post index gap4 config

		0x00,00,				// index sync config

		0x00,0x00,0,			// index mark coding
		0x00,0x00,0,

		0x4E,32,				// gap1 config

		0x00,12,

		0x00,12,

		0xA1,0xFF,3,
		0xFE,0xFF,1,

		0x4E,22,

		0xA1,0xFF,3,
		0xFB,0xFF,1,

		0x4E,84,

		0x4E,255,

		0xFF,0xFF,0xFF,0xFF,	// Track - Side - Sector - Size

		0x1021,0xFFFF,			// crc, initial value

		0x00,0x00,0,			// post crc header glith
		0x00,0x00,0				// post crc data glith

	}
};
