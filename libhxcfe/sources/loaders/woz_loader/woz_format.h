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
#pragma pack(1)

// Chunk definitions

// v1
#define CHUNK_INFO 0x4F464E49
#define CHUNK_TMAP 0x50414D54
#define CHUNK_TRKS 0x534B5254
#define CHUNK_META 0x4154454D
// v2
#define CHUNK_WRIT 0x54495257

typedef struct woz_fileheader_
{
	uint8_t   headertag[3];         // "WOZ"
	uint8_t   version;              // '1' or '2'
	uint8_t   pad;                  // 0xFF
	uint8_t   lfcrlf[3];            // 0x0A 0x0D 0x0A
	uint32_t  crc32;                // CRC32 of the remaining file content
}woz_fileheader;

typedef struct woz_chunk_
{
	uint32_t   id;                  // ID
	uint32_t   size;                // Chunk data size
	uint8_t    data[];              // Data
}woz_chunk;

typedef struct woz_info_
{
	// Rev 1,2 & 3
	uint8_t   version;              // 1,2 or 3
	uint8_t   disk_type;            // 1 = 5.25, 2 = 3.5
	uint8_t   write_protected;
	uint8_t   sync;
	uint8_t   cleaned;
	uint8_t   creator[32];

	// Rev 2 & 3
	uint8_t   sides_count;
	uint8_t   boot_sector_format;   // (1 = Contains boot sector for 16-sector, 2 = Contains boot sector for 13-sector, 3 = Contains boot sectors for both)
	uint8_t   bit_timing;           // 125 increments. example : 8= 125*8 = 1uS
	uint16_t  compatible_hw;        // 0x0001 = Apple ][, 0x0002 = Apple ][ Plus, 0x0004 = Apple //e (unenhanced), 0x0008 = Apple //c, 0x0010 = Apple //e Enhanced
									// 0x0020 = Apple IIgs, 0x0040 = Apple //c Plus, 0x0080 = Apple ///, 0x0100 = Apple /// Plus
	uint16_t  required_ram;         // In kB
	uint16_t  largest_track;        // In 512 blocks number.

	// Rev 3
	uint16_t flux_block;
	uint16_t largest_flux_track;

}woz_info;

typedef struct woz_trk_
{
	uint16_t starting_block;        // 1 Block = 512 bytes. From the beginning of the file.
	uint16_t block_count;           // Area size in blocks.
	uint32_t bit_count;             // Number of bits to use.
}woz_trk;

typedef struct woz_trk_v1_
{
	uint8_t  bitstream[6646];
	uint16_t bytes_count;           // 1 Block = 512 bytes. From the beginning of the file.
	uint16_t bit_count;             // Number of bits to use.
	uint16_t bit_splice_point;      // Index of first bit after track splice (write hint). If no splice information -> 0xFFFF.
	uint8_t  splice_nibble;	        // Nibble value to use for splice (write hint).
	uint8_t  splice_bit_count;      // Bit count of splice nibble (write hint).
	uint16_t reserved;              // RFU
}woz_trk_v1;

#pragma pack()
