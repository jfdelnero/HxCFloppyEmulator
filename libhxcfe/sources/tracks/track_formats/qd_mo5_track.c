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
// File : qd_mo5_track.c
// Contains: QuickDisk MO5 MFM track support
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

#include "qd_mo5_track.h"

#include "tracks/trackutils.h"
#include "tracks/encoding/mfm_encoding.h"

#include "tracks/luts.h"

#include "sector_sm.h"

int get_next_QDMO5_sector(HXCFE* floppycontext,HXCFE_SIDE * track,HXCFE_SECTCFG * sector,int track_offset)
{
	int bit_offset_bak,bit_offset,tmp_bit_offset;
	int sector_size;
	unsigned char mfm_buffer[32];
	unsigned char tmp_buffer[32];
	unsigned char * tmp_sector;
	unsigned char checksum;
	int sector_extractor_sm;
	int k;

	memset(sector,0,sizeof(HXCFE_SECTCFG));

	bit_offset=track_offset;

	sector_extractor_sm=LOOKFOR_GAP1;

	do
	{
		switch(sector_extractor_sm)
		{
			case LOOKFOR_GAP1:
				mfm_buffer[0] = 0xA9;
				mfm_buffer[1] = 0x14;
				mfm_buffer[2] = 0xA9;
				mfm_buffer[3] = 0x14;
				mfm_buffer[4] = 0xA9;
				mfm_buffer[5] = 0x14;
				mfm_buffer[6] = 0xA9;
				mfm_buffer[7] = 0x14;
				mfm_buffer[8] = 0xA9;
				mfm_buffer[9] = 0x14;
				mfm_buffer[10] = 0x44;
				mfm_buffer[11] = 0x91;

				bit_offset = searchBitStream(track->databuffer,track->tracklen,-1,mfm_buffer,12*8,bit_offset);
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

				bit_offset = chgbitptr( track->tracklen, bit_offset, ( 10 * 8 ));

				tmp_bit_offset = mfmtobin(track->databuffer,NULL,track->tracklen,tmp_buffer,16,bit_offset,0);

				sector->cylinder = 0;
				sector->head = 0;
				sector->sector = (tmp_buffer[1]<<8) + tmp_buffer[2];
				sector->sectorsize = 128;
				sector->alternate_sector_size_id = 0;
				sector->trackencoding = QD_MO5_MFM;
				sector->alternate_datamark=0x00;
				sector->use_alternate_datamark = 0xFF;
				sector->header_crc = 0x00;
				sector->use_alternate_header_crc = 0xFF;

				sector->startsectorindex = bit_offset;

				if(track->timingbuffer)
					sector->bitrate = track->timingbuffer[bit_offset/8];
				else
					sector->bitrate = track->bitrate;

				floppycontext->hxc_printf(MSG_DEBUG,"Valid QD MO5 sector header found - Cyl:%d Side:%d Sect:%d Size:%d",tmp_buffer[4],tmp_buffer[5],tmp_buffer[6],sectorsize[tmp_buffer[7]&0x7]);
				sector->use_alternate_header_crc = 0;

				bit_offset = chgbitptr( track->tracklen, bit_offset, 1);

				sector_size = sector->sectorsize;
				bit_offset_bak = bit_offset;

				mfm_buffer[0] = 0xA9;
				mfm_buffer[1] = 0x14;
				mfm_buffer[2] = 0xA9;
				mfm_buffer[3] = 0x14;
				mfm_buffer[4] = 0xA9;
				mfm_buffer[5] = 0x14;
				mfm_buffer[6] = 0xA9;
				mfm_buffer[7] = 0x14;
				mfm_buffer[8] = 0xA9;
				mfm_buffer[9] = 0x14;
				mfm_buffer[10] = 0x91;
				mfm_buffer[11] = 0x44;
				bit_offset = searchBitStream(track->databuffer,track->tracklen,(88+16)*8,mfm_buffer,12*8,bit_offset);

				if((bit_offset!=-1))
				{
					tmp_sector=(unsigned char*)malloc(1+sector_size+1);
					if( !tmp_sector )
						return -1;

					memset(tmp_sector,0,1+sector_size+1);

					sector->startdataindex=bit_offset;
					bit_offset = chgbitptr( track->tracklen, bit_offset, ( 10 * 8 ));
					sector->endsectorindex=mfmtobin(track->databuffer,NULL,track->tracklen,tmp_sector,1 + sector_size + 1,bit_offset,0);
					sector->alternate_datamark = tmp_sector[0];
					sector->use_alternate_datamark = 0xFF;

					checksum = 0;
					for(k=0;k<1+128;k++)
					{
						checksum += tmp_sector[k];
					}

					sector->data_crc = tmp_sector[k];

					if(checksum == tmp_sector[k])
					{ // crc ok !!!
						floppycontext->hxc_printf(MSG_DEBUG,"Data checksum Ok. (0x%.2X)",sector->data_crc);
					}
					else
					{
						floppycontext->hxc_printf(MSG_DEBUG,"Data checksum ERROR ! (0x%.2X)",sector->data_crc);
						sector->use_alternate_data_crc=0xFF;
					}

					sector->input_data = (unsigned char*)malloc(sector_size);
					if(sector->input_data)
					{
						memcpy(sector->input_data,&tmp_sector[1],sector_size);
					}
					free(tmp_sector);

					// "Empty" sector detection
					checkEmptySector(sector);

					bit_offset = chgbitptr( track->tracklen, bit_offset, 1);

					sector_extractor_sm=ENDOFSECTOR;
				}
				else
				{
					sector->startdataindex = tmp_bit_offset;
					sector->endsectorindex = tmp_bit_offset;

					bit_offset = chgbitptr( track->tracklen, bit_offset_bak, 1);

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
