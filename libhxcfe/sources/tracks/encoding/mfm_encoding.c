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
// File : mfm_encoding.c
// Contains: MFM encoding support
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
#include "mfm_encoding.h"

#include "tracks/luts.h"

// -----------------------------------------------------------------------------
// MFM      : Reversal at each '1' or between 2 '0' (at the clock place).
// Data     : 0 c 0 c 1 c 1 c 1 c 0 c 1 c 1 c 1 c 1 c 0 c 0 c 0
//               _____     ___         ___     ___       ___
// Reversal : __|     |___|   |_______|   |___|   |_____|   |___
// Cells      0 1 0 0 1 0 1 0 1 0 0 0 1 0 1 0 1 0 1 0 0 1 0 1 0
// Decoding :  | 0 | 1 | 1 | 1 | 0 | 1 | 1 | 1 | 1 | 0 | 0 | 0 |
// -----------------------------------------------------------------------------

int mfmtobin(unsigned char * input_data,int * data_index_buf,int input_data_size,unsigned char * decod_data,int decod_data_size,int bit_offset,int lastbit)
{
	int i,j;
	unsigned char b,c1,c2;
	int first_loop_data_size;

	i = 0;
	b = 0x80;

	bit_offset %= input_data_size;
	j = bit_offset >> 3;

	first_loop_data_size =  (input_data_size - bit_offset) / 16;

	if(first_loop_data_size > decod_data_size)
		first_loop_data_size = decod_data_size;

	if( data_index_buf && (i < decod_data_size) )
	{
		data_index_buf[i] = bit_offset;
	}

	// First loop without modulo.
	while(i<first_loop_data_size)
	{
		c1 = (unsigned char)( input_data[j] & (0x80>>(bit_offset&7)) );
		bit_offset++;
		j = bit_offset>>3;

		c2 = (unsigned char)( input_data[j] & (0x80>>(bit_offset&7)) );
		bit_offset++;
		j = bit_offset>>3;

		if( !c1 && c2 )
			decod_data[i] = (unsigned char)( decod_data[i] | b );
		else
			decod_data[i] = (unsigned char)( decod_data[i] & ~b );

		b = (unsigned char)( b>>1 );
		if(!b)
		{
			b=0x80;
			i++;

			if( data_index_buf && (i < decod_data_size) )
			{
				data_index_buf[i] = bit_offset;
			}
		}
	}

	bit_offset%= input_data_size;
	j = bit_offset>>3;

	while(i<decod_data_size)
	{
		c1 = (unsigned char)( input_data[j] & (0x80>>(bit_offset&7)) );
		bit_offset = (bit_offset+1)%input_data_size;
		j = bit_offset>>3;

		c2 = (unsigned char)( input_data[j] & (0x80>>(bit_offset&7)) );
		bit_offset = (bit_offset+1)%input_data_size;
		j = bit_offset>>3;

		if( !c1 && c2 )
			decod_data[i] = (unsigned char)( decod_data[i] | b );
		else
			decod_data[i] = (unsigned char)( decod_data[i] & ~b );

		b = (unsigned char)( b>>1 );
		if(!b)
		{
			b=0x80;
			i++;

			if( data_index_buf && (i < decod_data_size) )
			{
				data_index_buf[i] = bit_offset;
			}
		}
	}

	return bit_offset;
}

int bintomfm(unsigned char * track_data,int track_data_size,unsigned char * bin_data,int bin_data_size,int bit_offset)
{
	int i,lastbit;
	unsigned char b;

	i = 0;
	b = 0x80;

	bit_offset = bit_offset%track_data_size;

	lastbit = 0;
	if(bit_offset)
	{
		if(getbit(track_data,bit_offset-1) )
			lastbit = 1;
	}
	else
	{
		if(getbit(track_data,track_data_size-1) )
			lastbit = 1;
	}

	do
	{
		if(bin_data[i] & b)
		{
			setbit(track_data,bit_offset,0);
			bit_offset = (bit_offset+1)%track_data_size;
			setbit(track_data,bit_offset,1);
			bit_offset = (bit_offset+1)%track_data_size;
			lastbit = 1;
		}
		else
		{
			if(lastbit)
			{
				setbit(track_data,bit_offset,0);
				bit_offset = (bit_offset+1)%track_data_size;
				setbit(track_data,bit_offset,0);
				bit_offset = (bit_offset+1)%track_data_size;
			}
			else
			{
				setbit(track_data,bit_offset,1);
				bit_offset = (bit_offset+1)%track_data_size;
				setbit(track_data,bit_offset,0);
				bit_offset = (bit_offset+1)%track_data_size;
			}
			lastbit = 0;
		}

		b = (unsigned char)( b >> 1 );
		if(!b)
		{
			b = 0x80;
			i++;
		}

	}while(i<bin_data_size);

	return bit_offset;
}

