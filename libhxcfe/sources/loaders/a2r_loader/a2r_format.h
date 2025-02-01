/*
//
// Copyright (C) 2006-2021 Jean-François DEL NERO
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

typedef struct a2r_header_
{
	uint8_t  sign[4];               // "A2R2"
	uint8_t  ff_byte;               // 0xFF
	uint8_t  lfcrlf[3];             // 0A 0D 0A / LF CR LF
}a2r_header;

typedef struct a2r_chunk_header_
{
	uint8_t  sign[4];               // "INFO" / "STRM" / "META"
	uint32_t  chunk_size;
}a2r_chunk_header;

typedef struct a2r_info_
{
	uint8_t  version;               // 1
	char     creator[32];
	uint8_t  disk_type;             // 1 = 5.25, 2 = 3.5
	uint8_t  write_protected;       // 1 = Floppy is write protected
	uint8_t  synchronized;          // 1 = Cross track sync was used during imaging
}a2r_info;

typedef struct a2r_capture_
{
	uint8_t  location;
	uint8_t  capture_type;
	uint32_t data_length;
	uint32_t estimated_loop_point;
}a2r_capture;

#pragma pack()
