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
// File : amiga_mfm_track.c
// Contains: Amiga track support
//
// Written by: Jean-François DEL NERO
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

#include "tracks/sector_extractor.h"
#include "tracks/crc.h"
#include "tracks/track_types_defs.h"

#include "amiga_mfm_track.h"

#include "tracks/trackutils.h"
#include "tracks/encoding/mfm_encoding.h"

#include "tracks/luts.h"

#include "sector_sm.h"

int get_next_AMIGAMFM_sector(HXCFE* floppycontext,HXCFE_SIDE * track,HXCFE_SECTCFG * sector_conf,int track_offset)
{
	int bit_offset;
	int last_start_offset;
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
					last_start_offset = bit_offset;
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

				sector_conf->startdataindex = mfmtobin(track->databuffer,NULL,track->tracklen,sector_data,32,bit_offset,0);

				sector_conf->endsectorindex = mfmtobin(track->databuffer,NULL,track->tracklen,sector_data,544,bit_offset,0);

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

					headerparity[0]^=( LUT_Byte2OddBits[tmp_buffer[0]]<<4)|( LUT_Byte2OddBits[tmp_buffer[1]]);
					headerparity[1]^=( LUT_Byte2OddBits[tmp_buffer[2]]<<4)|( LUT_Byte2OddBits[tmp_buffer[3]]);
					headerparity[0]^=(LUT_Byte2EvenBits[tmp_buffer[0]]<<4)|(LUT_Byte2EvenBits[tmp_buffer[1]]);
					headerparity[1]^=(LUT_Byte2EvenBits[tmp_buffer[2]]<<4)|(LUT_Byte2EvenBits[tmp_buffer[3]]);
				}

				sector_conf->header_crc = headerparity[1] | (headerparity[0]<<8) ;

				// Is the header valid (parity ok?)
				if( (header[0]==0xFF) && ( (headerparity[0] == sector_data[26]) && (headerparity[1] == sector_data[27]) ) )
				{
					sector_conf->startsectorindex = start_sector_bit_offset;

					sector_size = 512;

					floppycontext->hxc_printf(MSG_DEBUG,"Valid Amiga MFM sector header found - Cyl:%d Side:%d Sect:%d LeftSect:%d Size:%d",header[1]>>1,header[1]&1,header[2],header[3],sector_size);

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

						sectorparity[0]^=( LUT_Byte2OddBits[tmp_buffer[0]]<<4)|( LUT_Byte2OddBits[tmp_buffer[1]]);
						sectorparity[1]^=( LUT_Byte2OddBits[tmp_buffer[2]]<<4)|( LUT_Byte2OddBits[tmp_buffer[3]]);
						sectorparity[0]^=(LUT_Byte2EvenBits[tmp_buffer[0]]<<4)|(LUT_Byte2EvenBits[tmp_buffer[1]]);
						sectorparity[1]^=(LUT_Byte2EvenBits[tmp_buffer[2]]<<4)|(LUT_Byte2EvenBits[tmp_buffer[3]]);
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

					sector_conf->input_data = (unsigned char*)malloc(sector_size);
					if( sector_conf->input_data )
					{
						memcpy(sector_conf->input_data,&sector_data[32],sector_size);
					}

					// "Empty" sector detection
					checkEmptySector(sector_conf);

					bit_offset = chgbitptr(track->tracklen,bit_offset,(8*2)+1);

					sector_extractor_sm=ENDOFSECTOR;

				}
				else
				{
					sector_conf->use_alternate_header_crc = 0xFF;
					sector_conf->endsectorindex = sector_conf->startdataindex;

					bit_offset = chgbitptr( track->tracklen, bit_offset, (8*2)+1 );
					if( bit_offset < last_start_offset )
					{	// track position roll-over ? -> End
						sector_extractor_sm = ENDOFTRACK;
						bit_offset = -1;
					}
					else
					{
						sector_extractor_sm = LOOKFOR_GAP1;
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
		sectorparity[0]^=(LUT_Byte2OddBits[buffer[i]]<<4) | LUT_Byte2OddBits[buffer[i+1]];
		sectorparity[1]^=(LUT_Byte2OddBits[buffer[i+2]]<<4) | LUT_Byte2OddBits[buffer[i+3]];
	}

	for(i=0;i<sector->sectorsize;i=i+4)
	{
		sectorparity[0]^=(LUT_Byte2EvenBits[buffer[i]]<<4) | LUT_Byte2EvenBits[buffer[i+1]];
		sectorparity[1]^=(LUT_Byte2EvenBits[buffer[i+2]]<<4) | LUT_Byte2EvenBits[buffer[i+3]];
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
		byte = (uint8_t)((LUT_Byte2OddBits[buffer[l]]<<4) | LUT_Byte2OddBits[buffer[l+1]]);
		mfm_code = (uint16_t)(LUT_Byte2MFM[byte] & lastbit);

		mfm_buffer[i++] = (uint8_t)(mfm_code>>8);
		mfm_buffer[i++] = (uint8_t)(mfm_code&0xFF);

		lastbit = (uint16_t)(~(LUT_Byte2MFM[byte]<<15));
	}

	for(l=0;l<buffersize;l=l+2)
	{
		byte = (uint8_t)((LUT_Byte2EvenBits[buffer[l]]<<4) | LUT_Byte2EvenBits[buffer[l+1]]);
		mfm_code = (uint16_t)(LUT_Byte2MFM[byte] & lastbit);

		mfm_buffer[i++] = (uint8_t)(mfm_code>>8);
		mfm_buffer[i++] = (uint8_t)(mfm_code&0xFF);

		lastbit = (uint16_t)(~(LUT_Byte2MFM[byte]<<15));
	}

	tg.mfm_last_bit = lastbit;
	tg.last_bit_offset = tg.last_bit_offset + ( buffersize * 2 * 8 );
	pushTrackCode(&tg,0x00,0xFF,track,sector->trackencoding);

	return 0;
}

