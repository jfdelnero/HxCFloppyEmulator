/*
//
// Copyright (C) 2006 - 2013 Jean-François DEL NERO
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
////////////////////////////////////////////////////////////////////////////////////////////////////

#define CALLINGMETHOD

#define VARIABLEBITRATE					-1
#define VARIABLEENCODING				 1

#define ISOIBM_MFM_ENCODING				0x00
#define AMIGA_MFM_ENCODING				0x01
#define ISOIBM_FM_ENCODING				0x02
#define EMU_FM_ENCODING					0x03
#define TYCOM_FM_ENCODING				0x04
#define MEMBRAIN_MFM_ENCODING			0x05
#define APPLEII_GCR1_ENCODING			0x06
#define APPLEII_GCR2_ENCODING			0x07
#define APPLEII_HDDD_A2_GCR1_ENCODING	0x08
#define APPLEII_HDDD_A2_GCR2_ENCODING	0x09
#define ARBURG_ENCODING					0x0A
#define UNKNOWN_ENCODING				0xFF

typedef struct SIDE_
{
	unsigned int    number_of_sector;		// nombre de secteurs sur la piste (informatif/optionnel) -> -1 si inconnu.
	unsigned char * databuffer;				// buffer data
	long            bitrate;				// si == a VARIABLEBITRATE utiliser timingbuffer
	unsigned long * timingbuffer;			// buffer bitrate de la piste.
	unsigned char * flakybitsbuffer;		// si = 0 pas de flakey/weak bits.
	unsigned char * indexbuffer;			// buffer signal index 1->activé 0->désactivé
	unsigned char * track_encoding_buffer;	// buffer code codage
	
	
	unsigned char   track_encoding;

	unsigned long   tracklen;				// longueur de  databuffer/timingbuffer/flakybitsbuffer/indexbuffer (nombre d'elements)
}SIDE;

typedef struct CYLINDER_
{
	unsigned short  floppyRPM;				// rotation par minute (informatif/optionnel)
	unsigned char   number_of_side;
	SIDE    **      sides;
}CYLINDER;

typedef struct FLOPPY_
{
	unsigned int    floppyBitRate;

	unsigned char   floppyNumberOfSide;
	unsigned short  floppyNumberOfTrack;
	unsigned short  floppySectorPerTrack;

	unsigned short  floppyiftype;
	unsigned char   double_step;

	CYLINDER    **  tracks;
}FLOPPY;

