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
// File : apple_mac_gcr_track.c
// Contains: Apple Macintosh track support.
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

#include "apple2_gcr_track.h"

#include "tracks/trackutils.h"
#include "tracks/encoding/fm_encoding.h"

#include "tracks/luts.h"

#include "sector_sm.h"

/////////////////////////////////////////////////////////
// Apple II Translation tables
/////////////////////////////////////////////////////////
static unsigned char byte_translation_SixAndTwo[] = {
      0x96, 0x97, 0x9a, 0x9b, 0x9d, 0x9e, 0x9f, 0xa6,
      0xa7, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb2, 0xb3,
      0xb4, 0xb5, 0xb6, 0xb7, 0xb9, 0xba, 0xbb, 0xbc,
      0xbd, 0xbe, 0xbf, 0xcb, 0xcd, 0xce, 0xcf, 0xd3,
      0xd6, 0xd7, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde,
      0xdf, 0xe5, 0xe6, 0xe7, 0xe9, 0xea, 0xeb, 0xec,
      0xed, 0xee, 0xef, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6,
      0xf7, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};

static const unsigned char SixAndTwo_to_byte_translation[] = {
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,   /* 0x00 */
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,   /* 0x10 */
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,   /* 0x20 */
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,   /* 0x30 */
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,   /* 0x40 */
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,   /* 0x50 */
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,   /* 0x60 */
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,   /* 0x70 */
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,   /* 0x80 */
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x01,   /* 0x90 */
      0xff, 0xff, 0x02, 0x03, 0xff, 0x04, 0x05, 0x06,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x07, 0x08,   /* 0xA0 */
      0xff, 0xff, 0xff, 0x09, 0x0A, 0x0B, 0x0C, 0x0D,
      0xff, 0xff, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13,   /* 0xB0 */
      0xff, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,   /* 0xC0 */
      0xff, 0xff, 0xff, 0x1B, 0xff, 0x1C, 0x1D, 0x1E,
      0xff, 0xff, 0xff, 0x1F, 0xff, 0xff, 0x20, 0x21,   /* 0xD0 */
      0xff, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
      0xff, 0xff, 0xff, 0xff, 0xff, 0x29, 0x2A, 0x2B,   /* 0xE0 */
      0xff, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32,
      0xff, 0xff, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38,   /* 0xF0 */
      0xff, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F
};

/////////////////////////////////////////////////////////

#if 0
static void NybbleSector6and2( unsigned char * dataIn, unsigned char * nybbleOut)
{
	unsigned short buff1_offset;
	unsigned char byte,i,j;

	buff1_offset = 2;

	for(i=0;i<3;i++)
	{
		for(j=0;j<86;j++)
		{
			buff1_offset = (buff1_offset + 0xFF) & 0xFF;
			byte = dataIn[buff1_offset];
			nybbleOut[buff1_offset] = byte >> 2;
			nybbleOut[j+256] = ( (nybbleOut[j+256]<<2) + ((byte&2)>>1) + ((byte&1)<<1) ) & 0x3F;
		}
	}
}
#endif

int cellstobin(unsigned char * input_data,int input_data_size,unsigned char * decod_data,int decod_data_size,int bit_offset,int lastbit)
{
	int i;
	int bitshift;
	unsigned char binbyte;

	i = 0;
	bitshift = 0;
	binbyte = 0;
	do
	{
		//0C0D0C0D

		binbyte = (unsigned char)( binbyte | (getbit(input_data,(bit_offset+1)%input_data_size)<<3) | getbit(input_data,(bit_offset+3)%input_data_size)<<2) | (getbit(input_data,(bit_offset+5)%input_data_size)<<1) | (getbit(input_data,(bit_offset+7)%input_data_size)<<0);

		bitshift += 4;

		if(bitshift == 8)
		{
			decod_data[i] = binbyte;
			bitshift = 0;
			binbyte = 0;
			i++;
		}
		else
		{
			binbyte = (unsigned char)( binbyte << 4 );
		}

		bit_offset = (bit_offset+8)%input_data_size;

	}while(i<decod_data_size);

	return bit_offset;
}

// Function written by Nathan Woods and R. Belmont
// MESS (sony_denibblize35 / ap_dsk35.cpp)

