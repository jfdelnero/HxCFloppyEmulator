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
// File : victor9k_gcr_track.c
// Contains: Victor 9000 GCR track support
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

#include "victor9k_gcr_track.h"

#include "tracks/trackutils.h"
#include "tracks/luts.h"

#include "sector_sm.h"

static const unsigned char gcrencodingtable[16]=
{
	0x0A,0x0B,0x12,0x13,
	0x0E,0x0F,0x16,0x17,
	0x09,0x19,0x1A,0x1B,
	0x0D,0x1D,0x1E,0x15
};

static const unsigned char gcrdecodingtable[32]=
{
	0xFF, // 00000
	0xFF, // 00001
	0xFF, // 00010
	0xFF, // 00011
	0xFF, // 00100
	0xFF, // 00101
	0xFF, // 00110
	0xFF, // 00111

	0xFF, // 01000
	0x08, // 01001
	0x00, // 01010
	0x01, // 01011
	0xFF, // 01100
	0x0C, // 01101
	0x04, // 01110
	0x05, // 01111

	0xFF, // 10000
	0xFF, // 10001
	0x02, // 10010
	0x03, // 10011
	0xFF, // 10100
	0x0F, // 10101
	0x06, // 10110
	0x07, // 10111

	0xFF, // 11000
	0x09, // 11001
	0x0A, // 11010
	0x0B, // 11011
	0xFF, // 11100
	0x0D, // 11101
	0x0E, // 11110
	0xFF  // 11111
};


int victor9kgcrtonibble(unsigned char * input_data,int input_data_size,int bit_offset, unsigned char * out_bin_nibble)
{
	int i,j;
	unsigned char c1;
	unsigned char gcr_nibble;

	i = 0;

	bit_offset %= input_data_size;
	j = bit_offset >> 3;

	gcr_nibble = 0;

	while(i<5)
	{
		gcr_nibble = gcr_nibble << 1;

		c1 = (unsigned char)( input_data[j] & (0x80>>(bit_offset&7)) );
		bit_offset = (bit_offset+2)%input_data_size;
		j = bit_offset>>3;

		if(c1)
			gcr_nibble |= 0x1;

		i++;
	}


	*out_bin_nibble = gcrdecodingtable[gcr_nibble & 0x1F];

	return bit_offset;
}

int victor9kgcrtobyte(unsigned char * input_data,int input_data_size,int bit_offset, unsigned char * out_bin_byte)
{
	unsigned char nibble1,nibble2;

	bit_offset = victor9kgcrtonibble(input_data, input_data_size, bit_offset, &nibble1);
	bit_offset = victor9kgcrtonibble(input_data, input_data_size, bit_offset, &nibble2);

	*out_bin_byte =  ((nibble1<<4)&0xF0) | (nibble2&0xF);

 	return bit_offset;
}

// GCR encoder
unsigned char * BuildVictor9kGCRCylinder(int * gcrtracksize,unsigned char * track,unsigned char * nongcrpart,int size)
{
	int i,j,k,l;

	unsigned char byte,quartet;
	unsigned char gcrcode;
	unsigned char nongcrcode;
	unsigned char shift,shift2;

	int finalsize;
	unsigned char * finalbuffer;

	finalsize=0;
	for(i=0;i<size;i++)
	{
		if(nongcrpart[i])
		{
			finalsize += (4*2);
		}
		else
		{
			finalsize += (5*2);
		}
	}

	finalsize *= 2;
	finalsize /= 8;

	finalbuffer = (unsigned char *)malloc(finalsize);
	if(!finalbuffer)
		return NULL;

	*gcrtracksize = finalsize;

	// Clean up
	for(i=0;i<(finalsize);i++)
	{
		finalbuffer[i]=0x00;
	}

	// GCR Encoding
	shift=0;
	shift2=0;
	k=0;
	j=0;
	l=0;

	while(l<size)
	{
		byte=track[l];
		nongcrcode=nongcrpart[l];

		if( !((nongcrcode>>(4-shift)) & 0xF ) )
		{
			gcrcode = gcrencodingtable[(byte>>(4-shift))&0xF];

			for(j=0;j<5;j++)
			{

				if(gcrcode & (0x10>>j))
				{
					finalbuffer[k] |= (0x80>>shift2);
				}
				else
				{
					finalbuffer[k] &= ~(0xC0>>shift2);
				}

				shift2 += 2;
				if( shift2 == 8 )
				{
					shift2 = 0;
					k++;
				}
			}
		}
		else
		{//non gcr - direct copy
			quartet = (byte>>(4-shift)) & 0xF;

			for(j=0;j<4;j++)
			{
				if(quartet & (0x08>>j))
				{
					finalbuffer[k] |= (0x80>>shift2);
				}
				else
				{
					finalbuffer[k] &= (~(0xC0>>shift2));
				}

				shift2 += 2;
				if(shift2==8)
				{
					shift2=0;
					k++;
				}
			}
		}

		shift += 4;
		if( shift == 8 )
		{
			shift = 0;
			l++;
		}
	};

	return finalbuffer;

}

