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
#pragma pack(1)

typedef struct pasti_fileheader_
{
 uint8_t   headertag[4]; // "RSY\0"
 uint16_t  codeversion1;
 uint16_t  codeversion2;
 uint16_t  unused1;
 uint8_t   number_of_track; //valide si <= 0xA8
 uint8_t   unknowvalue;
 uint32_t  unused2;
}pasti_fileheader;


typedef struct pasti_trackheader_
{
 uint32_t  tracksize;
 uint32_t  unused1;
 uint16_t  numberofsector;
 uint16_t  flags;
 uint16_t  Tvalue;
 uint8_t   track_code;
 uint8_t   unused2;

}pasti_trackheader;

typedef struct pasti_sector_
{
 uint32_t  sector_pos;
 uint16_t  sector_pos_timing;
 uint16_t  sector_speed_timing;
 uint8_t   track_num;
 uint8_t   side_num;
 uint8_t   sector_num;
 uint8_t   sector_size;
 uint16_t  header_crc;
 uint8_t   FDC_status;
 uint8_t   sector_flags; // (always 00)
}pasti_sector;

#pragma pack()
