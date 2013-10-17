/*
//
// Copyright (C) 2006 - 2013 Jean-François DEL NERO
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

#include "font.h"

s_trackdisplay * hxcfe_td_init(HXCFLOPPYEMULATOR* floppycontext,unsigned long xsize,unsigned long ysize)
{
	s_trackdisplay * td;
	
	td=malloc(sizeof(s_trackdisplay));
	memset(td,0,sizeof(s_trackdisplay));
	
	td->xsize=xsize;
	td->ysize=ysize;
	
	td->framebuffer=malloc(td->xsize*td->ysize*sizeof(unsigned int));
	memset(td->framebuffer,0,td->xsize*td->ysize*sizeof(unsigned int));

	td->x_us=200*1000;
	td->y_us=64;

	td->x_start_us=0;

	return td;
}

void hxcfe_td_setparams(HXCFLOPPYEMULATOR* floppycontext,s_trackdisplay *td,unsigned long x_us,unsigned long y_us,unsigned long x_start_us)
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
	while(element->next_element);//element->)
	{
		element=element->next_element;
	}

	return element;
}

struct s_sectorlist_ * addelement(struct s_sectorlist_ * element,void *object)
{
	while(element->next_element);//element->)
	{
		element=element->next_element;
	}

	element->next_element=malloc(sizeof(struct s_sectorlist_));
	memset(element->next_element,0,sizeof(struct s_sectorlist_));

	return element->next_element;
}

void freelist(struct s_sectorlist_ * element)
{
	struct s_sectorlist_ * nextelement;

	nextelement=element->next_element;
	free(element);
	if(nextelement)
		freelist(nextelement);
	
	return;
}

double getOffsetTiming(SIDE *currentside,int offset,double timingoffset,int start)
{
	int i,j,bitrate,totaloffset;


	if(offset >= start)
	{
		totaloffset = offset;
	}
	else
	{
		totaloffset = start + ((currentside->tracklen - start) + offset);
	}

	i=start;
	j=start%currentside->tracklen;

	do
	{
		if(currentside->bitrate==VARIABLEBITRATE)
			bitrate=currentside->timingbuffer[j>>3];
		else
			bitrate=currentside->bitrate;

		timingoffset = timingoffset + ((double)(500000)/(double)bitrate);
		i++;
		j=(j+1)%currentside->tracklen;
	}while(i<totaloffset);

	return timingoffset;
}

void putchar8x8(s_trackdisplay *td,int x_pos,int y_pos,unsigned char c,unsigned int color,int vertical)
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
						col=(s_col *)&td->framebuffer[(td->xsize*ypos) + xpos];
						col->blue=col->blue/2;
						col->red=col->red/2;
						col->green=col->green/2;
					}
				}
			}
		}
	}
}

void putstring8x8(s_trackdisplay *td,int x_pos,int y_pos,unsigned char * str,unsigned int color,int vertical)
{
	int i;

	i=0;

	while(str[i])
	{

		if(vertical)
		{
			putchar8x8(td,x_pos,y_pos-(i*8),str[i],0x00000000,vertical);
		}
		else
		{
			putchar8x8(td,x_pos+(i*8),y_pos,str[i],0x00000000,vertical);
		}
		i++;
	}

}

s_sectorlist * display_sectors(HXCFLOPPYEMULATOR* floppycontext,s_trackdisplay *td,FLOPPY * floppydisk,int track,int side,double timingoffset_offset, int TRACKTYPE)
{
	int tracksize;
	int i,j,old_i;
	char tempstr[512];	
	double timingoffset;
	double timingoffset2;
	double timingoffset3;
	int xpos_startheader,xpos_endsector,xpos_startdata;
	int xpos_tmp;
	int endfill,loop;
	s_col * col;
	SECTORSEARCH* ss;
	SECTORCONFIG* sc;
	SIDE * currentside;
	s_sectorlist * sl,*oldsl;

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
		xpos_tmp = (int)( ( timingoffset - timingoffset_offset ) / ((double)td->x_us/(double)td->xsize) );
		if( xpos_tmp < td->xsize )
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

						oldsl = sl;
						sl=malloc(sizeof(s_sectorlist));
						memset(sl,0,sizeof(s_sectorlist));

						sl->next_element = oldsl;

						timingoffset = getOffsetTiming(currentside,sc->startsectorindex,timingoffset,old_i);
						old_i = sc->startsectorindex;
						xpos_startheader = (int)( ( timingoffset - timingoffset_offset ) / ((double)td->x_us/(double)td->xsize) );
						
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

						xpos_startdata = (int)( ( timingoffset3 - timingoffset_offset ) / ((double)td->x_us/(double)td->xsize) );

						xpos_endsector = (int)( ( timingoffset2 - timingoffset_offset ) / ((double)td->x_us/(double)td->xsize) );

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
												col->blue=3*col->blue/4;
												col->red=3*col->red/4;
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
								case ARBURG_SD:
									sprintf(tempstr,"Arburg SD %.3dB ",sc->sectorsize);
								break;
								case ARBURG_SYS:
									sprintf(tempstr,"Arburg SYS %.3dB ",sc->sectorsize);
								break;


							}

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
								case ARBURG_SD:
									sprintf(tempstr,"Arburg SD Data ?");
								break;
								case ARBURG_SYS:
									sprintf(tempstr,"Arburg SYS Data ?");
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

				hxcfe_deinitSectorSearch(ss);

				loop++;
			}
		}
		else
			endfill=1;
	}while(!endfill);

	td->sl=sl;
	
	return sl;
}

void hxcfe_td_activate_analyzer(HXCFLOPPYEMULATOR* floppycontext,s_trackdisplay *td,int TRACKTYPE,int enable)
{
	if(td && TRACKTYPE<32)
	{
		if(enable)
			td->enabledtrackmode = td->enabledtrackmode | (0x00000001 << TRACKTYPE);
		else
			td->enabledtrackmode = td->enabledtrackmode & ( ~(0x00000001 << TRACKTYPE) );
	}
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
	s_sectorlist * sl,*oldsl;
	SIDE * currentside;
	s_col * col;

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
				ypos= td->ysize - (int)( ( (double) ((interbit+1) * 500000) / (double) bitrate ) / (double)((double)td->y_us/(double)td->ysize));

				if( (xpos<td->xsize) && (ypos<td->ysize) && ypos>=0 )
				{
					if(td->x_us>1000)
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
					else
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
			col->green=255 - (col->red );
			col->blue=255 - (col->red );
			col->red=255 - (col->red );
			
			col->green=col->green/2;
			col->blue=col->blue/2;
			col->red=col->red/2;
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

	for(i=0;i<32;i++)
	{
		if(td->enabledtrackmode & (0x00000001<<i) )
		{
			display_sectors(floppycontext,td,floppydisk,track,side,timingoffset_offset,i);
		}
	}
}

s_sectorlist * hxcfe_td_getlastsectorlist(HXCFLOPPYEMULATOR* floppycontext,s_trackdisplay *td)
{
	return td->sl;
}

void plot(s_trackdisplay *td,int x,int y,unsigned int color)
{
	if(x>=0 && x<td->xsize)
	{
		if(y>=0 && y<td->ysize)
		{
			td->framebuffer[(td->xsize*y)+x]=color;
		}
	}
}

void circle(s_trackdisplay *td,int x_centre,int y_centre,int r,unsigned int color)
{
	int x;
	int y;
	int d;

	x=0;
	y=r;
	d=r-1;

	while(y>=x)
	{

		plot(td, x+x_centre, y+y_centre , color);
		plot(td, y+x_centre, x+y_centre , color);
		plot(td, -x+x_centre, y+y_centre  , color);
		plot(td, -y+x_centre, x+y_centre  , color);
		plot(td, x+x_centre, -y+y_centre  , color);
		plot(td, y+x_centre, -x+y_centre  , color);
		plot(td, -x+x_centre, -y+y_centre  , color);
		plot(td, -y+x_centre, -x+y_centre  , color);

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



void hxcfe_td_draw_disk(HXCFLOPPYEMULATOR* floppycontext,s_trackdisplay *td,FLOPPY * floppydisk)
{
	int tracksize;
	int i,old_i;
	int track,side;
	SIDE * currentside;
	unsigned int color;
	int y_pos,x_pos_1,x_pos_2;
	int numberoftrack;
	float track_ep,t;

	track=0;
	side=0;
	currentside=floppydisk->tracks[track]->sides[side];

	tracksize=currentside->tracklen;
	
	old_i=0;
	i=0;

	numberoftrack=80;
	y_pos = td->ysize/2;
	x_pos_1 = td->xsize/4;
	x_pos_2 = td->xsize - (td->xsize/4);

	track_ep=(float)( (td->ysize-(y_pos)) - 10 ) /(float) numberoftrack;
	t=0;

	for(i=25;i<(td->ysize-(y_pos));i++)
	{
		color=0xFF1133+(((int)t)<<2);
		if(i&2) color=0xFFFF00;
		else color=0x00FFFF;
		t=t+track_ep;
		circle(td,x_pos_1,y_pos,i,color);
		circle(td,x_pos_2,y_pos,i,color);
	}
}


void hxcfe_td_deinit(HXCFLOPPYEMULATOR* floppycontext,s_trackdisplay *td)
{
	free(td->framebuffer);
	free(td);
}
