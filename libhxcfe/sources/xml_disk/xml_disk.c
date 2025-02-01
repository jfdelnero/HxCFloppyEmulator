/*
//
// Copyright (C) 2006-2025 Jean-Fran�ois DEL NERO
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
//-------------------------------------------------------------------------------//
//-----------H----H--X----X-----CCCCC----22222----0000-----0000------11----------//
//----------H----H----X-X-----C--------------2---0----0---0----0--1--1-----------//
//---------HHHHHH-----X------C----------22222---0----0---0----0-----1------------//
//--------H----H----X--X----C----------2-------0----0---0----0-----1-------------//
//-------H----H---X-----X---CCCCC-----222222----0000-----0000----1111------------//
//-------------------------------------------------------------------------------//
//----------------------------------------------------- http://hxc2001.free.fr --//
///////////////////////////////////////////////////////////////////////////////////
// File : xml_disk.c
// Contains: raw disk loader/creator
//
// Written by: Jean-Fran�ois DEL NERO
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "types.h"

#include <expat.h>

#include "internal_libhxcfe.h"
#include "libhxcfe.h"
#include "floppy_loader.h"
#include "floppy_utils.h"
#include "libhxcadaptor.h"

#include "xml_disk.h"
#include "./DiskLayouts/LayoutsIndex.h"
#include "./packer/pack.h"

#define XML_STRING_MAXSIZE 512

typedef struct parse_stack_
{
	int32_t state;
	int32_t track_type;
	int32_t cur_track;
}parse_stack;

typedef struct track_state_
{
	int32_t base_adress;
	int32_t track_size;
	int32_t set;
}track_state;

#define MAX_TRACKS_NB 256

typedef struct app_data
{
	HXCFE* floppycontext;

	int32_t current_state;
	void * p;

	int32_t stack_ptr;

	int32_t layout_id;
	char * xmlfile_path[512];

	parse_stack statestack[32];

	HXCFE_FLPGEN * fb;
	HXCFE_FLOPPY* floppy;

	track_state ts[MAX_TRACKS_NB*2];

	int32_t xmlcheck;

	char name[XML_STRING_MAXSIZE];
	char description[XML_STRING_MAXSIZE];
	char file_extensions[XML_STRING_MAXSIZE];

	uint8_t * image_data;

	uint8_t * sector_data;
	int32_t sector_data_offset;

	int32_t buffer_size;

	int32_t skew_per_track;
	int32_t skew_per_side;

	int32_t interface_mode;
	int32_t double_step;

	int32_t write_protect;

	int32_t fill_value;

	int32_t min_nb_of_tracks;
	int32_t max_nb_of_tracks;
	int32_t nb_of_tracks;

	int32_t min_nb_of_sides;
	int32_t max_nb_of_sides;
	int32_t nb_of_sides;

	int32_t file_size;

	int32_t nb_sectors_per_track;
	int32_t sector_size;

	int32_t index_length;
	int32_t cur_index_number;

} AppData;

char * get_ext(int index,char * bufin,char * bufferout)
{
	int i,j;

	j = 0;
	i = 0;
	while( (i < index) && bufin[j])
	{
		while( ( bufin[j] != ',' ) && bufin[j] )
		{
			j++;
		}

		i++;
		if(bufin[j])
			j++;
	}

	if(bufin[j])
	{
		if(bufin[j] == ',')
			j++;

		i = 0;
		bufferout[i] = 0;
		do
		{
			bufferout[i] = bufin[j];
			i++;
			j++;
		}while(bufin[j] != ',' && bufin[j]);
		bufferout[i] = 0;

		return bufferout;
	}
	else
	{
		return 0;
	}
}

int is_ext_matching(char * filename,char * extlist)
{
	int i;
	char temp_ext[8];
	char * ptr;

	i = 0;
	do{
		ptr = get_ext(i,extlist,(char*)&temp_ext);

		if(ptr)
		{
			if( hxc_checkfileext(filename,ptr,SYS_PATH_TYPE) )
			{
				return 1;
			}

			i++;
		}
	}while(ptr);

	return 0;
}

int generateDisk(AppData *ad,unsigned char * diskdata,int buffersize)
{
	int i,j,ret;
	int numberofsector,numberoftrack,numberofside,sectorsize;
	int type;
	int statetracknb;
	unsigned int rpm;
	int offset;
	unsigned char * partialbuf;

	offset = 0;
	sectorsize = hxcfe_getCurrentSectorSize(ad->fb);
	numberofsector = hxcfe_getCurrentNumberOfSector(ad->fb);
	numberoftrack = hxcfe_getCurrentNumberOfTrack(ad->fb);
	numberofside = hxcfe_getCurrentNumberOfSide(ad->fb);
	rpm = hxcfe_getCurrentRPM(ad->fb);
	type = hxcfe_getCurrentTrackType(ad->fb);

	for(i = 0 ; i < numberoftrack ; i++ )
	{
		for(j = 0 ; j < numberofside ; j++ )
		{
			statetracknb = (i<<1) | (j&1);

			if(!ad->ts[statetracknb].set)
			{

				hxcfe_pushTrack (ad->fb,rpm,i,j,type);

				hxcfe_setTrackSkew(ad->fb, (hxcfe_getCurrentSkew(ad->fb) + ( ad->skew_per_track * i ) + ( j * ad->skew_per_side ))%numberofsector );

				if( ( offset  >= buffersize) || !diskdata )
				{
					ret = hxcfe_addSectors(ad->fb,j,i,0,0,numberofsector);
					if(ret != HXCFE_NOERROR)
						ad->floppycontext->hxc_printf(MSG_ERROR,"generateDisk : hxcfe_addSectors failed -> %d !",ret);
				}
				else
				{

					if(buffersize - offset >= (sectorsize * numberofsector) )
					{
						ret = hxcfe_addSectors(ad->fb,j,i,&diskdata[offset],(sectorsize * numberofsector),numberofsector);
						if(ret != HXCFE_NOERROR)
							ad->floppycontext->hxc_printf(MSG_ERROR,"generateDisk : hxcfe_addSectors failed -> %d !",ret);
					}
					else
					{
						partialbuf = malloc(sectorsize * numberofsector);
						if(partialbuf)
						{
							memset(partialbuf,ad->fill_value,sectorsize * numberofsector);
							memcpy(partialbuf,&diskdata[offset],buffersize - offset);

							ret = hxcfe_addSectors(ad->fb,j,i,partialbuf,(sectorsize * numberofsector),numberofsector);
							if(ret != HXCFE_NOERROR)
								ad->floppycontext->hxc_printf(MSG_ERROR,"generateDisk : hxcfe_addSectors failed -> %d !",ret);
							free(partialbuf);
						}
						else
						{
							ret = hxcfe_addSectors(ad->fb,j,i,0,0,numberofsector);
							if(ret != HXCFE_NOERROR)
								ad->floppycontext->hxc_printf(MSG_ERROR,"generateDisk : hxcfe_addSectors failed -> %d !",ret);
						}

					}
				}

				offset = offset + (sectorsize * numberofsector);
				hxcfe_popTrack (ad->fb);
			}
			else
			{
				offset = ad->ts[statetracknb].base_adress + ad->ts[statetracknb].track_size;
			}
		}
	}

	return HXCFE_NOERROR;
}

int dumpDisk(AppData *ad,unsigned char * diskdata,int buffersize,HXCFE_FLOPPY * fp)
{
	int i,j,ret;
	int numberofsector,numberoftrack,numberofside,sectorsize;
	int type;
	int statetracknb;
	unsigned int rpm;
	int offset;
	unsigned char * partialbuf;

	offset = 0;
	sectorsize = hxcfe_getCurrentSectorSize(ad->fb);
	numberofsector = hxcfe_getCurrentNumberOfSector(ad->fb);
	numberoftrack = hxcfe_getCurrentNumberOfTrack(ad->fb);
	numberofside = hxcfe_getCurrentNumberOfSide(ad->fb);
	rpm = hxcfe_getCurrentRPM(ad->fb);
	type = hxcfe_getCurrentTrackType(ad->fb);

	for(i = 0 ; i < numberoftrack ; i++ )
	{
		for(j = 0 ; j < numberofside ; j++ )
		{
			statetracknb = (i<<1) | (j&1);

			if(!ad->ts[statetracknb].set)
			{

				hxcfe_pushTrack (ad->fb,rpm,i,j,type);

				hxcfe_setTrackSkew(ad->fb, (hxcfe_getCurrentSkew(ad->fb) + ( ad->skew_per_track * i ) + ( j * ad->skew_per_side ))%numberofsector );


				if( ( offset  >= buffersize) || !diskdata )
				{
					ret = hxcfe_addSectors(ad->fb,j,i,0,0,numberofsector);
				}
				else
				{

					if(buffersize - offset >= (sectorsize * numberofsector) )
					{
						ret = hxcfe_addSectors(ad->fb,j,i,&diskdata[offset],(sectorsize * numberofsector),numberofsector);
						if(ret != HXCFE_NOERROR)
							ad->floppycontext->hxc_printf(MSG_ERROR,"dumpDisk : hxcfe_addSectors failed -> %d !",ret);
					}
					else
					{
						partialbuf = malloc(sectorsize * numberofsector);
						if(partialbuf)
						{
							memset(partialbuf,ad->fill_value,sectorsize * numberofsector);
							memcpy(partialbuf,&diskdata[offset],buffersize - offset);

							ret = hxcfe_addSectors(ad->fb,j,i,partialbuf,(sectorsize * numberofsector),numberofsector);
							if(ret != HXCFE_NOERROR)
								ad->floppycontext->hxc_printf(MSG_ERROR,"dumpDisk : hxcfe_addSectors failed -> %d !",ret);

							free(partialbuf);
						}
						else
						{
							ret = hxcfe_addSectors(ad->fb,j,i,0,0,numberofsector);
							if(ret != HXCFE_NOERROR)
								ad->floppycontext->hxc_printf(MSG_ERROR,"dumpDisk : hxcfe_addSectors failed -> %d !",ret);
						}

					}
				}

				offset = offset + (sectorsize * numberofsector);
				hxcfe_popTrack (ad->fb);
			}
			else
			{
				offset = ad->ts[statetracknb].base_adress + ad->ts[statetracknb].track_size;
			}
		}
	}

	return HXCFE_NOERROR;
}

int getnewstate(char * keyword, int currentstate)
{
	int i;

	i=0;

	while(keyword_list[i].keyword)
	{
		if( !strcmp( keyword_list[i].keyword ,keyword) && ( currentstate == keyword_list[i].prev_state) )
		{
			return keyword_list[i].state;
		}

		i++;
	}

	return -1;
}

int getstate(char * keyword, int currentstate)
{
	int i;

	i=0;

	while(keyword_list[i].keyword)
	{
		if( !strcmp( keyword_list[i].keyword ,keyword) && ( currentstate == keyword_list[i].state) )
		{
			return keyword_list[i].state;
		}

		i++;
	}

	return -1;
}

uint32_t ahextoi(char * str)
{
	uint32_t hexval;
	int i;
	unsigned char c;

	hexval = 0;
	i = 0;

	while(str[i])
	{
		c = str[i];

		if( ( c>='0' && c<='9' ) || ( c>='a' && c<='f' ) || ( c>='A' && c<='F' ) )
		{
			hexval = hexval << 4;

			if( c>='0' && c<='9' )
			{
				hexval = hexval | (c - '0');
			}
			else
			{
				if(( c>='a' && c<='f' ))
				{
					hexval = hexval | ( (c - 'a') + 10);
				}
				else
				{
					hexval = hexval | ( (c - 'A') + 10);
				}

			}
		}
		else
		{
			hexval = 0;
		}

		i++;
	}

	return hexval;
}

static void XMLCALL charhandler(void *data, const char *s, int len)
{
	AppData *ad = (AppData *) data;
	char * buffer;
	unsigned int datamark;
	unsigned int crc;
	int track,sectorsize,i;
	int tmp_base_adress;

	buffer = malloc(len + 1);
	if(!buffer) return;

	memset(buffer,0,len + 1);
	memcpy(buffer,s,len);

	switch(ad->current_state)
	{
		case DISK_LAYOUT_NAME:
			strncpy((char*)&ad->name,(char*)buffer,XML_STRING_MAXSIZE - 1);
			ad->name[XML_STRING_MAXSIZE - 1] = 0;
		break;
		case DISK_LAYOUT_DESCRIPTION:
			strncpy((char*)&ad->description,(char*)buffer,XML_STRING_MAXSIZE - 1);
			ad->description[XML_STRING_MAXSIZE - 1] = 0;
		break;
		case FILEEXT:
			strncpy((char*)&ad->file_extensions,(char*)buffer,XML_STRING_MAXSIZE - 1);
			ad->file_extensions[XML_STRING_MAXSIZE - 1] = 0;
		break;
		case INTERFACE_MODE:
			ad->interface_mode = hxcfe_getFloppyInterfaceModeID(ad->floppycontext,(char*)buffer);
		break;
		case DOUBLE_STEP:
			if(!strcmp(buffer,"on"))
				ad->double_step = 1;

			if(!strcmp(buffer,"off"))
				ad->double_step = 0;
		break;
		case FILESIZE:
			ad->file_size = atoi(buffer);
		break;
		case WRITE_PROTECT:
			ad->write_protect = atoi(buffer);
		break;
/*      case TRACKSIZE:
			ad->track_size = atoi(buffer);
		break;*/
		case MINNUMBEROFTRACKS:
			ad->min_nb_of_tracks = atoi(buffer);
		break;
		case MAXNUMBEROFTRACKS:
			ad->max_nb_of_tracks = atoi(buffer);
		break;

		case NUMBEROFTRACK:
			ad->nb_of_tracks = atoi(buffer);
			if(!ad->xmlcheck)
				hxcfe_setNumberOfTrack (ad->fb,(unsigned short)ad->nb_of_tracks);
		break;
		case NUMBEROFSIDE:
			ad->nb_of_sides = (unsigned short)atoi(buffer);
			if(!ad->xmlcheck)
				hxcfe_setNumberOfSide (ad->fb,(unsigned char)ad->nb_of_sides);
		break;
		case NUMBEROFSECTOR:
			ad->nb_sectors_per_track = (unsigned short)atoi(buffer);
			if(!ad->xmlcheck)
				hxcfe_setNumberOfSector (ad->fb,(unsigned short)ad->nb_sectors_per_track);
		break;
		case SECTORSIZE:
			ad->sector_size = (unsigned short)atoi(buffer);
			if(!ad->xmlcheck)
				hxcfe_setSectorSize(ad->fb,ad->sector_size);
		break;
		case INTERLEAVE_TRACK:
		case INTERLEAVE:
			if(!ad->xmlcheck)
				hxcfe_setTrackInterleave(ad->fb,atoi(buffer));
		break;
		case SKEW_TRACK:
		case SKEW:
			if(!ad->xmlcheck)
				hxcfe_setTrackSkew(ad->fb,atoi(buffer));
		break;
		case SKEW_PER_TRACK:
			ad->skew_per_track = atoi(buffer);
		break;
		case SKEW_PER_SIDE:
			ad->skew_per_side = atoi(buffer);
		break;
		case FORMATVALUE:
		case DATAFILL_SECTOR:
			if(!ad->xmlcheck)
			{
				hxcfe_setSectorFill (ad->fb,(unsigned char)ahextoi(buffer));
			}

			ad->fill_value = ahextoi(buffer);
		break;
		case FORMAT_TRACK:
		case FORMAT:
			if(!ad->xmlcheck)
			{
				ad->statestack[ad->stack_ptr].track_type = IBMFORMAT_DD;

				if(!strcmp(buffer,"ISO_MFM"))
				{
					ad->statestack[ad->stack_ptr].track_type = ISOFORMAT_DD;
				}
				if(!strcmp(buffer,"IBM_MFM"))
				{
					ad->statestack[ad->stack_ptr].track_type = IBMFORMAT_DD;
				}
				if(!strcmp(buffer,"ISO_FM"))
				{
					ad->statestack[ad->stack_ptr].track_type = ISOFORMAT_SD;
				}
				if(!strcmp(buffer,"IBM_FM"))
				{
					ad->statestack[ad->stack_ptr].track_type = IBMFORMAT_SD;
				}
				if(!strcmp(buffer,"DECRX02_M2FM"))
				{
					ad->statestack[ad->stack_ptr].track_type = DECRX02_SDDD;
				}
				if(!strcmp(buffer,"UKNC_MFM"))
				{
					ad->statestack[ad->stack_ptr].track_type = UKNCFORMAT_DD;
				}
				if(!strcmp(buffer,"AMIGA_MFM"))
				{
					ad->statestack[ad->stack_ptr].track_type = AMIGAFORMAT_DD;
				}
				if(!strcmp(buffer,"AED6200P_MFM"))
				{
					ad->statestack[ad->stack_ptr].track_type = AED6200P_DD;
				}
				if(!strcmp(buffer,"VICTOR9K_GCR"))
				{
					ad->statestack[ad->stack_ptr].track_type = VICTOR9K_GCR;
				}

				hxcfe_setTrackType(ad->fb,ad->statestack[ad->stack_ptr].track_type);
			}
		break;
		case TRACK_PREGAP:
		case PREGAP:
			if(!ad->xmlcheck)
				hxcfe_setTrackPreGap (ad->fb,(unsigned short)atoi(buffer));
		break;
		case TRACK_PREGAP_US:
			if(!ad->xmlcheck)
				hxcfe_setTrackPreGap (ad->fb,atoi(buffer) | 0x40000000);
		break;
		case TRACK_GAP3:
		case GAP3:
			if(!ad->xmlcheck)
				hxcfe_setSectorGap3 (ad->fb,(unsigned char)atoi(buffer));
		break;
		case RPM:
			if(!ad->xmlcheck)
				hxcfe_setRPM (ad->fb,(unsigned short)atoi(buffer));
		break;
		case DISK_CRC32:
		break;
		case TRACK_LENGTH:
			if(!ad->xmlcheck)
				hxcfe_setRPM (ad->fb,(atoi(buffer)/10) | 0x40000000);
		break;
		case SECTORIDSTART:
			if(!ad->xmlcheck)
				hxcfe_setStartSectorID(ad->fb,(unsigned char)atoi(buffer));
		break;
		case TRACKID_SECTOR:
			if(!ad->xmlcheck)
				hxcfe_setSectorTrackID(ad->fb,(unsigned char)atoi(buffer));
		break;
		case SIDEID_SECTOR:
			if(!ad->xmlcheck)
				hxcfe_setSectorHeadID(ad->fb,(unsigned char)atoi(buffer));
		break;
		case SECTORID_SECTOR:
			if(!ad->xmlcheck)
				hxcfe_setSectorID(ad->fb,(unsigned char)atoi(buffer));
		break;

		case BITRATE:
			if(!ad->xmlcheck)
				hxcfe_setTrackBitrate(ad->fb,atoi(buffer));
		break;
		case DATAOFFSET:
			track = ad->statestack[ad->stack_ptr].cur_track;
			ad->ts[track].base_adress =  ahextoi(buffer);
		break;
		case DATAOFFSETSECTOR:
			if(!ad->xmlcheck)
			{
				track = ad->statestack[ad->stack_ptr].cur_track;
				tmp_base_adress =  ahextoi(buffer);

				sectorsize = hxcfe_getCurrentSectorSize(ad->fb);

				if(tmp_base_adress + sectorsize < ad->buffer_size)
				{
					hxcfe_setSectorData(ad->fb,&ad->image_data[tmp_base_adress],sectorsize);
				}
			}
		break;
		case DATAMARK_SECTOR:
			if(!ad->xmlcheck)
			{
				sscanf(buffer,"0x%X",&datamark);
				hxcfe_setSectorDataMark (ad->fb,(unsigned char)datamark);
			}
		break;
		case DATACRC_SECTOR:
			if(!ad->xmlcheck)
			{
				sscanf(buffer,"0x%X",&crc);
				hxcfe_setSectorDataCRC (ad->fb,(unsigned short)crc);
			}
		break;
		case HEADERCRC_SECTOR:
			if(!ad->xmlcheck)
			{
				sscanf(buffer,"0x%X",&crc);
				hxcfe_setSectorHeaderCRC (ad->fb,(unsigned short)crc);
			}
		break;
		case SECTORDATA_SECTOR:
			if(!ad->xmlcheck)
			{
				sectorsize = hxcfe_getCurrentSectorSize(ad->fb);

				if(!ad->sector_data)
				{
					ad->sector_data = malloc(sectorsize);
					if(ad->sector_data)
					{
						memset(ad->sector_data,0x00,sectorsize);
					}
					ad->sector_data_offset = 0;
				}

				if(ad->sector_data)
				{
					for(i=0;i<len/2;i++)
					{
						if(buffer[i*2]>='A')
							ad->sector_data[ad->sector_data_offset] = (uint8_t)(((buffer[i*2] - 'A') + 10 ) << 4);
						else
							ad->sector_data[ad->sector_data_offset] = (uint8_t)(((buffer[i*2] - '0') ) << 4);

						if(buffer[(i*2)+1]>='A')
							ad->sector_data[ad->sector_data_offset] |= (uint8_t)(((buffer[(i*2)+1] - 'A') + 10) & 0xF);
						else
							ad->sector_data[ad->sector_data_offset] |= (uint8_t)(((buffer[(i*2)+1] - '0')) & 0xF);

						ad->sector_data_offset++;
						if( ad->sector_data_offset == sectorsize )
							ad->sector_data_offset = sectorsize - 1;
					}
				}
			}
		break;

		case SET_INDEX_LENGTH:
			if(!ad->xmlcheck)
			{
				ad->index_length = atoi(buffer);
				hxcfe_setIndexLength(ad->fb,ad->cur_index_number,ad->index_length);
			}
		break;

		case SET_INDEX_POSITION:
			if(!ad->xmlcheck)
			{
				hxcfe_setIndexLength(ad->fb,ad->cur_index_number,ad->index_length);
				hxcfe_setIndexPosition(ad->fb,ad->cur_index_number,atoi(buffer),1);
			}

			ad->cur_index_number++;
		break;
	}

	free(buffer);
}

