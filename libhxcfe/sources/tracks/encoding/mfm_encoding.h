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

int mfmtobin(unsigned char * input_data,int * data_index_buf,int input_data_size,unsigned char * decod_data,int decod_data_size,int bit_offset,int lastbit);
int bintomfm(unsigned char * track_data,int track_data_size,unsigned char * bin_data,int bin_data_size,int bit_offset);

void getMFMcode(track_generator *tg,uint8_t data,uint8_t clock,uint8_t * dstbuf);

int32_t BuildMFMCylinder(uint8_t * mfm_buffer,int32_t mfm_size,uint8_t * track_clk,uint8_t * track_data,int32_t track_size);

void FastMFMgenerator(track_generator *tg,HXCFE_SIDE * side,unsigned char * track_data,int size);
void FastAmigaMFMgenerator(track_generator *tg,HXCFE_SIDE * side,unsigned char * track_data,int size);