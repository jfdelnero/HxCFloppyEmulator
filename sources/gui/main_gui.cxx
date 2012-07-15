/*
//
// Copyright (C) 2006-2012 Jean-François DEL NERO
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
// File : main_gui.cxx
// Contains: Main GUI window
//
// Written by:	DEL NERO Jean Francois
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include "fl_includes.h"

#include <errno.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h> 
#include <time.h>

#include "batch_converter_window.h"
#include "filesystem_generator_window.h"
#include "floppy_dump_window.h"
#include "floppy_infos_window.h"
#include "rawfile_loader_window.h"
#include "sdhxcfecfg_window.h"
#include "usbhxcfecfg_window.h"
#include "log_gui.h"
#include "about_gui.h"

#include "soft_cfg_file.h"
#include "fl_dnd_box.h"
#include "win32\resource.h"

extern "C"
{
	#include "libhxcfe.h"
	#include "usb_hxcfloppyemulator.h"
	#include "version.h"
}

#include "loader.h"
#include "main_gui.h"
#include "main.h"

#include "cb_floppy_dump_window.h"
#include "cb_floppy_infos_window.h"

#include "msg_txt.h"

extern int CUI_affiche(int MSGTYPE,char * chaine, ...);
extern s_gui_context * guicontext;

static void tick_main(void *v) {
	
	Main_Window *window;
	
	window=(Main_Window *)v;	
	window->make_current();
	Fl::repeat_timeout(0.10, tick_main, v);
}

void menu_clicked(Fl_Widget * w, void * fc_ptr) 
{	
	int i;
	Main_Window *mw;
	Fl_Window *dw;
	i=(int)fc_ptr;

	dw=((Fl_Window*)(w->parent()));
	mw=(Main_Window *)dw->user_data();

	switch(i)
	{
		case 1:
			mw->rawloader_window->window->show();
		break;
		case 2:
			mw->batchconv_window->window->show();
		break;
		case 3:
			mw->fs_window->window->show();
		break;
		case 5:
			mw->sdcfg_window->window->show();
		break;
		case 6:
			mw->usbcfg_window->window->show();
		break;
		case 7:
			mw->fdump_window->window->show();
		break;
		case 8:
			mw->log_box->show();
		break;
		case 9:
			mw->about_window->show();
		break;
		case 10:
			mw->infos_window->window->show();
		break;

	}

}

void bt_clicked(Fl_Widget * w, void * fc_ptr) 
{	
	int i;
	Main_Window *mw;
	Fl_Window *dw;
	i=(int)fc_ptr;

	dw=((Fl_Window*)(w->parent()->parent()));
	mw=(Main_Window *)dw->user_data();

	switch(i)
	{
		case 1:
			mw->rawloader_window->window->show();
		break;
		case 2:
			mw->batchconv_window->window->show();
		break;
		case 3:
			mw->fs_window->window->show();
		break;
		case 5:
			mw->sdcfg_window->window->show();
		break;
		case 6:
			mw->usbcfg_window->window->show();
		break;
		case 7:
			mw->fdump_window->window->show();
		break;
		case 8:
			mw->log_box->show();
		break;
		case 9:
			mw->about_window->show();
		break;

	}

}


void load_file_image(Fl_Widget * w, void * fc_ptr) 
{
	Fl_Native_File_Chooser fnfc;
  
	fnfc.title("Load image file");
	fnfc.type(Fl_Native_File_Chooser::BROWSE_FILE);
	fnfc.filter("Floppy disk image file\t*.*\n");
	//fnfc.directory("/var/tmp"); // default directory to use
	// Show native chooser
	switch ( fnfc.show() ) {
		case -1:
		{
			break; // ERROR
		}
		case 1:
		{
			break; // CANCEL
		}
		default:
		{
			load_floppy_image((char*)fnfc.filename());
			guicontext->updatefloppyinfos++;
			break; // FILE CHOSEN
		}
	}
}

void save_file_image(Fl_Widget * w, void * fc_ptr) 
{
	int i;
	int loaderid;
	Fl_Native_File_Chooser fnfc;
	unsigned char deffilename[512];
	
	const char * plugid_lst[]=
	{
		{PLUGIN_HXC_HFE},
		{PLUGIN_VTR_IMG},
		{PLUGIN_HXC_MFM},
		{PLUGIN_HXC_AFI},
		{PLUGIN_RAW_LOADER},
		{PLUGIN_AMSTRADCPC_DSK},
		{PLUGIN_IMD_IMG},
		{PLUGIN_TI994A_V9T9},
		{PLUGIN_HXC_EXTHFE}
	};

	if(!guicontext->loadedfloppy)
	{
		fl_alert("No floppy loaded !\nPlease drag and drop your disk file into the window\n",10.0);
	}
	else
	{
		
		sprintf((char*)deffilename,"%s",guicontext->bufferfilename);
		i=0;
		while(deffilename[i]!=0)
		{
			if(deffilename[i]=='.')deffilename[i]='_';
			i++;
		}

		fnfc.title("Export disk/Save As");
		fnfc.preset_file((char*)deffilename);
		fnfc.type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
		fnfc.options(Fl_Native_File_Chooser::SAVEAS_CONFIRM|Fl_Native_File_Chooser::NEW_FOLDER);
		fnfc.filter("HFE file (SDCard HxC Floppy Emulator file format)\t*.hfe\n"
					"VTR file (VTrucco Floppy Emulator file format)\t*.vtr\n"
					"MFM file (MFM/FM track file format)\t*.mfm\n"
					"AFI file (Advanced File image format)\t*.afi\n"
					"IMG file (RAW Sector file format)\t*.img\n"
					"CPC DSK file\t*.dsk\n"
					"IMD file\t*.imd\n"
					"TI99/4A V9T9 DSK file\t*.dsk\n"
					"HFE file (Rev 2 - Experimental)\t*.hfe\n");


		//fnfc.directory("/var/tmp"); // default directory to use
		
		// Show native chooser
		switch ( fnfc.show() ) {
			case -1:
				break; // ERROR
			case 1:
				break; // CANCEL
			default:
			{
				i=fnfc.filter_value();
				
				loaderid=hxcfe_getLoaderID(guicontext->hxcfe,(char*)plugid_lst[i]);
				
				if(loaderid>=0)
				{				
					hxcfe_floppyExport(guicontext->hxcfe,guicontext->loadedfloppy,(char*)fnfc.filename(),loaderid);
				}
				break; // FILE CHOSEN
			}
		}
	}
}


void load_file_image_pb(Fl_Widget * widget, void * ptr) 
{
	load_file_image(widget,ptr);
}

void fc_callback(Fl_File_Chooser * fc, void *) 
{
	const char * fp;
	fp=fc->value();
}


#define BUTTON_XPOS 5
#define BUTTON_YPOS 30
#define BUTTON_XSIZE 100
#define BUTTON_YSIZE 35
#define BUTTON_YSTEP 35

#define WINDOW_XSIZE 341

void sync_if_config()
{
	if(guicontext->loadedfloppy)
	{
		if(guicontext->autoselectmode)
		{
			guicontext->interfacemode = hxcfe_floppyGetInterfaceMode(guicontext->hxcfe,guicontext->loadedfloppy);
		}
		else
		{
			hxcfe_floppySetInterfaceMode(guicontext->hxcfe,guicontext->loadedfloppy,guicontext->interfacemode);
		}
		hxcfe_floppySetDoubleStep(guicontext->hxcfe,guicontext->loadedfloppy,guicontext->doublestep);
	}

	if(guicontext->usbhxcfe)
		libusbhxcfe_setInterfaceMode(guicontext->hxcfe,guicontext->usbhxcfe,guicontext->interfacemode,guicontext->doublestep,guicontext->driveid);
}

void format_choice_cb(Fl_Widget *, void *v) 
{
	guicontext->interfacemode =  (int)v;

	sync_if_config();
}

void dnd_open(const char *urls)
{
	load_floppy_image((char*)urls);
	guicontext->updatefloppyinfos++;
}

void dnd_cb(Fl_Widget *o, void *v)
{
    Fl_DND_Box *dnd = (Fl_DND_Box*)o;

    if(dnd->event() == FL_PASTE)
        dnd_open(dnd->event_text());
}

static void tick_mw(void *v) {
	Main_Window *window;
	int i,j;
	char tempstr[1024];
	char tempstr2[1024];

	window=(Main_Window *)v;

	if(strlen(guicontext->bufferfilename))
	{
		sprintf(tempstr,"%s - %d track(s) %d side(s)     ",guicontext->bufferfilename,guicontext->loadedfloppy->floppyNumberOfTrack,guicontext->loadedfloppy->floppyNumberOfSide);

		if( guicontext->txtindex >= strlen(tempstr) )
		{
			guicontext->txtindex=0;
		}
				
		i=0;
		j=guicontext->txtindex;
		do
		{
			tempstr2[i]=tempstr[j];
			i++;
			j++;
			if(j>=(int)strlen(tempstr)) j=0;
		}while(i<(int)strlen(tempstr));
		tempstr2[i]=0;
		memcpy(&tempstr2[i],tempstr2,strlen(tempstr2));
			
		window->file_name_txt->value((const char*)tempstr2);
		//SetDlgItemText(hwndDlg,IDC_EDIT_STATUS,tempstr2);
			
		guicontext->txtindex++;
	}
	else
	{
		if(guicontext->loadstatus!=HXCFE_NOERROR)
		{
			switch(guicontext->loadstatus)
			{
				case HXCFE_UNSUPPORTEDFILE:
					window->file_name_txt->value((const char*)"Load error! Image file not supported!");
				break;
				case HXCFE_FILECORRUPTED:
					window->file_name_txt->value((const char*)"Load error! File corrupted/Read error ?");
				break;
				case HXCFE_ACCESSERROR:
					window->file_name_txt->value((const char*)"Load error! Read file error!");
				break;
				default:
					sprintf(tempstr2,"Load error! error %d",guicontext->loadstatus);
					window->file_name_txt->value((const char*)tempstr2);
				break;
			}
		}
	}
			
	if(guicontext->loadedfloppy)
	{
		sprintf(tempstr,"Track %d/%d",libusbhxcfe_getCurTrack(guicontext->hxcfe,guicontext->usbhxcfe),guicontext->loadedfloppy->floppyNumberOfTrack);

		window->track_pos_str->value((const char*)tempstr);
		window->track_pos->minimum(0);
		window->track_pos->maximum(guicontext->loadedfloppy->floppyNumberOfTrack);
		window->track_pos->value((float)libusbhxcfe_getCurTrack(guicontext->hxcfe,guicontext->usbhxcfe));
	}

	if(guicontext->autoselectmode)
	{
		if(!window->sdcfg_window->chk_hfr_autoifmode->value())
			window->sdcfg_window->chk_hfr_autoifmode->value(1);
		if(!window->usbcfg_window->chk_autoifmode->value())
			window->usbcfg_window->chk_autoifmode->value(1);

		window->usbcfg_window->choice_ifmode->deactivate();
		window->sdcfg_window->choice_hfeifmode->deactivate();

		i=0;
		while( if_choices[i].text && ((int)if_choices[i].user_data_ != guicontext->interfacemode))
		{
			i++;
		}
		window->usbcfg_window->choice_ifmode->value(i);
		window->sdcfg_window->choice_hfeifmode->value(i);
	}
	else
	{
		if(window->sdcfg_window->chk_hfr_autoifmode->value())
			window->sdcfg_window->chk_hfr_autoifmode->value(0);
		if(window->usbcfg_window->chk_autoifmode->value())
			window->usbcfg_window->chk_autoifmode->value(0);

		i=0;
		while( if_choices[i].text && ((int)if_choices[i].user_data_ != guicontext->interfacemode))
		{
			i++;
		}
		window->usbcfg_window->choice_ifmode->value(i);
		window->sdcfg_window->choice_hfeifmode->value(i);


		window->usbcfg_window->choice_ifmode->activate();
		window->sdcfg_window->choice_hfeifmode->activate();
	}

	if(guicontext->doublestep)
	{
		if(!window->sdcfg_window->chk_hfe_doublestep->value())
			window->sdcfg_window->chk_hfe_doublestep->value(1);
		if(!window->usbcfg_window->chk_doublestep->value())
			window->usbcfg_window->chk_doublestep->value(1);
	}
	else
	{
		if(window->sdcfg_window->chk_hfe_doublestep->value())
			window->sdcfg_window->chk_hfe_doublestep->value(0);
		if(window->usbcfg_window->chk_doublestep->value())
			window->usbcfg_window->chk_doublestep->value(0);
	}

	Fl::repeat_timeout(0.10, tick_mw, v);

}

Main_Window::Main_Window()
  : Fl_Window(WINDOW_XSIZE,392)
{
	int i;		
	txtindex=0;
	i=0;
	evt_txt=0;
	USBStats stats;

	guicontext->loadedfloppy=0;
	guicontext->autoselectmode=0xFF;
	guicontext->driveid=0x00;
	guicontext->doublestep=0x00;
	guicontext->interfacemode=GENERIC_SHUGART_DD_FLOPPYMODE;
	guicontext->hxcfe=hxcfe_init();
	hxcfe_setOutputFunc(guicontext->hxcfe,CUI_affiche);
		
	guicontext->usbhxcfe=libusbhxcfe_init(guicontext->hxcfe);
		

	Fl::scheme("gtk+");

	Fl_Group group(0,0,WINDOW_XSIZE,392);
	
	this->icon((char *)LoadIcon(fl_display, MAKEINTRESOURCE(101)));

	group.image(new Fl_Tiled_Image(new Fl_BMP_Image("floppy.bmp")));
	group.align(FL_ALIGN_TEXT_OVER_IMAGE);

	for(i=0;i<8;i++)
	{
		txt_buttons_main[i].button = new Fl_Button(BUTTON_XPOS, BUTTON_YPOS+(BUTTON_YSTEP*i), BUTTON_XSIZE, BUTTON_YSIZE, txt_buttons_main[i].label);
		txt_buttons_main[i].button->labelsize(12);
		txt_buttons_main[i].button->callback(bt_clicked,(void*)i);	

		Fl_Box *box = new Fl_Box(FL_NO_BOX,BUTTON_XPOS+BUTTON_XSIZE,BUTTON_YPOS+(BUTTON_YSIZE/4)+(BUTTON_YSTEP*i),BUTTON_XSIZE*4,BUTTON_YSIZE/2,txt_buttons_main[i].desc);
		box->align(FL_ALIGN_INSIDE|FL_ALIGN_LEFT);
		//box->labelfont(FL_BOLD);
		box->labelsize(11);
		box->labeltype(FL_EMBOSSED_LABEL);
	}

	file_name_txt = new Fl_Output(BUTTON_XPOS, BUTTON_YPOS+(BUTTON_YSTEP*i++), WINDOW_XSIZE-(BUTTON_XPOS*2), 25);
	file_name_txt->labelsize(12);
	file_name_txt->textsize(12);
	file_name_txt->align(FL_ALIGN_TOP_LEFT);
	file_name_txt->value("No disk loaded.");
	file_name_txt->box(FL_PLASTIC_UP_BOX);

	track_pos_str = new Fl_Output(BUTTON_XPOS, BUTTON_YPOS+(BUTTON_YSTEP*i++)-10, WINDOW_XSIZE-(BUTTON_XPOS*2), 25);
	track_pos_str->labelsize(12);
	track_pos_str->textsize(12);
	track_pos_str->align(FL_ALIGN_TOP_LEFT);
	track_pos_str->box(FL_PLASTIC_UP_BOX);

	track_pos = new Fl_Progress(BUTTON_XPOS, BUTTON_YPOS+(BUTTON_YSTEP*i++)-18, WINDOW_XSIZE-(BUTTON_XPOS*2), 25);
	track_pos->box(FL_THIN_UP_BOX);
	track_pos->selection_color((Fl_Color)137);
	track_pos->minimum(0);
	track_pos->maximum(255);
	track_pos->value(0);

	track_pos->box(FL_PLASTIC_UP_BOX);

	group.end();

	txt_buttons_main[0].button->callback(load_file_image,0);
	txt_buttons_main[4].button->callback(save_file_image,0);
		
	menutable[1].user_data_=fc_load;
	menutable[4].user_data_=fc_save;
	Fl_Menu_Bar menubar(0,0,WINDOW_XSIZE,24); 
	menubar.menu(menutable);

	// Fl_DND_Box is constructed with the same dimensions and at the same position as Fl_Scroll
	Fl_DND_Box *o = new Fl_DND_Box(0, 0,WINDOW_XSIZE, 400, 0);
	o->callback(dnd_cb);
	end();

	label(NOMFENETRE);
	//show(argc, argv);
	show();

	tick_main(this);

	user_data((void*)(this));

	load_last_cfg();

	//////////////////////////////////////////////
	// Floppy dump window
	this->fdump_window=new floppy_dump_window();
	this->fdump_window->double_sided->value(1);
	this->fdump_window->double_step->value(0);
	this->fdump_window->number_of_retry->value(10);
	this->fdump_window->number_of_track->value(80);
	this->fdump_window->sel_drive_a->value(1);
	this->fdump_window->sel_drive_b->value(0);
	guicontext->xsize=460;
	guicontext->ysize=200;
	guicontext->mapfloppybuffer=(unsigned char*)malloc(guicontext->xsize * guicontext->ysize * 4);
	memset(guicontext->mapfloppybuffer,0,guicontext->xsize*guicontext->ysize*4);
	tick_dump(this->fdump_window);
	
	//////////////////////////////////////////////
	// Floppy view window
	this->infos_window=new floppy_infos_window();	
	this->infos_window->x_offset->bounds(0.0, 100);
	this->infos_window->x_time->scrollvalue(300,1,1,1000);
	this->infos_window->y_time->scrollvalue(16,1,2,64);
	this->infos_window->track_view_bt->value(1);
	this->infos_window->disc_view_bt->value(0);
	this->infos_window->disc_view_bt->deactivate();
	guicontext->td=hxcfe_td_init(guicontext->hxcfe,this->infos_window->floppy_map_disp->w(),this->infos_window->floppy_map_disp->h());
	this->infos_window->buf=new Fl_Text_Buffer;
	this->infos_window->object_txt->buffer(this->infos_window->buf);

	tick_infos(this->infos_window);
	guicontext->updatefloppyinfos++;
	
	//////////////////////////////////////////////
	// Batch convert window
	this->batchconv_window=new batch_converter_window();
	batchconv_window->choice_file_format->menu(format_choices);
	batchconv_window->choice_file_format->value(0);

	//////////////////////////////////////////////
	// File system window
	this->fs_window=new filesystem_generator_window();
	fs_window->choice_filesystype->menu(fs_choices);
	fs_window->choice_filesystype->value(0);

	//////////////////////////////////////////////
	// Raw floppy window
	this->rawloader_window=new rawfile_loader_window();
	rawloader_window->choice_sectorsize->menu(sectorsize_choices);
	rawloader_window->choice_sectorsize->value(2);
	rawloader_window->choice_tracktype->menu(track_type_choices);
	rawloader_window->choice_tracktype->value(3);
	rawloader_window->chk_twosides->value(1);
	rawloader_window->chk_autogap3->value(1);
	rawloader_window->innum_rpm->value(300);
	rawloader_window->innum_sectoridstart->value(1);
	rawloader_window->innum_bitrate->value(250000);
	rawloader_window->innum_sectorpertrack->value(9);
	rawloader_window->numin_formatvalue->value(246);
	rawloader_window->innum_nbtrack->value(80);
	rawloader_window->numin_gap3->value(84);
	rawloader_window->numin_interleave->value(1);
	rawloader_window->numin_skew->value(0);
	raw_loader_window_datachanged(rawloader_window->numin_skew, 0);

	//////////////////////////////////////////////
	// Log window
	this->log_box=new Log_box();

	//////////////////////////////////////////////
	// SD FE CFG window
	guicontext->backlight_tmr = 20;
	guicontext->standby_tmr = 20;
	guicontext->lcd_scroll = 150;
	guicontext->step_sound = 0xD8;
	guicontext->ui_sound = 0x60;

	this->sdcfg_window=new sdhxcfecfg_window();
	sdcfg_window->choice_hfeifmode->menu(if_choices);
	sdcfg_window->slider_scrolltxt_speed->scrollvalue(0xFF-guicontext->lcd_scroll,1,0,(255-64));
	sdcfg_window->slider_stepsound_level->scrollvalue(0xFF-guicontext->step_sound,1,0x00, 0xFF-0xD8);
	sdcfg_window->slider_uisound_level->scrollvalue(guicontext->ui_sound,1,0,128);
	sdcfg_window->valslider_device_backlight_timeout->scrollvalue(guicontext->backlight_tmr,1,0,256);
	sdcfg_window->valslider_device_standby_timeout->scrollvalue(guicontext->standby_tmr,1,0,256);

	//////////////////////////////////////////////
	// USB FE CFG window

	libusbhxcfe_getStats(guicontext->hxcfe,guicontext->usbhxcfe,&stats,0);
	this->usbcfg_window=new usbhxcfecfg_window();
	usbcfg_window->choice_ifmode->menu(if_choices);
	usbcfg_window->slider_usbpacket_size->scrollvalue(stats.packetsize,128,512,4096-(512-128));
	usbcfg_window->rbt_ds0->value(1);

	//////////////////////////////////////////////
	// About window
	this->about_window=new About_box();

	sync_if_config();

	txtindex=0;
	tick_mw(this);

//	Fl::dnd_text_ops(1);
	Fl::run();
}



Main_Window::~Main_Window()
{
	Fl::remove_timeout(tick_main,0); 
}

