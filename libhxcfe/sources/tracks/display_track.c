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
// File : display_track.c
// Contains:
//
// Written by: Jean-François DEL NERO
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "types.h"

#include "internal_libhxcfe.h"
#include "tracks/track_generator.h"
#include "libhxcfe.h"

#include "tracks/luts.h"

#include "version.h"

#include "crc.h"
#include "std_crc32.h"

#include "floppy_utils.h"

#include "display_track.h"

#include "font.h"

#include "stream_analyzer/fluxStreamAnalyzer.h"

#include "loaders/bmp_loader/bmp_loader.h"
#include "loaders/bmp_loader/bmp_file.h"

#include "xml_disk/packer/pack.h"
#include "misc/data/data_bmp_hxc2001_logo_bmp.h"

#define PI    ((float)  3.141592654f)

#define LAYER_SUPPORT 0
#define LAYER_IDENTIFICATION 1
#define LAYER_TEXT 2

#define LOGOALPHA 150

int dummy_graph_progress(unsigned int current,unsigned int total,void * td,void * user)
{
	return 0;
}

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

#pragma pack(1)
typedef struct s_col_
{
	uint8_t red;
	uint8_t green;
	uint8_t blue;
	uint8_t alpha;
}s_col;
#pragma pack()

#define COLOR_RGBA(R,G,B,A) ( (R & 0xFF) | ( ( G & 0xFF ) << 8 ) | ( ( B & 0xFF ) << 16 ) | ( ( A & 0xFF ) << 24 ) )
#define COLOR_RGB2RGBA(RGB,A) ( RGB | ( ( A & 0xFF ) << 24 ) )

static uint32_t alpha(uint32_t dst, uint32_t src, uint8_t alpha_value)
{
	return  ( ( ( (uint32_t)(alpha_value) * ((src&0x0000FF)>>0) + (uint32_t)(255-alpha_value) * ((dst&0x0000FF)>>0) ) >> 8 ) & 0xFF ) | \
			( ( (uint32_t)(alpha_value) * ((src&0x00FF00)>>8) + (uint32_t)(255-alpha_value) * ((dst&0x00FF00)>>8) ) & 0xFF00 ) | \
			( ( ( (uint32_t)(alpha_value) * ((src&0xFF0000)>>16) + (uint32_t)(255-alpha_value) * ((dst&0xFF0000)>>16) ) << 8 ) & 0xFF0000);
}

static uint32_t alpha2(uint32_t dst, uint32_t src)
{
	uint8_t alpha_value;

	alpha_value = src >> 24;

	return  ( ( ( (uint32_t)(alpha_value) * ((src&0x0000FF)>>0) + (uint32_t)(255-alpha_value) * ((dst&0x0000FF)>>0) ) >> 8 ) & 0xFF ) | \
			( ( (uint32_t)(alpha_value) * ((src&0x00FF00)>>8) + (uint32_t)(255-alpha_value) * ((dst&0x00FF00)>>8) ) & 0xFF00 ) | \
			( ( ( (uint32_t)(alpha_value) * ((src&0xFF0000)>>16) + (uint32_t)(255-alpha_value) * ((dst&0xFF0000)>>16) ) << 8 ) & 0xFF0000);
}

void putchar8x8(HXCFE_TD *td,int layer,int x_pos,int y_pos,unsigned char c,uint32_t color,uint32_t bgcolor,int vertical,int transparent)
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
					col=(s_col *)&td->layers[layer][(td->xsize*ypos) + xpos];
					col->alpha = 255;

					if( ( font8x8[charoffset + j ] & (0x01<<(i&7)) ) )
					{
						if(!color)
						{
							col->blue=(unsigned char)(col->blue/2);
							col->red=(unsigned char)(col->red/2);
							col->green=(unsigned char)(col->green/2);
							col->alpha = 0xFF;
						}
						else
						{
							col->blue=(unsigned char)((color>>16)&0xFF);
							col->red=(unsigned char)(color&0xFF);
							col->green=(unsigned char)((color>>8)&0xFF);
							col->alpha = 0xFF;
						}
					}
					else
					{
						if(!transparent)
						{
							col->blue=(unsigned char)((bgcolor>>16)&0xFF);
							col->red=(unsigned char)(bgcolor&0xFF);
							col->green=(unsigned char)((bgcolor>>8)&0xFF);
						}
						else
						{
							col->alpha = 0x00;
						}
					}
				}
			}
		}
	}
}

void putstring8x8(HXCFE_TD *td,int layer, int x_pos,int y_pos,char * str,uint32_t color,uint32_t bgcolor,int vertical,int transparent)
{
	int i;

	i=0;

	if( td && str )
	{
		while(str[i])
		{
			if(vertical)
			{
				putchar8x8(td,layer,x_pos,y_pos-(i*8),str[i],color,bgcolor,vertical,transparent);
			}
			else
			{
				putchar8x8(td,layer,x_pos+(i*8),y_pos,str[i],color,bgcolor,vertical,transparent);
			}
			i++;
		}
	}
}

void splash_sprite(bmaptype * bmp,uint32_t * dest_buffer, int xsize, int ysize, int xpos, int ypos, uint8_t alpha)
{
	int i,j;
	int offset;
	int src_offset;
	uint32_t pixel;
	uint8_t  src_r,src_g,src_b;

	for(j=0;j<bmp->Ysize;j++)
	{
		for(i=0;i<bmp->Xsize;i++)
		{
			if( ( (xpos + i) >=0 && (xpos + i) < xsize) &&
				( (ypos + j) >=0 && (ypos + j) < ysize) )
			{
				offset = (((ypos + j) * xsize) + (xpos + i));
				src_offset = (3*256) + (((j * bmp->Xsize) + i));

				src_r = bmp->unpacked_data[ (bmp->unpacked_data[src_offset] * 3) + 2 ];
				src_g = bmp->unpacked_data[ (bmp->unpacked_data[src_offset] * 3) + 1 ];
				src_b = bmp->unpacked_data[ (bmp->unpacked_data[src_offset] * 3) + 0 ];

				if( !(src_r==255 && src_g==0 && src_b==255) )
				{
					pixel =  (((uint32_t)src_r)<<0) |
							 (((uint32_t)src_g)<<8) |
							 (((uint32_t)src_b)<<16) |
							 (((uint32_t)alpha)<<24);

					dest_buffer[offset] = pixel;
				}
			}
		}
	}
}

void clear_layers( HXCFE_TD *td )
{
	int i;

	memset(td->framebuffer,0,td->xsize*td->ysize*sizeof(uint32_t));

	for(i=0;i<16;i++)
	{
		if( td->layers[i] )
		{
			memset(td->layers[i],0,td->xsize*td->ysize*sizeof(uint32_t));
		}
	}
}

void plot(HXCFE_TD *td, int layer, int x, int y, uint32_t color, uint8_t alpha_value, int op)
{
	unsigned char color_r,color_v,color_b;
	uint32_t rdcolor;

	if( ( x >= td->window_xpos ) && ( x < ( td->window_xpos + td->xsize ) ) )
	{
		if( ( y >= td->window_ypos ) && ( y < ( td->window_ypos + td->ysize ) ) )
		{
			x -= td->window_xpos;
			y -= td->window_ypos;

			switch(op)
			{
				case 0:
				default:
					td->layers[layer][(td->xsize*y)+x] = (color & 0xFFFFFF) | (alpha_value << 24);
				break;
				case 1:
					rdcolor = td->layers[layer][(td->xsize*y)+x];

					color_r = (unsigned char)(rdcolor & 0xFF);
					color_v = (unsigned char)((rdcolor>>8) & 0xFF);
					color_b = (unsigned char)((rdcolor>>16) & 0xFF);

					color_r = (unsigned char)(color_r * (float)((float)(color&0xFF)/(float)255));
					color_v = (unsigned char)(color_v * (float)((float)((color>>8)&0xFF)/(float)255));
					color_b = (unsigned char)(color_b * (float)((float)((color>>16)&0xFF)/(float)255));

					td->layers[layer][(td->xsize*y)+x] = ( ( alpha_value & 0xFF ) << 24 ) | (color_b<<16) | (color_v<<8) | color_r;

				break;
				case 2:
					td->layers[layer][(td->xsize*y)+x] = alpha(td->layers[layer][(td->xsize*y)+x], color, alpha_value);
				break;
			}
		}
	}
}

