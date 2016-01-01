/*
//
// Copyright (C) 2006-2016 Jean-François DEL NERO
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

extern void copyPict(uint32_t * dest,int d_xsize,int d_ysize,int d_xpos,int d_ypos,uint32_t * src,int s_xsize,int s_ysize);

extern unsigned char getPixelCode(uint32_t pix,uint32_t * pal,int * nbcol);

static int progress_callback(unsigned int current,unsigned int total,void * td,void * user)
{
	return hxcfe_imgCallProgressCallback((HXCFE_IMGLDR*)user,current,total);
}

int BMP_Disk_libWrite_DiskFile(HXCFE_IMGLDR* imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * filename)
{
	int ret,i,j,k;
	HXCFE_TD * td;
	uint32_t * ptr;
	unsigned char * ptrchar;
	bitmap_data bdata;
	char name[1024];

	uint32_t pal[256];
	int nbcol;

	ret = HXCFE_NOERROR;

	imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"Write BMP file %s",filename);

	td = hxcfe_td_init(imgldr_ctx->hxcfe,1920,940);
	if(td)
	{
		hxcfe_td_setProgressCallback(td,&progress_callback,(void*)imgldr_ctx);

		hxc_getfilenamewext(filename, (char*)&name );
		hxcfe_td_setName( td , (char*)&name );

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

		ptr = malloc(td->xsize*td->ysize*4);
		if(ptr)
		{
			memset(ptr,0,(td->xsize*td->ysize*4));

			imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"Generate track BMP...");

			hxcfe_td_draw_disk(td,floppydisk);

			copyPict((uint32_t *)ptr,td->xsize,td->ysize,0,0,(uint32_t *)td->framebuffer,td->xsize,td->ysize);

			for(i=0;i<256;i++)
			{
				pal[i]=i|(i<<8)|(i<<16);
			}

			ptrchar = malloc((td->xsize*td->ysize));
			if(ptrchar)
			{
				imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"Converting image...");
				nbcol = 0;
				k=0;
				for(i=0;i< ( td->ysize );i++)
				{
					for(j=0;j< ( td->xsize );j++)
					{
						ptrchar[k] = getPixelCode(ptr[k],(uint32_t*)&pal,&nbcol);
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
					for(i=0;i< ( td->ysize );i++)
					{
						for(j=0;j< ( td->xsize );j++)
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

