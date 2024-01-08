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
// File : dec_m2fm_encoding.c
// Contains: DEC RX02 M2FM encoding support
//
// Written by: Jean-François DEL NERO
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include "types.h"

#include <string.h>

#include "internal_libhxcfe.h"
#include "tracks/track_generator.h"
#include "libhxcfe.h"

#include "tracks/trackutils.h"
#include "mfm_encoding.h"
#include "dec_m2fm_encoding.h"

int decm2fmtobin(unsigned char * input_data,int input_data_size,unsigned char * decod_data,int decod_data_size,int bit_offset,int lastbit)
{
	unsigned char tmp_mfm_buffer[(256 + 2)*2 + 1]; // ((256 data + 2 crc) * 2)
	int i,off;
	unsigned char tosearch[4];
	unsigned char toreplace[4];

	if(decod_data_size > 256 + 2)
		decod_data_size = 256+2;

	memset(tmp_mfm_buffer,0,sizeof(tmp_mfm_buffer));

	setbit(tmp_mfm_buffer,0, 0 );
	for(i=0;i<(decod_data_size*8*2);i++)
	{
		setbit(tmp_mfm_buffer,i + 1, getbit(input_data,(bit_offset+i)%(input_data_size*8)) );
	}

	#define WORDSIZE 11

	// Encoded DEC M2FM "011110" replacement...

	tosearch[0] = 0x44;         //0100 0100 010 - DEC M2FM
	tosearch[1] = 0x40;

	toreplace[0] = 0x2A;        //0010 1010 100 - Normal MFM
	toreplace[1] = 0x80;

	off = 0;
	do
	{
		off = slowSearchBitStream(tmp_mfm_buffer,(decod_data_size*8*2),-1,(unsigned char*)&tosearch,WORDSIZE,off);
		if(off != -1)
		{
			if(off&1)
			{
				// Unaligned !
				off++;
			}
			else
			{
				// Replace the DEC rule with the normal MFM encoding
				for(i=0;i<WORDSIZE;i++)
				{
					setbit(tmp_mfm_buffer,off+i, getbit(toreplace,i) );
				}
				off += WORDSIZE-1;
			}
		}
	}while(off != -1);

	// Use the normal MFM decoding.
	return mfmtobin(tmp_mfm_buffer, NULL, input_data_size+1, decod_data,decod_data_size, 1,lastbit) + bit_offset;
}

int mfmtodecm2fm(unsigned char * input_data,int input_data_size,int bit_offset,int size)
{
	int i,off,passed,prev_offset;
	unsigned char tosearch[4];
	unsigned char toreplace[4];
	int start_offset,aligned;

	// Encoded DEC M2FM "011110" replacement...

	#define WORDSIZE 11

	toreplace[0] = 0x44;       // 0100 0100 010 - DEC M2FM
	toreplace[1] = 0x40;

	tosearch[0] = 0x2A;        // 0010 1010 100 - Normal MFM
	tosearch[1] = 0x80;

	// To avoid false bit stream mfm detection after the crc.
	setbit(input_data,(bit_offset+size)%input_data_size, 1 );

	off = bit_offset;
	start_offset = bit_offset;
	do
	{
		prev_offset = off;

		off = slowSearchBitStream(input_data,input_data_size,size,(unsigned char*)&tosearch,WORDSIZE,off);
		if(off != -1)
		{
			if( prev_offset < off )
			{
				passed = off - prev_offset;
			}
			else
			{
				passed = (input_data_size - off) + prev_offset;
			}

			if( !(start_offset & 1) )
			{
				if(off&1)
					aligned = 1;
				else
					aligned = 0;
			}
			else
			{
				if( !(off & 1) )
					aligned = 1;
				else
					aligned = 0;
			}

			if(!aligned)
			{
				// Unaligned !
				off++;
				size--;
			}
			else
			{
				// Replace the DEC rule with the normal MFM encoding
				for(i=0;i<WORDSIZE;i++)
				{
					setbit(input_data,(off+i)%input_data_size, getbit(toreplace,i) );
				}

				off += WORDSIZE;
				size -= WORDSIZE;
			}

			size -= passed;
		}
	}while(off != -1 && size > WORDSIZE );

	return 0;
}