static void DeNybbleSector6and2(uint8_t *out, const uint8_t *nib_ptr, uint8_t *checksum)
{
	int i, j;
	uint32_t c1,c2,c3,c4;
	uint8_t val;
	uint8_t w1,w2,w3=0,w4;
	uint8_t b1[175],b2[175],b3[175];

	j = 0;
	for (i=0; i<=174; i++)
	{
		w4 = nib_ptr[j++];
		w1 = nib_ptr[j++];
		w2 = nib_ptr[j++];

		if (i != 174) w3 = nib_ptr[j++];

		b1[i] = (w1 & 0x3F) | ((w4 << 2) & 0xC0);
		b2[i] = (w2 & 0x3F) | ((w4 << 4) & 0xC0);
		b3[i] = (w3 & 0x3F) | ((w4 << 6) & 0xC0);
	}

	/* Copy from the user's buffer to our buffer, while computing
	 * the three-byte data checksum
	 */

	i = 0;
	j = 0;
	c1 = 0;
	c2 = 0;
	c3 = 0;
	while (1)
	{
		c1 = (c1 & 0xFF) << 1;
		if (c1 & 0x0100)
			c1++;

		val = (b1[j] ^ c1) & 0xFF;
		c3 += val;
		if (c1 & 0x0100)
		{
			c3++;
			c1 &= 0xFF;
		}
		out[i++] = val;

		val = (b2[j] ^ c3) & 0xFF;
		c2 += val;
		if (c3 > 0xFF)
		{
			c2++;
			c3 &= 0xFF;
		}
		out[i++] = val;

		if (i == 524) break;

		val = (b3[j] ^ c2) & 0xFF;
		c1 += val;
		if (c2 > 0xFF)
		{
			c1++;
			c2 &= 0xFF;
		}
		out[i++] = val;
		j++;
	}
	c4 =  ((c1 & 0xC0) >> 6) | ((c2 & 0xC0) >> 4) | ((c3 & 0xC0) >> 2);
	b3[174] = 0;

	checksum[0] = c1 & 0x3F;
	checksum[1] = c2 & 0x3F;
	checksum[2] = c3 & 0x3F;
	checksum[3] = c4 & 0x3F;
}

