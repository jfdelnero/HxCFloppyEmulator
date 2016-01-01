/*
//
// Copyright (C) 2006-2016 Jean-François DEL NERO
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
// File : microintro.c
// Contains: About dialog animation
//
// Written by:	DEL NERO Jean Francois
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#ifdef WIN32  
#include <windows.h>
#endif
#include "hxcmod.h"

#include "microintro.h"
#include "packer/pack.h"

#include "data/data_bmp_sob_bmp.h"
#include "data/data_bmp_hxc2001_bmp.h"

#include "data/data_jozz_cognition_mod.h"
#include "data/data_maktone_class_cracktro15_mod.h"
#include "data/data_vim_not_again_mod.h"
#include "data/data_zandax_supplydas_booze_mod.h"

typedef struct scrolltab_
{
	int ysrc;
	int ydst;
	int len;
	int speed;
	int offset;
}scrolltab;

scrolltab scroll[]=
{
	{240,5,52,1,0},  // ciel
	{0,80,74,48,0}, // montagnes

	{74,154,2,60,0}, // pelouse...
	{76,156,4,80,0},
	{80,160,8,100,0},
	{88,168,8,120,0},
	{96,176,12,140,0},
	
	{142,76,6,20,0},
	{148,66,10,40,0},
	{158,46,20,80,0},
	{178,15,40,120,0},
	{-2,0,22, 160,0},

	{218,0,22, 160,0},
	
	{108,166,34,160,0},  // barrieres
	{-1,0,0,0,0}
};

void putsprite(uintro_context * democontext,unsigned int x,unsigned int y,unsigned int * buffer,unsigned int sx,unsigned int sy,unsigned int * sprite)
{
	float f4;
	unsigned int start,i,j,t2,adr;
	unsigned char ty;

	t2=0;
	start=y*democontext->xsize+x;
	j=0;

	democontext->sprt_f1 = democontext->sprt_f1+(float)0.5;
	f4 = democontext->sprt_f1;
	democontext->sprt_f2 = democontext->sprt_f2+(float)0.01;

	if(democontext->xsize<=(111+x)) t2=(111+x)-democontext->xsize;

	for(j=0;j<sy;j++)
	{
		f4 = f4 + ((float)sin( democontext->sprt_f2 )/(float)2);
		ty=(unsigned char)((float)cos( f4 )*(float)2+(float)4);
		
		if(democontext->xsize<=(111+x+ty)) t2=(111+x+ty)-democontext->xsize;
		else t2=0;

		for(i=0;i<(sx-t2);i++) 
		{
			adr=(j*democontext->xsize+i+start+ty);
			
			if(adr<(unsigned int)(democontext->xsize*democontext->ysize))
			{
				buffer[adr]=sprite[j*sx+i];
			}
		}
	}
}

void convert8b16b(bmaptype * img,unsigned short transcolor)
{
	int i,j;
	unsigned int * newbuffer;
	unsigned char palletteindex;
	unsigned char r,v,b;

	switch(img->type)
	{
	case 9:
		newbuffer=malloc(img->Ysize*img->Xsize*sizeof(unsigned int ));
		for(i=0;i<img->Ysize;i++)
		{
			for(j=0;j<img->Xsize;j++)
			{
				// 11111 000000 11111
				palletteindex=img->unpacked_data[3*256+((i*img->Xsize)+j)];
				
				
				b=img->unpacked_data[palletteindex*3];
				v=img->unpacked_data[palletteindex*3+1];
				r=img->unpacked_data[palletteindex*3+2];

				newbuffer[(i*img->Xsize)+j]=b | (v<<8) | (r<<16);// /*0x1F |*/ /*(0x7E<<4) */(0x1F <<11);//img->data[palletteindex+1]>>3;

			}
		}
		free(img->unpacked_data);
		img->unpacked_data=(unsigned char*)newbuffer;

		break;
	case 1:
		newbuffer=malloc(img->Ysize*img->Xsize*sizeof(unsigned int ));
		for(i=0;i<((img->Ysize*img->Xsize)/8);i++)
		{
			for(j=0;j<8;j++)
			{

				if(img->unpacked_data[i]&(0x80>>j)) newbuffer[i*8+j]=0xFFFF;
				else newbuffer[i*8+j]=0;
			
			}
			
		}

		free(img->unpacked_data);
		img->unpacked_data=(unsigned char*)newbuffer;
		break;
	}
}

