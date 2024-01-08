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

#define DEFAULT_BITS_PERIOD 40000          // ps -> 25Mhz

#define STREAMHFE_HDRFLAG_SINGLE_SIDE 0x00000001
#define STREAMHFE_HDRFLAG_DOUBLE_STEP 0x00000001

#define STREAMHFE_TRKFLAG_PACKED      0x00000001

typedef struct streamhfe_fileheader_
{
	uint8_t   signature[16];               // "HxC_Stream_Image"
	uint32_t  formatrevision;
	uint32_t  flags;
	uint32_t  index_array_offset;
	uint32_t  track_list_offset;
	uint32_t  track_data_offset;
	uint32_t  number_of_track;
	uint32_t  number_of_side;
	uint32_t  bits_period;                 // ps seconds
	uint32_t  floppyinterfacemode;
	uint32_t  RFU[4];                      // 0x00000000 by default
	uint8_t   Internal_Name[128];          // Null terminated string.
	uint8_t   Internal_Date[16];           // Null terminated string.
}streamhfe_fileheader;

typedef struct streamhfe_index_element_
{
	uint32_t flags;
	uint32_t start_track_position;
	uint32_t length;
	uint32_t pad_rfu;
}streamhfe_index_element;

typedef struct streamhfe_track_def_
{
	uint32_t flags;
	uint32_t packed_data_offset;          // In bytes - From the file begining
	uint32_t packed_data_size;            // In bytes
	uint32_t unpacked_data_size;          // In bytes
	uint32_t track_len;                   // In bits  - Keep it 32bits aligned (Total track time = track_len * bits_period)
	uint32_t nb_pulses;
	uint32_t pad_rfu1;
	uint32_t pad_rfu2;
}streamhfe_track_def;

#pragma pack()
