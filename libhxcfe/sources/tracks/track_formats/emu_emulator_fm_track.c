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
// File : emu_emulator_fm_track.c
// Contains: Emulator I/II/SP1200 track support
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
#include "emu_emulator_fm_track.h"

#include "tracks/trackutils.h"
#include "tracks/encoding/fm_encoding.h"

#include "tracks/luts.h"

#include "sector_sm.h"

int get_next_EMU_sector(HXCFE* floppycontext,HXCFE_SIDE * track,HXCFE_SECTCFG * sector,int track_offset)
{

	int bit_offset,old_bit_offset;
	int sector_size;
	int last_start_offset;
	unsigned char fm_buffer[32];
	unsigned char tmp_buffer[32];
	unsigned char CRC16_High;
	unsigned char CRC16_Low;
	int sector_extractor_sm;
	int k;
	unsigned char crctable[32];

	bit_offset = track_offset;
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
					last_start_offset = bit_offset;
					sector_extractor_sm = LOOKFOR_ADDM;
				}
				else
				{
					sector_extractor_sm = ENDOFTRACK;
				}
			break;

			case LOOKFOR_ADDM:

				sector->endsectorindex = fmtobin(track->databuffer,NULL,track->tracklen,tmp_buffer,5,bit_offset,0);
				if((LUT_ByteBitsInverter[tmp_buffer[0]]==0xFA) && (LUT_ByteBitsInverter[tmp_buffer[1]]==0x96))
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
						floppycontext->hxc_printf(MSG_DEBUG,"Valid EmuII FM sector header found - Sect:%d",LUT_ByteBitsInverter[tmp_buffer[2]]);
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

						bit_offset = searchBitStream(track->databuffer,track->tracklen,((88+16)*8*2),fm_buffer,8*8,bit_offset+(4*8*4));
						if(bit_offset!=-1)
						{
							sector->cylinder = LUT_ByteBitsInverter[tmp_buffer[2]]>>1;
							sector->head = LUT_ByteBitsInverter[tmp_buffer[2]]&1;
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

								sector->endsectorindex = fmtobin(track->databuffer,NULL,track->tracklen,sector->input_data,sector_size+2,bit_offset+(8 *8),0);

								CRC16_Init(&CRC16_High,&CRC16_Low,(unsigned char*)crctable,0x8005,0x0000);
								for(k=0;k<sector_size+2;k++)
								{
									CRC16_Update(&CRC16_High,&CRC16_Low, sector->input_data[k],(unsigned char*)crctable );
								}

								sector->data_crc = (sector->input_data[sector_size]<<8) | (sector->input_data[sector_size+1]);

								if(!CRC16_High && !CRC16_Low)
								{ // crc ok !!!
									floppycontext->hxc_printf(MSG_DEBUG,"Data CRC Ok. (0x%.4X)",sector->data_crc);
									sector->use_alternate_data_crc = 0x00;
								}
								else
								{
									floppycontext->hxc_printf(MSG_DEBUG,"Data CRC ERROR ! (0x%.4X)",sector->data_crc);
									sector->use_alternate_data_crc = 0xFF;
								}

								for(k=0;k<sector_size;k++)
								{
									sector->input_data[k]=LUT_ByteBitsInverter[sector->input_data[k]];
								}

								// "Empty" sector detection
								checkEmptySector(sector);
							}

							bit_offset = chgbitptr( track->tracklen, bit_offset, sector_size*4);

							sector_extractor_sm=ENDOFSECTOR;

						}
						else
						{
							bit_offset = chgbitptr( track->tracklen, old_bit_offset, 1);

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
	}while( (sector_extractor_sm!=ENDOFTRACK) && (sector_extractor_sm!=ENDOFSECTOR) );

	return bit_offset;
}

