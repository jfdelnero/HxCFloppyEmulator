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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"

#include "internal_libhxcfe.h"
#include "libhxcfe.h"
#include "libhxcadaptor.h"

#include "bmp_file.h"

int bmp_load(char * file,bitmap_data * bdata)
{
	FILE * f;
	int i,j;
	int xsize,ysize;
	int bitperpixel;
	int palettesize;
	unsigned int    bitmapdataoffset,bitmapdatalinesize;
	unsigned char * linebuffer;
	BITMAPFILEHEADER bitmap_header;
	BITMAPINFOHEADER bitmap_info_header;
	RGBQUAD * palette;
	uint32_t * data_ptr;

	data_ptr = NULL;
	linebuffer = NULL;
	palette = NULL;
	f = NULL;

	f = hxc_fopen(file,"rb");
	if(f)
	{
		hxc_fread(&bitmap_header,sizeof(BITMAPFILEHEADER),f);

		if( bitmap_header.bfType == 19778 )
		{
			hxc_fread(&bitmap_info_header,sizeof(BITMAPINFOHEADER),f);

			xsize=bitmap_info_header.biWidth;
			ysize=bitmap_info_header.biHeight;
			bitperpixel=bitmap_info_header.biBitCount;

			if(!bitmap_info_header.biCompression)
			{
				// read palette
				switch(bitperpixel)
				{
					case 1:
						palettesize = 2;
					break;
					case 4:
						palettesize = 16;
					break;
					case 8:
						palettesize = 256;
					break;
					case 24:
						palettesize = 0;
					break;
					case 32:
						palettesize = 0;
					break;
					default:
						palettesize = 0;
						//non supported
					break;
				}

				palette=0;
				if(palettesize)
				{
					palette = (RGBQUAD *)malloc(palettesize*sizeof(RGBQUAD));
					if(!palette)
						goto error;

					memset(palette,0,palettesize*sizeof(RGBQUAD));

					fseek(f,sizeof(BITMAPFILEHEADER)+ bitmap_info_header.biSize ,SEEK_SET);
					hxc_fread(palette,palettesize*sizeof(RGBQUAD),f);
				}

				bitmapdataoffset = sizeof(BITMAPFILEHEADER)+ bitmap_info_header.biSize + (palettesize*sizeof(RGBQUAD));
				fseek(f,bitmapdataoffset ,SEEK_SET);

				bdata->xsize = xsize;
				bdata->ysize = ysize;
				bdata->data = (void *)malloc(xsize*ysize*sizeof(uint32_t));
				if( !bdata->data )
					goto error;

				memset(bdata->data,0,xsize*ysize*sizeof(uint32_t));

				switch(bitperpixel)
				{
					case 1:
						if((xsize/8 + (xsize&7))&0x3)
							bitmapdatalinesize = (((xsize/8 + (xsize&7)) & (~0x3)) + 0x4);
						else
							bitmapdatalinesize = (xsize/8 + (xsize&7));

						linebuffer = (unsigned char *)malloc(bitmapdatalinesize);
						if( !linebuffer )
							goto error;

						data_ptr = (uint32_t*)bdata->data;

						for(i = 0; i < ysize; i++ )
						{
							fseek(f,bitmapdataoffset + (bitmapdatalinesize*i) ,SEEK_SET);
							hxc_fread(linebuffer,bitmapdatalinesize,f);

							for(j=0;j<xsize;j++)
							{
								data_ptr[(xsize*((ysize-1)-i))+j] = palette[(linebuffer[(j/8)]>>(7-(j&7)))&0x1].rgbRed |
																	palette[(linebuffer[(j/8)]>>(7-(j&7)))&0x1].rgbGreen<<8 |
																	palette[(linebuffer[(j/8)]>>(7-(j&7)))&0x1].rgbBlue<<16;
							}
						}

						free(linebuffer);
						linebuffer = NULL;
						free(palette);
						palette = NULL;
					break;

					case 4:
						if((xsize/2 + (xsize&1))&0x3)
							bitmapdatalinesize = (((xsize/2 + (xsize&1)) & (~0x3)) + 0x4);
						else
							bitmapdatalinesize = (xsize/2 + (xsize&1));

						linebuffer = (unsigned char *)malloc(bitmapdatalinesize);
						if( !linebuffer )
							goto error;

						data_ptr = (uint32_t*)bdata->data;

						for(i=0;i<ysize;i++)
						{
							fseek(f,bitmapdataoffset + (bitmapdatalinesize*i) ,SEEK_SET);
							hxc_fread(linebuffer,bitmapdatalinesize,f);

							for(j=0;j<xsize;j++)
							{
								data_ptr[(xsize*((ysize-1)-i))+j] = palette[(linebuffer[(j/2)]>>(4*((~j)&1)))&0xF].rgbRed |
																	palette[(linebuffer[(j/2)]>>(4*((~j)&1)))&0xF].rgbGreen<<8 |
																	palette[(linebuffer[(j/2)]>>(4*((~j)&1)))&0xF].rgbBlue<<16;
							}
						}

						free(linebuffer);
						linebuffer = NULL;
						free(palette);
						palette = NULL;
					break;

					case 8:
						if(xsize&0x3)
							bitmapdatalinesize = ((xsize & (~0x3)) + 0x4);
						else
							bitmapdatalinesize = xsize;

						linebuffer = (unsigned char *)malloc(bitmapdatalinesize);
						if( !linebuffer )
							goto error;

						data_ptr = (uint32_t*)bdata->data;

						for(i=0;i<ysize;i++)
						{
							fseek(f,bitmapdataoffset + (bitmapdatalinesize*i) ,SEEK_SET);
							hxc_fread(linebuffer,bitmapdatalinesize,f);

							for(j=0;j<xsize;j++)
							{
								data_ptr[(xsize*((ysize-1)-i))+j] = palette[linebuffer[j]].rgbRed |
																	palette[linebuffer[j]].rgbGreen<<8 |
																	palette[linebuffer[j]].rgbBlue<<16;
							}
						}

						free(linebuffer);
						linebuffer = NULL;
						free(palette);
						palette = NULL;
					break;

					case 24:
						if((xsize*3)&0x3)
							bitmapdatalinesize=(((xsize*3) & (~0x3)) + 0x4);
						else
							bitmapdatalinesize=(xsize*3);

						linebuffer=(unsigned char *)malloc(bitmapdatalinesize);
						if( !linebuffer )
							goto error;

						data_ptr = (uint32_t*)bdata->data;

						for(i=0;i<ysize;i++)
						{
							fseek(f,bitmapdataoffset + (bitmapdatalinesize*i) ,SEEK_SET);
							hxc_fread(linebuffer,bitmapdatalinesize,f);

							for(j=0;j<xsize;j++)
							{
								data_ptr[(xsize*((ysize-1)-i))+j] = linebuffer[j*3+2] |
																	linebuffer[j*3+1]<<8 |
																	linebuffer[j*3+0]<<16;
							}
						}

						free(linebuffer);
						linebuffer = NULL;
					break;

					case 32:
						if((xsize*4)&0x3)
							bitmapdatalinesize=(((xsize*4) & (~0x3)) + 0x4);
						else
							bitmapdatalinesize=(xsize*4);

						linebuffer=(unsigned char *)malloc(bitmapdatalinesize);
						if( !linebuffer )
							goto error;

						data_ptr = (uint32_t*)bdata->data;

						for(i=0;i<ysize;i++)
						{
							fseek(f,bitmapdataoffset + (bitmapdatalinesize*i) ,SEEK_SET);
							hxc_fread(linebuffer,bitmapdatalinesize,f);

							for(j=0;j<xsize;j++)
							{
								data_ptr[(xsize*((ysize-1)-i))+j] = linebuffer[j*4+2] |
																	linebuffer[j*4+1]<<8 |
																	linebuffer[j*4+0]<<16;
							}
						}

						free(linebuffer);
						linebuffer = NULL;

					break;

					default:
						//non supported
					break;
				}
			}
			else
			{
				// non supported
				hxc_fclose(f);
				return -1;
			}
		}
		else
		{
			// non bitmap file
			hxc_fclose(f);
			return -2;
		}

		hxc_fclose(f);
		return 0;  // ok
	}

	//file error
	return -3;

error:

	free( data_ptr );
	free( linebuffer );
	free( palette );

	if( f )
		fclose( f );

	return -3;
}

