/*
//
// Copyright (C) 2006-2016 Jean-François DEL NERO
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

typedef struct stt_header_
{
	uint32_t	stt_signature;//STEM
	uint16_t	stt_version;
	uint16_t	stt_flags;
	uint16_t	tracks_flags;
	uint16_t	number_of_tracks;
	uint16_t	number_of_sides;	
}stt_header;

typedef struct stt_track_offset_
{
	uint32_t	track_offset;
	uint16_t	track_size;
}stt_track_offset;


typedef struct stt_track_header_
{
	uint32_t	stt_track_signature; //TRCK
	uint16_t	tracks_flags;
	uint16_t	section_size;
	uint16_t	sectors_flags;
	uint16_t	number_of_sectors;
}stt_track_header;


typedef struct stt_sector_
{
	uint8_t	    track_nb_id;
	uint8_t	    side_nb_id;
	uint8_t	    sector_nb_id;
	uint8_t	    sector_len_code;
	uint8_t	    crc_byte_1;
	uint8_t	    crc_byte_2;
	uint16_t	data_offset;
	uint16_t	data_len;
}stt_sector;

#pragma pack()