void getMFMcode(track_generator *tg,uint8_t data,uint8_t clock,uint8_t * dstbuf)
{
	uint16_t mfm_code;
	mfm_code = (uint16_t)(LUT_Byte2MFM[data] & LUT_Byte2MFMClkMask[clock] & tg->mfm_last_bit);
	tg->mfm_last_bit = (uint16_t)(~(LUT_Byte2MFM[data]<<15));
	dstbuf[1] = (uint8_t)(mfm_code&0xFF);
	dstbuf[0] = (uint8_t)(mfm_code>>8);

	return;
}

// Fast Bin to MFM converter
int32_t BuildMFMCylinder(uint8_t * mfm_buffer,int32_t mfm_size,uint8_t * track_clk,uint8_t * track_data,int32_t track_size)
{
	int i,l;
	unsigned char byte,clock;
	unsigned short lastbit;
	unsigned short mfm_code;

	if(track_size*2>mfm_size)
	{
		track_size=mfm_size/2;
	}

	// MFM Encoding
	lastbit=0xFFFF;
	i=0;
	for(l=0;l<track_size;l++)
	{
		byte  = track_data[l];
		clock = track_clk[l];

		mfm_code = (uint16_t)(LUT_Byte2MFM[byte] & LUT_Byte2MFMClkMask[clock] & lastbit);

		mfm_buffer[i++] = (uint8_t)(mfm_code>>8);
		mfm_buffer[i++] = (uint8_t)(mfm_code&0xFF);

		lastbit = (uint16_t)(~(LUT_Byte2MFM[byte]<<15));
	}

	// Clear the remaining buffer bytes
	while(i<mfm_size)
	{
		mfm_buffer[i++]=0x00;
	}

	return track_size;
}

// Fast Bin to MFM converter
void FastMFMgenerator(track_generator *tg,HXCFE_SIDE * side,unsigned char * track_data,int size)
{
	int l;
	unsigned int i;
	uint8_t  byte;
	uint16_t lastbit;
	uint16_t mfm_code;
	unsigned char * mfm_buffer;

	mfm_buffer=&side->databuffer[tg->last_bit_offset/8];

	// MFM Encoding
	lastbit = tg->mfm_last_bit;
	i=0;
	for(l=0;l<size;l++)
	{
		byte = track_data[l];
		mfm_code = (uint16_t)(LUT_Byte2MFM[byte] & lastbit);

		mfm_buffer[i++] = (uint8_t)(mfm_code>>8);
		mfm_buffer[i++] = (uint8_t)(mfm_code&0xFF);

		lastbit = (uint16_t)(~(LUT_Byte2MFM[byte]<<15));
	}

	tg->mfm_last_bit = lastbit;
	tg->last_bit_offset = tg->last_bit_offset+(i*8);

	return;
}

// Fast Amiga Bin to MFM converter
void FastAmigaMFMgenerator(track_generator *tg,HXCFE_SIDE * side,unsigned char * track_data,int size)
{
	int32_t  i,l;
	uint8_t  byte;
	uint16_t lastbit;
	uint16_t mfm_code;
	unsigned char * mfm_buffer;

	mfm_buffer = &side->databuffer[tg->last_bit_offset/8];

	// MFM Encoding
	lastbit = tg->mfm_last_bit;
	i=0;

	for(l=0;l<size;l=l+2)
	{
		byte = (uint8_t)((LUT_Byte2OddBits[track_data[l]]<<4) | LUT_Byte2OddBits[track_data[l+1]]);
		mfm_code = (uint16_t)(LUT_Byte2MFM[byte] & lastbit);

		mfm_buffer[i++] = (uint8_t)(mfm_code>>8);
		mfm_buffer[i++] = (uint8_t)(mfm_code&0xFF);

		lastbit = (uint16_t)(~(LUT_Byte2MFM[byte]<<15));
	}


	for(l=0;l<size;l=l+2)
	{
		byte = (uint8_t)((LUT_Byte2EvenBits[track_data[l]]<<4) | LUT_Byte2EvenBits[track_data[l+1]]);
		mfm_code = (uint16_t)(LUT_Byte2MFM[byte] & lastbit);

		mfm_buffer[i++] = (uint8_t)(mfm_code>>8);
		mfm_buffer[i++] = (uint8_t)(mfm_code&0xFF);

		lastbit = (uint16_t)(~(LUT_Byte2MFM[byte]<<15));
	}

	tg->mfm_last_bit=lastbit;
	tg->last_bit_offset=tg->last_bit_offset+(i*8);

	return;
}
