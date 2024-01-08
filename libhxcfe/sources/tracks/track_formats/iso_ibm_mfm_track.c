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
// File : iso_ibm_mfm_track.c
// Contains: ISO/IBM MFM track support
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

#include "tracks/track_formats/iso_ibm_mfm_track.h"

#include "tracks/trackutils.h"
#include "tracks/encoding/mfm_encoding.h"

#include "tracks/luts.h"

#include "sector_sm.h"

int get_next_MFM_sector(HXCFE* floppycontext,HXCFE_SIDE * track,HXCFE_SECTCFG * sector,int track_offset)
{
	int bit_offset_bak,bit_offset,tmp_bit_offset;
	int last_start_offset;
	int sector_size;
	unsigned char mfm_buffer[32];
	unsigned char tmp_buffer[32];
	unsigned char * tmp_sector;
	int * tmp_sector_index;
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
					last_start_offset = bit_offset;
					sector_extractor_sm=LOOKFOR_ADDM;
				}
				else
				{
					sector_extractor_sm=ENDOFTRACK;
				}
			break;

			case LOOKFOR_ADDM:
				tmp_bit_offset = mfmtobin(track->databuffer,NULL,track->tracklen,tmp_buffer,3+7,bit_offset,0);
				if(tmp_buffer[3]==0xFE)
				{
					#define SECT_HEADER_SIZE (3+7)

					CRC16_Init(&CRC16_High,&CRC16_Low,(unsigned char*)crctable,0x1021,0xFFFF);
					for(k=0;k<SECT_HEADER_SIZE;k++)
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
					sector->header_crc = ( tmp_buffer[SECT_HEADER_SIZE-2]<<8 ) | tmp_buffer[SECT_HEADER_SIZE-1] ;
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

					bit_offset = chgbitptr( track->tracklen, bit_offset, 1 );

					sector_size = sectorsize[tmp_buffer[7]&0x7];
					bit_offset_bak = bit_offset;

					mfm_buffer[0] = 0x44;
					mfm_buffer[1] = 0x89;
					mfm_buffer[2] = 0x44;
					mfm_buffer[3] = 0x89;
					mfm_buffer[4] = 0x44;
					mfm_buffer[5] = 0x89;
					bit_offset = searchBitStream(track->databuffer,track->tracklen,(89+16)*8,mfm_buffer,6*8,bit_offset);

					if((bit_offset!=-1))
					{
						tmp_sector_index = NULL;

						tmp_sector=(unsigned char*)malloc(3+1+sector_size+2);
						if(!tmp_sector)
							goto error;

						memset(tmp_sector,0,3+1+sector_size+2);

						tmp_sector_index=(int*)malloc((3+1+sector_size+2) * sizeof(int));
						if(!tmp_sector_index)
							goto error;

						memset(tmp_sector_index,0,(3+1+sector_size+2) * sizeof(int));

						sector->startdataindex = bit_offset;
						sector->endsectorindex = mfmtobin(track->databuffer,tmp_sector_index,track->tracklen,tmp_sector,3+1+sector_size+2,bit_offset,0);

						if(tmp_sector[3] != 0xFE)
						{
							sector->alternate_datamark = tmp_sector[3];
							sector->use_alternate_datamark = 0xFF;

							#define SECT_DATA_SIZE (3+1+sector_size+2)
							CRC16_Init(&CRC16_High,&CRC16_Low,(unsigned char*)crctable,0x1021,0xFFFF);
							for(k=0;k<SECT_DATA_SIZE;k++)
							{
								CRC16_Update(&CRC16_High,&CRC16_Low, tmp_sector[k],(unsigned char*)crctable );
							}

							sector->data_crc = ( tmp_sector[SECT_DATA_SIZE-2]<<8 ) | tmp_sector[SECT_DATA_SIZE-1] ;

							if(!CRC16_High && !CRC16_Low)
							{ // crc ok !!!
								floppycontext->hxc_printf(MSG_DEBUG,"Data CRC Ok. (0x%.4X)",sector->data_crc);
							}
							else
							{
								floppycontext->hxc_printf(MSG_DEBUG,"Data CRC ERROR ! (0x%.4X)",sector->data_crc);
								sector->use_alternate_data_crc=0xFF;
							}

							sector->input_data = (unsigned char*)malloc(sector_size);
							if(sector->input_data)
							{
								memcpy(sector->input_data,&tmp_sector[4],sector_size);

								if(tmp_sector_index)
								{
									sector->input_data_index = (int*)malloc(sector_size*sizeof(int));
									if(sector->input_data_index)
									{
										memcpy(sector->input_data_index,&tmp_sector_index[4],sector_size*sizeof(int));
									}
								}
							}

							if(tmp_sector_index)
								free(tmp_sector_index);

							free(tmp_sector);

							// "Empty" sector detection
							checkEmptySector(sector);

							if(sector->alternate_datamark!=0xFE)
								bit_offset = chgbitptr( track->tracklen, bit_offset, 1 );
						}
						else
						{
							sector->startdataindex = tmp_bit_offset;
							sector->endsectorindex = tmp_bit_offset;
						}

						sector_extractor_sm=ENDOFSECTOR;
					}
					else
					{
						sector->startdataindex = tmp_bit_offset;
						sector->endsectorindex = tmp_bit_offset;

						bit_offset = chgbitptr( track->tracklen, bit_offset_bak, 1 );

						sector_extractor_sm=ENDOFSECTOR;
					}
				}
				else
				{
					if( ( (tmp_buffer[3]&0xFC)==0xF8) && (bit_offset > (88 * 8)) )
					{
						sector->startsectorindex = bit_offset;
						sector->startdataindex = bit_offset;
						sector->endsectorindex = mfmtobin(track->databuffer,NULL,track->tracklen,tmp_buffer,3+7,bit_offset,0);
 						floppycontext->hxc_printf(MSG_DEBUG,"get_next_MFM_sector : Data sector without sector header !?!");

						sector->cylinder = 0;
						sector->head = 0;
						sector->sector = 0;
						sector->sectorsize = 0;
						sector->alternate_sector_size_id = 0;
						sector->trackencoding = ISOFORMAT_DD;
						sector->alternate_datamark = tmp_buffer[3];
						sector->use_alternate_datamark = 0xFF;
						sector->header_crc = 0;
						bit_offset = chgbitptr( track->tracklen, bit_offset, 1 );
						bit_offset_bak = bit_offset;

						sector_extractor_sm=ENDOFSECTOR;
					}
					else
					{
						bit_offset = chgbitptr( track->tracklen, bit_offset, 1);
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

error:
	if(tmp_sector)
		free(tmp_sector);

	if(tmp_sector_index)
		free(tmp_sector_index);

	return -1;
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
	CRC16_Update(&CRC16_High,&CRC16_Low, (unsigned char)sector->alternate_datamark ,(unsigned char*)crctable );

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
