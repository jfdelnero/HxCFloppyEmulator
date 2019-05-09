/*
//
// Copyright (C) 2006-2019 Jean-François DEL NERO
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
// File : trackutils.c
// Contains:
//
// Written by: Jean-François DEL NERO
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "types.h"

#include "luts.h"

int getbit(unsigned char * input_data,int bit_offset)
{
	return ( ( input_data[bit_offset>>3] >> ( 0x7 - (bit_offset&0x7) ) ) ) & 0x01;
}

void setbit(unsigned char * input_data,int bit_offset,int state)
{
	if(state)
	{
		input_data[bit_offset>>3] = (unsigned char)( input_data[bit_offset>>3] |  (0x80 >> ( bit_offset&0x7 ) ) );
	}
	else
	{
		input_data[bit_offset>>3] = (unsigned char)( input_data[bit_offset>>3] & ~(0x80 >> ( bit_offset&0x7 ) ) );
	}

	return;
}

void setfieldbit(unsigned char * dstbuffer,unsigned char byte,int bitoffset,int size)
{
	int i,j;

	i = bitoffset;

	for(j=0;j<size;j++)
	{
		if(byte&((0x80)>>(j&7)))
			dstbuffer[i>>3] = (unsigned char)( dstbuffer[i>>3] | ( (0x80>>(i&7))) );
		else
			dstbuffer[i>>3] = (unsigned char)( dstbuffer[i>>3] & (~(0x80>>(i&7))) );

		i++;
	}
}

int chgbitptr(int tracklen,int cur_offset,int offset)
{
	if( offset>=0 )
	{
		return (cur_offset + offset) % tracklen;
	}
	else
	{
		if(cur_offset >= -offset)
		{
			return cur_offset + offset;
		}
		else
		{
			return (tracklen - ( ((-offset) - cur_offset) ) );
		}
	}
}


int mfmtobin(unsigned char * input_data,int input_data_size,unsigned char * decod_data,int decod_data_size,int bit_offset,int lastbit)
{
	int i,j;
	unsigned char b,c1,c2;
	i=0;
	b=0x80;

	bit_offset = bit_offset%input_data_size;
	j=bit_offset>>3;

	do
	{

		c1 = (unsigned char)( input_data[j] & (0x80>>(bit_offset&7)) );
		bit_offset=(bit_offset+1)%input_data_size;
		j=bit_offset>>3;

		c2 = (unsigned char)( input_data[j] & (0x80>>(bit_offset&7)) );
		bit_offset=(bit_offset+1)%input_data_size;
		j=bit_offset>>3;

		if( c2 && !c1 )
			decod_data[i] = (unsigned char)( decod_data[i] | b );
		else
			decod_data[i] = (unsigned char)( decod_data[i] & ~b );

		b = (unsigned char)( b>>1 );
		if(!b)
		{
			b=0x80;
			i++;
		}

	}while(i<decod_data_size);

	return bit_offset;
}

int bintomfm(unsigned char * track_data,int track_data_size,unsigned char * bin_data,int bin_data_size,int bit_offset)
{
	int i,lastbit;
	unsigned char b;
	i=0;
	b=0x80;

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
			b=0x80;
			i++;
		}

	}while(i<bin_data_size);

	return bit_offset;
}

int fmtobin(unsigned char * input_data,int input_data_size,unsigned char * decod_data,int decod_data_size,int bit_offset,int lastbit)
{
	int i;
	int bitshift;
	unsigned char binbyte;

	i=0;
	bitshift=0;
	binbyte=0;
	do
	{
		//0C0D0C0D

		binbyte = (unsigned char)( binbyte | (getbit(input_data,(bit_offset+3)%input_data_size)<<1) | (getbit(input_data,(bit_offset+7)%input_data_size)<<0));

		bitshift=bitshift+2;

		if(bitshift==8)
		{
			decod_data[i]=binbyte;
			bitshift=0;
			binbyte=0;
			i++;
		}
		else
		{
			binbyte = (unsigned char)( binbyte << 2 );
		}

		bit_offset=(bit_offset+8)%input_data_size;

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

int slowSearchBitStream(unsigned char * input_data,uint32_t input_data_size,int searchlen,unsigned char * chr_data,uint32_t chr_data_size,uint32_t bit_offset)
{
	uint32_t cur_startoffset;
	uint32_t i;
	int tracksearchlen;
	int len;

	cur_startoffset = bit_offset;
	len = 0;

	if(searchlen<=0)
	{
		tracksearchlen = input_data_size;
	}
	else
	{
		tracksearchlen = searchlen;
	}

	while( ( cur_startoffset < input_data_size ) && ( len < tracksearchlen ) )
	{
		i=0;
		while( ( i < chr_data_size) && ( ( getbit(input_data,( (cur_startoffset + i) % input_data_size)) == getbit(chr_data, i % chr_data_size) ) ) )
		{
			i++;
		}

		if(i == chr_data_size)
		{
			return cur_startoffset;
		}

		cur_startoffset++;
		len++;

	}

	// End of track passed ?
	if( (searchlen>=0) && (cur_startoffset == input_data_size ) && ( len < tracksearchlen ) )
	{
		cur_startoffset = 0;
		while( ( cur_startoffset < input_data_size ) && ( len < tracksearchlen ) )
		{
			i=0;
			while( ( i < chr_data_size) && ( ( getbit(input_data,( (cur_startoffset + i) % input_data_size)) == getbit(chr_data, i % chr_data_size) ) ) )
			{
				i++;
			}

			if(i == chr_data_size)
			{
				return cur_startoffset;
			}

			cur_startoffset++;
			len++;

		}
	}

	return -1;
}

int searchBitStream(unsigned char * input_data,uint32_t input_data_size,int searchlen,unsigned char * chr_data,uint32_t chr_data_size,uint32_t bit_offset)
{
	uint32_t i,j,trackoffset,cnt,starti;
	unsigned char stringtosearch[8][128];
	unsigned char prev;
	uint32_t tracksize;
	int searchsize;
	int t;
	int bitoffset;

	cnt=(chr_data_size>>3);
	if(chr_data_size&7)
		cnt++;

	// Prepare strings & mask ( input string shifted 7 times...)
	for(i=0;i<8;i++)
	{
		prev=0;
		for(j=0;j<cnt;j++)
		{
			stringtosearch[i][j]= (unsigned char)(prev | (chr_data[j]>>i));
			prev = (unsigned char)(chr_data[j] << (8-i));
		}
		stringtosearch[i][j]=prev;
	}

	starti = bit_offset & 7;
	trackoffset = bit_offset >> 3;

	tracksize = input_data_size >> 3;
	if( input_data_size & 7 ) tracksize++;

	tracksize= tracksize - ( chr_data_size >> 3 );
	if( chr_data_size & 7 ) tracksize--;

	if(searchlen>0)
	{
		searchsize = searchlen >> 3;
		if( searchlen & 7 ) searchsize++;
	}
	else
	{
		searchsize = tracksize;
	}

	t=0;
	// Scan the track data...
	while( ((trackoffset+(cnt+1))<tracksize) && (t<searchsize) )
	{
		for(i=starti;i<8;i++)
		{
			j=1;
			while( ( j < cnt ) && !( stringtosearch[i][j] ^ input_data[trackoffset + j] ) )
			{
				j++;
			}

			if( j == cnt )
			{	// found!
				if( !( ( stringtosearch[i][0] ^ input_data[trackoffset] ) & (0xFF>>i) ) )
				{
					if( !( ( stringtosearch[i][j] ^ input_data[trackoffset + j] ) & (0xFF<<(8-i)) ) )
					{
						return ( trackoffset << 3 ) + i;
					}
				}
			}
		}

		trackoffset++;
		t++;

		starti=0;
	}

	if(t<searchsize)
	{
		if(searchlen>0)
		{
			if(searchlen - (t*8) > 0)
			{
				bitoffset = slowSearchBitStream(input_data,input_data_size,searchlen - (t*8),chr_data,chr_data_size,trackoffset<<3 | starti);
			}
			else
			{
				bitoffset = -1;
			}
		}
		else
		{
			bitoffset = slowSearchBitStream(input_data,input_data_size,searchlen,chr_data,chr_data_size,trackoffset<<3 | starti);
		}
	}
	else
	{
		bitoffset = -1;
	}

	return bitoffset;
}

void sortbuffer(unsigned char * buffer,unsigned char * outbuffer,int size)
{
	int i;
	unsigned short * word_outbuffer,w;

	word_outbuffer=(unsigned short *)outbuffer;
	for(i=0;i<(size/2);i++)
	{
		w = (unsigned short)((LUT_Byte2ShortOddBitsExpander[buffer[i]]<<1)| (LUT_Byte2ShortOddBitsExpander[buffer[i+(size/2)]]));
		word_outbuffer[i] = (unsigned short)( (w>>8) | (w<<8) );
	}
}
