/*
//
// Copyright (C) 2006-2014 Jean-François DEL NERO
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

/*

SpeccyDOS disk images :

File extension .SDD, it is raw store of sectors, side 0 stored first.
Sector size 256 bytes.
Boot sector started with "TRKY2" string for identify. (00h-04h bytes)
0Dh bit 0: =0 MFM, =1 FM (For the MFM 250000 the bitrate, and 125000 for FM.)
0EH bit 6-0: number of tracks, bit 7=0 single sided, =1 double sided
0FH sectors per track

*/

#pragma pack(1)
typedef struct sddfileformats_t_
{
	uint8_t SIGN[4];  // "TRKY2"
	uint8_t PAD[9];
	uint8_t density;
	uint8_t nb_track_side;
	uint8_t nb_sector_per_track;
}sddfileformats_t;
#pragma pack()
