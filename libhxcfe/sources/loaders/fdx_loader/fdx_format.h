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
// FDX68 file format documentation translated from this page :
//
// http://retropc.net/gimons/fdx68/diskimage.html
//

//   This is the specification of the dedicated disk image format (FDX format).
//
//   With revision 3, the index and track size have been expanded to the bit level.
//
//   Image structure :
//
//        -> Header information (256 bytes of disk attributes)
//        -> Track information (16 bytes of track attributes + track data) * number of tracks
//
//   Numbers are stored in little endian
//
//   File Header information :

#pragma pack(1)

typedef struct fdxheader_t_
{
	uint8_t  fdx_signature[3];               // 0x000 - Extension('F','D','X')
	uint8_t  revision;                       // 0x003 - Revision (should be 3)
	uint8_t  disk_name[60];                  // 0x004 - Disk name
	uint8_t  pad[4];                         // 0x040 - Padding
	int32_t  disk_type;                      // 0x044 - Type (0:2D 1:2DD 2:2HD 9:RAW)
	int32_t  nb_of_cylinders;                // 0x048 - Number of cylinders
	int32_t  nb_of_heads;                    // 0x04C - Number of heads
	int32_t  default_bitrate;                // 0x050 - Transfer rate (with clock) [500|1000]
	int32_t  disk_rpm;                       // 0x054 - RPM
	uint32_t write_protect;                  // 0x058 - Write protect (0: OFF 1: ON)
	uint32_t option;                         // 0x05C - Behavior options (currently unused)
	uint32_t unused;                         // 0x060 - Not used
	int32_t  track_block_size;               // 0x064 - Track block lengh (number of bytes)
	uint8_t  reserved[152];                  // 0x068 - Reserved (padding to 256 bytes header)
} fdxheader_t;

//   Track Header information :
//
//   Fixed attribute information and variable length track data.
//   The format of the track data is MFM/FM level encoded data or sampling level RAW data (disk_type = 9).
//
//   Tracks order :
//   [C0H0][C0H1][C1H0][C1H1]...and continue in order of cylinder and head.
//   The track data block length is track_block_size in the header information, and the number of track data blocks is cylinders * heads.

typedef struct fdxtrack_t_
{
	int32_t  cylinder;                       // Cylinder
	int32_t  head;                           // Head
	int32_t  index_bit_place;                // Index hole place (bit number)
	int32_t  bit_track_length;               // Track data length (bit number)
	// Track data : header track_block_size - 16
} fdxtrack_t;

//
// Additional informations:
//
// Weakbits/flakybits area seems to be set to 0 in the track data buffer.
//

#define FDX_RAW_TICK_PERIOD 250000 // 250 ns per tick (4MHz) (to be confirmed !)

#pragma pack()
