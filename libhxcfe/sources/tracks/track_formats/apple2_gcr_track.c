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
// File : apple2_gcr_track.c
// Contains: Apple II track support.
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
static unsigned char byte_translation_SixAndTwo[64] = {
      0x96, 0x97, 0x9a, 0x9b, 0x9d, 0x9e, 0x9f, 0xa6,
      0xa7, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb2, 0xb3,
      0xb4, 0xb5, 0xb6, 0xb7, 0xb9, 0xba, 0xbb, 0xbc,
      0xbd, 0xbe, 0xbf, 0xcb, 0xcd, 0xce, 0xcf, 0xd3,
      0xd6, 0xd7, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde,
      0xdf, 0xe5, 0xe6, 0xe7, 0xe9, 0xea, 0xeb, 0xec,
      0xed, 0xee, 0xef, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6,
      0xf7, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};

static unsigned short byte_read_translation_SixAndTwo[256];

#if 0
static unsigned char byte_translation_FiveAndThree[] = {
      0xAB, 0xAD, 0xAE, 0xAF, 0xB5, 0xB6, 0xB7, 0xBA,
      0xBB, 0xBD, 0xBE, 0xBF, 0xD6, 0xD7, 0xDA, 0xDB,
      0xDD, 0xDE, 0xDF, 0xEA, 0xEB, 0xED, 0xEE, 0xEF,
      0xF5, 0xF6, 0xF7, 0xFA, 0xFB, 0xFD, 0xFE, 0xFF
};
#endif

#if 0
static unsigned short byte_read_translation_FiveAndThree[256];
#endif

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

#if 0
uint8_t kInvDiskBytes62[256];
#define kInvInvalidValue 0xff

void CalcNibbleInvTables(void)
{
	unsigned int i;

	/*
	memset(kInvDiskBytes53, kInvInvalidValue, sizeof(kInvDiskBytes53));
	for (i = 0; i < sizeof(kDiskBytes53); i++)
	{
		kInvDiskBytes53[kDiskBytes53[i]] = i;
	}
	*/

	memset(kInvDiskBytes62, kInvInvalidValue, sizeof(kInvDiskBytes62));
    for (i = 0; i < sizeof(byte_translation_SixAndTwo); i++)
	{
		kInvDiskBytes62[byte_translation_SixAndTwo[i]] = i;
	}
}
#endif

static uint32_t DeNybbleSector6and2(unsigned char * dataOut,unsigned char * input_data,uint32_t intput_data_size,uint32_t bit_offset,unsigned char * crc_error)
{
	unsigned char buff1_offset;
	unsigned char byte;
	unsigned char nibblebuf[512];
	unsigned char nibblebuf2[512];
	unsigned char xor;
	unsigned short word,k;
	int bit_i,i;

	for(i=0;i<256;i++)
	{
		byte_read_translation_SixAndTwo[i] = 0xFFFF;
	}

	for(i=0;i<sizeof(byte_translation_SixAndTwo);i++)
	{
		byte_read_translation_SixAndTwo[byte_translation_SixAndTwo[i]] = i;
	}

	memset(nibblebuf,0,512);

	for(bit_i=0;bit_i<(1 + 342 + 1) * 8;bit_i++)
	{
		if(getbit(input_data,(bit_offset + (bit_i*2) +1)%intput_data_size ) )
		{
			nibblebuf[bit_i>>3] = nibblebuf[bit_i>>3] | (0x80>>(bit_i&7));
		}
	}

	xor = 0;
	k=0;
	for(i=341; i >= 256; i-- )
	{
		word = byte_read_translation_SixAndTwo[nibblebuf[k]];
		k++;

		byte = (word&0xFF) ^ xor;
		nibblebuf2[i] = byte;
		xor = byte;
	}

	for(i = 0; i < 256; i++ )
	{
		word = byte_read_translation_SixAndTwo[nibblebuf[k]];
		k++;

		byte = (word&0xFF) ^ xor;
		nibblebuf2[i] = byte;
		xor = byte;
	}

	word = byte_read_translation_SixAndTwo[nibblebuf[k]];

	if(crc_error)
		*crc_error = ((word&0xFF) ^ xor);

	buff1_offset = 0;
	for (i=0;i<256;i++)
	{
		buff1_offset = (buff1_offset + 85) % 86;
		byte = nibblebuf2[ 256 + buff1_offset];
		nibblebuf2[256 + buff1_offset] = byte >> 2;
		dataOut[i] = (nibblebuf2[i]<<2) | ((byte&2)>>1) | ((byte&1)<<1);
	}

	return (bit_offset + (bit_i*2))%intput_data_size;
}

