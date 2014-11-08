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
#include <math.h>

#include "internal_libhxcfe.h"
#include "tracks/track_generator.h"
#include "libhxcfe.h"

#include "crc.h"
#include "floppy_utils.h"

#include "display_track.h"

#include "font.h"

#define PI    ((float)  3.141592654f)

int dummy_graph_progress(unsigned int current,unsigned int total,void * td,void * user)
{
    return 0;
}

HXCFE_TD * hxcfe_td_init(HXCFE* floppycontext,unsigned long xsize,unsigned long ysize)
{
	HXCFE_TD * td;

	td = malloc(sizeof(HXCFE_TD));
	if(td)
	{
		memset(td,0,sizeof(HXCFE_TD));
		td->hxcfe = floppycontext;
		td->xsize = xsize;
		td->ysize = ysize;

		td->x_us=200*1000;
		td->y_us=64;

		td->x_start_us=0;

		td->framebuffer = malloc(td->xsize*td->ysize*sizeof(unsigned int));

		td->hxc_setprogress = dummy_graph_progress;
		td->progress_userdata = 0;

		if(td->framebuffer)
		{
			memset(td->framebuffer,0,td->xsize*td->ysize*sizeof(unsigned int));
		}
		else
		{
			free(td);
			td = 0;
		}
	}
	return td;
}

