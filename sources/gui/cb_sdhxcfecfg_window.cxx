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
// File : cb_sdhxcfecfg_window.cxx
// Contains: SD HxC Setting window
//
// Written by:	DEL NERO Jean Francois
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include "sdhxcfecfg_window.h"
#include "cb_sdhxcfecfg_window.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "fileselector.h"
#include "sdhxcfe_cfg.h"

extern "C"
{
	#include "libhxcfe.h"
	#include "usb_hxcfloppyemulator.h"
	#include "libhxcadaptor.h"
}

#include "main.h"
#include "loader.h"

unsigned char filebuffer[8*1024];

extern s_gui_context * guicontext;
extern Fl_Menu_Item if_choices[];

void fill_cfg(sdhxcfecfg_window *sdcfgw,sdhxcfecfgfile * filecfg)
{
	memset(filebuffer,0,8192);
	sprintf(filecfg->signature,"HXCFECFGV1.0");

	if((unsigned char)sdcfgw->slider_uisound_level->value())
	{
		filecfg->ihm_sound=0xFF;
		filecfg->buzzer_duty_cycle=(unsigned char)sdcfgw->slider_uisound_level->value();
	}
	else
	{
		filecfg->ihm_sound=0x00;
		filecfg->buzzer_duty_cycle=0x80;
	}

	if(sdcfgw->chk_disabediskdriveselector->value())
		filecfg->disable_drive_select=0xFF;
	else
		filecfg->disable_drive_select=0x00;
					
	if(sdcfgw->chk_loadlastloaded->value())
		filecfg->load_last_floppy=0x00;
	else
		filecfg->load_last_floppy=0xFF;

	if(sdcfgw->chk_enable_autoboot_mode->value())
		filecfg->number_of_slot=0x01;
	else
		filecfg->number_of_slot=0x00;

	if(sdcfgw->chk_enable_indexed_mode->value())
		filecfg->indexed_mode=0xFF;
	else
		filecfg->indexed_mode=0x00;

	filecfg->back_light_tmr=(unsigned char)sdcfgw->valslider_device_backlight_timeout->value();
	filecfg->standby_tmr=(unsigned char)sdcfgw->valslider_device_standby_timeout->value();
					
	filecfg->buzzer_step_duration=0xFF-(unsigned char)sdcfgw->slider_stepsound_level->value();
	if(filecfg->buzzer_step_duration==0xFF)
		filecfg->step_sound=0x00;
	else
		filecfg->step_sound=0xFF;

	filecfg->lcd_scroll_speed=64+((255-64)) - (unsigned char)sdcfgw->slider_scrolltxt_speed->value();

	filecfg->startup_mode=0;

	if(sdcfgw->chk_force_loading_startupa->value())
		filecfg->startup_mode=filecfg->startup_mode|0x1;

	if(sdcfgw->chk_force_loading_startupb->value())
		filecfg->startup_mode=filecfg->startup_mode|0x2;

	if(sdcfgw->chk_force_loading_autoboot->value())
		filecfg->startup_mode=filecfg->startup_mode|0x4;

	if(sdcfgw->chk_preindex->value())
		filecfg->startup_mode=filecfg->startup_mode|0x8;


}