////////////////////////
//
// Amiga Sector
// Gap :  0xFF 0xFF
// Sync : 0xA1 0xA1 (Clock 0x0A 0x0A)
//  ->Sector ID : 0xFF(B3) TR(B2) SE(B1) 11-SE(B0)
//  Sector ID (even B3)
//  Sector ID (even B2)
//  Sector ID (even B1)
//  Sector ID (even B0)
//  Sector ID (odd  B3)
//  Sector ID (odd  B2)
//  Sector ID (odd  B1)
//  Sector ID (odd  B0)
//  Gap - 16 bytes (0x00)
//  -> Header CRC
//  Header CRC (odd B3)
//  Header CRC (odd B2)
//  Header CRC (odd B1)
//  Header CRC (odd B0)
//  Header CRC (even B3)
//  Header CRC (even B2)
//  Header CRC (even B1)
//  Header CRC (even B0)
//  Data CRC (odd B3)
//  Data CRC (odd B2)
//  Data CRC (odd B1)
//  Data CRC (odd B0)
//  Data CRC (even B3)
//  Data CRC (even B2)
//  Data CRC (even B1)
//  Data CRC (even B0)
//  Data ( even and odd)

void tg_addAmigaSectorToTrack(track_generator *tg,HXCFE_SECTCFG * sectorconfig,HXCFE_SIDE * currentside)
{

	int32_t  i;
	int32_t  trackenc;
	int32_t  startindex,j;
	uint8_t  header[4];
	uint8_t  headerparity[2];
	uint8_t  sectorparity[2];

	isoibm_config * configptr;

	configptr = tg->disk_formats_LUT[sectorconfig->trackencoding];

	startindex=tg->last_bit_offset/8;

	sectorconfig->startsectorindex=tg->last_bit_offset/8;

	// sync
	for(i=0;i<configptr->len_ssync;i++)
	{
		pushTrackCode(tg,configptr->data_ssync,0xFF,currentside,sectorconfig->trackencoding);
	}

	// add mark
	for(i=0;i<configptr->len_addrmarkp1;i++)
	{
		pushTrackCode(tg,configptr->data_addrmarkp1,configptr->clock_addrmarkp1,currentside,sectorconfig->trackencoding);
	}

	headerparity[0]=0;
	headerparity[1]=0;

	header[0] = 0xFF;
	header[1] = (uint8_t)((sectorconfig->cylinder<<1) | (sectorconfig->head&1));
	header[2] = (uint8_t)sectorconfig->sector;
	header[3] = (uint8_t)sectorconfig->sectorsleft;

	pushTrackCode(tg,(unsigned char)(( LUT_Byte2OddBits[header[0]]<<4)|( LUT_Byte2OddBits[header[1]])),0xFF,currentside,sectorconfig->trackencoding);
	pushTrackCode(tg,(unsigned char)(( LUT_Byte2OddBits[header[2]]<<4)|( LUT_Byte2OddBits[header[3]])),0xFF,currentside,sectorconfig->trackencoding);
	pushTrackCode(tg,(unsigned char)((LUT_Byte2EvenBits[header[0]]<<4)|(LUT_Byte2EvenBits[header[1]])),0xFF,currentside,sectorconfig->trackencoding);
	pushTrackCode(tg,(unsigned char)((LUT_Byte2EvenBits[header[2]]<<4)|(LUT_Byte2EvenBits[header[3]])),0xFF,currentside,sectorconfig->trackencoding);

	headerparity[0]^=( LUT_Byte2OddBits[header[0]]<<4)|( LUT_Byte2OddBits[header[1]]);
	headerparity[1]^=( LUT_Byte2OddBits[header[2]]<<4)|( LUT_Byte2OddBits[header[3]]);
	headerparity[0]^=(LUT_Byte2EvenBits[header[0]]<<4)|(LUT_Byte2EvenBits[header[1]]);
	headerparity[1]^=(LUT_Byte2EvenBits[header[2]]<<4)|(LUT_Byte2EvenBits[header[3]]);

	// gap2
	for(i=0;i<configptr->len_gap2;i++)
	{
		pushTrackCode(tg,configptr->data_gap2,0xFF,currentside,sectorconfig->trackencoding);
	}

	for(i=0;i<configptr->len_gap2;i=i+2)
	{
		headerparity[0]^=configptr->data_gap2;
		headerparity[1]^=configptr->data_gap2;
	}

	pushTrackCode(tg,0x00,0xFF,currentside,sectorconfig->trackencoding);
	pushTrackCode(tg,0x00,0xFF,currentside,sectorconfig->trackencoding);
	pushTrackCode(tg,(unsigned char)headerparity[0],0xFF,currentside,sectorconfig->trackencoding);
	pushTrackCode(tg,(unsigned char)headerparity[1],0xFF,currentside,sectorconfig->trackencoding);

	sectorparity[0]=0;
	sectorparity[1]=0;
	if(sectorconfig->input_data)
	{
		for(i=0;i<sectorconfig->sectorsize;i=i+4)
		{
			sectorparity[0]^=(LUT_Byte2OddBits[sectorconfig->input_data[i]]<<4) | LUT_Byte2OddBits[sectorconfig->input_data[i+1]];
			sectorparity[1]^=(LUT_Byte2OddBits[sectorconfig->input_data[i+2]]<<4) | LUT_Byte2OddBits[sectorconfig->input_data[i+3]];
		}

		for(i=0;i<sectorconfig->sectorsize;i=i+4)
		{
			sectorparity[0]^=(LUT_Byte2EvenBits[sectorconfig->input_data[i]]<<4) | LUT_Byte2EvenBits[sectorconfig->input_data[i+1]];
			sectorparity[1]^=(LUT_Byte2EvenBits[sectorconfig->input_data[i+2]]<<4) | LUT_Byte2EvenBits[sectorconfig->input_data[i+3]];
		}
	}

	pushTrackCode(tg,0x00,0xFF,currentside,sectorconfig->trackencoding);
	pushTrackCode(tg,0x00,0xFF,currentside,sectorconfig->trackencoding);
	pushTrackCode(tg,(unsigned char)sectorparity[0],0xFF,currentside,sectorconfig->trackencoding);
	pushTrackCode(tg,(unsigned char)sectorparity[1],0xFF,currentside,sectorconfig->trackencoding);

	sectorconfig->startdataindex=tg->last_bit_offset/8;
	if(sectorconfig->input_data)
	{
		FastMFMFMgenerator(tg,currentside,sectorconfig->input_data,sectorconfig->sectorsize,sectorconfig->trackencoding);
	}
	else
	{
		for(i=0;i<sectorconfig->sectorsize;i++)
		{
			pushTrackCode(tg,sectorconfig->fill_byte,0xFF,currentside,sectorconfig->trackencoding);
		}
	}

	//gap3
	if(sectorconfig->gap3!=255)
	{
		for(i=0;i<sectorconfig->gap3;i++)
		{
			pushTrackCode(tg,configptr->data_gap3,0xFF,currentside,sectorconfig->trackencoding);
		}
	}

	// fill timing & encoding buffer
	if(currentside->timingbuffer)
	{
		for(j=startindex;j<(tg->last_bit_offset/8);j++)
		{
			currentside->timingbuffer[j]=sectorconfig->bitrate;
		}
	}

	trackenc=AMIGA_MFM_ENCODING;

	if(currentside->track_encoding_buffer)
	{
		for(j=startindex;j<(tg->last_bit_offset/8);j++)
		{
			currentside->track_encoding_buffer[j] = (uint8_t)trackenc;
		}
	}

	currentside->number_of_sector++;
}