void convert8b24b(bmaptype * img,unsigned short transcolor)
{
	int i,j;
	unsigned char * newbuffer;
	unsigned char palletteindex;
	unsigned char r,v,b;

	switch(img->type)
	{
	case 9:
		newbuffer=malloc(img->Ysize*img->Xsize*3);
		for(i=0;i<img->Ysize;i++)
		{
			for(j=0;j<img->Xsize;j++)
			{
				// 11111 000000 11111
				palletteindex=img->unpacked_data[3*256+((i*img->Xsize)+j)];
				
				
				b=img->unpacked_data[palletteindex*3];
				v=img->unpacked_data[palletteindex*3+1];
				r=img->unpacked_data[palletteindex*3+2];

				newbuffer[((i*img->Xsize)*3)+j*3+0]=r;
				newbuffer[((i*img->Xsize)*3)+j*3+1]=v;
				newbuffer[((i*img->Xsize)*3)+j*3+2]=b;

			}
		}
		free(img->unpacked_data);
		img->unpacked_data=(unsigned char*)newbuffer;

		break;
	case 1:
		newbuffer=malloc(img->Ysize*img->Xsize*3);
		for(i=0;i<((img->Ysize*img->Xsize)/8);i++)
		{
			for(j=0;j<8;j++)
			{

				if(img->unpacked_data[i]&(0x80>>j))
				{
					newbuffer[(i*8)*3+(j*3)+0]=0xFF;
					newbuffer[(i*8)*3+(j*3)+1]=0xFF;
					newbuffer[(i*8)*3+(j*3)+2]=0xFF;
				}
				else
				{
					newbuffer[(i*8)*3+(j*3)+0]=0x00;
					newbuffer[(i*8)*3+(j*3)+1]=0x00;
					newbuffer[(i*8)*3+(j*3)+2]=0x00;
				}
			
			}
			
		}

		free(img->unpacked_data);
		img->unpacked_data=(unsigned char*)newbuffer;
		break;
	}
}


uintro_context * uintro_init(unsigned short xsize,unsigned short ysize)
{
	uintro_context * ui_context;

	ui_context=(uintro_context *)malloc(sizeof(uintro_context));
	memset(ui_context,0,sizeof(uintro_context));	

	ui_context->xsize=xsize;
	ui_context->ysize=ysize;

	ui_context->tick=0;

	ui_context->framebuffer=(unsigned int *)malloc(ui_context->xsize*ui_context->ysize*sizeof(unsigned int));
	memset(ui_context->framebuffer,0,ui_context->xsize*ui_context->ysize*sizeof(unsigned int));


	ui_context->blurbuffer=(unsigned int *)malloc(ui_context->xsize*ui_context->ysize*sizeof(unsigned int));
	memset(ui_context->blurbuffer,0,ui_context->xsize*ui_context->ysize*sizeof(unsigned int));

	bitmap_sob_bmp->unpacked_data=mi_unpack(bitmap_sob_bmp->data,bitmap_sob_bmp->csize ,bitmap_sob_bmp->data, bitmap_sob_bmp->size);
	convert8b16b(bitmap_sob_bmp,(unsigned short)0xFFFF);

	bitmap_hxc2001_bmp->unpacked_data=mi_unpack(bitmap_hxc2001_bmp->data,bitmap_hxc2001_bmp->csize ,bitmap_hxc2001_bmp->data, bitmap_hxc2001_bmp->size);
	convert8b16b(bitmap_hxc2001_bmp,(unsigned short)0xFFFF);

	ui_context->modctx = malloc( sizeof(modcontext) );
	if(ui_context->modctx)
	{
		memset( ui_context->modctx, 0, sizeof(modcontext));

		hxcmod_init( (modcontext*)ui_context->modctx );
		hxcmod_setcfg( (modcontext*)ui_context->modctx, 44100, 16, 1, 1, 1);

#ifdef WIN32    
		srand(GetTickCount());
#endif
		switch(rand()&3)
		{
			case 0:
				data_maktone_class_cracktro15_mod->unpacked_data=mi_unpack(data_maktone_class_cracktro15_mod->data,data_maktone_class_cracktro15_mod->csize ,data_maktone_class_cracktro15_mod->data, data_maktone_class_cracktro15_mod->size);
				hxcmod_load((modcontext*)ui_context->modctx, (void*)data_maktone_class_cracktro15_mod->unpacked_data, data_maktone_class_cracktro15_mod->size);
			break;
			case 1:
				data_jozz_cognition_mod->unpacked_data=mi_unpack(data_jozz_cognition_mod->data,data_jozz_cognition_mod->csize ,data_jozz_cognition_mod->data, data_jozz_cognition_mod->size);
				hxcmod_load((modcontext*)ui_context->modctx, (void*)data_jozz_cognition_mod->unpacked_data, data_jozz_cognition_mod->size);
			break;
			case 2:
				data_zandax_supplydas_booze_mod->unpacked_data=mi_unpack(data_zandax_supplydas_booze_mod->data,data_zandax_supplydas_booze_mod->csize ,data_zandax_supplydas_booze_mod->data, data_zandax_supplydas_booze_mod->size);
				hxcmod_load((modcontext*)ui_context->modctx, (void*)data_zandax_supplydas_booze_mod->unpacked_data, data_zandax_supplydas_booze_mod->size);
			break;
			case 3:
				data_vim_not_again_mod->unpacked_data=mi_unpack(data_vim_not_again_mod->data,data_vim_not_again_mod->csize ,data_vim_not_again_mod->data, data_vim_not_again_mod->size);
				hxcmod_load((modcontext*)ui_context->modctx, (void*)data_vim_not_again_mod->unpacked_data, data_vim_not_again_mod->size);
			break;

		}
	}

	return ui_context;
}