int bmp24b_write(char * file,bitmap_data * bdata)
{
	FILE * f;
	int i,j;
	BITMAPFILEHEADER bitmap_header;
	BITMAPINFOHEADER bitmap_info_header;
	uint32_t bitmapdatalinesize;
	unsigned char * linebuffer;

	f = hxc_fopen(file,"wb");
	if(f)
	{
		memset(&bitmap_header,0,sizeof(BITMAPFILEHEADER));

		bitmap_header.bfType = 19778;
		bitmap_header.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

		bitmapdatalinesize = bdata->xsize*3;

		if(bitmapdatalinesize&0x3)
			bitmapdatalinesize = ((bitmapdatalinesize & (~0x3)) + 0x4);

		bitmap_header.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + (bitmapdatalinesize*bdata->ysize);
		fwrite(&bitmap_header,sizeof(BITMAPFILEHEADER),1,f);

		memset(&bitmap_info_header,0,sizeof(BITMAPINFOHEADER));
		bitmap_info_header.biSize = sizeof(BITMAPINFOHEADER);
		bitmap_info_header.biBitCount = 24;
		bitmap_info_header.biHeight = bdata->ysize;
		bitmap_info_header.biWidth = bdata->xsize;
		bitmap_info_header.biPlanes = 1;
		bitmap_info_header.biSizeImage = bitmapdatalinesize*bdata->ysize;

		fwrite(&bitmap_info_header,sizeof(BITMAPINFOHEADER),1,f);

		linebuffer = (unsigned char *)malloc(bitmapdatalinesize);
		if( linebuffer )
		{
			memset(linebuffer,0,bitmapdatalinesize);

			uint32_t * data_ptr = (uint32_t*)bdata->data;

			for(i=0;i<bdata->ysize;i++)
			{
				for(j=0;j<bdata->xsize;j++)
				{
					linebuffer[(j*3)+2] = (unsigned char)( (data_ptr[((((bdata->ysize-1)-i))*bdata->xsize) + j]) & 0xFF);
					linebuffer[(j*3)+1] = (unsigned char)( (data_ptr[((((bdata->ysize-1)-i))*bdata->xsize) + j]>>8) & 0xFF);
					linebuffer[(j*3)+0] = (unsigned char)( (data_ptr[((((bdata->ysize-1)-i))*bdata->xsize) + j]>>16) & 0xFF);
				}

				fwrite(linebuffer,bitmapdatalinesize,1,f);
			}

			free(linebuffer);
			linebuffer = NULL;
		}
		hxc_fclose(f);
	}

	return 0;
}

