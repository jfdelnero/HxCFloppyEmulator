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
// File : micraln_fm_track.c
// Contains: R2E Micral Hardsectored track support
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

#include "tracks/track_formats/micraln_fm_track.h"

#include "tracks/trackutils.h"
#include "tracks/encoding/fm_encoding.h"

#include "tracks/luts.h"

#include "sector_sm.h"

#define GAP_VALUE 0x00
#define SYNC_WORD 0xFF
#define CHECKSUM_INIT_VALUE 0x00
#define SECTOR_DATA_SIZE 128
#define SECTOR_SIZE (1 + 2 + SECTOR_DATA_SIZE + 1) // Sync + Header + Data + Checksum


uint8_t update_checksum(uint8_t checksum,uint8_t data)
{
	uint8_t  for_carry,carry;

	for_carry = ((data ^ checksum) ^ 0xff) & ((data + checksum) ^ data);
	carry = (for_carry & 0x80) ? 1 : 0;
	checksum = checksum + data + carry;

	return checksum;
}

int get_next_FM_MicralN_sector(HXCFE* floppycontext,HXCFE_SIDE * track,HXCFE_SECTCFG * sector,int track_offset)
{
	int bit_offset,tmp_bit_offset;
	int i;
	unsigned char fm_buffer[32];
	unsigned char tmp_buffer[SECTOR_SIZE]; // Sync + Data + Checksum
	int * tmp_sector_index;
	unsigned char checksum;
	int sector_extractor_sm;

	memset(sector,0,sizeof(HXCFE_SECTCFG));

	bit_offset = track_offset;

	sector_extractor_sm=LOOKFOR_GAP1;

	do
	{
		switch(sector_extractor_sm)
		{
			case LOOKFOR_GAP1:
				memset(tmp_buffer,GAP_VALUE,sizeof(tmp_buffer));
				tmp_buffer[3] = SYNC_WORD;
				bintofm(fm_buffer,sizeof(fm_buffer)*8,tmp_buffer,4,0);

				bit_offset %= track->tracklen;

				if( track->indexbuffer[ bit_offset / 8 ] )
				{
					// Search the index start point
					while( track->indexbuffer[ bit_offset / 8 ] && (bit_offset > 0) )
					{
						bit_offset--;
					}
				}
				else
				{
					// Search next index
					while( (bit_offset < track->tracklen) && !track->indexbuffer[ bit_offset / 8 ] )
					{
						bit_offset++;
					}

					if( bit_offset >= track->tracklen )
					{
						sector_extractor_sm = ENDOFTRACK;
						bit_offset = -1;
						return bit_offset;
					}
				}

				bit_offset = searchBitStream(track->databuffer,track->tracklen,-1,fm_buffer,4*8*4,bit_offset);

				if(bit_offset!=-1 && ( (bit_offset + (4*8*4)) < track->tracklen ) )
				{
					if( track->indexbuffer[ (bit_offset + (4*8*4))  / 8 ] )
					{
						sector_extractor_sm = LOOKFOR_ADDM;
					}
					else
					{
						bit_offset += (4*8*4);
					}
				}
				else
				{
					sector_extractor_sm=ENDOFTRACK;
				}
			break;

			case LOOKFOR_ADDM:

				tmp_sector_index=(int*)malloc((SECTOR_SIZE) * sizeof(int));
				if(!tmp_sector_index)
					goto error;

				memset(tmp_sector_index,0,(SECTOR_SIZE) * sizeof(int));

				tmp_bit_offset = fmtobin(track->databuffer,tmp_sector_index,track->tracklen,tmp_buffer,SECTOR_SIZE,bit_offset + (4 * 8 * 3),0);
				if( tmp_buffer[0] == SYNC_WORD )
				{
					checksum = CHECKSUM_INIT_VALUE;
					for(i=0;i<SECTOR_DATA_SIZE;i++)
					{
						checksum = update_checksum(checksum,tmp_buffer[ 1 + 2 + i ]);
					}

					sector->data_crc = checksum;
					sector->use_alternate_data_crc = 0x00;

					sector->startsectorindex = (bit_offset + (3*8*4)) % track->tracklen;
					sector->startdataindex = (sector->startsectorindex + (1*8*4)) % track->tracklen;
					sector->endsectorindex = tmp_bit_offset;

					sector->cylinder = tmp_buffer[ 2 ];
					sector->head = 0;
					sector->sector = tmp_buffer[ 1 ];
					sector->sectorsize = SECTOR_DATA_SIZE;
					sector->alternate_sector_size_id = 1;
					sector->trackencoding = MICRALN_HS_SD;
					sector->alternate_datamark = SYNC_WORD;
					sector->use_alternate_datamark = 0xFF;

					sector->alternate_addressmark = 0x00;
					sector->use_alternate_addressmark = 0x00;
					sector->header_crc = tmp_buffer[ SECTOR_SIZE - 1 ];

					sector->data_crc = tmp_buffer[ SECTOR_SIZE - 1 ];
					sector->use_alternate_header_crc = 0x00;

					if(track->timingbuffer)
						sector->bitrate = track->timingbuffer[bit_offset/8];
					else
						sector->bitrate = track->bitrate;

					sector->input_data = (unsigned char*)malloc(sector->sectorsize);
					if(sector->input_data)
					{
						for(i=0;i<sector->sectorsize;i++)
						{
							sector->input_data[i] = tmp_buffer[1 + 2 + i];
						}

						if(tmp_sector_index)
						{
							sector->input_data_index = (int*)malloc(sector->sectorsize*sizeof(int));
							if(sector->input_data_index)
							{
								memcpy(sector->input_data_index,&tmp_sector_index[1],sector->sectorsize*sizeof(int));
							}
						}
					}

					// "Empty" sector detection
					checkEmptySector(sector);

					if( checksum == tmp_buffer[ SECTOR_SIZE - 1 ] )
					{ // checksum ok !!!
						floppycontext->hxc_printf(MSG_DEBUG,"Data checksum Ok. (0x%.2X)",sector->data_crc);
					}
					else
					{
						floppycontext->hxc_printf(MSG_DEBUG,"Data checksum ERROR ! (0x%.2X)",sector->data_crc);
						sector->use_alternate_data_crc = 0xFF;
					}

					bit_offset = sector->endsectorindex;

					sector_extractor_sm = ENDOFSECTOR;
				}

				free(tmp_sector_index);

			break;

			case ENDOFTRACK:

			break;

			default:
				sector_extractor_sm=ENDOFTRACK;
			break;

		}
	}while(	(sector_extractor_sm!=ENDOFTRACK) && (sector_extractor_sm!=ENDOFSECTOR));

	return bit_offset;

error:
	if(tmp_sector_index)
		free(tmp_sector_index);

	return -1;
}

