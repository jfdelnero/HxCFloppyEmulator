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
// File : track_generator.c
// Contains: ISO/IBM/Amiga track builder/encoder
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
#include "track_generator.h"
#include "libhxcfe.h"

#include "crc.h"
#include "floppy_utils.h"
#include "math.h"

#include "tracks/track_formats/aed6200p_track.h"
#include "tracks/track_formats/amiga_mfm_track.h"
#include "tracks/track_formats/apple2_gcr_track.h"
#include "tracks/track_formats/apple_mac_gcr_track.h"
#include "tracks/track_formats/arburg_track.h"
#include "tracks/track_formats/c64_gcr_track.h"
#include "tracks/track_formats/dec_rx02_track.h"
#include "tracks/track_formats/emu_emulator_fm_track.h"
#include "tracks/track_formats/heathkit_fm_track.h"
#include "tracks/track_formats/iso_ibm_fm_track.h"
#include "tracks/track_formats/iso_ibm_mfm_track.h"
#include "tracks/track_formats/membrain_mfm_track.h"
#include "tracks/track_formats/northstar_mfm_track.h"
#include "tracks/track_formats/tycom_fm_track.h"
#include "tracks/track_formats/micraln_fm_track.h"

#include "tracks/encoding/mfm_encoding.h"
#include "tracks/encoding/fm_encoding.h"
#include "tracks/encoding/dec_m2fm_encoding.h"

#include "trackutils.h"

#include "track_types_defs.h"

#include "luts.h"

typedef struct gap3conf_
{
	uint8_t  trackmode;
	uint16_t sectorsize;
	uint8_t  numberofsector;
	uint8_t  gap3;
}gap3conf;

static gap3conf std_gap3_tab[]=
{
	{IBMFORMAT_DD, 256 ,0x12,0x0C},
	{IBMFORMAT_DD, 256 ,0x10,0x32},
	{IBMFORMAT_DD, 512 ,0x08,0x50},
	{IBMFORMAT_DD, 512 ,0x09,0x50},
	{IBMFORMAT_DD, 1024,0x04,0xF0},
	{IBMFORMAT_DD, 2048,0x02,0xF0},
	{IBMFORMAT_DD, 4096,0x01,0xF0},
	{IBMFORMAT_DD, 256 ,0x1A,0x36},
	{IBMFORMAT_DD, 512 ,0x0F,0x54},
	{IBMFORMAT_DD, 512 ,0x12,0x6C},
	{IBMFORMAT_DD, 1024,0x08,0x74},
	{IBMFORMAT_DD, 2048,0x04,0xF0},
	{IBMFORMAT_DD, 4096,0x02,0xF0},
	{IBMFORMAT_DD, 8192,0x01,0xF0},
	{IBMFORMAT_DD, 512 ,0x24,0x53},

	//8"FM
	{IBMFORMAT_SD, 128 ,0x1A,0x1B},
	{IBMFORMAT_SD, 256 ,0x0F,0x2A},
	{IBMFORMAT_SD, 512 ,0x08,0x3A},
	{IBMFORMAT_SD,1024 ,0x04,0x8A},
	{IBMFORMAT_SD,2048 ,0x02,0xF8},
	{IBMFORMAT_SD,4096 ,0x01,0xF8},

	//8"MFM
	{IBMFORMAT_DD, 256 ,0x1A,0x36},
	{IBMFORMAT_DD, 512 ,0x0F,0x54},
	{IBMFORMAT_DD,1024 ,0x08,0x74},
	{IBMFORMAT_DD,2048 ,0x04,0xF8},
	{IBMFORMAT_DD,4096 ,0x02,0xF8},
	{IBMFORMAT_DD,8192 ,0x01,0xF8},

	//5"FM
	{IBMFORMAT_SD, 128 ,0x12,0x09},
	{IBMFORMAT_SD, 128 ,0x10,0x19},
	{IBMFORMAT_SD, 256 ,0x08,0x30},
	{IBMFORMAT_SD, 512 ,0x04,0x87},
	{IBMFORMAT_SD,1024 ,0x02,0xF8},
	{IBMFORMAT_SD,2048 ,0x01,0xF8},

	//5"MFM
	{IBMFORMAT_DD, 256 ,0x12,0x0C},
	{IBMFORMAT_DD, 256 ,0x10,0x32},
	{IBMFORMAT_DD, 512 ,0x08,0x50},
	{IBMFORMAT_DD, 512 ,0x09,0x40},
	{IBMFORMAT_DD, 512 ,0x0A,0x10},
	{IBMFORMAT_DD,1024 ,0x04,0xF0},
	{IBMFORMAT_DD,2048 ,0x02,0xF8},
	{IBMFORMAT_DD,4096 ,0x01,0xF8},
///////

	{ISOFORMAT_DD, 256 ,0x12,0x0C},
	{ISOFORMAT_DD, 256 ,0x10,0x32},
	{ISOFORMAT_DD, 512 ,0x08,0x50},
	{ISOFORMAT_DD, 512 ,0x09,0x50},
	{ISOFORMAT_DD, 1024,0x04,0xF0},
	{ISOFORMAT_DD, 2048,0x02,0xF0},
	{ISOFORMAT_DD, 4096,0x01,0xF0},
	{ISOFORMAT_DD, 256 ,0x1A,0x36},
	{ISOFORMAT_DD, 512 ,0x0F,0x54},
	{ISOFORMAT_DD, 512 ,0x12,0x6C},
	{ISOFORMAT_DD, 1024,0x08,0x74},
	{ISOFORMAT_DD, 2048,0x04,0xF0},
	{ISOFORMAT_DD, 4096,0x02,0xF0},
	{ISOFORMAT_DD, 8192,0x01,0xF0},
	{ISOFORMAT_DD, 512, 0x24,0x53},

	//8"FM
	{ISOFORMAT_SD, 128 ,0x1A,0x1B},
	{ISOFORMAT_SD, 256 ,0x0F,0x2A},
	{ISOFORMAT_SD, 512 ,0x08,0x3A},
	{ISOFORMAT_SD,1024 ,0x04,0x8A},
	{ISOFORMAT_SD,2048 ,0x02,0xF8},
	{ISOFORMAT_SD,4096 ,0x01,0xF8},

	//8"MFM
	{ISOFORMAT_DD, 256 ,0x1A,0x36},
	{ISOFORMAT_DD, 512 ,0x0F,0x54},
	{ISOFORMAT_DD,1024 ,0x08,0x74},
	{ISOFORMAT_DD,2048 ,0x04,0xF8},
	{ISOFORMAT_DD,4096 ,0x02,0xF8},
	{ISOFORMAT_DD,8192 ,0x01,0xF8},

	//5"FM
	{ISOFORMAT_SD, 128 ,0x12,0x09},
	{ISOFORMAT_SD, 128 ,0x10,0x19},
	{ISOFORMAT_SD, 256 ,0x08,0x30},
	{ISOFORMAT_SD, 512 ,0x04,0x87},
	{ISOFORMAT_SD,1024 ,0x02,0xF8},
	{ISOFORMAT_SD,2048 ,0x01,0xF8},

	//5"MFM
	{ISOFORMAT_DD, 256 ,0x12,0x0C},
	{ISOFORMAT_DD, 256 ,0x10,0x32},
	{ISOFORMAT_DD, 512 ,0x08,0x50},
	{ISOFORMAT_DD, 512 ,0x09,0x40},
	{ISOFORMAT_DD, 512 ,0x0A,0x10},
	{ISOFORMAT_DD,1024 ,0x04,0xF0},
	{ISOFORMAT_DD,2048 ,0x02,0xF8},
	{ISOFORMAT_DD,4096 ,0x01,0xF8},

	{0xFF,0xFFFF,0xFF,0xFF}
};