int hxcfe_td_setProgressCallback(HXCFE_TD *td,HXCFE_TDPROGRESSOUT_FUNC progress_func,void * userdata)
{
	if(td)
	{
		if(progress_func)
		{
			td->progress_userdata = userdata;
			td->hxc_setprogress = progress_func;
		}
	}
	return 0;
}
void hxcfe_td_setparams(HXCFE_TD *td,unsigned long x_us,unsigned long y_us,unsigned long x_start_us)
{
	if(td)
	{
		td->x_us=x_us;
		td->y_us=y_us;

		td->x_start_us=x_start_us;
	}
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


struct s_sectorlist_ * getlastelement(struct s_sectorlist_ * element)
{
	while(element->next_element)
	{
		element=element->next_element;
	}

	return element;
}

struct s_sectorlist_ * addelement(struct s_sectorlist_ * element,void *object)
{
	while(element->next_element)
	{
		element=element->next_element;
	}

	element->next_element = malloc(sizeof(struct s_sectorlist_));
	if(element->next_element)
	{
		memset(element->next_element,0,sizeof(struct s_sectorlist_));
	}

	return element->next_element;
}

void freelist(struct s_sectorlist_ * element)
{
	struct s_sectorlist_ * nextelement;

	nextelement = element->next_element;

	if(element)
		free(element);

	if(nextelement)
		freelist(nextelement);

	return;
}

double getOffsetTiming(HXCFE_SIDE *currentside,int offset,double timingoffset,int start)
{
	int i,j,totaloffset;
	unsigned long bitrate;

	if( offset >= start )
	{
		totaloffset = offset;
	}
	else
	{
		totaloffset = start + ((currentside->tracklen - start) + offset);
	}

	i = start;
	j = start%currentside->tracklen;

	if(currentside->bitrate==VARIABLEBITRATE)
	{
		while( i < totaloffset )
		{
			bitrate = currentside->timingbuffer[j>>3];
			timingoffset = timingoffset + ((double)(500000)/(double)bitrate);
			i++;
			j = (j+1) % currentside->tracklen;
		}
	}
	else
	{
		timingoffset = timingoffset + ( ((double)(500000)/(double)currentside->bitrate) * ( totaloffset - start ) );
	}

	return timingoffset;
}

void putchar8x8(HXCFE_TD *td,int x_pos,int y_pos,unsigned char c,unsigned int color,int vertical)
{
	int charoffset;
	int xpos,ypos;
	int i,j;
	s_col * col;

	charoffset=(c&0x7F)*8;

	for(j=0;j<8;j++) // y
	{
		for(i=0;i<8;i++) // x
		{
			if( font8x8[charoffset + j ] & (0x01<<(i&7)) )
			{
				if(vertical)
				{
					xpos=(x_pos + j);
					ypos=(y_pos + (8-i));
				}
				else
				{
					xpos=(x_pos + i);
					ypos=(y_pos + j);
				}

				if(xpos>=0 && xpos<td->xsize)
				{
					if(ypos>=0 && ypos<td->ysize)
					{
						if(!color)
						{
							col=(s_col *)&td->framebuffer[(td->xsize*ypos) + xpos];
							col->blue=col->blue/2;
							col->red=col->red/2;
							col->green=col->green/2;
						}
						else
						{
							col=(s_col *)&td->framebuffer[(td->xsize*ypos) + xpos];
							col->blue=(color>>16)&0xFF;
							col->red=color&0xFF;
							col->green=(color>>8)&0xFF;
						}
					}
				}
			}
		}
	}
}

void putstring8x8(HXCFE_TD *td,int x_pos,int y_pos,char * str,unsigned int color,int vertical)
{
	int i;

	i=0;

	while(str[i])
	{

		if(vertical)
		{
			putchar8x8(td,x_pos,y_pos-(i*8),str[i],color,vertical);
		}
		else
		{
			putchar8x8(td,x_pos+(i*8),y_pos,str[i],color,vertical);
		}
		i++;
	}

}

s_sectorlist * display_sectors(HXCFE_TD *td,HXCFE_FLOPPY * floppydisk,int track,int side,double timingoffset_offset, int TRACKTYPE)
{
	int tracksize;
	int i,j,old_i;
	char tempstr[512];
	char tempstr2[32];
	double timingoffset;
	double timingoffset2;
	double timingoffset3;
	double xresstep;
	int xpos_startheader,xpos_endsector,xpos_startdata;
	int xpos_tmp;
	int endfill,loop;
	s_col * col;
	HXCFE_SECTORACCESS* ss;
	HXCFE_SECTCFG* sc;
	HXCFE_SIDE * currentside;
	s_sectorlist * sl,*oldsl;

	xresstep = (double)td->x_us/(double)td->xsize;

	old_i=0;
	timingoffset=0;//timingoffset_offset;
	loop=0;
	endfill=0;

	sl=td->sl;
	oldsl=0;

	currentside=floppydisk->tracks[track]->sides[side];
	tracksize=currentside->tracklen;
	do
	{

		timingoffset = ( getOffsetTiming(currentside,tracksize,0,0) * loop );
		xpos_tmp = (int)( ( timingoffset - timingoffset_offset ) / xresstep );
		if( xpos_tmp < td->xsize )
		{

			old_i=0;

			ss=hxcfe_initSectorAccess(td->hxcfe,floppydisk);
			if(ss)
			{
				endfill=0;
				do
				{
					sc=hxcfe_getNextSector(ss,track,side,TRACKTYPE);
					if(sc)
					{

						oldsl = sl;
						sl=malloc(sizeof(s_sectorlist));
						memset(sl,0,sizeof(s_sectorlist));

						sl->track = track;
						sl->side = side;

						sl->next_element = oldsl;

						timingoffset = getOffsetTiming(currentside,sc->startsectorindex,timingoffset,old_i);
						old_i = sc->startsectorindex;
						xpos_startheader = (int)( ( timingoffset - timingoffset_offset ) / xresstep );

						if(sc->endsectorindex<sc->startsectorindex)
							timingoffset2 = getOffsetTiming(currentside,sc->startsectorindex+ ((tracksize - sc->startsectorindex) + sc->endsectorindex),timingoffset,old_i);
						else
							timingoffset2 = getOffsetTiming(currentside,sc->endsectorindex,timingoffset,old_i);

						if(sc->startdataindex<sc->startsectorindex)
						{
							timingoffset3 = getOffsetTiming(currentside,sc->startsectorindex+ ((tracksize - sc->startsectorindex) + sc->startdataindex),timingoffset,old_i);
						}
						else
							timingoffset3 = getOffsetTiming(currentside,sc->startdataindex,timingoffset,old_i);

						xpos_startdata = (int)( ( timingoffset3 - timingoffset_offset ) / xresstep );

						xpos_endsector = (int)( ( timingoffset2 - timingoffset_offset ) / xresstep );

						sl->x_pos1 = xpos_startheader;
						sl->y_pos1 = 50;
						sl->x_pos2 = xpos_endsector;
						sl->y_pos2 = td->ysize-10;

						sl->sectorconfig=sc;

						// Main Header block
						for(i= xpos_startheader;i<xpos_startdata;i++)
						{
							if(i>=0)
							{
								if( (i<td->xsize) && i>=0)
								{
									if( sc->use_alternate_header_crc )
									{
										// Bad header block -> Red
										for(j=50;j<(td->ysize-10);j++)
										{
											col=(s_col *)&td->framebuffer[(td->xsize*j) + i];
											col->blue=3*col->blue/4;
											col->green=3*col->green/4;
										}
									}
									else
									{
										// Sector ok -> Green
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

						// Main Data block
						for(i = xpos_startdata;i < xpos_endsector;i++)
						{
							if(i>=0)
							{
								if( (i<td->xsize) && i>=0)
								{
									if(sc->startdataindex == sc->startsectorindex)
									{
										// Unknow size (no header) : blue
										for(j=50;j<(td->ysize-10);j++)
										{
											col=(s_col *)&td->framebuffer[(td->xsize*j) + i];
											col->green=3*col->green/4;
											col->red=3*col->red/4;
										}
									}
									else
									{
										if(sc->use_alternate_data_crc)
										{
											// CRC error -> Red
											for(j=50;j<(td->ysize-10);j++)
											{
												col=(s_col *)&td->framebuffer[(td->xsize*j) + i];
												col->blue=3*col->blue/4;
												col->green=3*col->green/4;
											}
										}
										else
										{
											// Sector ok -> Green
											for(j=50;j<(td->ysize-10);j++)
											{
												col=(s_col *)&td->framebuffer[(td->xsize*j) + i];
												if(!sc->fill_byte_used)
												{
													col->blue=3*col->blue/6;
													col->red=3*col->red/6;
												}
												else
												{
													col->blue=3*col->blue/4;
													col->red=3*col->red/4;
												}
											}
										}
									}
								}
								else
								{
									endfill=1;
								}
							}
						}

						// Left Line
						for(j=50;j<(td->ysize-10);j++)
						{
							if( (xpos_startheader < td->xsize) && xpos_startheader>=0)
							{
								if(j&8)
								{
									if(sc->use_alternate_data_crc)
									{
										col=(s_col *)&td->framebuffer[(td->xsize*j) + xpos_startheader];
										col->blue=3*col->blue/8;
										col->green=3*col->green/8;
									}
									else
									{
										col=(s_col *)&td->framebuffer[(td->xsize*j) + xpos_startheader];
										col->blue=3*col->blue/8;
										col->red=3*col->red/8;
									}
								}
							}
						}


						if(sc->startdataindex != sc->startsectorindex)
						{
							// Data Line
							for(j=50;j<(td->ysize-10);j++)
							{
								if( (xpos_startdata<td->xsize) && xpos_startdata >=0)
								{
									if(!(j&7))
									{
										if(sc->use_alternate_data_crc)
										{
											col=(s_col *)&td->framebuffer[(td->xsize*j) + xpos_startdata];
											col->blue=3*col->blue/16;
											col->green=3*col->green/16;
											col->red=3*col->red/4;
										}
										else
										{
											col=(s_col *)&td->framebuffer[(td->xsize*j) + xpos_startdata];
											col->blue=3*col->blue/16;
											col->green=3*col->green/4;
											col->red=3*col->red/16;
										}
									}
								}
							}

							sprintf(tempstr,"---- %.3d Bytes",sc->sectorsize);

							switch(sc->trackencoding)
							{
								case ISOFORMAT_SD:
									if(sc->startdataindex != sc->endsectorindex)
										sprintf(tempstr,"FM   %.3dB DM:%.2Xh",sc->sectorsize,sc->alternate_datamark);
									else
										sprintf(tempstr,"FM   %.3dB DM: ??",sc->sectorsize);
									break;
								case ISOFORMAT_DD:
									if(sc->startdataindex != sc->endsectorindex)
										sprintf(tempstr,"MFM  %.3dB DM:%.2Xh",sc->sectorsize,sc->alternate_datamark);
									else
										sprintf(tempstr,"MFM  %.3dB DM: ??",sc->sectorsize);
									break;
								case AMIGAFORMAT_DD:
									sprintf(tempstr,"AMFM %.3dB ",sc->sectorsize);
									break;
								case TYCOMFORMAT_SD:
									sprintf(tempstr,"TYFM %.3dB ",sc->sectorsize);
								break;
								case MEMBRAINFORMAT_DD:
									sprintf(tempstr,"MEMBRAIN %.3dB DM:%.2Xh",sc->sectorsize,sc->alternate_datamark);
								break;
								case EMUFORMAT_SD:
									sprintf(tempstr,"E-mu %.3dB ",sc->sectorsize);
								break;
								case APPLE2_GCR5A3:
									sprintf(tempstr,"Apple II 5A3 %.3dB ",sc->sectorsize);
								break;
								case APPLE2_GCR6A2:
									sprintf(tempstr,"Apple II 6A2 %.3dB ",sc->sectorsize);
								break;
								case ARBURG_DAT:
									sprintf(tempstr,"Arburg DATA %.3dB ",sc->sectorsize);
								break;
								case ARBURG_SYS:
									sprintf(tempstr,"Arburg SYSTEM %.3dB ",sc->sectorsize);
								break;
							}

							if(sc->fill_byte_used)
								sprintf(tempstr2," F:%.2Xh",sc->fill_byte);
							else
								sprintf(tempstr2,"");

							strcat(tempstr,tempstr2);

							putstring8x8(td,xpos_startheader,225,tempstr,0x000,1);

							if(sc->startdataindex != sc->endsectorindex)
								sprintf(tempstr,"T:%.2d H:%d S:%.3d CRC:%.4X",sc->cylinder,sc->head,sc->sector,sc->data_crc);
							else
								sprintf(tempstr,"T:%.2d H:%d S:%.3d NO DATA?",sc->cylinder,sc->head,sc->sector);
							putstring8x8(td,xpos_startheader+8,225,tempstr,0x000,1);
						}
						else
						{
							sprintf(tempstr,"----");
							switch(sc->trackencoding)
							{
								case ISOFORMAT_SD:
									sprintf(tempstr,"FM   DATA ? DM:%.2Xh",sc->alternate_datamark);
									break;
								case ISOFORMAT_DD:
									sprintf(tempstr,"MFM  DATA ? DM:%.2Xh",sc->alternate_datamark);
									break;
								case AMIGAFORMAT_DD:
									sprintf(tempstr,"AMFM DATA ?");
									break;

								case TYCOMFORMAT_SD:
									sprintf(tempstr,"TYFM DATA ? DM:%.2Xh",sc->alternate_datamark);
								break;

								case MEMBRAINFORMAT_DD:
									sprintf(tempstr,"MEMBRAIN DATA ? DM:%.2Xh",sc->alternate_datamark);
								break;
								case EMUFORMAT_SD:
									sprintf(tempstr,"E-mu Data ?");
								break;
								case APPLE2_GCR5A3:
									sprintf(tempstr,"Apple II 5A3 Data ?");
								break;
								case APPLE2_GCR6A2:
									sprintf(tempstr,"Apple II 6A2 Data ?");
								break;
								case ARBURG_DAT:
									sprintf(tempstr,"Arburg DATA Data ?");
								break;
								case ARBURG_SYS:
									sprintf(tempstr,"Arburg SYSTEM Data ?");
								break;

							}
							putstring8x8(td,xpos_startheader,225,tempstr,0x000,1);
						}

						// Right Line
						for(j=50;j<(td->ysize-10);j++)
						{
							if( ((xpos_endsector-1)<td->xsize) && xpos_endsector>=0 )
							{
								if(!(j&8))
								{
									if(sc->use_alternate_data_crc)
									{
										col=(s_col *)&td->framebuffer[(td->xsize*j) + (xpos_endsector-1)];
										col->blue=3*col->blue/8;
										col->green=3*col->green/8;
									}
									else
									{
										col=(s_col *)&td->framebuffer[(td->xsize*j) + (xpos_endsector-1)];
										col->blue=3*col->blue/8;
										col->red=3*col->red/8;
									}
								}
							}
						}
					}
				}while(sc);

				hxcfe_deinitSectorAccess(ss);

				loop++;
			}
		}
		else
			endfill=1;
	}while(!endfill);

	td->sl=sl;

	return sl;
}

void hxcfe_td_activate_analyzer(HXCFE_TD *td,int TRACKTYPE,int enable)
{
	if(td && TRACKTYPE<32)
	{
		if(enable)
			td->enabledtrackmode = td->enabledtrackmode | (0x00000001 << TRACKTYPE);
		else
			td->enabledtrackmode = td->enabledtrackmode & ( ~(0x00000001 << TRACKTYPE) );
	}
}

void hxcfe_td_draw_track(HXCFE_TD *td,HXCFE_FLOPPY * floppydisk,int track,int side)
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
	double xresstep;
	s_sectorlist * sl,*oldsl;
	s_pulseslist * pl,*oldpl;
	HXCFE_SIDE * currentside;
	s_col * col;

	char tmp_str[32];

	sl=td->sl;
	while(sl)
	{

		oldsl = sl->next_element;
		//sl = sl->next_element;

		if(sl->sectorconfig)
		{
			if(sl->sectorconfig->input_data)
			{
				free(sl->sectorconfig->input_data);
			}
			free(sl->sectorconfig);
		}
		free(sl);

		sl = oldsl;
	}
	td->sl=0;

	pl=td->pl;
	while(pl)
	{

		oldpl = pl->next_element;

		free(pl);

		pl = oldpl;
	}
	td->pl=0;

	if(track>=floppydisk->floppyNumberOfTrack) track = floppydisk->floppyNumberOfTrack - 1;
	if(track<0) track = 0;

	if(!floppydisk->floppyNumberOfTrack || !floppydisk->floppyNumberOfSide)
	{
		memset(td->framebuffer,0xCC,td->xsize*td->ysize*sizeof(unsigned int));
		return;
	}

	currentside=floppydisk->tracks[track]->sides[side];

	tracksize=currentside->tracklen;

	old_i=0;
	i=0;

	memset(td->framebuffer,0,td->xsize*td->ysize*sizeof(unsigned int));

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

	bitrate = currentside->bitrate;

	tracksize=currentside->tracklen;
	timingoffset=0;
	interbit=0;
	i=buffer_offset;
	
	xresstep = 0;
	if(td->xsize)
		xresstep = (double)td->x_us/(double)td->xsize;
	endfill=0;

	if(!xresstep)
		return;


	//////////////////////////////////////////
	// Scatter drawing
	do
	{
		do
		{

			if( currentside->databuffer[i>>3] & (0x80>>(i&7)) )
			{
				if(currentside->bitrate == VARIABLEBITRATE)
					bitrate = currentside->timingbuffer[i>>3];

				xpos= (int)( timingoffset / (xresstep) );
				ypos= td->ysize - (int)( ( (double) ((interbit+1) * 500000) / (double) bitrate ) / (double)((double)td->y_us/(double)td->ysize));

				if(td->x_us>250)
				{
					// Scather gatter display mode
					if( (xpos<td->xsize) && (ypos<td->ysize) && ypos>=0 )
					{
						td->framebuffer[(td->xsize*ypos) + xpos]++;

						ypos--;
						if( (ypos<td->ysize) && ypos>=0 )
						{
							td->framebuffer[(td->xsize*ypos) + xpos]++;
						}
						ypos=ypos+2;
						if( (ypos<td->ysize) && ypos>=0 )
						{
							td->framebuffer[(td->xsize*ypos) + xpos]++;
						}
					}
				}
				else
				{
					// Pulses display mode
					if( (xpos<td->xsize) )
					{

						for(ypos= td->ysize - 40 ; ypos > (td->ysize - 250) ; ypos--)
						{
							td->framebuffer[(td->xsize*ypos) + xpos]=255;
						}
					}
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
				bitrate = currentside->timingbuffer[i>>3];

			timingoffset=timingoffset + ((double)(500000)/(double)bitrate);

			i++;

		}while(i<tracksize && !endfill);

		xpos= (int)( timingoffset / xresstep );
		if(xpos>td->xsize)
			endfill=1;

		i=0;

	}while(!endfill);

	//////////////////////////////////////////
	// Color inverting
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
			col->green=255 - (col->red );
			col->blue=255 - (col->red );
			col->red=255 - (col->red );

			col->green=col->green/2;
			col->blue=col->blue/2;
			col->red=col->red/2;
		}
	}

	//////////////////////////////////////////
	// Flakey bits drawing
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
				xpos= (int) ( timingoffset / xresstep );
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

			}while(i<tracksize && !endfill);

			i=0;
			old_i=0;
		}while(!endfill);

	}

	//////////////////////////////////////////
	// Index drawing
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
			xpos= (int)( timingoffset / xresstep );

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
		}while(i<tracksize && !endfill);

		old_i=0;
		i=0;

	}while(!endfill);

	//////////////////////////////////////////
	// cell drawing

	if(!(td->x_us>250))
	{
		// Only possible in pulses display mode
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
				timingoffset2=getOffsetTiming(currentside,i+1,timingoffset,i);

				old_i=i;
				xpos = (int)( timingoffset / xresstep ) - (int)(( timingoffset2 - timingoffset) / (xresstep *2) );

				if(xpos>=0 && (xpos<td->xsize) )
				{
					for(ypos= td->ysize - 40 ; ypos > (td->ysize - 50) ; ypos--)
					{
						col=(s_col *)&td->framebuffer[(td->xsize*ypos) + xpos];
						col->blue=0;
						col->green=155;
						col->red=0;
					}
					// Add the pulse to the pulses list.
					oldpl = pl;
					pl = malloc(sizeof(s_pulseslist));
					if(pl)
					{
						memset(pl,0,sizeof(s_pulseslist));
						pl->track = track;
						pl->side = side;
						pl->pulse_number = i;
						pl->x_pos1 = xpos;
						pl->x_pos2 = xpos + (int)(( timingoffset2 - timingoffset) / (xresstep) );
						pl->next_element = oldpl;
					}
				}
				else
				{
					if(xpos >= 0 )
						endfill = 1;
				};
				i++;
			}while(i<tracksize && !endfill);

			old_i=0;
			i=0;

		}while(!endfill);

		td->pl = pl;

		ypos = td->ysize - 40;
		for(xpos = 0;xpos < td->xsize ; xpos++)
		{
			col=(s_col *)&td->framebuffer[(td->xsize*ypos) + xpos];
			col->blue=0;
			col->green=155;
			col->red=0;
		}
	}

	//////////////////////////////////////////
	// Sector drawing
	for(i=0;i<32;i++)
	{
		if(td->enabledtrackmode & (0x00000001<<i) )
		{
			display_sectors(td,floppydisk,track,side,timingoffset_offset,i);
		}
	}

	sprintf(tmp_str,"T:%.3d S:%.1d",track,side);
	putstring8x8(td,1,1,tmp_str,0x000,0);

}

