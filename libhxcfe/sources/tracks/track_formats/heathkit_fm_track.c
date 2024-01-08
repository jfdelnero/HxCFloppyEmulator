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
// File : heathkit_fm_track.c
// Contains: Heathkit Hardsectored track support
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

#include "tracks/track_formats/heathkit_fm_track.h"

#include "tracks/trackutils.h"
#include "tracks/encoding/fm_encoding.h"

#include "tracks/luts.h"

#include "sector_sm.h"

int get_next_FM_Heathkit_sector(HXCFE* floppycontext,HXCFE_SIDE * track,HXCFE_SECTCFG * sector,int track_offset)
{
	int bit_offset,tmp_bit_offset;
	int i;
	unsigned char fm_buffer[32];
	unsigned char tmp_buffer[8 + 256 + 1]; // Sync + Data + Checksum
	unsigned char checksum;
	int sector_extractor_sm;
	unsigned char tmp_sector[3+1+256+1];

	memset(sector,0,sizeof(HXCFE_SECTCFG));

	bit_offset=track_offset;

	sector_extractor_sm=LOOKFOR_GAP1;

	do
	{
		switch(sector_extractor_sm)
		{
			case LOOKFOR_GAP1:
				memset(tmp_buffer,0x00,sizeof(tmp_buffer));
				tmp_buffer[3] = LUT_ByteBitsInverter[0xFD];
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
					while( bit_offset < track->tracklen && !track->indexbuffer[ bit_offset / 8 ] )
					{
						bit_offset++;
					}

					if( bit_offset >= track->tracklen )
					{
						sector_extractor_sm=ENDOFTRACK;
						bit_offset = -1;
						break;
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

				tmp_bit_offset = fmtobin(track->databuffer,NULL,track->tracklen,tmp_buffer,5,bit_offset + (4 * 8 * 3),0);
				if( tmp_buffer[0] == LUT_ByteBitsInverter[0xFD] )
				{
					checksum = 0x00;

					for(i=0;i<3;i++)
					{
						checksum ^= LUT_ByteBitsInverter[ tmp_buffer[ 1 + i ] ];
						checksum = (checksum >> 7) | (checksum << 1);
					}

					if( checksum != LUT_ByteBitsInverter[tmp_buffer[ 4 ]] )
					{

						bit_offset = tmp_bit_offset + 1;

						while( track->indexbuffer[ bit_offset / 8 ] && ( bit_offset < track->tracklen ) )
						{
							bit_offset++;
						}
						sector_extractor_sm=LOOKFOR_GAP1;
						break;
					}

					sector->startsectorindex = (bit_offset + (3*8*4)) % track->tracklen;
					sector->startdataindex = sector->startsectorindex;
					sector->endsectorindex = tmp_bit_offset;

					sector->cylinder = LUT_ByteBitsInverter[ tmp_buffer[2] ];
					sector->head = 0;
					sector->sector = LUT_ByteBitsInverter[ tmp_buffer[3] ];
					sector->sectorsize = 256;
					sector->alternate_sector_size_id = 1;
					sector->trackencoding = HEATHKIT_HS_SD;
					sector->alternate_datamark = 0xFD;
					sector->use_alternate_datamark = 0xFF;

					// Volume (hijack the alternate_addressmark field to transport it...)
					sector->alternate_addressmark = LUT_ByteBitsInverter[ tmp_buffer[1] ];
					sector->use_alternate_addressmark = 0x00;
					sector->header_crc = LUT_ByteBitsInverter[tmp_buffer[ 4 ]];

					sector->data_crc = LUT_ByteBitsInverter[tmp_buffer[ 4 ]];
					sector->use_alternate_header_crc = 0x00;

					if(track->timingbuffer)
						sector->bitrate = track->timingbuffer[bit_offset/8];
					else
						sector->bitrate = track->bitrate;

					if( checksum == LUT_ByteBitsInverter[tmp_buffer[ 4 ]] )
					{ // checksum ok !!!
 						floppycontext->hxc_printf(MSG_DEBUG,"Valid FM Heathkit sector found - Cyl:%d Side:%d Sect:%d Size:%d",LUT_ByteBitsInverter[ tmp_buffer[2] ],tmp_buffer[5],LUT_ByteBitsInverter[ tmp_buffer[3] ],sectorsize[tmp_buffer[7]&0x7]);
					}
					else
					{
						floppycontext->hxc_printf(MSG_DEBUG,"Bad FM Heathkit header found - Cyl:%d Side:%d Sect:%d Size:%d",LUT_ByteBitsInverter[ tmp_buffer[2] ],tmp_buffer[5],LUT_ByteBitsInverter[ tmp_buffer[3] ],sectorsize[tmp_buffer[7]&0x7]);
						sector->use_alternate_header_crc = 0xFF;
					}

					memset(tmp_buffer,0x00,sizeof(tmp_buffer));
					tmp_buffer[3] = LUT_ByteBitsInverter[0xFD];
					bintofm(fm_buffer,sizeof(fm_buffer)*8,tmp_buffer,8,0);

					bit_offset++;

					bit_offset = searchBitStream(track->databuffer,track->tracklen,(88+16)*8,fm_buffer,4*8*4,bit_offset);
					if((bit_offset!=-1))
					{
						memset(tmp_sector, 0, sizeof(tmp_sector));

						sector->startdataindex = (bit_offset + (3*8*4)) % track->tracklen;
						sector->endsectorindex = fmtobin(track->databuffer,NULL,track->tracklen,tmp_sector,3+1+sector->sectorsize+1,bit_offset,0);

						if(tmp_sector[3+0] == LUT_ByteBitsInverter[0xFD])
						{
							sector->alternate_datamark = LUT_ByteBitsInverter[tmp_sector[3+0]];
							sector->use_alternate_datamark = 0xFF;

							checksum = 0x00;

							for(i=0;i<sector->sectorsize;i++)
							{
								checksum ^= LUT_ByteBitsInverter[ tmp_buffer[ 3 + 1 + i ] ];
								checksum = (checksum >> 7) | (checksum << 1);
							}

							sector->data_crc = checksum;
							sector->use_alternate_data_crc = 0x00;

							if( checksum == LUT_ByteBitsInverter[tmp_buffer[ 3 + 1 +sector->sectorsize ]] )
							{ // crc ok !!!
								floppycontext->hxc_printf(MSG_DEBUG,"Data CRC Ok. (0x%.4X)",sector->data_crc);
							}
							else
							{
								floppycontext->hxc_printf(MSG_DEBUG,"Data CRC ERROR ! (0x%.4X)",sector->data_crc);
								sector->use_alternate_data_crc = 0xFF;
							}

							sector->input_data = (unsigned char*)malloc(sector->sectorsize);
							if( sector->input_data )
							{
								for(i=0;i<256;i++)
								{
									sector->input_data[i] = LUT_ByteBitsInverter[ tmp_sector[ 3 +  1 + i] ];
								}
							}

							// "Empty" sector detection
							checkEmptySector(sector);

							bit_offset = sector->endsectorindex;
						}
						else
						{
							sector->startdataindex = tmp_bit_offset;
							sector->endsectorindex = tmp_bit_offset;
						}

						sector_extractor_sm = ENDOFSECTOR;
					}
					else
					{
						sector->startdataindex = tmp_bit_offset;
						sector->endsectorindex = tmp_bit_offset;

						sector_extractor_sm = ENDOFSECTOR;
					}

					sector_extractor_sm = ENDOFSECTOR;

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

void tg_addHeathkitSectorToTrack(track_generator *tg,HXCFE_SECTCFG * sectorconfig,HXCFE_SIDE * currentside)
{
	int32_t  i;
	int32_t  trackenc;
	int32_t  startindex,j;
	uint8_t  checksum;

	startindex=tg->last_bit_offset/8;

	sectorconfig->startsectorindex=tg->last_bit_offset/8;

	for(i=0;i<17;i++)
	{
		pushTrackCode(tg,0x00,0xFF,currentside,HEATHKIT_HS_SD);
	}

	if(currentside->indexbuffer)
	{
		// Hard-sectored Heathkit H8 disks use track to track sector skew of 5
		// -> need to place the main index on sector 9 for even tracks and sector 4 for odd tracks...
		if(sectorconfig->cylinder & 1)
		{
			if( sectorconfig->sector == 4 )
			{
				us2index( (tg->last_bit_offset + 5000) % currentside->tracklen,currentside,2000,1,0);
			}
		}
		else
		{
			if( sectorconfig->sector == 9 )
			{
				us2index( (tg->last_bit_offset + 5000) % currentside->tracklen,currentside,2000,1,0);
			}
		}

		us2index( tg->last_bit_offset % currentside->tracklen,currentside,2000,1,0);
	}

	for(i=0;i<14;i++)
	{
		pushTrackCode(tg,0x00,0xFF,currentside,HEATHKIT_HS_SD);
	}

	checksum = 0x00;

	// sync
	pushTrackCode(tg,LUT_ByteBitsInverter[0xFD],0xFF,currentside,HEATHKIT_HS_SD);

	// Volume (hijack the alternate_addressmark field to transport it...)
	pushTrackCode(tg,LUT_ByteBitsInverter[sectorconfig->alternate_addressmark],0xFF,currentside,HEATHKIT_HS_SD);

	checksum ^= sectorconfig->alternate_addressmark;
	checksum = (checksum >> 7) | (checksum << 1);

	// Track
	pushTrackCode(tg,LUT_ByteBitsInverter[sectorconfig->cylinder],0xFF,currentside,HEATHKIT_HS_SD);

	checksum ^= sectorconfig->cylinder;
	checksum = (checksum >> 7) | (checksum << 1);

	// Sector
	pushTrackCode(tg,LUT_ByteBitsInverter[sectorconfig->sector],0xFF,currentside,HEATHKIT_HS_SD);

	checksum ^= sectorconfig->sector;
	checksum = (checksum >> 7) | (checksum << 1);

	// Header Checksum
	pushTrackCode(tg,LUT_ByteBitsInverter[checksum],0xFF,currentside,HEATHKIT_HS_SD);

	checksum = 0x00;

	for(i=0;i<17;i++)
	{
		pushTrackCode(tg,0x00,0xFF,currentside,HEATHKIT_HS_SD);
	}

	// data sync
	pushTrackCode(tg,LUT_ByteBitsInverter[0xFD],0xFF,currentside,HEATHKIT_HS_SD);

	// data
	for(i=0;i<256;i++)
	{
		pushTrackCode(tg,LUT_ByteBitsInverter[sectorconfig->input_data[ i ]],0xFF,currentside,HEATHKIT_HS_SD);
		checksum ^= sectorconfig->input_data[ i ];
		checksum = (checksum >> 7) | (checksum << 1);
	}

	// Data Checksum
	pushTrackCode(tg,LUT_ByteBitsInverter[checksum],0xFF,currentside,HEATHKIT_HS_SD);

	pushTrackCode(tg,0x00,0xFF,currentside,HEATHKIT_HS_SD);

	// fill timing & encoding buffer
	if(currentside->timingbuffer)
	{
		for(j=startindex;j<(tg->last_bit_offset/8);j++)
		{
			currentside->timingbuffer[j]=sectorconfig->bitrate;
		}
	}

	trackenc = HEATHKIT_HS_FM_ENCODING;

	if(currentside->track_encoding_buffer)
	{
		for(j=startindex;j<(tg->last_bit_offset/8);j++)
		{
			currentside->track_encoding_buffer[j] = (uint8_t)trackenc;
		}
	}

	currentside->number_of_sector++;
}