void getDirectcode(track_generator *tg,unsigned char data,unsigned char * dstbuf)
{
	uint16_t * direct_code;
	unsigned char k,i;

	if(tg)
	{
		direct_code=(uint16_t *)dstbuf;

		*direct_code=0;
		for(k=0;k<2;k++)
		{
			*direct_code = (uint16_t)(*direct_code >> 8);

			////////////////////////////////////
			// data
			for(i=0;i<4;i++)
			{
				if(data&(0x80>>(i+(k*4)) ))
				{
					*direct_code = (uint16_t)(*direct_code | ((0x80>>(i*2))<<8));
				}
			}
		}
	}
	return;
}

int32_t pushTrackCode(track_generator *tg,uint8_t data,uint8_t clock,HXCFE_SIDE * side,int32_t trackencoding)
{
	int i;
	unsigned char tmp_encoded_data[4];

	switch(trackencoding)
	{
		case IBMFORMAT_SD:
		case ISOFORMAT_SD:
		case TYCOMFORMAT_SD:
		case HEATHKIT_HS_SD:
		case DECRX02_SDDD:
		case MICRALN_HS_SD:
			if( tg->last_bit_offset & 7)
			{
				getFMcode(tg,data,clock,(unsigned char*)&tmp_encoded_data);

				for(i=0;i<(4*8);i++)
				{
					if( tg->last_bit_offset < side->tracklen )
					{
						setbit(side->databuffer, tg->last_bit_offset , getbit(tmp_encoded_data,i) );
						tg->last_bit_offset++;
					}
				}
			}
			else
			{
				if( tg->last_bit_offset < side->tracklen )
				{
					getFMcode(tg,data,clock,&side->databuffer[tg->last_bit_offset/8]);
					tg->last_bit_offset += (4*8);
				}
			}
		break;

		case IBMFORMAT_DD:
		case ISOFORMAT_DD:
		case ISOFORMAT_DD11S:
		case AMIGAFORMAT_DD:
		case MEMBRAINFORMAT_DD:
		case UKNCFORMAT_DD:
		case AED6200P_DD:
		case NORTHSTAR_HS_DD:
			if( tg->last_bit_offset & 7)
			{
				getMFMcode(tg,data,clock,(unsigned char*)&tmp_encoded_data);

				for(i=0;i<(2*8);i++)
				{
					if( tg->last_bit_offset < side->tracklen )
					{
						setbit(side->databuffer, tg->last_bit_offset , getbit(tmp_encoded_data,i) );
						tg->last_bit_offset++;
					}
				}
			}
			else
			{
				if( tg->last_bit_offset < side->tracklen )
				{
					getMFMcode(tg,data,clock,&side->databuffer[tg->last_bit_offset/8]);
					tg->last_bit_offset += (2*8);
				}
			}
		break;

		case APPLE2_GCR5A3:
		case APPLE2_GCR6A2:
		case APPLEMAC_GCR6A2:
		case DIRECT_ENCODING:
			if( tg->last_bit_offset & 7)
			{
				getDirectcode(tg,data,(unsigned char*)&tmp_encoded_data);

				for(i=0;i<(2*8);i++)
				{
					if( tg->last_bit_offset < side->tracklen )
					{
						setbit(side->databuffer, tg->last_bit_offset , getbit(tmp_encoded_data,i) );
						tg->last_bit_offset++;
					}
				}
			}
			else
			{
				if( tg->last_bit_offset < side->tracklen )
				{
					getDirectcode(tg,data,&side->databuffer[tg->last_bit_offset/8]);
					tg->last_bit_offset += (2*8);
				}
			}

		break;

		default:
		break;
	}
	return 0;
}

void FastMFMFMgenerator(track_generator *tg,HXCFE_SIDE * side,unsigned char * track_data,int size,int trackencoding)
{
	switch(trackencoding)
	{
		case IBMFORMAT_SD:
		case ISOFORMAT_SD:
		case TYCOMFORMAT_SD:
			FastFMgenerator(tg,side,track_data,size);
		break;

		case IBMFORMAT_DD:
		case ISOFORMAT_DD:
		case ISOFORMAT_DD11S:
		case MEMBRAINFORMAT_DD:
		case UKNCFORMAT_DD:
		case AED6200P_DD:
			FastMFMgenerator(tg,side,track_data,size);
		break;

		case AMIGAFORMAT_DD:
			FastAmigaMFMgenerator(tg,side,track_data,size);
		break;

		default:
		break;
	}
	return;
}

unsigned char* compute_interleave_tab(int interleave,int skew,int numberofsector)
{

	int i,j;
	uint8_t *interleave_tab;
	uint8_t *allocated_tab;

	interleave_tab = NULL;
	allocated_tab = NULL;

	if( numberofsector > 0 )
	{
		interleave_tab = (uint8_t *)malloc(numberofsector*sizeof(uint8_t));
		if( !interleave_tab )
			goto error;

		memset(interleave_tab,0,numberofsector*sizeof(uint8_t));

		allocated_tab = (uint8_t *)malloc(numberofsector*sizeof(uint8_t));
		if( !allocated_tab )
			goto error;

		memset(allocated_tab,0,numberofsector*sizeof(uint8_t));

		j=skew%(numberofsector);
		i=0;
		do
		{
			while(allocated_tab[j])
			{
				j=(j+1)%(numberofsector);
			}

			interleave_tab[j] = (uint8_t)i;
			allocated_tab[j] = 0xFF;

			j=(j+interleave)%(numberofsector);
			i++;
		}while(i<numberofsector);

		free(allocated_tab);
	}

	return interleave_tab;

error:
	if( interleave_tab )
		free(interleave_tab);

	if( allocated_tab )
		free( allocated_tab );

	return NULL;
}

