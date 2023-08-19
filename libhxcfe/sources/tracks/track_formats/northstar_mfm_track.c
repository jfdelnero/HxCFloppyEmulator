/*
//
// Copyright (C) 2006-2023 Jean-François DEL NERO
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
// File : northstar_mfm_track.c
// Contains: Northstar MFM hardsectored track support
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

#include "floppy_utils.h"

#include "tracks/sector_extractor.h"
#include "tracks/crc.h"

#include "northstar_mfm_track.h"

#include "tracks/trackutils.h"
#include "tracks/encoding/mfm_encoding.h"

#include "tracks/luts.h"

#include "sector_sm.h"

int get_next_MFM_Northstar_sector(HXCFE* floppycontext,HXCFE_SIDE * track,HXCFE_SECTCFG * sector,int track_offset)
{
	int bit_offset,tmp_bit_offset;
	int sector_size,i,j;
	unsigned char mfm_buffer[32];
	unsigned char tmp_buffer[2 + 512 + 1]; // Sync + Sector Info + Data + Checksum
	unsigned char checksum;
	int sector_extractor_sm;

	memset(sector,0,sizeof(HXCFE_SECTCFG));

	bit_offset=track_offset;

	sector_extractor_sm=LOOKFOR_GAP1;

	do
	{
		switch(sector_extractor_sm)
		{
			case LOOKFOR_GAP1:
				memset(mfm_buffer,0x00,sizeof(mfm_buffer));
				memset(tmp_buffer,0x00,sizeof(tmp_buffer));
				tmp_buffer[7] = 0xFB;
				bintomfm(mfm_buffer,sizeof(mfm_buffer)*8,tmp_buffer,8,0);

				bit_offset = searchBitStream(track->databuffer,track->tracklen,-1,mfm_buffer,2*8*8,bit_offset);

				if(bit_offset!=-1)
				{
					if( bit_offset >= 500 )
						i = (bit_offset - 500) % track->tracklen;
					else
						i =  (track->tracklen -  (500 - bit_offset) ) % track->tracklen;

					j = 0;
					while( !track->indexbuffer[(( i % track->tracklen )/8)] && j < 1000 )
					{
						i++;
						j++;
					}

					if(j < 1000)
						sector_extractor_sm=LOOKFOR_ADDM;
					else
						bit_offset = chgbitptr( track->tracklen, bit_offset, 1 );
				}
				else
				{
					sector_extractor_sm=ENDOFTRACK;
				}
			break;

			case LOOKFOR_ADDM:
				tmp_bit_offset = mfmtobin(track->databuffer,NULL,track->tracklen,tmp_buffer,2 + 512 + 1,bit_offset + (8 * 7 * 2),0);
				if( tmp_buffer[0] == 0xFB )
				{
					sector->startdataindex = chgbitptr( track->tracklen, bit_offset, (8 * 9 * 2) );
					sector->endsectorindex = tmp_bit_offset;

					checksum = 0x00;

					for(i=0;i<512;i++)
					{
						checksum ^= tmp_buffer[ 2 + i];
						checksum = (checksum >> 7) | (checksum << 1);
					}

					sector->cylinder = (tmp_buffer[1] >> 4) & 0xF;
					sector->head = 0;
					sector->sector = tmp_buffer[1] & 0xF;
					sector->sectorsize = 512;
					sector->alternate_sector_size_id = 2;
					sector->trackencoding = NORTHSTAR_HS_DD;
					sector->alternate_datamark = 0x00;
					sector->use_alternate_datamark = 0x00;
					sector->alternate_addressmark = 0xFB;
					sector->use_alternate_addressmark = 0xFF;
					sector->header_crc = 0x0000;
					sector->use_alternate_header_crc = 0x00;

					sector->startsectorindex=bit_offset;

					sector->data_crc = tmp_buffer[2 + 512];

					if(track->timingbuffer)
						sector->bitrate = track->timingbuffer[bit_offset/8];
					else
						sector->bitrate = track->bitrate;

					if(tmp_buffer[2 + 512] == checksum)
					{ // checksum ok !!!
 						floppycontext->hxc_printf(MSG_DEBUG,"Valid MFM Northstar sector found - Cyl:%d Side:%d Sect:%d Size:%d",tmp_buffer[4],tmp_buffer[5],tmp_buffer[6],sectorsize[tmp_buffer[7]&0x7]);
					}
					else
					{
						floppycontext->hxc_printf(MSG_DEBUG,"Bad MFM Northstar header found - Cyl:%d Side:%d Sect:%d Size:%d",tmp_buffer[4],tmp_buffer[5],tmp_buffer[6],sectorsize[tmp_buffer[7]&0x7]);
						sector->use_alternate_data_crc = 0xFF;
					}

					sector_size = 512;

					sector->input_data=(unsigned char*)malloc(sector_size);
					if(sector->input_data)
						memcpy(sector->input_data,&tmp_buffer[2],sector_size);

					bit_offset = chgbitptr( track->tracklen, bit_offset, 1 );

					sector_extractor_sm=ENDOFSECTOR;
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

void tg_addNorthstarSectorToTrack(track_generator *tg,HXCFE_SECTCFG * sectorconfig,HXCFE_SIDE * currentside)
{

	int32_t  i;
	int32_t  trackenc;
	int32_t  startindex,j;
	uint8_t  checksum;

	startindex=tg->last_bit_offset/8;

	sectorconfig->startsectorindex=tg->last_bit_offset/8;

	if(currentside->indexbuffer)
	{
		if( sectorconfig->sector == 9 )
		{
			us2index( (tg->last_bit_offset + 5000 ) % currentside->tracklen,currentside,4000,1,0);
		}

		us2index( (tg->last_bit_offset) % currentside->tracklen,currentside,4000,1,0);
	}

	for(i=0;i<32;i++)
	{
		pushTrackCode(tg,0x00,0xFF,currentside,NORTHSTAR_HS_DD);
	}

	// sync
	pushTrackCode(tg,0xFB,0xFF,currentside,NORTHSTAR_HS_DD);

	// info
	pushTrackCode(tg, (uint8_t)( ( ((sectorconfig->head & 1)<<4) | (sectorconfig->cylinder&1)<<(4+2)) | ((sectorconfig->cylinder&2)<<(4+2)) | (sectorconfig->sector&0xF) ),0xFF,currentside,NORTHSTAR_HS_DD);

	checksum = 0x00;

	// data
	for(i=0;i<512;i++)
	{
		pushTrackCode(tg,sectorconfig->input_data[ i ],0xFF,currentside,NORTHSTAR_HS_DD);
		checksum ^= sectorconfig->input_data[ i ];
		checksum = (checksum >> 7) | (checksum << 1);
	}

	// Checksum
	pushTrackCode(tg,checksum,0xFF,currentside,NORTHSTAR_HS_DD);

	// "zero"
	for(i=0;i<78;i++)
	{
		if(tg->last_bit_offset < (currentside->tracklen - 1) )
		{
			pushTrackCode(tg,0x00,0xFF,currentside,NORTHSTAR_HS_DD);
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

	trackenc=NORTHSTAR_HS_MFM_ENCODING;

	if(currentside->track_encoding_buffer)
	{
		for(j=startindex;j<(tg->last_bit_offset/8);j++)
		{
			currentside->track_encoding_buffer[j] = (uint8_t)trackenc;
		}
	}

	currentside->number_of_sector++;
}
