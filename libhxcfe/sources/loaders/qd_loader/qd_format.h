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
//-------------------------------------------------------------------------------//
//-----------H----H--X----X-----CCCCC----22222----0000-----0000------11----------//
//----------H----H----X-X-----C--------------2---0----0---0----0--1--1-----------//
//---------HHHHHH-----X------C----------22222---0----0---0----0-----1------------//
//--------H----H----X--X----C----------2-------0----0---0----0-----1-------------//
//-------H----H---X-----X---CCCCC-----222222----0000-----0000----1111------------//
//-------------------------------------------------------------------------------//
//----------------------------------------------------- http://hxc2001.free.fr --//
///////////////////////////////////////////////////////////////////////////////////
// File : qd_format.h
// Contains: HxC Quickdisk floppy image format /
//           (c) HxC2001 / Jean-François DEL NERO
//
// Written by: Jean-François DEL NERO
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

// All structure datas are packed !
#pragma pack(1)

typedef struct qdhfefileformatheader_
{
	unsigned char HEADERSIGNATURE[8]; // "HXCQDDRV"
	uint32_t formatrevision;          // 0
	uint32_t number_of_track;         // Should be 1, but we can store multiple disks in one file
	uint32_t number_of_side;          // 1 or 2
	uint32_t track_encoding;          //
	uint32_t write_protected;
	uint32_t bitRate;                 // (cells rate / s)
	uint32_t flags;
	uint32_t track_list_offset;       // In bytes
}qdhfefileformatheader;

// The qdtrack array can be found after the file header.
// The array file position is 512 bytes aligned.
// Number of elements in the qdtrack array : number_of_track * number_of_side
// Here is one qdtrack element :

typedef struct qdtrack_
{
	uint32_t offset;                  // Absolute file offset
	uint32_t track_len;               // In bytes.
	uint32_t start_sw_position;       // start switch track offset, in bytes
	uint32_t stop_sw_position;        // end switch track offset, in bytes
}qdtrack;

// Tracks cell datas can be found after the track description array.
// Each data byte must be sent to the QD floppy interface from the bit 0 to the bit 7.

#pragma pack()