int searchgap3value(unsigned int numberofsector,HXCFE_SECTCFG * sectorconfigtab)
{
	unsigned int i,j;
	unsigned char gap3;

	gap3=0xFF;
	j=0;
	do
	{
		i=0;
		while( (i<numberofsector) && ( (std_gap3_tab[j].trackmode==sectorconfigtab[i].trackencoding) && (std_gap3_tab[j].sectorsize==sectorconfigtab[i].sectorsize) && (std_gap3_tab[j].numberofsector==numberofsector)) )
		{
			i++;
		}

		if(i==numberofsector)
		{
			gap3=std_gap3_tab[j].gap3;
			return gap3;
		}
		j++;
	}while((i<numberofsector) && (std_gap3_tab[j].trackmode!=0xFF) && gap3==0xFF);

	return -1;
}
void tg_initTrackEncoder(track_generator *tg)
{
	int i,j;

	memset(tg,0,sizeof(track_generator));
	tg->mfm_last_bit=0xFFFF;

	i = 0;
	while( formatstab[i].indexformat )
	{
		i++;
	}

	for(j=0;j<256;j++)
	{
		tg->disk_formats_LUT[j] = (void*)&formatstab[i];
	}

	i = 0;
	while( formatstab[i].indexformat )
	{
		if(formatstab[i].indexformat < 256)
		{
			tg->disk_formats_LUT[formatstab[i].indexformat] = (void*)&formatstab[i];
		}
		i++;
	}

}

int32_t tg_timeToSize(track_generator *tg,int32_t trackencoding,int32_t bitrate,int32_t time)
{
	int cellsize;

	switch(trackencoding)
	{
		case IBMFORMAT_SD:
		case ISOFORMAT_SD:
		case TYCOMFORMAT_SD:
		case DECRX02_SDDD:
			cellsize = 4 * 8;
			break;

		case IBMFORMAT_DD:
		case ISOFORMAT_DD:
		case ISOFORMAT_DD11S:
		case MEMBRAINFORMAT_DD:
		case UKNCFORMAT_DD:
		case AED6200P_DD:
			cellsize = 2 * 8;
			break;

		default:
			cellsize = 2 * 8;
			break;
	}

	return (int32_t)( ( ((float)((float)time/(float)1000000) * (float)bitrate*2) ) / (float)cellsize );
}

int32_t tg_computeMinTrackSize(track_generator *tg,int32_t trackencoding,int32_t bitrate,int32_t numberofsector,HXCFE_SECTCFG * sectorconfigtab,int32_t pregaplen,int32_t * track_period)
{
	int32_t j;
	int32_t tck_period;
	isoibm_config * configptr;
	int32_t total_track_size,sector_size,track_size;
	unsigned char gap3;

	total_track_size = 0;
	if(tg)
	{
		tck_period=0;

		configptr = tg->disk_formats_LUT[trackencoding];

		total_track_size=(configptr->len_gap4a+pregaplen)+configptr->len_isync+configptr->len_indexmarkp1+configptr->len_indexmarkp2 + \
						 configptr->len_gap1;

		switch(trackencoding)
		{
			case IBMFORMAT_SD:
			case ISOFORMAT_SD:
			case TYCOMFORMAT_SD:
			case DECRX02_SDDD:
				total_track_size=total_track_size*4;
				break;

			case IBMFORMAT_DD:
			case ISOFORMAT_DD:
			case ISOFORMAT_DD11S:
			case MEMBRAINFORMAT_DD:
			case UKNCFORMAT_DD:
			case AED6200P_DD:
				total_track_size=total_track_size*2;
				break;

			default:
				total_track_size=total_track_size*2;
				break;
		}

		if(total_track_size && bitrate)
 			tck_period=tck_period+ (int)( (float)100000 * ((float)(total_track_size*4)/(float)bitrate));

		for(j=0;j<numberofsector;j++)
		{
			// if gap3 is set to "to be computed" we consider it as zero for the moment...
			if(sectorconfigtab[j].gap3==255)
			{
				gap3=0;
			}
			else
			{
				gap3 = (uint8_t)(sectorconfigtab[j].gap3);
			}

			configptr = tg->disk_formats_LUT[sectorconfigtab[j].trackencoding];

			sector_size = sectorconfigtab[j].sectorsize;
			if(sectorconfigtab[j].trackencoding == TYCOMFORMAT_SD || sectorconfigtab[j].trackencoding == DECRX02_SDDD)
				sector_size = 128;

			track_size=(configptr->len_ssync+configptr->len_addrmarkp1+configptr->len_addrmarkp2 + 2 +configptr->len_gap2 +configptr->len_dsync+configptr->len_datamarkp1+configptr->len_datamarkp2+2+gap3+configptr->posthcrc_len+configptr->postdcrc_len);
			track_size=track_size+sector_size + 2;

			if(configptr->sector_id) track_size++;
			if(configptr->sector_size_id) track_size++;
			if(configptr->side_id) track_size++;
			if(configptr->track_id) track_size++;

			switch(sectorconfigtab[j].trackencoding)
			{
				case IBMFORMAT_SD:
				case ISOFORMAT_SD:
				case TYCOMFORMAT_SD:
				case DECRX02_SDDD:
					track_size=track_size*4;
					break;

				case IBMFORMAT_DD:
				case ISOFORMAT_DD:
				case ISOFORMAT_DD11S:
				case MEMBRAINFORMAT_DD:
				case UKNCFORMAT_DD:
				case AED6200P_DD:
					track_size=track_size*2;
					break;

				default:
					track_size=track_size*2;
					break;
			}

			total_track_size=total_track_size+track_size;

			if( sectorconfigtab[0].bitrate && track_size )
				tck_period=tck_period+ (int)( (float)10000000 * ((float)(track_size*4)/(float)(sectorconfigtab[0].bitrate*100)));
		}

		if(track_period)
			*track_period=tck_period;

		total_track_size=total_track_size*8;
	}

	return total_track_size;
}