static void XMLCALL start(void *data, const char *el, const char **attr)
{
	int i;
	int newstate;
	int track,side,sector,sectorsize;
	AppData *ad = (AppData *) data;

	sector = 0xFF;
	sectorsize = 128;

	newstate = getnewstate((char*)el, ad->current_state);
	if ( newstate >= 0 )
	{
		ad->stack_ptr++;
		memcpy(&ad->statestack[ad->stack_ptr], &ad->statestack[ad->stack_ptr-1],sizeof(parse_stack));
		ad->statestack[ad->stack_ptr].state = newstate;
		ad->current_state = newstate;

		switch(ad->current_state)
		{
			case LAYOUT:
				if(!ad->xmlcheck)
					ad->fb = hxcfe_initFloppy(ad->floppycontext,80,2);
			break;
			case TRACK:
				i=0;
				side=0;
				track=0;

				while(attr[i] && strcmp("track_number",attr[i]) )
				{
					i++;
				}
				if(attr[i+1])
				{
					track = atoi(attr[i+1]);
				}

				i=0;
				while(attr[i] && strcmp("side_number",attr[i]) )
				{
					i++;
				}
				if(attr[i+1])
				{
					side = atoi(attr[i+1]);
				}

				ad->statestack[ad->stack_ptr].cur_track = (track<<1) | (side&1);
				ad->ts[(track<<1) | (side&1)].set=1;

				if(!ad->xmlcheck)
					hxcfe_pushTrackPFS (ad->fb,track,side);
			break;

			case SECTOR:
				i=0;
				while(attr[i] && strcmp("sector_id",attr[i]) )
				{
					i++;
				}
				if(attr[i+1])
				{
					sector = atoi(attr[i+1]);
				}

				i=0;
				while(attr[i] && strcmp("sector_size",attr[i]) )
				{
					i++;
				}
				if(attr[i+1])
				{
					sectorsize = atoi(attr[i+1]);
				}

				track = ad->statestack[ad->stack_ptr].cur_track;

				if(!ad->xmlcheck)
				{
					hxcfe_pushSector(ad->fb);

					hxcfe_setSectorHeadID(ad->fb,(unsigned char)(track&1));
					hxcfe_setSectorTrackID(ad->fb,(unsigned char)(track>>1));
					hxcfe_setSectorID(ad->fb,(unsigned char)sector);
					hxcfe_setSectorSize(ad->fb,sectorsize);
					if(ad->ts[track].base_adress + ad->ts[track].track_size + sectorsize <= ad->buffer_size)
					{
						hxcfe_setSectorData(ad->fb,&ad->image_data[ad->ts[track].base_adress + ad->ts[track].track_size],sectorsize);
					}
				}

				ad->ts[track].track_size +=  sectorsize;

			break;
		}
	}
	else
	{
		XML_StopParser(ad->p, 0);
	}
}