void uintro_reset(uintro_context * ui_context)
{
	ui_context->col_f1 = 0;
	ui_context->col_f2 = 0;
	ui_context->col_f3 = 0;
	ui_context->col_f1s = 0;
	ui_context->col_f2s = 0;
	ui_context->col_f3s = 0;

	ui_context->f1=0;
	ui_context->f2=0;

	ui_context->sprt_f1=0;
	ui_context->sprt_f2=0;

	ui_context->tick = 0;

	memset(ui_context->framebuffer,0,ui_context->xsize*ui_context->ysize*sizeof(unsigned int));
	memset(ui_context->blurbuffer,0,ui_context->xsize*ui_context->ysize*sizeof(unsigned int));

#ifdef WIN32    
	srand(GetTickCount());
#endif
	switch(rand()&3)
	{
		case 0:
			if(!data_maktone_class_cracktro15_mod->unpacked_data)
				data_maktone_class_cracktro15_mod->unpacked_data=mi_unpack(data_maktone_class_cracktro15_mod->data,data_maktone_class_cracktro15_mod->csize ,data_maktone_class_cracktro15_mod->data, data_maktone_class_cracktro15_mod->size);
				hxcmod_load((modcontext*)ui_context->modctx,(void*)data_maktone_class_cracktro15_mod->unpacked_data,data_maktone_class_cracktro15_mod->size);
		break;
		case 1:
			if(!data_jozz_cognition_mod->unpacked_data)
				data_jozz_cognition_mod->unpacked_data=mi_unpack(data_jozz_cognition_mod->data,data_jozz_cognition_mod->csize ,data_jozz_cognition_mod->data, data_jozz_cognition_mod->size);
				hxcmod_load((modcontext*)ui_context->modctx,(void*)data_jozz_cognition_mod->unpacked_data,data_jozz_cognition_mod->size);
		break;
		case 2:
			if(!data_zandax_supplydas_booze_mod->unpacked_data)
				data_zandax_supplydas_booze_mod->unpacked_data=mi_unpack(data_zandax_supplydas_booze_mod->data,data_zandax_supplydas_booze_mod->csize ,data_zandax_supplydas_booze_mod->data, data_zandax_supplydas_booze_mod->size);
				hxcmod_load((modcontext*)ui_context->modctx,(void*)data_zandax_supplydas_booze_mod->unpacked_data,data_zandax_supplydas_booze_mod->size);
		break;
		case 3:
			if(!data_vim_not_again_mod->unpacked_data)
				data_vim_not_again_mod->unpacked_data=mi_unpack(data_vim_not_again_mod->data,data_vim_not_again_mod->csize ,data_vim_not_again_mod->data, data_vim_not_again_mod->size);
				hxcmod_load((modcontext*)ui_context->modctx,(void*)(data_vim_not_again_mod->unpacked_data),data_vim_not_again_mod->size);
		break;

	}
}


void uintro_getnext_soundsample(uintro_context * democontext,unsigned short* buffer,int size)
{
	hxcmod_fillbuffer( (modcontext*)democontext->modctx, buffer, size/2,0);
}


void colorize(uintro_context * democontext,bmaptype * bitmaptype)
{
	int i,j;
	unsigned int * ptr;
	unsigned char r,v,b;
	ptr=(unsigned int *)bitmaptype->unpacked_data;

	democontext->col_f1=democontext->col_f1s;
	democontext->col_f2=democontext->col_f2s;
	democontext->col_f3=democontext->col_f3s;

	for(i=0;i<bitmaptype->Ysize;i++)
	{
		for(j=0;j<bitmaptype->Xsize;j++)
		{
			if(ptr[i*bitmaptype->Xsize + j])
			{
				r=(unsigned char)(cos(democontext->col_f1)*(float)120)+129;
				v=(unsigned char)(sin(democontext->col_f2)*(float)120)+129;
				b=(unsigned char)(cos(democontext->col_f3)*(float)120)+129;

				ptr[i*bitmaptype->Xsize + j]=(r<<16)|(v<<8)|b;
			}
			
			democontext->col_f1=democontext->col_f1+(float)0.0001;
			democontext->col_f2=democontext->col_f2+(float)0.00011;
			democontext->col_f3=democontext->col_f3+(float)0.000101;

		}
	}

	democontext->col_f1s=democontext->col_f1s+(float)0.03;
	democontext->col_f2s=democontext->col_f2s+(float)0.033;
	democontext->col_f3s=democontext->col_f3s+(float)0.0333;
}