s_sectorlist * hxcfe_td_getlastsectorlist(HXCFE_TD *td)
{
	return td->sl;
}

s_pulseslist * hxcfe_td_getlastpulselist(HXCFE_TD *td)
{
	return td->pl;
}

void plot(HXCFE_TD *td,int x,int y,unsigned long color,int op)
{
	unsigned char color_r,color_v,color_b;
	unsigned long rdcolor;

	if(x>=0 && x<td->xsize)
	{
		if(y>=0 && y<td->ysize)
		{
			switch(op)
			{
				case 0:
				default:
					td->framebuffer[(td->xsize*y)+x]=color;
				break;
				case 1:
					rdcolor = td->framebuffer[(td->xsize*y)+x];

					color_r = (unsigned char)(rdcolor & 0xFF);
					color_v = (unsigned char)((rdcolor>>8) & 0xFF);
					color_b = (unsigned char)((rdcolor>>16) & 0xFF);

					color_r = (unsigned char)(color_r * (float)((float)(color&0xFF)/(float)255));
					color_v = (unsigned char)(color_v * (float)((float)((color>>8)&0xFF)/(float)255));
					color_b = (unsigned char)(color_b * (float)((float)((color>>16)&0xFF)/(float)255));

					td->framebuffer[(td->xsize*y)+x] = (color_b<<16) | (color_v<<8) | color_r;

				break;
			}
		}
	}
}

