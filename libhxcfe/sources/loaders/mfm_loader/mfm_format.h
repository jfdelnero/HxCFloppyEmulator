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

#define byte  unsigned char
#define word  unsigned short
#define dword unsigned int

// All structure datas are packed !
#pragma pack(1)

typedef struct MFMIMG_
{
	byte headername[7];          // "HXCMFM\0"

	word number_of_track;
	byte number_of_side;         // Number of elements in the MFMTRACKIMG array : number_of_track * number_of_side

	word floppyRPM;              // Rotation per minute.
	word floppyBitRate;          // 250 = 250Kbits/s, 300 = 300Kbits/s...
	byte floppyiftype;

	dword mfmtracklistoffset;    // Offset of the MFMTRACKIMG array from the beginning of the file in number of bytes.
}MFMIMG;

// Right after this header, the MFMTRACKIMG array is present. 
// Number of element in the MFMTRACKIMG array : number_of_track * number_of_side
// Here is one MFMTRACKIMG element :

typedef struct MFMTRACKIMG_
{
	word track_number;
	byte side_number;
	dword mfmtracksize;          // MFM/FM Track size in bytes
	dword mfmtrackoffset;        // Offset of the track data from the beginning of the file in number of bytes.
}MFMTRACKIMG;

// After this array, track datas can be found.
// Each data byte must be sent to the floppy interface from the bit 7 to the bit 0.

#pragma pack()
