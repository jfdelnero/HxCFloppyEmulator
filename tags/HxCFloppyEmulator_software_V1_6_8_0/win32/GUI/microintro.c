#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#include "microintro.h"
#include "hxc2001bmp.h"
////////////////////////////////////////////////////////////////////////// 
//
//  Copie d'un sprite avec ondulation...
//  
//////////////////////////////////////////////////////////////////////////


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



uintro_context * uintro_init(unsigned short xsize,unsigned short ysize)
{
	uintro_context * ui_context;
	
	ui_context=(uintro_context *)malloc(sizeof(uintro_context));
	memset(ui_context,0,sizeof(uintro_context));

	ui_context->xsize=xsize;
	ui_context->ysize=ysize;

	ui_context->sprite_xsize=130;
	ui_context->sprite_ysize=35;

	ui_context->framebuffer=(unsigned long *)malloc(ui_context->xsize*ui_context->ysize*sizeof(unsigned long));
	memset(ui_context->framebuffer,0,ui_context->xsize*ui_context->ysize*sizeof(unsigned long));


	return ui_context;
}


void uintro_getnextframe(uintro_context * democontext)
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


void uintro_deinit(uintro_context * democontext)
{
	free(democontext->framebuffer);
	free(democontext);
}