static void XMLCALL end(void *data, const char *el)
{
	AppData *ad = (AppData *) data;
	int state;

	state = getstate((char*)el, ad->current_state);

	if(!ad->xmlcheck)
	{

		switch(state)
		{
			case LAYOUT:
				generateDisk(ad,ad->image_data,ad->buffer_size);
				ad->floppy = hxcfe_getFloppy (ad->fb);

				if(ad->interface_mode>=0)
					hxcfe_floppySetInterfaceMode(ad->floppycontext,ad->floppy,ad->interface_mode);

				if(ad->write_protect)
					hxcfe_floppySetFlags(ad->floppycontext, ad->floppy, HXCFE_FLOPPY_WRPROTECTED_FLAG );
			break;

			case TRACK:
				hxcfe_popTrack (ad->fb);
			break;

			case SECTOR:
				if(ad->sector_data)
				{
					hxcfe_setSectorData(ad->fb,ad->sector_data,hxcfe_getCurrentSectorSize(ad->fb));
					free(ad->sector_data);
					ad->sector_data = 0;
					ad->sector_data_offset = 0;
				}
				hxcfe_popSector(ad->fb);
			break;
		}
	}

	if(ad->stack_ptr)
	{
		ad->stack_ptr--;
	}

	ad->current_state = ad->statestack[ad->stack_ptr].state;

}