int bmp16b_write(char * file,bitmap_data * bdata)
{
	FILE * f;
	int i,j;
	unsigned short color;
	BITMAPFILEHEADER bitmap_header;
	BITMAPINFOHEADER bitmap_info_header;
	uint32_t bitmapdatalinesize;
	unsigned char * linebuffer;

	f = hxc_fopen(file,"wb");
	if(f)
	{
		memset(&bitmap_header,0,sizeof(BITMAPFILEHEADER));

		bitmap_header.bfType = 19778;
		bitmap_header.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

		bitmapdatalinesize = bdata->xsize*2;

		if(bitmapdatalinesize&0x3)
			bitmapdatalinesize = ((bitmapdatalinesize & (~0x3)) + 0x4);

		bitmap_header.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + (bitmapdatalinesize*bdata->ysize);
		fwrite(&bitmap_header,sizeof(BITMAPFILEHEADER),1,f);

		memset(&bitmap_info_header,0,sizeof(BITMAPINFOHEADER));
		bitmap_info_header.biSize = sizeof(BITMAPINFOHEADER);
		bitmap_info_header.biBitCount = 16;
		bitmap_info_header.biHeight = bdata->ysize;
		bitmap_info_header.biWidth = bdata->xsize;
		bitmap_info_header.biPlanes = 1;
		bitmap_info_header.biSizeImage = bitmapdatalinesize*bdata->ysize;

		fwrite(&bitmap_info_header,sizeof(BITMAPINFOHEADER),1,f);

		linebuffer = (unsigned char *)malloc(bitmapdatalinesize);
		if( linebuffer )
		{
			memset(linebuffer,0,bitmapdatalinesize);

			uint32_t * data_ptr = (uint32_t*)bdata->data;

			for(i=0;i<bdata->ysize;i++)
			{
				for(j=0;j<bdata->xsize;j++)
				{
					color = (unsigned short)( (data_ptr[((((bdata->ysize-1)-i))*bdata->xsize) + j]) & 0xFF)>>3;
					color = (color<<5) | (unsigned short)( (data_ptr[((((bdata->ysize-1)-i))*bdata->xsize) + j]>>8) & 0xFF)>>3;
					color = (color<<5) | (unsigned short)( (data_ptr[((((bdata->ysize-1)-i))*bdata->xsize) + j]>>16) & 0xFF)>>3;

					linebuffer[(j*2)+0] = color & 0xFF;
					linebuffer[(j*2)+1] = (color>>8) & 0xFF;
				}

				fwrite(linebuffer,bitmapdatalinesize,1,f);
			}

			free(linebuffer);
			linebuffer = NULL;
		}
		hxc_fclose(f);
	}

	return 0;
}