void set_cfg(sdhxcfecfg_window *sdcfgw,sdhxcfecfgfile * filecfg)
{
	
	if(!strncmp(filecfg->signature,"HXCFECFGV1.0",12))
	{

		if(filecfg->ihm_sound)
			sdcfgw->slider_uisound_level->value(filecfg->buzzer_duty_cycle);
		else
			sdcfgw->slider_uisound_level->value(0);

		if(filecfg->disable_drive_select)
			sdcfgw->chk_disabediskdriveselector->set();
		else
			sdcfgw->chk_disabediskdriveselector->clear();
			
		if(filecfg->load_last_floppy)
			sdcfgw->chk_loadlastloaded->clear();
		else
			sdcfgw->chk_loadlastloaded->set();

		if(filecfg->number_of_slot)
			sdcfgw->chk_enable_autoboot_mode->set();
		else
			sdcfgw->chk_enable_autoboot_mode->clear();

		if(filecfg->indexed_mode)
			sdcfgw->chk_enable_indexed_mode->set();
		else
			sdcfgw->chk_enable_indexed_mode->clear();

		sdcfgw->valslider_device_backlight_timeout->value(filecfg->back_light_tmr);
		sdcfgw->valslider_device_standby_timeout->value(filecfg->standby_tmr);
						
		sdcfgw->slider_stepsound_level->value(0xFF-filecfg->buzzer_step_duration);
		
		sdcfgw->slider_scrolltxt_speed->value(64+((255-64)) - filecfg->lcd_scroll_speed);

		if(filecfg->startup_mode&0x1)
			sdcfgw->chk_force_loading_startupa->set();
		else
			sdcfgw->chk_force_loading_startupa->clear();

		if(filecfg->startup_mode&0x2)
			sdcfgw->chk_force_loading_startupb->set();
		else
			sdcfgw->chk_force_loading_startupb->clear();

		if(filecfg->startup_mode&0x4)
			sdcfgw->chk_force_loading_autoboot->set();
		else
			sdcfgw->chk_force_loading_autoboot->clear();
			
		if(filecfg->startup_mode&0x8)
			sdcfgw->chk_preindex->set();
		else
			sdcfgw->chk_preindex->clear();
	}
}


void sdhxcfecfg_window_datachanged(Fl_Widget* w, void*)
{
	sdhxcfecfg_window *sdcfgw;
	Fl_Widget* tw;

	tw=w;
	do
	{
		tw=tw->parent();
		sdcfgw=(sdhxcfecfg_window *)tw->user_data();
	}while(!sdcfgw);

	fill_cfg(sdcfgw,(sdhxcfecfgfile *)&filebuffer);

}

void sdhxcfecfg_window_bt_load(Fl_Button* bt, void*)
{
	int temp[512];
	FILE *f;
	sdhxcfecfg_window *sdcfgw;
	Fl_Window *dw;

	dw=((Fl_Window*)(bt->parent()));
	sdcfgw=(sdhxcfecfg_window *)dw->user_data();
	
	if(!fileselector((char*)"Select config file",(char*)temp,(char*)"*.cfg",(char*)"*.cfg",0,0))
	{
		f=hxc_fopen((char*)temp,"r+b");
		if(f)
		{
			fread(filebuffer,8*1024,1,f);
			hxc_fclose(f);
			set_cfg(sdcfgw,(sdhxcfecfgfile *)&filebuffer);
		}
	}
}

void sdhxcfecfg_window_bt_save(Fl_Button* bt, void*)
{
	int temp[512];
	FILE *f;
	sdhxcfecfg_window *sdcfgw;
	Fl_Window *dw;

	dw=((Fl_Window*)(bt->parent()));
	sdcfgw=(sdhxcfecfg_window *)dw->user_data();

	fill_cfg(sdcfgw,(sdhxcfecfgfile *)&filebuffer);

	if(!fileselector((char*)"Select config file",(char*)temp,(char*)"HXCSDFE.CFG",(char*)"*.cfg",1,0))
	{
		f=hxc_fopen((char*)temp,"w+b");
		if(f)
		{
			fwrite(filebuffer,8*1024,1,f);
			hxc_fclose(f);
		}
	}
}

void save_ifcfg_window_bt(Fl_Button * bt,void *)
{

}

void load_ifcfg_window_bt(Fl_Button * bt,void *)
{

}

void ifcfg_window_datachanged(Fl_Widget * w,void * bt)
{

	sdhxcfecfg_window *sdcfgw;
	Fl_Widget* tw;

	tw=w;
	do
	{
		tw=tw->parent();
		sdcfgw=(sdhxcfecfg_window *)tw->user_data();
	}while(!sdcfgw);


	if(!sdcfgw->chk_hfr_autoifmode->value())
	{
		guicontext->autoselectmode = 0x00;
	}
	else
	{
		guicontext->autoselectmode = 0xFF;
	}

	if(!sdcfgw->chk_hfe_doublestep->value())
	{
		guicontext->doublestep = 0x00;
	}
	else
	{
		guicontext->doublestep = 0xFF;
	}

}