void circle(HXCFE_TD *td,int x_centre,int y_centre,int r,unsigned int color)
{
	int x;
	int y;
	int d;
	int offset;

	x=0;
	y=r;
	d=r-1;

//    8  1
//  7     2
//  6     3
//    5 4

	while(y>=x)
	{
		offset = (int)((2048 / 8) * (float)((float)x / (float)y));

		plot(td, x+x_centre, -y+y_centre  , color,0);   // 1 -
		plot(td, y+x_centre, -x+y_centre  , color,0);   // 2 +
		plot(td, x_centre + y, x+y_centre , color,0);   // 3 -
		plot(td, x_centre + x, y+y_centre , color,0);   // 4 +
		plot(td, -x+x_centre, y+y_centre  , color,0);   // 5 -
		plot(td, -y+x_centre, x+y_centre  , color,0);   // 6 +
		plot(td, -y+x_centre, -x+y_centre  , color,0);  // 7 -
		plot(td, -x+x_centre, -y+y_centre  , color,0);  // 8 +


		if (d >= 2*x)
		{
			d = d - ( 2 * x ) -1;
			x = x+1;

		}
		else
		{
			if(d <= 2*(r-y))
			{
				d = d+2*y-1;
				y = y-1;
			}
			else
			{
				d = d+2*(y-x-1);
				y = y-1;
				x = x+1;
			}
		}
	}
}

