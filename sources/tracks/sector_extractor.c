/*
//
// Copyright (C) 2006-2014 Jean-François DEL NERO
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

#include "types.h"

#include "internal_libhxcfe.h"
#include "tracks/track_generator.h"
#include "sector_search.h"
#include "fdc_ctrl.h"

#include "libhxcfe.h"

#include "sector_extractor.h"
#include "./tracks/crc.h"

#include "trackutils.h"

#include "apple2.h"
#include "arburg_track.h"

extern unsigned char bit_inverter_emuii[];
extern unsigned char even_tab[];
extern unsigned char odd_tab[];
extern unsigned short MFM_tab[];

#define LOOKFOR_GAP1 0x01
#define LOOKFOR_ADDM 0x02
#define ENDOFTRACK 0x03
#define EXTRACTSECTORINFO 0x04
#define ENDOFSECTOR 0x05

unsigned short sectorsize[]={128,256,512,1024,2048,4096,8192,16384};

void checkEmptySector(HXCFE_SECTCFG * sector)
{
	int k,sector_size;
	unsigned char c;

	sector_size = sector->sectorsize;
	c = sector->input_data[0];
	k = 0;
	while( ( k < sector_size ) && ( c == sector->input_data[k] ) )
	{
		k++;
	};

	if( k == sector_size )
	{
		sector->fill_byte = c;
		sector->fill_byte_used = 0xFF;
	}
}

int get_next_MFM_sector(HXCFE* floppycontext,HXCFE_SIDE * track,HXCFE_SECTCFG * sector,int track_offset)
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

	memset(sector,0,sizeof(HXCFE_SECTCFG));

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

				bit_offset = searchBitStream(track->databuffer,track->tracklen,-1,mfm_buffer,6*8,bit_offset);

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

					if(track->timingbuffer)
						sector->bitrate = track->timingbuffer[bit_offset/8];
					else
						sector->bitrate = track->bitrate;

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
					bit_offset = searchBitStream(track->databuffer,track->tracklen,(88+16)*8,mfm_buffer,6*8,bit_offset);

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
							floppycontext->hxc_printf(MSG_DEBUG,"CRC data ok.");
						}
						else
						{
							floppycontext->hxc_printf(MSG_DEBUG,"CRC data error!");
							sector->use_alternate_data_crc=0xFF;
						}

						sector->input_data=(unsigned char*)malloc(sector_size);
						memcpy(sector->input_data,&tmp_sector[4],sector_size);
						free(tmp_sector);

						// "Empty" sector detection
						checkEmptySector(sector);

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
					if( ( (tmp_buffer[3]&0xFC)==0xF8) && (bit_offset > (88 * 8)) )
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

int get_next_MEMBRAIN_sector(HXCFE* floppycontext,HXCFE_SIDE * track,HXCFE_SECTCFG * sector,int track_offset)
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

	memset(sector,0,sizeof(HXCFE_SECTCFG));

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

				bit_offset = searchBitStream(track->databuffer,track->tracklen,-1,mfm_buffer,4*8,bit_offset);

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

					if(track->timingbuffer)
						sector->bitrate = track->timingbuffer[bit_offset/8];
					else
						sector->bitrate = track->bitrate;

					if(!CRC16_High && !CRC16_Low)
					{ // crc ok !!!
 						floppycontext->hxc_printf(MSG_DEBUG,"Valid MEMBRAIN MFM sector header found - Cyl:%d Side:%d Sect:%d Size:%d",tmp_buffer[4],tmp_buffer[5],tmp_buffer[6],sectorsize[tmp_buffer[7]&0x7]);
						sector->use_alternate_header_crc = 0;
					}
					else
					{
						floppycontext->hxc_printf(MSG_DEBUG,"Bad MEMBRAIN MFM sector header found - Cyl:%d Side:%d Sect:%d Size:%d",tmp_buffer[4],tmp_buffer[5],tmp_buffer[6],sectorsize[tmp_buffer[7]&0x7]);
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
					bit_offset = searchBitStream(track->databuffer,track->tracklen,(88+16)*8,mfm_buffer,4*8,bit_offset);

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

						sector->data_crc = ( tmp_sector[k-2]<<8 ) | tmp_sector[k-1] ;

						if(!CRC16_High && !CRC16_Low)
						{ // crc ok !!!
							floppycontext->hxc_printf(MSG_DEBUG,"CRC data ok.");
						}
						else
						{
							floppycontext->hxc_printf(MSG_DEBUG,"CRC data error!");
							sector->use_alternate_data_crc=0xFF;
						}

						sector->input_data=(unsigned char*)malloc(sector_size);
						memcpy(sector->input_data,&tmp_sector[2],sector_size);
						free(tmp_sector);

						// "Empty" sector detection
						checkEmptySector(sector);

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


int get_next_AMIGAMFM_sector(HXCFE* floppycontext,HXCFE_SIDE * track,HXCFE_SECTCFG * sector_conf,int track_offset)
{
	int bit_offset,old_bit_offset;
	int start_sector_bit_offset;
	int sector_size;

	unsigned char   header[4];
	unsigned char   headerparity[2];
	unsigned char   sectorparity[2];

	unsigned char mfm_buffer[32];
	unsigned char tmp_buffer[32];
	unsigned char sector_data[544];
	unsigned char temp_sector[512];
	int sector_extractor_sm,i;

	memset(sector_conf,0,sizeof(HXCFE_SECTCFG));

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

				bit_offset = searchBitStream(track->databuffer,track->tracklen,-1,mfm_buffer,6*8,bit_offset);

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

				start_sector_bit_offset = bit_offset;

				bit_offset = chgbitptr(track->tracklen,bit_offset,-(8*2));

				sector_conf->startdataindex = mfmtobin(track->databuffer,track->tracklen,sector_data,32,bit_offset,0);

				sector_conf->endsectorindex = mfmtobin(track->databuffer,track->tracklen,sector_data,544,bit_offset,0);

				if(track->timingbuffer)
					sector_conf->bitrate = track->timingbuffer[bit_offset/8];
				else
					sector_conf->bitrate = track->bitrate;

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
					sector_conf->startsectorindex = start_sector_bit_offset;

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

					// "Empty" sector detection
					checkEmptySector(sector_conf);

					bit_offset = chgbitptr(track->tracklen,bit_offset,(8*2)+1);

					sector_extractor_sm=ENDOFSECTOR;

				}
				else
				{
					bit_offset = chgbitptr(track->tracklen,bit_offset,(8*2)+1);
					if(!bit_offset)
						sector_extractor_sm = ENDOFTRACK;
					else
						sector_extractor_sm = LOOKFOR_GAP1;

					sector_conf->use_alternate_header_crc = 0xFF;

					sector_conf->endsectorindex = sector_conf->startdataindex;
					
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

int get_next_FM_sector(HXCFE* floppycontext,HXCFE_SIDE * track,HXCFE_SECTCFG * sector,int track_offset)
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
	memset(sector,0,sizeof(HXCFE_SECTCFG));

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

				bit_offset = searchBitStream(track->databuffer,track->tracklen,-1,fm_buffer,4*8,bit_offset);

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

					if(track->timingbuffer)
						sector->bitrate = track->timingbuffer[bit_offset/8];
					else
						sector->bitrate = track->bitrate;

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

						bit_offset = chgbitptr(track->tracklen,bit_offset,7 * 8);

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
							bit_offset = searchBitStream(track->databuffer,track->tracklen,((88+16)*8*2),fm_buffer,4*8,old_bit_offset);
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

							sector->data_crc = ( tmp_sector[k-2]<<8 ) | tmp_sector[k-1] ;

							if(!CRC16_High && !CRC16_Low)
							{ // crc ok !!!
								floppycontext->hxc_printf(MSG_DEBUG,"CRC data ok.");
							}
							else
							{
								floppycontext->hxc_printf(MSG_DEBUG,"CRC data error!");
								sector->use_alternate_data_crc=0xFF;
							}

							sector->input_data=(unsigned char*)malloc(sector_size);
							memcpy(sector->input_data,&tmp_sector[1],sector_size);
							free(tmp_sector);

							// "Empty" sector detection
							checkEmptySector(sector);

							bit_offset = chgbitptr(track->tracklen,bit_offset,1);

							sector_extractor_sm=ENDOFSECTOR;

						}
						else
						{
							bit_offset = chgbitptr(track->tracklen,old_bit_offset,1);

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


int get_next_TYCOMFM_sector(HXCFE* floppycontext,HXCFE_SIDE * track,HXCFE_SECTCFG * sector,int track_offset)
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
	memset(sector,0,sizeof(HXCFE_SECTCFG));

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

				bit_offset = searchBitStream(track->databuffer,track->tracklen,-1,fm_buffer,4*8,bit_offset);

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

						bit_offset = chgbitptr(track->tracklen,bit_offset, 5 * 8 );

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

						if(track->timingbuffer)
							sector->bitrate = track->timingbuffer[bit_offset/8];
						else
							sector->bitrate = track->bitrate;

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
							bit_offset = searchBitStream(track->databuffer,track->tracklen,((88+16)*8*2),fm_buffer,4*8,old_bit_offset);
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

							sector->data_crc = ( tmp_sector[k-2]<<8 ) | tmp_sector[k-1] ;

							if(!CRC16_High && !CRC16_Low)
							{ // crc ok !!!
								floppycontext->hxc_printf(MSG_DEBUG,"CRC data ok.");
							}
							else
							{
								floppycontext->hxc_printf(MSG_DEBUG,"CRC data error!");
								sector->use_alternate_data_crc=0xFF;
							}

							sector->input_data=(unsigned char*)malloc(sector_size);
							memcpy(sector->input_data,&tmp_sector[1],sector_size);
							free(tmp_sector);

							// "Empty" sector detection
							checkEmptySector(sector);

							bit_offset = chgbitptr(track->tracklen,bit_offset, 1);

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

int get_next_EMU_sector(HXCFE* floppycontext,HXCFE_SIDE * track,HXCFE_SECTCFG * sector,int track_offset)
{

	int bit_offset,old_bit_offset;
	int sector_size;
	unsigned char fm_buffer[32];
	unsigned char tmp_buffer[32];
	unsigned char CRC16_High;
	unsigned char CRC16_Low;
	int sector_extractor_sm;
	int k;
	unsigned char crctable[32];

	bit_offset=track_offset;
	memset(sector,0,sizeof(HXCFE_SECTCFG));

	sector_extractor_sm=LOOKFOR_GAP1;

	do
	{
		switch(sector_extractor_sm)
		{
			case LOOKFOR_GAP1:
				fm_buffer[0]=0x45;
				fm_buffer[1]=0x45;
				fm_buffer[2]=0x55;
				fm_buffer[3]=0x55;
				fm_buffer[4]=0x45;
				fm_buffer[5]=0x54;
				fm_buffer[6]=0x54;
				fm_buffer[7]=0x45;

				bit_offset = searchBitStream(track->databuffer,track->tracklen,-1,fm_buffer,8*8,bit_offset);

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

				sector->endsectorindex = fmtobin(track->databuffer,track->tracklen,tmp_buffer,5,bit_offset,0);
				if((bit_inverter_emuii[tmp_buffer[0]]==0xFA) && (bit_inverter_emuii[tmp_buffer[1]]==0x96))
				{
					sector->startsectorindex = bit_offset;
					sector->startdataindex = sector->endsectorindex;

					sector->use_alternate_addressmark = 0x00;
					sector->alternate_addressmark = 0x00;

					sector->use_alternate_datamark = 0x00;
					sector->alternate_datamark = 0x00;

					if(track->timingbuffer)
						sector->bitrate = track->timingbuffer[bit_offset/8];
					else
						sector->bitrate = track->bitrate;

					sector->use_alternate_header_crc = 0xFF;
					CRC16_Init(&CRC16_High,&CRC16_Low,(unsigned char*)crctable,0x8005,0x0000);
					for(k=0;k<3;k++)
					{
						CRC16_Update(&CRC16_High,&CRC16_Low, tmp_buffer[2+k],(unsigned char*)crctable );
					}

					sector->header_crc = ( tmp_buffer[k-2]<<8 ) | tmp_buffer[k-1] ;

					if(!CRC16_High && !CRC16_Low)
					{ // crc ok !!!

						sector->use_alternate_header_crc = 0x00;
						floppycontext->hxc_printf(MSG_DEBUG,"Valid EmuII FM sector header found - Sect:%d",bit_inverter_emuii[tmp_buffer[2]]);
						old_bit_offset=bit_offset;

						//11111011
						fm_buffer[0]=0x45;
						fm_buffer[1]=0x45;
						fm_buffer[2]=0x55;
						fm_buffer[3]=0x55;
						fm_buffer[4]=0x45;
						fm_buffer[5]=0x54;
						fm_buffer[6]=0x54;
						fm_buffer[7]=0x45;

						bit_offset = searchBitStream(track->databuffer,track->tracklen,-1,fm_buffer,8*8,bit_offset+(4*8*4));

						if((bit_offset-old_bit_offset<((88+16)*8*2)) && bit_offset!=-1)
						{

							sector->cylinder = bit_inverter_emuii[tmp_buffer[2]]>>1;
							sector->head = bit_inverter_emuii[tmp_buffer[2]]&1;
							sector->sector = 1;
							sector_size = 0xE00;
							sector->sectorsize = sector_size;
							sector->trackencoding = EMUFORMAT_SD;

							sector->use_alternate_datamark = 0x00;
							sector->alternate_datamark = 0x00;

							sector->startdataindex=bit_offset;

							sector->input_data =(unsigned char*)malloc(sector_size+2);
							if(sector->input_data)
							{
								memset(sector->input_data,0,sector_size+2);

								sector->endsectorindex = fmtobin(track->databuffer,track->tracklen,sector->input_data,sector_size+2,bit_offset+(8 *8),0);

								CRC16_Init(&CRC16_High,&CRC16_Low,(unsigned char*)crctable,0x8005,0x0000);
								for(k=0;k<sector_size+2;k++)
								{
									CRC16_Update(&CRC16_High,&CRC16_Low, sector->input_data[k],(unsigned char*)crctable );
								}

								sector->data_crc = (sector->input_data[sector_size]<<8) | (sector->input_data[sector_size+1]);

								if(!CRC16_High && !CRC16_Low)
								{ // crc ok !!!
									floppycontext->hxc_printf(MSG_DEBUG,"CRC data ok.");
									sector->use_alternate_data_crc = 0x00;
								}
								else
								{
									floppycontext->hxc_printf(MSG_DEBUG,"CRC data error!");
									sector->use_alternate_data_crc = 0xFF;
								}

								for(k=0;k<sector_size;k++)
								{
									sector->input_data[k]=bit_inverter_emuii[sector->input_data[k]];
								}

								// "Empty" sector detection
								checkEmptySector(sector);
							}

							bit_offset=bit_offset+(sector_size*4);

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

int get_next_Arburg_sector(HXCFE* floppycontext,HXCFE_SIDE * track,HXCFE_SECTCFG * sector,int track_offset)
{
	/*
		Arburg Track format:
		Sync : 0xFF
		Data : 0x9FE Bytes
		Checksum : 1 Low Byte
		Checksum : 1 High Byte
		Sync? : OxDF Bytes (High bytes checksum)
	*/

	int bit_offset,k;
	int sector_size;
	unsigned char fm_buffer[32];
	uint16_t checksum;
	int sector_extractor_sm;

	bit_offset=track_offset;
	memset(sector,0,sizeof(HXCFE_SECTCFG));

	sector_extractor_sm=LOOKFOR_GAP1;

	do
	{
		switch(sector_extractor_sm)
		{
			case LOOKFOR_GAP1:
				fm_buffer[0] = 0x44;
				fm_buffer[1] = 0x44;
				fm_buffer[2] = 0x44;
				fm_buffer[3] = 0x44;
				fm_buffer[4] = 0x55;
				fm_buffer[5] = 0x55;
				fm_buffer[6] = 0x55;
				fm_buffer[7] = 0x55;

				bit_offset = searchBitStream(track->databuffer,track->tracklen,-1,fm_buffer,8*8,bit_offset);

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

				bit_offset = chgbitptr(track->tracklen,bit_offset,8*4);

				sector->use_alternate_header_crc = 0x00;

				sector->startsectorindex = bit_offset;
				sector->endsectorindex = chgbitptr(track->tracklen,bit_offset,8*4);
				sector->startdataindex = sector->endsectorindex;

				sector->use_alternate_addressmark = 0x00;
				sector->alternate_addressmark = 0x00;

				sector->use_alternate_datamark = 0x00;
				sector->alternate_datamark = 0x00;

				if(track->timingbuffer)
					sector->bitrate = track->timingbuffer[bit_offset/8];
				else
					sector->bitrate = track->bitrate;

				sector->header_crc = 0x0000 ;

				sector->cylinder = 0x00;
				sector->head = 0x00;
				sector->sector = 1;
				sector_size = ARBURB_DATATRACK_SIZE + 2;
				sector->sectorsize = ARBURB_DATATRACK_SIZE;
				sector->trackencoding = ARBURG_DAT;

				sector->use_alternate_datamark = 0x00;
				sector->alternate_datamark = 0x00;

				sector->input_data =(unsigned char*)malloc(sector_size+2);

				if(sector->input_data)
				{
					memset(sector->input_data,0,sector_size+2);

					sector->endsectorindex = fmtobin(track->databuffer,track->tracklen,sector->input_data,sector_size,bit_offset,0);

					sector->data_crc = 0x0000;
					sector->use_alternate_data_crc = 0x00;

					for(k=0;k< 1 + 0x9FE + 2;k++)
					{
						sector->input_data[k]=bit_inverter_emuii[sector->input_data[k + 1]];
					}

					checksum = 0;
					for(k=0;k < 0x9FE;k++)
					{
						checksum = (uint16_t)(checksum + sector->input_data[k]);
					}

					if( ((checksum & 0xFF) == sector->input_data[0x9FE]) &&  (((checksum>>8) & 0xFF) == sector->input_data[0x9FF]) )
					{
						floppycontext->hxc_printf(MSG_DEBUG,"Checksum data ok.");
						sector->use_alternate_data_crc = 0x00;
						sector->data_crc = sector->input_data[0x9FE];
						sector->data_crc = sector->data_crc | (sector->input_data[0x9FF]<<8);
					}
					else
					{
						floppycontext->hxc_printf(MSG_DEBUG,"Checksum data Error !");
						sector->use_alternate_data_crc = 0xFF;
						sector->data_crc = sector->input_data[0x9FE];
						sector->data_crc = sector->data_crc | (sector->input_data[0x9FF]<<8);
					}

					// "Empty" sector detection
					checkEmptySector(sector);
				}

				bit_offset = track->tracklen;

				sector_extractor_sm = ENDOFSECTOR;
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

int get_next_ArburgSyst_sector(HXCFE* floppycontext,HXCFE_SIDE * track,HXCFE_SECTCFG * sector,int track_offset)
{
	/*
		Arburg Track format:
		Sync : 0xFF
		Data : 0xEFE Bytes
		Checksum : 1 Low Byte
		Checksum : 1 High Byte
		Sync? : OxDF Bytes (High bytes checksum)
	*/

	int bit_offset,k;
	int sector_size;
	unsigned char fm_buffer[32];
	uint16_t checksum;
	int sector_extractor_sm;

	bit_offset=track_offset;
	memset(sector,0,sizeof(HXCFE_SECTCFG));

	sector_extractor_sm=LOOKFOR_GAP1;

	do
	{
		switch(sector_extractor_sm)
		{
			case LOOKFOR_GAP1:
				fm_buffer[0] = 0x55;
				fm_buffer[1] = 0x55;
				fm_buffer[2] = 0x55;
				fm_buffer[3] = 0x55;
				fm_buffer[4] = 0x55;
				fm_buffer[5] = 0x24;
				fm_buffer[6] = 0x92;
				fm_buffer[7] = 0x49;

				bit_offset = searchBitStream(track->databuffer,track->tracklen,-1,fm_buffer,8*8,bit_offset);

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

				bit_offset = chgbitptr(track->tracklen,bit_offset,8*5);

				sector->use_alternate_header_crc = 0x00;

				sector->startsectorindex = bit_offset;
				sector->endsectorindex = chgbitptr(track->tracklen,bit_offset,8*5);
				sector->startdataindex = sector->endsectorindex;

				sector->use_alternate_addressmark = 0x00;
				sector->alternate_addressmark = 0x00;

				sector->use_alternate_datamark = 0x00;
				sector->alternate_datamark = 0x00;

				if(track->timingbuffer)
					sector->bitrate = track->timingbuffer[bit_offset/8];
				else
					sector->bitrate = track->bitrate;

				sector->header_crc = 0x0000 ;

				sector->cylinder = 0x00;
				sector->head = 0x00;
				sector->sector = 1;
				sector_size = ARBURB_SYSTEMTRACK_SIZE + 2;
				sector->sectorsize = ARBURB_SYSTEMTRACK_SIZE;
				sector->trackencoding = ARBURG_SYS;

				sector->use_alternate_datamark = 0x00;
				sector->alternate_datamark = 0x00;

				sector->input_data =(unsigned char*)malloc(sector_size+2);

				if(sector->input_data)
				{
					memset(sector->input_data,0,sector_size+2);

					sector->endsectorindex = arburgsysfmtobin(track->databuffer,track->tracklen,sector->input_data,sector_size,bit_offset,0);

					sector->data_crc = 0x0000;
					sector->use_alternate_data_crc = 0x00;

					for(k=0;k< 1 + 0xEFE + 2;k++)
					{
						sector->input_data[k]=bit_inverter_emuii[sector->input_data[k + 1]];
					}

					checksum = 0;
					for(k=0;k < 0xEFE;k++)
					{
						checksum = (uint16_t)(checksum + sector->input_data[k]);
					}

					if( ((checksum & 0xFF) == sector->input_data[0xEFE]) &&  (((checksum>>8) & 0xFF) == sector->input_data[0xEFF]) )
					{
						floppycontext->hxc_printf(MSG_DEBUG,"Checksum data ok.");
						sector->use_alternate_data_crc = 0x00;
						sector->data_crc = sector->input_data[0xEFE];
						sector->data_crc = sector->data_crc | (sector->input_data[0xEFF]<<8);
					}
					else
					{
						floppycontext->hxc_printf(MSG_DEBUG,"Checksum data Error !");
						sector->use_alternate_data_crc = 0xFF;
						sector->data_crc = sector->input_data[0xEFE];
						sector->data_crc = sector->data_crc | (sector->input_data[0xEFF]<<8);
					}

					// "Empty" sector detection
					checkEmptySector(sector);

				}

				bit_offset = track->tracklen;

				sector_extractor_sm=ENDOFSECTOR;
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

int write_FM_sectordata(HXCFE* floppycontext,HXCFE_SIDE * track,HXCFE_SECTCFG * sector,unsigned char * buffer,int buffersize)
{
	int bit_offset,i;
	unsigned char CRC16_High;
	unsigned char CRC16_Low;
	unsigned char crctable[32];
	unsigned char temp;

	// Data CRC
	CRC16_Init(&CRC16_High,&CRC16_Low,(unsigned char*)crctable,0x1021,0xFFFF);

	CRC16_Update(&CRC16_High,&CRC16_Low, sector->alternate_datamark ,(unsigned char*)crctable );

	for(i=0;i<buffersize;i++)
	{
		CRC16_Update(&CRC16_High,&CRC16_Low, buffer[i],(unsigned char*)crctable );
	}

	bit_offset = sector->startdataindex;
	temp = (unsigned char)sector->alternate_datamark;
	bit_offset = bintofm(track->databuffer,track->tracklen,&temp,1,bit_offset);
	// Clear missing clocks
	setbit(track->databuffer,bit_offset-15,0);
	setbit(track->databuffer,bit_offset-19,0);
	setbit(track->databuffer,bit_offset-23,0);
	bit_offset = bintofm(track->databuffer,track->tracklen,buffer,buffersize,bit_offset);
	bit_offset = bintofm(track->databuffer,track->tracklen,&CRC16_High,1,bit_offset);
	bit_offset = bintofm(track->databuffer,track->tracklen,&CRC16_Low ,1,bit_offset);

	return 0;
}

int write_MFM_sectordata(HXCFE* floppycontext,HXCFE_SIDE * track,HXCFE_SECTCFG * sector,unsigned char * buffer,int buffersize)
{
	int bit_offset,i;
	unsigned char CRC16_High;
	unsigned char CRC16_Low;
	unsigned char temp;
	unsigned char crctable[32];

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
	temp = (unsigned char)sector->alternate_datamark;
	bit_offset = bintomfm(track->databuffer,track->tracklen,&temp,1,bit_offset);
	bit_offset = bintomfm(track->databuffer,track->tracklen,buffer,buffersize,bit_offset);
	bit_offset = bintomfm(track->databuffer,track->tracklen,&CRC16_High,1,bit_offset);
	bit_offset = bintomfm(track->databuffer,track->tracklen,&CRC16_Low ,1,bit_offset);
	temp = 0x4E;
	bit_offset = bintomfm(track->databuffer,track->tracklen,&temp ,1,bit_offset);

	return 0;
}

int write_AMIGAMFM_sectordata(HXCFE* floppycontext,HXCFE_SIDE * track,HXCFE_SECTCFG * sector,unsigned char * buffer,int buffersize)
{
	int bit_offset;
	unsigned char sectorparity[2];
	int i,l;
	uint8_t  byte;
	uint16_t lastbit,mfm_code;
	unsigned char  *mfm_buffer;
	track_generator tg;


	sectorparity[0]=0;
	sectorparity[1]=0;

	for(i=0;i<sector->sectorsize;i=i+4)
	{
		sectorparity[0]^=(odd_tab[buffer[i]]<<4) | odd_tab[buffer[i+1]];
		sectorparity[1]^=(odd_tab[buffer[i+2]]<<4) | odd_tab[buffer[i+3]];
	}

	for(i=0;i<sector->sectorsize;i=i+4)
	{
		sectorparity[0]^=(even_tab[buffer[i]]<<4) | even_tab[buffer[i+1]];
		sectorparity[1]^=(even_tab[buffer[i+2]]<<4) | even_tab[buffer[i+3]];
	}

	bit_offset = sector->startdataindex;

	bit_offset = bit_offset - (((3*8)*2));

	tg_initTrackEncoder(&tg);

	tg.last_bit_offset = bit_offset;

	pushTrackCode(&tg,0x00,0xFF,track,sector->trackencoding);
	pushTrackCode(&tg,(unsigned char)sectorparity[0],0xFF,track,sector->trackencoding);
	pushTrackCode(&tg,(unsigned char)sectorparity[1],0xFF,track,sector->trackencoding);

	bit_offset = tg.last_bit_offset;

	mfm_buffer = &track->databuffer[bit_offset/8];

	// MFM Encoding
	lastbit = tg.mfm_last_bit;
	i=0;

	for(l=0;l<buffersize;l=l+2)
	{
		byte = (uint8_t)((odd_tab[buffer[l]]<<4) | odd_tab[buffer[l+1]]);
		mfm_code = (uint16_t)(MFM_tab[byte] & lastbit);

		mfm_buffer[i++] = (uint8_t)(mfm_code>>8);
		mfm_buffer[i++] = (uint8_t)(mfm_code&0xFF);

		lastbit = (uint16_t)(~(MFM_tab[byte]<<15));
	}

	for(l=0;l<buffersize;l=l+2)
	{
		byte = (uint8_t)((even_tab[buffer[l]]<<4) | even_tab[buffer[l+1]]);
		mfm_code = (uint16_t)(MFM_tab[byte] & lastbit);

		mfm_buffer[i++] = (uint8_t)(mfm_code>>8);
		mfm_buffer[i++] = (uint8_t)(mfm_code&0xFF);

		lastbit = (uint16_t)(~(MFM_tab[byte]<<15));
	}

	tg.mfm_last_bit = lastbit;
	tg.last_bit_offset = tg.last_bit_offset + ( buffersize * 2 * 8 );
	pushTrackCode(&tg,0x00,0xFF,track,sector->trackencoding);

	return 0;
}

int analysis_and_extract_sector_EMUIIFM(HXCFE* floppycontext,HXCFE_SIDE * track,sect_track * sectors)
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


				bit_offset = searchBitStream(track->databuffer,track->tracklen,-1,fm_buffer,8*8,bit_offset);

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

						bit_offset = searchBitStream(track->databuffer,track->tracklen,-1,fm_buffer,8*8,bit_offset+(4*8*4));

						if((bit_offset-old_bit_offset<((88+16)*8*2)) && bit_offset!=-1)
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
								floppycontext->hxc_printf(MSG_DEBUG,"CRC data ok.");
							}
							else
							{
								floppycontext->hxc_printf(MSG_DEBUG,"CRC data error!");
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

HXCFE_SECTORACCESS* hxcfe_initSectorAccess(HXCFE* floppycontext,HXCFE_FLOPPY *fp)
{
	HXCFE_SECTORACCESS* ss_ctx;
	int i;

	ss_ctx = (HXCFE_SECTORACCESS*) malloc(sizeof(HXCFE_SECTORACCESS));
	memset(ss_ctx,0,sizeof(HXCFE_SECTORACCESS));

	ss_ctx->fp = fp;
	ss_ctx->bitoffset = 0;
	ss_ctx->old_bitoffset = 0;
	ss_ctx->cur_side = 0;
	ss_ctx->cur_track = 0;
	ss_ctx->hxcfe = floppycontext;

	if(fp->floppyNumberOfTrack)
	{
		ss_ctx->track_cache = malloc(sizeof(SECTORSEARCHTRACKCACHE) * fp->floppyNumberOfTrack * 2);
		if(ss_ctx->track_cache)
		{
			for(i=0;i<fp->floppyNumberOfTrack * 2;i++)
			{
				ss_ctx->track_cache[i].nb_sector_cached = 0;
			}
		}
	}

	return ss_ctx;
}

HXCFE_SECTCFG* hxcfe_getNextSector( HXCFE_SECTORACCESS* ss_ctx, int32_t track, int32_t side, int32_t type )
{
	HXCFE_SECTCFG * sc;
	SECTORSEARCHTRACKCACHE * trackcache;
	int bitoffset,tmp_bitoffset;
	int i;

	if((ss_ctx->bitoffset == -1) || (ss_ctx->cur_side != side) || (ss_ctx->cur_track != track))
	{
		bitoffset = 0;
	}
	else
		bitoffset = ss_ctx->bitoffset;

	ss_ctx->cur_track = track;
	ss_ctx->cur_side = side;

	if(!ss_ctx->fp)
		return 0;

	if(track >= ss_ctx->fp->floppyNumberOfTrack)
		return 0;

	if(!ss_ctx->fp->tracks[track])
		return 0;

	if(side >= ss_ctx->fp->tracks[track]->number_of_side)
		return 0;

	if(!ss_ctx->fp->tracks[track]->sides[side])
		return 0;

	// end of track already reached
	if( ss_ctx->old_bitoffset > ss_ctx->bitoffset )
		return 0;

	ss_ctx->old_bitoffset = ss_ctx->bitoffset;

	sc=(HXCFE_SECTCFG *) malloc(sizeof(HXCFE_SECTCFG));

	tmp_bitoffset = bitoffset;
	switch(type)
	{
		case ISOIBM_MFM_ENCODING:
			bitoffset = get_next_MFM_sector(ss_ctx->hxcfe,ss_ctx->fp->tracks[track]->sides[side],sc,bitoffset);
		break;
		case AMIGA_MFM_ENCODING:
			bitoffset = get_next_AMIGAMFM_sector(ss_ctx->hxcfe,ss_ctx->fp->tracks[track]->sides[side],sc,bitoffset);
		break;
		case ISOIBM_FM_ENCODING:
			bitoffset = get_next_FM_sector(ss_ctx->hxcfe,ss_ctx->fp->tracks[track]->sides[side],sc,bitoffset);
		break;
		case TYCOM_FM_ENCODING:
			bitoffset = get_next_TYCOMFM_sector(ss_ctx->hxcfe,ss_ctx->fp->tracks[track]->sides[side],sc,bitoffset);
		break;
		case MEMBRAIN_MFM_ENCODING:
			bitoffset = get_next_MEMBRAIN_sector(ss_ctx->hxcfe,ss_ctx->fp->tracks[track]->sides[side],sc,bitoffset);
		break;
		case EMU_FM_ENCODING:
			bitoffset = get_next_EMU_sector(ss_ctx->hxcfe,ss_ctx->fp->tracks[track]->sides[side],sc,bitoffset);
		break;
		case APPLEII_GCR1_ENCODING:
			bitoffset = get_next_A2GCR1_sector(ss_ctx->hxcfe,ss_ctx->fp->tracks[track]->sides[side],sc,bitoffset);
		break;
		case APPLEII_GCR2_ENCODING:
			bitoffset = get_next_A2GCR2_sector(ss_ctx->hxcfe,ss_ctx->fp->tracks[track]->sides[side],sc,bitoffset);
		break;
		case ARBURGDAT_ENCODING:
			bitoffset = get_next_Arburg_sector(ss_ctx->hxcfe,ss_ctx->fp->tracks[track]->sides[side],sc,bitoffset);
		break;
		case ARBURGSYS_ENCODING:
			bitoffset = get_next_ArburgSyst_sector(ss_ctx->hxcfe,ss_ctx->fp->tracks[track]->sides[side],sc,bitoffset);
		break;
		default:
			bitoffset=-1;
		break;
	}


	if(bitoffset == tmp_bitoffset)
	{
		ss_ctx->bitoffset = -1;
		free(sc);
		return 0;
	}

	ss_ctx->bitoffset = bitoffset;

	if(track<ss_ctx->fp->floppyNumberOfTrack && ss_ctx->track_cache)
	{
		trackcache = &ss_ctx->track_cache[(track<<1) | (side&1)];
		if(trackcache->nb_sector_cached<512 && bitoffset>=0)
		{
			//Add a new cache entry
			i=0;
			while(i<trackcache->nb_sector_cached && ( trackcache->sectorcache[i].startsectorindex != sc->startsectorindex) && i<512)
			{
				i++;
			}

			if( i<512 )
			{
				if(i == trackcache->nb_sector_cached)
				{
					memcpy(&(trackcache->sectorcache[i]),sc,sizeof(HXCFE_SECTCFG));
					trackcache->sectorcache[i].input_data = 0;
					trackcache->nb_sector_cached++;
				}
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

void hxcfe_resetSearchTrackPosition(HXCFE_SECTORACCESS* ss_ctx)
{
	if(ss_ctx)
	{
		ss_ctx->bitoffset = 0;
		ss_ctx->old_bitoffset = 0;
	}
}

HXCFE_SECTCFG** hxcfe_getAllTrackSectors( HXCFE_SECTORACCESS* ss_ctx, int32_t track, int32_t side, int32_t type, int32_t * nb_sectorfound )
{
	int i;
	HXCFE_SECTCFG * sc;
	HXCFE_SECTCFG ** scarray;
	int nb_of_sector;

	nb_of_sector = 0;
	// First : Count the number of sectors
	hxcfe_resetSearchTrackPosition(ss_ctx);
	do
	{
		sc = hxcfe_getNextSector(ss_ctx,track,side,type);
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

	hxcfe_resetSearchTrackPosition(ss_ctx);
	scarray = 0;
	if(nb_of_sector)
	{
		scarray = malloc(sizeof(HXCFE_SECTCFG *) * (nb_of_sector+1));
		if(scarray)
		{
			memset(scarray,0,sizeof(HXCFE_SECTCFG *) * (nb_of_sector+1));
			for(i=0;i<nb_of_sector;i++)
			{
				sc = hxcfe_getNextSector(ss_ctx,track,side,type);
				scarray[i] = sc;
			}
		}
	}

	hxcfe_resetSearchTrackPosition(ss_ctx);

	return scarray;
}

HXCFE_SECTCFG** hxcfe_getAllTrackISOSectors( HXCFE_SECTORACCESS* ss_ctx, int32_t track, int32_t side, int32_t * nb_sectorfound )
{
	int i,i_fm,i_mfm;
	HXCFE_SECTCFG * sc;

	HXCFE_SECTCFG ** sc_fm_array;
	HXCFE_SECTCFG ** sc_mfm_array;

	HXCFE_SECTCFG ** scarray;
	int nb_of_sector;

	nb_of_sector = 0;
	// First : Count the number of sectors
	hxcfe_resetSearchTrackPosition(ss_ctx);
	do
	{
		sc = hxcfe_getNextSector(ss_ctx,track,side,ISOIBM_MFM_ENCODING);
		if(sc)
		{
			if(sc->input_data)
				free(sc->input_data);
			free(sc);

			nb_of_sector++;
		}
	}while(sc);

	// FM
	hxcfe_resetSearchTrackPosition(ss_ctx);
	do
	{
		sc = hxcfe_getNextSector(ss_ctx,track,side,ISOIBM_FM_ENCODING);
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
		sc_mfm_array = malloc(sizeof(HXCFE_SECTCFG *) * (nb_of_sector+1));
		sc_fm_array  = malloc(sizeof(HXCFE_SECTCFG *) * (nb_of_sector+1));
		scarray = malloc(sizeof(HXCFE_SECTCFG *) * (nb_of_sector+1));
		if(scarray && sc_mfm_array && sc_fm_array)
		{
			memset(scarray     ,0,sizeof(HXCFE_SECTCFG *) * (nb_of_sector+1));
			memset(sc_mfm_array,0,sizeof(HXCFE_SECTCFG *) * (nb_of_sector+1));
			memset(sc_fm_array ,0,sizeof(HXCFE_SECTCFG *) * (nb_of_sector+1));

			hxcfe_resetSearchTrackPosition(ss_ctx);
			i = 0;
			do
			{
				sc = hxcfe_getNextSector(ss_ctx,track,side,ISOIBM_FM_ENCODING);
				sc_fm_array[i] = sc;
				i++;
			}while(sc);

			hxcfe_resetSearchTrackPosition(ss_ctx);
			i = 0;
			do
			{
				sc = hxcfe_getNextSector(ss_ctx,track,side,ISOIBM_MFM_ENCODING);
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
						scarray[i] = malloc(sizeof(HXCFE_SECTCFG));
						if(scarray[i])
						{
							memset(scarray[i],0,sizeof(HXCFE_SECTCFG));
							memcpy(scarray[i], sc_fm_array[i_fm], sizeof(HXCFE_SECTCFG));
							free(sc_fm_array[i_fm]);
						}
						i_fm++;
					}
					else
					{
						scarray[i] = malloc(sizeof(HXCFE_SECTCFG));
						if(scarray[i])
						{
							memset(scarray[i],0,sizeof(HXCFE_SECTCFG));
							memcpy(scarray[i], sc_mfm_array[i_mfm], sizeof(HXCFE_SECTCFG));
							free(sc_mfm_array[i_mfm]);
						}
						i_mfm++;
					}
				}
				else
				{
					if(sc_fm_array[i_fm])
					{
						scarray[i] = malloc(sizeof(HXCFE_SECTCFG));
						if(scarray[i])
						{
							memset(scarray[i],0,sizeof(HXCFE_SECTCFG));
							memcpy(scarray[i], sc_fm_array[i_fm], sizeof(HXCFE_SECTCFG));
							free(sc_fm_array[i_fm]);
						}
						i_fm++;
					}
					else
					{
						if(sc_mfm_array[i_mfm])
						{
							scarray[i] = malloc(sizeof(HXCFE_SECTCFG));
							if(scarray[i])
							{
								memset(scarray[i],0,sizeof(HXCFE_SECTCFG));
								memcpy(scarray[i], sc_mfm_array[i_mfm], sizeof(HXCFE_SECTCFG));
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

	hxcfe_resetSearchTrackPosition(ss_ctx);

	return scarray;
}


HXCFE_SECTCFG* hxcfe_searchSector ( HXCFE_SECTORACCESS* ss_ctx, int32_t track, int32_t side, int32_t id, int32_t type )
{
	HXCFE_SECTCFG * sc;
	SECTORSEARCHTRACKCACHE * trackcache;
	int i;

	if(track<ss_ctx->fp->floppyNumberOfTrack && ss_ctx->track_cache)
	{
		trackcache = &ss_ctx->track_cache[(track<<1) | (side&1)];

		// Search in the cache
		i = 0;
		while( i < trackcache->nb_sector_cached )
		{
			if((trackcache->sectorcache[i].sector == id) && (trackcache->sectorcache[i].cylinder == track) && (trackcache->sectorcache[i].head == side) )
			{
				ss_ctx->cur_side = side;
				ss_ctx->cur_track = track;
				ss_ctx->bitoffset = trackcache->sectorcache[i].startsectorindex;
				ss_ctx->old_bitoffset = ss_ctx->bitoffset;
				sc = hxcfe_getNextSector(ss_ctx,track,side,type);
				return sc;
			}
			i++;
		}

		if(trackcache->nb_sector_cached)
		{
			ss_ctx->bitoffset = trackcache->sectorcache[trackcache->nb_sector_cached-1].startdataindex+1;
			ss_ctx->cur_side = side;
			ss_ctx->cur_track = track;
			ss_ctx->old_bitoffset = ss_ctx->bitoffset;
		}
		else
		{
			hxcfe_resetSearchTrackPosition(ss_ctx);
		}
	}
	else
	{
		hxcfe_resetSearchTrackPosition(ss_ctx);

		if(track>=ss_ctx->fp->floppyNumberOfTrack)
		{
			return 0;
		}
	}

	do
	{
		sc = hxcfe_getNextSector(ss_ctx,track,side,type);

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

int32_t hxcfe_getSectorSize( HXCFE_SECTORACCESS* ss_ctx, HXCFE_SECTCFG* sc )
{
	return sc->sectorsize;
}

uint8_t * hxcfe_getSectorData(HXCFE_SECTORACCESS* ss_ctx,HXCFE_SECTCFG* sc)
{
	return sc->input_data;
}

int32_t hxcfe_getFloppySize( HXCFE* floppycontext, HXCFE_FLOPPY *fp, int32_t * nbsector )
{
	HXCFE_SECTORACCESS* ss_ctx;
	HXCFE_SECTCFG* sc;
	int floppysize;
	int nbofsector;
	int track;
	int side;
	int i,type,secfound,t;
	int typetab[16];

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
	typetab[i++]=APPLEII_GCR1_ENCODING;
	typetab[i++]=APPLEII_GCR2_ENCODING;
	typetab[i++]=-1;

	ss_ctx=hxcfe_initSectorAccess(floppycontext,fp);
	if(ss_ctx)
	{
		for(track=0;track<fp->floppyNumberOfTrack;track++)
		{
			for(side=0;side<fp->floppyNumberOfSide;side++)
			{
				secfound=0;
				type=0;

				while(typetab[type]!=-1 && !secfound)
				{
					hxcfe_resetSearchTrackPosition(ss_ctx);
					do
					{
						sc=hxcfe_getNextSector(ss_ctx,track,side,typetab[type]);
						if(sc)
						{
							floppysize=floppysize+sc->sectorsize;
							nbofsector++;
							secfound=1;

							hxcfe_freeSectorConfig(ss_ctx,sc);
						}

					}while(sc);

					if(!secfound)
					{
						hxcfe_resetSearchTrackPosition(ss_ctx);
						type++;
					}

				}

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

	hxcfe_deinitSectorAccess(ss_ctx);
	return floppysize;

}

int hxcfe_readSectorData(HXCFE_SECTORACCESS* ss_ctx,int track,int side,int sector,int numberofsector,int sectorsize,int type,unsigned char * buffer,int * fdcstatus)
{
	HXCFE_SECTCFG * sc;
	int nbsectorread;

	nbsectorread=0;

	if(fdcstatus)
		*fdcstatus = FDC_NOERROR;

	if ( side < ss_ctx->fp->floppyNumberOfSide && track < ss_ctx->fp->floppyNumberOfTrack )
	{
		do
		{
			sc = hxcfe_searchSector ( ss_ctx, track, side, sector + nbsectorread, type);
			if(sc)
			{
				if(sc->sectorsize == sectorsize)
				{
					if(sc->input_data)
					{
						memcpy(&buffer[sectorsize*(sc->sector-sector)],sc->input_data,sectorsize);
						free(sc->input_data);
						sc->input_data=0;
					}

					if(sc->use_alternate_data_crc)
					{
						if(fdcstatus)
							*fdcstatus = FDC_BAD_DATA_CRC;

						ss_ctx->hxcfe->hxc_printf(MSG_ERROR,"hxcfe_readSectorData : ERROR -> Bad Data CRC ! track %d, side %d, sector %d,Sector size:%d,Type:%x",track,side,sector+nbsectorread,sectorsize,type);
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
			else
			{
				if(fdcstatus)
					*fdcstatus = FDC_SECTOR_NOT_FOUND;

				ss_ctx->hxcfe->hxc_printf(MSG_ERROR,"hxcfe_readSectorData : ERROR -> Sector not found ! track %d, side %d, sector %d,Sector size:%d,Type:%x",track,side,sector+nbsectorread,sectorsize,type);
			}

		}while((nbsectorread<numberofsector) && sc);
	}

	return nbsectorread;
}

int hxcfe_writeSectorData(HXCFE_SECTORACCESS* ss_ctx,int track,int side,int sector,int numberofsector,int sectorsize,int type,unsigned char * buffer,int * fdcstatus)
{
	HXCFE_SECTCFG * sc;
	int nbsectorwrite;

	nbsectorwrite=0;

	if(fdcstatus)
		*fdcstatus = FDC_NOERROR;

	if ( side < ss_ctx->fp->floppyNumberOfSide && track < ss_ctx->fp->floppyNumberOfTrack )
	{
		do
		{
			sc = hxcfe_searchSector ( ss_ctx, track, side, sector + nbsectorwrite, type);
			if(sc)
			{
				if(((sc->sector>=sector) && (sc->sector < ( sector + numberofsector )) ) )
				{
					switch(type)
					{
						case ISOIBM_MFM_ENCODING:
							write_MFM_sectordata(ss_ctx->hxcfe,ss_ctx->fp->tracks[track]->sides[side],sc,&buffer[sectorsize*nbsectorwrite],sectorsize);
						break;
						case AMIGA_MFM_ENCODING:
							write_AMIGAMFM_sectordata(ss_ctx->hxcfe,ss_ctx->fp->tracks[track]->sides[side],sc,&buffer[sectorsize*nbsectorwrite],sectorsize);
						break;
						case TYCOM_FM_ENCODING:
						case ISOIBM_FM_ENCODING:
							write_FM_sectordata(ss_ctx->hxcfe,ss_ctx->fp->tracks[track]->sides[side],sc,&buffer[sectorsize*nbsectorwrite],sectorsize);
						break;
						case MEMBRAIN_MFM_ENCODING:
							//write_ALT01_MFM_sectordata(ss_ctx->hxcfe,ss_ctx->fp->tracks[track]->sides[side],sc,&buffer[sectorsize*nbsectorwrite],sectorsize);
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
			else
			{
				if(fdcstatus)
					*fdcstatus = FDC_SECTOR_NOT_FOUND;

				ss_ctx->hxcfe->hxc_printf(MSG_ERROR,"hxcfe_writeSectorData : ERROR -> Sector not found ! track %d, side %d, sector %d,Sector size:%d,Type:%x",track,side,sector+nbsectorwrite,sectorsize,type);
			}

		}while(( nbsectorwrite < numberofsector ) && sc);
	}

	return nbsectorwrite;

}


void hxcfe_freeSectorConfig(HXCFE_SECTORACCESS* ss_ctx,HXCFE_SECTCFG* sc)
{
	if(sc)
	{
		if(sc->input_data)
			free(sc->input_data);
		free(sc);
	}
}

int32_t hxcfe_getSectorConfigEncoding(HXCFE* floppycontext,HXCFE_SECTCFG* sc)
{
	if(sc)
	{
		return sc->trackencoding;
	}
	return 0;
}

int32_t hxcfe_getSectorConfigSectorID(HXCFE* floppycontext,HXCFE_SECTCFG* sc)
{
	if(sc)
	{
		return sc->sector;
	}
	return 0;
}

int32_t hxcfe_getSectorConfigDataMark(HXCFE* floppycontext,HXCFE_SECTCFG* sc)
{
	if(sc)
	{
		return sc->alternate_datamark;
	}
	return 0;
}

int32_t hxcfe_getSectorConfigSideID(HXCFE* floppycontext,HXCFE_SECTCFG* sc)
{
	if(sc)
	{
		return sc->head;
	}
	return 0;
}

int32_t hxcfe_getSectorConfigSizeID(HXCFE* floppycontext,HXCFE_SECTCFG* sc)
{
	if(sc)
	{
		return sc->alternate_sector_size_id;
	}
	return 0;
}

int32_t hxcfe_getSectorConfigTrackID(HXCFE* floppycontext,HXCFE_SECTCFG* sc)
{
	if(sc)
	{
		return sc->cylinder;
	}
	return 0;
}

uint32_t hxcfe_getSectorConfigHCRC(HXCFE* floppycontext,HXCFE_SECTCFG* sc)
{
	if(sc)
	{
		return sc->header_crc;
	}
	return 0;
}

uint32_t hxcfe_getSectorConfigDCRC(HXCFE* floppycontext,HXCFE_SECTCFG* sc)
{
	if(sc)
	{
		return sc->data_crc;
	}
	return 0;
}

int32_t hxcfe_getSectorConfigSectorSize(HXCFE* floppycontext,HXCFE_SECTCFG* sc)
{
	if(sc)
	{
		return sc->sectorsize;
	}
	return 0;
}

int32_t hxcfe_getSectorConfigStartSectorIndex(HXCFE* floppycontext,HXCFE_SECTCFG* sc)
{
	if(sc)
	{
		return sc->startsectorindex;
	}
	return 0;
}

int32_t hxcfe_getSectorConfigStartDataIndex(HXCFE* floppycontext,HXCFE_SECTCFG* sc)
{
	if(sc)
	{
		return sc->startdataindex;
	}
	return 0;
}

int32_t hxcfe_getSectorConfigEndSectorIndex(HXCFE* floppycontext,HXCFE_SECTCFG* sc)
{
	if(sc)
	{
		return sc->endsectorindex;
	}
	return 0;
}

uint8_t * hxcfe_getSectorConfigInputData(HXCFE* floppycontext,HXCFE_SECTCFG* sc)
{
	if(sc)
	{
		return sc->input_data;
	}
	return 0;
}

int32_t hxcfe_getSectorConfigHCRCStatus(HXCFE* floppycontext,HXCFE_SECTCFG* sc)
{
	if(sc)
	{
		return sc->use_alternate_header_crc;
	}
	return 0;
}

int32_t hxcfe_getSectorConfigDCRCStatus(HXCFE* floppycontext,HXCFE_SECTCFG* sc)
{
	if(sc)
	{
		return sc->use_alternate_data_crc;
	}
	return 0;
}

void hxcfe_deinitSectorAccess(HXCFE_SECTORACCESS* ss_ctx)
{
	if(ss_ctx)
	{
		if(ss_ctx->track_cache)
			free(ss_ctx->track_cache);
		free(ss_ctx);
	}
}

HXCFE_FDCCTRL * hxcfe_initFDC (HXCFE* floppycontext)
{
	HXCFE_FDCCTRL * fdc;

	fdc = malloc(sizeof(HXCFE_FDCCTRL));
	if( fdc )
	{
		memset(fdc,0,sizeof(HXCFE_FDCCTRL));
		fdc->floppycontext = floppycontext;
		return fdc;
	}

	return 0;
}

int hxcfe_insertDiskFDC (HXCFE_FDCCTRL * fdc,HXCFE_FLOPPY *fp)
{
	if(fdc)
	{
		fdc->loadedfp = fp;

		if( fdc->ss_ctx )
		{
			hxcfe_deinitSectorAccess(fdc->ss_ctx);
			fdc->ss_ctx = 0;
		}
		fdc->ss_ctx = hxcfe_initSectorAccess(fdc->floppycontext,fp);

		return HXCFE_NOERROR;
	}

	return HXCFE_BADPARAMETER;
}

int hxcfe_readSectorFDC (HXCFE_FDCCTRL * fdc,unsigned char track,unsigned char side,unsigned char sector,int sectorsize,int mode,int nbsector,unsigned char * buffer,int buffer_size,int * fdcstatus)
{
	if(fdc)
	{
		if(fdc->ss_ctx && fdc->loadedfp && ((sectorsize*nbsector)<=buffer_size))
			return hxcfe_readSectorData(fdc->ss_ctx,track,side,sector,nbsector,sectorsize,mode,buffer,fdcstatus);
		else
			return HXCFE_BADPARAMETER;
	}

	return HXCFE_BADPARAMETER;
}

int hxcfe_writeSectorFDC (HXCFE_FDCCTRL * fdc,unsigned char track,unsigned char side,unsigned char sector,int sectorsize,int mode,int nbsector,unsigned char * buffer,int buffer_size,int * fdcstatus)
{
	if(fdc)
	{
		if(fdc->ss_ctx && fdc->loadedfp && ((sectorsize*nbsector)<=buffer_size))
			return hxcfe_writeSectorData(fdc->ss_ctx,track,side,sector,nbsector,sectorsize,mode,buffer,fdcstatus);
		else
			return HXCFE_BADPARAMETER;
	}

	return HXCFE_BADPARAMETER;
}

void hxcfe_deinitFDC (HXCFE_FDCCTRL * fdc)
{
	if(fdc)
	{
		if(fdc->ss_ctx)
			hxcfe_deinitSectorAccess(fdc->ss_ctx);
		free(fdc);
	}
}

int hxcfe_FDC_READSECTOR (HXCFE* floppycontext,HXCFE_FLOPPY *fp,unsigned char track,unsigned char side,unsigned char sector,int sectorsize,int mode,int nbsector,unsigned char * buffer,int buffer_size,int * fdcstatus)
{
	HXCFE_FDCCTRL * fdcctrl;
	int cnt;

	cnt = 0;

	fdcctrl = hxcfe_initFDC (floppycontext);
	if( fdcctrl )
	{
		hxcfe_insertDiskFDC (fdcctrl,fp);

		cnt = hxcfe_readSectorFDC (fdcctrl,track,side,sector,sectorsize,mode,nbsector,buffer,buffer_size,fdcstatus);

		hxcfe_deinitFDC (fdcctrl);
	}

	return cnt;
}

int hxcfe_FDC_WRITESECTOR (HXCFE* floppycontext,HXCFE_FLOPPY *fp,unsigned char track,unsigned char side,unsigned char sector,int sectorsize,int mode,int nbsector,unsigned char * buffer,int buffer_size,int * fdcstatus)
{
	HXCFE_FDCCTRL * fdcctrl;
	int cnt;

	cnt = 0;

	fdcctrl = hxcfe_initFDC (floppycontext);
	if( fdcctrl )
	{
		hxcfe_insertDiskFDC (fdcctrl,fp);

		cnt = hxcfe_writeSectorFDC (fdcctrl,track,side,sector,sectorsize,mode,nbsector,buffer,buffer_size,fdcstatus);

		hxcfe_deinitFDC (fdcctrl);
	}

	return cnt;
}

int hxcfe_FDC_FORMAT(HXCFE* floppycontext,unsigned char track,unsigned char side,unsigned char nbsector,int sectorsize,int sectoridstart,int skew,int interleave,int mode,int * fdcstatus)
{
	//TODO
	return 0;
}

int hxcfe_FDC_SCANSECTOR (HXCFE* floppycontext,unsigned char track,unsigned char side,int mode,unsigned char * sector,unsigned char * buffer,int buffer_size,int * fdcstatus)
{
	//TODO
	return 0;
}