int packlineRLE(unsigned char * src,int size,unsigned char * dst)
{
	int cnt,i,j,lastvalue;


	i = 0;
	j = 0;
	lastvalue = src[0];
	cnt = 0;
	while(i < size)
	{
		if( (src[i] == lastvalue) && (cnt<254))
		{
			cnt++;
		}
		else
		{
			dst[j++] = cnt;
			dst[j++] = lastvalue;

			lastvalue = src[i];
			cnt = 1;
		}

		i++;
	}

	if(cnt)
	{
		dst[j++] = cnt;
		dst[j++] = lastvalue;
	}

	dst[j++] = 0;
	dst[j++] = 0;

	return j;
}

int bmpRLE8b_write(char * file,bitmap_data * bdata)
{
	FILE * f;
	int i,lnsize,datasize;
	BITMAPFILEHEADER bitmap_header;
	BITMAPINFOHEADER bitmap_info_header;
	uint32_t bitmapdatalinesize;
	unsigned char * linebuffer;
	unsigned char * src_data;
	unsigned char pal[256*4];

	f = hxc_fopen(file,"wb");
	if(f)
	{
		memset(&bitmap_header,0,sizeof(BITMAPFILEHEADER));

		bitmap_header.bfType = 19778;
		bitmap_header.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + (256*4);

		bitmapdatalinesize = bdata->xsize*1;

		if(bitmapdatalinesize&0x3)
			bitmapdatalinesize = ((bitmapdatalinesize & (~0x3)) + 0x4);

		bitmap_header.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + sizeof(pal) + (bitmapdatalinesize*bdata->ysize);
		fwrite(&bitmap_header,sizeof(BITMAPFILEHEADER),1,f);

		memset(&bitmap_info_header,0,sizeof(BITMAPINFOHEADER));
		bitmap_info_header.biSize = sizeof(BITMAPINFOHEADER);
		bitmap_info_header.biBitCount = 8;
		bitmap_info_header.biHeight = bdata->ysize;
		bitmap_info_header.biWidth = bdata->xsize;
		bitmap_info_header.biPlanes = 1;
		bitmap_info_header.biCompression = 1;
		bitmap_info_header.biSizeImage = bitmapdatalinesize*bdata->ysize;

		fwrite(&bitmap_info_header,sizeof(BITMAPINFOHEADER),1,f);

		memset(pal,0,sizeof(pal));
		for(i=0;i<256;i++)
		{
			pal[(i*4) + 0] = (bdata->palette[(i*4) + 2]);
			pal[(i*4) + 1] = (bdata->palette[(i*4) + 1]);
			pal[(i*4) + 2] = (bdata->palette[(i*4) + 0]);
		}

		fwrite(pal,sizeof(pal),1,f);

		datasize = 0;

		linebuffer = (unsigned char *)malloc((bitmapdatalinesize*2) + (bdata->ysize*2));
		if( linebuffer )
		{
			memset(linebuffer,0,(bitmapdatalinesize*2) + (bdata->ysize*2));

			src_data = (unsigned char*)bdata->data;
			for(i=0;i<bdata->ysize;i++)
			{
				lnsize = packlineRLE(&src_data[((bdata->ysize-1)-i)*bdata->xsize],bdata->xsize,linebuffer);

				if( i == (bdata->ysize - 1) )
				{
					if(lnsize)
					{
						linebuffer[lnsize-1] = 0x01; // End of BMP
					}
				}

				fwrite(linebuffer,lnsize,1,f);

				datasize = datasize + lnsize;
			}

			free(linebuffer);
			linebuffer = NULL;
		}

		bitmap_header.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + sizeof(pal) + datasize;
		fseek(f,0,SEEK_SET);
		fwrite(&bitmap_header,sizeof(BITMAPFILEHEADER),1,f);

		bitmap_info_header.biSizeImage = datasize;
		fseek(f,sizeof(BITMAPFILEHEADER),SEEK_SET);
		fwrite(&bitmap_info_header,sizeof(BITMAPINFOHEADER),1,f);

		hxc_fclose(f);
	}

	return 0;
}
