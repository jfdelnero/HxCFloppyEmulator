/*
//
// Copyright (C) 2006-2026 Jean-François DEL NERO
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
 uint32_t  track_block_size;
 uint32_t  flakey_mask_size;
 uint16_t  numberofsector;
 uint16_t  flags;
 uint16_t  track_size;
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

// File descriptor
#define PASTI_FILE_VERSION         3      // codeversion1
#define PASTI_FILE_REVISION_OLD    0x00   // unknowvalue (revision) : old format (Macrodos/Speedlock fixed timing table)
#define PASTI_FILE_REVISION_NEW    0x02   // unknowvalue (revision) : new format (per sector timing records)

// Track descriptor flags
#define PASTI_TRK_SYNC             0x0080 // Track image header contains the first sync offset
#define PASTI_TRK_IMAGE            0x0040 // Track record contains a track image (WD1772 "Read track" dump)
#define PASTI_TRK_PROT             0x0020 // Track contains protections (always set with PASTI_TRK_SECT)
#define PASTI_TRK_SECT             0x0001 // Track record contains sector descriptors

// Sector descriptor FDC status / flags
#define PASTI_SECT_FUZZY           0x80   // Sector contains fuzzy bits (fuzzy mask present)
#define PASTI_SECT_RECORD_TYPE     0x20   // Deleted data mark
#define PASTI_SECT_RNF             0x10   // Record not found (no data block)
#define PASTI_SECT_CRC             0x08   // CRC error (data if RNF=0, address field if RNF=1)
#define PASTI_SECT_VARIABLE_TIME   0x01   // Intra sector bit width variation (timing record with revision 2)

// Timing record (revision 2) : header followed by one big endian 16 bits value per 16 bytes block.
// Each value is the 16 bytes block read time in units of 32 FDC cycles @ 8MHz (+28 cycles per block).
// The nominal value is 127 (~512us).
#define PASTI_TIMING_FLAGS         0x0005
#define PASTI_TIMING_BLOCK_SIZE    16
#define PASTI_TIMING_NOMINAL       127

#pragma pack(1)

typedef struct pasti_timingheader_
{
 uint16_t  flags;      // Always 5 ?
 uint16_t  size;       // Total size of the timing record (header included)
}pasti_timingheader;

#pragma pack()
