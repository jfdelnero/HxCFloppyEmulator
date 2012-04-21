 /*
//
// Copyright (C) 2006 - 2012 Jean-François DEL NERO
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
// File : display_track.c
// Contains: 
//
// Written by:	DEL NERO Jean Francois
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "libhxcfe.h"

#include "crc.h"
#include "floppy_utils.h"
#include "math.h"

#include "display_track.h"


s_trackdisplay * hxcfe_td_init(HXCFLOPPYEMULATOR* floppycontext,unsigned long xsize,unsigned long ysize,unsigned long x_us,unsigned long y_us,unsigned long x_start_us)
{
	s_trackdisplay * td;
	
	td=malloc(sizeof(s_trackdisplay));
	memset(td,0,sizeof(s_trackdisplay));
	
	td->xsize=xsize;
	td->ysize=ysize;
	
	td->framebuffer=malloc(td->xsize*td->ysize*sizeof(unsigned long));
	memset(td->framebuffer,0,td->xsize*td->ysize*sizeof(unsigned long));

	td->x_us=x_us;
	td->y_us=y_us;

	td->x_start_us=x_start_us;

	return td;
}

#pragma pack(1)
typedef struct s_col_
{
	unsigned char red;
	unsigned char green;
	unsigned char blue;
	unsigned char spare;
}s_col;
#pragma pack()

double getOffsetTiming(SIDE *currentside,int offset,double timingoffset,int start)
{
	int i,bitrate;

	i=start;
	//timingoffset=0;

	do
	{
		if(currentside->bitrate==VARIABLEBITRATE)
			bitrate=currentside->timingbuffer[i>>3];
		else
			bitrate=currentside->bitrate;

		timingoffset = timingoffset + ((double)(500000)/(double)bitrate);
		i++;
	}while(i<offset);

	return timingoffset;
}

void display_sectors(HXCFLOPPYEMULATOR* floppycontext,s_trackdisplay *td,FLOPPY * floppydisk,int track,int side,double timingoffset_offset, int TRACKTYPE)
{
	int tracksize;
	int i,j,old_i;
	
	double timingoffset;
	double timingoffset2;
	int xpos,xpos2;
	int endfill,loop;
	s_col * col;
	SECTORSEARCH* ss;
	SECTORCONFIG* sc;
	SIDE * currentside;

	old_i=0;
	timingoffset=0;//timingoffset_offset;
	loop=0;
	endfill=0;

	currentside=floppydisk->tracks[track]->sides[side];
	tracksize=currentside->tracklen;
	do
	{

		timingoffset = ( getOffsetTiming(currentside,tracksize,0,0) * loop );
		xpos= (int)( ( timingoffset - timingoffset_offset ) / ((double)td->x_us/(double)td->xsize) );
		if( (xpos<td->xsize) )
		{

			old_i=0;

			ss=hxcfe_initSectorSearch(floppycontext,floppydisk);
			if(ss)
			{				
				endfill=0;
				do
				{
					sc=hxcfe_getNextSector(ss,track,side,TRACKTYPE);
					if(sc)
					{
						timingoffset = getOffsetTiming(currentside,sc->startsectorindex,timingoffset,old_i);
						old_i = sc->startsectorindex;
						xpos = (int)( ( timingoffset - timingoffset_offset ) / ((double)td->x_us/(double)td->xsize) );
						timingoffset2 = getOffsetTiming(currentside,sc->endsectorindex,timingoffset,old_i);
						//old_i=sc->endsectorindex;
						xpos2 = (int)( ( timingoffset2 - timingoffset_offset ) / ((double)td->x_us/(double)td->xsize) );

						for(i=xpos;i<xpos2;i++)
						{
							if(i>=0)
							{
								if( (i<td->xsize) )
								{
									if(sc->use_alternate_data_crc)
									{
										for(j=50;j<(td->ysize-10);j++)
										{
											col=(s_col *)&td->framebuffer[(td->xsize*j) + i];
											col->blue=3*col->blue/4;
											col->green=3*col->red/4;
										}
									}
									else
									{
										for(j=50;j<(td->ysize-10);j++)
										{
											col=(s_col *)&td->framebuffer[(td->xsize*j) + i];
											col->blue=3*col->blue/4;
											col->red=3*col->red/4;
										}
									}
								}
								else
								{
									endfill=1;
								}
							}
						}

						hxcfe_freeSectorConfig(ss,sc);
					}
				}while(sc);

				hxcfe_deinitSectorSearch(ss);

				loop++;

			}
		}
		else
			endfill=1;
	}while(!endfill);

}


void hxcfe_td_draw_track(HXCFLOPPYEMULATOR* floppycontext,s_trackdisplay *td,FLOPPY * floppydisk,int track,int side)
{
	int tracksize;
	int i,j,old_i;
	
	double timingoffset_offset;
	int	   buffer_offset;

	double timingoffset;
	double timingoffset2;
	int interbit;
	int bitrate;
	int xpos,ypos;
	int endfill;
	SIDE * currentside;
	s_col * col;

	currentside=floppydisk->tracks[track]->sides[side];


	tracksize=currentside->tracklen;
	
	old_i=0;
	i=0;

	timingoffset = ( getOffsetTiming(currentside,tracksize,0,0));


	timingoffset2=( timingoffset * td->x_start_us ) / (100 * 1000);
	timingoffset=0;
	while((i<tracksize) && timingoffset2>timingoffset)
	{			
		timingoffset=getOffsetTiming(currentside,i,timingoffset,old_i);
		old_i=i;
		i++;
	};

	buffer_offset=i;
	timingoffset_offset=timingoffset;

	tracksize=currentside->tracklen;
	timingoffset=0;
	interbit=0;
	i=buffer_offset;
	endfill=0;
	do
	{
		do
		{	
			
			if( currentside->databuffer[i>>3] & (0x80>>(i&7)) )
			{
				if(currentside->bitrate==VARIABLEBITRATE)
					bitrate=currentside->timingbuffer[i>>3];
				else
					bitrate=currentside->bitrate;
				
				xpos= (int)( timingoffset / ((double)td->x_us/(double)td->xsize) );
				ypos=td->ysize - (int)( ((double)(interbit * 1000000 )/ (double) bitrate) / (double)((double)td->y_us/(double)td->ysize));

				if( (xpos<td->xsize) && (ypos<td->ysize) && ypos>=0 )
				{
					td->framebuffer[(td->xsize*ypos) + xpos]++;
				}

				if(xpos>td->xsize)
					endfill=1;

				interbit=0;
			}
			else
			{
				interbit++;
			}

			if(currentside->bitrate==VARIABLEBITRATE)
				bitrate=currentside->timingbuffer[i>>3];
			else
				bitrate=currentside->bitrate;

			timingoffset=timingoffset + ((double)(500000)/(double)bitrate);

			i++;

		}while(i<tracksize);

		xpos= (int)( timingoffset / ((double)td->x_us/(double)td->xsize) );
		if(xpos>td->xsize)
			endfill=1;

		i=0;

	}while(!endfill);

	for(i=0;i<td->xsize*td->ysize;i++)
	{
		col=(s_col *)&td->framebuffer[i];

		col->spare=0;
		if(!col->red && !col->green && !col->blue)
		{
			col->red=255;
			col->green=255;
			col->blue=255;
		}
		else
		{
			col->green=255 - (col->red + 40);
			col->blue=255 - (col->red + 40);
			col->red=255 - (col->red + 40);
		}
	}

	if(currentside->flakybitsbuffer)
	{
		tracksize=currentside->tracklen;;
		timingoffset=0;
		i=buffer_offset;
		old_i=buffer_offset;

		endfill=0;
		do
		{
			do
			{	

				timingoffset=getOffsetTiming(currentside,i,timingoffset,old_i);
				old_i=i;
				xpos= (int) ( timingoffset / ((double)td->x_us/(double)td->xsize) );
				if( (xpos>=td->xsize) )
				{
					endfill=1;
				}
				
				if( currentside->flakybitsbuffer[i>>3] & (0x80>>(i&7)) )
				{
					if( (xpos<td->xsize) )
					{
						for(j=30;j<50;j++)
						{
							col=(s_col *)&td->framebuffer[(td->xsize*j) + xpos];
							col->blue=0;
							col->green=0;
							col->red=255;
						}
					}
				}

				i++;

			}while(i<tracksize);

			i=0;
			old_i=0;
		}while(!endfill);

	}

	tracksize=currentside->tracklen;
	old_i=buffer_offset;
	i=buffer_offset;
	timingoffset=0;
	endfill=0;
	do
	{
		do
		{			
			timingoffset=getOffsetTiming(currentside,i,timingoffset,old_i);
			old_i=i;
			xpos= (int)( timingoffset / ((double)td->x_us/(double)td->xsize) );

			if( (xpos<td->xsize) )
			{
				if( currentside->indexbuffer[i>>3] )
				{	
					for(j=10;j<12;j++)
					{
						col=(s_col *)&td->framebuffer[(td->xsize*j) + xpos];
						col->blue=255;
						col->green=0;
						col->red=0;
					}
				}
				else
				{
					for(j=25;j<27;j++)
					{
						col=(s_col *)&td->framebuffer[(td->xsize*j) + xpos];
						col->blue=255;
						col->green=0;
						col->red=0;
					}
				}
			}
			else
			{
				endfill=1;
			};
			i++;
		}while(i<tracksize);

		old_i=0;
		i=0;

	}while(!endfill);

	display_sectors(floppycontext,td,floppydisk,track,side,timingoffset_offset,ISOIBM_MFM_ENCODING);
	//display_sectors(floppycontext,td,floppydisk,track,side,timingoffset_offset,AMIGA_MFM_ENCODING);
	display_sectors(floppycontext,td,floppydisk,track,side,timingoffset_offset,ISOIBM_FM_ENCODING);
	//display_sectors(floppycontext,td,floppydisk,track,side,timingoffset_offset,EMU_FM_ENCODING);
}

void hxcfe_td_deinit(HXCFLOPPYEMULATOR* floppycontext,s_trackdisplay *td)
{
	free(td->framebuffer);
	free(td);
}
