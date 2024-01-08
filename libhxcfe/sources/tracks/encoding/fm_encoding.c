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
///////////////////////////////////////////////////////////////////////////////////
//-------------------------------------------------------------------------------//
//-----------H----H--X----X-----CCCCC-----22222----0000-----0000-----11----------//
//----------H----H----X-X-----C--------------2---0----0---0----0---1-1-----------//
//---------HHHHHH-----X------C----------22222---0----0---0----0-----1------------//
//--------H----H----X--X----C----------2-------0----0---0----0-----1-------------//
//-------H----H---X-----X---CCCCC-----22222----0000-----0000----11111------------//
//-------------------------------------------------------------------------------//
//----------------------------------------------------- http://hxc2001.free.fr --//
///////////////////////////////////////////////////////////////////////////////////
// File : fm_encoding.c
// Contains: FM encoding support
//
// Written by: Jean-François DEL NERO
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include "types.h"

#include "internal_libhxcfe.h"
#include "tracks/track_generator.h"
#include "libhxcfe.h"

#include "tracks/trackutils.h"
#include "fm_encoding.h"

int fmtobin(unsigned char * input_data,int * data_index_buf,int input_data_size,unsigned char * decod_data,int decod_data_size,int bit_offset,int lastbit)
{
	int i;
	int bitshift;
	unsigned char binbyte;

	i = 0;
	bitshift = 0;
	binbyte = 0;

	if( data_index_buf && (i < decod_data_size) )
	{
		data_index_buf[i] = bit_offset;
	}

	do
	{
		//0C0D0C0D

		binbyte = (unsigned char)( binbyte | (getbit(input_data,(bit_offset+3)%input_data_size)<<1) | (getbit(input_data,(bit_offset+7)%input_data_size)<<0));

		bitshift += 2;

		if(bitshift == 8)
		{
			decod_data[i] = binbyte;
			bitshift = 0;
			binbyte = 0;
			i++;
		}
		else
		{
			binbyte = (unsigned char)( binbyte << 2 );
		}

		bit_offset = (bit_offset+8)%input_data_size;

		if(bitshift == 0)
		{
			if( data_index_buf && (i < decod_data_size) )
			{
				data_index_buf[i] = bit_offset;
			}
		}

	}while(i<decod_data_size);

	return bit_offset;
}

int bintofm(unsigned char * track_data,int track_data_size,unsigned char * bin_data,int bin_data_size,int bit_offset)
{
	int i;
	unsigned char b;

	i=0;
	b=0x80;

	bit_offset = bit_offset%track_data_size;

	do
	{
		// Clock
		setbit(track_data,bit_offset,0);
		bit_offset = (bit_offset+1) % track_data_size;
		setbit(track_data,bit_offset,1);
		bit_offset = (bit_offset+1) % track_data_size;

		// Data
		if(bin_data[i] & b)
		{
			setbit(track_data,bit_offset,0);
			bit_offset = (bit_offset+1) % track_data_size;
			setbit(track_data,bit_offset,1);
			bit_offset = (bit_offset+1) % track_data_size;
		}
		else
		{
			setbit(track_data,bit_offset,0);
			bit_offset = (bit_offset+1) % track_data_size;
			setbit(track_data,bit_offset,0);
			bit_offset = (bit_offset+1) % track_data_size;
		}

		b = (unsigned char)( b >> 1 );
		if(!b)
		{
			b=0x80;
			i++;
		}

	}while(i<bin_data_size);

	return bit_offset;
}

void getFMcode(track_generator *tg,uint8_t data,uint8_t clock,uint8_t * dstbuf)
{
	uint32_t * fm_code;
	unsigned char k,i;

	if(tg)
	{
		fm_code=(uint32_t *)dstbuf;

		*fm_code=0;
		for(k=0;k<4;k++)
		{
			*fm_code=*fm_code>>8;

			////////////////////////////////////
			// data
			for(i=0;i<2;i++)
			{
				if(data&(0x80>>(i+(k*2)) ))
				{	// 0x10
					// 00010001)
					*fm_code=*fm_code | ((0x10>>(i*4))<<24);
				}
			}

			// clock
			for(i=0;i<2;i++)
			{
				if(clock&(0x80>>(i+(k*2)) ))
				{	// 0x40
					// 01000100)
					*fm_code=*fm_code | ((0x40>>(i*4))<<24);
				}
			}
		}
	}
	return;
}

// FM encoder
void BuildFMCylinder(uint8_t * buffer,int32_t fmtracksize,uint8_t * bufferclk,uint8_t * track,int32_t size)
{
	int i,j,k,l;
	unsigned char byte,clock;

	// Clean up
	for(i=0;i<(fmtracksize);i++)
	{
		buffer[i] = 0x00;
	}

	j=0;

	// FM Encoding
	j=0;
	for(l=0;l<size;l++)
	{
		byte = track[l];
		clock = bufferclk[l];

		for(k=0;k<4;k++)
		{
			buffer[j] = 0x00;
			////////////////////////////////////
			// data
			for(i=0;i<2;i++)
			{
				if(byte&(0x80>>(i+(k*2)) ))
				{	// 0x10
					// 00010001)
					buffer[j] = (uint8_t)(buffer[j] | (0x10>>(i*4)));
				}
			}

			// clock
			for(i=0;i<2;i++)
			{
				if(clock&(0x80>>(i+(k*2)) ))
				{	// 0x40
					// 01000100)
					buffer[j] = (uint8_t)( buffer[j] | (0x40>>(i*4)));
				}
			}

			j++;
		}
	}
}

// Fast Bin to FM converter
void FastFMgenerator(track_generator *tg,HXCFE_SIDE * side,unsigned char * track_data,int size)
{
	int32_t j,l;
	int32_t i,k;
	unsigned char  byte;
	unsigned char * fm_buffer;

	fm_buffer = &side->databuffer[tg->last_bit_offset/8];

	j=0;
	for(l=0;l<size;l++)
	{
		byte = track_data[l];

		for(k=0;k<4;k++)
		{
			fm_buffer[j] = 0x44;
			////////////////////////////////////
			// data
			for(i=0;i<2;i++)
			{
				if(byte&(0x80>>(i+(k*2)) ))
				{	// 0x10
					// 00010001)
					fm_buffer[j] = (uint8_t)( fm_buffer[j] | (0x10>>(i*4)) );
				}
			}

			j++;
		}
	}

	tg->last_bit_offset=tg->last_bit_offset+(j*8);

	return;
}