int32_t BuildEmuIITrack(HXCFE* floppycontext,int tracknumber,int sidenumber,unsigned char* datain,unsigned char * fmdata,int32_t * fmsizebuffer,int trackformat)
{
	int i,j,k;
	unsigned char CRC16_High;
	unsigned char CRC16_Low;
	unsigned char *tempdata;
	unsigned char *tempclock;
	int32_t finalsize;
	int32_t current_buffer_size;

	unsigned char crctable[32];

	int32_t sectorsize;
	unsigned char track_num;
	int32_t buffersize;

	buffersize=*fmsizebuffer/8;

	sectorsize=3584;

	track_num = 0;

	current_buffer_size=buffersize/4;
	finalsize=20 + 10 +8 + 6 + sectorsize + 2 + 2 + 20;

	switch(trackformat)
	{
		case 1:
			track_num=tracknumber;
		break;

		case 2:
			track_num=tracknumber*2+sidenumber;
		break;
		default:
			floppycontext->hxc_printf(MSG_ERROR,"BuildEmuIITrack : Bad track format !");
			return finalsize;
		break;
	}

	if(finalsize<=current_buffer_size)
	{
		j=0;

		tempdata=(unsigned char *)malloc((buffersize/4)+1);
		tempclock=(unsigned char *)malloc((buffersize/4)+1);
		if(tempdata && tempclock)
		{

			memset(tempclock,0xFF,(buffersize/4)+1);
			memset(tempdata, 0xFF,(buffersize/4)+1);


			/////////////////////////////////////////////////////////////////////////////////////////////
			//Track GAP
			for(k=0;k<20;k++)
			{
				setfieldbit(tempdata,LUT_ByteBitsInverter[0xFF],j,8);
				j=j+8;
			}

			/////////////////////////////////////////////////////////////////////////////////////////////
			//Sector Header
			for(k=0;k<4;k++)
			{
				setfieldbit(tempdata,LUT_ByteBitsInverter[0x00],j,8);
				j += 8;
			}

			setfieldbit(tempdata,LUT_ByteBitsInverter[0xFA],j,8);
			j += 8;
			setfieldbit(tempdata,LUT_ByteBitsInverter[0x96],j,8);
			j += 8;
			setfieldbit(tempdata,LUT_ByteBitsInverter[track_num],j,8);
			j += 8;

			//CRC The sector Header CRC
			CRC16_Init(&CRC16_High,&CRC16_Low,(unsigned char*)&crctable,0x8005,0x0000);
			CRC16_Update(&CRC16_High,&CRC16_Low,LUT_ByteBitsInverter[track_num],(unsigned char*)&crctable );

			setfieldbit(tempdata,CRC16_High,j,8);
			j += 8;
			setfieldbit(tempdata,CRC16_Low,j,8);
			j += 8;

			switch(trackformat)
			{

				// Emu 1
				case 1:
					setfieldbit(tempdata,LUT_ByteBitsInverter[0x00],j,8);
					j += 8;
					setfieldbit(tempdata,LUT_ByteBitsInverter[0x00],j,8);
					j += 8;

					for(k=0;k<7;k++)
					{
						setfieldbit(tempdata,LUT_ByteBitsInverter[0xFF],j,8);
						j += 8;
					}
					//tempdata[j++]=LUT_ByteBitsInverter[0xFF];

				break;

				// Emu 2
				case 2:
					setfieldbit(tempdata,LUT_ByteBitsInverter[0x00],j,8);
					j += 8;
					for(k=0;k<7;k++)
					{
						setfieldbit(tempdata,LUT_ByteBitsInverter[0xFF],j,8);
						j += 8;
					}

					setfieldbit(tempdata,LUT_ByteBitsInverter[0xFF],j,4);
					j += 4;
				break;
			}


			/////////////////////////////////////////////////////////////////////////////////////////////
			//Sector Data

			for(k=0;k<4;k++)
			{
				setfieldbit(tempdata,LUT_ByteBitsInverter[0x00],j,8);
				j += 8;
			}

			setfieldbit(tempdata,LUT_ByteBitsInverter[0xFA],j,8);
			j += 8;
			setfieldbit(tempdata,LUT_ByteBitsInverter[0x96],j,8);
			j += 8;

			//CRC The sector data CRC
			CRC16_Init(&CRC16_High,&CRC16_Low,(unsigned char*)&crctable,0x8005,0x0000);

			for(k=0;k<sectorsize;k++)
			{
				setfieldbit(tempdata,LUT_ByteBitsInverter[datain[k]],j,8);
				j += 8;
				CRC16_Update(&CRC16_High,&CRC16_Low,LUT_ByteBitsInverter[datain[k]],(unsigned char*)&crctable );
			}

			setfieldbit(tempdata,CRC16_High,j,8);
			j += 8;
			setfieldbit(tempdata,CRC16_Low,j,8);
			j += 8;

			setfieldbit(tempdata,LUT_ByteBitsInverter[0x00],j,8);
			j += 8;
			setfieldbit(tempdata,LUT_ByteBitsInverter[0x00],j,8);
			j += 8;


			for(k=0;k<20;k++)
			{
				setfieldbit(tempdata,LUT_ByteBitsInverter[0xFF],j,8);
				j += 8;
			}
			/////////////////////////////////////////////////////////////////////////////////////////////

			if((j/8)<=current_buffer_size)
			{
				for(i=j;i<(current_buffer_size*8);i=i+8)
				{
					if(j+8<(current_buffer_size*8))
					{
						setfieldbit(tempdata,LUT_ByteBitsInverter[0xFF],j,8);
					}
					else
					{
						setfieldbit(tempdata,LUT_ByteBitsInverter[0xFF],j,(current_buffer_size*8)-j);
					}

					j = j + 8;
				}
			}

			BuildFMCylinder(fmdata,buffersize,tempclock,tempdata,(buffersize)/4);

			free(tempdata);
			free(tempclock);

			return 0;
		}
		else
		{
			if(tempdata)
				free(tempdata);
			if(tempclock)
				free(tempclock);

			return -1;
		}
	}
	else
	{
		floppycontext->hxc_printf(MSG_ERROR,"BuildEmuIITrack : No enough space on this track !");
		return finalsize;
	}

}
