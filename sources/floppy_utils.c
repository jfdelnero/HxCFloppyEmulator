/*
//
// Copyright (C) 2006-2014 Jean-François DEL NERO
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
// File : floppy_utils.c
// Contains: utils functions
//
// Written by:	DEL NERO Jean Francois
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "types.h"

#include "internal_libhxcfe.h"
#include "tracks/track_generator.h"
#include "sector_search.h"
#include "libhxcfe.h"
#include "floppy_loader.h"

#include "libhxcadaptor.h"

int32_t us2index(int32_t startindex,HXCFE_SIDE * track,uint32_t us,unsigned char fill,char fillorder)
{
	uint32_t time,freq;

	if(!fillorder)
	{
		if(track->bitrate==VARIABLEBITRATE)
		{
			time=0;
			do
			{
				if(fill)
					track->indexbuffer[startindex>>3]=0xFF;

				freq = track->timingbuffer[startindex>>3];

				startindex++;

				if(startindex >= track->tracklen) 
					startindex = 0;

				if(freq)
					time = time + (((1000000000/2)/freq));

			}while(us>(time/1000));

			return startindex;
		}
		else
		{
			freq=track->bitrate;
			time=0;

			if ( freq )
			{
				do
				{
					if(fill)
						track->indexbuffer[startindex>>3]=0xFF;

					startindex++;

					if(startindex >= track->tracklen)
						startindex = 0;

					time = time + (((1000000000/2) / freq));

				}while(us>(time/1000));
			}
			return startindex;
		}
	}
	else
	{
		if(track->bitrate==VARIABLEBITRATE)
		{
			time=0;
			do
			{
				if(fill)track->indexbuffer[startindex>>3]=0xFF;
				freq=track->timingbuffer[startindex>>3];

				if(startindex)
					startindex--;
				else
					startindex=track->tracklen-1;

				if(freq)
					time = time + (( (1000000000/2) / freq));

			}while(us>(time/1000));

			return startindex;
		}
		else
		{
			freq=track->bitrate;
			time=0;
			if( freq )
			{
				do
				{
					if(fill)track->indexbuffer[startindex>>3]=0xFF;

					if(startindex)
						startindex--;
					else
						startindex=track->tracklen-1;

					time = time + (((1000000000/2)/freq));
				}while(us>(time/1000));
			}
			return startindex;
		}
	}
};

int32_t fillindex(int32_t startindex,HXCFE_SIDE * track,uint32_t us,unsigned char fill,char fillorder)
{
	int32_t start_index;

	if(startindex>=0)
	{
		start_index=us2index(0,track,startindex,0,0);
	}
	else
	{
		start_index=us2index(0,track,-startindex,0,1);
	}

	return us2index(start_index,track,us&0xFFFFFF,fill,fillorder);
}

HXCFE_CYLINDER* allocCylinderEntry(int32_t rpm,int32_t number_of_side)
{
	HXCFE_CYLINDER* cyl;

	cyl = 0;
	
	if(number_of_side>0)
	{
		cyl=(HXCFE_CYLINDER*)malloc(sizeof(HXCFE_CYLINDER));
		cyl->floppyRPM=rpm;
		cyl->number_of_side=number_of_side;
		cyl->sides=(HXCFE_SIDE**)malloc(sizeof(HXCFE_SIDE*)*number_of_side);
		memset(cyl->sides,0,sizeof(HXCFE_SIDE*)*number_of_side);
	}

	return cyl;
}

void savebuffer(char * name,unsigned char * buffer, int size)
{
	FILE * f;

	f=hxc_fopen(name,"w+b");
	if(f)
	{
		fwrite(buffer,size,1,f);
		hxc_fclose(f);
	}
}

double GetTrackPeriod(HXCFE* floppycontext,HXCFE_SIDE * curside)
{
	int tracklen,i;
	double total_period;

	total_period = 0;

	if(floppycontext)
	{
		tracklen = curside->tracklen /8;
		if(curside->tracklen & 7)
			tracklen++;

		if(curside->timingbuffer)
		{
			for(i=0;i<tracklen;i++)
			{
				total_period = total_period + (double)((double)(1*4)/(double)curside->timingbuffer[i]);
			}
		}
		else
		{
			for(i=0;i<tracklen;i++)
			{
				total_period = total_period + (double)((double)(1*4)/(double)curside->bitrate);
			}
		}
	}

	if(total_period)
		return total_period;
	else
		return 1;
}

double MeasureTrackTiming(HXCFE* floppycontext,HXCFE_SIDE * curside,int32_t startpulse,int32_t endpulse)
{
	uint32_t lenbyte,i,lenbit;
	double   total_period;

	total_period = 0;

	if(floppycontext)
	{
		if( (startpulse < curside->tracklen) && (endpulse < curside->tracklen) ) 
		{
			if(startpulse<=endpulse)
			{
				lenbit = endpulse - startpulse;
			}
			else
			{
				lenbit = endpulse + (curside->tracklen - startpulse);			
			}

			lenbyte = lenbit /8;
			if(lenbit & 7)
				lenbyte++;

			total_period = 0;
			if(curside->timingbuffer)
			{
				for(i=0;i<lenbyte;i++)
				{
					total_period = total_period + (double)((double)(1*4)/(double)curside->timingbuffer[((startpulse/8) + i)%(curside->tracklen/8)]);
				}
			}
			else
			{
				for(i=0;i<lenbyte;i++)
				{
					total_period = total_period + (double)((double)(1*4)/(double)curside->bitrate);
				}
			}
		}
	}

	if(total_period)
		return total_period;
	else
		return 1;
}

unsigned char tracktypelisttoscan[]=
{
	ISOIBM_MFM_ENCODING,
	ISOIBM_FM_ENCODING,
	AMIGA_MFM_ENCODING,
	EMU_FM_ENCODING,
	ARBURGSYS_ENCODING,
	ARBURGDAT_ENCODING,
	UNKNOWN_ENCODING
};

int floppyTrackTypeIdentification(HXCFE* floppycontext,HXCFE_FLOPPY *fp)
{
	int i,j,t;
	int sectnum;
	unsigned char first_track_encoding;
	int nb_sectorfound;
	HXCFE_SECTORACCESS* ss;
	HXCFE_SECTCFG** scl;
	
	i = 0;
	
	first_track_encoding = UNKNOWN_ENCODING;

	ss = hxcfe_initSectorAccess(floppycontext,fp);
	if(ss)
	{
		for(i=0;i<fp->floppyNumberOfTrack;i++)
		{
			for(j=0;j<fp->floppyNumberOfSide;j++)
			{

				nb_sectorfound = 0;
				t=0;
				do{
					first_track_encoding = tracktypelisttoscan[t];
					scl = hxcfe_getAllTrackSectors(ss,i,j,first_track_encoding,&nb_sectorfound);
					ss->old_bitoffset = 0;
					ss->bitoffset = 0;
					t++;
				}while(!nb_sectorfound && (tracktypelisttoscan[t] != UNKNOWN_ENCODING));

				if(nb_sectorfound)
				{
					fp->tracks[i]->sides[j]->track_encoding = first_track_encoding ;
				}

				if(scl)
				{
					for(sectnum=0;sectnum<nb_sectorfound;sectnum++)
					{
						if(first_track_encoding == UNKNOWN_ENCODING)
							first_track_encoding = tracktypelisttoscan[i];

						hxcfe_freeSectorConfig  (ss,scl[sectnum]);
					}
					
					free(scl);
				}
			}
		}

		hxcfe_deinitSectorAccess(ss);

	}

	return 0;
}

unsigned char  size_to_code(uint32_t size)
{

	switch(size)
	{
		case 128:
			return 0;
		break;
		case 256:
			return 1;
		break;
		case 512:
			return 2;
		break;
		case 1024:
			return 3;
		break;
		case 2048:
			return 4;
		break;
		case 4096:
			return 5;
		break;
		case 8192:
			return 6;
		break;
		case 16384:
			return 7;
		break;
		default:
			return 0;
		break;
	}
}