void draw_circle (HXCFE_TD *td,unsigned long col,float start_angle,float stop_angle,int xpos,int ypos,int diametre,int op,int thickness)
{
    int x, y,i;
    int length;
    float angle = 0.0;
    float angle_stepsize = (float)0.001;

	length = diametre;
	
	if(op!=1) thickness++;

	length += thickness;

	i = 0;
	do
	{
		angle = start_angle;
		// go through all angles from 0 to 2 * PI radians
		do //2 * 3.14159)
		{
			// calculate x, y from a vector with known length and angle
			x = (int)((length) * cos (angle));
			y = (int)((length) * sin (angle));

			plot(td, x+xpos, -y+ypos  , col, op);

			angle += angle_stepsize;
		}while (angle < stop_angle );

		length--;
		i++;
	}while(i<(thickness));
}

s_sectorlist * display_sectors_disk(HXCFE_TD *td,HXCFE_FLOPPY * floppydisk,int track,int side,double timingoffset_offset, int TRACKTYPE,int xpos,int ypos,int diam,int thickness)
{
	int tracksize;
	int old_i;
	float track_timing;
	float startsector_timingoffset;
	float datasector_timingoffset;
	float endsector_timingoffset;
	float timingoffset;
	int color;
	int xpos_tmp;
	int endfill,loop;
	HXCFE_SECTORACCESS* ss;
	HXCFE_SECTCFG* sc;
	HXCFE_SIDE * currentside;
	s_sectorlist * sl,*oldsl;

	old_i=0;
	loop=0;
	endfill=0;

	sl=td->sl;
	oldsl=0;

	currentside=floppydisk->tracks[track]->sides[side];
	tracksize=currentside->tracklen;

	startsector_timingoffset = 0;
	xpos_tmp = 0;

	old_i=0;
	timingoffset = 0;

	track_timing = (float)getOffsetTiming(currentside,tracksize,timingoffset,old_i);

	ss=hxcfe_initSectorAccess(td->hxcfe,floppydisk);
	if(ss)
	{
		do
		{
			sc=hxcfe_getNextSector(ss,track,side,TRACKTYPE);
			if(sc)
			{
				oldsl = sl;
				sl=malloc(sizeof(s_sectorlist));
				if(sl)
				{
					memset(sl,0,sizeof(s_sectorlist));

					sl->track = track;
					sl->side = side;

					sl->next_element = oldsl;

					sl->sectorconfig = sc;

					startsector_timingoffset = (float)getOffsetTiming(currentside,sc->startsectorindex,timingoffset,old_i);

					if(sc->endsectorindex<sc->startsectorindex)
					{
						endsector_timingoffset = (float)getOffsetTiming(currentside,sc->startsectorindex+ ((tracksize - sc->startsectorindex) + sc->endsectorindex),timingoffset,old_i);
					}
					else
					{
						endsector_timingoffset = (float)getOffsetTiming(currentside,sc->endsectorindex,timingoffset,old_i);
					}

					if(sc->startdataindex<sc->startsectorindex)
					{
						datasector_timingoffset = (float)getOffsetTiming(currentside,sc->startsectorindex+ ((tracksize - sc->startsectorindex) + sc->startdataindex),timingoffset,old_i);
					}
					else
					{
						datasector_timingoffset = (float)getOffsetTiming(currentside,sc->startdataindex,timingoffset,old_i);
					}

					///////////////////////////////////////
					// Header Block
					///////////////////////////////////////
					if( sc->use_alternate_header_crc )
					{
 						color = 0xFF0000;
					}
					else
					{
						color = 0x99FF99;
					}

					draw_circle (td,color,(float)((float)2 * PI)*(startsector_timingoffset/track_timing),(float)((float)2 * PI)*(datasector_timingoffset/track_timing),xpos,ypos,diam,0,thickness);

					///////////////////////////////////////
					// Data Block
					///////////////////////////////////////
					if(sc->startdataindex == sc->startsectorindex)
					{
						// CRC error -> Blue
						color = 0xFF0000;
					}
					else
					{
						if(sc->use_alternate_data_crc)
						{
							// CRC error -> Red
							color = 0x008FFA;
						}
						else
						{
							if(!sc->fill_byte_used)
							{
								// Sector ok -> Green
								color = 0x00FF00;
							}
							else
							{
								color = 0x66EE00;
							}
						}
					}

					draw_circle (td,color,(float)((float)2 * PI)*(datasector_timingoffset/track_timing),(float)((float)2 * PI)*(endsector_timingoffset/track_timing),xpos,ypos,diam,0,thickness);

					// Left Line
					draw_circle (td,0xFF6666,(float)((float)2 * PI)*(startsector_timingoffset/track_timing),(float)((float)2 * PI)*(startsector_timingoffset/track_timing),xpos,ypos,diam,0,thickness);

					// Data Line
					draw_circle (td,0x7F6666,(float)((float)2 * PI)*(datasector_timingoffset/track_timing),(float)((float)2 * PI)*(datasector_timingoffset/track_timing),xpos,ypos,diam,0,thickness);

					// Right Line
					draw_circle (td,0xFF6666,(float)((float)2 * PI)*(endsector_timingoffset/track_timing),(float)((float)2 * PI)*(endsector_timingoffset/track_timing),xpos,ypos,diam,0,thickness);

					sl->start_angle = (float)((float)2 * PI)*(startsector_timingoffset/track_timing);
					sl->end_angle = (float)((float)2 * PI)*(endsector_timingoffset/track_timing);

					sl->diameter = diam;
					sl->thickness = thickness;
				}
			}
		}while(sc);

		hxcfe_deinitSectorAccess(ss);
		loop++;
	}


	td->sl=sl;

	return sl;
}