int32_t BuildVictor9kGCRTrack(int numberofsector,int sectorsize,int tracknumber,int sidenumber,unsigned char* datain,unsigned char * mfmdata,int32_t * mfmsizebuffer)
{
	int i,j,k,l,t;
	unsigned char *tempdata;

	unsigned char *temptrack;
	int temptracksize;
	unsigned char *tempnongcr;
	int32_t finalsize;
	int32_t current_buffer_size;

	/*
	Here's the layout of a standard low-level pattern on a 1541 disk. Use the
	above sample to follow along.

	1. Header sync       FF FF FF FF FF (40 'on' bits, not GCR)
	2. Header info       52 54 B5 29 4B 7A 5E 95 55 55 (10 GCR bytes)
	3. Header gap        55 55 55 55 55 55 55 55 55 (9 bytes, never read)
	4. Data sync         FF FF FF FF FF (40 'on' bits, not GCR)
	5. Data block        55...4A (325 GCR bytes)
	6. Inter-sector gap  55 55 55 55...55 55 (4 to 12 bytes, never read)
	1. Header sync       (SYNC for the next sector)


	The 10 byte header info (#2) is GCR encoded and must be decoded  to  it's
	normal 8 bytes to be understood. Once decoded, its breakdown is as follows:

	Byte   $00 - header block ID ($08)
			01 - header block checksum (EOR of $02-$05)
			02 - Sector
			03 - Track
			04 - Format ID byte #2
			05 - Format ID byte #1
			06-07 - $0F ("off" bytes)

	*/

	finalsize= ( 5 + 8 + 9 + 5 + 260 + 12) * numberofsector;

	current_buffer_size=(int)(*mfmsizebuffer * 0.8);

	if(finalsize<=current_buffer_size)
	{
		j=0;
		tempdata = (unsigned char *)malloc( current_buffer_size + 1);
		tempnongcr = (unsigned char *)malloc( current_buffer_size + 1);
		if(tempdata && tempnongcr)
		{
			memset(tempnongcr, 0, current_buffer_size + 1 );
			memset(tempdata, 0, current_buffer_size + 1 );

			// sectors
			for(l=0;l<numberofsector;l++)
			{
				// sync
				for(k=0;k<5;k++)
				{
					tempdata[j] = 0xFF;
					tempnongcr[j] = 0xFF;
					j++;
				}

				tempdata[j++] = 0x08;             // $00 - header block ID ($08)
				tempdata[j++] = 0x00;             // header block checksum (EOR of $02-$05)
				tempdata[j++] = l;                // Sector
				tempdata[j++] = 1 + tracknumber;  // Track
				tempdata[j++] = 0xA1;             // Format ID byte #2
				tempdata[j++] = 0x1A;             // Format ID byte #1

				tempdata[j++] = 0x0F;  // $0F ("off" bytes)
				tempdata[j++] = 0x0F;  // $0F ("off" bytes)
				tempdata[j-7] = tempdata[j-6] ^ tempdata[j-5] ^ tempdata[j-4] ^ tempdata[j-3];

				for(k=0;k<9;k++) // Header gap
				{
					tempdata[j] = 0x55;
					tempnongcr[j] = 0xFF;
					j++;
				}

				for(k=0;k<5;k++) // Data sync
				{
					tempdata[j] = 0xFF;
					tempnongcr[j] = 0xFF;
					j++;
				}

				tempdata[j] = 0x07;  // data block ID ($07)
				j++;
				t = j + 256;
				for(i=0;i<sectorsize;i++) // data sector & checksum
				{
					tempdata[j] = datain[(sectorsize*l)+i];
					tempdata[t] = tempdata[t]^tempdata[j];
					j++;
				}
				j++; // data checksum (EOR of data sector)

				tempdata[j++] = 0x00;  // $00 ("off" bytes)
				tempdata[j++] = 0x00;  // $00 ("off" bytes)

				for(k=0;k<8;k++) // sector gap
				{
					tempdata[j]=0x55;
					tempnongcr[j]=0xFF;
					j++;
				}
			}

			temptrack = BuildVictor9kGCRCylinder(&temptracksize,tempdata,tempnongcr,j);
			if(temptrack)
			{
				memset(mfmdata,0x22,*mfmsizebuffer);

				if(*mfmsizebuffer>=temptracksize)
				{
					memcpy(mfmdata,temptrack,temptracksize);

				}
				free(temptrack);
			}

			free(tempdata);
			free(tempnongcr);
			return 0;
		}
		else
		{
			if(tempdata)
				free(tempdata);

			if(tempnongcr)
				free(tempnongcr);

			return -1;
		}
	 }
	 else
	 {
		return finalsize;
	 }
}