// 6 and 2 GCR encoding
int get_next_AppleMacGCR_sector(HXCFE* floppycontext,HXCFE_SIDE * track,HXCFE_SECTCFG * sector,int track_offset)
{
	int bit_offset,old_bit_offset,last_start_offset;
	int sector_size;
	unsigned char fm_buffer[32];
	unsigned char tmp_buffer[32];
	unsigned char CRC16_Low,datachksumerr;
	int sector_extractor_sm;
	int i;
	unsigned char nibble_sector_data[1 + 699 + 3 + 3 ];
	uint8_t checksum[4];

	bit_offset=track_offset;
	memset(sector,0,sizeof(HXCFE_SECTCFG));

	sector_extractor_sm=LOOKFOR_GAP1;

	do
	{
		switch(sector_extractor_sm)
		{
			case LOOKFOR_GAP1:
				// 0xD5 0xAA 0x96
				//    D        5        A        A        9        6
				// 1101     0101     1010     1010     1001     0110
				// 01010001 00010001 01000100 01000100 01000001 00010100
				//     0x51     0x11     0x44     0x44     0x41     0x14

				fm_buffer[0]=0x51;
				fm_buffer[1]=0x11;
				fm_buffer[2]=0x44;
				fm_buffer[3]=0x44;
				fm_buffer[4]=0x41;
				fm_buffer[5]=0x14;

				bit_offset = searchBitStream(track->databuffer,track->tracklen,-1,fm_buffer,8*6,bit_offset);
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

				memset(tmp_buffer,0,sizeof(tmp_buffer));

				bit_offset = chgbitptr( track->tracklen, bit_offset, ( 6 * 8 ));

				sector->endsectorindex = cellstobin(track->databuffer,track->tracklen,tmp_buffer,7,bit_offset,0);

				for(i=0;i<5;i++)
				{
					tmp_buffer[i] = SixAndTwo_to_byte_translation[tmp_buffer[i]];
				}

				CRC16_Low = 0x00;
				for(i=0;i<4;i++)
				{
					CRC16_Low = tmp_buffer[i] ^ CRC16_Low;
				}

				sector->cylinder = ((tmp_buffer[2]&3)<<6) | (tmp_buffer[0]&0x3F);
				sector->head = (tmp_buffer[2]>> 5) & 1;
				sector->sector = tmp_buffer[1];

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

				sector->use_alternate_header_crc = 0x00;

				sector->header_crc = tmp_buffer[4] ;

				if( (CRC16_Low&0x3F) == (tmp_buffer[4]) )
				{ // crc ok !!!

					sector->use_alternate_header_crc = 0x00;
					floppycontext->hxc_printf(MSG_DEBUG,"Valid Apple Macintosh sector header found - Sect:%d",tmp_buffer[2]);
					old_bit_offset=bit_offset;

					// 0xD5 0xAA 0xAD
					//    D        5        A        A        A        D
					// 1101     0101     1010     1010     1010     1101
					// 01010001 00010001 01000100 01000100 01000100 01010001
					//     0x51      0x11    0x44     0x44     0x44     0x51

					fm_buffer[0]=0x51;
					fm_buffer[1]=0x11;
					fm_buffer[2]=0x44;
					fm_buffer[3]=0x44;
					fm_buffer[4]=0x44;
					fm_buffer[5]=0x51;

					bit_offset = chgbitptr( track->tracklen, bit_offset, ( 4 * 8 ));

					bit_offset = searchBitStream(track->databuffer,track->tracklen,64*8,fm_buffer,8*6,bit_offset);
					if( bit_offset!=-1)
					{
						sector_size = 512;
						sector->sectorsize = sector_size;
						sector->trackencoding = APPLEMAC_GCR6A2;

						sector->use_alternate_datamark = 0x00;
						sector->alternate_datamark = 0x00;

						sector->startdataindex=bit_offset;

						sector->input_data =(unsigned char*)malloc(sector_size+12+2);
						if(sector->input_data)
						{
							memset(sector->input_data,0,sector_size+12+2);

							bit_offset = chgbitptr( track->tracklen, bit_offset, ( 6 * 8 ));

							sector->endsectorindex = cellstobin(track->databuffer,track->tracklen,nibble_sector_data,sizeof(nibble_sector_data),bit_offset,0);

							for(i=0;i<sizeof(nibble_sector_data);i++)
							{
								nibble_sector_data[i] = SixAndTwo_to_byte_translation[nibble_sector_data[i]];
							}

							DeNybbleSector6and2(sector->input_data, (uint8_t*)&nibble_sector_data[1], (uint8_t*)&checksum);

							for(i=0;i<512;i++)
							{
								sector->input_data[i] = sector->input_data[i + 12];
							}

							if(
								(checksum[3] == nibble_sector_data[700]) &&
								(checksum[2] == nibble_sector_data[701]) &&
								(checksum[1] == nibble_sector_data[702]) &&
								(checksum[0] == nibble_sector_data[703])
							)
							{
								datachksumerr = 0;
							}
							else
							{
								datachksumerr = 1;
							}

							sector->data_crc = (nibble_sector_data[700] << 24 ) | \
											   (nibble_sector_data[701] << 16 ) | \
											   (nibble_sector_data[702] << 8 ) | \
											   (nibble_sector_data[703] << 0 );

							if(!datachksumerr)
							{ // crc ok !!!
								floppycontext->hxc_printf(MSG_DEBUG,"crc data ok.");
								sector->use_alternate_data_crc = 0x00;
							}
							else
							{
								floppycontext->hxc_printf(MSG_DEBUG,"crc data error!");
								sector->use_alternate_data_crc = 0xFF;
							}

							// "Empty" sector detection
							checkEmptySector(sector);
						}

						bit_offset = chgbitptr( track->tracklen, bit_offset, (sector_size*4));

						sector_extractor_sm=ENDOFSECTOR;

					}
					else
					{
						bit_offset = chgbitptr( track->tracklen, old_bit_offset, 1);
						floppycontext->hxc_printf(MSG_DEBUG,"No data!");
						sector_extractor_sm = ENDOFSECTOR;
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
}

void tg_addAppleMacSectorToTrack(track_generator *tg,HXCFE_SECTCFG * sectorconfig,HXCFE_SIDE * currentside)
{

	int32_t   trackenc;
	unsigned char   sector_buffer[300];
	int32_t   startindex,j,i;
	unsigned char	volume,checksum,nibbleval;

	checksum = 0;
	volume = 254;

	startindex=tg->last_bit_offset/8;

	sectorconfig->startsectorindex=tg->last_bit_offset/8;

	// Sync bytes
	// 14 Self Synch 0xFFs
	for (i = 0; i < 14; i++)
	{
		pushTrackCode(tg,0xFF,0xFF,currentside,DIRECT_ENCODING);
		tg->last_bit_offset += (2*2); // 2 zeros synch
	}

	// Sector header start
	pushTrackCode(tg,0xD5,0xFF,currentside,DIRECT_ENCODING);
	pushTrackCode(tg,0xAA,0xFF,currentside,DIRECT_ENCODING);
	pushTrackCode(tg,0x96,0xFF,currentside,DIRECT_ENCODING);

	checksum = 0;

	pushTrackCode(tg, (unsigned char)((volume >> 1) | 0xaa),0xFF,currentside,DIRECT_ENCODING);
	pushTrackCode(tg, (unsigned char)((volume )     | 0xaa),0xFF,currentside,DIRECT_ENCODING);
	checksum = checksum ^ volume;

	pushTrackCode(tg, (unsigned char)((sectorconfig->cylinder >> 1) | 0xaa),0xFF,currentside,DIRECT_ENCODING);
	pushTrackCode(tg, (unsigned char)((sectorconfig->cylinder )     | 0xaa),0xFF,currentside,DIRECT_ENCODING);
	checksum = checksum ^ sectorconfig->cylinder;

	pushTrackCode(tg, (unsigned char)((sectorconfig->sector >> 1) | 0xaa),0xFF,currentside,DIRECT_ENCODING);
	pushTrackCode(tg, (unsigned char)((sectorconfig->sector )     | 0xaa),0xFF,currentside,DIRECT_ENCODING);
	checksum = checksum ^ sectorconfig->sector;

	pushTrackCode(tg, (unsigned char)((checksum >> 1) | 0xaa), 0xFF, currentside, DIRECT_ENCODING );
	pushTrackCode(tg, (unsigned char)((checksum )     | 0xaa), 0xFF, currentside, DIRECT_ENCODING );

	// Sector header end
	pushTrackCode(tg,0xDE,0xFF,currentside,DIRECT_ENCODING);
	pushTrackCode(tg,0xAA,0xFF,currentside,DIRECT_ENCODING);
	pushTrackCode(tg,0xEB,0xFF,currentside,DIRECT_ENCODING);

	// Sync bytes
	for (i = 0; i < 6; i++)
	{
		pushTrackCode(tg,0xFF,0xFF,currentside,DIRECT_ENCODING);
		tg->last_bit_offset += (2*2); // 2 zeros synch
	}

	// Sector data block start
	pushTrackCode(tg,0xD5,0xFF,currentside,DIRECT_ENCODING);
	pushTrackCode(tg,0xAA,0xFF,currentside,DIRECT_ENCODING);
	pushTrackCode(tg,0xAD,0xFF,currentside,DIRECT_ENCODING);

	sectorconfig->startdataindex=tg->last_bit_offset/8;

	memset(sector_buffer,0x00,300);

	if(sectorconfig->input_data)
	{
		for(i=0;i<256;i++)
		{
			sector_buffer[i] = sectorconfig->input_data[i];
		}
	}
	else
	{
		for(i=0;i<256;i++)
		{
			sector_buffer[i] = sectorconfig->fill_byte;
		}
	}

	checksum = 0;
	for (i = 0; i < 86; i++)
	{
		nibbleval  = ( (sector_buffer[i] & 0x01) << 1 );
		nibbleval |= ( (sector_buffer[i] & 0x02) >> 1 );
		nibbleval |= ( (sector_buffer[i + 86] & 0x01) << 3 );
		nibbleval |= ( (sector_buffer[i + 86] & 0x02) << 1 );
		nibbleval |= ( (sector_buffer[i + 172] & 0x01) << 5 );
		nibbleval |= ( (sector_buffer[i + 172] & 0x02) << 3 );

		pushTrackCode(tg,byte_translation_SixAndTwo[nibbleval ^ checksum],0xFF,currentside,DIRECT_ENCODING);

		checksum = nibbleval;
	}

	for (i = 0; i < 256; i++)
	{
		nibbleval = (sector_buffer[i] >> 2);
		pushTrackCode(tg,byte_translation_SixAndTwo[nibbleval ^ checksum],0xFF,currentside,DIRECT_ENCODING);
		checksum = nibbleval;
	}

	// Push the Checksum
	pushTrackCode(tg,byte_translation_SixAndTwo[checksum],0xFF,currentside,DIRECT_ENCODING);

	// Data block end
	pushTrackCode(tg,0xDE,0xFF,currentside,DIRECT_ENCODING);
	pushTrackCode(tg,0xAA,0xFF,currentside,DIRECT_ENCODING);
	pushTrackCode(tg,0xEB,0xFF,currentside,DIRECT_ENCODING);

	//gap3
	if(sectorconfig->gap3!=255)
	{
		for(i=0;i<sectorconfig->gap3;i++)
		{
			pushTrackCode(tg,0xFF,0xFF,currentside,sectorconfig->trackencoding);
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

	trackenc = APPLEMAC_GCR_ENCODING;

	if(currentside->track_encoding_buffer)
	{
		for(j=startindex;j<(tg->last_bit_offset/8);j++)
		{
			currentside->track_encoding_buffer[j]=trackenc;
		}
	}

	currentside->number_of_sector++;
}
