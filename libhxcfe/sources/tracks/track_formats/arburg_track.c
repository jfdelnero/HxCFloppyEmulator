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
// File : arburg_track.c
// Contains: Arburg track support
//
// Written by: Jean-François DEL NERO
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "types.h"

#include "internal_libhxcfe.h"
#include "tracks/track_generator.h"
#include "libhxcfe.h"

#include "tracks/sector_extractor.h"

#include "tracks/crc.h"
#include "arburg_track.h"

#include "tracks/trackutils.h"
#include "tracks/encoding/fm_encoding.h"

#include "tracks/luts.h"

#include "sector_sm.h"

int arburgsysfmtobin(unsigned char * input_data,int input_data_size,unsigned char * decod_data,int decod_data_size,int bit_offset,int lastbit)
{
	int i;
	int bitshift;
	unsigned char binbyte;

	i=0;
	bitshift=0;
	binbyte=0;
	do
	{

		if( getbit(input_data,(bit_offset+1)%input_data_size) )
		{
			//01 -> 0

			binbyte = (unsigned char)( binbyte & 0xFE );

			bit_offset=(bit_offset+2)%input_data_size;
		}
		else
		{
			//001 -> 1

			binbyte = (unsigned char)( binbyte | 0x01 );

			bit_offset=(bit_offset+3)%input_data_size;
		}

		bitshift++;

		if(bitshift==8)
		{
			decod_data[i]=binbyte;
			bitshift=0;
			binbyte=0;
			i++;
		}
		else
		{
			binbyte = (unsigned char)( binbyte << 1 );
		}

	}while(i<decod_data_size);

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

					sector->endsectorindex = fmtobin(track->databuffer,NULL,track->tracklen,sector->input_data,sector_size,bit_offset,0);

					sector->data_crc = 0x0000;
					sector->use_alternate_data_crc = 0x00;

					for(k=0;k< 1 + 0x9FE + 2;k++)
					{
						sector->input_data[k]=LUT_ByteBitsInverter[sector->input_data[k + 1]];
					}

					checksum = 0;
					for(k=0;k < 0x9FE;k++)
					{
						checksum = (uint16_t)(checksum + sector->input_data[k]);
					}

					if( ((checksum & 0xFF) == sector->input_data[0x9FE]) &&  (((checksum>>8) & 0xFF) == sector->input_data[0x9FF]) )
					{
						floppycontext->hxc_printf(MSG_DEBUG,"get_next_Arburg_sector : Checksum data ok.");
						sector->use_alternate_data_crc = 0x00;
						sector->data_crc = sector->input_data[0x9FE];
						sector->data_crc = sector->data_crc | (sector->input_data[0x9FF]<<8);
					}
					else
					{
						floppycontext->hxc_printf(MSG_DEBUG,"get_next_Arburg_sector : Checksum data Error !");
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
						sector->input_data[k]=LUT_ByteBitsInverter[sector->input_data[k + 1]];
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

int32_t BuildArburgTrack(HXCFE* floppycontext,int32_t tracknumber,int32_t sidenumber,uint8_t * datain,uint8_t * fmdata,int32_t * fmsizebuffer,int32_t trackformat)
{
	/*
		Arburg Track format:
		Sync : 0xFF
		Data : 0x9FE Bytes
		Checksum : 1 Low Byte
		Checksum : 1 High Byte
		Sync? : OxDF Bytes (High bytes checksum)
	*/

	int i,j,k;
	unsigned char *tempdata;
	unsigned char *tempclock;
	int32_t finalsize;
	int32_t current_buffer_size;

	uint16_t checksum;

	int32_t sectorsize;
	int32_t buffersize;


	buffersize=*fmsizebuffer/8;

	sectorsize=0x9FE;

	current_buffer_size=buffersize/4;
	finalsize=1 + sectorsize + 1 + 0xDF;

	if(finalsize<=current_buffer_size)
	{

		j=0;

		tempdata=(unsigned char *)malloc((buffersize/4)+1);
		tempclock=(unsigned char *)malloc((buffersize/4)+1);

		if(tempdata && tempclock)
		{

			memset(tempclock,0xFF,(buffersize/4)+1);
			memset(tempdata, 0x00,(buffersize/4)+1);


			/////////////////////////////////////////////////////////////////////////////////////////////
			//Track GAP
			for(k=0;k<(56*8);k++)
			{
				setfieldbit(tempdata,LUT_ByteBitsInverter[0x00],j,8);
				j=j+8;
			}

			/////////////////////////////////////////////////////////////////////////////////////////////
			//Sector Header
			setfieldbit(tempdata,LUT_ByteBitsInverter[0xFF],j,8);
			j=j+8;

			/////////////////////////////////////////////////////////////////////////////////////////////
			//Sector Data

			checksum = 0;
			for(k=0;k<ARBURB_DATATRACK_SIZE-2;k++)
			{
				checksum = checksum + datain[k];
			}


			for(k=0;k<ARBURB_DATATRACK_SIZE-2;k++)
			{
				setfieldbit(tempdata,LUT_ByteBitsInverter[datain[k]],j,8);
				j=j+8;
			}

			#ifndef IGNORE_ARBURG_CHECKSUM

			for(k=ARBURB_DATATRACK_SIZE-2;k<ARBURB_DATATRACK_SIZE;k++)
			{
				setfieldbit(tempdata,LUT_ByteBitsInverter[datain[k]],j,8);
				j=j+8;
			}

			/////////////////////////////////////////////////////////////////////////////////////////////
			//Checksum
			if( ( datain[ARBURB_DATATRACK_SIZE-2] != (checksum&0xFF) ) || ( datain[ARBURB_DATATRACK_SIZE-1] != (checksum>>8) ) )
			{
				floppycontext->hxc_printf(MSG_WARNING,"BuildArburgTrack : Block Checksum error !");
			}

			#else

			setfieldbit(tempdata,LUT_ByteBitsInverter[checksum&0xFF],j,8);
			j=j+8;
			setfieldbit(tempdata,LUT_ByteBitsInverter[checksum>>8],j,8);
			j=j+8;

			#endif


			if((j/8)<=current_buffer_size)
			{
				for(i=j;i<(current_buffer_size*8);i=i+8)
				{
					if(j+8<(current_buffer_size*8))
						setfieldbit(tempdata,LUT_ByteBitsInverter[checksum>>8],j,8);
					else
						setfieldbit(tempdata,LUT_ByteBitsInverter[checksum>>8],j,(current_buffer_size*8)-j);

					j=j+8;
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
		floppycontext->hxc_printf(MSG_ERROR,"BuildArburgTrack : No enough space on this track !");
		return finalsize;
	}
}

int pushArburgSysByte(unsigned char byte, int bitoffset, unsigned char * track,uint32_t tracksize)
{
	int k;

	for(k=0;k<8;k++)
	{
		if(byte&(0x01<<k))
		{
			bitoffset = (bitoffset + 1) % (tracksize);
			track[bitoffset>>3] = track[bitoffset>>3] & ~(0x80>> (bitoffset&7));
			bitoffset = (bitoffset + 1) % (tracksize);
			track[bitoffset>>3] = track[bitoffset>>3] & ~(0x80>> (bitoffset&7));
			bitoffset = (bitoffset + 1) % (tracksize);
			track[bitoffset>>3] = track[bitoffset>>3] | (0x80>> (bitoffset&7));
		}
		else
		{
			bitoffset = (bitoffset + 1) % (tracksize);
			track[bitoffset>>3] = track[bitoffset>>3] & ~(0x80>> (bitoffset&7));
			bitoffset = (bitoffset + 1) % (tracksize);
			track[bitoffset>>3] = track[bitoffset>>3] | (0x80>> (bitoffset&7));
		}
	}

	return bitoffset;
}

int32_t BuildArburgSysTrack(HXCFE* floppycontext,int32_t tracknumber,int32_t sidenumber,uint8_t* datain, uint8_t * fmdata, int32_t * fmsizebuffer,int32_t trackformat)
{
	/*
		Arburg Track format:
		Sync : 0xFF
		Data : 0xEFE Bytes
		Checksum : 1 Low Byte
		Checksum : 1 High Byte
		Sync? : OxDF Bytes (High bytes checksum)
	*/

	int k;
	uint16_t checksum;
	int bitoffset,newbitoffset;

	bitoffset = 0;

	memset(fmdata, 0x44,*fmsizebuffer/8);

	/////////////////////////////////////////////////////////////////////////////////////////////
	//Track GAP
	for(k=0;k<(60*8);k++)
	{
		bitoffset = pushArburgSysByte(0x00,bitoffset,fmdata,*fmsizebuffer);
	}

	/////////////////////////////////////////////////////////////////////////////////////////////
	//Sector Header
	bitoffset = pushArburgSysByte(0xFF,bitoffset,fmdata,*fmsizebuffer);

	/////////////////////////////////////////////////////////////////////////////////////////////
	//Sector Data
	checksum = 0;
	for(k=0;k<(ARBURB_SYSTEMTRACK_SIZE-2);k++)
	{
		checksum = checksum + datain[k];
	}

	for(k=0;k<(ARBURB_SYSTEMTRACK_SIZE-2);k++)
	{
		bitoffset = pushArburgSysByte(datain[k],bitoffset,fmdata,*fmsizebuffer);
	}

	#ifndef IGNORE_ARBURG_CHECKSUM

	for(k=ARBURB_SYSTEMTRACK_SIZE-2;k<ARBURB_SYSTEMTRACK_SIZE;k++)
	{
		bitoffset = pushArburgSysByte(datain[k],bitoffset,fmdata,*fmsizebuffer);
	}

	if( ( datain[ARBURB_SYSTEMTRACK_SIZE-2] != (checksum&0xFF) ) || ( datain[ARBURB_SYSTEMTRACK_SIZE-1] != (checksum>>8) ) )
	{
		floppycontext->hxc_printf(MSG_WARNING,"BuildArburgSysTrack : Block Checksum error !");
	}

	#else

	// Low byte
	bitoffset = pushArburgSysByte((unsigned char)(checksum&0xFF),bitoffset,fmdata,*fmsizebuffer);
	// High byte
	bitoffset = pushArburgSysByte((unsigned char)(checksum>>8),bitoffset,fmdata,*fmsizebuffer);

	#endif

	newbitoffset = bitoffset;
	do
	{
		bitoffset = newbitoffset;
		newbitoffset = pushArburgSysByte((unsigned char)(checksum>>8),bitoffset,fmdata,*fmsizebuffer);
	}while(newbitoffset > bitoffset );


	return 0;
}
