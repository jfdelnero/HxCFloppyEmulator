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
// File : sector_extractor.c
// Contains: ISO/IBM sector reader
//
// Written by:	DEL NERO Jean Francois
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "libhxcfe.h"
#include "sector_extractor.h"
#include "./tracks/crc.h"

extern unsigned char bit_inverter_emuii[];
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

extern unsigned char even_tab[];
extern unsigned char odd_tab[];

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


int fmtobin(unsigned char * input_data,int intput_data_size,unsigned char * decod_data,int decod_data_size,int bit_offset,int lastbit)
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

		binbyte=binbyte | (getbit(input_data,bit_offset+3)<<1) | (getbit(input_data,bit_offset+7)<<0);

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

		bit_offset=bit_offset+8;

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

int searchBitStream(unsigned char * input_data,unsigned long intput_data_size,int searchlen,unsigned char * chr_data,unsigned long chr_data_size,unsigned long bit_offset)
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

	tracksize = intput_data_size >> 3;
	if( intput_data_size & 7 ) tracksize++;

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
	do
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

	}while(!found && (trackoffset<tracksize) && (t<searchsize));

	if(!found)
	{
		bitoffset=-1;
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


#define LOOKFOR_GAP1 0x01
#define LOOKFOR_ADDM 0x02
#define ENDOFTRACK 0x03
#define EXTRACTSECTORINFO 0x04
#define ENDOFSECTOR 0x05

unsigned short sectorsize[]={128,256,512,1024,2048,4096,8192,16384};

int get_next_MFM_sector(HXCFLOPPYEMULATOR* floppycontext,SIDE * track,SECTORCONFIG * sector,int track_offset)
{
	int bit_offset_bak,bit_offset,old_bit_offset,tmp_bit_offset;
	int sector_size;
	unsigned char mfm_buffer[32];
	unsigned char tmp_buffer[32];
	unsigned char * tmp_sector;
	unsigned char CRC16_High;
	unsigned char CRC16_Low;
	int sector_extractor_sm;
	int k;
	unsigned char crctable[32];

	memset(sector,0,sizeof(SECTORCONFIG));

	bit_offset=track_offset;

	sector_extractor_sm=LOOKFOR_GAP1;

	do
	{
		switch(sector_extractor_sm)
		{
			case LOOKFOR_GAP1:

				mfm_buffer[0]=0x44;
				mfm_buffer[1]=0x89;
				mfm_buffer[2]=0x44;
				mfm_buffer[3]=0x89;
				mfm_buffer[4]=0x44;
				mfm_buffer[5]=0x89;

				bit_offset=searchBitStream(track->databuffer,track->tracklen,-1,mfm_buffer,6*8,bit_offset);

				if(bit_offset!=-1)
				{
					sector_extractor_sm=LOOKFOR_ADDM;
				}
				else
				{
					sector_extractor_sm=ENDOFTRACK;
				}
			break;

			case LOOKFOR_ADDM:
				tmp_bit_offset = mfmtobin(track->databuffer,track->tracklen,tmp_buffer,3+7,bit_offset,0);
				if(tmp_buffer[3]==0xFE)
				{
					CRC16_Init(&CRC16_High,&CRC16_Low,(unsigned char*)crctable,0x1021,0xFFFF);
					for(k=0;k<3+7;k++)
					{
						CRC16_Update(&CRC16_High,&CRC16_Low, tmp_buffer[k],(unsigned char*)crctable );
					}

					sector->cylinder = tmp_buffer[4];
					sector->head = tmp_buffer[5];
					sector->sector = tmp_buffer[6];
					sector->sectorsize = sectorsize[tmp_buffer[7]&0x7];
					sector->alternate_sector_size_id = tmp_buffer[7];
					sector->trackencoding = ISOFORMAT_DD;
					sector->alternate_datamark = 0x00;
					sector->use_alternate_datamark = 0x00;
					sector->alternate_addressmark = 0xFE;
					sector->use_alternate_addressmark = 0xFF;
					sector->header_crc = ( tmp_buffer[k-2]<<8 ) | tmp_buffer[k-1] ;
					sector->use_alternate_header_crc = 0xFF;

					sector->startsectorindex=bit_offset;

					if(!CRC16_High && !CRC16_Low)
					{ // crc ok !!!
 						floppycontext->hxc_printf(MSG_DEBUG,"Valid MFM sector header found - Cyl:%d Side:%d Sect:%d Size:%d",tmp_buffer[4],tmp_buffer[5],tmp_buffer[6],sectorsize[tmp_buffer[7]&0x7]);
						sector->use_alternate_header_crc = 0;
					}
					else
					{
						floppycontext->hxc_printf(MSG_DEBUG,"Bad MFM sector header found - Cyl:%d Side:%d Sect:%d Size:%d",tmp_buffer[4],tmp_buffer[5],tmp_buffer[6],sectorsize[tmp_buffer[7]&0x7]);
					}

					old_bit_offset = bit_offset;

					bit_offset++;
					sector_size = sectorsize[tmp_buffer[7]&0x7];
					bit_offset_bak = bit_offset;

					mfm_buffer[0] = 0x44;
					mfm_buffer[1] = 0x89;
					mfm_buffer[2] = 0x44;
					mfm_buffer[3] = 0x89;
					mfm_buffer[4] = 0x44;
					mfm_buffer[5] = 0x89;
					bit_offset=searchBitStream(track->databuffer,track->tracklen,(88+10)*8,mfm_buffer,6*8,bit_offset);

					if((bit_offset!=-1))
					{

						tmp_sector=(unsigned char*)malloc(3+1+sector_size+2);
						memset(tmp_sector,0,3+1+sector_size+2);

						sector->startdataindex = bit_offset;
						sector->endsectorindex = mfmtobin(track->databuffer,track->tracklen,tmp_sector,3+1+sector_size+2,bit_offset,0);
						sector->alternate_datamark = tmp_sector[3];
						sector->use_alternate_datamark = 0xFF;

						CRC16_Init(&CRC16_High,&CRC16_Low,(unsigned char*)crctable,0x1021,0xFFFF);
						for(k=0;k<3+1+sector_size+2;k++)
						{
							CRC16_Update(&CRC16_High,&CRC16_Low, tmp_sector[k],(unsigned char*)crctable );
						}

						sector->data_crc = ( tmp_sector[k-2]<<8 ) | tmp_sector[k-1] ;

						if(!CRC16_High && !CRC16_Low)
						{ // crc ok !!!
							floppycontext->hxc_printf(MSG_DEBUG,"crc data ok.");
						}
						else
						{
							floppycontext->hxc_printf(MSG_DEBUG,"crc data error!");
							sector->use_alternate_data_crc=0xFF;
						}

						sector->input_data=(unsigned char*)malloc(sector_size);
						memcpy(sector->input_data,&tmp_sector[4],sector_size);
						free(tmp_sector);
						
						bit_offset++;

						sector_extractor_sm=ENDOFSECTOR;
					}
					else
					{
						sector->startdataindex = tmp_bit_offset;
						sector->endsectorindex = tmp_bit_offset;

						bit_offset = bit_offset_bak + 1;

						sector_extractor_sm=ENDOFSECTOR;
					}
				}
				else
				{
					if(tmp_buffer[3]==0xF8 || tmp_buffer[3]==0xF9 || tmp_buffer[3]==0xFA || tmp_buffer[3]==0xFB)
					{
						sector->startsectorindex = bit_offset;
						sector->startdataindex = bit_offset;
						sector->endsectorindex = mfmtobin(track->databuffer,track->tracklen,tmp_buffer,3+7,bit_offset,0);
 						floppycontext->hxc_printf(MSG_DEBUG,"get_next_MFM_sector : Data sector without sector header !?!");
						
						old_bit_offset=bit_offset;

						sector->cylinder = 0;
						sector->head = 0;
						sector->sector = 0;
						sector->sectorsize = 0;
						sector->alternate_sector_size_id = 0;
						sector->trackencoding = ISOFORMAT_DD;
						sector->alternate_datamark = tmp_buffer[3];
						sector->use_alternate_datamark= 0xFF;
						sector->header_crc = 0;
						bit_offset++;
						bit_offset_bak=bit_offset;

						sector_extractor_sm=ENDOFSECTOR;
					}
					else
					{
						bit_offset++;
						sector_extractor_sm=LOOKFOR_GAP1;
					}
				}
			break;

			case ENDOFTRACK:

			break;

			default:
				sector_extractor_sm=ENDOFTRACK;
			break;

		}
	}while(	(sector_extractor_sm!=ENDOFTRACK) && (sector_extractor_sm!=ENDOFSECTOR));

	return bit_offset;
}

int get_next_MEMBRAIN_sector(HXCFLOPPYEMULATOR* floppycontext,SIDE * track,SECTORCONFIG * sector,int track_offset)
{
	int bit_offset_bak,bit_offset,old_bit_offset,tmp_bit_offset;
	int sector_size;
	unsigned char mfm_buffer[32];
	unsigned char tmp_buffer[32];
	unsigned char * tmp_sector;
	unsigned char CRC16_High;
	unsigned char CRC16_Low;
	int sector_extractor_sm;
	int k;
	unsigned char crctable[32];

	memset(sector,0,sizeof(SECTORCONFIG));

	bit_offset=track_offset;

	sector_extractor_sm=LOOKFOR_GAP1;

	do
	{
		switch(sector_extractor_sm)
		{
			case LOOKFOR_GAP1:

				mfm_buffer[0]=0x44;
				mfm_buffer[1]=0x89;
				mfm_buffer[2]=0x55;
				mfm_buffer[3]=0x54;

				bit_offset=searchBitStream(track->databuffer,track->tracklen,-1,mfm_buffer,4*8,bit_offset);

				if(bit_offset!=-1)
				{
					sector_extractor_sm=LOOKFOR_ADDM;
				}
				else
				{
					sector_extractor_sm=ENDOFTRACK;
				}
			break;

			case LOOKFOR_ADDM:
				tmp_bit_offset = mfmtobin(track->databuffer,track->tracklen,tmp_buffer,3+7,bit_offset,0);
				if(tmp_buffer[1]==0xFE)
				{
					CRC16_Init(&CRC16_High,&CRC16_Low,(unsigned char*)crctable,0x8005,0x0000);
					for(k=0;k<2+4;k++)
					{
						CRC16_Update(&CRC16_High,&CRC16_Low, tmp_buffer[k],(unsigned char*)crctable );
					}

					sector->cylinder = ((tmp_buffer[2]&0x1F)<<3) | ( (tmp_buffer[3]&0xE0)>>5 );
					sector->head = ((tmp_buffer[3])>>4)&1;
					sector->sector = tmp_buffer[3]&0xF;
					sector->sectorsize = sectorsize[2];
					sector->alternate_sector_size_id = 2;// tmp_buffer[7];
					sector->trackencoding = MEMBRAINFORMAT_DD;
					sector->alternate_datamark=0x00;
					sector->use_alternate_datamark=0xFF;
					sector->header_crc = ( tmp_buffer[k-2]<<8 ) | tmp_buffer[k-1] ;
					sector->use_alternate_header_crc = 0xFF;

					sector->startsectorindex=bit_offset;

					if(!CRC16_High && !CRC16_Low)
					{ // crc ok !!!
 						floppycontext->hxc_printf(MSG_DEBUG,"Valid MFM sector header found - Cyl:%d Side:%d Sect:%d Size:%d",tmp_buffer[4],tmp_buffer[5],tmp_buffer[6],sectorsize[tmp_buffer[7]&0x7]);
						sector->use_alternate_header_crc = 0;
					}
					else
					{
						floppycontext->hxc_printf(MSG_DEBUG,"Bad MFM sector header found - Cyl:%d Side:%d Sect:%d Size:%d",tmp_buffer[4],tmp_buffer[5],tmp_buffer[6],sectorsize[tmp_buffer[7]&0x7]);
						sector_extractor_sm=LOOKFOR_GAP1;
						bit_offset++;
						break;
					}

					old_bit_offset = bit_offset;

					bit_offset++;
					sector_size = sectorsize[2];
					bit_offset_bak = bit_offset;

					mfm_buffer[0] = 0x44;
					mfm_buffer[1] = 0x89;
					mfm_buffer[2] = 0x55;
					mfm_buffer[3] = 0x4A;
					bit_offset=searchBitStream(track->databuffer,track->tracklen,(88+10)*8,mfm_buffer,4*8,bit_offset);

					if((bit_offset!=-1))
					{

						tmp_sector=(unsigned char*)malloc(1+1+sector_size+2);
						memset(tmp_sector,0,1+1+sector_size+2);

						sector->startdataindex=bit_offset;
						sector->endsectorindex=mfmtobin(track->databuffer,track->tracklen,tmp_sector,1+1+sector_size+2,bit_offset,0);
						sector->alternate_datamark=tmp_sector[1];
						sector->use_alternate_datamark=0xFF;

						CRC16_Init(&CRC16_High,&CRC16_Low,(unsigned char*)crctable,0x8005,0x0000);
						for(k=0;k<1+1+sector_size+2;k++)
						{
							CRC16_Update(&CRC16_High,&CRC16_Low, tmp_sector[k],(unsigned char*)crctable );
						}

						sector->data_crc= ( tmp_sector[k-2]<<8 ) | tmp_sector[k-1] ;

						if(!CRC16_High && !CRC16_Low)
						{ // crc ok !!!
							floppycontext->hxc_printf(MSG_DEBUG,"crc data ok.");
						}
						else
						{
							floppycontext->hxc_printf(MSG_DEBUG,"crc data error!");
							sector->use_alternate_data_crc=0xFF;
						}

						sector->input_data=(unsigned char*)malloc(sector_size);
						memcpy(sector->input_data,&tmp_sector[2],sector_size);
						free(tmp_sector);
						
						bit_offset++;

						sector_extractor_sm=ENDOFSECTOR;
					}
					else
					{
						sector->startdataindex = tmp_bit_offset;
						sector->endsectorindex = tmp_bit_offset;

						bit_offset = bit_offset_bak + 1;

						sector_extractor_sm=ENDOFSECTOR;
					}
				}
				else
				{
					if(tmp_buffer[3]==0xF8 || tmp_buffer[3]==0xF9 || tmp_buffer[3]==0xFA || tmp_buffer[3]==0xFB)
					{
						sector->startsectorindex = bit_offset;
						sector->startdataindex = bit_offset;
						sector->endsectorindex = mfmtobin(track->databuffer,track->tracklen,tmp_buffer,3+7,bit_offset,0);
 						floppycontext->hxc_printf(MSG_DEBUG,"get_next_MEMBRAIN_sector : Data sector without sector header !?!");
						
						old_bit_offset=bit_offset;

						sector->cylinder = 0;
						sector->head = 0;
						sector->sector = 0;
						sector->sectorsize = 0;
						sector->alternate_sector_size_id = 0;
						sector->trackencoding = MEMBRAINFORMAT_DD;
						sector->alternate_datamark=tmp_buffer[3];
						sector->use_alternate_datamark= 0xFF;
						sector->header_crc = 0;
						bit_offset++;
						bit_offset_bak=bit_offset;

						sector_extractor_sm=ENDOFSECTOR;
					}
					else
					{
						bit_offset++;
						sector_extractor_sm=LOOKFOR_GAP1;
					}
				}
			break;

			case ENDOFTRACK:

			break;

			default:
				sector_extractor_sm=ENDOFTRACK;
			break;

		}
	}while(	(sector_extractor_sm!=ENDOFTRACK) && (sector_extractor_sm!=ENDOFSECTOR));

	return bit_offset;
}


int get_next_AMIGAMFM_sector(HXCFLOPPYEMULATOR* floppycontext,SIDE * track,SECTORCONFIG * sector_conf,int track_offset)
{
	int bit_offset,old_bit_offset;
	int sector_size;

	unsigned char   header[4];
	unsigned char   headerparity[2];
	unsigned char   sectorparity[2];

	unsigned char mfm_buffer[32];
	unsigned char tmp_buffer[32];
	unsigned char sector_data[544];
	unsigned char temp_sector[512];
	int sector_extractor_sm,i;

	memset(sector_conf,0,sizeof(SECTORCONFIG));

	bit_offset=track_offset;

	sector_extractor_sm=LOOKFOR_GAP1;

	headerparity[0]=0;
	headerparity[1]=0;

	sectorparity[0]=0;
	sectorparity[1]=0;

	do
	{
		switch(sector_extractor_sm)
		{
			case LOOKFOR_GAP1:

				mfm_buffer[0]=0xAA;
				mfm_buffer[1]=0xAA;
				mfm_buffer[2]=0x44;
				mfm_buffer[3]=0x89;
				mfm_buffer[4]=0x44;
				mfm_buffer[5]=0x89;

				bit_offset=searchBitStream(track->databuffer,track->tracklen,-1,mfm_buffer,6*8,bit_offset);

				if(bit_offset!=-1)
				{
					sector_extractor_sm=LOOKFOR_ADDM;
				}
				else
				{
					sector_extractor_sm=ENDOFTRACK;
				}
			break;

			case LOOKFOR_ADDM:
				

				bit_offset=bit_offset-(8*2);

				sector_conf->startdataindex = mfmtobin(track->databuffer,track->tracklen,sector_data,32,bit_offset,0);

				sector_conf->endsectorindex = mfmtobin(track->databuffer,track->tracklen,sector_data,544,bit_offset,0);

				memcpy(&header,&sector_data[4],4);
				sortbuffer((unsigned char*)&header,tmp_buffer,4);
				memcpy(&header,tmp_buffer,4);
								
				// Compute the header parity
				for(i=0;i<5;i++)
				{
					sortbuffer(&sector_data[4+(i*4)],tmp_buffer,4);

					headerparity[0]^=( odd_tab[tmp_buffer[0]]<<4)|( odd_tab[tmp_buffer[1]]);
					headerparity[1]^=( odd_tab[tmp_buffer[2]]<<4)|( odd_tab[tmp_buffer[3]]);
					headerparity[0]^=(even_tab[tmp_buffer[0]]<<4)|(even_tab[tmp_buffer[1]]);
					headerparity[1]^=(even_tab[tmp_buffer[2]]<<4)|(even_tab[tmp_buffer[3]]);
				}

				sector_conf->header_crc = headerparity[1] | (headerparity[0]<<8) ;

				// Is the header valid (parity ok?)
				if( (header[0]==0xFF) && ( (headerparity[0] == sector_data[26]) && (headerparity[1] == sector_data[27]) ) )
				{
					sector_conf->startsectorindex=bit_offset;
					//sector_conf->startdataindex=bit_offset;
					
					sector_size = 512;

					floppycontext->hxc_printf(MSG_DEBUG,"Valid Amiga MFM sector header found - Cyl:%d Side:%d Sect:%d Size:%d",header[1]>>1,header[1]&1,header[2],sector_size);

					old_bit_offset = bit_offset;

					sortbuffer(&sector_data[32],temp_sector,sector_size);
					memcpy(&sector_data[32],temp_sector,sector_size);

					sector_conf->cylinder = header[1]>>1;
					sector_conf->head = header[1]&1;
					sector_conf->sector = header[2];
					sector_conf->sectorsize = sector_size;
					sector_conf->trackencoding = AMIGAFORMAT_DD;

					// Check the data parity
					for(i=0;i<128;i++)
					{
						memcpy(tmp_buffer,&sector_data[32+(i*4)],4);

						sectorparity[0]^=( odd_tab[tmp_buffer[0]]<<4)|( odd_tab[tmp_buffer[1]]);
						sectorparity[1]^=( odd_tab[tmp_buffer[2]]<<4)|( odd_tab[tmp_buffer[3]]);
						sectorparity[0]^=(even_tab[tmp_buffer[0]]<<4)|(even_tab[tmp_buffer[1]]);
						sectorparity[1]^=(even_tab[tmp_buffer[2]]<<4)|(even_tab[tmp_buffer[3]]);
					}
					
					sector_conf->data_crc = sectorparity[1] | (sectorparity[0]<<8) ;

					if( ( sectorparity[0] == sector_data[30]) && ( sectorparity[1] == sector_data[31]) )
					{
						// parity ok !!!
						floppycontext->hxc_printf(MSG_DEBUG,"data parity ok.");
						sector_conf->use_alternate_data_crc = 0x00;	

					}
					else
					{
						floppycontext->hxc_printf(MSG_DEBUG,"data parity error!");
						sector_conf->use_alternate_data_crc = 0xFF;
					}

					sector_conf->input_data=(unsigned char*)malloc(sector_size);
					memcpy(sector_conf->input_data,&sector_data[32],sector_size);

					bit_offset=bit_offset+(8*2)+1;

					sector_extractor_sm=ENDOFSECTOR;

				}
				else
				{
					bit_offset=bit_offset+(8*2)+1;
					
					sector_conf->use_alternate_header_crc = 0xFF;

					sector_conf->endsectorindex = sector_conf->startdataindex;
					sector_extractor_sm = LOOKFOR_GAP1;
				}


			break;

			case ENDOFTRACK:
			break;

			default:
				sector_extractor_sm=ENDOFTRACK;
			break;

		}
	}while(	(sector_extractor_sm!=ENDOFTRACK) && (sector_extractor_sm!=ENDOFSECTOR));

	return bit_offset;
}

int get_next_FM_sector(HXCFLOPPYEMULATOR* floppycontext,SIDE * track,SECTORCONFIG * sector,int track_offset)
{
	int bit_offset,old_bit_offset;
	int sector_size;
	unsigned char fm_buffer[32];
	unsigned char tmp_buffer[32];
	unsigned char * tmp_sector;
	unsigned char CRC16_High;
	unsigned char CRC16_Low;
	int sector_extractor_sm;
	int k,i;
	unsigned char crctable[32];

	//0xF8 - 01000100 // 0x44
	//0xF9 - 01000101 // 0x45
	//0xFA - 01010100 // 0x54
	//0xFB - 01010101 // 0x55
	unsigned char datamark[4]={0x44,0x45,0x54,0x55};

	bit_offset=track_offset;
	memset(sector,0,sizeof(SECTORCONFIG));

	sector_extractor_sm=LOOKFOR_GAP1;

	do
	{
		switch(sector_extractor_sm)
		{
			case LOOKFOR_GAP1:
				fm_buffer[0]=0x55;
				fm_buffer[1]=0x11;
				fm_buffer[2]=0x15;
				fm_buffer[3]=0x54;

				bit_offset=searchBitStream(track->databuffer,track->tracklen,-1,fm_buffer,4*8,bit_offset);

				if(bit_offset!=-1)
				{
					sector_extractor_sm=LOOKFOR_ADDM;
				}
				else
				{
					sector_extractor_sm=ENDOFTRACK;
				}
			break;

			case LOOKFOR_ADDM:
				sector->endsectorindex = fmtobin(track->databuffer,track->tracklen,tmp_buffer,7,bit_offset,0);
				if(tmp_buffer[0]==0xFE)
				{
					sector->startsectorindex = bit_offset;
					sector->startdataindex = sector->endsectorindex;  

					sector->use_alternate_addressmark = 0xFF;
					sector->alternate_addressmark = 0xFE;

					sector->use_alternate_datamark = 0x00;
					sector->alternate_datamark = 0x00;

					sector->cylinder = tmp_buffer[1];
					sector->head = tmp_buffer[2];
					sector->sector = tmp_buffer[3];
					sector->sectorsize = sectorsize[tmp_buffer[4]&0x7];
					sector->alternate_sector_size_id = tmp_buffer[4];
					sector->trackencoding = ISOFORMAT_SD;

					sector->use_alternate_header_crc = 0xFF;
					CRC16_Init(&CRC16_High,&CRC16_Low,(unsigned char*)crctable,0x1021,0xFFFF);
					for(k=0;k<7;k++)
					{
						CRC16_Update(&CRC16_High,&CRC16_Low, tmp_buffer[k],(unsigned char*)crctable );
					}

					sector->header_crc = ( tmp_buffer[k-2]<<8 ) | tmp_buffer[k-1] ;


					if(!CRC16_High && !CRC16_Low)
					{ // crc ok !!!
						sector->use_alternate_header_crc = 0x00;
						bit_offset = bit_offset + ( 7 * 8 );
						floppycontext->hxc_printf(MSG_DEBUG,"Valid FM sector header found - Cyl:%d Side:%d Sect:%d Size:%d",tmp_buffer[1],tmp_buffer[2],tmp_buffer[3],sectorsize[tmp_buffer[4]&0x7]);
						old_bit_offset=bit_offset;

						sector_size = sector->sectorsize;


//;01000100 01010101 00010001 00010100 01[s]010101
//;            1   1    1   1    1   0       1   1   -- FB
//;            1   1    1   1    1   0       1   0   -- FA
//;            1   1    1   1    1   0       0   0   -- F8
//;            1   1    1   1    1   0       0   1   -- F9

						//11111011
						fm_buffer[0]=0x55;
						fm_buffer[1]=0x11;
						fm_buffer[2]=0x14;
						fm_buffer[3]=0x55;

						i=0;
						do
						{
							fm_buffer[3]=datamark[i];
							bit_offset=searchBitStream(track->databuffer,track->tracklen,((88+10)*8*2),fm_buffer,4*8,old_bit_offset);
							i++;
						}while(i<4 && bit_offset==-1 );

						if(bit_offset != -1)
						{
							sector->use_alternate_datamark = 0xFF;
							sector->alternate_datamark=0xF8 + (i-1);

							tmp_sector=(unsigned char*)malloc(1+sector_size+2);
							memset(tmp_sector,0,1+sector_size+2);

							sector->startdataindex=bit_offset;
							sector->endsectorindex=fmtobin(track->databuffer,track->tracklen,tmp_sector,1+sector_size+2,bit_offset+(0*8),0);

							CRC16_Init(&CRC16_High,&CRC16_Low,(unsigned char*)crctable,0x1021,0xFFFF);
							for(k=0;k<1+sector_size+2;k++)
							{
								CRC16_Update(&CRC16_High,&CRC16_Low, tmp_sector[k],(unsigned char*)crctable );
							}

							sector->data_crc= ( tmp_sector[k-2]<<8 ) | tmp_sector[k-1] ;

							if(!CRC16_High && !CRC16_Low)
							{ // crc ok !!!
								floppycontext->hxc_printf(MSG_DEBUG,"crc data ok.");
							}
							else
							{
								floppycontext->hxc_printf(MSG_DEBUG,"crc data error!");
								sector->use_alternate_data_crc=0xFF;
							}

							sector->input_data=(unsigned char*)malloc(sector_size);
							memcpy(sector->input_data,&tmp_sector[1],sector_size);
							free(tmp_sector);

							bit_offset=bit_offset+1;//(((sector_size+2)*4)*8);

							sector_extractor_sm=ENDOFSECTOR;

						}
						else
						{
							bit_offset=old_bit_offset+1;
							floppycontext->hxc_printf(MSG_DEBUG,"No data!");
							sector_extractor_sm=ENDOFSECTOR;
						}
					}
					else
					{
						sector_extractor_sm=LOOKFOR_GAP1;
						bit_offset++;
					}
				}
				else
				{
					sector_extractor_sm=LOOKFOR_GAP1;
					bit_offset++;
				}
			break;

			case ENDOFTRACK:

			break;

			default:
				sector_extractor_sm=ENDOFTRACK;
			break;

		}
	}while(	(sector_extractor_sm!=ENDOFTRACK) && (sector_extractor_sm!=ENDOFSECTOR));

	return bit_offset;
}


int get_next_TYCOMFM_sector(HXCFLOPPYEMULATOR* floppycontext,SIDE * track,SECTORCONFIG * sector,int track_offset)
{
	int bit_offset,old_bit_offset;
	int sector_size;
	unsigned char fm_buffer[32];
	unsigned char tmp_buffer[32];
	unsigned char * tmp_sector;
	unsigned char CRC16_High;
	unsigned char CRC16_Low;
	int sector_extractor_sm;
	int k,i;
	unsigned char crctable[32];

	//0xF8 - 01000100 // 0x44
	//0xF9 - 01000101 // 0x45
	//0xFA - 01010100 // 0x54
	//0xFB - 01010101 // 0x55
	unsigned char datamark[4]={0x44,0x45,0x54,0x55};

	bit_offset=track_offset;
	memset(sector,0,sizeof(SECTORCONFIG));

	sector_extractor_sm=LOOKFOR_GAP1;

	do
	{
		switch(sector_extractor_sm)
		{
			case LOOKFOR_GAP1:
				fm_buffer[0]=0x55;
				fm_buffer[1]=0x11;
				fm_buffer[2]=0x15;
				fm_buffer[3]=0x54;

				bit_offset=searchBitStream(track->databuffer,track->tracklen,-1,fm_buffer,4*8,bit_offset);

				if(bit_offset!=-1)
				{
					sector_extractor_sm=LOOKFOR_ADDM;
				}
				else
				{
					sector_extractor_sm=ENDOFTRACK;
				}
			break;

			case LOOKFOR_ADDM:
				sector->endsectorindex=fmtobin(track->databuffer,track->tracklen,tmp_buffer,7,bit_offset,0);
				if(tmp_buffer[0]==0xFE)
				{
					CRC16_Init(&CRC16_High,&CRC16_Low,(unsigned char*)crctable,0x1021,0xFFFF);
					for(k=0;k<5;k++)
					{
						CRC16_Update(&CRC16_High,&CRC16_Low, tmp_buffer[k],(unsigned char*)crctable );
					}

					if(!CRC16_High && !CRC16_Low)
					{ // crc ok !!!
						sector->startsectorindex=bit_offset;
						bit_offset = bit_offset + ( 5 * 8 );
						floppycontext->hxc_printf(MSG_DEBUG,"Valid TYCOM FM sector header found - Cyl:%d Side:%d Sect:%d Size:%d",tmp_buffer[1],tmp_buffer[2],tmp_buffer[3],sectorsize[tmp_buffer[4]&0x7]);
						old_bit_offset=bit_offset;

						sector->use_alternate_addressmark=0xFF;
						sector->alternate_datamark=0x00;
						sector->cylinder=tmp_buffer[1];
						sector->head=0x00;
						sector->sector=tmp_buffer[2];
						sector->sectorsize=128;
						sector->alternate_sector_size_id = 0x00;
						sector->trackencoding = ISOFORMAT_SD;
						sector->header_crc = ( tmp_buffer[k-2]<<8 ) | tmp_buffer[k-1] ;
						sector_size = sector->sectorsize;


//;01000100 01010101 00010001 00010100 01[s]010101
//;            1   1    1   1    1   0       1   1   -- FB
//;            1   1    1   1    1   0       1   0   -- FA
//;            1   1    1   1    1   0       0   0   -- F8
//;            1   1    1   1    1   0       0   1   -- F9

						//11111011
						fm_buffer[0]=0x55;
						fm_buffer[1]=0x11;
						fm_buffer[2]=0x14;
						fm_buffer[3]=0x55;

						i=0;
						do
						{
							fm_buffer[3]=datamark[i];
							bit_offset=searchBitStream(track->databuffer,track->tracklen,((88+10)*8*2),fm_buffer,4*8,old_bit_offset);
							i++;
						}while(i<4 && bit_offset==-1 );

						if(bit_offset != -1)
						{
							sector->alternate_datamark=0xF8 + (i-1);

							tmp_sector=(unsigned char*)malloc(1+sector_size+2);
							memset(tmp_sector,0,1+sector_size+2);

							sector->startdataindex=bit_offset;
							sector->endsectorindex=fmtobin(track->databuffer,track->tracklen,tmp_sector,1+sector_size+2,bit_offset+(0*8),0);

							CRC16_Init(&CRC16_High,&CRC16_Low,(unsigned char*)crctable,0x1021,0xFFFF);
							for(k=0;k<1+sector_size+2;k++)
							{
								CRC16_Update(&CRC16_High,&CRC16_Low, tmp_sector[k],(unsigned char*)crctable );
							}

							sector->data_crc= ( tmp_sector[k-2]<<8 ) | tmp_sector[k-1] ;

							if(!CRC16_High && !CRC16_Low)
							{ // crc ok !!!
								floppycontext->hxc_printf(MSG_DEBUG,"crc data ok.");
							}
							else
							{
								floppycontext->hxc_printf(MSG_DEBUG,"crc data error!");
								sector->use_alternate_data_crc=0xFF;
							}

							sector->input_data=(unsigned char*)malloc(sector_size);
							memcpy(sector->input_data,&tmp_sector[1],sector_size);
							free(tmp_sector);

							bit_offset=bit_offset+1;//(((sector_size+2)*4)*8);

							sector_extractor_sm=ENDOFSECTOR;

						}
						else
						{
							bit_offset=old_bit_offset+1;
							floppycontext->hxc_printf(MSG_DEBUG,"No data!");
							sector_extractor_sm=ENDOFSECTOR;
						}
					}
					else
					{
						sector_extractor_sm=LOOKFOR_GAP1;
						bit_offset++;
					}
				}
				else
				{
					sector_extractor_sm=LOOKFOR_GAP1;
					bit_offset++;
				}
			break;

			case ENDOFTRACK:

			break;

			default:
				sector_extractor_sm=ENDOFTRACK;
			break;

		}
	}while(	(sector_extractor_sm!=ENDOFTRACK) && (sector_extractor_sm!=ENDOFSECTOR));

	return bit_offset;
}

int write_FM_sectordata(HXCFLOPPYEMULATOR* floppycontext,SIDE * track,SECTORCONFIG * sector,unsigned char * buffer,int buffersize)
{
	int bit_offset,i;
	unsigned char CRC16_High;
	unsigned char CRC16_Low;
	unsigned char crctable[32];

	// Data CRC
	CRC16_Init(&CRC16_High,&CRC16_Low,(unsigned char*)crctable,0x1021,0xFFFF);

	CRC16_Update(&CRC16_High,&CRC16_Low, sector->alternate_datamark ,(unsigned char*)crctable );

	for(i=0;i<buffersize;i++)
	{
		CRC16_Update(&CRC16_High,&CRC16_Low, buffer[i],(unsigned char*)crctable );
	}

	bit_offset = sector->startdataindex;
	bit_offset = bintofm(track->databuffer,track->tracklen,&sector->alternate_datamark,1,bit_offset);
	// Clear missing clocks
	setbit(track->databuffer,bit_offset-15,0);
	setbit(track->databuffer,bit_offset-19,0);
	setbit(track->databuffer,bit_offset-23,0);
	bit_offset = bintofm(track->databuffer,track->tracklen,buffer,buffersize,bit_offset);
	bit_offset = bintofm(track->databuffer,track->tracklen,&CRC16_High,1,bit_offset);
	bit_offset = bintofm(track->databuffer,track->tracklen,&CRC16_Low ,1,bit_offset);

	return 0;
}

int write_MFM_sectordata(HXCFLOPPYEMULATOR* floppycontext,SIDE * track,SECTORCONFIG * sector,unsigned char * buffer,int buffersize)
{
	int bit_offset,i;
	unsigned char CRC16_High;
	unsigned char CRC16_Low;
	unsigned char temp;
	unsigned char crctable[32];

	temp = 0x4E;
	// Data CRC
	CRC16_Init(&CRC16_High,&CRC16_Low,(unsigned char*)crctable,0x1021,0xFFFF);

	CRC16_Update(&CRC16_High,&CRC16_Low, 0xA1 ,(unsigned char*)crctable );
	CRC16_Update(&CRC16_High,&CRC16_Low, 0xA1 ,(unsigned char*)crctable );
	CRC16_Update(&CRC16_High,&CRC16_Low, 0xA1 ,(unsigned char*)crctable );
	CRC16_Update(&CRC16_High,&CRC16_Low, sector->alternate_datamark ,(unsigned char*)crctable );

	for(i=0;i<buffersize;i++)
	{
		CRC16_Update(&CRC16_High,&CRC16_Low, buffer[i],(unsigned char*)crctable );
	}

	bit_offset = sector->startdataindex + (3*8*2);
	bit_offset = bintomfm(track->databuffer,track->tracklen,&sector->alternate_datamark,1,bit_offset);
	bit_offset = bintomfm(track->databuffer,track->tracklen,buffer,buffersize,bit_offset);
	bit_offset = bintomfm(track->databuffer,track->tracklen,&CRC16_High,1,bit_offset);
	bit_offset = bintomfm(track->databuffer,track->tracklen,&CRC16_Low ,1,bit_offset);
	bit_offset = bintomfm(track->databuffer,track->tracklen,&temp ,1,bit_offset);

	return 0;
}

int write_AMIGAMFM_sectordata(HXCFLOPPYEMULATOR* floppycontext,SIDE * track,SECTORCONFIG * sector,unsigned char * buffer,int buffersize)
{
	int bit_offset;
	unsigned char CRC16_High;
	unsigned char CRC16_Low;

	bit_offset = sector->startdataindex;
	bit_offset = bintomfm(track->databuffer,track->tracklen,&sector->alternate_addressmark,1,bit_offset);
	bit_offset = bintomfm(track->databuffer,track->tracklen,buffer,buffersize,bit_offset);
	bit_offset = bintomfm(track->databuffer,track->tracklen,&CRC16_High,1,bit_offset);
	bit_offset = bintomfm(track->databuffer,track->tracklen,&CRC16_Low ,1,bit_offset);

	return 0;
}

int analysis_and_extract_sector_EMUIIFM(HXCFLOPPYEMULATOR* floppycontext,SIDE * track,sect_track * sectors)
{
	int bit_offset,old_bit_offset;
	int sector_size;
	unsigned char fm_buffer[32];
	unsigned char tmp_buffer[32];
	unsigned char * tmp_sector;
	unsigned char CRC16_High;
	unsigned char CRC16_Low;
	int sector_extractor_sm;
	int number_of_sector;
	int k;
	unsigned char crctable[32];
	bit_offset=0;
	number_of_sector=0;

	sector_extractor_sm=LOOKFOR_GAP1;

	do
	{
		switch(sector_extractor_sm)
		{
			case LOOKFOR_GAP1:
/*				fm_buffer[0]=0x55;
				fm_buffer[1]=0x55;
				fm_buffer[2]=0x54;
				fm_buffer[3]=0x54;
				fm_buffer[4]=0x54;
				fm_buffer[5]=0x45;
				fm_buffer[6]=0x45;
				fm_buffer[7]=0x54;*/

				fm_buffer[0]=0x45;
				fm_buffer[1]=0x45;
				fm_buffer[2]=0x55;
				fm_buffer[3]=0x55;
				fm_buffer[4]=0x45;
				fm_buffer[5]=0x54;
				fm_buffer[6]=0x54;
				fm_buffer[7]=0x45;


				bit_offset=searchBitStream(track->databuffer,track->tracklen,-1,fm_buffer,8*8,bit_offset);

				if(bit_offset!=-1)
				{
					sector_extractor_sm=LOOKFOR_ADDM;
				}
				else
				{
					sector_extractor_sm=ENDOFTRACK;
				}
			break;

			case LOOKFOR_ADDM:
				fmtobin(track->databuffer,track->tracklen,tmp_buffer,5,bit_offset,0);
				if((bit_inverter_emuii[tmp_buffer[0]]==0xFA) && (bit_inverter_emuii[tmp_buffer[1]]==0x96))
				{
					CRC16_Init(&CRC16_High,&CRC16_Low,(unsigned char*)crctable,0x8005,0x0000);
					for(k=0;k<3;k++)
					CRC16_Update(&CRC16_High,&CRC16_Low, tmp_buffer[2+k],(unsigned char*)crctable );


					if(!CRC16_High && !CRC16_Low)
					{ // crc ok !!!
						number_of_sector++;
						floppycontext->hxc_printf(MSG_DEBUG,"Valid EmuII FM sector header found - Sect:%d",bit_inverter_emuii[tmp_buffer[2]]);
						old_bit_offset=bit_offset;

						sector_size = 0xE00;

						//11111011
						fm_buffer[0]=0x45;
						fm_buffer[1]=0x45;
						fm_buffer[2]=0x55;
						fm_buffer[3]=0x55;
						fm_buffer[4]=0x45;
						fm_buffer[5]=0x54;
						fm_buffer[6]=0x54;
						fm_buffer[7]=0x45;

						bit_offset=searchBitStream(track->databuffer,track->tracklen,-1,fm_buffer,8*8,bit_offset+(4*8*4));

						if((bit_offset-old_bit_offset<((88+10)*8*2)) && bit_offset!=-1)
						{
							sectors->number_of_sector++;
							sectors->sectorlist=(sect_sector **)realloc(sectors->sectorlist,sizeof(sect_sector *)*sectors->number_of_sector);
							sectors->sectorlist[sectors->number_of_sector-1]=(sect_sector*)malloc(sizeof(sect_sector));
							memset(sectors->sectorlist[sectors->number_of_sector-1],0,sizeof(sect_sector));

							sectors->sectorlist[sectors->number_of_sector-1]->track_id=bit_inverter_emuii[tmp_buffer[2]]>>1;
							sectors->sectorlist[sectors->number_of_sector-1]->side_id=bit_inverter_emuii[tmp_buffer[2]]&1;
							sectors->sectorlist[sectors->number_of_sector-1]->sector_id=1;
							sectors->sectorlist[sectors->number_of_sector-1]->sectorsize=0xE00;

							tmp_sector=(unsigned char*)malloc(sector_size+2);
							memset(tmp_sector,0,sector_size+2);

							fmtobin(track->databuffer,track->tracklen,tmp_sector,sector_size+2,bit_offset+(8 *8),0);

							CRC16_Init(&CRC16_High,&CRC16_Low,(unsigned char*)crctable,0x8005,0x0000);
							for(k=0;k<sector_size+2;k++)
							{
								CRC16_Update(&CRC16_High,&CRC16_Low, tmp_sector[k],(unsigned char*)crctable );
							}

							if(!CRC16_High && !CRC16_Low)
							{ // crc ok !!!
								floppycontext->hxc_printf(MSG_DEBUG,"crc data ok.");
							}
							else
							{
								floppycontext->hxc_printf(MSG_DEBUG,"crc data error!");
							}

							for(k=0;k<sector_size;k++)
							{
								tmp_sector[k]=bit_inverter_emuii[tmp_sector[k]];
							}

							sectors->sectorlist[sectors->number_of_sector-1]->buffer=(unsigned char*)malloc(sector_size);
							memcpy(sectors->sectorlist[sectors->number_of_sector-1]->buffer,tmp_sector,sector_size);
							free(tmp_sector);

							bit_offset=bit_offset+(sector_size*4);

						}
						else
						{
							bit_offset=old_bit_offset+1;
							floppycontext->hxc_printf(MSG_DEBUG,"No data!");

						}
					}
					else
					{
						sector_extractor_sm=LOOKFOR_GAP1;
						bit_offset++;
					}
				}
				else
				{
					sector_extractor_sm=LOOKFOR_GAP1;
					bit_offset++;
				}

				sector_extractor_sm=LOOKFOR_GAP1;
			break;

			case ENDOFTRACK:

			break;

			default:
				sector_extractor_sm=ENDOFTRACK;
			break;

		}
	}while(	sector_extractor_sm!=ENDOFTRACK);

	return number_of_sector;
}

SECTORSEARCH* hxcfe_initSectorSearch(HXCFLOPPYEMULATOR* floppycontext,FLOPPY *fp)
{
	SECTORSEARCH* ss;

	ss = (SECTORSEARCH*) malloc(sizeof(SECTORSEARCH));
	memset(ss,0,sizeof(SECTORSEARCH));

	ss->fp=fp;
	ss->bitoffset=0;
	ss->cur_side=0;
	ss->cur_track=0;
	ss->hxcfe=floppycontext;

	return ss;
}

SECTORCONFIG* hxcfe_getNextSector(SECTORSEARCH* ss,int track,int side,int type)
{
	SECTORCONFIG * sc;
	int bitoffset;
	int i;

	if((ss->bitoffset==-1) || (ss->cur_side!=side) || (ss->cur_track!=track))
	{
		bitoffset=0;
		ss->nb_sector_cached = 0;
	}
	else
		bitoffset=ss->bitoffset;

	ss->cur_track=track;
	ss->cur_side=side;

	sc=(SECTORCONFIG *) malloc(sizeof(SECTORCONFIG));

	switch(type)
	{
		case ISOIBM_MFM_ENCODING:
			bitoffset=get_next_MFM_sector(ss->hxcfe,ss->fp->tracks[track]->sides[side],sc,bitoffset);
		break;
		case AMIGA_MFM_ENCODING:
			bitoffset=get_next_AMIGAMFM_sector(ss->hxcfe,ss->fp->tracks[track]->sides[side],sc,bitoffset);
		break;
		case ISOIBM_FM_ENCODING:
			bitoffset=get_next_FM_sector(ss->hxcfe,ss->fp->tracks[track]->sides[side],sc,bitoffset);
		break;
		case TYCOM_FM_ENCODING:
			bitoffset=get_next_TYCOMFM_sector(ss->hxcfe,ss->fp->tracks[track]->sides[side],sc,bitoffset);
		break;
		case MEMBRAIN_MFM_ENCODING:
			bitoffset=get_next_MEMBRAIN_sector(ss->hxcfe,ss->fp->tracks[track]->sides[side],sc,bitoffset);
		break;
		case EMU_FM_ENCODING:
			bitoffset=-1;
		break;
		default:
			bitoffset=-1;
		break;
	}


	ss->bitoffset=bitoffset;

	if(ss->nb_sector_cached<512 && bitoffset>=0)
	{
		//Add a new cache entry
		i=0;
		while(i<ss->nb_sector_cached && ss->sectorcache[i].startsectorindex!=(unsigned long)bitoffset && i<512)
		{
			i++;
		}

		if( i<512 )
		{
			if(i == ss->nb_sector_cached)
			{
				memcpy(&ss->sectorcache[i],sc,sizeof(SECTORCONFIG));
				ss->sectorcache[i].input_data = 0;
				ss->nb_sector_cached++;
			}
		}
	}

	if(bitoffset!=-1)
		return sc;
	else
	{
		free(sc);
		return 0;
	}
}

SECTORCONFIG** hxcfe_getAllTrackSectors(SECTORSEARCH* ss,int track,int side,int type,int * nb_sectorfound)
{
	int i;
	SECTORCONFIG * sc;
	SECTORCONFIG ** scarray;
	int nb_of_sector;

	nb_of_sector = 0;
	// First : Count the number of sectors
	do
	{
		sc = hxcfe_getNextSector(ss,track,side,type);
		if(sc)
		{
			if(sc->input_data)
				free(sc->input_data);
			free(sc);

			nb_of_sector++;
		}
	}while(sc);

	if(nb_sectorfound)
		*nb_sectorfound = nb_of_sector;

	scarray = 0;
	if(nb_of_sector)
	{
		scarray = malloc(sizeof(SECTORCONFIG *) * (nb_of_sector+1));
		if(scarray)
		{
			memset(scarray,0,sizeof(SECTORCONFIG *) * (nb_of_sector+1));
			for(i=0;i<nb_of_sector;i++)
			{
				sc = hxcfe_getNextSector(ss,track,side,type);
				scarray[i] = sc;
			}
		}
	}

	return scarray;
}


SECTORCONFIG** hxcfe_getAllTrackISOSectors(SECTORSEARCH* ss,int track,int side,int * nb_sectorfound)
{
	int i,i_fm,i_mfm;
	SECTORCONFIG * sc;

	SECTORCONFIG ** sc_fm_array;
	SECTORCONFIG ** sc_mfm_array;

	SECTORCONFIG ** scarray;
	int nb_of_sector;

	nb_of_sector = 0;
	// First : Count the number of sectors
	do
	{
		sc = hxcfe_getNextSector(ss,track,side,ISOIBM_MFM_ENCODING);
		if(sc)
		{
			if(sc->input_data)
				free(sc->input_data);
			free(sc);

			nb_of_sector++;
		}
	}while(sc);

	// FM
	do
	{
		sc = hxcfe_getNextSector(ss,track,side,ISOIBM_FM_ENCODING);
		if(sc)
		{
			if(sc->input_data)
				free(sc->input_data);
			free(sc);

			nb_of_sector++;
		}
	}while(sc);


	if(nb_sectorfound)
		*nb_sectorfound = nb_of_sector;

	scarray = 0;
	if(nb_of_sector)
	{
		sc_mfm_array = malloc(sizeof(SECTORCONFIG *) * (nb_of_sector+1));
		sc_fm_array  = malloc(sizeof(SECTORCONFIG *) * (nb_of_sector+1));
		scarray = malloc(sizeof(SECTORCONFIG *) * (nb_of_sector+1));
		if(scarray && sc_mfm_array && sc_fm_array)
		{
			memset(scarray     ,0,sizeof(SECTORCONFIG *) * (nb_of_sector+1));
			memset(sc_mfm_array,0,sizeof(SECTORCONFIG *) * (nb_of_sector+1));
			memset(sc_fm_array ,0,sizeof(SECTORCONFIG *) * (nb_of_sector+1));

			i = 0;
			do
			{
				sc = hxcfe_getNextSector(ss,track,side,ISOIBM_FM_ENCODING);
				sc_fm_array[i] = sc;
				i++;
			}while(sc);

			i = 0;
			do
			{
				sc = hxcfe_getNextSector(ss,track,side,ISOIBM_MFM_ENCODING);
				sc_mfm_array[i] = sc;
				i++;
			}while(sc);

			i_fm = 0;
			i_mfm = 0;
			for(i=0;i<nb_of_sector;i++)
			{
				if(sc_fm_array[i_fm] && sc_mfm_array[i_mfm])
				{
					if(sc_fm_array[i_fm]->startsectorindex < sc_mfm_array[i_mfm]->startsectorindex)
					{
						scarray[i] = malloc(sizeof(SECTORCONFIG));
						if(scarray[i])
						{
							memset(scarray[i],0,sizeof(SECTORCONFIG));
							memcpy(scarray[i], sc_fm_array[i_fm], sizeof(SECTORCONFIG));
							free(sc_fm_array[i_fm]);
						}
						i_fm++;
					}
					else
					{
						scarray[i] = malloc(sizeof(SECTORCONFIG));
						if(scarray[i])
						{
							memset(scarray[i],0,sizeof(SECTORCONFIG));
							memcpy(scarray[i], sc_mfm_array[i_mfm], sizeof(SECTORCONFIG));
							free(sc_mfm_array[i_mfm]);
						}
						i_mfm++;
					}
				}
				else
				{
					if(sc_fm_array[i_fm])
					{
						scarray[i] = malloc(sizeof(SECTORCONFIG));
						if(scarray[i])
						{
							memset(scarray[i],0,sizeof(SECTORCONFIG));
							memcpy(scarray[i], sc_fm_array[i_fm], sizeof(SECTORCONFIG));
							free(sc_fm_array[i_fm]);
						}
						i_fm++;
					}
					else
					{
						if(sc_mfm_array[i_mfm])
						{
							scarray[i] = malloc(sizeof(SECTORCONFIG));
							if(scarray[i])
							{
								memset(scarray[i],0,sizeof(SECTORCONFIG));
								memcpy(scarray[i], sc_mfm_array[i_mfm], sizeof(SECTORCONFIG));
								free(sc_mfm_array[i_mfm]);
							}
							i_mfm++;
						}
					}
				}
			}
		}

		if(sc_fm_array)
			free(sc_fm_array);
		if(sc_mfm_array)
			free(sc_mfm_array);

	}

	return scarray;
}


SECTORCONFIG* hxcfe_searchSector(SECTORSEARCH* ss,int track,int side,int id,int type)
{
	SECTORCONFIG * sc;
	int i;

	if((ss->cur_side == side) && (ss->cur_track == track))
	{
		// Search in the cache
		i = 0;
		while( i < ss->nb_sector_cached )
		{
			if((ss->sectorcache[i].sector == id) && (ss->sectorcache[i].cylinder == track) && (ss->sectorcache[i].head == side) )
			{
				ss->bitoffset =  ss->sectorcache[i].startsectorindex;
				sc = hxcfe_getNextSector(ss,track,side,type);
				return sc;
			}
			i++;
		}
	}
	else
	{
		ss->nb_sector_cached = 0;
	}

	ss->bitoffset = 0;
	if(ss->nb_sector_cached)
		ss->bitoffset = ss->sectorcache[ss->nb_sector_cached-1].startsectorindex+1;

	do
	{
		sc = hxcfe_getNextSector(ss,track,side,type);

		if(sc)
		{
			if(sc->sector == id )
			{
				return sc;
			}
			else
			{
				if(sc->input_data)
					free(sc->input_data);

				free(sc);
			}
		}

	}while( sc );

	return 0;
}

int hxcfe_getSectorSize(SECTORSEARCH* ss,SECTORCONFIG* sc)
{
	return sc->sectorsize;
}

unsigned char * hxcfe_getSectorData(SECTORSEARCH* ss,SECTORCONFIG* sc)
{
	return sc->input_data;
}

int hxcfe_getFloppySize(HXCFLOPPYEMULATOR* floppycontext,FLOPPY *fp,int * nbsector)
{
	SECTORSEARCH* ss;
	SECTORCONFIG* sc;
	int floppysize;
	int nbofsector;
	int track;
	int side;
	int i,type,secfound,t;
	int typetab[8];

	floppysize=0;
	nbofsector=0;

	type=0;
	secfound=0;

	i=0;
	typetab[i++]=ISOIBM_MFM_ENCODING;
	typetab[i++]=AMIGA_MFM_ENCODING;
	typetab[i++]=ISOIBM_FM_ENCODING;
	typetab[i++]=TYCOM_FM_ENCODING;
	typetab[i++]=MEMBRAIN_MFM_ENCODING;
	typetab[i++]=EMU_FM_ENCODING;
	typetab[i++]=-1;

	ss=hxcfe_initSectorSearch(floppycontext,fp);
	if(ss)
	{
		for(track=0;track<fp->floppyNumberOfTrack;track++)
		{
			for(side=0;side<fp->floppyNumberOfSide;side++)
			{
				secfound=0;
				type=0;

				do
				{
					do
					{
						sc=hxcfe_getNextSector(ss,track,side,typetab[type]);
						if(sc)
						{
							floppysize=floppysize+sc->sectorsize;
							nbofsector++;
							secfound=1;
						}
						hxcfe_freeSectorConfig(ss,sc);
					}while(sc);

					if(!secfound)
					{
						type++;
					}

				}while(type<4 && !secfound);

				if(secfound)
				{
					t=typetab[0];
					typetab[0]=typetab[type];
					typetab[type]=t;
				}
			}
		}
	}

	if(nbsector)
		*nbsector=nbofsector;

	hxcfe_deinitSectorSearch(ss);
	return floppysize;

}

int hxcfe_readSectorData(SECTORSEARCH* ss,int track,int side,int sector,int numberofsector,int sectorsize,int type,unsigned char * buffer)
{
	SECTORCONFIG * sc;
	int bitoffset;
	int nbsectorread;

	bitoffset=0;
	nbsectorread=0;

	if ( side < ss->fp->floppyNumberOfSide && track < ss->fp->floppyNumberOfTrack )
	{

		do
		{
			sc = hxcfe_searchSector ( ss, track, side, sector + nbsectorread, type);
			if(sc)
			{
				if(sc->sectorsize == (unsigned int)sectorsize)
				{
					if(sc->input_data)
					{
						memcpy(&buffer[sectorsize*(sc->sector-sector)],sc->input_data,sectorsize);
						free(sc->input_data);
						sc->input_data=0;	
					}

					free(sc);

					nbsectorread++;
				}
				else
				{
					if(sc->input_data)
					{
						free(sc->input_data);
						sc->input_data=0;
					}
					free(sc);

					return 0;
				}

			}

		}while((nbsectorread<numberofsector) && sc);
	}

	return nbsectorread;
}

int hxcfe_writeSectorData(SECTORSEARCH* ss,int track,int side,int sector,int numberofsector,int sectorsize,int type,unsigned char * buffer)
{
	SECTORCONFIG * sc;
	int bitoffset;
	int nbsectorwrite;

	bitoffset=0;
	nbsectorwrite=0;

	if ( side < ss->fp->floppyNumberOfSide && track < ss->fp->floppyNumberOfTrack )
	{

		do
		{
			sc = hxcfe_searchSector ( ss, track, side, sector + nbsectorwrite, type);
			if(sc)
			{
				if(((sc->sector>=sector) && (sc->sector < ( sector + numberofsector )) ) )
				{
					switch(type)
					{
						case ISOIBM_MFM_ENCODING:
							write_MFM_sectordata(ss->hxcfe,ss->fp->tracks[track]->sides[side],sc,&buffer[sectorsize*nbsectorwrite],sectorsize);
						break;
						case AMIGA_MFM_ENCODING:
							write_AMIGAMFM_sectordata(ss->hxcfe,ss->fp->tracks[track]->sides[side],sc,&buffer[sectorsize*nbsectorwrite],sectorsize);
						break;
						case TYCOM_FM_ENCODING:
						case ISOIBM_FM_ENCODING:
							write_FM_sectordata(ss->hxcfe,ss->fp->tracks[track]->sides[side],sc,&buffer[sectorsize*nbsectorwrite],sectorsize);
						break;
						case MEMBRAIN_MFM_ENCODING:
							//write_ALT01_MFM_sectordata(ss->hxcfe,ss->fp->tracks[track]->sides[side],sc,&buffer[sectorsize*nbsectorwrite],sectorsize);
						break;
						case EMU_FM_ENCODING:
						break;
						default:
						break;
					}
					
					nbsectorwrite++;
				}

				if(sc->input_data)
				{
					free(sc->input_data);
					sc->input_data=0;
				}

				free(sc);

			}

		}while(( nbsectorwrite < numberofsector ) && sc);
	}

	return nbsectorwrite;

}


void hxcfe_freeSectorConfig(SECTORSEARCH* ss,SECTORCONFIG* sc)
{
	if(sc)
	{
		if(sc->input_data)
			free(sc->input_data);
		free(sc);
	}
}


void hxcfe_deinitSectorSearch(SECTORSEARCH* ss)
{
	if(ss)
		free(ss);
}

FDCCTRL * hxcfe_initFDC (HXCFLOPPYEMULATOR* floppycontext)
{
	FDCCTRL * fdc;

	fdc = malloc(sizeof(FDCCTRL));
	if( fdc )
	{
		memset(fdc,0,sizeof(FDCCTRL));
		fdc->floppycontext = floppycontext;
		return fdc;
	}

	return 0;
}

int hxcfe_insertDiskFDC (FDCCTRL * fdc,FLOPPY *fp)
{
	if(fdc)
	{
		fdc->loadedfp = fp;

		if( fdc->ss )
		{
			hxcfe_deinitSectorSearch(fdc->ss);
			fdc->ss = 0;
		}
		fdc->ss = hxcfe_initSectorSearch(fdc->floppycontext,fp);

		return HXCFE_NOERROR;
	}

	return HXCFE_BADPARAMETER;
}

int hxcfe_readSectorFDC (FDCCTRL * fdc,unsigned char track,unsigned char side,unsigned char sector,int sectorsize,int mode,int nbsector,unsigned char * buffer,int buffer_size)
{
	if(fdc)
	{
		if(fdc->ss && fdc->loadedfp && ((sectorsize*nbsector)<=buffer_size))
			return hxcfe_readSectorData(fdc->ss,track,side,sector,nbsector,sectorsize,mode,buffer);
		else
			return HXCFE_BADPARAMETER;
	}

	return HXCFE_BADPARAMETER;
}

int hxcfe_writeSectorFDC (FDCCTRL * fdc,unsigned char track,unsigned char side,unsigned char sector,int sectorsize,int mode,int nbsector,unsigned char * buffer,int buffer_size)
{
	if(fdc)
	{
		if(fdc->ss && fdc->loadedfp && ((sectorsize*nbsector)<=buffer_size))
			return hxcfe_writeSectorData(fdc->ss,track,side,sector,nbsector,sectorsize,mode,buffer);
		else
			return HXCFE_BADPARAMETER;
	}

	return HXCFE_BADPARAMETER;
}

void hxcfe_deinitFDC (FDCCTRL * fdc)
{
	if(fdc)
	{
		if(fdc->ss)
			hxcfe_deinitSectorSearch(fdc->ss);
		free(fdc);
	}
}

int hxcfe_FDC_READSECTOR (HXCFLOPPYEMULATOR* floppycontext,FLOPPY *fp,unsigned char track,unsigned char side,unsigned char sector,int sectorsize,int mode,int nbsector,unsigned char * buffer,int buffer_size)
{
	FDCCTRL * fdcctrl;
	unsigned char cnt;

	fdcctrl = hxcfe_initFDC (floppycontext);
	if( fdcctrl )
	{
		hxcfe_insertDiskFDC (fdcctrl,fp);

		cnt = hxcfe_readSectorFDC (fdcctrl,track,side,sector,sectorsize,mode,nbsector,buffer,buffer_size);

		hxcfe_deinitFDC (fdcctrl);
	}

	return cnt;
}

int hxcfe_FDC_WRITESECTOR (HXCFLOPPYEMULATOR* floppycontext,FLOPPY *fp,unsigned char track,unsigned char side,unsigned char sector,int sectorsize,int mode,int nbsector,unsigned char * buffer,int buffer_size)
{
	FDCCTRL * fdcctrl;
	unsigned char cnt;

	fdcctrl = hxcfe_initFDC (floppycontext);
	if( fdcctrl )
	{
		hxcfe_insertDiskFDC (fdcctrl,fp);

		cnt = hxcfe_writeSectorFDC (fdcctrl,track,side,sector,sectorsize,mode,nbsector,buffer,buffer_size);

		hxcfe_deinitFDC (fdcctrl);
	}

	return cnt;
}

int hxcfe_FDC_FORMAT(HXCFLOPPYEMULATOR* floppycontext,unsigned char track,unsigned char side,unsigned char nbsector,int sectorsize,int sectoridstart,int skew,int interleave,int mode)
{
	//TODO
	return 0;
}

int hxcfe_FDC_SCANSECTOR (HXCFLOPPYEMULATOR* floppycontext,unsigned char track,unsigned char side,int mode,unsigned char * sector,unsigned char * buffer,int buffer_size)
{
	//TODO
	return 0;
}