static void XMLCALL ns_start(void *data, const char *prefix, const char *uri)
{

}

static void XMLCALL ns_end( void *data, const char *prefix )
{

}

HXCFE_XMLLDR* hxcfe_initXmlFloppy( HXCFE* floppycontext )
{
	AppData *ad;
	HXCFE_XMLLDR * rfw;
	int i;

	rfw = malloc(sizeof(HXCFE_XMLLDR));
	if(rfw)
	{
		memset(rfw,0,sizeof(HXCFE_XMLLDR));

		rfw->xml_parser = XML_ParserCreate(NULL);
		ad = malloc(sizeof(AppData));
		if(!ad || !rfw->xml_parser)
		{
			free(ad);
			XML_ParserFree(rfw->xml_parser);
			free(rfw);
			return 0;
		}

		memset(ad,0,sizeof(AppData));

		ad->interface_mode = -1;
		ad->min_nb_of_tracks = -1;
		ad->max_nb_of_tracks = -1;
		ad->nb_of_tracks = -1;
		ad->file_size = -1;

		ad->cur_index_number = 0;
		ad->index_length = 0;

		i=0;
		while( disklayout_list[i])
		{
			if(disklayout_list[i]->unpacked_data)
			{
				free(disklayout_list[i]->unpacked_data);
				disklayout_list[i]->unpacked_data = 0;
			}

			disklayout_list[i]->unpacked_data = data_unpack(disklayout_list[i]->data,disklayout_list[i]->csize,0,disklayout_list[i]->size);
			i++;
		}

		ad->floppycontext = floppycontext;
		ad->current_state = ENTRY_STATE;
		ad->stack_ptr = 0;
		memset(ad->statestack,0xFF,sizeof(int) * 32);
		ad->statestack[0].state = ad->current_state;
		ad->statestack[0].track_type = IBMFORMAT_DD;
		ad->p = rfw->xml_parser;

		memset(ad->ts,0,sizeof(track_state)*MAX_TRACKS_NB*2);

		XML_ParserReset(rfw->xml_parser, NULL);

		XML_SetUserData(rfw->xml_parser, (void *) ad);
		XML_SetElementHandler(rfw->xml_parser, start, end);

		XML_SetCharacterDataHandler(rfw->xml_parser, charhandler);
		XML_SetNamespaceDeclHandler(rfw->xml_parser, ns_start, ns_end);

		rfw->ad = ad;
	}

	return rfw;
}

