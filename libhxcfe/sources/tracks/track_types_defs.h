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

typedef struct isoibm_config_
{
	// format index
	int32_t   indexformat;

	// post index gap4 config
	uint8_t   data_gap4a;
	int32_t   len_gap4a;

	// index sync config
	uint8_t   data_isync;
	int32_t   len_isync;

	// index mark coding
	uint8_t   data_indexmarkp1;
	uint8_t   clock_indexmarkp1;
	int32_t   len_indexmarkp1;

	uint8_t   data_indexmarkp2;
	uint8_t   clock_indexmarkp2;
	int32_t   len_indexmarkp2;

	// gap1 config

	uint8_t   data_gap1;
	int32_t   len_gap1;

	// h sync config
	uint8_t   data_ssync;
	int32_t   len_ssync;

	// d sync config
	uint8_t   data_dsync;
	int32_t   len_dsync;

	// address mark coding
	uint8_t   data_addrmarkp1;
	uint8_t   clock_addrmarkp1;
	int32_t   len_addrmarkp1;

	uint8_t   data_addrmarkp2;
	uint8_t   clock_addrmarkp2;
	int32_t   len_addrmarkp2;

	// gap2 config
	uint8_t   data_gap2;
	int32_t   len_gap2;

	// data mark coding
	uint8_t   data_datamarkp1;
	uint8_t   clock_datamarkp1;
	int32_t   len_datamarkp1;

	uint8_t   data_datamarkp2;
	uint8_t   clock_datamarkp2;
	int32_t   len_datamarkp2;

	// gap3 config
	uint8_t   data_gap3;
	int32_t   len_gap3;

	uint8_t   data_gap4b;
	int32_t   len_gap4b;

	uint8_t   track_id;
	uint8_t   side_id;
	uint8_t   sector_id;
	uint8_t   sector_size_id;

	uint16_t  crc_poly,crc_initial;

	uint8_t   posthcrc_glitch_data;
	uint8_t   posthcrc_glitch_clock;
	int32_t   posthcrc_len;

	uint8_t   postdcrc_glitch_data;
	uint8_t   postdcrc_glitch_clock;
	int32_t   postdcrc_len;

}isoibm_config;

extern isoibm_config formatstab[];
