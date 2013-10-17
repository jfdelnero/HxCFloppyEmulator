/*
//
// Copyright (C) 2006 - 2013 Jean-François DEL NERO
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
//-------------------------------------------------------------------------------//
//-----------H----H--X----X-----CCCCC----22222----0000-----0000------11----------//
//----------H----H----X-X-----C--------------2---0----0---0----0--1--1-----------//
//---------HHHHHH-----X------C----------22222---0----0---0----0-----1------------//
//--------H----H----X--X----C----------2-------0----0---0----0-----1-------------//
//-------H----H---X-----X---CCCCC-----222222----0000-----0000----1111------------//
//-------------------------------------------------------------------------------//
//----------------------------------------------------- http://hxc2001.free.fr --//
///////////////////////////////////////////////////////////////////////////////////
// File : trackutils.c
// Contains:
//
// Written by:	DEL NERO Jean Francois
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

unsigned short biteven[]=
{
	0x0000, 0x0001, 0x0004, 0x0005, 0x0010, 0x0011, 0x0014, 0x0015,
	0x0040, 0x0041, 0x0044, 0x0045, 0x0050, 0x0051, 0x0054, 0x0055,
	0x0100, 0x0101, 0x0104, 0x0105, 0x0110, 0x0111, 0x0114, 0x0115,
	0x0140, 0x0141, 0x0144, 0x0145, 0x0150, 0x0151, 0x0154, 0x0155,
	0x0400, 0x0401, 0x0404, 0x0405, 0x0410, 0x0411, 0x0414, 0x0415,
	0x0440, 0x0441, 0x0444, 0x0445, 0x0450, 0x0451, 0x0454, 0x0455,
	0x0500, 0x0501, 0x0504, 0x0505, 0x0510, 0x0511, 0x0514, 0x0515,
	0x0540, 0x0541, 0x0544, 0x0545, 0x0550, 0x0551, 0x0554, 0x0555,
	0x1000, 0x1001, 0x1004, 0x1005, 0x1010, 0x1011, 0x1014, 0x1015,
	0x1040, 0x1041, 0x1044, 0x1045, 0x1050, 0x1051, 0x1054, 0x1055,
	0x1100, 0x1101, 0x1104, 0x1105, 0x1110, 0x1111, 0x1114, 0x1115,
	0x1140, 0x1141, 0x1144, 0x1145, 0x1150, 0x1151, 0x1154, 0x1155,
	0x1400, 0x1401, 0x1404, 0x1405, 0x1410, 0x1411, 0x1414, 0x1415,
	0x1440, 0x1441, 0x1444, 0x1445, 0x1450, 0x1451, 0x1454, 0x1455,
	0x1500, 0x1501, 0x1504, 0x1505, 0x1510, 0x1511, 0x1514, 0x1515,
	0x1540, 0x1541, 0x1544, 0x1545, 0x1550, 0x1551, 0x1554, 0x1555,
	0x4000, 0x4001, 0x4004, 0x4005, 0x4010, 0x4011, 0x4014, 0x4015,
	0x4040, 0x4041, 0x4044, 0x4045, 0x4050, 0x4051, 0x4054, 0x4055,
	0x4100, 0x4101, 0x4104, 0x4105, 0x4110, 0x4111, 0x4114, 0x4115,
	0x4140, 0x4141, 0x4144, 0x4145, 0x4150, 0x4151, 0x4154, 0x4155,
	0x4400, 0x4401, 0x4404, 0x4405, 0x4410, 0x4411, 0x4414, 0x4415,
	0x4440, 0x4441, 0x4444, 0x4445, 0x4450, 0x4451, 0x4454, 0x4455,
	0x4500, 0x4501, 0x4504, 0x4505, 0x4510, 0x4511, 0x4514, 0x4515,
	0x4540, 0x4541, 0x4544, 0x4545, 0x4550, 0x4551, 0x4554, 0x4555,
	0x5000, 0x5001, 0x5004, 0x5005, 0x5010, 0x5011, 0x5014, 0x5015,
	0x5040, 0x5041, 0x5044, 0x5045, 0x5050, 0x5051, 0x5054, 0x5055,
	0x5100, 0x5101, 0x5104, 0x5105, 0x5110, 0x5111, 0x5114, 0x5115,
	0x5140, 0x5141, 0x5144, 0x5145, 0x5150, 0x5151, 0x5154, 0x5155,
	0x5400, 0x5401, 0x5404, 0x5405, 0x5410, 0x5411, 0x5414, 0x5415,
	0x5440, 0x5441, 0x5444, 0x5445, 0x5450, 0x5451, 0x5454, 0x5455,
	0x5500, 0x5501, 0x5504, 0x5505, 0x5510, 0x5511, 0x5514, 0x5515,
	0x5540, 0x5541, 0x5544, 0x5545, 0x5550, 0x5551, 0x5554, 0x5555
};

int getbit(unsigned char * input_data,int bit_offset)
{
	return ((input_data[bit_offset>>3]>>(0x7-(bit_offset&0x7))))&0x01;
}

void setbit(unsigned char * input_data,int bit_offset,int state)
{
	if(state)
	{
		input_data[bit_offset>>3] = input_data[bit_offset>>3] |  (0x80 >> ( bit_offset&0x7 ) );
	}
	else
	{
		input_data[bit_offset>>3] = input_data[bit_offset>>3] & ~(0x80 >> ( bit_offset&0x7 ) );
	}

	return;
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

		c1=input_data[j] & (0x80>>(bit_offset&7));
		bit_offset=(bit_offset+1)%input_data_size;
		j=bit_offset>>3;

		c2=input_data[j] & (0x80>>(bit_offset&7));
		bit_offset=(bit_offset+1)%input_data_size;
		j=bit_offset>>3;

		if( c2 && !c1 )
			decod_data[i] = decod_data[i] | b;
		else
			decod_data[i] = decod_data[i] & ~b;

		b=b>>1;
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

		b=b>>1;
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

		binbyte=binbyte | (getbit(input_data,(bit_offset+3)%input_data_size)<<1) | (getbit(input_data,(bit_offset+7)%input_data_size)<<0);

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
			binbyte=binbyte<<2;
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

		b=b>>1;
		if(!b)
		{
			b=0x80;
			i++;
		}

	}while(i<bin_data_size);

	return bit_offset;
}

int arburgsysfmtobin(unsigned char * input_data,int input_data_size,unsigned char * decod_data,int decod_data_size,int bit_offset,int lastbit)
{
	int i;
	int bitshift;
	unsigned char binbyte;

	i=0;
	bitshift=0;
	binbyte=0;
	do
	{

		if( getbit(input_data,(bit_offset+1)%input_data_size) )
		{
			//01 -> 0

			binbyte = binbyte & 0xFE;

			bit_offset=(bit_offset+2)%input_data_size;
		}
		else
		{
			//001 -> 1

			binbyte = binbyte | 0x01;

			bit_offset=(bit_offset+3)%input_data_size;
		}

		bitshift++;

		if(bitshift==8)
		{
			decod_data[i]=binbyte;
			bitshift=0;
			binbyte=0;
			i++;
		}
		else
		{
			binbyte=binbyte<<1;
		}

	}while(i<decod_data_size);

	return bit_offset;
}


int slowSearchBitStream(unsigned char * input_data,unsigned long input_data_size,int searchlen,unsigned char * chr_data,unsigned long chr_data_size,unsigned long bit_offset)
{
	unsigned long cur_startoffset;
	unsigned long i;
	int len;
	
	cur_startoffset = bit_offset;
	len = 0;

	if(searchlen<=0)
	{
		searchlen = input_data_size;
	}

	while( ( cur_startoffset < input_data_size ) && ( len < searchlen ) )
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

	return -1;
}


int searchBitStream(unsigned char * input_data,unsigned long input_data_size,int searchlen,unsigned char * chr_data,unsigned long chr_data_size,unsigned long bit_offset)
{
	unsigned long i,j,trackoffset,cnt,k,starti;
	unsigned char stringtosearch[8][128];
	unsigned char mask[8][128];

	unsigned char prev;
	unsigned long tracksize;
	int searchsize;
	int t;
	int found;
	int bitoffset;

	memset(stringtosearch,0,8*128);
	memset(mask,0xFF,8*128);

	cnt=(chr_data_size>>3);
	if(chr_data_size&7)
		cnt++;

	// Prepare strings & mask ( input string shifted 7 times...)
	for(i=0;i<8;i++)
	{
		prev=0;

		mask[i][0]=0xFF>>i;
		for(j=0;j<cnt;j++)
		{
			stringtosearch[i][j]=prev | (chr_data[j]>>i);
			prev=chr_data[j]<<(8-i);
		}
		stringtosearch[i][j]=prev;
		mask[i][j]=0xFF<<(8-i);

	}

	found=0;
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
	while(!found && (trackoffset<tracksize) && (t<searchsize))
	{

		for(i=starti;i<8;i++)
		{
			k = trackoffset;
			j=0;

			while( ( j < (cnt+1) ) && ( ( stringtosearch[i][j] & mask[i][j] ) == ( input_data[k] & mask[i][j] ) )  )
			{
				j++;
				k++;
			}

			if( j == (cnt+1) )
			{
				found=0xFF;
				bitoffset = ( trackoffset << 3 ) + i;
				i=8;
			}
		}

		trackoffset++;
		t++;

		starti=0;

	}

	if(!found)
	{
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
		w=(biteven[buffer[i]]<<1)| (biteven[buffer[i+(size/2)]]);
		word_outbuffer[i]=(w>>8) | (w<<8);
	}
}