HXCFE_SIDE * tg_initTrack(track_generator *tg,int32_t tracksize,int32_t numberofsector,int32_t trackencoding,int32_t bitrate,HXCFE_SECTCFG * sectorconfigtab,int32_t pregap)
{
	HXCFE_SIDE * currentside;
	int variable_param,tracklen;
	int32_t i;
	int32_t startindex;
	isoibm_config * configptr;

	startindex=tg->last_bit_offset/8;

	currentside = (HXCFE_SIDE*)malloc(sizeof(HXCFE_SIDE));
	if ( !currentside )
		return NULL;

	memset(currentside,0,sizeof(HXCFE_SIDE));

	currentside->number_of_sector = numberofsector;

	tracklen=tracksize/8;
	if(tracksize&7) tracklen++;

	currentside->tracklen=tracksize;

	configptr = tg->disk_formats_LUT[trackencoding];

	if(numberofsector)
	{
		//////////////////////////////
		// bitrate buffer allocation
		variable_param=0;
		currentside->bitrate = sectorconfigtab[0].bitrate;
		i=0;
		while(i<currentside->number_of_sector && !variable_param)
		{
			if(sectorconfigtab[i].bitrate != sectorconfigtab[0].bitrate)
			{
				variable_param = 1;
				currentside->bitrate = VARIABLEBITRATE;

				currentside->timingbuffer = malloc(tracklen*sizeof(uint32_t));
				if(!currentside->timingbuffer)
					goto alloc_error;

				memset(currentside->timingbuffer,0,tracklen*sizeof(uint32_t));
			}
			i++;
		}

		///////////////////////////////////////////
		// track encoding code buffer allocation
		variable_param=0;
		currentside->track_encoding=sectorconfigtab[0].trackencoding;
		i=0;
		while(i<currentside->number_of_sector && !variable_param)
		{
			if(sectorconfigtab[i].trackencoding!=sectorconfigtab[0].trackencoding)
			{
				variable_param=1;
				currentside->track_encoding=VARIABLEENCODING;

				currentside->track_encoding_buffer = malloc(tracklen*sizeof(unsigned char));
				if(!currentside->track_encoding_buffer)
					goto alloc_error;

				memset(currentside->track_encoding_buffer,0,tracklen*sizeof(unsigned char));
			}
			i++;
		}

		///////////////////////////////////////////
		// weak bits buffer allocation
		variable_param=0;
		currentside->flakybitsbuffer=0;

		i=0;
		while(i<currentside->number_of_sector && !variable_param)
		{
			if( sectorconfigtab[i].weak_bits_mask )
			{
				variable_param=1;

				if( !currentside->flakybitsbuffer )
				{
					currentside->flakybitsbuffer = malloc(tracklen*sizeof(unsigned char));
					if(!currentside->flakybitsbuffer)
						goto alloc_error;
				}

				memset(currentside->flakybitsbuffer,0,tracklen*sizeof(unsigned char));
			}
			i++;
		}
	}
	else
	{
		currentside->bitrate = bitrate;
		currentside->track_encoding = trackencoding;
	}

	/////////////////////////////
	// data buffer allocation
	currentside->databuffer = malloc(tracklen);
	if(!currentside->databuffer)
		goto alloc_error;

	memset(currentside->databuffer,0,tracklen);

	/////////////////////////////
	// index buffer allocation
	currentside->indexbuffer=malloc(tracklen);
	if(!currentside->indexbuffer)
		goto alloc_error;

	memset(currentside->indexbuffer,0,tracklen);

	if(numberofsector)
	{
		//gap4a (post index gap4)
		for(i=0;i< ( configptr->len_gap4a + pregap );i++)
		{
			pushTrackCode(tg,configptr->data_gap4a,0xFF,currentside,trackencoding);
		}

		//i sync
		for(i=0;i<configptr->len_isync;i++)
		{
			pushTrackCode(tg,configptr->data_isync,0xFF,currentside,trackencoding);
		}

		// index mark
		for(i=0;i<configptr->len_indexmarkp1;i++)
		{
			pushTrackCode(tg,configptr->data_indexmarkp1,configptr->clock_indexmarkp1,currentside,trackencoding);
		}

		for(i=0;i<configptr->len_indexmarkp2;i++)
		{
			pushTrackCode(tg,configptr->data_indexmarkp2,configptr->clock_indexmarkp2,currentside,trackencoding);
		}

		// gap1
		for(i=0;i<configptr->len_gap1;i++)
		{
			pushTrackCode(tg,configptr->data_gap1,0xFF,currentside,trackencoding);
		}
	}

	currentside->tracklen=tracksize;

	switch(trackencoding)
	{
		case IBMFORMAT_SD:
		case ISOFORMAT_SD:
		case TYCOMFORMAT_SD:
			currentside->track_encoding = ISOIBM_FM_ENCODING;
		break;

		case ISOFORMAT_DD11S:
		case IBMFORMAT_DD:
		case ISOFORMAT_DD:
		case UKNCFORMAT_DD:
			currentside->track_encoding = ISOIBM_MFM_ENCODING;
		break;

		case MEMBRAINFORMAT_DD:
			currentside->track_encoding = MEMBRAIN_MFM_ENCODING;
		break;

		case EMUFORMAT_SD:
			currentside->track_encoding = EMU_FM_ENCODING;
		break;

		case APPLE2_GCR5A3:
			currentside->track_encoding = APPLEII_GCR1_ENCODING;
		break;

		case APPLE2_GCR6A2:
			currentside->track_encoding = APPLEII_GCR2_ENCODING;
		break;

		case APPLEMAC_GCR6A2:
			currentside->track_encoding = APPLEMAC_GCR_ENCODING;
		break;

		case AMIGAFORMAT_DD:
			currentside->track_encoding = AMIGA_MFM_ENCODING;
		break;

		case AED6200P_DD:
			currentside->track_encoding = AED6200P_MFM_ENCODING;
		break;

		case DECRX02_SDDD:
			currentside->track_encoding = DEC_RX02_M2FM_ENCODING;
		break;

		default:
			currentside->track_encoding = ISOIBM_MFM_ENCODING;
		break;
	}

	// fill timing & encoding buffer
	if(currentside->timingbuffer)
	{
		for(i=startindex;i<(tg->last_bit_offset/8);i++)
		{
			currentside->timingbuffer[i]=sectorconfigtab[0].bitrate;
		}
	}

	if(currentside->track_encoding_buffer)
	{
		for(i=startindex;i<(tg->last_bit_offset/8);i++)
		{
			currentside->track_encoding_buffer[i] = (uint8_t)(currentside->track_encoding);
		}
	}

	return currentside;

alloc_error:
	if( currentside->timingbuffer )
		free( currentside->timingbuffer );

	if( currentside->track_encoding_buffer )
		free( currentside->track_encoding_buffer );

	if( currentside->databuffer )
		free( currentside->databuffer );

	if( currentside->indexbuffer )
		free( currentside->indexbuffer );

	if( currentside->flakybitsbuffer )
		free( currentside->flakybitsbuffer );

	free( currentside );

	return NULL;
}