void uintro_getnextframe(uintro_context * democontext)
{
	int i,j,k,l,totalpix;
	int x_coef_motion,y_coef_motion;
	unsigned int * src_ptr;
	unsigned int * dst_ptr;
	unsigned int * framebuffer;
	unsigned int * blurbuffer;
	int xsize,ysize,baroffset;
	unsigned int pix;
	
	xsize = democontext->xsize;
	ysize = democontext->ysize;

	totalpix = xsize * ysize;
	framebuffer = democontext->framebuffer;
	blurbuffer = democontext->blurbuffer;

	democontext->tick++;
	if(democontext->tick>=50)
	{
		democontext->tick=0;
	}


	k=0;
	
	for(i=0;i<totalpix;i++)
	{
		framebuffer[i]=0x00607080;
	}

	colorize(democontext,bitmap_hxc2001_bmp);
	////////////////////////
	x_coef_motion=(democontext->xsize-(bitmap_hxc2001_bmp->Xsize+5))/4;
	y_coef_motion=(democontext->ysize-bitmap_hxc2001_bmp->Ysize)/4;

	memset(democontext->blurbuffer,0,democontext->xsize*democontext->ysize*4);

	putsprite(democontext,
			(unsigned int)((float)cos(democontext->f1)*(float)x_coef_motion+((float)x_coef_motion*2)),
			(unsigned int)((float)cos(democontext->f2)*(float)y_coef_motion+((float)y_coef_motion*2)),
			democontext->blurbuffer,
			bitmap_hxc2001_bmp->Xsize,
			bitmap_hxc2001_bmp->Ysize,
			(unsigned int*)bitmap_hxc2001_bmp->unpacked_data);

	democontext->f1=democontext->f1+(float)0.11;
	democontext->f2=democontext->f2+(float)0.09;
	////////////////////////

	while(scroll[k].ysrc!=-1)
	{
		if(scroll[k].ysrc!=-2)
		{

			l=(((scroll[k].ysrc)*4)*bitmap_sob_bmp->Xsize);	
			baroffset = scroll[k].offset/32;

			for(i=scroll[k].ydst;i<(scroll[k].ydst+scroll[k].len);i++) 
			{
				src_ptr = (unsigned int*)&bitmap_sob_bmp->unpacked_data[l];
				dst_ptr = (unsigned int*)&framebuffer[i*xsize];

				for(j=baroffset;j<(xsize);j++)
				{
					pix = *(src_ptr++);
					if(pix != 0x00FF00)
					{						
						dst_ptr[j] = pix;
					}
				}

				for(j=0;j<baroffset;j++)
				{
					pix = *(src_ptr++);
					if(pix != 0x00FF00)
					{						
						dst_ptr[j] = pix;
					}
				}

				l=l+(xsize<<2);
			}

		}
		else
		{
			for(i=0;i<totalpix;i++)
			{
				if(blurbuffer[i])
				{
					framebuffer[i]=blurbuffer[i];
				}
			}
		}
		k++;
	}

	k=0;
	while(scroll[k].ysrc!=-1)
	{
		scroll[k].offset=(scroll[k].offset+scroll[k].speed) % (xsize*32);
		k++;
	}

}


void uintro_deinit(uintro_context * democontext)
{
	if(data_maktone_class_cracktro15_mod->unpacked_data) free(data_maktone_class_cracktro15_mod->unpacked_data);
	if(data_jozz_cognition_mod->unpacked_data) free(data_jozz_cognition_mod->unpacked_data);
	if(data_zandax_supplydas_booze_mod->unpacked_data) free(data_zandax_supplydas_booze_mod->unpacked_data);
	if(data_vim_not_again_mod->unpacked_data) free(data_vim_not_again_mod->unpacked_data);

	if(democontext->modctx)
	{
		hxcmod_unload( (modcontext*)democontext->modctx );
		free(democontext->modctx);
		democontext->modctx = 0;
	}

	data_jozz_cognition_mod->unpacked_data=0;
	data_maktone_class_cracktro15_mod->unpacked_data=0;
	data_zandax_supplydas_booze_mod->unpacked_data=0;
	data_vim_not_again_mod->unpacked_data=0;

	free(bitmap_hxc2001_bmp->unpacked_data);
	free(bitmap_sob_bmp->unpacked_data);
	free(democontext->framebuffer);
	free(democontext->blurbuffer);
	free(democontext);
}