void tg_addMicralNSectorToTrack(track_generator *tg,HXCFE_SECTCFG * sectorconfig,HXCFE_SIDE * currentside)
{
	int32_t  i;
	int32_t  trackenc;
	int32_t  startindex,j;
	uint8_t  checksum,c;

	startindex=tg->last_bit_offset/8;

	sectorconfig->startsectorindex=tg->last_bit_offset/8;

	for(i=0;i<17;i++)
	{
		pushTrackCode(tg,GAP_VALUE,0xFF,currentside,MICRALN_HS_SD);
	}

	if(currentside->indexbuffer)
	{
		if( sectorconfig->sector == 31 )
		{
			us2index( (tg->last_bit_offset + 3500) % currentside->tracklen,currentside,1000,1,0);
			//printf("devmem 0x%.8X 32 %d\n",0xFF200070,(int)((float)((tg->last_bit_offset + 2500) % currentside->tracklen)*(float)6.25));

		}

		us2index( tg->last_bit_offset % currentside->tracklen,currentside,1000,1,0);
		//printf("devmem 0x%.8X 32 %d\n",0xFF200180+(sectorconfig->sector*4),(int)((float)((tg->last_bit_offset + 9100) % currentside->tracklen)*(float)6.25));
	}

	for(i=0;i<16;i++)
	{
		pushTrackCode(tg,GAP_VALUE,0xFF,currentside,MICRALN_HS_SD);
	}

	// Sync
	pushTrackCode(tg,SYNC_WORD,0xFF,currentside,MICRALN_HS_SD);

	// Warning !!!
	// The machine need these 2 bytes but their exact
	// meaning ("Sector" & "Track") are currently speculative.
	// The Micral N boot ROM doesn't use them
	// but expect 2 bytes after the sync word and before the
	// sector data.

	// Sector number
	pushTrackCode(tg,sectorconfig->sector,0xFF,currentside,MICRALN_HS_SD);

	// Track
	pushTrackCode(tg,sectorconfig->cylinder,0xFF,currentside,MICRALN_HS_SD);

	// (End of Warning)

	checksum = CHECKSUM_INIT_VALUE;

	// Data
	for(i=0;i<128;i++)
	{
		c = sectorconfig->input_data[ i ];

		pushTrackCode(tg,c,0xFF,currentside,MICRALN_HS_SD);

		checksum = update_checksum( checksum, c );
	}

	// Checksum
	pushTrackCode(tg,checksum,0xFF,currentside,MICRALN_HS_SD);

	for(i=0;i<30;i++)
	{
		pushTrackCode(tg,GAP_VALUE,0xFF,currentside,MICRALN_HS_SD);
	}

	// fill timing & encoding buffer
	if(currentside->timingbuffer)
	{
		for(j=startindex;j<(tg->last_bit_offset/8);j++)
		{
			currentside->timingbuffer[j]=sectorconfig->bitrate;
		}
	}

	trackenc = MICRALN_HS_FM_ENCODING;

	if(currentside->track_encoding_buffer)
	{
		for(j=startindex;j<(tg->last_bit_offset/8);j++)
		{
			currentside->track_encoding_buffer[j] = (uint8_t)trackenc;
		}
	}

	currentside->number_of_sector++;
}