void tg_addISOSectorToTrack(track_generator *tg,HXCFE_SECTCFG * sectorconfig,HXCFE_SIDE * currentside)
{
	int32_t  i,j,k;
	uint8_t  c,trackenc;
	uint8_t  CRC16_High;
	uint8_t  CRC16_Low;
	uint8_t  crctable[32];
	int32_t  startindex,sectorsize;
	int data_part_encoding;
	int startdatabitindex;
	isoibm_config * configptr;

	startindex=tg->last_bit_offset/8;

	sectorconfig->startsectorindex = tg->last_bit_offset/8;

	configptr = tg->disk_formats_LUT[sectorconfig->trackencoding];

	// sync
	for(i=0;i<configptr->len_ssync;i++)
	{
		pushTrackCode(tg,configptr->data_ssync,0xFF,currentside,sectorconfig->trackencoding);
	}

	CRC16_Init(&CRC16_High,&CRC16_Low,(unsigned char*)&crctable,configptr->crc_poly,configptr->crc_initial);
	// add mark
	for(i=0;i<configptr->len_addrmarkp1;i++)
	{
		pushTrackCode(tg,configptr->data_addrmarkp1,configptr->clock_addrmarkp1,currentside,sectorconfig->trackencoding);
		CRC16_Update(&CRC16_High,&CRC16_Low, configptr->data_addrmarkp1,(unsigned char*)&crctable);
	}

	if(sectorconfig->use_alternate_addressmark)
	{
		for(i=0;i<configptr->len_addrmarkp2;i++)
		{
			pushTrackCode(tg,(uint8_t)sectorconfig->alternate_addressmark,configptr->clock_addrmarkp2,currentside,sectorconfig->trackencoding);
			CRC16_Update(&CRC16_High,&CRC16_Low, (uint8_t)(sectorconfig->alternate_addressmark),(unsigned char*)&crctable );
		}
	}
	else
	{
		for(i=0;i<configptr->len_addrmarkp2;i++)
		{
			pushTrackCode(tg,configptr->data_addrmarkp2,configptr->clock_addrmarkp2,currentside,sectorconfig->trackencoding);
			CRC16_Update(&CRC16_High,&CRC16_Low, configptr->data_addrmarkp2,(unsigned char*)&crctable );
		}
	}

	if(configptr->indexformat==MEMBRAINFORMAT_DD)
	{
		c = (uint8_t)(sectorconfig->cylinder>>3);
		pushTrackCode(tg,c,0xFF,currentside,sectorconfig->trackencoding);
		CRC16_Update(&CRC16_High,&CRC16_Low, c,(unsigned char*)&crctable );

		c = (uint8_t)((sectorconfig->cylinder<<5) | ((sectorconfig->head&1) << 4) | (sectorconfig->sector&0xF));
		pushTrackCode(tg,c,0xFF,currentside,sectorconfig->trackencoding);
		CRC16_Update(&CRC16_High,&CRC16_Low, c,(unsigned char*)&crctable );

		sectorsize = sectorconfig->sectorsize;
	}
	else
	{
		// track number
		if(configptr->track_id)
		{
			pushTrackCode(tg,(uint8_t)sectorconfig->cylinder,0xFF,currentside,sectorconfig->trackencoding);
			CRC16_Update(&CRC16_High,&CRC16_Low, (uint8_t)sectorconfig->cylinder,(unsigned char*)&crctable );
		}

		//01 Side # The side number this is (0 or 1)
		if(configptr->side_id)
		{
			pushTrackCode(tg,(uint8_t)sectorconfig->head,  0xFF,currentside,sectorconfig->trackencoding);
			CRC16_Update(&CRC16_High,&CRC16_Low, (uint8_t)sectorconfig->head,(unsigned char*)&crctable );
		}

		//01 Sector # The sector number
		if(configptr->sector_id)
		{
			pushTrackCode(tg,(uint8_t)sectorconfig->sector,0xFF,currentside,sectorconfig->trackencoding);
			CRC16_Update(&CRC16_High,&CRC16_Low, (uint8_t)sectorconfig->sector,(unsigned char*)&crctable );
		}

		//01 Sector size: 02=512. (00=128, 01=256, 02=512, 03=1024)

		sectorsize = 128;

		if(configptr->sector_size_id)
		{
			sectorsize = sectorconfig->sectorsize;
			if(sectorconfig->use_alternate_sector_size_id)
			{
				c = (uint8_t)sectorconfig->alternate_sector_size_id;
			}
			else
			{
				c=0;
				while(((128<<(unsigned int)c) != sectorsize ) && c<8)
				{
					c++;
				}
			}

			if( configptr->indexformat == DECRX02_SDDD )
			{
				// DEC RX02 : Size field always set to 0.
				c = 0;
				if(sectorconfig->sectorsize < 256)
					sectorsize = 128;
				else
					sectorsize = 256;
			}

			pushTrackCode(tg,c,0xFF,currentside,sectorconfig->trackencoding);
			CRC16_Update(&CRC16_High,&CRC16_Low, c,(unsigned char*)&crctable );
		}
	}

	//02 CRC The sector Header CRC
	if(sectorconfig->use_alternate_header_crc&0x2)
	{
		pushTrackCode(tg, (unsigned char) (sectorconfig->header_crc&0xFF),    0xFF,currentside,sectorconfig->trackencoding);
		pushTrackCode(tg, (unsigned char)((sectorconfig->header_crc>>8)&0xFF),0xFF,currentside,sectorconfig->trackencoding);
	}
	else
	{
		//bad crc
		if(sectorconfig->use_alternate_header_crc&0x1)
		{
			pushTrackCode(tg,(unsigned char)(CRC16_High^0x13),0xFF,currentside,sectorconfig->trackencoding);
			pushTrackCode(tg,(unsigned char)(CRC16_Low ^0x17),0xFF,currentside,sectorconfig->trackencoding);
		}
		else
		{
			pushTrackCode(tg,CRC16_High,0xFF,currentside,sectorconfig->trackencoding);
			pushTrackCode(tg,CRC16_Low,0xFF,currentside,sectorconfig->trackencoding);
		}
	}

	// add extra desync
	for(i=0;i<configptr->posthcrc_len;i++)
	{
		pushTrackCode(tg,configptr->posthcrc_glitch_data,configptr->posthcrc_glitch_clock,currentside,sectorconfig->trackencoding);
	}

	// gap2
	for(i=0;i<configptr->len_gap2;i++)
	{
		pushTrackCode(tg,configptr->data_gap2,0xFF,currentside,sectorconfig->trackencoding);
	}

	if( !(sectorconfig->flags & TRACKGEN_NO_DATA) )
	{

		// sync
		for(i=0;i<configptr->len_dsync;i++)
		{
			pushTrackCode(tg,configptr->data_dsync,0xFF,currentside,sectorconfig->trackencoding);
		}

		//02 CRC The CRC of the data
		CRC16_Init(&CRC16_High,&CRC16_Low,(unsigned char*)&crctable,configptr->crc_poly,configptr->crc_initial);

		// data mark
		for(i=0;i<configptr->len_datamarkp1;i++)
		{
			pushTrackCode(tg,configptr->data_datamarkp1,configptr->clock_datamarkp1,currentside,sectorconfig->trackencoding);
			CRC16_Update(&CRC16_High,&CRC16_Low, configptr->data_datamarkp1,(unsigned char*)&crctable );
		}

		if(sectorconfig->use_alternate_datamark)
		{
			for(i=0;i<configptr->len_datamarkp2;i++)
			{
				pushTrackCode(tg,(uint8_t)sectorconfig->alternate_datamark,configptr->clock_datamarkp2,currentside,sectorconfig->trackencoding);
				CRC16_Update(&CRC16_High,&CRC16_Low, (uint8_t)sectorconfig->alternate_datamark,(unsigned char*)&crctable );
			}
		}
		else
		{
			for(i=0;i<configptr->len_datamarkp2;i++)
			{
				pushTrackCode(tg,configptr->data_datamarkp2,configptr->clock_datamarkp2,currentside,sectorconfig->trackencoding);
				CRC16_Update(&CRC16_High,&CRC16_Low, configptr->data_datamarkp2,(unsigned char*)&crctable );
			}
		}

		data_part_encoding = sectorconfig->trackencoding;


		if( configptr->indexformat == DECRX02_SDDD)
		{
			// DEC RX02 : Encode the Data + CRC in MFM.
			data_part_encoding = ISOFORMAT_DD;
			tg->mfm_last_bit = 0x7FFF;
			tg->last_bit_offset++;
		}

		startdatabitindex = tg->last_bit_offset;
		sectorconfig->startdataindex = tg->last_bit_offset/8;
		if(sectorconfig->input_data)
		{
			if( tg->last_bit_offset & 7 )
			{
				for(i=0;i<sectorsize;i++)
				{
					pushTrackCode(tg,sectorconfig->input_data[i],0xFF,currentside,data_part_encoding);
				}
			}
			else
			{
				FastMFMFMgenerator(tg,currentside,sectorconfig->input_data,sectorsize,data_part_encoding);
			}

			// data crc
			for(i=0;i<sectorsize;i++)
			{
				CRC16_Update(&CRC16_High,&CRC16_Low, sectorconfig->input_data[i],(unsigned char*)&crctable );
			}
		}
		else
		{
			for(i=0;i<sectorsize;i++)
			{
				pushTrackCode(tg,sectorconfig->fill_byte,0xFF,currentside,data_part_encoding);
				CRC16_Update(&CRC16_High,&CRC16_Low, sectorconfig->fill_byte,(unsigned char*)&crctable );
			}
		}

		if(sectorconfig->weak_bits_mask)
		{
			if( currentside->flakybitsbuffer)
			{
				for(i=0;i<sectorsize*8;i++)
				{
					if(sectorconfig->weak_bits_mask[i>>3] & (0x80 >> (i&7)))
					{
						k = (int)((float)( tg->last_bit_offset - ( sectorconfig->startdataindex * 8 ) ) * (float)( (float)i / (float)(sectorsize*8) ) );

						currentside->flakybitsbuffer[sectorconfig->startdataindex + (k>>3)] |= ( 0x80 >> (k&7) );
					}
				}
			}
		}

		if(sectorconfig->use_alternate_data_crc&0x2)
		{
			// alternate crc
			pushTrackCode(tg,(unsigned char)(sectorconfig->data_crc&0xFF),     0xFF,currentside,data_part_encoding);
			pushTrackCode(tg,(unsigned char)((sectorconfig->data_crc>>8)&0xFF),0xFF,currentside,data_part_encoding);
		}
		else
		{
			//bad crc
			if(sectorconfig->use_alternate_data_crc&0x1)
			{
				pushTrackCode(tg,(unsigned char)(CRC16_High^0x21),0xFF,currentside,data_part_encoding);
				pushTrackCode(tg,(unsigned char)(CRC16_Low ^0x20),0xFF,currentside,data_part_encoding);
			}
			else
			{
				pushTrackCode(tg,CRC16_High,0xFF,currentside,data_part_encoding);
				pushTrackCode(tg,CRC16_Low ,0xFF,currentside,data_part_encoding);
			}
		}

		if( configptr->indexformat == DECRX02_SDDD )
		{
			// DEC RX02 : Convert/Patch the encoded MFM to DEC M2FM.
			mfmtodecm2fm( currentside->databuffer, currentside->tracklen, startdatabitindex, ((256+2)*8*2) );
		}

		// add extra desync
		for(i=0;i<configptr->postdcrc_len;i++)
		{
			pushTrackCode(tg,configptr->postdcrc_glitch_data,configptr->postdcrc_glitch_clock,currentside,sectorconfig->trackencoding);
		}

		//gap3
		if(sectorconfig->gap3!=255)
		{
			for(i=0;i<sectorconfig->gap3;i++)
			{
				pushTrackCode(tg,configptr->data_gap3,0xFF,currentside,sectorconfig->trackencoding);
			}

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

	trackenc = ISOIBM_MFM_ENCODING;

	switch(sectorconfig->trackencoding)
	{
		case IBMFORMAT_SD:
		case ISOFORMAT_SD:
		case TYCOMFORMAT_SD:
			trackenc = ISOIBM_FM_ENCODING;
		break;

		case ISOFORMAT_DD11S:
		case IBMFORMAT_DD:
		case ISOFORMAT_DD:
		case UKNCFORMAT_DD:
			trackenc = ISOIBM_MFM_ENCODING;
		break;

		case DECRX02_SDDD:
			trackenc = DEC_RX02_M2FM_ENCODING;
		break;

		case AED6200P_DD:
			trackenc = AED6200P_MFM_ENCODING;
		break;

		case MEMBRAINFORMAT_DD:
			currentside->track_encoding = MEMBRAIN_MFM_ENCODING;
		break;

		case NORTHSTAR_HS_DD:
			currentside->track_encoding = NORTHSTAR_HS_MFM_ENCODING;
		break;

		case HEATHKIT_HS_SD:
			currentside->track_encoding = HEATHKIT_HS_FM_ENCODING;
		break;

		case MICRALN_HS_SD:
			currentside->track_encoding = MICRALN_HS_FM_ENCODING;
		break;

		default:
			trackenc = ISOIBM_MFM_ENCODING;
		break;
	}

	if(currentside->track_encoding_buffer)
	{
		for(j=startindex;j<(tg->last_bit_offset/8);j++)
		{
			currentside->track_encoding_buffer[j] = trackenc;
		}
	}

	currentside->number_of_sector++;
}

void tg_addSectorToTrack(track_generator *tg,HXCFE_SECTCFG * sectorconfig,HXCFE_SIDE * currentside)
{

	switch(sectorconfig->trackencoding)
	{
		case IBMFORMAT_SD:
		case IBMFORMAT_DD:
		case ISOFORMAT_SD:
		case ISOFORMAT_DD:
		case ISOFORMAT_DD11S:
		case DECRX02_SDDD:
		case TYCOMFORMAT_SD:
		case MEMBRAINFORMAT_DD:
		case UKNCFORMAT_DD:
		case AED6200P_DD:
			tg_addISOSectorToTrack(tg,sectorconfig,currentside);
		break;

		case AMIGAFORMAT_DD:
			tg_addAmigaSectorToTrack(tg,sectorconfig,currentside);
		break;

		case APPLE2_GCR5A3:
		case APPLE2_GCR6A2:
			tg_addAppleSectorToTrack(tg,sectorconfig,currentside);
		break;

		case APPLEMAC_GCR6A2:
			tg_addAppleMacSectorToTrack(tg,sectorconfig,currentside);
		break;

		case NORTHSTAR_HS_DD:
			tg_addNorthstarSectorToTrack(tg,sectorconfig,currentside);
		break;

		case HEATHKIT_HS_SD:
			tg_addHeathkitSectorToTrack(tg,sectorconfig,currentside);
		break;

		case MICRALN_HS_SD:
			tg_addMicralNSectorToTrack(tg,sectorconfig,currentside);
		break;
	}
}

void tg_completeTrack(track_generator *tg, HXCFE_SIDE * currentside,int32_t trackencoding)
{
	int32_t tracklen,trackoffset;
	uint8_t c,oldval;
	int32_t startindex,i;
	isoibm_config * configptr;

	configptr = tg->disk_formats_LUT[trackencoding];

	tracklen=currentside->tracklen/8;
	if(currentside->tracklen&7) tracklen++;

	startindex=tg->last_bit_offset/8;
	trackoffset=startindex;
	while(trackoffset<tracklen)
	{
		pushTrackCode(tg,configptr->data_gap4b,0xFF,currentside,trackencoding);
		trackoffset=tg->last_bit_offset/8;
	}

	if( (trackencoding == IBMFORMAT_SD) || \
		(trackencoding == ISOFORMAT_SD) || \
		(trackencoding == TYCOMFORMAT_SD) || \
		(trackencoding == DECRX02_SDDD)
	)
	{
		//
		// SCAN Command FM issue workaround for some FDC  : Desync the clock at the end of the track.
		//

		oldval = 0;
		startindex = startindex + (( (trackoffset - startindex) * 3 ) / 4);
		trackoffset=startindex;
		if(trackoffset<tracklen)
		{
			currentside->databuffer[trackoffset] = (uint8_t)(currentside->databuffer[trackoffset] & 0x3F);
			while(trackoffset<tracklen)
			{
				c = currentside->databuffer[trackoffset];
				c = (uint8_t)( (c >> 1) | ( (oldval << 7) & 0x80 ) );

				oldval = currentside->databuffer[trackoffset];
				currentside->databuffer[trackoffset] = c;

				trackoffset++;
			}

		}

		if(trackoffset && trackoffset<tracklen)
		{
			currentside->databuffer[trackoffset-1] = (uint8_t)(currentside->databuffer[trackoffset-1] & 0xFC);
		}
	}

	// fill timing & encoding buffer
	if(currentside->timingbuffer)
	{
		for(i=startindex;i<(tg->last_bit_offset/8);i++)
		{
			currentside->timingbuffer[i]=currentside->timingbuffer[startindex-1];
		}
	}

	if(currentside->track_encoding_buffer)
	{
		for(i=startindex;i<(tg->last_bit_offset/8);i++)
		{
			currentside->track_encoding_buffer[i]=currentside->track_encoding_buffer[startindex-1];
		}
	}
}

HXCFE_SIDE * tg_generateTrackEx(int32_t number_of_sector,HXCFE_SECTCFG * sectorconfigtab,int32_t interleave,int32_t skew,int32_t bitrate,int32_t rpm,int32_t trackencoding,int32_t pregap,int32_t indexlen,int32_t indexpos)
{
	int32_t i;
	int32_t tracksize;
	int32_t track_period,wanted_trackperiod,indexperiod;
	unsigned char * interleavetab;
	int32_t gap3tocompute;
	uint32_t gap3period,computedgap3;
	int32_t gap3;

	track_generator tg;
	HXCFE_SIDE * currentside;

	if(!sectorconfigtab && number_of_sector)
		return NULL;

	// compute the sectors interleaving.
	interleavetab = compute_interleave_tab(interleave,skew,number_of_sector);

	tg_initTrackEncoder(&tg);

	if(pregap & 0x40000000)
	{
		// Compute gap
		pregap = tg_timeToSize( &tg, trackencoding, bitrate, pregap & ~0xC0000000);
	}

	// get minimum track size
	tracksize = tg_computeMinTrackSize(&tg,trackencoding,bitrate,number_of_sector,sectorconfigtab,pregap,&track_period);

	if(rpm)
	{
		if(rpm & 0x40000000)
			wanted_trackperiod = rpm & ~0xC0000000;
		else
			wanted_trackperiod = (100000*60)/rpm;
	}
	else
	{
		wanted_trackperiod=(100000*60)/300;
	}
	// compute the adjustable gap3 length
	// how many gap3 we need to compute ?
	gap3tocompute=0;
	for(i=0;i<number_of_sector;i++)
	{
		// if gap3 not set...
		if(sectorconfigtab[i].gap3==0xFF)
		{
			gap3tocompute++;
		}
	}

	indexperiod=0;
	if(indexlen&NO_SECTOR_UNDER_INDEX)
	{
		indexperiod=(indexlen&0xFFFFFF)/10;
	}

	// Set/Fix the sector left field (Amiga)
	for(i=0;i<number_of_sector;i++)
	{
		sectorconfigtab[i].sectorsleft = number_of_sector - i;
	}

	//first try : get a standard value...
	if(gap3tocompute==number_of_sector)
	{
		gap3=searchgap3value(number_of_sector,sectorconfigtab);
		if(gap3!=-1)
		{
			for(i=0;i<number_of_sector;i++)
			{
				sectorconfigtab[i].gap3=(unsigned char)gap3;
			}
			gap3tocompute=0;
		}
	}

	// compute the dispatched the gap3 period
	gap3period=0;
	if(gap3tocompute && (wanted_trackperiod>(track_period+indexperiod)))
	{
		gap3period=wanted_trackperiod-(track_period+indexperiod);
		gap3period=gap3period/gap3tocompute;

		// set the right gap3 length according to the sector bitrate
		for(i=0;i<number_of_sector;i++)
		{
			// if gap3 not set...
			if(sectorconfigtab[i].gap3==0xFF)
			{
				// TODO: make integer this...
				computedgap3=(uint32_t)floor((float)gap3period*(float)((float)sectorconfigtab[i].bitrate/(float)100000));

				switch(sectorconfigtab[i].trackencoding)
				{
					case IBMFORMAT_SD:
					case ISOFORMAT_SD:
					case TYCOMFORMAT_SD:
					case HEATHKIT_HS_SD:
					case DECRX02_SDDD:
						computedgap3=computedgap3/(2*8);
					break;

					case ISOFORMAT_DD11S:
					case IBMFORMAT_DD:
					case ISOFORMAT_DD:
					case MEMBRAINFORMAT_DD:
					case UKNCFORMAT_DD:
					case AED6200P_DD:
					case NORTHSTAR_HS_DD:
						computedgap3=computedgap3/(1*8);
					break;
				}

				if(computedgap3>200)
					computedgap3=200;
				sectorconfigtab[i].gap3=(unsigned char)computedgap3;


				//floppycontext->hxc_printf(MSG_DEBUG,"Sector:%d Computed Gap:%d",sectorconfigtab[i].sector, computedgap3);
			}
		}
	}

	// recompute the track size with the new gap settings.
	tracksize = tg_computeMinTrackSize(&tg,trackencoding,bitrate,number_of_sector,sectorconfigtab,pregap,&track_period);

	// adjust the track length to get the right rpm.
	if(wanted_trackperiod>track_period)
	{
		while( wanted_trackperiod>track_period )
		{
			if(wanted_trackperiod - track_period > 40000)
			{
				tracksize += (((40000 * ((bitrate/4)/4) )/(12500/4)));
				track_period += 40000;
			}
			else
			{
				tracksize += ((((wanted_trackperiod-track_period) * ((bitrate/4)/4) )/(12500/4)));
				track_period += (wanted_trackperiod - track_period);
			}
		}
	}

	if(tracksize<0)
		return 0;

	// align the track size
	if(tracksize&0x1F)
	{
		tracksize=(tracksize&(~0x1F))+0x20;
	}

	// alloc the track...
	currentside = tg_initTrack(&tg,tracksize,number_of_sector,trackencoding,bitrate,sectorconfigtab,pregap);
	if( !currentside )
		return NULL;

	// and push all sectors to the track...
	for(i=0;i<number_of_sector;i++)
	{
		tg_addSectorToTrack(&tg,&sectorconfigtab[interleavetab[i]],currentside);
	}

	// "close" the track : extend/add post gap..
	tg_completeTrack(&tg,currentside,trackencoding);

	if(indexlen & REVERTED_INDEX)
	{
		fillindex(indexpos,currentside,indexlen,1,1);
	}
	else
	{
		fillindex(indexpos,currentside,indexlen,1,0);
	}

	if(interleavetab) free(interleavetab);

	return currentside;
}

HXCFE_SIDE * tg_generateTrack(unsigned char * sectors_data,int32_t sector_size,int32_t number_of_sector,int32_t track,int32_t side,int32_t sectorid,int32_t interleave,int32_t skew,int32_t bitrate,int32_t rpm,int32_t trackencoding,int32_t gap3,int32_t pregap, int32_t indexlen,int32_t indexpos)
{
	unsigned short i;
	HXCFE_SIDE * currentside;
	HXCFE_SECTCFG * sectorconfigtab;

	sectorconfigtab = malloc(sizeof(HXCFE_SECTCFG)*number_of_sector);
	if(!sectorconfigtab)
		return NULL;

	memset(sectorconfigtab,0,sizeof(HXCFE_SECTCFG)*number_of_sector);

	for(i=0;i<number_of_sector;i++)
	{
		sectorconfigtab[i].cylinder = track;
		sectorconfigtab[i].head = side;
		sectorconfigtab[i].bitrate = bitrate;
		sectorconfigtab[i].gap3 = gap3;
		sectorconfigtab[i].input_data = &sectors_data[sector_size*i];
		sectorconfigtab[i].sectorsize = sector_size;
		sectorconfigtab[i].trackencoding = trackencoding;
		sectorconfigtab[i].sector = sectorid + i;
		sectorconfigtab[i].sectorsleft = number_of_sector - i; // Used in Amiga tracks.
	}

	currentside = tg_generateTrackEx(number_of_sector,sectorconfigtab,interleave,skew,bitrate,rpm,trackencoding,pregap,indexlen,indexpos);

	free(sectorconfigtab);

	return currentside;
}

HXCFE_SIDE * tg_alloctrack(int32_t bitrate,int32_t trackencoding,int32_t rpm,int32_t tracksize,int32_t indexlen,int32_t indexpos,int32_t buffertoalloc)
{
	HXCFE_SIDE * currentside;
	unsigned int tracklen;
	unsigned int i;

	currentside = (HXCFE_SIDE*) malloc(sizeof(HXCFE_SIDE));
	if(!currentside)
		return NULL;

	memset(currentside,0,sizeof(HXCFE_SIDE));

	currentside->number_of_sector=0;

	tracklen=tracksize/8;
	if(tracksize&7) tracklen++;

	currentside->tracklen=tracksize;

	//////////////////////////////
	// bitrate buffer allocation
	currentside->bitrate=bitrate;

	if(buffertoalloc & TG_ALLOCTRACK_ALLOCTIMIMGBUFFER)
	{
		currentside->bitrate=VARIABLEBITRATE;
		currentside->timingbuffer = malloc(tracklen*sizeof(uint32_t));
		if(!currentside->timingbuffer)
			goto alloc_error;

		for(i=0;i<tracklen;i++)
		{
			currentside->timingbuffer[i] = bitrate;
		}
	}


	///////////////////////////////////////////
	// track encoding code buffer allocation
	currentside->track_encoding=trackencoding;
	if(buffertoalloc & TG_ALLOCTRACK_ALLOCENCODINGBUFFER)
	{
		currentside->track_encoding=VARIABLEENCODING;
		currentside->track_encoding_buffer = malloc(tracklen*sizeof(unsigned char));
		if(!currentside->track_encoding_buffer)
			goto alloc_error;

		for(i=0;i<tracklen;i++)
		{
			currentside->track_encoding_buffer[i] = (uint8_t)trackencoding;
		}
	}

	///////////////////////////////////////////
	// track flakey bits allocation
	if(buffertoalloc & TG_ALLOCTRACK_ALLOCFLAKEYBUFFER)
	{
		currentside->flakybitsbuffer = malloc(tracklen*sizeof(unsigned char));
		if(!currentside->flakybitsbuffer)
			goto alloc_error;

		if(buffertoalloc & TG_ALLOCTRACK_UNFORMATEDBUFFER )
		{
			memset(currentside->flakybitsbuffer,0xFF,tracklen*sizeof(unsigned char));
		}
		else
		{
			memset(currentside->flakybitsbuffer,0x00,tracklen*sizeof(unsigned char));
		}
	}

	/////////////////////////////
	// data buffer allocation
	currentside->databuffer = malloc(tracklen);
	if(!currentside->databuffer)
		goto alloc_error;

	memset(currentside->databuffer,0,tracklen);
	if(buffertoalloc & TG_ALLOCTRACK_RANDOMIZEDATABUFFER)
	{
		for(i=0;i<tracklen;i++)
		{
			currentside->databuffer[i] = (uint8_t)(rand());
		}
	}

	/////////////////////////////
	// index buffer allocation
	currentside->indexbuffer = malloc(tracklen);
	if(!currentside->indexbuffer)
		goto alloc_error;

	memset(currentside->indexbuffer,0,tracklen);

	if(indexlen & REVERTED_INDEX)
	{
		fillindex(indexpos,currentside,indexlen,1,1);
	}
	else
	{
		fillindex(indexpos,currentside,indexlen,1,0);
	}

	return currentside;

alloc_error:
	if(currentside->timingbuffer)
		free(currentside->timingbuffer);

	if(currentside->track_encoding_buffer)
		free(currentside->track_encoding_buffer);

	if(currentside->flakybitsbuffer)
		free(currentside->flakybitsbuffer);

	if(currentside->databuffer)
		free(currentside->databuffer);

	if(currentside->indexbuffer)
		free(currentside->indexbuffer);

	free(currentside);

	return NULL;
}

uint32_t * tg_allocsubtrack_long( int32_t tracksize, uint32_t initvalue )
{
	unsigned int tracklen;
	unsigned int i;
	uint32_t * ptr;

	tracklen=tracksize/8;
	if(tracksize&7) tracklen++;

	ptr = malloc(tracklen*sizeof(uint32_t));
	if(ptr)
	{
		for(i=0;i<tracklen;i++)
		{
			ptr[i]=initvalue;
		}
	}

	return ptr;
}

uint8_t  * tg_allocsubtrack_char( int32_t tracksize, uint8_t initvalue )
{
	int32_t tracklen;
	int32_t i;
	unsigned char * ptr;

	tracklen=tracksize/8;
	if(tracksize&7) tracklen++;

	ptr = malloc(tracklen*sizeof(uint8_t));
	if(ptr)
	{
		for(i=0;i<tracklen;i++)
		{
			ptr[i]=initvalue;
		}
	}

	return ptr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