void hxcfe_deinitXmlFloppy( HXCFE_XMLLDR* xmlfb_ctx )
{
	int i;

	XML_ParserFree(xmlfb_ctx->xml_parser);

	i=0;
	while( disklayout_list[i])
	{
		if(disklayout_list[i]->unpacked_data)
		{
			free(disklayout_list[i]->unpacked_data);
			disklayout_list[i]->unpacked_data = 0;
		}
		i++;
	}

	free(xmlfb_ctx);
}

int32_t hxcfe_getXmlLayoutID( HXCFE_XMLLDR* xmlfb_ctx, char * container )
{
	int i;
	AppData *ad = (AppData *) xmlfb_ctx->ad;

	i = 0;
	do
	{
		XML_ParserReset(xmlfb_ctx->xml_parser, NULL);
		XML_SetUserData(xmlfb_ctx->xml_parser, (void *) ad);
		XML_SetElementHandler(xmlfb_ctx->xml_parser, start, end);
		XML_SetCharacterDataHandler(xmlfb_ctx->xml_parser, charhandler);
		XML_SetNamespaceDeclHandler(xmlfb_ctx->xml_parser, ns_start, ns_end);

		ad->xmlcheck = 1;
		XML_Parse(xmlfb_ctx->xml_parser, (char*)disklayout_list[i]->unpacked_data, disklayout_list[i]->size, 1);

		if(!strcmp((char*)ad->name,container))
		{
			return i;
		}

		memset(ad->name,0,512);

		i++;
	}while(disklayout_list[i]);

	return -1;
}

