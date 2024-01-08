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
// File : iso_ibm_fm_track.c
// Contains: ISO/IBM FM track support
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

#include "tracks/track_formats/iso_ibm_fm_track.h"

#include "tracks/trackutils.h"
#include "tracks/encoding/fm_encoding.h"

#include "tracks/luts.h"

#include "sector_sm.h"

int get_next_FM_sector(HXCFE* floppycontext,HXCFE_SIDE * track,HXCFE_SECTCFG * sector,int track_offset)
{
	int bit_offset,old_bit_offset;
	int last_start_offset;
	int sector_size;
	unsigned char fm_buffer[32];
	unsigned char tmp_buffer[32];
	unsigned char * tmp_sector;
	int * tmp_sector_index;
	unsigned char CRC16_High;
	unsigned char CRC16_Low;
	int sector_extractor_sm;
	int k,i;
	unsigned char crctable[32];

    //        C D C D  C D C D
	//0xF8 - 00010100 01000100 // 0x1444
	//0xF9 - 00010100 01000101 // 0x1445
	//0xFA - 00010100 01010100 // 0x1454
	//0xFB - 00010100 01010101 // 0x1455
	//0xFC - 00010101 01000100 // 0x1544
	//0xFD - 00010101 01000101 // 0x1545
	unsigned short datamark[6]={0x1444,0x1445,0x1454,0x1455,0x1544,0x1545};

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
					last_start_offset = bit_offset;
					sector_extractor_sm = LOOKFOR_ADDM;
				}
				else
				{
					sector_extractor_sm = ENDOFTRACK;
				}
			break;

			case LOOKFOR_ADDM:
				sector->endsectorindex = fmtobin(track->databuffer,NULL,track->tracklen,tmp_buffer,7,bit_offset,0);
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
					#define ISO_FM_HEADER_SIZE 7
					for(k=0;k<ISO_FM_HEADER_SIZE;k++)
					{
						CRC16_Update(&CRC16_High,&CRC16_Low, tmp_buffer[k],(unsigned char*)crctable );
					}

					sector->header_crc = ( tmp_buffer[ISO_FM_HEADER_SIZE-2]<<8 ) | tmp_buffer[ISO_FM_HEADER_SIZE-1] ;


					if(!CRC16_High && !CRC16_Low)
					{ // crc ok !!!
						sector->use_alternate_header_crc = 0x00;

						bit_offset = chgbitptr(track->tracklen,bit_offset,7 * 8);

						floppycontext->hxc_printf(MSG_DEBUG,"Valid FM sector header found - Cyl:%d Side:%d Sect:%d Size:%d",tmp_buffer[1],tmp_buffer[2],tmp_buffer[3],sectorsize[tmp_buffer[4]&0x7]);
						old_bit_offset=bit_offset;

						sector_size = sector->sectorsize;


//;01000100 01010101 00010001 00010100 01[s]010101



//;            1   1    1   1    1   0       0   0   -- F8
//;            1   1    1   1    1   0       0   1   -- F9
//;            1   1    1   1    1   0       1   0   -- FA
//;            1   1    1   1    1   0       1   1   -- FB

// DEC RX1/RX2 FM Sync
//;            1   1    1   1    1   1       0   0   -- FC
//;            1   1    1   1    1   1       0   1   -- FD

						//11111011
						fm_buffer[0]=0x55;
						fm_buffer[1]=0x11;
						fm_buffer[2]=0x14;
						fm_buffer[3]=0x55;

						i=0;
						do
						{
							fm_buffer[2] = (datamark[i] >> 8) & 0xFF;
							fm_buffer[3] = datamark[i] & 0xFF;
							bit_offset = searchBitStream(track->databuffer,track->tracklen,((88+16)*8*2),fm_buffer,4*8,old_bit_offset);
							i++;
						}while(i<6 && bit_offset==-1 );

						if(bit_offset != -1)
						{
							sector->use_alternate_datamark = 0xFF;
							sector->alternate_datamark=0xF8 + (i-1);

							tmp_sector_index = NULL;

							tmp_sector=(unsigned char*)malloc(1+sector_size+2);
							if(!tmp_sector)
								goto error;

							memset(tmp_sector,0,1+sector_size+2);

							tmp_sector_index=(int*)malloc((1+sector_size+2) * sizeof(int));
							if(!tmp_sector_index)
								goto error;

							memset(tmp_sector_index,0,(1+sector_size+2) * sizeof(int));

							sector->startdataindex = bit_offset;
							sector->endsectorindex = fmtobin(track->databuffer,tmp_sector_index,track->tracklen,tmp_sector,1+sector_size+2,bit_offset+(0*8),0);

							CRC16_Init(&CRC16_High,&CRC16_Low,(unsigned char*)crctable,0x1021,0xFFFF);
							for(k=0;k<1+sector_size+2;k++)
							{
								CRC16_Update(&CRC16_High,&CRC16_Low, tmp_sector[k],(unsigned char*)crctable );
							}

							sector->data_crc = ( tmp_sector[1+sector_size]<<8 ) | tmp_sector[1+sector_size+1] ;

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
								memcpy(sector->input_data,&tmp_sector[1],sector_size);

								if(tmp_sector_index)
								{
									sector->input_data_index = (int*)malloc(sector_size*sizeof(int));
									if(sector->input_data_index)
									{
										memcpy(sector->input_data_index,&tmp_sector_index[1],sector_size*sizeof(int));
									}
								}
							}

							free(tmp_sector_index);
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

int write_FM_sectordata(HXCFE* floppycontext,HXCFE_SIDE * track,HXCFE_SECTCFG * sector,unsigned char * buffer,int buffersize)
{
	int bit_offset,i;
	unsigned char CRC16_High;
	unsigned char CRC16_Low;
	unsigned char crctable[32];
	unsigned char temp;

	// Data CRC
	CRC16_Init(&CRC16_High,&CRC16_Low,(unsigned char*)crctable,0x1021,0xFFFF);

	CRC16_Update(&CRC16_High,&CRC16_Low, (unsigned char)sector->alternate_datamark ,(unsigned char*)crctable );

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
