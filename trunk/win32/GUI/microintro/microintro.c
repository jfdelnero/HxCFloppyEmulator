/*
//
// Copyright (C) 2006, 2007, 2008, 2009, 2010, 2011 Jean-François DEL NERO
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

#include "microintro.h"
#include "mod32.h"
#include "packer\pack.h"

#include "data\data_bmp_sob_bmp.h"
#include "data\data_bmp_hxc2001_bmp.h"

#include "data\data_jozz_cognition_mod.h"
#include "data\data_maktone_class_cracktro15_mod.h"
#include "data\data_vim_not_again_mod.h"
#include "data\data_zandax_supplydas_booze_mod.h"

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

void putsprite(uintro_context * democontext,unsigned int x,unsigned int y,unsigned long * buffer,unsigned int sx,unsigned int sy,unsigned long * sprite)
{
	
	static int t3=0;
	static float f3=0,f4,f5=0,f6=0;
	unsigned int start,i,j,t2,adr;
	unsigned char ty;
	static unsigned char m=0;
	t2=0;
	start=y*democontext->xsize+x;
	j=0;
	t3++;
	
	f3=f3+(float)0.5;
	f4=f3;
	f5=f5+(float)0.01;
	f6=f6+(float)0.02;

	if(t3>50*30){m=0;t3=0; }
	if(t3>50*20)  m=1;
	else if(t3>50*10)  m=2;

	if(democontext->xsize<=(111+x)) t2=(111+x)-democontext->xsize;

	for(j=0;j<sy;j++)
	{
		f4=f4+((float)sin(f5)/(float)2);
		ty=(unsigned char)((float)cos(f4)*(float)2+(float)4);
		if(democontext->xsize<=(111+x+ty)) t2=(111+x+ty)-democontext->xsize;
		else t2=0;
		for(i=0;i<(sx-t2);i++) 
		{
			adr=(j*democontext->xsize+i+start+ty);
			
			if(adr<(unsigned int)(democontext->xsize*democontext->ysize))
				buffer[adr]=sprite[j*sx+i];
		}
	}
}

void convert8b16b(bmaptype * img,unsigned short transcolor)
{
	int i,j;
	unsigned long * newbuffer;
	unsigned char palletteindex;
	unsigned char r,v,b;

	switch(img->type)
	{
	case 9:
		newbuffer=malloc(img->Ysize*img->Xsize*sizeof(unsigned long ));
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
		newbuffer=malloc(img->Ysize*img->Xsize*sizeof(unsigned long ));
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


uintro_context * uintro_init(unsigned short xsize,unsigned short ysize)
{
	uintro_context * ui_context;
	char buffer1[16];

	ui_context=(uintro_context *)malloc(sizeof(uintro_context));
	memset(ui_context,0,sizeof(uintro_context));	

	ui_context->xsize=xsize;
	ui_context->ysize=ysize;

	ui_context->tick=0;

	ui_context->framebuffer=(unsigned long *)malloc(ui_context->xsize*ui_context->ysize*sizeof(unsigned long));
	memset(ui_context->framebuffer,0,ui_context->xsize*ui_context->ysize*sizeof(unsigned long));


	ui_context->blurbuffer=(unsigned long *)malloc(ui_context->xsize*ui_context->ysize*sizeof(unsigned long));
	memset(ui_context->blurbuffer,0,ui_context->xsize*ui_context->ysize*sizeof(unsigned long));

	bitmap_sob_bmp->unpacked_data=mi_unpack(bitmap_sob_bmp->data,bitmap_sob_bmp->csize ,bitmap_sob_bmp->data, bitmap_sob_bmp->size);
	convert8b16b(bitmap_sob_bmp,(unsigned short)0xFFFF);

	bitmap_hxc2001_bmp->unpacked_data=mi_unpack(bitmap_hxc2001_bmp->data,bitmap_hxc2001_bmp->csize ,bitmap_hxc2001_bmp->data, bitmap_hxc2001_bmp->size);
	convert8b16b(bitmap_hxc2001_bmp,(unsigned short)0xFFFF);

	srand(GetTickCount());

	switch(rand()&3)
	{
		case 0:
			data_maktone_class_cracktro15_mod->unpacked_data=mi_unpack(data_maktone_class_cracktro15_mod->data,data_maktone_class_cracktro15_mod->csize ,data_maktone_class_cracktro15_mod->data, data_maktone_class_cracktro15_mod->size);
			InitModule(NULL,buffer1,16,data_maktone_class_cracktro15_mod->unpacked_data);
		break;
		case 1:
			data_jozz_cognition_mod->unpacked_data=mi_unpack(data_jozz_cognition_mod->data,data_jozz_cognition_mod->csize ,data_jozz_cognition_mod->data, data_jozz_cognition_mod->size);
			InitModule(NULL,buffer1,16,data_jozz_cognition_mod->unpacked_data);
		break;
		case 2:
			data_zandax_supplydas_booze_mod->unpacked_data=mi_unpack(data_zandax_supplydas_booze_mod->data,data_zandax_supplydas_booze_mod->csize ,data_zandax_supplydas_booze_mod->data, data_zandax_supplydas_booze_mod->size);
			InitModule(NULL,buffer1,16,data_zandax_supplydas_booze_mod->unpacked_data);
		break;
		case 3:
			data_vim_not_again_mod->unpacked_data=mi_unpack(data_vim_not_again_mod->data,data_vim_not_again_mod->csize ,data_vim_not_again_mod->data, data_vim_not_again_mod->size);
			InitModule(NULL,buffer1,16,data_vim_not_again_mod->unpacked_data);
		break;

	}
		
	return ui_context;
}


void uintro_getnext_soundsample(uintro_context * democontext,unsigned char* buffer,int size)
{
	GiveMeSamples(buffer,size);
}


void colorize(bmaptype * bitmaptype)
{
	int i,j;
	static float f1=0,f2=0,f3=0;
	static float f1s=0,f2s=0,f3s=0;
	unsigned long * ptr;
	unsigned char r,v,b;
	ptr=(unsigned long *)bitmaptype->unpacked_data;

	f1=f1s;
	f2=f2s;
	f3=f3s;

	for(i=0;i<bitmaptype->Ysize;i++)
	{
		for(j=0;j<bitmaptype->Xsize;j++)
		{
			if(ptr[i*bitmaptype->Xsize + j])
			{
				r=(unsigned char)(cos(f1)*(float)120)+129;
				v=(unsigned char)(sin(f2)*(float)120)+129;
				b=(unsigned char)(cos(f3)*(float)120)+129;

				ptr[i*bitmaptype->Xsize + j]=(r<<16)|(v<<8)|b;
			}
			
			f1=f1+(float)0.0001;
			f2=f2+(float)0.00011;
			f3=f3+(float)0.000101;

		}
	}

	f1s=f1s+(float)0.03;
	f2s=f2s+(float)0.033;
	f3s=f3s+(float)0.0333;
}



void uintro_getnextframe(uintro_context * democontext)
{
	int i,j,k,l;
	int x_coef_motion,y_coef_motion;

	democontext->tick++;
	if(democontext->tick>=50)
	{
		democontext->tick=0;
	}


	k=0;
	
	for(i=0;i<democontext->xsize*democontext->ysize;i++)
	{
		democontext->framebuffer[i]=0x00607080;
	}

	colorize(bitmap_hxc2001_bmp);
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
			(unsigned long*)bitmap_hxc2001_bmp->unpacked_data);
			
	democontext->f1=democontext->f1+(float)0.11;
	democontext->f2=democontext->f2+(float)0.09;
	////////////////////////

	while(scroll[k].ysrc!=-1)
	{
		if(scroll[k].ysrc!=-2)
		{

			l=(((scroll[k].ysrc)*4)*bitmap_sob_bmp->Xsize);	
			for(i=scroll[k].ydst;i<(scroll[k].ydst+scroll[k].len);i++) 
			{

				for(j=0;j<democontext->xsize;j++)
				{
					if(bitmap_sob_bmp->unpacked_data[l]!=0x00 || bitmap_sob_bmp->unpacked_data[l+1]!=0xFF || bitmap_sob_bmp->unpacked_data[l+2]!=00)
					democontext->framebuffer[(i*democontext->xsize)+((j+(scroll[k].offset/32))%320)]=bitmap_sob_bmp->unpacked_data[l]|(bitmap_sob_bmp->unpacked_data[l+1]<<8)|(bitmap_sob_bmp->unpacked_data[l+2]<<16);
					l=l+4;
				}
			}
		}
		else
		{
			for(i=0;i<democontext->xsize*democontext->ysize;i++)
			{
				if(democontext->blurbuffer[i])
				{

					//democontext->framebuffer[i]=((255/((democontext->blurbuffer[i])&0xFF)) * democontext->framebuffer[i])&0xFF;
					democontext->framebuffer[i]=democontext->blurbuffer[i];
				}

			}
		}
		k++;
	}

	k=0;
	while(scroll[k].ysrc!=-1)
	{
		scroll[k].offset=scroll[k].offset+scroll[k].speed;
		k++;
	}

}


void uintro_deinit(uintro_context * democontext)
{
	if(data_maktone_class_cracktro15_mod->unpacked_data) free(data_maktone_class_cracktro15_mod->unpacked_data);
	if(data_jozz_cognition_mod->unpacked_data) free(data_jozz_cognition_mod->unpacked_data);
	if(data_zandax_supplydas_booze_mod->unpacked_data) free(data_zandax_supplydas_booze_mod->unpacked_data);
	if(data_vim_not_again_mod->unpacked_data) free(data_vim_not_again_mod->unpacked_data);

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