// 6 and 2 GCR encoding
int get_next_A2GCR2_sector(HXCFE* floppycontext,HXCFE_SIDE * track,HXCFE_SECTCFG * sector,int track_offset)
{
	int bit_offset,old_bit_offset,last_start_offset;
	int sector_size;
	unsigned char fm_buffer[32];
	unsigned char tmp_buffer[32];
	unsigned char CRC16_Low,datachksumerr;
	int sector_extractor_sm;
	int i;

	#ifdef DBG_A2_GCR
	int jj;
	unsigned char test_buffer[2048+1];
	unsigned char test_buffer2[64];
	#endif

	bit_offset=track_offset;
	memset(sector,0,sizeof(HXCFE_SECTCFG));

	sector_extractor_sm=LOOKFOR_GAP1;

	do
	{
		switch(sector_extractor_sm)
		{
			case LOOKFOR_GAP1:

				// Sector header prolog
				// 0xD5 0xAA 0x96
				// 1101 0101 1010 1010 1001 0110
				// 01010001 00010001 01000100 01000100 01000001 00010100
				// 0x51 11 44 44 41 14

				fm_buffer[0]=0x51;
				fm_buffer[1]=0x11;
				fm_buffer[2]=0x44;
				fm_buffer[3]=0x44;
				fm_buffer[4]=0x41;
				fm_buffer[5]=0x14;


				bit_offset=searchBitStream(track->databuffer,track->tracklen,-1,fm_buffer,8*6,bit_offset);
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

				#ifdef DBG_A2_GCR
				i = 0;
				memset(test_buffer,0,sizeof(test_buffer));
				test_buffer[i++] = 'A';
				do
				{
					test_buffer[i] = '0' + getbit(track->databuffer,( (bit_offset + i) % track->tracklen));
					i++;
				}while(i<1024);
				floppycontext->hxc_printf(MSG_DEBUG,test_buffer);
				#endif

				bit_offset = chgbitptr( track->tracklen, bit_offset, ( 6 * 8 ));

				sector->endsectorindex = fmtobin(track->databuffer,NULL,track->tracklen,tmp_buffer,7,bit_offset,0);
				if(1)
				{
					tmp_buffer[0] = LUT_Byte2ShortOddBitsExpander[tmp_buffer[0]>>4]<<1 | (LUT_Byte2ShortOddBitsExpander[tmp_buffer[0]&0xF]);
					tmp_buffer[1] = LUT_Byte2ShortOddBitsExpander[tmp_buffer[1]>>4]<<1 | (LUT_Byte2ShortOddBitsExpander[tmp_buffer[1]&0xF]);
					tmp_buffer[2] = LUT_Byte2ShortOddBitsExpander[tmp_buffer[2]>>4]<<1 | (LUT_Byte2ShortOddBitsExpander[tmp_buffer[2]&0xF]);
					tmp_buffer[3] = LUT_Byte2ShortOddBitsExpander[tmp_buffer[3]>>4]<<1 | (LUT_Byte2ShortOddBitsExpander[tmp_buffer[3]&0xF]);

					#ifdef DBG_A2_GCR
					floppycontext->hxc_printf(MSG_DEBUG,"DBG %.2X %.2X %.2X %.2X %.2X %.2X %.2X",tmp_buffer[0],tmp_buffer[1],tmp_buffer[2],tmp_buffer[3],tmp_buffer[4],tmp_buffer[5],tmp_buffer[6]);
					#endif

					CRC16_Low = 0x00;
					for(i=0;i<4;i++)
					{
						CRC16_Low = tmp_buffer[i] ^ CRC16_Low;
					}

					sector->cylinder = tmp_buffer[1];
					sector->head = 0;
					sector->sector = tmp_buffer[2];

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

					sector->header_crc = tmp_buffer[3] ;


					old_bit_offset=bit_offset;

					if(!CRC16_Low)
					{ // crc ok !!!

						#ifdef DBG_A2_GCR
						i = 0;
 						memset(test_buffer,0,sizeof(test_buffer));
						test_buffer[i++] = 'D';
						do
						{
							test_buffer[i] = '0' + getbit(track->databuffer,( (bit_offset + i) % track->tracklen));
							i++;
						}while(i<1024);
						floppycontext->hxc_printf(MSG_DEBUG,test_buffer);
						#endif

						// Sector header epilog
						// 0xDE 0xAA 0xEB
						// 1101 1110 1010 1010 1110 1011
						// 01010001 01010100 01000100 01000100 01010100 01000101
						// 0x51 54 44 44 54 45

						fm_buffer[0]=0x51;
						fm_buffer[1]=0x54;
						fm_buffer[2]=0x44;
						fm_buffer[3]=0x44;
						fm_buffer[4]=0x54;
						fm_buffer[5]=0x45;

						bit_offset = searchBitStream(track->databuffer,track->tracklen,64*8,fm_buffer,(8*4),bit_offset );

						if(bit_offset!=-1)
						{
							bit_offset = chgbitptr( track->tracklen, bit_offset, ( 6 * 8 ));
							sector->use_alternate_header_crc = 0x00;
							floppycontext->hxc_printf(MSG_DEBUG,"Valid Apple II 6 and 2 GCR sector header found - Sect:%d",tmp_buffer[2]);
							old_bit_offset = bit_offset;

							// 0xD5 0xAA 0xAD
							// 1101 0101 1010 1010 1010 1101
							// 01010001 00010001 01000100 01000100 01000100 01010001
							// 0x51 11 44 44 44 51
							fm_buffer[0]=0x51;
							fm_buffer[1]=0x11;
							fm_buffer[2]=0x44;
							fm_buffer[3]=0x44;
							fm_buffer[4]=0x44;
							fm_buffer[5]=0x51;

							bit_offset = searchBitStream(track->databuffer,track->tracklen,64*8,fm_buffer,8*6,bit_offset);

							#ifdef DBG_A2_GCR
							jj = bit_offset;
							#endif

							if( bit_offset!=-1 )
							{

								sector_size = 256;
								sector->sectorsize = sector_size;
								sector->trackencoding = APPLE2_GCR6A2;

								sector->use_alternate_datamark = 0x00;
								sector->alternate_datamark = 0x00;

								sector->startdataindex=bit_offset;

								sector->input_data =(unsigned char*)malloc(sector_size+2);
								if(sector->input_data)
								{
									memset(sector->input_data,0,sector_size+2);

									bit_offset = chgbitptr( track->tracklen, bit_offset, (6*8));

									sector->endsectorindex = DeNybbleSector6and2(sector->input_data,track->databuffer,track->tracklen,bit_offset,&datachksumerr);

									if(datachksumerr)
									{
										// TOCHECK : Some disk appears to need this... Is it a kind of copy protection ?
										bit_offset = chgbitptr( track->tracklen, bit_offset,  2);
										sector->endsectorindex = DeNybbleSector6and2(sector->input_data,track->databuffer,track->tracklen,bit_offset,&datachksumerr);
									}

									sector->data_crc = datachksumerr;

									if(!datachksumerr)
									{ // crc ok !!!

										#ifdef DBG_A2_GCR
										floppycontext->hxc_printf(MSG_DEBUG,"DBG2 %d",i);
										i = 0;
										memset(test_buffer,0,sizeof(test_buffer));
										strcpy(test_buffer,"TST: ");
										do
										{
											sprintf(test_buffer2,"%d",getbit(track->databuffer,( (jj + i) % track->tracklen)));
											strcat(test_buffer, test_buffer2);
											i++;
										}while(i<1024);
										floppycontext->hxc_printf(MSG_DEBUG,test_buffer);

										i = 0;
										memset(test_buffer,0,sizeof(test_buffer));
										strcpy(test_buffer,"TST2: ");
										do
										{
											sprintf(test_buffer2,"%d",getbit(track->databuffer,( (bit_offset + i) % track->tracklen)));
											strcat(test_buffer, test_buffer2);
											i++;
										}while(i<1024);
										floppycontext->hxc_printf(MSG_DEBUG,test_buffer);
										#endif

										floppycontext->hxc_printf(MSG_DEBUG,"crc data ok.");
										sector->use_alternate_data_crc = 0x00;
									}
									else
									{
										floppycontext->hxc_printf(MSG_DEBUG,"crc data error!");
										sector->use_alternate_data_crc = 0xFF;
									}
								}

								// "Empty" sector detection
								checkEmptySector(sector);

								bit_offset = chgbitptr( track->tracklen, bit_offset, sector_size*4 );

								sector_extractor_sm = ENDOFSECTOR;

							}
							else
							{
								bit_offset = chgbitptr( track->tracklen, old_bit_offset, 1 );

								floppycontext->hxc_printf(MSG_DEBUG,"No data!");
								sector_extractor_sm=ENDOFSECTOR;
							}
						}
						else
						{
							bit_offset = chgbitptr( track->tracklen, old_bit_offset, 1 );

							floppycontext->hxc_printf(MSG_DEBUG,"No data! 2");
							sector_extractor_sm=ENDOFSECTOR;
							sector->use_alternate_data_crc = 0xFF;
						}
					}
					else
					{
						bit_offset = chgbitptr( track->tracklen, bit_offset, 1 );

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
					bit_offset = chgbitptr( track->tracklen, bit_offset, 1 );
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

// TODO : older 5 and 3 GCR encoding
int get_next_A2GCR1_sector(HXCFE* floppycontext,HXCFE_SIDE * track,HXCFE_SECTCFG * sector,int track_offset)
{
	return -1;
}

void tg_addAppleSectorToTrack(track_generator *tg,HXCFE_SECTCFG * sectorconfig,HXCFE_SIDE * currentside)
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

	trackenc = APPLEII_GCR2_ENCODING;

	if(currentside->track_encoding_buffer)
	{
		for(j=startindex;j<(tg->last_bit_offset/8);j++)
		{
			currentside->track_encoding_buffer[j]=trackenc;
		}
	}

	currentside->number_of_sector++;
}