const char* hxcfe_getXmlLayoutDesc( HXCFE_XMLLDR* xmlfb_ctx, int32_t moduleID )
{
	AppData *ad = (AppData *) xmlfb_ctx->ad;

	if(hxcfe_numberOfXmlLayout(xmlfb_ctx) > moduleID)
	{
		ad->xmlcheck = 1;
		XML_ParserReset(xmlfb_ctx->xml_parser, NULL);
		XML_SetUserData(xmlfb_ctx->xml_parser, (void *) ad);
		XML_SetElementHandler(xmlfb_ctx->xml_parser, start, end);
		XML_SetCharacterDataHandler(xmlfb_ctx->xml_parser, charhandler);
		XML_SetNamespaceDeclHandler(xmlfb_ctx->xml_parser, ns_start, ns_end);

		XML_Parse(xmlfb_ctx->xml_parser, (char*)disklayout_list[moduleID]->unpacked_data, disklayout_list[moduleID]->size, 1);

		return (const char*)ad->description;
	}

	return NULL;
}

const char* hxcfe_getXmlLayoutName( HXCFE_XMLLDR* xmlfb_ctx, int32_t moduleID )
{
	AppData *ad = (AppData *) xmlfb_ctx->ad;

	if(hxcfe_numberOfXmlLayout(xmlfb_ctx) > moduleID)
	{
		ad->xmlcheck = 1;
		XML_ParserReset(xmlfb_ctx->xml_parser, NULL);
		XML_SetUserData(xmlfb_ctx->xml_parser, (void *) ad);
		XML_SetElementHandler(xmlfb_ctx->xml_parser, start, end);
		XML_SetCharacterDataHandler(xmlfb_ctx->xml_parser, charhandler);
		XML_SetNamespaceDeclHandler(xmlfb_ctx->xml_parser, ns_start, ns_end);

		XML_Parse(xmlfb_ctx->xml_parser, (char*)disklayout_list[moduleID]->unpacked_data, disklayout_list[moduleID]->size, 1);

		return (const char*)ad->name;
	}

	return NULL;
}