void circle(HXCFE_TD *td,int layer,int x_centre,int y_centre,int r,unsigned int color, uint8_t alpha)
{
	int x;
	int y;
	int d;

	x=0;
	y=r;
	d=r-1;

//    8  1
//  7     2
//  6     3
//    5 4

	while(y>=x)
	{
		plot(td, layer, x+x_centre, -y+y_centre  , color,alpha,0);   // 1 -
		plot(td, layer, y+x_centre, -x+y_centre  , color,alpha,0);   // 2 +
		plot(td, layer, x_centre + y, x+y_centre , color,alpha,0);   // 3 -
		plot(td, layer, x_centre + x, y+y_centre , color,alpha,0);   // 4 +
		plot(td, layer, -x+x_centre, y+y_centre  , color,alpha,0);   // 5 -
		plot(td, layer, -y+x_centre, x+y_centre  , color,alpha,0);   // 6 +
		plot(td, layer, -y+x_centre, -x+y_centre  , color,alpha,0);  // 7 -
		plot(td, layer, -x+x_centre, -y+y_centre  , color,alpha,0);  // 8 +

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

void disc(HXCFE_TD *td,int layer, int x_centre,int y_centre,int r,unsigned int color,unsigned int bcolor,uint8_t alpha)
{
	int i;

	for(i=0;i<r;i++)
	{
		circle(td,layer,x_centre,y_centre,i,color,alpha);
	}

	circle(td,layer,x_centre,y_centre,r,bcolor,alpha);
}

void box(HXCFE_TD *td,int layer, int x1,int y1,int x2,int y2, uint32_t color, int alpha_val, int op )
{
	int t,i;

	if(x1 > x2)
	{
		t = x1;
		x1 = x2;
		x2 = t;
	}

	if(y1 > y2)
	{
		t = y1;
		y1 = y2;
		y2 = t;
	}

	for(i = x1; i < x2 ;i++)
	{
		plot(td, layer, i, y1, color, alpha_val, op);
		plot(td, layer, i, y2, color, alpha_val, op);
	}

	for(i = y1; i < y2 ;i++)
	{
		plot(td, layer, x1, i, color, alpha_val, op);
		plot(td, layer, x2, i, color, alpha_val, op);
	}
}

void render(HXCFE_TD *td)
{
	int i,size;
	uint32_t *src;
	uint32_t *dst, *end_dest;
	register uint32_t reg;

	size = td->xsize*td->ysize;

	src = td->layers[0];
	dst = td->framebuffer;

	end_dest = dst + size;

	while( dst < end_dest )
	{
		reg = *src;

		if( reg & 0xFF000000 )
		{
			if( ( reg & 0xFF000000 ) == 0xFF000000 )
			{
				*dst = reg;
			}
			else
			{
				*dst = alpha2(0x00000000, reg);
			}
		}
		else
		{
			*dst = 0x00000000;
		}

		src++;
		dst++;
	}

	for(i=1;i<3;i++)
	{
		src = td->layers[i];
		dst = td->framebuffer;

		end_dest = dst + size;

		while( dst < end_dest )
		{
			reg = *src;

			if( reg & 0xFF000000 )
			{
				if( ( reg & 0xFF000000 ) == 0xFF000000 )
				{
					*dst = reg;
				}
				else
				{
					*dst = alpha2(*dst, reg);
				}
			}

			src++;
			dst++;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

HXCFE_TD * hxcfe_td_init(HXCFE* floppycontext,uint32_t xsize,uint32_t ysize)
{
	HXCFE_TD * td;
	int i;

	td = malloc(sizeof(HXCFE_TD));
	if(td)
	{
		memset(td,0,sizeof(HXCFE_TD));
		td->hxcfe = floppycontext;

		td->xsize = xsize;
		td->ysize = ysize;

		td->virtual_xsize = xsize;
		td->virtual_ysize = ysize;

		td->window_xpos = 0;
		td->window_ypos = 0;

		td->x_us=200*1000;
		td->y_us=64;

		td->x_start_us=0;

		if(!bitmap_hxc2001_logo_bmp->unpacked_data)
		{
			bitmap_hxc2001_logo_bmp->unpacked_data = data_unpack(bitmap_hxc2001_logo_bmp->data,bitmap_hxc2001_logo_bmp->csize ,bitmap_hxc2001_logo_bmp->data, bitmap_hxc2001_logo_bmp->size);
		}

		td->framebuffer = malloc(td->xsize*td->ysize*sizeof(uint32_t));
		if(!td->framebuffer)
			goto alloc_error;

		memset(td->framebuffer,0,td->xsize*td->ysize*sizeof(uint32_t));

		i = 0;
		while( i < 3 )
		{
			td->layers[i] = malloc(td->xsize*td->ysize*sizeof(uint32_t));
			if(!td->layers[i])
				goto alloc_error;

			memset(td->layers[i],0,td->xsize*td->ysize*sizeof(uint32_t));
			i++;
		}

		td->hxc_setprogress = dummy_graph_progress;
		td->progress_userdata = 0;
	}

	return td;

alloc_error:

	free(td->framebuffer);

	i = 0;
	while( i < 3 )
	{
		free(td->layers[i]);
		i++;
	}

	free(td);

	return NULL;
}

int32_t hxcfe_td_zoom_area( HXCFE_TD *td, uint32_t x1, uint32_t y1 ,uint32_t x2, uint32_t y2)
{
	uint32_t t;
	float x_ratio, y_ratio, ratio;
	int32_t new_virtual_xsize;
	int32_t new_virtual_ysize;
	int32_t x_center, y_center;

#if 0
	printf("zi: x1:%d y1:%d x2:%d y2:%d\n",x1,y1,x2,y2);
	printf("zi: xsize:%d ysize:%d vxsize:%d vysize:%d\n",td->xsize,td->ysize,td->virtual_xsize,td->virtual_ysize);
	printf("zi: xpos:%d ypos:%d\n",td->window_xpos,td->window_ypos);
#endif

	if( !x1 && !x2 && !y1 && !y2 )
	{
		// restore initial 1x zoom

		td->virtual_xsize = td->xsize;
		td->virtual_ysize = td->ysize;

		td->window_xpos = 0;
		td->window_ypos = 0;

		return 0;
	}
	else
	{
		if( x1 > x2 )
		{
			t = x1;
			x1 = x2;
			x2 = t;
		}

		if( y1 > y2 )
		{
			t = y1;
			y1 = y2;
			y2 = t;
		}

		if(
			( (x2 - x1 > 0) && (y2 - y1 > 0) ) &&
			( (x1 < td->xsize ) && (x2 < td->xsize ) ) &&
			( (y1 < td->ysize ) && (y2 < td->ysize ) )
		)
		{
			// Valid parameters

			x_ratio = (float) td->virtual_xsize / (float) (x2 - x1);
			y_ratio = (float) td->virtual_ysize / (float) (y2 - y1);

			if( x_ratio < y_ratio )
				ratio = x_ratio;
			else
				ratio = y_ratio;

			new_virtual_xsize = (int32_t)(ratio * (float)td->virtual_xsize);

			if( new_virtual_xsize > 32*1024 )
			{
				ratio = (float)(32*1024) / (float)td->virtual_xsize;

				new_virtual_xsize = (int32_t)(ratio * (float)td->virtual_xsize);
			}

			new_virtual_ysize = (int32_t)(ratio * (float)td->virtual_ysize);
			if( new_virtual_ysize > 32*1024 )
			{
				ratio = (float)(32*1024) / (float)td->virtual_ysize;

				new_virtual_ysize = (int32_t)(ratio * (float)td->virtual_ysize);
				new_virtual_xsize = (int32_t)(ratio * (float)td->virtual_xsize);
			}

			x_center = (float)( ((x2 - x1) / 2) + x1 + td->window_xpos ) * ratio;
			y_center = (float)( ((y2 - y1) / 2) + y1 + td->window_ypos ) * ratio;

			td->window_xpos = x_center - (td->xsize / 2);
			td->window_ypos = y_center - (td->ysize / 2);

			if ( td->window_xpos + td->xsize >= new_virtual_xsize )
				td->window_xpos = new_virtual_xsize - ( td->xsize + 1 );

			if ( td->window_ypos + td->ysize >= new_virtual_ysize )
				td->window_ypos = new_virtual_ysize - ( td->ysize + 1 );

			if( td->window_xpos < 0 )
				td->window_xpos = 0;

			if( td->window_ypos < 0 )
				td->window_ypos = 0;

			td->virtual_xsize = new_virtual_xsize;
			td->virtual_ysize = new_virtual_ysize;

#if 0
			printf("zo: x1:%d y1:%d x2:%d y2:%d\n",x1,y1,x2,y2);
			printf("zo: xsize:%d ysize:%d vxsize:%d vysize:%d\n",td->xsize,td->ysize,td->virtual_xsize,td->virtual_ysize);
			printf("zo: xpos:%d ypos:%d\n",td->window_xpos,td->window_ypos);
#endif

			return 0;
		}
		else
		{
			return -1;
		}
	}
}

int32_t hxcfe_td_setProgressCallback( HXCFE_TD *td, HXCFE_TDPROGRESSOUT_FUNC progress_func, void * userdata )
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

void hxcfe_td_setparams( HXCFE_TD *td, uint32_t x_us, uint32_t y_us, uint32_t x_start_us, uint32_t flags )
{
	if(td)
	{
		td->x_us=x_us;
		td->y_us=y_us;

		td->x_start_us=x_start_us;

		td->flags = flags;
	}
}

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


//
// timingoffset -> uS
// start -> track bit start point
// offset -> track bit offset
//
float getOffsetTiming(HXCFE_SIDE *currentside,int offset,float timingoffset,int start)
{
	int i,j,totaloffset,partial_offset;
	uint32_t bitrate;
	float timinginc;
	uint32_t tracklen;
	uint32_t oldbitrate;

	tracklen = currentside->tracklen;

	if( offset >= start )
	{
		totaloffset = offset;
	}
	else
	{
		totaloffset = start + ((tracklen - start) + offset);
	}

	i = start;
	j = start%tracklen;

	if(currentside->cell_to_tick)
	{
		uint32_t start_tick,tracklen_tick,end_tick,offset_tick;

		start_tick = currentside->cell_to_tick[i>>3];
		tracklen_tick = currentside->cell_to_tick[(tracklen-1)>>3];
		end_tick = currentside->cell_to_tick[((( totaloffset ))%tracklen)>>3];

		if(end_tick >= start_tick)
		{
			offset_tick = end_tick - start_tick;
		}
		else
		{
			offset_tick = (tracklen_tick - start_tick) + end_tick;
		}

		timingoffset = timingoffset + (float)((float)offset_tick / (float)(currentside->tick_freq/1000000));

		return timingoffset;
	}

	if(currentside->bitrate==VARIABLEBITRATE)
	{
		if( i < totaloffset )
		{
			bitrate = currentside->timingbuffer[j>>3];
			oldbitrate = bitrate;
			timinginc = ((float)(500000)/(float)bitrate);

			while( i < totaloffset )
			{
				if( ((int)tracklen - j) > (totaloffset - i) )
				{
					partial_offset = totaloffset;
				}
				else
				{
					partial_offset = (tracklen - j);
				}

				while( i < partial_offset )
				{
					if(!(j&7))
					{
						bitrate = currentside->timingbuffer[j>>3];

						if( oldbitrate != bitrate)
						{
							timinginc = ((float)(500000)/(float)bitrate);
							oldbitrate = bitrate;
						}
					}

					timingoffset = timingoffset + timinginc;
					i++;
					j++;
				}

				j = 0;
			}
		}
	}
	else
	{
		timingoffset = timingoffset + ( ((float)(500000)/(float)currentside->bitrate) * ( totaloffset - start ) );
	}

	return timingoffset;
}

void hxcfe_td_draw_markers( HXCFE_TD *td, HXCFE_FLOPPY * floppydisk, int32_t track, int32_t side, int buffer_offset )
{
	int tracksize;
	int i,j,m;
	int y;
	float timingoffset;
	int bitrate;
	int xpos;
	int endfill;
	float xresstep;
	HXCFE_SIDE * currentside;

	xresstep = (float)td->x_us/(float)td->xsize;
	currentside=floppydisk->tracks[track]->sides[side];
	bitrate = currentside->bitrate;

	// Markers
	for( m = 0; m < MAX_MARKER ; m++ )
	{
		if( td->markers[m].flags & TD_MARKER_FLAG_ENABLE )
		{
			//////////////////////////////////////////
			// Markers drawing

			tracksize = currentside->tracklen;
			timingoffset = 0;
			i = buffer_offset;

			endfill = 0;

			do
			{
				do
				{
					if(td->markers[m].cell_pos == i)
					{
						if(currentside->bitrate == VARIABLEBITRATE)
							bitrate = currentside->timingbuffer[i>>3];

						xpos = (int)( timingoffset / (xresstep) );

						if(xpos>=td->xsize)
							endfill=1;
						else
						{
							int y_off;
							int x_off;
							int k;

							for(y = 0; y<td->ysize;y++)
							{
								if( !(y & 3) )
									td->layers[LAYER_TEXT][(td->xsize*y) + xpos] = COLOR_RGB2RGBA(td->markers[m].color, 255);
							}

							switch(td->markers[m].type)
							{
								case 0:

									x_off = 0;
									y_off = 30;
									k = 13;
									while( k>0 )
									{
										for(j=0;j<k;j++)
										{
											if( (y_off + k) < td->ysize && (xpos + x_off) < td->xsize)
											{
												td->layers[LAYER_TEXT][(td->xsize*(y_off + j)) + xpos + x_off] = COLOR_RGB2RGBA(td->markers[m].color, 255);
											}
										}
										y_off++;
										k = k - 2;
										x_off++;
									}
								break;
								case 1:
									x_off = 0;
									y_off = 30;
									k = 13;
									while( k>0 )
									{
										for(j=0;j<k;j++)
										{
											if( (y_off + k) < td->ysize && (xpos - x_off) < td->xsize && (xpos - x_off) >= 0)
											{
												td->layers[LAYER_TEXT][(td->xsize*(y_off + j)) + xpos - x_off] = COLOR_RGB2RGBA(td->markers[m].color, 255);
											}
										}
										y_off++;
										k = k - 2;
										x_off++;
									}
								break;
							}
						}
					}

					if(currentside->bitrate==VARIABLEBITRATE)
						bitrate = currentside->timingbuffer[i>>3];

					timingoffset=timingoffset + ((float)(500000)/(float)bitrate);

					i++;

				}while(i<tracksize && !endfill);

				xpos= (int)( timingoffset / xresstep );
				if(xpos>td->xsize)
					endfill=1;

				i=0;

			}while(!endfill);
		}
	}

	render(td);
}

void hxcfe_td_draw_rules( HXCFE_TD *td )
{
	int i,y;
	int xpos,ypos;
	char tmp_str[256];

	float x_us_per_pixel,y_us_per_pixel;
	float xpos_step;
	float float_xpos;

	x_us_per_pixel = ((float)((float)td->x_us/(float)td->xsize));
	y_us_per_pixel = ((float)((float)td->y_us/(float)td->ysize));

	// Print pixel density
	sprintf(tmp_str,"x:%.3f uS/pix y:%.3f uS/pix",x_us_per_pixel,y_us_per_pixel);
	putstring8x8(td,LAYER_TEXT,1+8*6,1,tmp_str,COLOR_RGBA(0,0,0,0),COLOR_RGBA(255,255,255,0),0,1);

	// Vertical rule
	for(i=0;i < 80*10;i++)
	{
		ypos = (td->ysize - 1) - (float)(((float)1/y_us_per_pixel) * ((float)i/(float)10));

		for(xpos=0;xpos < 2; xpos++)
		{
			if ( ( xpos < td->xsize ) && ( ( ypos < td->ysize ) && ( ypos >= 0 ) ) )
				td->layers[LAYER_TEXT][(td->xsize*ypos) + xpos] = COLOR_RGBA(0,0,0,255);
		}
	}

	for(i=0;i < 80;i++)
	{
		ypos = (td->ysize - 1) - ((1/y_us_per_pixel) * (float)i);

		for(xpos=(8*4);xpos < ((8*4)+6); xpos++)
		{
			if ( ( xpos < td->xsize ) && ( ( ypos < td->ysize ) && ( ypos >= 0 ) ) )
				td->layers[LAYER_TEXT][(td->xsize*ypos) + xpos] = COLOR_RGBA(0,0,0,255);
		}

		sprintf(tmp_str,"%.2duS",i);
		putstring8x8(td,LAYER_TEXT,1,ypos - 4,tmp_str,COLOR_RGBA(0,80,255,0),COLOR_RGBA(255,255,255,0),0,1);
	}

	// Rule : 100 us steps
	i = 0;
	xpos_step = ( 100 / x_us_per_pixel );
	float_xpos = 0;
	xpos = 0;
	while( xpos < td->xsize )
	{
		ypos = (td->ysize - 1);

		for(y=0;y<1;y++)
		{
			if ( ( xpos < (int)td->xsize ) && ( ( (ypos + y - 1) < (int)td->ysize ) && ( (ypos + y - 1) >= 0 ) ) )
				td->layers[LAYER_TEXT][(td->xsize*(ypos + y - 1)) + xpos] = COLOR_RGBA(0,0,0,255);
		}

		float_xpos += xpos_step;
		xpos = (int)float_xpos;
	}

	// Rule : 1 ms steps
	i = 0;
	xpos_step = ( 1000 / x_us_per_pixel );
	float_xpos = 0;
	xpos = 0;
	while( xpos < td->xsize )
	{
		ypos = (td->ysize - 1);

		for(y=ypos - 3;y<ypos;y++)
		{
			if ( y>=0 &&  y < (unsigned int)td->ysize )
				td->layers[LAYER_TEXT][(td->xsize*y) + xpos] = COLOR_RGBA(0,0,0,255);
		}

		float_xpos += xpos_step;
		xpos = (int)float_xpos;
	}

	// Rule : 10 ms steps
	i = 0;
	xpos_step = ( 10000 / x_us_per_pixel );
	float_xpos = 0;
	xpos = 0;
	while( xpos < td->xsize )
	{
		ypos = (td->ysize - 1);

		for(y=ypos - 6;y<ypos;y++)
		{
			if ( y>=0 &&  y < (unsigned int)td->ysize )
				td->layers[LAYER_TEXT][(td->xsize*y) + xpos] = COLOR_RGBA(0,0,0,255);
		}

		float_xpos += xpos_step;
		xpos = (int)float_xpos;
	}
}

int isAsciiChar(char c)
{
	if( c >= 0x20 && c <= 0x7E )
	{
		return 1;
	}

	return 0;
}

s_sectorlist * display_sectors(HXCFE_TD *td,HXCFE_FLOPPY * floppydisk,int track,int side,float timingoffset_offset, int TRACKTYPE)
{
	int tracksize;
	int i,j,old_i;
	char tempstr[512];
	char tempstr2[32];
	float timingoffset;
	float timingoffset2;
	float timingoffset3;
	float xresstep;
	int xpos_startheader,xpos_endsector,xpos_startdata;
	int xpos_tmp;
	int endfill,loop;
	s_col * col;
	HXCFE_SECTORACCESS* ss;
	HXCFE_SECTCFG* sc;
	HXCFE_SIDE * currentside;
	s_sectorlist * sl,*oldsl;

	xresstep = (float)td->x_us/(float)td->xsize;

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
											col=(s_col *)&td->layers[LAYER_IDENTIFICATION][(td->xsize*j) + i];
											col->blue=(unsigned char)((3*col->blue)/4);
											col->green=(unsigned char)((3*col->green)/4);
											col->alpha=128;
										}
									}
									else
									{
										// Sector ok -> Green
										for(j=50;j<(td->ysize-10);j++)
										{
											col=(s_col *)&td->layers[LAYER_IDENTIFICATION][(td->xsize*j) + i];
											col->blue=(unsigned char)((3*col->blue)/4);
											col->red=(unsigned char)((3*col->red)/4);
											col->alpha=128;
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
											col=(s_col *)&td->layers[LAYER_IDENTIFICATION][(td->xsize*j) + i];
											col->green=(unsigned char)((3*col->green)/4);
											col->red=(unsigned char)((3*col->red)/4);
											col->alpha=128;
										}
									}
									else
									{
										if(sc->use_alternate_data_crc)
										{
											// CRC error -> Red
											for(j=50;j<(td->ysize-10);j++)
											{
												col=(s_col *)&td->layers[LAYER_IDENTIFICATION][(td->xsize*j) + i];
												col->blue=(unsigned char)((3*col->blue)/4);
												col->green=(unsigned char)((3*col->green)/4);
												col->alpha=128;
											}
										}
										else
										{
											// Sector ok -> Green
											for(j=50;j<(td->ysize-10);j++)
											{
												col=(s_col *)&td->layers[LAYER_IDENTIFICATION][(td->xsize*j) + i];
												if(!sc->fill_byte_used)
												{
													col->blue=(unsigned char)((3*col->blue)/6);
													col->red=(unsigned char)((3*col->red)/6);
												}
												else
												{
													col->blue=(unsigned char)((3*col->blue)/4);
													col->red=(unsigned char)((3*col->red)/4);
												}
												col->alpha=128;
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
										col=(s_col *)&td->layers[LAYER_IDENTIFICATION][(td->xsize*j) + xpos_startheader];
										col->blue=(unsigned char)((3*col->blue)/8);
										col->green=(unsigned char)((3*col->green)/8);
										col->alpha=255;
									}
									else
									{
										col=(s_col *)&td->layers[LAYER_IDENTIFICATION][(td->xsize*j) + xpos_startheader];
										col->blue=(unsigned char)((3*col->blue)/8);
										col->red=(unsigned char)((3*col->red)/8);
										col->alpha=255;
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
											col=(s_col *)&td->layers[LAYER_IDENTIFICATION][(td->xsize*j) + xpos_startdata];
											col->blue=(unsigned char)((3*col->blue)/16);
											col->green=(unsigned char)((3*col->green)/16);
											col->red=(unsigned char)((3*col->red)/4);
											col->alpha=255;
										}
										else
										{
											col=(s_col *)&td->layers[LAYER_IDENTIFICATION][(td->xsize*j) + xpos_startdata];
											col->blue=(unsigned char)((3*col->blue)/16);
											col->green=(unsigned char)((3*col->green)/4);
											col->red=(unsigned char)((3*col->red)/16);
											col->alpha=255;
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
								case DECRX02_SDDD:
									if(sc->startdataindex != sc->endsectorindex)
										sprintf(tempstr,"DEC M2FM  %.3dB DM:%.2Xh",sc->sectorsize,sc->alternate_datamark);
									else
										sprintf(tempstr,"DEC M2FM  %.3dB DM: ??",sc->sectorsize);
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
									sprintf(tempstr,"Apple II GCR5A3 %.3dB ",sc->sectorsize);
								break;
								case APPLE2_GCR6A2:
									sprintf(tempstr,"Apple II GCR6A2 %.3dB ",sc->sectorsize);
								break;
								case APPLEMAC_GCR6A2:
									sprintf(tempstr,"Apple Macintosh GCR6A2 %.3dB ",sc->sectorsize);
								break;
								case ARBURG_DAT:
									sprintf(tempstr,"Arburg DATA %.3dB ",sc->sectorsize);
								break;
								case ARBURG_SYS:
									sprintf(tempstr,"Arburg SYSTEM %.3dB ",sc->sectorsize);
								break;
								case AED6200P_DD:
									if(sc->startdataindex != sc->endsectorindex)
										sprintf(tempstr,"AED 6200P %.3dB DM:%.2Xh",sc->sectorsize,sc->alternate_datamark);
									else
										sprintf(tempstr,"AED 6200P %.3dB DM: ??",sc->sectorsize);
									break;
								break;
								case QD_MO5_MFM:
									sprintf(tempstr,"QD MO5 %.3dB",sc->sectorsize);
								break;
								case C64_GCR:
									sprintf(tempstr,"C64 GCR %.3dB",sc->sectorsize);
								break;
								case VICTOR9K_GCR:
									sprintf(tempstr,"VICTOR 9K GCR %.3dB",sc->sectorsize);
								break;
							}

							if(sc->fill_byte_used)
								sprintf(tempstr2," F:%.2Xh",sc->fill_byte);
							else
								strcpy(tempstr2,"");

							strcat(tempstr,tempstr2);

							putstring8x8(td,LAYER_TEXT, xpos_startheader,225,tempstr,0x000,0x000,1,1);

							if(sc->startdataindex != sc->endsectorindex)
								sprintf(tempstr,"T:%.2d H:%d S:%.3d CRC:%.4X",sc->cylinder,sc->head,sc->sector,sc->data_crc);
							else
								sprintf(tempstr,"T:%.2d H:%d S:%.3d NO DATA?",sc->cylinder,sc->head,sc->sector);
							putstring8x8(td,LAYER_TEXT, xpos_startheader+8,225,tempstr,0x000,0x000,1,1);
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
								case AED6200P_DD:
									sprintf(tempstr,"AED 6200P Data ?");
								break;
								case QD_MO5_MFM:
									sprintf(tempstr,"QD MO5 Data ?");
								break;
								case VICTOR9K_GCR:
									sprintf(tempstr,"Victor 9K Data ?");
								break;
							}
							putstring8x8(td,LAYER_TEXT,xpos_startheader,225,tempstr,0x000,0x000,1,1);
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
										col=(s_col *)&td->layers[LAYER_IDENTIFICATION][(td->xsize*j) + (xpos_endsector-1)];
										col->blue=(unsigned char)((3*col->blue)/8);
										col->green=(unsigned char)((3*col->green)/8);
										col->alpha=255;
									}
									else
									{
										col=(s_col *)&td->layers[LAYER_IDENTIFICATION][(td->xsize*j) + (xpos_endsector-1)];
										col->blue = (unsigned char)((3*col->blue)/8);
										col->red = (unsigned char)((3*col->red)/8);
										col->alpha=255;
									}
								}
							}
						}

						if(sc->input_data_index && sc->input_data && sc->sectorsize > 2)
						{
							int x1,x2;

							timingoffset2 = getOffsetTiming(currentside,sc->input_data_index[0],timingoffset,old_i);
							x1 = (int)( ( timingoffset2 - timingoffset_offset ) / xresstep );

							timingoffset3 = getOffsetTiming(currentside,sc->input_data_index[1],timingoffset,old_i);
							x2 = (int)( ( timingoffset3 - timingoffset_offset ) / xresstep );

							if(x2-x1>=8)
							{
								for(i=0;i<sc->sectorsize;i++)
								{
									timingoffset3 = getOffsetTiming(currentside,sc->input_data_index[i],timingoffset,old_i);
									xpos_tmp = (int)( ( timingoffset3 - timingoffset_offset ) / xresstep );
									if( xpos_tmp < td->xsize )
									{
										if(x2-x1>=16)
										{
											sprintf(tempstr,"%.2X",sc->input_data[i]);
											putstring8x8(td,LAYER_TEXT,xpos_tmp,225,tempstr,0x000,0x000,0,1);
										}

										if( isAsciiChar(sc->input_data[i]) )
										{
											sprintf(tempstr,"%c",sc->input_data[i]);
											putstring8x8(td,LAYER_TEXT,xpos_tmp+4,225+10,tempstr,0xF25,0x000,0,1);
										}
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

		if(loop>100 || td->noloop_trackmode)
			endfill=1;
	}while(!endfill);

	td->sl=sl;

	return sl;
}

void hxcfe_td_activate_analyzer( HXCFE_TD *td, int32_t TRACKTYPE, int32_t enable )
{
	if(td && TRACKTYPE<32)
	{
		if(enable)
			td->enabledtrackmode |=  (0x00000001 << TRACKTYPE);
		else
			td->enabledtrackmode &= ~(0x00000001 << TRACKTYPE);
	}
}

void hxcfe_td_draw_track( HXCFE_TD *td, HXCFE_FLOPPY * floppydisk, int32_t track, int32_t side )
{
	int tracksize;
	int i,j,old_i;
	float timingoffset_offset;
	int buffer_offset;
	float timingoffset;
	float timingoffset2;
	int interbit;
	int bitrate;
	int old_index_state;
	int xpos,ypos,old_xpos;
	int endfill,loopcnt;
	int last_index_xpos;
	float last_index_timingoffset;
	int tmp;
	float xresstep,index_period;
	s_sectorlist * sl,*oldsl;
	s_pulseslist * pl,*oldpl;
	HXCFE_SIDE * currentside;
	s_col * col;

	char tmp_str[256];

	sl=td->sl;
	while(sl)
	{
		oldsl = sl->next_element;
		//sl = sl->next_element;

		hxcfe_freeSectorConfig( 0, sl->sectorconfig );

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

	if(side>=floppydisk->floppyNumberOfSide) side = floppydisk->floppyNumberOfSide - 1;
	if(side<0) side = 0;

	if(!floppydisk->floppyNumberOfTrack || !floppydisk->floppyNumberOfSide)
	{
		memset(td->layers[LAYER_SUPPORT],0xCC,td->xsize*td->ysize*sizeof(unsigned int));
		return;
	}

	currentside=floppydisk->tracks[track]->sides[side];

	tracksize=currentside->tracklen;

	old_i=0;
	i=0;

	clear_layers( td );

	timingoffset = ( getOffsetTiming(currentside,tracksize,0,0));

	timingoffset2=( timingoffset * td->x_start_us ) / (100 * 1000);

	timingoffset=0;
	while((i<tracksize) && timingoffset2>timingoffset)
	{
		timingoffset=getOffsetTiming(currentside,i,timingoffset,old_i);
		old_i=i;
		i++;
	};

	timingoffset_offset = getOffsetTiming(currentside,i,0,0);

	buffer_offset=i;
	bitrate = currentside->bitrate;

	tracksize=currentside->tracklen;
	timingoffset=0;
	interbit=0;
	i=buffer_offset;

	xresstep = 0;
	if(td->xsize)
		xresstep = (float)td->x_us/(float)td->xsize;
	endfill=0;

	if(!xresstep)
		return;

	if(td->name)
	{
		sprintf(tmp_str,"%s T:%.3d S:%.1d",td->name,track,side);
	}
	else
	{
		sprintf(tmp_str,"T:%.3d S:%.1d",track,side);
	}
	putstring8x8(td,LAYER_IDENTIFICATION,td->xsize - (strlen(tmp_str)*8) ,1,tmp_str,0xAAAAAA,0x000000,0,1);

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

				if(bitrate <= 0)
				{
					// Invalid bitrate...
					if(currentside->bitrate == VARIABLEBITRATE)
						sprintf(tmp_str,"ERROR : Invalid bitrate -> bitrate[%d] == %d ! Please check the loader or report the issue !",i>>3,bitrate);
					else
						sprintf(tmp_str,"ERROR : Invalid bitrate -> bitrate == %d ! Please check the loader or report the issue !",bitrate);

					putstring8x8(td,LAYER_IDENTIFICATION,td->xsize/2 - (strlen(tmp_str)*8)/2 ,td->ysize/2,tmp_str,0x0000FF,0x000000,0,1);

					td->hxcfe->hxc_printf(MSG_ERROR,tmp_str);
					return;
				}

				xpos= (int)( timingoffset / (xresstep) );
				ypos= td->ysize - (int)( ( (float) ((interbit+1) * 500000) / (float) bitrate ) / (float)((float)td->y_us/(float)td->ysize));

				if(td->x_us>250)
				{
					// Scather gatter display mode
					if( (xpos<td->xsize) && (ypos<td->ysize) && ypos>=0 )
					{
						td->layers[LAYER_SUPPORT][(td->xsize*ypos) + xpos]++;

						ypos--;
						if( (ypos<td->ysize) && ypos>=0 )
						{
							td->layers[LAYER_SUPPORT][(td->xsize*ypos) + xpos]++;
						}
						ypos=ypos+2;
						if( (ypos<td->ysize) && ypos>=0 )
						{
							td->layers[LAYER_SUPPORT][(td->xsize*ypos) + xpos]++;
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
							td->layers[LAYER_SUPPORT][(td->xsize*ypos) + xpos]=255;
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

			timingoffset += ((float)(500000)/(float)bitrate);

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
		col=(s_col *)&td->layers[LAYER_SUPPORT][i];

		if(!col->red && !col->green && !col->blue)
		{
			td->layers[LAYER_SUPPORT][i] = COLOR_RGBA(255,255,255,255);
		}
		else
		{
			col->green = (unsigned char)( 255 - col->red );
			col->blue = (unsigned char)( 255 - col->red );
			col->red = (unsigned char)( 255 - col->red );

			col->green = (unsigned char)(col->green / 2);
			col->blue = (unsigned char)(col->blue / 2);
			col->red = (unsigned char)(col->red / 2);
			col->alpha=255;
		}
	}

	for(i=0;i<td->xsize*td->ysize;i++)
	{
		td->layers[LAYER_IDENTIFICATION][i] = COLOR_RGBA(255,255,255,0);
	}

	//////////////////////////////////////////
	// Flakey bits drawing
	if(currentside->flakybitsbuffer)
	{
		tracksize = currentside->tracklen;;
		timingoffset = 0;
		i = buffer_offset;
		old_i = buffer_offset;

		loopcnt = 0;
		endfill = 0;
		do
		{
			do
			{

				timingoffset = getOffsetTiming(currentside,i,timingoffset,old_i);
				old_i = i;
				xpos = (int) ( timingoffset / xresstep );
				if( ( xpos >= td->xsize ) )
				{
					endfill = 1;
				}

				if( currentside->flakybitsbuffer[i>>3] & (0x80>>(i&7)) )
				{
					if( (xpos<td->xsize) )
					{
						for(j=30;j<50;j++)
						{
							td->layers[LAYER_SUPPORT][(td->xsize*j) + xpos] = COLOR_RGBA(255,0,0,255);
						}
					}
				}

				i++;

			}while(i<tracksize && !endfill);

			loopcnt++;

			if( i == tracksize && loopcnt>1000)
				endfill = 1;

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
	loopcnt = 0;
	endfill=0;
	last_index_xpos = -1;
	last_index_timingoffset = 0;
	old_index_state = 0;
	do
	{
		do
		{
			timingoffset = getOffsetTiming(currentside,i,timingoffset,old_i);
			old_i=i;
			xpos= (int)( timingoffset / xresstep );

			if( (xpos<td->xsize) )
			{
				if( currentside->indexbuffer[i>>3] )
				{
					if(!old_index_state)
					{
						if(last_index_xpos != -1)
						{
							index_period = ((float)(timingoffset - last_index_timingoffset ));
							sprintf(tmp_str,"%.2f RPM / %.2f ms", (float)(60 * 1000) / (index_period / (float)1000), index_period / (float)1000 );

							if(xpos - last_index_xpos > strlen(tmp_str)*8)
							{
								tmp = ((xpos - last_index_xpos) - (strlen(tmp_str)*8)) / 2;
							}
							else
								tmp = 8;

							putstring8x8(td,LAYER_IDENTIFICATION,last_index_xpos + tmp, 16,tmp_str,0x000000,0x000000,0,1);
						}

						last_index_xpos = xpos;
						last_index_timingoffset = timingoffset;
						old_index_state = 1;
					}

					for(j=10;j<12;j++)
					{
						td->layers[LAYER_SUPPORT][(td->xsize*j) + xpos] = COLOR_RGBA(0,0,255,255);
					}
				}
				else
				{
					old_index_state = 0;
					for(j=25;j<27;j++)
					{
						td->layers[LAYER_SUPPORT][(td->xsize*j) + xpos] = COLOR_RGBA(0,0,255,255);
					}
				}
			}
			else
			{
				endfill=1;
			};
			i++;
		}while(i<tracksize && !endfill);

		loopcnt++;

		if( i == tracksize && loopcnt>1000)
			endfill = 1;

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
				xpos = (int)(( timingoffset / xresstep ) - (( timingoffset2 - timingoffset) / (xresstep *2) ));

				if(xpos>=0 && (xpos<td->xsize) )
				{
					for(ypos= td->ysize - 40 ; ypos > (td->ysize - 50) ; ypos--)
					{
						td->layers[LAYER_SUPPORT][(td->xsize*ypos) + xpos] = COLOR_RGBA(0,255,0,255);
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
			td->layers[LAYER_SUPPORT][(td->xsize*ypos) + xpos] = COLOR_RGBA(0,155,0,255);
		}
	}
	else
	{
		// Pulse list generation
		tracksize = currentside->tracklen;
		old_i = buffer_offset;
		i = buffer_offset;
		timingoffset=0;

		loopcnt = 0;
		endfill=0;
		old_xpos = -1;
		do
		{
			do
			{
				timingoffset=getOffsetTiming(currentside,i,timingoffset,old_i);
				timingoffset2=getOffsetTiming(currentside,i+1,timingoffset,i);

				old_i=i;
				xpos = (int)( timingoffset / xresstep ) - (int)(( timingoffset2 - timingoffset) / (xresstep *2) );

				if(xpos>=0 && (xpos<td->xsize) && ( old_xpos != xpos ) )
				{
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
						pl->x_pos2 = xpos + 1;//(int)(( timingoffset2 - timingoffset) / (xresstep) );
						pl->next_element = oldpl;
						old_xpos = xpos;
					}
				}
				else
				{
					if(xpos >= 0 && ( old_xpos != xpos ) )
						endfill = 1;
				};
				i++;
			}while(i<tracksize && !endfill);

			loopcnt++;

			if( i == tracksize && loopcnt>1000)
				endfill = 1;

			old_i=0;
			i=0;


		}while(!endfill);

		td->pl = pl;
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

	hxcfe_td_draw_rules( td );

	hxcfe_td_draw_markers( td, floppydisk, track, side, buffer_offset );

	sprintf(tmp_str,"libhxcfe v%s",STR_FILE_VERSION2);
	putstring8x8(td,LAYER_TEXT,td->xsize - (8 + 1),td->ysize - (8*3 + 1),tmp_str,0x000000,0xFFFFFF,1,1);

	splash_sprite(bitmap_hxc2001_logo_bmp,td->layers[LAYER_TEXT], td->xsize, td->ysize, td->xsize - (bitmap_hxc2001_logo_bmp->Xsize + 16), td->ysize - (bitmap_hxc2001_logo_bmp->Ysize + 16), LOGOALPHA);

	render( td );
}

extern int tracktypelist[];

int32_t bands_no_band[]={0,90000,-1,-1};
int32_t bands_dd_mfm[]={3500,4600, 5400,6600, 7400,8600,-1,-1};
int32_t bands_hd_mfm[]={1250,2250, 2750,3250, 3750,4250,-1,-1};
int32_t bands_dd_fm[]={3000,5000, 7000,8000,-1,-1};
int32_t bands_hd_fm[]={1500,2500, 3500,4500,-1,-1};

int32_t * bands_type[]={
	bands_no_band,
	bands_dd_mfm,
	bands_hd_mfm,
	bands_dd_fm,
	bands_hd_fm,
	NULL
};

int is_valid_timing(HXCFE_TD *td, int ps )
{
	int32_t * bands;
	int i;

	bands = bands_type[td->bands_type];

	i = 0;
	while( bands[i] >= 0 )
	{
		if( ps > bands[i] && ps <= bands[i+1])
			return 1;

		i += 2;
	}

	return 0;
}

int hxcfe_td_stream_to_sound( HXCFE_TD *td, HXCFE_STREAMCHANNEL* stream_channel, int stream_index,uint16_t * sound_buffer, int nbsamples, int samplerate)
{
	int i;
	int cur_ticks_count;
	int ticks_window,ticks_sampleshift;
	int pulses_count;
	int next_start_index;
	int next_start_remain;
	int next_start_updated;

	td->hxcfe->hxc_printf(MSG_INFO_1,"hxcfe_td_stream_to_sound: stream_index:%d  stream_channel->nb_of_pulses : %d",stream_index,stream_channel->nb_of_pulses);

	// | Stream window scan : Count pulses |    - cnt 1
	//      | Stream window scan : Count pulses |    - cnt 2 (+22uS - 44100)
	//           | Stream window scan : Count pulses |    - cnt 3 (+22uS - 44100)
	// | cnt 1 | cnt 2 | cnt 3 |  calc mean
	// min / max / mean buffer normalise

	ticks_window = (int)(( (float)TICKFREQ / (float)1000000 ) * (float)200); // 200 us
	ticks_sampleshift = (int)(( (float)TICKFREQ / (float)1000000 ) * (float)((float)((float)1000000/(float)samplerate) ));

	if( stream_index >= stream_channel->nb_of_pulses)
		return stream_index;

	next_start_updated = 0;
	next_start_remain = 0;
	next_start_index = stream_index;

	for(i=0;i<nbsamples;i++)
	{
		pulses_count = 0;
		cur_ticks_count = next_start_remain;
		stream_index = next_start_index;
		next_start_updated = 0;

		while(cur_ticks_count < ticks_window && stream_index < stream_channel->nb_of_pulses )
		{
			cur_ticks_count += stream_channel->stream[stream_index];

			if( !next_start_updated && (cur_ticks_count >= ticks_sampleshift) )
			{
				next_start_index = stream_index;
				next_start_remain = cur_ticks_count - ticks_sampleshift;
				next_start_updated = 1;
				td->hxcfe->hxc_printf(MSG_INFO_1,"hxcfe_td_stream_to_sound: >>next_start_index:%d - next_start_remain:%d",next_start_index,pulses_count,cur_ticks_count,ticks_window,ticks_sampleshift);

			}

			if(cur_ticks_count >= ticks_window)
			{
				break;
			}
			else
				pulses_count++;

			stream_index++;
		}

		td->hxcfe->hxc_printf(MSG_INFO_1,"hxcfe_td_stream_to_sound: >>stream_index:%d - pulses_count:%d cur_ticks_count:%d ticks_window:%d ticks_sampleshift:%d",stream_index,pulses_count,cur_ticks_count,ticks_window,ticks_sampleshift);

		sound_buffer[i] = pulses_count;
	}

	return stream_index;
}

void hxcfe_td_draw_trkstream( HXCFE_TD *td, HXCFE_TRKSTREAM* track_stream )
{
	int buffer_offset;
	int total_tick,max_total_tick;
	int i,j,x,y,tmp;
	int t_ofs1,t_ofs2;
	int xpos_start,xpos,ypos;
	int last_index_xpos;
	int last_index_total_offset;
	char tmp_str[256];
	uint32_t total_offset,cur_ticks,contrast;
	HXCFE_SIDE* side;
	HXCFE_FLOPPY *fp;
	int tracksize;
	int tick_to_remove;

	int bitrate;
	int channel,bitstate,bitstate2;
	uint32_t * histo;

	float x_us_per_pixel,y_us_per_pixel;
	float x_tick_to_pix;
	float y_tick_to_pix;
	float tick_to_ps;
	float index_period;
	float timingoffset,timingoffset2;
	float timingoffset_offset;
	HXCFE_SIDE * currentside;

	int channel_maxtick[MAX_NB_OF_STREAMCHANNEL];
	int channel_buffer_offset[MAX_NB_OF_STREAMCHANNEL];

	HXCFE_FXSA * fxs;

	fxs = hxcfe_initFxStream( td->hxcfe );

	td->noloop_trackmode = 1;

	clear_layers( td );
	memset(td->layers[LAYER_SUPPORT],0xFF,td->xsize*td->ysize*sizeof(uint32_t));

	x_us_per_pixel = ((float)((float)td->x_us/(float)td->xsize));
	y_us_per_pixel = ((float)((float)td->y_us/(float)td->ysize));

	x_tick_to_pix = ( (float)1.0 / x_us_per_pixel ) * ( (float)1000000 / (float)track_stream->tick_freq );
	y_tick_to_pix = ( (float)1.0 / y_us_per_pixel ) * ( (float)1000000 / (float)track_stream->tick_freq );

	if( hxcfe_getEnvVarValue( td->hxcfe, "BMPEXPORT_STREAM_HIGHCONTRAST" ) || (x_us_per_pixel < 200) )
	{
		td->flags |= TD_FLAG_HICONTRAST;
	}

	if( hxcfe_getEnvVarValue( td->hxcfe, "BMPEXPORT_STREAM_BIG_DOTS" ) || (x_us_per_pixel < 16) || td->disk_type == 2 )
	{
		td->flags |= TD_FLAG_BIGDOT;
	}

	tick_to_ps = (float) ( (float)1000000000 / (float)track_stream->tick_freq);

	memset(channel_buffer_offset,0,sizeof(channel_buffer_offset));

	max_total_tick = 0;

	if(td->x_start_us)
	{
		memset(channel_maxtick,0,sizeof(channel_maxtick));

		for(channel = 0;channel < MAX_NB_OF_STREAMCHANNEL; channel++ )
		{
			if(track_stream->channels[channel].stream && track_stream->channels[channel].nb_of_pulses)
			{
				total_tick = 0;
				for (i = 0; i < (int)track_stream->channels[channel].nb_of_pulses; i++)
				{
					total_tick += track_stream->channels[channel].stream[i];
				}

				channel_maxtick[channel] = total_tick;

				td->hxcfe->hxc_printf(MSG_DEBUG,"hxcfe_td_draw_trkstream: chn %d , tick %d , pulses %d\n",channel,total_tick,track_stream->channels[channel].nb_of_pulses);
#if 0
				printf("hxcfe_td_draw_trkstream: chn %d , tick %d , pulses %d\n",channel,total_tick,track_stream->channels[channel].nb_of_pulses);

				if(track_stream->channels[channel].nb_of_pulses<16)
				{
					for(i=0;i<track_stream->channels[channel].nb_of_pulses;i++)
					{
						printf("%d , tick  %d\n",i,track_stream->channels[channel].stream[i]);
					}
				}
#endif
				if(total_tick > max_total_tick)
					max_total_tick = total_tick;
			}
		}
	}

	tmp = td->ysize * td->xsize;
	for(i = 0; i < tmp; i++)
	{
		td->layers[LAYER_IDENTIFICATION][i] = 0xFFFFFF;
	}

	contrast = 0x202020;
	if(td->flags & TD_FLAG_HICONTRAST)
		contrast = 0x808080;

	for(channel = 0;channel < MAX_NB_OF_STREAMCHANNEL; channel++ )
	{
		if(track_stream->channels[channel].stream && track_stream->channels[channel].nb_of_pulses)
		{
			tick_to_remove = 0;
			timingoffset_offset = 0;
			bitstate2 = 0;
			if(td->x_start_us)
			{
				i = 0;
				t_ofs2= ((float)( td->x_start_us ) / (float)(100 * 1000)) * max_total_tick;
				t_ofs1= track_stream->channels[channel].stream[i];
				while((i<(int)(track_stream->channels[channel].nb_of_pulses - 1)) && t_ofs2>t_ofs1)
				{
					t_ofs1 += track_stream->channels[channel].stream[++i];
					bitstate2 ^= 1;
				};

				tick_to_remove = t_ofs2 - ( t_ofs1 - track_stream->channels[channel].stream[i] );

				buffer_offset = i;
			}
			else
			{
				t_ofs2 = 0;
				buffer_offset = 0;
			}

			channel_buffer_offset[channel] = buffer_offset;

			switch(track_stream->channels[channel].type)
			{
				case HXCFE_STREAMCHANNEL_TYPE_RLEEVT:
					//////////////////////////////////////////
					// Scatter drawing
					total_offset = 0;
					for (i = buffer_offset; i < (int)track_stream->channels[channel].nb_of_pulses; i++)
					{
						cur_ticks = track_stream->channels[channel].stream[i];

						cur_ticks -= tick_to_remove;
						tick_to_remove =0;

						total_offset += cur_ticks;

						xpos = (int)( (float)total_offset * x_tick_to_pix );
						ypos = td->ysize - (int)( (float)cur_ticks * y_tick_to_pix );

						if ( ( xpos < td->xsize ) && ( ( ypos < td->ysize ) && ( ypos >= 0 ) ) )
						{
							if( is_valid_timing(td,(int)((float)cur_ticks * tick_to_ps )) )
							{
								uint32_t * tmpptr;

								if( td->flags & TD_FLAG_BIGDOT )
								{
									for(x=0;x<3;x++)
									{
										for(y=0;y<3;y++)
										{
											if( xpos + x - 1 >= 0 && xpos + x + 1 < td->xsize)
											{
												if( ypos + y - 1 >= 0 && ypos + y + 1 < td->ysize)
												{

													tmpptr = &td->layers[LAYER_IDENTIFICATION][(td->xsize*(ypos + y - 1)) + (xpos + x - 1)];

													if( (*tmpptr & 0xFF) >= (contrast & 0xFF) )
													{
														*tmpptr = ( (*tmpptr & 0xFFFFFF) - contrast ) | 0xFF000000;
													}
													else
													{
														*tmpptr = COLOR_RGBA(0,0,0,255);
													}
												}
											}
										}
									}
								}
								else
								{
									tmpptr = &td->layers[LAYER_IDENTIFICATION][(td->xsize*(ypos)) + xpos];

									if( (*tmpptr & 0xFF) >= (contrast & 0xFF) )
									{
										*tmpptr = ( (*tmpptr & 0xFFFFFF) - contrast ) | 0xFF000000;
									}
									else
									{
										*tmpptr = COLOR_RGBA(0,0,0,255);
									}
								}
							}
							else
							{
								td->layers[LAYER_IDENTIFICATION][(td->xsize*ypos) + xpos] = COLOR_RGBA(255,0,0,255);
							}
						}
					}
				break;

				case HXCFE_STREAMCHANNEL_TYPE_RLETOGGLESTATE_0:
				case HXCFE_STREAMCHANNEL_TYPE_RLETOGGLESTATE_1:

					total_offset = 0;
					xpos_start = 0;
					bitstate = 0;

					if( track_stream->channels[channel].type == HXCFE_STREAMCHANNEL_TYPE_RLETOGGLESTATE_1)
						bitstate = 1;

					for (i = buffer_offset; i < (int)track_stream->channels[channel].nb_of_pulses; i++)
					{
						cur_ticks = track_stream->channels[channel].stream[i];

						cur_ticks -= tick_to_remove;

						tick_to_remove = 0;

						total_offset += cur_ticks;

						xpos = (int)( (float)total_offset * x_tick_to_pix );

						if(bitstate ^ bitstate2)
							ypos = (channel * 15) + 20;
						else
							ypos = (channel * 15) + 30;

						bitstate ^= 1;

						for(;xpos_start<xpos;xpos_start++)
						{
							if ( ( xpos_start < td->xsize ) && ( ( ypos < td->ysize ) && ( ypos >= 0 ) ) )
							{
								td->layers[LAYER_IDENTIFICATION][(td->xsize*ypos) + xpos_start] = COLOR_RGBA(0,0,255,255);
							}
						}
					}

				break;
			}
		}
	}

	//////////////////////////////////////////
	// Draw indexes

	last_index_xpos = -1;
	last_index_total_offset = 0;

	for(i=0;i < (int)track_stream->nb_of_index;i++)
	{
		if( channel_buffer_offset[0] <= (int)track_stream->index_evt_tab[i].dump_offset )
		{
			break;
		}
	}

	for(;i < (int)track_stream->nb_of_index;i++)
	{
		j = channel_buffer_offset[0];
		total_offset = 0;
		while( ( j < (int)track_stream->index_evt_tab[i].dump_offset ) && ( j < (int)track_stream->channels[0].nb_of_pulses ) )
		{
			j++;
			total_offset += track_stream->channels[0].stream[j];
		}

		xpos = (int)( (float)total_offset * x_tick_to_pix );
		ypos = 0;

		if ( ( xpos < td->xsize ) && ( ypos < td->ysize ) )
		{
			for(ypos=0;ypos<td->ysize;ypos++)
			{
				if( ! (td->layers[LAYER_IDENTIFICATION][(td->xsize*(ypos)) + xpos] & 0xFF000000 ) )
				{
					td->layers[LAYER_IDENTIFICATION][(td->xsize*(ypos)) + xpos] = COLOR_RGBA(0,0,255,255);
				}
			}
		}

		if(last_index_xpos != -1)
		{
			index_period = ((float)(total_offset - last_index_total_offset ) * ( (float)1000000 / (float)track_stream->tick_freq ) );
			sprintf(tmp_str,"%.2f RPM / %.2f ms", (float)(60 * 1000) / (index_period / (float)1000), index_period / (float)1000 );

			if(xpos - last_index_xpos > strlen(tmp_str)*8)
			{
				tmp = ((xpos - last_index_xpos) - (strlen(tmp_str)*8)) / 2;
			}
			else
				tmp = 8;

			putstring8x8(td,LAYER_TEXT,last_index_xpos + tmp, 8 + 2,tmp_str,0x000000,0x000000,0,1);
		}
		last_index_total_offset = total_offset;
		last_index_xpos = xpos;
	}

	bitrate = 500;

	histo = (uint32_t*)malloc(65536* sizeof(uint32_t));
	if(histo)
	{
		computehistogram(track_stream->channels[0].stream, track_stream->channels[0].nb_of_pulses, histo);

		bitrate = detectpeaks(td->hxcfe,&fxs->pll,histo);

		free(histo);
	}

	side = ScanAndDecodeStream(td->hxcfe, fxs, bitrate,track_stream,NULL,0, 0, 8, 0x0001);
	if(side)
	{
		cleanupTrack(side);

		side->track_encoding = UNKNOWN_ENCODING;
		side->bitrate = VARIABLEBITRATE;

		fp = makefloppyfromtrack(side);
		if(fp)
		{
			i = 0;
			timingoffset_offset = 0;

			if( td->x_start_us )
			{
				int track_size_bytes;
				uint32_t start_tick,ticks_point,cur_tick;

				currentside = fp->tracks[0]->sides[0];

				tracksize = currentside->tracklen;

				if(tracksize & 0x7)
					track_size_bytes = (tracksize >> 3);
				else
					track_size_bytes = ((tracksize >> 3) - 1);

				timingoffset = ((float)(currentside->cell_to_tick[track_size_bytes] - currentside->cell_to_tick[0]) / (float)(track_stream->tick_freq/1000000));

				timingoffset2 = ( timingoffset * td->x_start_us ) / (100 * 1000);

				ticks_point = (uint32_t)(timingoffset2 * (float)(track_stream->tick_freq/1000000));

				start_tick = currentside->cell_to_tick[0];

				cur_tick = 0;
				i = 0;
				while((i<track_size_bytes) && ticks_point>cur_tick)
				{
					cur_tick = currentside->cell_to_tick[i] - start_tick;
					i++;
				};

				timingoffset_offset = ((float)(cur_tick) / (float)(track_stream->tick_freq/1000000));
			}

			//////////////////////////////////////////
			// Sector drawing
			for(i=0;i<32;i++)
			{
				if(td->enabledtrackmode & (0x00000001<<i) )
				{
					display_sectors(td,fp,0,0,timingoffset_offset,i);
				}
			}

			freefloppy(fp);
		}

		hxcfe_freeSide(td->hxcfe,side);
	}

	hxcfe_deinitFxStream( fxs );

	hxcfe_td_draw_rules( td );

	sprintf(tmp_str,"libhxcfe v%s",STR_FILE_VERSION2);
	putstring8x8(td,LAYER_TEXT,td->xsize - (8 + 1),td->ysize - (8*3 + 1),tmp_str,0x000000,0xFFFFFF,1,1);

	splash_sprite(bitmap_hxc2001_logo_bmp,td->layers[LAYER_TEXT], td->xsize, td->ysize, td->xsize - (bitmap_hxc2001_logo_bmp->Xsize + 16), td->ysize - (bitmap_hxc2001_logo_bmp->Ysize + 16),LOGOALPHA);

	td->noloop_trackmode = 0;
}

void hxcfe_td_draw_stream_track( HXCFE_TD *td, HXCFE_FLOPPY * floppydisk, int32_t track, int32_t side )
{
	s_sectorlist * sl,*oldsl;
	s_pulseslist * pl,*oldpl;
	HXCFE_SIDE * currentside;
	char tmp_str[512];

	sl=td->sl;
	while(sl)
	{
		oldsl = sl->next_element;
		//sl = sl->next_element;

		hxcfe_freeSectorConfig( 0, sl->sectorconfig );

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

	if(side>=floppydisk->floppyNumberOfSide) side = floppydisk->floppyNumberOfSide - 1;
	if(side<0) side = 0;

	if(!floppydisk->floppyNumberOfTrack || !floppydisk->floppyNumberOfSide)
	{
		memset(td->layers[LAYER_SUPPORT],0xCC,td->xsize*td->ysize*sizeof(unsigned int));

		sprintf(tmp_str,"This track doesn't exist !");
		putstring8x8(td,LAYER_TEXT,(td->xsize/2) - ((strlen(tmp_str)*8)/2),td->ysize / 2,tmp_str,0x000000,0xCCCCCC,0,0);

		render(td);

		return;
	}

	currentside=floppydisk->tracks[track]->sides[side];

	if(currentside)
	{
		if(currentside->stream_dump)
		{
			hxcfe_td_draw_trkstream( td, currentside->stream_dump );

			if(td->name)
			{
				sprintf(tmp_str,"%s T:%.3d S:%.1d",td->name,track,side);
			}
			else
			{
				sprintf(tmp_str,"T:%.3d S:%.1d",track,side);
			}

			putstring8x8(td,LAYER_TEXT,td->xsize - (strlen(tmp_str)*8) ,1,tmp_str,0x000000,0xFFFFFF,0,1);
		}
		else
		{
			clear_layers( td );

			memset(td->layers[LAYER_SUPPORT],0xCC,td->xsize*td->ysize*sizeof(unsigned int));

			sprintf(tmp_str,"This track doesn't have any stream information !");
			putstring8x8(td,LAYER_TEXT,(td->xsize/2) - ((strlen(tmp_str)*8)/2),td->ysize / 2,tmp_str,0x000000,0xCCCCCC,0,1);

			sprintf(tmp_str,"libhxcfe v%s",STR_FILE_VERSION2);
			putstring8x8(td,LAYER_TEXT,td->xsize - (8 + 1),td->ysize - (8*3 + 1),tmp_str,0x000000,0xFFFFFF,1,1);

			splash_sprite(bitmap_hxc2001_logo_bmp,td->layers[LAYER_TEXT], td->xsize, td->ysize, td->xsize - (bitmap_hxc2001_logo_bmp->Xsize + 16), td->ysize - (bitmap_hxc2001_logo_bmp->Ysize + 16),LOGOALPHA);
		}
	}

	render(td);
}

int32_t hxcfe_td_setName( HXCFE_TD *td, char * name )
{
	if( td )
	{
		if(name)
		{
			if(td->name)
				free(td->name);

			td->name = malloc( strlen(name) + 1);
			if(td->name)
			{
				strcpy(td->name,name);
				return HXCFE_NOERROR;
			}

			return HXCFE_INTERNALERROR;
		}
	}

	return HXCFE_NOERROR;
}
s_sectorlist * hxcfe_td_getlastsectorlist(HXCFE_TD *td)
{
	return td->sl;
}

s_pulseslist * hxcfe_td_getlastpulselist(HXCFE_TD *td)
{
	return td->pl;
}

void draw_circle (HXCFE_TD *td,int layer, uint32_t col,uint8_t alpha_val,float start_angle,float stop_angle,int xpos,int ypos,int diametre,int op,int thickness,int counterclock)
{
	int x, y,i;
	int length;
	float angle = 0.0;
	float angle_stepsize = (float)0.001;

	length = diametre;

	if(op!=1) thickness++;

	i = 0;
	do
	{
		if(!counterclock)
		{
			angle = start_angle;
			// go through all angles from 0 to 2 * PI radians
			do //2 * 3.14159)
			{
				// calculate x, y from a vector with known length and angle
				x = (int)((length) * cos (angle));
				y = (int)((length) * sin (angle));

				plot(td, layer, x+xpos, -y+ypos  , col, alpha_val, op);

				angle += angle_stepsize;
			}while (angle < stop_angle );
		}
		else
		{
			angle = start_angle;
			// go through all angles from 0 to 2 * PI radians
			do //2 * 3.14159)
			{
				// calculate x, y from a vector with known length and angle
				x = (int)((length) * cos ((2*PI) - angle));
				y = (int)((length) * sin ((2*PI) - angle));

				plot(td, layer, x+xpos, -y+ypos  , col, alpha_val, op);

				angle += angle_stepsize;
			}while (angle < stop_angle );
		}

		length--;
		i++;
	}while(i<(thickness));
}

void draw_density_circle (HXCFE_TD *td, int layer, uint32_t col, uint8_t alpha_val, float start_angle,float stop_angle,int xpos,int ypos,int diametre,int op,int thickness,HXCFE_SIDE * side,int counterclock)
{
	int x, y,i, x_old,y_old;
	int length;
	int old_j,j,k;

	float timingoffset;
	float track_timing,timingoffset2;
	float angle = 0.0;
	float angle_stepsize = (float)0.001;
	int tracksize;
	int prev_offset;
	int bitcount;
	int totalcount;
	uint8_t lum,mask;

	length = diametre;

	if(op!=1) thickness++;

	track_timing = (float)getOffsetTiming(side,side->tracklen,0,0);

	tracksize = side->tracklen;

	i = 0;
	do
	{
		timingoffset = 0;
		old_j = 0;
		j = 0;
		prev_offset = 0;

		//bitcount
		angle = start_angle;
		if(!counterclock)
		{
			x_old = (int)((length) * cos (angle));
			y_old = (int)((length) * sin (angle));
		}
		else
		{
			x_old = (int)((length) * cos ((2*PI) - angle));
			y_old = (int)((length) * sin ((2*PI) - angle));
		}

		// go through all angles from 0 to 2 * PI radians
		do
		{
			// calculate x, y from a vector with known length and angle
			if(!counterclock)
			{
				x = (int)((length) * cos (angle));
				y = (int)((length) * sin (angle));
			}
			else
			{
				x = (int)((length) * cos ((2*PI) - angle));
				y = (int)((length) * sin ((2*PI) - angle));
			}


			if( (x_old != x) || (y_old != y) )
			{
				timingoffset2=( track_timing * ( angle / ( stop_angle - start_angle ) ) );

				while((j<tracksize) && timingoffset2>timingoffset)
				{
					timingoffset = getOffsetTiming(side,j,timingoffset,old_j);
					old_j=j;
					j++;
				};

				bitcount = 0;
				totalcount = 0;
				if(j - prev_offset)
				{

					mask = 0xFF >> (prev_offset&7);

					for(k=0;k<(j - prev_offset);k+=8)
					{
						bitcount = bitcount + LUT_Byte2HighBitsCount[(side->databuffer[(prev_offset+k)>>3])&mask];
						totalcount = totalcount + LUT_Byte2HighBitsCount[mask];
						if( (j - prev_offset) - k >= 8)
							mask = 0xFF;
						else
							mask = 0xFF << ( 8 - ( (j - prev_offset) - k ) ) ;
					}
				}

				lum = (uint8_t)((float)col * (float) ((float)bitcount/(float)totalcount) );

				plot(td, layer, x+xpos, -y+ypos  , (uint32_t)( (lum<<16) | ( (lum>>1) <<8) | (lum>>1)), alpha_val, op);

				prev_offset = old_j;
			}

			angle += angle_stepsize;

			x_old = x;
			y_old = y;

		}while (angle < stop_angle );

		length--;
		i++;
	}while(i<(thickness));
}

s_sectorlist * display_sectors_disk(HXCFE_TD *td,HXCFE_FLOPPY * floppydisk,int track,int side,float timingoffset_offset, int TRACKTYPE,int xpos,int ypos,int diam,int thickness,int mirror)
{
	int tracksize;
	int old_i;
	float track_timing;
	float startsector_timingoffset;
	float datasector_timingoffset;
	float endsector_timingoffset;
	float timingoffset;
	int color;
	int loop;
	HXCFE_SECTORACCESS* ss;
	HXCFE_SECTCFG* sc;
	HXCFE_SIDE * currentside;
	s_sectorlist * sl,*oldsl;
	uint32_t color_valid_sector_data,color_bad_sector_data,color_valid_empty_sector_data;
	uint32_t color_headerless_sector_data;
	uint32_t color_valid_sector_header,color_bad_sector_header;
	uint32_t color_sector_boundary,color_sector_data_boundary;
	int display_boundary;
	int color_sector_alpha;

	old_i=0;
	loop=0;

	sl=td->sl;
	oldsl=0;

	currentside=floppydisk->tracks[track]->sides[side];
	tracksize=currentside->tracklen;

	startsector_timingoffset = 0;

	old_i=0;
	timingoffset = 0;

	color_valid_sector_data = hxcfe_getEnvVarValue( td->hxcfe, "BMPDISKEXPORT_COLOR_VALID_SECTOR_DATA" );
	color_valid_empty_sector_data = hxcfe_getEnvVarValue( td->hxcfe, "BMPDISKEXPORT_COLOR_VALID_EMPTY_SECTOR_DATA" );
	color_bad_sector_data = hxcfe_getEnvVarValue( td->hxcfe, "BMPDISKEXPORT_COLOR_BAD_SECTOR_DATA" );
	color_headerless_sector_data = hxcfe_getEnvVarValue( td->hxcfe, "BMPDISKEXPORT_COLOR_HEADERLESS_SECTOR_DATA" );
	color_sector_alpha = hxcfe_getEnvVarValue( td->hxcfe, "BMPDISKEXPORT_COLOR_ALPHA" );

	color_valid_sector_header = hxcfe_getEnvVarValue( td->hxcfe, "BMPDISKEXPORT_COLOR_VALID_SECTOR_HEADER" );
	color_bad_sector_header = hxcfe_getEnvVarValue( td->hxcfe, "BMPDISKEXPORT_COLOR_BAD_SECTOR_HEADER" );

	color_sector_boundary = hxcfe_getEnvVarValue( td->hxcfe, "BMPDISKEXPORT_COLOR_SECTOR_BOUNDARY" );
	color_sector_data_boundary = hxcfe_getEnvVarValue( td->hxcfe, "BMPDISKEXPORT_COLOR_SECTOR_DATA_BOUNDARY" );

	display_boundary = hxcfe_getEnvVarValue( td->hxcfe, "BMPDISKEXPORT_DISPLAY_SECTOR_BOUNDARY" );

	track_timing = (float)getOffsetTiming(currentside,tracksize,timingoffset,old_i);

	ss=hxcfe_initSectorAccess(td->hxcfe,floppydisk);
	if(ss)
	{
		do
		{
			sc = hxcfe_getNextSector(ss,track,side,TRACKTYPE);
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
						color = color_bad_sector_header;
					}
					else
					{
						color = color_valid_sector_header;
					}

					draw_circle (td,LAYER_IDENTIFICATION,color,color_sector_alpha, (float)((float)2 * PI)*(startsector_timingoffset/track_timing),(float)((float)2 * PI)*(datasector_timingoffset/track_timing),xpos,ypos,diam,0,thickness,mirror);

					///////////////////////////////////////
					// Data Block
					///////////////////////////////////////
					if(sc->startdataindex == sc->startsectorindex)
					{
						// Header less sector -> Blue
						color = color_headerless_sector_data;
					}
					else
					{
						if(sc->use_alternate_data_crc)
						{
							// CRC error -> Red
							color = color_bad_sector_data;
						}
						else
						{
							if(!sc->fill_byte_used)
							{
								// Sector ok -> Green
								color = color_valid_sector_data;
							}
							else
							{
								// Ok and "empty" (same value on all bytes)
								color = color_valid_empty_sector_data;
							}
						}
					}

					draw_circle (td,LAYER_IDENTIFICATION,color,color_sector_alpha, (float)((float)2 * PI)*(datasector_timingoffset/track_timing),(float)((float)2 * PI)*(endsector_timingoffset/track_timing),xpos,ypos,diam,0,thickness,mirror);

					// Sector limits...
					if( display_boundary )
					{
						// Left Line
						draw_circle (td,LAYER_IDENTIFICATION,color_sector_boundary,color_sector_alpha,(float)((float)2 * PI)*(startsector_timingoffset/track_timing),(float)((float)2 * PI)*(startsector_timingoffset/track_timing),xpos,ypos,diam,0,thickness,mirror);

						// Data Line
						draw_circle (td,LAYER_IDENTIFICATION,color_sector_data_boundary,color_sector_alpha,(float)((float)2 * PI)*(datasector_timingoffset/track_timing),(float)((float)2 * PI)*(datasector_timingoffset/track_timing),xpos,ypos,diam,0,thickness,mirror);

						// Right Line
						draw_circle (td,LAYER_IDENTIFICATION,color_sector_boundary,color_sector_alpha,(float)((float)2 * PI)*(endsector_timingoffset/track_timing),(float)((float)2 * PI)*(endsector_timingoffset/track_timing),xpos,ypos,diam,0,thickness,mirror);
					}

					if(!mirror)
					{
						sl->start_angle = (float)((float)2 * PI)*(startsector_timingoffset/track_timing);
						sl->end_angle = (float)((float)2 * PI)*(endsector_timingoffset/track_timing);
					}
					else
					{
						sl->end_angle = (float)(2 * PI)-((float)2 * PI)*(startsector_timingoffset/track_timing);
						sl->start_angle = (float)(2 * PI)-((float)2 * PI)*(endsector_timingoffset/track_timing);
					}

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

int countSector(s_sectorlist * sl,int side)
{
	int cnt;

	cnt = 0;

	while(sl)
	{
		if(sl->side == side)
		{
			if(sl->sectorconfig)
			{
				cnt++;
			}
		}
		sl = sl->next_element;
	}

	return cnt;
}

int countSize(s_sectorlist * sl,int side)
{
	int size;

	size = 0;

	while(sl)
	{
		if(sl->side == side)
		{
			if(sl->sectorconfig)
			{
				size = size + sl->sectorconfig->sectorsize;
			}
		}
		sl = sl->next_element;
	}

	return size;
}

int countBadSectors(s_sectorlist * sl,int side)
{
	int cnt;

	cnt = 0;

	while(sl)
	{
		if(sl->side == side)
		{
			if(sl->sectorconfig)
			{
				if(!sl->sectorconfig->trackencoding || sl->sectorconfig->use_alternate_data_crc || !sl->sectorconfig->input_data)
				{
					cnt++;
				}
			}
		}
		sl = sl->next_element;
	}

	return cnt;
}

int countTrackType(s_sectorlist * sl,int side,int type)
{
	int cnt;

	cnt = 0;

	while(sl)
	{
		if(sl->side == side)
		{
			if(sl->sectorconfig)
			{
				if(sl->sectorconfig->trackencoding == type)
				{
					cnt++;
				}
			}
		}
		sl = sl->next_element;
	}

	return cnt;
}

typedef struct type_list_
{
	int track_type;
	char *name;
}type_list;


const static type_list track_type_list[]=
{
	{ISOFORMAT_SD,      "ISO FM"},
	{ISOFORMAT_DD,      "ISO MFM"},
	{AMIGAFORMAT_DD,    "Amiga MFM"},
	{TYCOMFORMAT_SD,    "TYFM"},
	{MEMBRAINFORMAT_DD, "MEMBRAIN"},
	{EMUFORMAT_SD,      "E-mu"},
	{APPLEMAC_GCR6A2,   "Apple Macintosh"},
	{APPLE2_GCR5A3,     "Apple II 5A3"},
	{APPLE2_GCR6A2,     "Apple II 6A2"},
	{ARBURG_DAT,        "Arburg DATA"},
	{ARBURG_SYS,        "Arburg SYSTEM"},
	{AED6200P_DD,       "AED 6200P"},
	{QD_MO5_MFM,        "QD MO5"},
	{C64_GCR,           "C64 GCR"},
	{VICTOR9K_GCR,      "Victor 9000 GCR"},
	{DECRX02_SDDD ,     "DEC RX02"},
	{MICRALN_HS_SD,     "R2E Micral N"},
	{NORTHSTAR_HS_DD,   "NorthStar"},
	{HEATHKIT_HS_SD,    "Heathkit"},
	{0,0}
};

typedef struct physical_floppy_dim_
{
	char * type_name;
	int total_radius_um;
	int hole_radius_um;
	int center_to_tracks_radius_um[2];
	int tracks_space_radius_um[2];
	int track_witdh_um[2];
	int track_spacing_um[2];
	int window[4];
	int index_type;
}physical_floppy_dim;

// 3.5" Floppy disk
// 135 tpi -> 135 per 2.54cm -> 53,149606299 track per cm -> 84 tracks = 84/53,149606299 = 1,580444444 cm
// 84 tracks : 15554

// 5"1/4 Floppy disk
// 48 tpi -> 48 per 2.54cm -> 18,897637795 track per cm -> 40 tracks = 40/18,897637795 = 2,116666667 cm
// 40 tracks : 21166

// 8" Floppy disk
// 48 tpi -> 48 per 2.54cm -> 18,897637795 track per cm -> 77 tracks = 77/18,897637795 = 4,074583333 cm
// 77 tracks : 40746

physical_floppy_dim floppy_dimension[]=
{   //                                 total_radius_um  hole_radius_um  center_to_tracks_radius_um  tracks_space_radius_um   track_witdh_um      track_spacing_um            window                  Index type
	{	"Track view",                      50000,          13000,         { 15000, 15000 },           { 35000, 35000 },       { 300, 300 },          { 0, 0 },      { -1,     0,    0,     0 }        , 0},
	{	"Stream view",                     50000,          13000,         { 15000, 15000 },           { 35000, 35000 },       { 300, 300 },          { 0, 0 },      { -1,     0,    0,     0 }        , 0},
	{	"Stream view (Fat pixels)",        50000,          13000,         { 15000, 15000 },           { 35000, 35000 },       { 300, 300 },          { 0, 0 },      { -1,     0,    0,     0 }        , 0},
	{	"Dummy disk",                      50000,          13000,         { 15000, 15000 },           { 35000, 35000 },       { 300, 300 },          { 0, 0 },      { -1,     0,    0,     0 }        , 0},
	{	"3\"1/2 135 TPI",                  43000,          12000,         { 23946, 22446 },           { 15754, 15754 },       { 115, 115 },          { 187, 187 },  { 19000, -4500, 42500, 4500 }     , 1},
	{	"5\"1/4 96 TPI",                   65000,          14500,         { 35000, 35000 },           { 22224, 22224 },       { 160, 160 },          { 264, 264 },  { 33000, -6500, 60000, 6500 }     , 2},
	{	"5\"1/4 48 TPI",                   65000,          14500,         { 34500, 34500 },           { 22724, 22724 },       { 330, 330 },          { 529, 529 },  { 33000, -6500, 60000, 6500 }     , 2},
	{	"8\" 48 TPI",                      99000,          19250,         { 54000, 54000 },           { 41576, 41576 },       { 330, 330 },          { 529, 529 },  { 50000, -6500, 98000, 6500 }     , 3},
	{	NULL,                              43000,          12000,         { 21000, 21000 },           { 15805, 15805 },       { 300, 300 },          { 420, 420 },  { 50000, -6500, 98000, 6500 }     , 1}
};

void hxcfe_td_select_view_type( HXCFE_TD *td, int32_t disk_type)
{
	if(td)
	{
		td->disk_type = disk_type;
	}
}

char * hxcfe_td_get_view_mode_name( HXCFE_TD *td, int32_t disk_type)
{
	int i;
	if(td)
	{
		i = 0;
		while(i<disk_type && floppy_dimension[i].type_name)
		{
			i++;
		}
		return floppy_dimension[i].type_name;
	}

	return NULL;
}

int um2pix(int totalpix, int size_um, int um )
{
	return (int)( (float)totalpix * ((float)um / (float)size_um) );
}

void hxcfe_draw_side(HXCFE_TD *td,HXCFE_FLOPPY * floppydisk, int x_center, int y_center, int side)
{
	int tracksize;
	int i;
	int track;
	HXCFE_SIDE * currentside;
	unsigned int color;
	int y_pos,x_pos_1;
	int total_radius,center_hole_radius;
	int start_tracks_space_radius;
	//int end_tracks_space_radius;
	int tracks_space_radius;
	float track_ep;
	int xpos;
	int max_physical_tracks;
	char tempstr[512];
	int physical_dim, doesntfit;
	int alpha;

	alpha = hxcfe_getEnvVarValue( td->hxcfe, "BMPDISKEXPORT_COLOR_ALPHA" );
	physical_dim = td->disk_type;
	doesntfit = 0;

	y_pos = y_center;
	x_pos_1 = x_center;

	if( td->virtual_ysize < (td->virtual_xsize / 2) )
	{
		total_radius = td->virtual_ysize - (td->virtual_ysize /2);
	}
	else
	{
		total_radius = (td->virtual_xsize/2) - (td->virtual_xsize / 4);
	}

	center_hole_radius = (int)((float)total_radius * ((float)floppy_dimension[physical_dim].hole_radius_um / \
	                                                  (float)floppy_dimension[physical_dim].total_radius_um));

	for(i=0;i<2;i++)
	{
		//end_tracks_space_radius = (int)((float)total_radius * ((float)floppy_dimension[physical_dim].center_to_tracks_radius_um[i] /
		//                             (float)floppy_dimension[physical_dim].total_radius_um));

		start_tracks_space_radius = (int)((float)total_radius * ((float)(floppy_dimension[physical_dim].center_to_tracks_radius_um[side] + floppy_dimension[physical_dim].tracks_space_radius_um[side]) / \
		                                     (float)floppy_dimension[physical_dim].total_radius_um));

		tracks_space_radius = (int)((float)total_radius * ((float)floppy_dimension[physical_dim].tracks_space_radius_um[side] / \
		                                                      (float)floppy_dimension[physical_dim].total_radius_um));

		if( floppy_dimension[physical_dim].track_spacing_um[side] > 0 )
		{
			track_ep = ((float)total_radius * ((float)floppy_dimension[physical_dim].track_spacing_um[side] / \
		                                          (float)floppy_dimension[physical_dim].total_radius_um));
		}
		else
		{
			track_ep = 1;
			if(floppydisk->floppyNumberOfTrack)
			{
				track_ep = ((float)tracks_space_radius / (float)floppydisk->floppyNumberOfTrack);
			};
		}

		max_physical_tracks = floppydisk->floppyNumberOfTrack;
		if((int)(max_physical_tracks * track_ep) > tracks_space_radius)
		{
			max_physical_tracks = (int)(tracks_space_radius / track_ep);
			doesntfit = 1;
		}
	}

	color = hxcfe_getEnvVarValue( td->hxcfe, "BMPDISKEXPORT_COLOR_MEDIA_SUBSTRATE" );;
	for(i=center_hole_radius;i<total_radius;i++)
	{
		circle(td,LAYER_SUPPORT,x_pos_1,y_pos,i,color,255);
	}

	if( !side )
	{
		putstring8x8(td,LAYER_SUPPORT,(x_pos_1 - td->window_xpos) + 40,(y_pos - td->window_ypos) - 3, "---",0xCCCCCC,0x000000,0,1);		
		putstring8x8(td,LAYER_SUPPORT,(x_pos_1 - td->window_xpos) - 24,(y_pos - td->window_ypos) + 8, "Side 0",0x666666,0x000000,0,1);
		putstring8x8(td,LAYER_SUPPORT,(x_pos_1 - td->window_xpos) - 40,(y_pos - td->window_ypos) + 16,"Bottom side",0x666666,0x000000,0,1);
		putstring8x8(td,LAYER_SUPPORT,(x_pos_1 - td->window_xpos) - 40,(y_pos - td->window_ypos) + 24,"Bottom view",0x666666,0x000000,0,1);
		putstring8x8(td,LAYER_SUPPORT,(x_pos_1 - td->window_xpos) - 4,(y_pos - td->window_ypos) - 44, "<-",0x666666,0x000000,0,1);
	}
	else
	{
		putstring8x8(td,LAYER_SUPPORT,(x_pos_1 - td->window_xpos) + 40,(y_pos - td->window_ypos) - 3, "---",0xCCCCCC,0x000000,0,1);
		putstring8x8(td,LAYER_SUPPORT,(x_pos_1 - td->window_xpos) - 24,(y_pos - td->window_ypos) + 8, "Side 1",0x666666,0x000000,0,1);
		putstring8x8(td,LAYER_SUPPORT,(x_pos_1 - td->window_xpos) - 32,(y_pos - td->window_ypos) + 16,"Top side",0x666666,0x000000,0,1);
		putstring8x8(td,LAYER_SUPPORT,(x_pos_1 - td->window_xpos) - 32,(y_pos - td->window_ypos) + 24,"Top view",0x666666,0x000000,0,1);
		putstring8x8(td,LAYER_SUPPORT,(x_pos_1 - td->window_xpos) - 4,(y_pos - td->window_ypos) - 44, "->",0x666666,0x000000,0,1);
	}

	switch(floppy_dimension[physical_dim].index_type)
	{
		case 0:
		break;
		case 1:
			disc(td,LAYER_SUPPORT,x_pos_1 + um2pix(total_radius, floppy_dimension[physical_dim].total_radius_um, 10000 ), \
			          y_pos, \
			          um2pix(total_radius, floppy_dimension[physical_dim].total_radius_um, 1250 ), \
			          0x00000000,0x808080, 255);
		break;
		case 2:
			disc(td,LAYER_SUPPORT,x_pos_1 + um2pix(total_radius, floppy_dimension[physical_dim].total_radius_um, 7000 ), \
			          y_pos + um2pix(total_radius, floppy_dimension[physical_dim].total_radius_um, 24000 ),
			          um2pix(total_radius, floppy_dimension[physical_dim].total_radius_um, 1000 ),
			          0x00000000,0x808080, 255);
		break;
		case 3:
			disc(td,LAYER_SUPPORT,x_pos_1 - um2pix(total_radius, floppy_dimension[physical_dim].total_radius_um, 39000 ), \
			          y_pos + um2pix(total_radius, floppy_dimension[physical_dim].total_radius_um,  4000 ),
			          um2pix(total_radius, floppy_dimension[physical_dim].total_radius_um, 2000 ),
			          0x00000000,0x808080, 255);
		break;
	}

	if(doesntfit)
	{
		sprintf(tempstr,"This image doesn't fit on a %s disk !",floppy_dimension[physical_dim].type_name);
		putstring8x8(td,LAYER_TEXT,(td->xsize/2) - ((strlen(tempstr)*8)/2),td->ysize / 2,tempstr,0x0000FF,0x000000,0,0);

		render(td);

		return;
	}

	render(td);

	for(track=0;track<max_physical_tracks;track++)
	{
		td->hxc_setprogress(track*floppydisk->floppyNumberOfSide,floppydisk->floppyNumberOfTrack*floppydisk->floppyNumberOfSide*2,td,td->progress_userdata);

		currentside = floppydisk->tracks[track]->sides[side];
		draw_density_circle (td,LAYER_SUPPORT,0x0000FF,0xFF,0,(float)((float)((float)2 * PI)),x_pos_1,y_pos,start_tracks_space_radius - (int)((float)track * track_ep),0,(int)track_ep,currentside,(side & 1)^1);

		if( !(track % 10) )
			render(td);
	}

	render(td);

	for(track=0;track<max_physical_tracks;track++)
	{
		td->hxc_setprogress((floppydisk->floppyNumberOfTrack*floppydisk->floppyNumberOfSide) + track * floppydisk->floppyNumberOfSide ,floppydisk->floppyNumberOfTrack*floppydisk->floppyNumberOfSide*2,td,td->progress_userdata);

		currentside = floppydisk->tracks[track]->sides[side];

		tracksize = currentside->tracklen;

		//////////////////////////////////////////
		// Sector drawing
		for(i=0;i<32;i++)
		{
			if(td->enabledtrackmode & (0x00000001<<i) )
			{
				display_sectors_disk(td,floppydisk,track,side,0,i,x_pos_1,y_pos,start_tracks_space_radius - (int)(track * track_ep),(int)track_ep, (side & 1)^1);
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
						draw_circle (td,LAYER_IDENTIFICATION,0x0000FF,alpha,(float)((float)((float)2 * PI)*((float)xpos/(float)2048)),(float)((float)((float)2 * PI)*((float)xpos/(float)2048)),x_pos_1,y_pos,start_tracks_space_radius - (int)((track * track_ep)),0,(int)track_ep, (side & 1)^1);
					}
				}

				i++;
			}while(i<tracksize);
		}

		if( !(track % 4) )
		{
			render(td);
		}
	}

	render(td);

	for(track=0;track<max_physical_tracks+1;track++)
	{
//		draw_circle (td,LAYER_IDENTIFICATION,0xF8F8F8,alpha,0,(float)((float)((float)2 * PI)),x_pos_1,y_pos,start_tracks_space_radius - (int)((track * track_ep)) + 1,1,(int)0,1);
	}

	draw_circle (td,LAYER_IDENTIFICATION,0xEFEFEF,alpha,0,(float)((float)((float)2 * PI)),x_pos_1,y_pos,start_tracks_space_radius - (int)(((max_physical_tracks+1) * track_ep)) + 1,1,(int)0,1);

	if(floppy_dimension[physical_dim].window[0] >= 0)
	{
		box(td,LAYER_IDENTIFICATION,um2pix(total_radius, floppy_dimension[physical_dim].total_radius_um, floppy_dimension[physical_dim].window[0] ) + x_pos_1, \
			   um2pix(total_radius, floppy_dimension[physical_dim].total_radius_um, floppy_dimension[physical_dim].window[1] ) + y_pos, \
			   um2pix(total_radius, floppy_dimension[physical_dim].total_radius_um, floppy_dimension[physical_dim].window[2] ) + x_pos_1, \
			   um2pix(total_radius, floppy_dimension[physical_dim].total_radius_um, floppy_dimension[physical_dim].window[3] ) + y_pos, \
			   0x909090, 255, 1 );
	}

	td->hxc_setprogress(floppydisk->floppyNumberOfTrack*floppydisk->floppyNumberOfSide,floppydisk->floppyNumberOfTrack*floppydisk->floppyNumberOfSide,td,td->progress_userdata);

	render(td);
}

void hxcfe_td_draw_disk(HXCFE_TD *td,HXCFE_FLOPPY * floppydisk)
{
	int i;
	uint32_t crc32;
	int y_pos,x_pos_1,x_pos_2,ytypepos;
	int total_radius;
	//int end_tracks_space_radius[2];
	int tracks_space_radius[2];
	float track_ep[2];
	int max_physical_tracks[2];
	char tempstr[512];
	s_sectorlist * sl,*oldsl;
	int physical_dim, doesntfit;

	physical_dim = td->disk_type;
	doesntfit = 0;

	sl=td->sl;
	while(sl)
	{
		oldsl = sl->next_element;

		hxcfe_freeSectorConfig( 0, sl->sectorconfig );

		free(sl);

		sl = oldsl;
	}
	td->sl=0;

	clear_layers( td );

	splash_sprite(bitmap_hxc2001_logo_bmp,td->layers[LAYER_TEXT], td->xsize, td->ysize, td->xsize - (bitmap_hxc2001_logo_bmp->Xsize + 16), td->ysize - (bitmap_hxc2001_logo_bmp->Ysize + 16),LOGOALPHA);

	sprintf(tempstr,"libhxcfe v%s",STR_FILE_VERSION2);
	putstring8x8(td,LAYER_TEXT,td->xsize - (8 + 1),td->ysize - (8*3 + 1),tempstr,0xAAAAAA,0x000000,1,1);

	sprintf(tempstr,"%s",floppy_dimension[physical_dim].type_name);
	putstring8x8(td,LAYER_TEXT,td->xsize - (16*8),td->ysize - (8 + 1),tempstr,0xAAAAAA,0x000000,0,1);

	if(td->name)
	{
		putstring8x8( td, LAYER_TEXT, td->xsize - ((strlen(td->name)*8) + 8),1,td->name,0xAAAAAA,0x000000,0,1);
		putstring8x8( td, LAYER_TEXT, (td->xsize/2) - ( (strlen(td->name)*8) + 8),1,td->name,0xAAAAAA,0x000000,0,1);
	}

	sprintf(tempstr,"Side %d, %d Tracks",0,floppydisk->floppyNumberOfTrack);
	putstring8x8(td,LAYER_TEXT,1,1,tempstr,0xAAAAAA,0x000000,0,1);

	if(floppydisk->floppyNumberOfSide == 2)
	{
		sprintf(tempstr,"Side %d, %d Tracks",1,floppydisk->floppyNumberOfTrack);
		putstring8x8(td,LAYER_TEXT,td->xsize/2,1,tempstr,0xAAAAAA,0x000000,0,1);
	}

	y_pos = td->virtual_ysize/2;
	x_pos_1 = td->virtual_xsize/4;
	x_pos_2 = td->virtual_xsize - (td->virtual_xsize/4);

	if( td->virtual_ysize < (td->virtual_xsize / 2) )
	{
		total_radius = td->virtual_ysize - (td->virtual_ysize /2);
	}
	else
	{
		total_radius = (td->virtual_xsize/2) - (td->virtual_xsize / 4);
	}

	for(i=0;i<2;i++)
	{
		//end_tracks_space_radius[i] = (int)((float)total_radius * ((float)floppy_dimension[physical_dim].center_to_tracks_radius_um[i] /
		//                             (float)floppy_dimension[physical_dim].total_radius_um));

		tracks_space_radius[i] = (int)((float)total_radius * ((float)floppy_dimension[physical_dim].tracks_space_radius_um[i] / \
		                                                      (float)floppy_dimension[physical_dim].total_radius_um));

		if( floppy_dimension[physical_dim].track_spacing_um[i] > 0 )
		{
			track_ep[i] = ((float)total_radius * ((float)floppy_dimension[physical_dim].track_spacing_um[i] / \
		                                          (float)floppy_dimension[physical_dim].total_radius_um));
		}
		else
		{
			track_ep[i] = 1;
			if(floppydisk->floppyNumberOfTrack)
			{
				track_ep[i] = ((float)tracks_space_radius[i] / (float)floppydisk->floppyNumberOfTrack);
			};
		}

		max_physical_tracks[i] = floppydisk->floppyNumberOfTrack;
		if((int)(max_physical_tracks[i] * track_ep[i]) > tracks_space_radius[i])
		{
			max_physical_tracks[i] = (int)(tracks_space_radius[i] / track_ep[i]);
			doesntfit = 1;
		}
	}

	if(doesntfit)
	{
		sprintf(tempstr,"This image doesn't fit on a %s disk !",floppy_dimension[physical_dim].type_name);
		putstring8x8(td,LAYER_TEXT,(td->xsize/2) - ((strlen(tempstr)*8)/2),td->ysize / 2,tempstr,0x0000FF,0x000000,0,0);

		render(td);

		return;
	}

	render(td);

	td->hxc_setprogress(floppydisk->floppyNumberOfTrack*floppydisk->floppyNumberOfSide,floppydisk->floppyNumberOfTrack*floppydisk->floppyNumberOfSide,td,td->progress_userdata);

	if(floppydisk->floppyNumberOfSide > 0)
	{
		if( td->window_xpos < td->virtual_xsize / 2 )
			hxcfe_draw_side(td, floppydisk, x_pos_1, y_pos, 0);
	}

	sprintf(tempstr,"%d Sectors,%d bad", countSector(td->sl,0),countBadSectors(td->sl,0));
	putstring8x8(td,LAYER_TEXT,1,11,tempstr,0xAAAAAA,0x000000,0,1);

	sprintf(tempstr,"%d Bytes", countSize(td->sl,0));
	putstring8x8(td,LAYER_TEXT,1,21,tempstr,0xAAAAAA,0x000000,0,1);

	sprintf(tempstr,"CRC32: --------");
	putstring8x8(td,LAYER_TEXT,td->xsize/2 - (8*18),td->ysize - 9,tempstr,0xAAAAAA,0x000000,0,1);

	ytypepos = 31;
	i = 0;
	while(track_type_list[i].name)
	{
		if(countTrackType(td->sl,0,track_type_list[i].track_type))
		{
			putstring8x8(td,LAYER_TEXT,1,ytypepos,"             ",0xAAAAAA,0x000000,0,1);
			putstring8x8(td,LAYER_TEXT,1,ytypepos,track_type_list[i].name,0xAAAAAA,0x000000,0,1);
			ytypepos += 10;
		}
		i++;
	}

	if( hxcfe_floppyGetFlags( td->hxcfe, floppydisk ) & HXCFE_FLOPPY_WRPROTECTED_FLAG )
		putstring8x8(td,LAYER_TEXT,1,ytypepos,"WrProt ON ",0xAAAAAA,0x000000,0,1);
	else
		putstring8x8(td,LAYER_TEXT,1,ytypepos,"WrProt OFF",0xAAAAAA,0x000000,0,1);

	render(td);

	if(floppydisk->floppyNumberOfSide > 1)
	{
		if( td->window_xpos + td->xsize >= td->virtual_xsize / 2 )
			hxcfe_draw_side(td, floppydisk, x_pos_2, y_pos, 1);
	}
	sprintf(tempstr,"%d Sectors,%d bad", countSector(td->sl,1),countBadSectors(td->sl,1));
	putstring8x8(td,LAYER_TEXT,(td->xsize/2)+1,11,tempstr,0xAAAAAA,0x000000,0,1);

	sprintf(tempstr,"%d Bytes", countSize(td->sl,1));
	putstring8x8(td,LAYER_TEXT,(td->xsize/2)+1,21,tempstr,0xAAAAAA,0x000000,0,1);

	ytypepos = 31;
	i = 0;
	while(track_type_list[i].name)
	{
		if(countTrackType(td->sl,1,track_type_list[i].track_type))
		{
			putstring8x8(td,LAYER_TEXT, (td->xsize/2)+1,ytypepos,"             ",0xAAAAAA,0x000000,0,1);
			putstring8x8(td,LAYER_TEXT, (td->xsize/2)+1,ytypepos,track_type_list[i].name,0xAAAAAA,0x000000,0,1);
			ytypepos += 10;
		}
		i++;
	}

	render(td);

	sprintf(tempstr,"CRC32: Processing");
	putstring8x8(td,LAYER_TEXT,td->xsize/2 - (8*18),td->ysize - 9,tempstr,0xAAAAAA,0x000000,0,1);

	render(td);

	hxcfe_getHash( td->hxcfe, floppydisk, -1, -1, -1, td->enabledtrackmode, HASH_TYPE_CRC32, HASH_FLAG_DATA | HASH_FLAG_PHYSICALORDER, (void*)&crc32, sizeof(crc32) );

	sprintf(tempstr,"CRC32: 0x%.8X",crc32);
	putstring8x8(td,LAYER_TEXT,td->xsize/2 - (8*18),td->ysize - 9,tempstr,0xAAAAAA,0x000000,0,1);

	render(td);

}

void * hxcfe_td_getframebuffer(HXCFE_TD *td)
{
	if(td)
	{
		return (void*)td->framebuffer;
	}

	return 0;
}

int32_t hxcfe_td_getframebuffer_xres( HXCFE_TD *td )
{
	if(td)
	{
		return td->xsize;
	}

	return 0;

}

int32_t hxcfe_td_getframebuffer_yres( HXCFE_TD *td )
{
	if(td)
	{
		return td->ysize;
	}

	return 0;
}

int32_t hxcfe_td_exportToBMP( HXCFE_TD *td, char * filename )
{
	int i,j,k;
	uint32_t * ptr;
	unsigned char * ptrchar;
	bitmap_data bdata;

	uint32_t pal[256];
	int nbcol;

	ptrchar = 0;

	ptr = malloc( td->xsize * td->ysize * sizeof(uint32_t) );
	if(!ptr)
		goto alloc_error;

	memset(ptr,0,(td->xsize * td->ysize * sizeof(uint32_t)));

	ptrchar = malloc((td->xsize*td->ysize));
	if(!ptrchar)
		goto alloc_error;

	td->hxcfe->hxc_printf(MSG_INFO_1,"Converting image...");
	nbcol = 0;

	k=0;
	for(i=0;i< ( td->ysize );i++)
	{
		for(j=0;j< (td->xsize );j++)
		{
			ptrchar[k] = getPixelCode(td->framebuffer[k],(uint32_t*)&pal,&nbcol);
			k++;
		}
	}

	if(nbcol>=256)
	{
		k = 0;
		for(i=0;i< ( td->ysize );i++)
		{
			for(j=0;j< ( td->xsize );j++)
			{
				ptrchar[k] = ptrchar[k] & 0xF8F8F8;
				k++;
			}
		}

		for(i=0;i<256;i++)
		{
			pal[i]=i|(i<<8)|(i<<16);
		}

		nbcol = 0;
		k=0;
		for(i=0;i< ( td->ysize );i++)
		{
			for(j=0;j< ( td->xsize );j++)
			{
				ptrchar[k] = getPixelCode(td->framebuffer[k],(uint32_t*)&pal,&nbcol);
				k++;
			}
		}
	}

	td->hxcfe->hxc_printf(MSG_INFO_1,"Writing %s...",filename);

	if(nbcol>=256)
	{
		bdata.nb_color = 16;
		bdata.xsize = td->xsize;
		bdata.ysize = td->ysize;
		bdata.data = (uint32_t*)ptr;
		bdata.palette = 0;
		bmp16b_write(filename,&bdata);
	}
	else
	{
		bdata.nb_color = 8;
		bdata.xsize = td->xsize;
		bdata.ysize = td->ysize;
		bdata.data = (uint32_t*)ptrchar;
		bdata.palette = (unsigned char*)&pal;

		bmpRLE8b_write(filename,&bdata);
	}

	free(ptr);
	free(ptrchar);

	return 0;

alloc_error:

	if(ptr)
		free(ptr);

	if(ptrchar)
		free(ptrchar);

	return -1;
}

void hxcfe_td_deinit(HXCFE_TD *td)
{
	int i;

	free(td->framebuffer);

	i = 0;
	while( i < 3 )
	{
		free(td->layers[i]);
		i++;
	}

	free(td->name);

	free(td);
}

void hxcfe_td_set_marker( HXCFE_TD *td, int32_t cell_pos, int32_t marker_id, uint32_t type, uint32_t color, uint32_t flags )
{
	if(!td)
		return;

	if( marker_id < 0 || marker_id > MAX_MARKER - 1)
		return;

	td->markers[marker_id].cell_pos = cell_pos;
	td->markers[marker_id].type = type;
	td->markers[marker_id].flags = flags;
	td->markers[marker_id].color = color;
}
