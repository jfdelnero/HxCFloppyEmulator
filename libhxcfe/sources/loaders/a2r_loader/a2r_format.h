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

//
// A2R file format documentation used :
//
// A2R 3.x : https://applesaucefdc.com/a2r/
// A2R 2.x : https://applesaucefdc.com/a2r2-reference/
//
// All data is stored little-endian.
//

#pragma pack(1)

typedef struct a2r_header_
{
	uint8_t  sign[4];               // "A2R2" / "A2R3"
	uint8_t  ff_byte;               // 0xFF
	uint8_t  lfcrlf[3];             // 0A 0D 0A / LF CR LF
}a2r_header;

typedef struct a2r_chunk_header_
{
	uint8_t  sign[4];               // "INFO" / "STRM" / "RWCP" / "SLVD" / "META"
	uint32_t  chunk_size;
}a2r_chunk_header;

// INFO chunk (2.x : 36 bytes, 3.x : 37 bytes)
typedef struct a2r_info_
{
	uint8_t  version;               // 1
	char     creator[32];           // UTF-8, padded with spaces
	uint8_t  disk_type;             // 2.x : 1 = 5.25, 2 = 3.5 / 3.x : drive type (A2R_DRIVE_xxx)
	uint8_t  write_protected;       // 1 = Floppy is write protected
	uint8_t  synchronized;          // 1 = Cross track sync/index was used during imaging
	uint8_t  hard_sector_count;     // 3.x only : 0 = soft sectored, 1+ = number of hard sectors
}a2r_info;

// 2.x STRM chunk capture header
typedef struct a2r_capture_
{
	uint8_t  location;
	uint8_t  capture_type;
	uint32_t data_length;
	uint32_t estimated_loop_point;
}a2r_capture;

#pragma pack()

#define A2R_HEADER_SIZE             8
#define A2R_CHUNK_HEADER_SIZE       8

#define A2R2_INFO_SIZE              36
#define A2R3_INFO_SIZE              37

#define A2R2_STRM_END               0xFF   // Location value ending the STRM captures
#define A2R2_CAPTURE_HEADER_SIZE    10
#define A2R2_TICK_PERIOD_PS         125000 // 125 ns

#define A2R3_CHUNK_SUBHEADER_SIZE   16     // RWCP/SLVD : version, resolution, 11 reserved bytes
#define A2R3_DEFAULT_TICK_PERIOD_PS 62500  // 62.5 ns

#define A2R3_RWCP_VERSION           1
#define A2R3_SLVD_VERSION           2
#define A2R3_INFO_VERSION           1

#define A2R3_MARK_CAPTURE           'C'
#define A2R3_MARK_TRACK             'T'
#define A2R3_MARK_END               'X'

#define A2R3_SLVD_TRACK_HEADER_SIZE 12     // Mark, location, mirror out, mirror in, 6 reserved, index count

// Capture types
#define A2R_CAPTURE_TIMING          1
#define A2R_CAPTURE_BITS            2
#define A2R_CAPTURE_XTIMING         3

#define A2R_BITS_CAPTURE_CELL_PS    4000000 // "bits" capture : bits normalized to 4 us

// 2.x disk types
#define A2R2_DISK_525               1
#define A2R2_DISK_35                2

// 3.x drive types
#define A2R_DRIVE_525_SS_40_QUARTER 1      // Location in quarter tracks
#define A2R_DRIVE_35_DS_80_APPLE    2      // Apple CLV
#define A2R_DRIVE_525_DS_80         3
#define A2R_DRIVE_525_DS_40         4
#define A2R_DRIVE_35_DS_80          5
#define A2R_DRIVE_8_DS              6
#define A2R_DRIVE_3_DS_80           7
#define A2R_DRIVE_3_DS_40           8