int32_t hxcfe_numberOfXmlLayout( HXCFE_XMLLDR* xmlfb_ctx )
{
	int32_t i;

	i = 0;
	while( disklayout_list[i] )
	{
		i++;
	}

	return i;
}


int32_t hxcfe_selectXmlFloppyLayout( HXCFE_XMLLDR* xmlfb_ctx, int32_t layoutid )
{
	AppData *ad = (AppData *) xmlfb_ctx->ad;

	if(hxcfe_numberOfXmlLayout(xmlfb_ctx) > layoutid)
	{
		memset((char*)&ad->xmlfile_path,0,sizeof(ad->xmlfile_path));
		ad->layout_id = layoutid;
		return HXCFE_NOERROR;
	}
	return HXCFE_BADPARAMETER;
}

int32_t hxcfe_setXmlFloppyLayoutFile(HXCFE_XMLLDR* xmlfb_ctx,char * filepath)
{
	char firstline[512];
	FILE * f;
	AppData *ad = (AppData *) xmlfb_ctx->ad;

	if(hxc_checkfileext(filepath,"xml",SYS_PATH_TYPE))
	{
		f = hxc_fopen(filepath,"rb");
		if( f )
		{
			memset(firstline,0,sizeof(firstline));
			hxc_fgets(firstline,sizeof(firstline)-1,f);
			hxc_fclose(f);

			if(strstr(firstline,"<?xml version="))
			{
				strcpy((char*)&ad->xmlfile_path,filepath);
				ad->layout_id = -1;
				ad->floppycontext->hxc_printf(MSG_DEBUG,"hxcfe_setXmlFloppyLayoutFile : XML file !");
				return HXCFE_NOERROR;
			}

			ad->floppycontext->hxc_printf(MSG_DEBUG,"hxcfe_setXmlFloppyLayoutFile : non XML file !");
			return HXCFE_BADFILE;
		}
	}

	return HXCFE_BADPARAMETER;
}


HXCFE_FLOPPY* hxcfe_generateXmlFloppy ( HXCFE_XMLLDR* xmlfb_ctx, uint8_t * rambuffer, uint32_t buffersize )
{
	AppData *ad;
	FILE *f;
	int filesize;
	char * xmlbuffer;

	ad = xmlfb_ctx->ad;

	ad->xmlcheck = 0;

	ad->image_data = rambuffer;
	ad->buffer_size = buffersize;

	memset(ad->ts,0,sizeof(track_state)*MAX_TRACKS_NB*2);

	XML_ParserReset(xmlfb_ctx->xml_parser, NULL);
	XML_SetUserData(xmlfb_ctx->xml_parser, (void *) ad);
	XML_SetElementHandler(xmlfb_ctx->xml_parser, start, end);
	XML_SetCharacterDataHandler(xmlfb_ctx->xml_parser, charhandler);
	XML_SetNamespaceDeclHandler(xmlfb_ctx->xml_parser, ns_start, ns_end);

	if(ad->layout_id!=-1)
	{
		XML_Parse(xmlfb_ctx->xml_parser, (char*)disklayout_list[ad->layout_id]->unpacked_data, disklayout_list[ad->layout_id]->size, 1);
	}
	else
	{
		ad->floppy = 0;

		f = hxc_fopen((char*)ad->xmlfile_path,"rb");
		if(f)
		{
			filesize = hxc_fgetsize(f);
			if(filesize>0)
			{
				xmlbuffer = malloc(filesize + 1);
				if(xmlbuffer)
				{
					memset(xmlbuffer,0,filesize + 1);
					hxc_fread(xmlbuffer,filesize,f);

					XML_Parse(xmlfb_ctx->xml_parser, xmlbuffer, filesize, 1);

					free(xmlbuffer);
				}
			}

			hxc_fclose(f);
		}
	}

	return ad->floppy;
}