// Victor 9000 GCR encoding
int get_next_Victor9k_sector(HXCFE* floppycontext,HXCFE_SIDE * track,HXCFE_SECTCFG * sector,int track_offset)
{
	int bit_offset,old_bit_offset;
	int last_start_offset;
	int sector_size;
	unsigned char fm_buffer[32];
	unsigned char tmp_buffer[32];
	unsigned char CRC16_Low;
	int datachksumerr;
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


				// Sector header prolog (0x0xfffffeab)
				// 0xFF 0xFF 0x55  (0xFF 0xFF GCR(0xxx))
				// 1111 1111 1111 1111 01010101
				// 01010101 01010101 01010101 01010101  00010001 00010001
				// 0x55     0x55     0x55     0x55      0x11     0x11

				fm_buffer[0]=0x55;
				fm_buffer[1]=0x55;
				fm_buffer[2]=0x55;
				fm_buffer[3]=0x55;
				fm_buffer[4]=0x55;
				fm_buffer[5]=0x55;
				fm_buffer[6]=0x11;
				fm_buffer[7]=0x11;
				fm_buffer[8]=0x00;

				bit_offset=searchBitStream(track->databuffer,track->tracklen,-1,fm_buffer,(8*8),bit_offset);

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

				bit_offset = chgbitptr( track->tracklen, bit_offset, ( 6 * 8 ) + 1);

				sector->startsectorindex = bit_offset;

				for(i=0;i<6;i++)
				{
					bit_offset = victor9kgcrtobyte(track->databuffer,track->tracklen,bit_offset, &tmp_buffer[i]);
				}

				sector->endsectorindex = bit_offset;

				CRC16_Low = tmp_buffer[1] + tmp_buffer[2] - tmp_buffer[3];
				if( !CRC16_Low )
				{
					sector->cylinder = tmp_buffer[1];
					sector->head = 0;
					sector->sector = tmp_buffer[2];

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

					sector->use_alternate_header_crc = 0x00;
					floppycontext->hxc_printf(MSG_DEBUG,"Valid Victor 9000 GCR sector header found - Sect:%d",tmp_buffer[2]);
					old_bit_offset = bit_offset;

					// Sector header prolog
					// 0xFF 0xFF 0x52  (0xFF 0xFF GCR(0x07)) (00010001000100010101)
					// 1111 1111 1111 1111 01010010
					// 01010101 01010101 01010101 01010101  00010001 00000100
					// 0x55     0x55     0x55     0x55      0x11     0x04

					fm_buffer[0]=0x55;
					fm_buffer[1]=0x55;
					fm_buffer[2]=0x55;
					fm_buffer[3]=0x55;
					fm_buffer[4]=0x55;
					fm_buffer[5]=0x55;
					fm_buffer[6]=0x11;
					fm_buffer[7]=0x04;
					fm_buffer[8]=0x10;

					bit_offset = searchBitStream(track->databuffer,track->tracklen,(64*8),fm_buffer,8*8,bit_offset);

					#ifdef DBG_A2_GCR
					jj = bit_offset;
					#endif

					if((bit_offset-old_bit_offset<((88+10)*8*2)) && bit_offset!=-1)
					{
						sector->startdataindex=bit_offset;

						sector_size = 512;
						sector->sectorsize = sector_size;
						sector->trackencoding = VICTOR9K_GCR;

						sector->use_alternate_datamark = 0x00;
						sector->alternate_datamark = 0x00;

						sector->input_data =(unsigned char*)malloc(sector_size+2);
						if(sector->input_data)
						{
							memset(sector->input_data,0,sector_size+2);

							bit_offset = chgbitptr( track->tracklen, bit_offset, (6*8) + 1);

							bit_offset = victor9kgcrtobyte(track->databuffer,track->tracklen,bit_offset, &sector->input_data[0]);

							datachksumerr = 0;
							for(i=0;i<512 + 2;i++)
							{
								bit_offset = victor9kgcrtobyte(track->databuffer,track->tracklen,bit_offset, &sector->input_data[i]);
								if(i<512)
									datachksumerr += sector->input_data[i];
							}

							datachksumerr &= 0xFFFF;

							datachksumerr -= (sector->input_data[512] + (256*(int)sector->input_data[512+1]));

							sector->endsectorindex = bit_offset;//DeNybbleSector6and2(sector->input_data,track->databuffer,track->tracklen,bit_offset,&datachksumerr);

							sector->data_crc = (sector->input_data[512] + (256*(int)sector->input_data[512+1]));

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
						}
						else
						{
							bit_offset = chgbitptr( track->tracklen, bit_offset, sector_size*4);
						}

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
			break;

			case ENDOFTRACK:

			break;

			default:
				sector_extractor_sm = ENDOFTRACK;
			break;

		}
	}while(	(sector_extractor_sm!=ENDOFTRACK) && (sector_extractor_sm!=ENDOFSECTOR));

	return bit_offset;
}
