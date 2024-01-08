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

//
// 86F file format documentation used :
//
// https://86box.readthedocs.io/en/latest/dev/formats/86f.html
//
//   File Header information :

#pragma pack(1)

typedef struct f86header_t_
{
	uint8_t  f86_signature[4];               // Extension("86BF")
	uint8_t  minor_revision;                 // Minor revision
	uint8_t  major_revision;                 // Major revision

/*
        Disk flags :

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
        Bit 5           Bitcell mode (1 = Extra bitcells count specified after
                        disk flags, 0 = No extra bitcells)
                        The maximum number of extra bitcells is 1024 (which
                        after decoding translates to 64 bytes)
        Bit 6           Revolutions (0 = one revolution, 1 track has 16-bit number of revolutions)
*/

    uint16_t disk_flags;

    // Tracks offsets
    /* Note that thick-track (eg. 360k) disks will have (tracks * 2) tracks, with each pair of tracks
        being identical to each other.
        Each side of each track is stored as its own track, in order (so, track 0 side 0, track 0 side 1,
        track 1 side 0, track 1 side 0, etc.).
        The table of the offsets of tracks is 2048 bytes long, each track offset is an unigned 32-bit
        integer. An offset of 00000000 indicates the track is not present in the file.
        As an example, an 86F representing a disk with 80 thin tracks and 2 sides per track, where all
        the tracks are present in the file, would have the first 160 offsets filled in, same for a disk
        with 40 thick tracks and 2 sides. Same with only 1 side but only the offsets at 0000000, 0000008,
        etc. (so every second offset) would be filled in.
    */


} f86header_t;

#define DISK_FLAG_GET_DISK_TYPE(flag) ((flag>>1)&0x3)

#define DISK_FLAG_HAS_DESCRIPTION_DATA 0x0001
#define DISK_FLAG_DOUBLE_SIDED         0x0008
#define DISK_FLAG_WRITE_PROTECTED      0x0010
#define DISK_FLAG_BITCELL_MODE         0x0020
#define DISK_FLAG_MULTIPLE_REVOLUTIONS 0x0040

//   Track Header information :

typedef struct f86track_t_
{

/*
        Track flags (16-bit)
        Bits 4, 3       Encoding
                        00 = FM
                        01 = MFM
                        10 = M2FM
                        11 = GCR
        Bits 2, 1, 0    Bit rate, if encoding is MFM:
                        000 = 500 kbps
                        001 = 300 kbps
                        010 = 250 kbps
                        011 = 1000 kbps
                        101 = 2000 kbps
                        If encoding is FM, the bit rate is half that.
        The RPM is determined from track length and data rate.
*/

	uint16_t  track_flags;

	uint32_t  number_of_bit_cells;

	uint32_t  index_position; // Bit cell where index hole is (32-bit)
} f86track_t;

#pragma pack()
