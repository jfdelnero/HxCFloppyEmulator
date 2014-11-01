/*
//
// Copyright (C) 2006-2014 Jean-François DEL NERO
//
// This file is part of HxCFloppyEmulator.
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

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#include "libhxcfe.h"

#include "usb_hxcfloppyemulator.h"
#include "bmp_file.h"

#include "utils.h"

#include "extractTracksLayoutPicture.h"

#define DISKLAYOUT_XRES 1024
#define DISKLAYOUT_YRES 480


void copyPict(unsigned long * dest,int d_xsize,int d_ysize,int d_xpos,int d_ypos,unsigned long * src,int s_xsize,int s_ysize)
{
	int j;
	unsigned long * ptr_line_src;
	unsigned long * ptr_line_dst;

	for(j=0;j<s_ysize;j++)
	{
		ptr_line_src = &src[(s_xsize * j)];
		ptr_line_dst = &dest[(d_xsize * (d_ypos + j)) + d_xpos];

		memcpy(ptr_line_dst,ptr_line_src,s_xsize*4);
	}
}

void vLine(unsigned long * dest,unsigned long d_xsize,unsigned long d_ysize,unsigned long ypos)
{
	unsigned long j;
	unsigned long * ptr_line_dst;

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

void hLine(unsigned long * dest,unsigned long d_xsize,unsigned long d_ysize,unsigned long ypos)
{
	unsigned long * ptr_line_dst;

	ptr_line_dst = &dest[ d_xsize * ypos ];

	if((d_xsize * ypos)<(d_xsize*d_ysize))
	{
		memset(ptr_line_dst,0,d_xsize*4);
	}
}

unsigned char getPixelCode(unsigned long pix,unsigned long * pal,int * nbcol)
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

int extractTracksLayoutPic(HXCFE* hxcfe,char * infile)
{
	int loaderid;
	int ret,i,j,k;
	int cur_col,cur_row;
	HXCFE_FLOPPY * floppydisk;
	HXCFE_IMGLDR * imgldr_ctx;
	HXCFE_TD * td;
	unsigned long * ptr;
	unsigned char * ptrchar;
	int nb_col,nb_row,max_row;
	bitmap_data bdata;
	char ofilename[256];

	unsigned long pal[256];
	int nbcol;

	imgldr_ctx = hxcfe_imgInitLoader(hxcfe);

	if(imgldr_ctx)
	{
		loaderid = hxcfe_imgAutoSetectLoader(imgldr_ctx,infile,0);
		if(loaderid>=0)
		{
			floppydisk = hxcfe_imgLoad(imgldr_ctx,infile,loaderid,&ret);
			if(ret!=HXCFE_NOERROR || !floppydisk)
			{
				switch(ret)
				{
					case HXCFE_UNSUPPORTEDFILE:
						printf("Load error!: Image file not yet supported!\n");
					break;
					case HXCFE_FILECORRUPTED:
						printf("Load error!: File corrupted ? Read error ?\n");
					break;
					case HXCFE_ACCESSERROR:
						printf("Load error!:  Read file error!\n");
					break;
					default:
						printf("Load error! error %d\n",ret);
					break;
				}
			}
			else
			{
				td = hxcfe_td_init(hxcfe,DISKLAYOUT_XRES,DISKLAYOUT_YRES);
				if(td)
				{
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

					if( hxcfe_getNumberOfTrack(hxcfe,floppydisk) * hxcfe_getNumberOfSide(hxcfe,floppydisk) < 88 )
					{
						max_row = 10;
					}
					else
					{
						if( hxcfe_getNumberOfTrack(hxcfe,floppydisk) * hxcfe_getNumberOfSide(hxcfe,floppydisk) < 180)
						{
							max_row = 16;
						}
					}

					nb_col = (( hxcfe_getNumberOfTrack(hxcfe,floppydisk) * hxcfe_getNumberOfSide(hxcfe,floppydisk) ) / max_row)+1;
					if( ( hxcfe_getNumberOfTrack(hxcfe,floppydisk) * hxcfe_getNumberOfSide(hxcfe,floppydisk) ) >= max_row)
						nb_row = max_row;
					else
						nb_row = hxcfe_getNumberOfTrack(hxcfe,floppydisk) * hxcfe_getNumberOfSide(hxcfe,floppydisk);

					ptr = malloc((DISKLAYOUT_XRES*DISKLAYOUT_YRES*4)*nb_row*nb_col);
					if(ptr)
					{
						memset(ptr,0,(DISKLAYOUT_XRES*DISKLAYOUT_YRES*4)*nb_row*nb_col);

						cur_row = 0;
						cur_col = 0;
						for(j=0;j<hxcfe_getNumberOfTrack(hxcfe,floppydisk);j++)
						{
							for(i=0;i<hxcfe_getNumberOfSide(hxcfe,floppydisk);i++)
							{
								printf("Generate track BMP %d:%d\n",j,i);
								hxcfe_td_draw_track(td,floppydisk,j,i);

								copyPict((unsigned long *)ptr,nb_col*DISKLAYOUT_XRES,nb_row*DISKLAYOUT_YRES,cur_col*DISKLAYOUT_XRES,cur_row*DISKLAYOUT_YRES,(unsigned long *)hxcfe_td_getframebuffer(td),DISKLAYOUT_XRES,DISKLAYOUT_YRES);

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
							if( ((i*DISKLAYOUT_XRES)-4) >= 0 )
								vLine(ptr,nb_col*DISKLAYOUT_XRES,nb_row*DISKLAYOUT_YRES,(i*DISKLAYOUT_XRES)-4);
						}

						for(j=0;j<nb_row;j++)
						{
							if(j&1)
							{
								if( ((j*DISKLAYOUT_YRES)-1) >= 0 )
									hLine(ptr,nb_col*DISKLAYOUT_XRES,nb_row*DISKLAYOUT_YRES,(j*DISKLAYOUT_YRES)-1);
							}
							else
							{
								if( ((j*DISKLAYOUT_YRES)-3) >= 0 )
									hLine(ptr,nb_col*DISKLAYOUT_XRES,nb_row*DISKLAYOUT_YRES,(j*DISKLAYOUT_YRES)-3);
								if( ((j*DISKLAYOUT_YRES)-2) >= 0 )
									hLine(ptr,nb_col*DISKLAYOUT_XRES,nb_row*DISKLAYOUT_YRES,(j*DISKLAYOUT_YRES)-2);
								if( ((j*DISKLAYOUT_YRES)-1) >= 0 )
									hLine(ptr,nb_col*DISKLAYOUT_XRES,nb_row*DISKLAYOUT_YRES,(j*DISKLAYOUT_YRES)-1);
							}
						}

						get_filename(infile,ofilename);
						strcat(ofilename,".bmp");

						for(i=0;i<256;i++)
						{
							pal[i]=i|(i<<8)|(i<<16);
						}
		
						ptrchar = malloc((DISKLAYOUT_XRES*DISKLAYOUT_YRES)*nb_row*nb_col);
						if(ptrchar)
						{
							printf("Converting image...\n");
							nbcol = 0;
							k=0;
							for(i=0;i< ( nb_row * DISKLAYOUT_YRES );i++)
							{
								for(j=0;j< ( nb_col * DISKLAYOUT_XRES );j++)
								{
									ptrchar[k] = getPixelCode(ptr[k],(unsigned long*)&pal,&nbcol);
									k++;
								}
							}

							if(nbcol>=256)
							{
								k = 0;
								for(i=0;i< ( nb_row * DISKLAYOUT_YRES );i++)
								{
									for(j=0;j< ( nb_col * DISKLAYOUT_XRES );j++)
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
								for(i=0;i< ( nb_row * DISKLAYOUT_YRES );i++)
								{
									for(j=0;j< ( nb_col * DISKLAYOUT_XRES );j++)
									{
										ptrchar[k] = getPixelCode(ptr[k],(unsigned long*)&pal,&nbcol);
										k++;
									}
								}
							}

							printf("Writing %s...\n",ofilename);

							if(nbcol>=256)
							{
								bdata.nb_color = 16;
								bdata.xsize = DISKLAYOUT_XRES * nb_col;
								bdata.ysize = DISKLAYOUT_YRES * nb_row;
								bdata.data = (unsigned long*)ptr;
								bdata.palette = 0;

								bmp16b_write(ofilename,&bdata);							
							}
 							else
							{
								bdata.nb_color = 8;
								bdata.xsize = DISKLAYOUT_XRES * nb_col;
								bdata.ysize = DISKLAYOUT_YRES * nb_row;
								bdata.data = (unsigned long*)ptrchar;
								bdata.palette = (unsigned char*)&pal;

								bmpRLE8b_write(ofilename,&bdata);
							}
							printf("Done!\n");

							free(ptrchar);
						}

						free(ptr);
					}

					hxcfe_td_deinit(td);
				}

				hxcfe_imgUnload(imgldr_ctx,floppydisk);
			}
		}

		hxcfe_imgDeInitLoader(imgldr_ctx);
	}
	return 0;
}
