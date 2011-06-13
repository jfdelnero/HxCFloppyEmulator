#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#include "microintro.h"
#include "mod32.h"
#include "packer\pack.h"
#include "data\hxc2001bmp.h"
#include "data\data_bmp_sob_bmp.h"

#include "data\data_jozz_cognition_mod.h"
#include "data\data_maktone_class_cracktro15_mod.h"
#include "data\data_vim_not_again_mod.h"
#include "data\data_zandax_supplydas_booze_mod.h"
#include "data\themod.h"

////////////////////////////////////////////////////////////////////////// 
//
//  Copie d'un sprite avec ondulation...
//  
//////////////////////////////////////////////////////////////////////////

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
	{218,0,22, 160,0},
	
	{108,166,34,160,0},  // barrieres
	{-1,0,0,0,0}
};

void putsprite(uintro_context * democontext,unsigned int x,unsigned int y,unsigned long * buffer,unsigned int sx,unsigned int sy,unsigned char * sprite)
{
	
	static int t3=0;
	static float f3=0,f4,f5=0,f6=0;
	unsigned int start,i,j,t2,adr;
	unsigned char t,ty;
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
		ty=(unsigned char)((float)cos(f4)*(float)3+(float)4);
		if(democontext->xsize<=(111+x+ty)) t2=(111+x+ty)-democontext->xsize;
		else t2=0;
		for(i=0;i<(sx-t2);i++) 
		{
			adr=(j*democontext->xsize+i+start+ty);
			t=sprite[j*sx+i];
			if(adr<(unsigned int)(democontext->xsize*democontext->ysize) && ((m==1 && t==52)|(m==2 && t!=52)|(m==0)))buffer[adr]=pallogoece[3*t]|pallogoece[3*t+1]<<8|pallogoece[3*t+2]<<16;
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

	ui_context->sprite_xsize=130;
	ui_context->sprite_ysize=35;
	ui_context->tick=0;

	ui_context->framebuffer=(unsigned long *)malloc(ui_context->xsize*ui_context->ysize*sizeof(unsigned long));
	memset(ui_context->framebuffer,0,ui_context->xsize*ui_context->ysize*sizeof(unsigned long));


	ui_context->blurbuffer=(unsigned long *)malloc(ui_context->xsize*ui_context->ysize*sizeof(unsigned long));
	memset(ui_context->blurbuffer,0,ui_context->xsize*ui_context->ysize*sizeof(unsigned long));

	bitmap_sob_bmp->unpacked_data=mi_unpack(bitmap_sob_bmp->data,bitmap_sob_bmp->csize ,bitmap_sob_bmp->data, bitmap_sob_bmp->size);
	convert8b16b(bitmap_sob_bmp,(unsigned short)0xFFFF);

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

void uintro_getnextframe_random(uintro_context * democontext)
{
	int i;
	unsigned char t;

	int x_coef_motion,y_coef_motion;


	for(i=0;i<(democontext->xsize*democontext->ysize);i++) 
	{
		t=rand();
		democontext->framebuffer[i]=t&0xff|t<<8|t<<16;
	}

	x_coef_motion=(democontext->xsize-(democontext->sprite_xsize+5))/2;
	y_coef_motion=(democontext->ysize-democontext->sprite_ysize)/2;
	putsprite(democontext,
			(unsigned int)((float)cos(democontext->f1)*(float)x_coef_motion+(float)x_coef_motion),
			(unsigned int)((float)cos(democontext->f2)*(float)y_coef_motion+(float)y_coef_motion),
			democontext->framebuffer,
			democontext->sprite_xsize,
			democontext->sprite_ysize,
			(unsigned char*)hxc2001bmp);
			
	democontext->f1=democontext->f1+(float)0.11;
	democontext->f2=democontext->f2+(float)0.09;
}


void uintro_getnext_soundsample(uintro_context * democontext,unsigned char* buffer,int size)
{
	GiveMeSamples(buffer,size);
}

void uintro_getnextframe_starfield(uintro_context * democontext)
{
	int i,j,k,l,t;
	int x_coef_motion,y_coef_motion;
	unsigned long * ptr1,*ptr2;

	k=0;
	
	for(i=0;i<democontext->xsize*democontext->ysize;i++)
	{
		democontext->framebuffer[i]=0x00607080;
	}

	while(scroll[k].ysrc!=-1)
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

		k++;
	}

	k=0;
	while(scroll[k].ysrc!=-1)
	{
		scroll[k].offset=scroll[k].offset+scroll[k].speed;
		k++;
	}



	
	ptr1=democontext->blurbuffer;
	ptr2=democontext->framebuffer;

	/*for(i=0;i<democontext->xsize*democontext->ysize;i++)
	{
		if(democontext->blurbuffer[i])
		{
			t=(democontext->blurbuffer[i]>>16 + (democontext->blurbuffer[i]>>8 &0xFF) + (democontext->blurbuffer[i] &0xFF))/3;
			democontext->blurbuffer[i]=democontext->blurbuffer[i]-0x010101;
		}

	}*/
	
	
	for(i=2;i<democontext->xsize;i++)
	{
		for(j=1;j<democontext->ysize-1;j++)
		{
			t=(((democontext->blurbuffer[(j-1)*(democontext->xsize)+i]>>16)&0xFF) + (democontext->blurbuffer[(j-1)*democontext->xsize+i]>>8 &0xFF) + (democontext->blurbuffer[(j-1)*democontext->xsize+i] &0xFF))/3;
			t=t+(((democontext->blurbuffer[(j+1)*(democontext->xsize)+i]>>16)&0xFF) + (democontext->blurbuffer[(j+1)*democontext->xsize+i]>>8 &0xFF) + (democontext->blurbuffer[(j+1)*democontext->xsize+i] &0xFF))/3;
			t=t+(((democontext->blurbuffer[j*(democontext->xsize)+i-1]>>16)&0xFF) + (democontext->blurbuffer[j*democontext->xsize+i-1]>>8 &0xFF) + (democontext->blurbuffer[j*democontext->xsize+i-1] &0xFF))/3;
			t=t+(((democontext->blurbuffer[j*(democontext->xsize)+i-2]>>16)&0xFF) + (democontext->blurbuffer[j*democontext->xsize+i-2]>>8 &0xFF) + (democontext->blurbuffer[j*democontext->xsize+i-2] &0xFF))/3;
			
			t=t/6;

			democontext->blurbuffer[(j)*(democontext->xsize)+i]=t<<16|t<<8|t;

		}
	}
	
	
	x_coef_motion=(democontext->xsize-(democontext->sprite_xsize+5))/4;
	y_coef_motion=(democontext->ysize-democontext->sprite_ysize)/4;
	putsprite(democontext,
			(unsigned int)((float)cos(democontext->f1)*(float)x_coef_motion+((float)x_coef_motion*2)),
			(unsigned int)((float)cos(democontext->f2)*(float)y_coef_motion+((float)y_coef_motion*2)),
			democontext->blurbuffer,
			democontext->sprite_xsize,
			democontext->sprite_ysize,
			(unsigned char*)hxc2001bmp);
			
	democontext->f1=democontext->f1+(float)0.11;
	democontext->f2=democontext->f2+(float)0.09;


	for(i=0;i<democontext->xsize*democontext->ysize;i++)
	{
		if(democontext->blurbuffer[i])
		{

			//democontext->framebuffer[i]=((255/((democontext->blurbuffer[i])&0xFF)) * democontext->framebuffer[i])&0xFF;
			democontext->framebuffer[i]=democontext->blurbuffer[i];
		}

	}

}


void uintro_getnextframe(uintro_context * democontext)
{



	democontext->tick++;
	if(democontext->tick>=50)
	{
		//democontext->part++;
		if(democontext->part>=2)
			democontext->part=0;
		democontext->tick=0;
	}

	switch(democontext->part)
	{
		case 1:
			uintro_getnextframe_random(democontext);
			break;

		case 0:
			uintro_getnextframe_starfield(democontext);
			break;

		default:
			uintro_getnextframe_random(democontext);
			break;

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

	free(bitmap_sob_bmp->unpacked_data);
	free(democontext->framebuffer);
	free(democontext->blurbuffer);
	free(democontext);
}