static int is_size_matching( HXCFE_XMLLDR* xmlfb_ctx, int image_size)
{
	AppData *ad;
	int i;
	int track_min;
	int track_max;
	int total_size;

	ad = xmlfb_ctx->ad;

	track_min = ad->nb_of_tracks;
	track_max = ad->nb_of_tracks;

	if( ad->min_nb_of_tracks != -1 )
		track_min = ad->min_nb_of_tracks;

	if( ad->max_nb_of_tracks != -1 )
		track_max = ad->max_nb_of_tracks;

	for( i = 0; i < track_max * 2 ; i++)
	{
		if(!ad->ts[i].set)
		{
			ad->ts[i].track_size = ad->nb_sectors_per_track * ad->sector_size;
			ad->ts[i].set = 1;
		}
	}

	i = 0;
	total_size = 0;
	while(i < track_min * 2 )
	{
		total_size += ad->ts[i].track_size;
		i++;

		if(ad->nb_of_sides == 2)
			total_size += ad->ts[i].track_size;
		i++;
	}

	for( i = track_min * 2; i <= track_max * 2; i += 2 )
	{
		total_size += ad->ts[i].track_size;

		if(ad->nb_of_sides == 2)
			total_size += ad->ts[i+1].track_size;

		if( total_size == image_size )
		{
			return i / 2;
		}
	}

	return -1;
}

int32_t hxcfe_isMatchingXmlFloppy ( HXCFE_XMLLDR* xmlfb_ctx, char * filename, uint8_t * rambuffer, uint32_t buffersize )
{
	AppData *ad;
	FILE *f;
	int filesize;
	char * xmlbuffer;

	ad = xmlfb_ctx->ad;

	ad->xmlcheck = 1;

	ad->image_data = rambuffer;
	ad->buffer_size = buffersize;

	memset(ad->ts,0,sizeof(track_state)*MAX_TRACKS_NB*2);

	XML_ParserReset(xmlfb_ctx->xml_parser, NULL);
	XML_SetUserData(xmlfb_ctx->xml_parser, (void *) ad);
	XML_SetElementHandler(xmlfb_ctx->xml_parser, start, end);
	XML_SetCharacterDataHandler(xmlfb_ctx->xml_parser, charhandler);
	XML_SetNamespaceDeclHandler(xmlfb_ctx->xml_parser, ns_start, ns_end);

	if(ad->layout_id!=-1)
	{
		XML_Parse(xmlfb_ctx->xml_parser, (char*)disklayout_list[ad->layout_id]->unpacked_data, disklayout_list[ad->layout_id]->size, 1);
	}
	else
	{
		ad->floppy = 0;

		f = hxc_fopen((char*)ad->xmlfile_path,"rb");
		if(f)
		{
			filesize = hxc_fgetsize(f);
			if(filesize>0)
			{
				xmlbuffer = malloc(filesize + 1);
				if(xmlbuffer)
				{
					memset(xmlbuffer,0,filesize + 1);
					hxc_fread(xmlbuffer,filesize,f);

					XML_Parse(xmlfb_ctx->xml_parser, xmlbuffer, filesize, 1);

					free(xmlbuffer);
				}
			}

			hxc_fclose(f);
		}
		else
		{
			return -1;
		}
	}

	// Check image.

	// Check file size
	if( ad->file_size != -1 )
	{
		if( buffersize != (uint32_t)ad->file_size )
		{
			return 0;
		}
	}

	// Check file extension(s).
	if(filename)
	{
		if( strlen( ad->file_extensions ) && strlen( filename ) )
		{
			if ( !is_ext_matching( filename, ad->file_extensions ) )
			{
				return 0;
			}
		}
	}

	// Check total raw sectors size
	if ( !is_size_matching( xmlfb_ctx, buffersize ) )
	{
		return 0;
	}

	return 1;
}

HXCFE_FLOPPY* hxcfe_generateXmlFileFloppy (HXCFE_XMLLDR* xmlfb_ctx,char *file)
{
	FILE * f;
	unsigned int filesize;
	HXCFE_FLOPPY* ret;
	unsigned char * buffer;

	ret = 0;
	f = hxc_fopen(file,"rb");
	if(f)
	{
		filesize = hxc_fgetsize(f);

		if(filesize)
		{
			if(filesize > 32*1024*1024)
			{
				filesize = 32*1024*1024;
			}

			buffer = malloc(filesize);
			if(buffer)
			{
				memset(buffer,0,filesize);

				hxc_fread(buffer, filesize, f);

				ret = hxcfe_generateXmlFloppy (xmlfb_ctx,buffer,filesize);

				free(buffer);
			}
		}

		hxc_fclose(f);
	}

	return ret;
}

int32_t hxcfe_foundMatchingXmlFileFloppy (HXCFE_XMLLDR* xmlfb_ctx,char *file)
{
	FILE * f;
	unsigned int filesize,i,number_of_layout;
	int32_t ret;
	unsigned char * buffer;

	ret = HXCFE_UNSUPPORTEDFILE;

	f = hxc_fopen(file,"rb");
	if(f)
	{
		filesize = hxc_fgetsize(f);

		if(filesize)
		{
			if(filesize > 32*1024*1024)
			{
				filesize = 32*1024*1024;
			}

			buffer = malloc(filesize);
			if(buffer)
			{
				memset(buffer,0,filesize);

				hxc_fread(buffer, filesize, f);

				number_of_layout = hxcfe_numberOfXmlLayout( xmlfb_ctx );

				i = 0;
				while( i < number_of_layout)
				{
					if( hxcfe_selectXmlFloppyLayout( xmlfb_ctx, i ) == HXCFE_NOERROR )
					{
						ret = hxcfe_isMatchingXmlFloppy(xmlfb_ctx,file,buffer,filesize);

						if( ret > 0 )
						{
							free(buffer);
							hxc_fclose(f);
							return i;
						}
					}
					i++;
				}

				free(buffer);
			}
		}

		hxc_fclose(f);
	}

	return ret;
}
