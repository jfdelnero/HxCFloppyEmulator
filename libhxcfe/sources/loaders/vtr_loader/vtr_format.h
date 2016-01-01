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

typedef struct vtrucco_picfileformatheader_
{
	uint8_t  HEADERSIGNATURE[7];
	uint8_t  write_protected;
	uint16_t floppyRPM;

	uint8_t  number_of_track;
	uint8_t  floppyinterfacemode;
	uint8_t  number_of_side;

	uint8_t  track_encoding;
	uint16_t bitRate;
	uint16_t track_list_offset;

	uint8_t  formatrevision;

	uint8_t  RESERVED[100];

	uint8_t  input_filename[128];

	uint8_t  RESERVED2[50];

	uint8_t  CREDITS[73];

}vtrucco_picfileformatheader;


typedef struct vtrucco_pictrack_
{
	uint16_t offset;
	uint16_t track_len;
}vtrucco_pictrack;

#pragma pack()
