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

//
// 86F file format documentation used :
//
// https://86box.readthedocs.io/en/latest/dev/formats/86f.html (v2.12)
// + 86Box reference implementation (src/floppy/fdd_86f.c)
//
//  00000000: Magic 4 bytes ("86BF")
//  00000004: Minor version (0C)
//  00000005: Major version (02)
//  00000006: Disk flags (16-bit)
//  00000008: Tracks offsets table (32-bit offsets, 0 = track not present)
//            Double sided : 512 entries (2048 bytes), entry = (track << 1) + side
//            Single sided : 256 entries (1024 bytes), entry = track
//            Tracks are in thin tracks geometry : thick tracks (40 tracks, 48 tpi)
//            disks have each track stored twice.
//
//  Track offset + 00000000: Track flags (16-bit)
//  If the bitcell count is present (disk flags bit 7) :
//      Track offset + 00000002: Extra (or total) bit cells count (32-bit)
//      Track offset + 00000006: Bit cell where index hole is (32-bit)
//      Track offset + 0000000A: FM/MFM/M2FM/GCR-encoded data (track length bytes)
//      followed by the surface description data if present (track length bytes)
//  Else:
//      Track offset + 00000002: Bit cell where index hole is (32-bit)
//      Track offset + 00000006: FM/MFM/M2FM/GCR-encoded data (track length bytes)
//      followed by the surface description data if present (track length bytes)
//
//  Bit cells are stored MSB first in big endian 16-bit words (so MSB first
//  byte per byte), unless the disk flags bit 11 is set (little endian words).
//

#pragma pack(1)

typedef struct f86header_t_
{
	uint8_t  f86_signature[4];               // "86BF"
	uint8_t  minor_revision;                 // Minor revision
	uint8_t  major_revision;                 // Major revision
	uint16_t disk_flags;                     // Disk flags
} f86header_t;

#pragma pack()

#define F86_FORMAT_MAJOR_VERSION      0x02
#define F86_FORMAT_MINOR_VERSION      0x0C

#define F86_HEADER_SIZE               8
#define F86_TRACK_TABLE_ENTRIES       512     // Double sided disk. (Single sided disk : 256)
#define F86_MAX_TRACK_WORDS           53048   // 86Box per track buffer size (16-bit words)
#define F86_MAX_BITCELLS              ( F86_MAX_TRACK_WORDS * 16 )
#define F86_MIN_FILE_SIZE             51052   // 86Box refuses smaller files.

/*
	Disk flags (16-bit) :

	Bit 0           Has surface description data (1 = yes, 0 = no)
	                This data indicates if the corresponding bit on the FM/MFM encoded surface
	                is a normal bit or a special bit (weak bit or hole, depending on the other bit):
	                    0 = The corresponding FM/MFM encoded surface bit is normal
	                    1 = The corresponding FM/MFM encoded surface bit is either a weak bit or a hole:
	                        Corresponding FM/MFM encoded bit is 0: Hole (noise on read, not overwritable)
	                        Corresponding FM/MFM encoded bit is 1: Weak bit (noise on read, overwritable)
	Bits 2, 1       Hole (3 = ED + 2000 kbps, 2 = ED, 1 = HD, 0 = DD)
	Bit 3           Sides (1 = 2 sides, 0 = 1 side)
	Bit 4           Write protect (1 = yes, 0 = no)
	Bits 6, 5       RPM slowdown (3 = 2%, 2 = 1.5%, 1 = 1%, 0 = 0%)
	Bit 7           Bitcell mode (1 = Extra bitcells count specified after the track flags)
	Bit 8           Disk type (1 = Zoned, 0 = Fixed RPM)
	Bits 10, 9      Zone type
	Bit 11          Data and surface bits are stored in reverse byte endianness
	Bit 12          If bits 6, 5 are not 0, they specify % of speedup instead of slowdown;
	                If bits 6, 5 are 0, and bit 7 is 1, the extra bitcell count specifies
	                the entire bitcell count.
*/

#define F86_DISKFLAG_SURFACE_DATA     0x0001
#define F86_DISKFLAG_HOLE_SHIFT       1
#define F86_DISKFLAG_HOLE_MASK        0x0006
#define F86_DISKFLAG_DOUBLE_SIDED     0x0008
#define F86_DISKFLAG_WRITE_PROTECT    0x0010
#define F86_DISKFLAG_RPM_SHIFT        5
#define F86_DISKFLAG_RPM_MASK         0x0060
#define F86_DISKFLAG_BITCELL_COUNT    0x0080
#define F86_DISKFLAG_ZONED            0x0100
#define F86_DISKFLAG_ZONE_TYPE_MASK   0x0600
#define F86_DISKFLAG_REVERSE_ENDIAN   0x0800
#define F86_DISKFLAG_TOTAL_BITCELLS   0x1000  // (aka speedup when bits 6,5 != 0)

#define F86_DISKFLAG_GET_HOLE(flag)   (((flag) & F86_DISKFLAG_HOLE_MASK) >> F86_DISKFLAG_HOLE_SHIFT)
#define F86_DISKFLAG_GET_RPM(flag)    (((flag) & F86_DISKFLAG_RPM_MASK) >> F86_DISKFLAG_RPM_SHIFT)

/*
	Track flags (16-bit) :

	Bits 7, 6, 5    RPM : 000 = 300 rpm, 001 = 360 rpm
	Bits 4, 3       Encoding : 00 = FM, 01 = MFM, 10 = M2FM, 11 = GCR
	Bits 2, 1, 0    Bit rate, if encoding is MFM:
	                    000 = 500 kbps
	                    001 = 300 kbps
	                    010 = 250 kbps
	                    011 = 1000 kbps
	                    101 = 2000 kbps
	                If encoding is FM, the bit rate is half that.
*/

#define F86_TRKFLAG_RPM_MASK          0x00E0
#define F86_TRKFLAG_RPM_360           0x0020
#define F86_TRKFLAG_ENC_SHIFT         3
#define F86_TRKFLAG_ENC_MASK          0x0018
#define F86_TRKFLAG_RATE_MASK         0x0007

#define F86_TRKFLAG_GET_ENC(flag)     (((flag) & F86_TRKFLAG_ENC_MASK) >> F86_TRKFLAG_ENC_SHIFT)

#define F86_TRK_ENC_FM                0
#define F86_TRK_ENC_MFM               1
#define F86_TRK_ENC_M2FM              2
#define F86_TRK_ENC_GCR               3

// Thick tracks handling modes (F86EXPORT_THICK_TRACKS / F86LOADER_THICK_TRACKS)
#define F86_THICK_TRACKS_AUTO         0
#define F86_THICK_TRACKS_FORCE        1
#define F86_THICK_TRACKS_NEVER        2