void hxcfe_td_draw_disk(HXCFE_TD *td,HXCFE_FLOPPY * floppydisk)
{
	int tracksize;
	int i,ysize,xsize;
	int track,side;
	HXCFE_SIDE * currentside;
	unsigned int color;
	int y_pos,x_pos_1,x_pos_2;
	float track_ep;
	int bitrate;
	int xpos;
	s_sectorlist * sl,*oldsl;

	sl=td->sl;
	while(sl)
	{

		oldsl = sl->next_element;

		if(sl->sectorconfig)
		{
			if(sl->sectorconfig->input_data)
			{
				free(sl->sectorconfig->input_data);
			}
			free(sl->sectorconfig);
		}
		free(sl);

		sl = oldsl;
	}
	td->sl=0;

	memset(td->framebuffer,0,td->xsize*td->ysize*sizeof(unsigned long));

	ysize=1;
	xsize=2048;

	y_pos = td->ysize/2;
	x_pos_1 = td->xsize/4;
	x_pos_2 = td->xsize - (td->xsize/4);

	color = 0x131313;
	for(i=25;i<(td->ysize-(y_pos));i++)
	{
		circle(td,x_pos_1,y_pos,i,color);
		circle(td,x_pos_2,y_pos,i,color);
	}

	y_pos = td->ysize/2;
	x_pos_1 = td->xsize/4;
	x_pos_2 = td->xsize - (td->xsize/4);

	for(side=0;side<floppydisk->floppyNumberOfSide;side++)
	{
		for(track=0;track<floppydisk->floppyNumberOfTrack;track++)
		{
			td->hxc_setprogress(track + (side*floppydisk->floppyNumberOfTrack),floppydisk->floppyNumberOfTrack*floppydisk->floppyNumberOfSide,td,td->progress_userdata);

			currentside=floppydisk->tracks[track]->sides[side];

			tracksize=currentside->tracklen;

			bitrate = currentside->bitrate;

			track_ep=(float)( (td->ysize-(y_pos)) - 60 ) /((float) floppydisk->floppyNumberOfTrack+1);

			//////////////////////////////////////////
			// Sector drawing
			for(i=0;i<32;i++)
			{
				if(td->enabledtrackmode & (0x00000001<<i) )
				{
					if(!side)
					{
						display_sectors_disk(td,floppydisk,track,side,0,i,x_pos_1,y_pos,60 + (int)((floppydisk->floppyNumberOfTrack-track) * track_ep),(int)track_ep);
					}
					else
					{
						display_sectors_disk(td,floppydisk,track,side,0,i,x_pos_2,y_pos,60 + (int)((floppydisk->floppyNumberOfTrack-track) * track_ep),(int)track_ep);
					}
				}
			}

			if(currentside->flakybitsbuffer)
			{
				i=0;
				do
				{

					if( currentside->flakybitsbuffer[i>>3] & (0x80>>(i&7)) )
					{
						xpos= (int)((float)2048 * ((float)i/(float)tracksize));
						if( (xpos<2048))
						{
							if(!side)
								draw_circle (td,0x0000FF,(float)((float)((float)2 * PI)*((float)xpos/(float)2048)),(float)((float)((float)2 * PI)*((float)xpos/(float)2048)),x_pos_1,y_pos,60 + (int)(((floppydisk->floppyNumberOfTrack-track) * track_ep)),0,(int)track_ep);
							else
								draw_circle (td,0x0000FF,(float)((float)((float)2 * PI)*((float)xpos/(float)2048)),(float)((float)((float)2 * PI)*((float)xpos/(float)2048)),x_pos_2,y_pos,60 + (int)(((floppydisk->floppyNumberOfTrack-track) * track_ep)),0,(int)track_ep);
						}
					}

					i++;
				}while(i<tracksize);
			}

			putstring8x8(td,x_pos_1 - 24,y_pos + 32,"Side 0",0x666666,0);
			putstring8x8(td,x_pos_2 - 24,y_pos + 32,"Side 1",0x666666,0);

			putstring8x8(td,x_pos_1 - 4,y_pos - 44,"->",0x666666,0);
			putstring8x8(td,x_pos_2 - 4,y_pos - 44,"->",0x666666,0);

			putstring8x8(td,x_pos_1 + 40,y_pos - 3,"---",0xCCCCCC,0);
			putstring8x8(td,x_pos_2 + 40,y_pos - 3,"---",0xCCCCCC,0);
		}
	}

	for(side=0;side<floppydisk->floppyNumberOfSide;side++)
	{
		for(track=0;track<floppydisk->floppyNumberOfTrack;track++)
		{
			currentside=floppydisk->tracks[track]->sides[side];

			tracksize=currentside->tracklen;

			track_ep=(float)( (td->ysize-(y_pos)) - 60 ) /(float) floppydisk->floppyNumberOfTrack;

			draw_circle (td,0xEFEFEF,0,(float)((float)((float)2 * PI)),x_pos_1,y_pos,60 + (int)(((floppydisk->floppyNumberOfTrack-track) * track_ep)),1,(int)0);
			draw_circle (td,0xEFEFEF,0,(float)((float)((float)2 * PI)),x_pos_2,y_pos,60 + (int)(((floppydisk->floppyNumberOfTrack-track) * track_ep)),1,(int)0);
		}
	}

	td->hxc_setprogress(floppydisk->floppyNumberOfTrack*floppydisk->floppyNumberOfSide,floppydisk->floppyNumberOfTrack*floppydisk->floppyNumberOfSide,td,td->progress_userdata);
}

void * hxcfe_td_getframebuffer(HXCFE_TD *td)
{
	if(td)
	{
		return (void*)td->framebuffer;
	}

	return 0;
}

int hxcfe_td_getframebuffer_xres(HXCFE_TD *td)
{
	if(td)
	{
		return td->xsize;
	}

	return 0;

}

int hxcfe_td_getframebuffer_yres(HXCFE_TD *td)
{
	if(td)
	{
		return td->ysize;
	}

	return 0;
}


void hxcfe_td_deinit(HXCFE_TD *td)
{
	free(td->framebuffer);
	free(td);
}
