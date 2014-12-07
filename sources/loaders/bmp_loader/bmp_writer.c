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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "types.h"

#include "internal_libhxcfe.h"
#include "libhxcfe.h"

#include "bmp_loader.h"

#include "libhxcadaptor.h"

#include "bmp_file.h"

static int progress_callback(unsigned int current,unsigned int total,void * td,void * user)
{
	return hxcfe_imgCallProgressCallback((HXCFE_IMGLDR*)user,current,total);
}

void copyPict(uint32_t * dest,int d_xsize,int d_ysize,int d_xpos,int d_ypos,uint32_t * src,int s_xsize,int s_ysize)
{
	int j;
	uint32_t * ptr_line_src;
	uint32_t * ptr_line_dst;

	for(j=0;j<s_ysize;j++)
	{
		ptr_line_src = &src[(s_xsize * j)];
		ptr_line_dst = &dest[(d_xsize * (d_ypos + j)) + d_xpos];

		memcpy(ptr_line_dst,ptr_line_src,s_xsize*4);
	}
}

void vLine(uint32_t * dest,uint32_t d_xsize,uint32_t d_ysize,uint32_t ypos)
{
	uint32_t j;
	uint32_t * ptr_line_dst;

	for(j=0;j<d_ysize;j++)
	{
		if( ((d_xsize * j) + ypos+4) < (d_xsize*d_ysize))
		{
			ptr_line_dst = &dest[(d_xsize * j) + ypos];

			*(ptr_line_dst) = 0x000000;
			*(ptr_line_dst+1) = 0x000000;
			*(ptr_line_dst+2) = 0x000000;
			*(ptr_line_dst+3) = 0x000000;
		}
	}
}

void hLine(uint32_t * dest,uint32_t d_xsize,uint32_t d_ysize,uint32_t ypos)
{
	uint32_t * ptr_line_dst;

	ptr_line_dst = &dest[ d_xsize * ypos ];

	if((d_xsize * ypos)<(d_xsize*d_ysize))
	{
		memset(ptr_line_dst,0,d_xsize*4);
	}
}

unsigned char getPixelCode(uint32_t pix,uint32_t * pal,int * nbcol)
{
	int i;

	for(i=0;i<*nbcol;i++)
	{
		if( pix == pal[i] )
		{
			return i;
		}
	}

	if(i==*nbcol && i<256)
	{
		if(*nbcol<256 && *nbcol>=0)
		{
			pal[*nbcol] = pix;

			*nbcol = (*nbcol+1);

			return *nbcol-1;
		}
	}

	return 0;
}

int BMP_libWrite_DiskFile(HXCFE_IMGLDR* imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * filename)
{
	int ret,i,j,k;
	int cur_col,cur_row;
	HXCFE_TD * td;
	uint32_t * ptr;
	unsigned char * ptrchar;
	int nb_col,nb_row,max_row;
	bitmap_data bdata;

	uint32_t pal[256];
	int nbcol;

	ret = HXCFE_NOERROR;

	imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"Write BMP file %s",filename);

	td = hxcfe_td_init(imgldr_ctx->hxcfe,1024,480);
	if(td)
	{
		hxcfe_td_setProgressCallback(td,&progress_callback,(void*)imgldr_ctx);

		hxcfe_td_activate_analyzer(td,ISOIBM_MFM_ENCODING,1);
		hxcfe_td_activate_analyzer(td,ISOIBM_FM_ENCODING,1);
		hxcfe_td_activate_analyzer(td,AMIGA_MFM_ENCODING,1);
		hxcfe_td_activate_analyzer(td,EMU_FM_ENCODING,1);
		hxcfe_td_activate_analyzer(td,MEMBRAIN_MFM_ENCODING,1);
		hxcfe_td_activate_analyzer(td,TYCOM_FM_ENCODING,1);
		hxcfe_td_activate_analyzer(td,APPLEII_GCR1_ENCODING,1);
		hxcfe_td_activate_analyzer(td,APPLEII_GCR2_ENCODING,1);
		//hxcfe_td_activate_analyzer(td,ARBURGDAT_ENCODING,1);
		//hxcfe_td_activate_analyzer(td,ARBURGSYS_ENCODING,1);

		hxcfe_td_setparams(td,240*1000,16,90*1000);

		max_row = 32;

		if( floppydisk->floppyNumberOfTrack * floppydisk->floppyNumberOfSide < 88 )
		{
			max_row = 10;
		}
		else
		{
			if( floppydisk->floppyNumberOfTrack * floppydisk->floppyNumberOfSide < 180)
			{
				max_row = 16;
			}
		}

		nb_col = (( floppydisk->floppyNumberOfTrack * floppydisk->floppyNumberOfSide ) / max_row)+1;
		if( ( floppydisk->floppyNumberOfTrack * floppydisk->floppyNumberOfSide ) >= max_row)
			nb_row = max_row;
		else
			nb_row = floppydisk->floppyNumberOfTrack * floppydisk->floppyNumberOfSide;

		ptr = malloc((td->xsize*td->ysize*4)*nb_row*nb_col);
		if(ptr)
		{
			memset(ptr,0,(td->xsize*td->ysize*4)*nb_row*nb_col);

			cur_row = 0;
			cur_col = 0;
			for(j=0;j<floppydisk->floppyNumberOfTrack;j++)
			{
				for(i=0;i<floppydisk->floppyNumberOfSide;i++)
				{
					hxcfe_imgCallProgressCallback(imgldr_ctx,(j<<1) | (i&1),floppydisk->floppyNumberOfTrack*2 );

					imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"Generate track BMP %d:%d",j,i);
					hxcfe_td_draw_track(td,floppydisk,j,i);

					copyPict((uint32_t *)ptr,nb_col*td->xsize,nb_row*td->ysize,cur_col*td->xsize,cur_row*td->ysize,(uint32_t *)td->framebuffer,td->xsize,td->ysize);

					cur_row++;
					if(cur_row==max_row)
					{
						cur_row = 0;
						cur_col++;
					}

				}
			}

			for(i=0;i<nb_col;i++)
			{
				if( ((i*td->xsize)-4) >= 0 )
					vLine(ptr,nb_col*td->xsize,nb_row*td->ysize,(i*td->xsize)-4);
			}

			for(j=0;j<nb_row;j++)
			{
				if(j&1)
				{
					if( ((j*td->ysize)-1) >= 0 )
						hLine(ptr,nb_col*td->xsize,nb_row*td->ysize,(j*td->ysize)-1);
				}
				else
				{
					if( ((j*td->ysize)-3) >= 0 )
						hLine(ptr,nb_col*td->xsize,nb_row*td->ysize,(j*td->ysize)-3);
					if( ((j*td->ysize)-2) >= 0 )
						hLine(ptr,nb_col*td->xsize,nb_row*td->ysize,(j*td->ysize)-2);
					if( ((j*td->ysize)-1) >= 0 )
						hLine(ptr,nb_col*td->xsize,nb_row*td->ysize,(j*td->ysize)-1);
				}
			}

			for(i=0;i<256;i++)
			{
				pal[i]=i|(i<<8)|(i<<16);
			}

			ptrchar = malloc((td->xsize*td->ysize)*nb_row*nb_col);
			if(ptrchar)
			{
				imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"Converting image...");
				nbcol = 0;
				k=0;
				for(i=0;i< ( nb_row * td->ysize );i++)
				{
					for(j=0;j< ( nb_col * td->xsize );j++)
					{
						ptrchar[k] = getPixelCode(ptr[k],(uint32_t*)&pal,&nbcol);
						k++;
					}
				}

				if(nbcol>=256)
				{
					k = 0;
					for(i=0;i< ( nb_row * td->ysize );i++)
					{
						for(j=0;j< ( nb_col * td->xsize );j++)
						{
							ptr[k] = ptr[k] & 0xF8F8F8;
							k++;
						}
					}

					for(i=0;i<256;i++)
					{
						pal[i]=i|(i<<8)|(i<<16);
					}

					nbcol = 0;
					k=0;
					for(i=0;i< ( nb_row * td->ysize );i++)
					{
						for(j=0;j< ( nb_col * td->xsize );j++)
						{
							ptrchar[k] = getPixelCode(ptr[k],(uint32_t*)&pal,&nbcol);
							k++;
						}
					}
				}

				imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"Writing %s...",filename);

				if(nbcol>=256)
				{
					bdata.nb_color = 16;
					bdata.xsize = td->xsize * nb_col;
					bdata.ysize = td->ysize * nb_row;
					bdata.data = (uint32_t*)ptr;
					bdata.palette = 0;

					bmp16b_write(filename,&bdata);
				}
				else
				{
					bdata.nb_color = 8;
					bdata.xsize = td->xsize * nb_col;
					bdata.ysize = td->ysize * nb_row;
					bdata.data = (uint32_t*)ptrchar;
					bdata.palette = (unsigned char*)&pal;

					bmpRLE8b_write(filename,&bdata);
				}

				imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"%d tracks written to the BMP file",floppydisk->floppyNumberOfTrack * floppydisk->floppyNumberOfSide);

				free(ptrchar);
			}
			else
			{
				ret = HXCFE_INTERNALERROR;
			}

			free(ptr);
		}
		else
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Internal error (malloc) !");
			return HXCFE_INTERNALERROR;
		}

		hxcfe_td_deinit(td);
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Internal error !");
		return HXCFE_INTERNALERROR;
	}

	return ret;
}

