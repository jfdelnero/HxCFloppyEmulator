/*
//
// Copyright (C) 2006, 2007, 2008, 2009 Jean-François DEL NERO
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

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/filename.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Progress.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Tiled_Image.H>
#include <FL/Fl_File_Browser.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Native_File_Chooser.H>

#include <FL/Fl_File_Icon.H>
#include <FL/Fl_Shared_Image.H>
#include <FL/Fl_PNM_Image.H>
#include <FL/Fl_Light_Button.H>
#include <FL/Fl_BMP_Image.H>

#include <FL/fl_ask.H>

#include <errno.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h> 
#include <time.h>

#include "msg_txt.h"

#include "batch_converter_window.h"
#include "filesystem_generator_window.h"
#include "floppy_dump_window.h"
#include "rawfile_loader_window.h"
#include "sdhxcfecfg_window.h"
#include "usbhxcfecfg_window.h"
#include "log_gui.h"

#include "soft_cfg_file.h"

extern "C"
{
	#include "hxc_floppy_emulator.h"
	#include "./usb_floppyemulator/usb_hxcfloppyemulator.h"
	#include "version.h"
}

#include "loader.h"

#include "about_gui.h"
#include "main_gui.h"
#include "mainrouts.h"

Fl_File_Browser		*files;
Fl_Shared_Image		*image = 0;

extern FLOPPY * thefloppydisk;
extern HXCFLOPPYEMULATOR * flopemu;
extern guicontext * gui_context;
extern HWINTERFACE * hwif;
int backlight_tmr,standby_tmr,step_sound,ui_sound,lcd_scroll;
unsigned int txtindex;

typedef struct main_button_list_
{
	Fl_Button * button;
	char * label;
	char * desc;
}main_button_list;


main_button_list  txt_buttons_main[]=
{
	{0,"Load","Load a floppy file image"},
	{0,"Load Raw image","Load a custom raw floppy image /\ncreate a custom floppy"},
	{0,"Batch converter","Convert multiple floppy file image."},
	{0,"Create FS Floppy","Create a DOS/AmigaDOS\nfile system based floppy disk."},
	{0,"Export","Export/save the loaded file image"},
	{0,"SD HxC Floppy\nEmulator settings","Configure the SDCard HxC Floppy Emulator"},
	{0,"USB HxC Floppy\nEmulator settings","Configure the USB HxC Floppy Emulator"},
	{0,"Floppy disk dump","Read a real disk"}
};



void beepcb(Fl_Widget *, void *) {
  printf("\007"); fflush(stdout);
}


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
			//printf("ERROR: %s\n", fnfc.errmsg());
			break; // ERROR
		}
		case 1:
		{
			//printf("CANCEL\n");
			break; // CANCEL
		}
		default:
		{
			//printf("PICKED: %s\n", fnfc.filename());
			loadfloppy((char*)fnfc.filename());
			break; // FILE CHOSEN
		}
	}
}


void save_file_image(Fl_Widget * w, void * fc_ptr) 
{
	int i;
	Fl_Native_File_Chooser fnfc;
	
	const char * plugid_lst[]=
	{
		{PLUGIN_HXC_HFE},
		{PLUGIN_VTR_IMG},
		{PLUGIN_HXC_MFM},
		{PLUGIN_HXC_AFI},
		{PLUGIN_RAW_IMG},
		{PLUGIN_AMSTRADCPC_DSK},
		{PLUGIN_HXC_EXTHFE}
	};

	if(!thefloppydisk)
	{
		fl_alert("No floppy loaded !\nPlease drag and drop your disk file into the window\n",10.0);
	}
	else
	{
		fnfc.title("Export disk/Save As");
		fnfc.type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
		fnfc.filter("HFE file (SDCard HxC Floppy Emulator file format)\t*.hfe\n"
					"VTR file (VTrucco Floppy Emulator file format)\t*.vtr\n"
					"MFM file (MFM/FM track file format)\t*.mfm\n"
					"AFI file (Advanced File image format)\t*.afi\n"
					"IMG file (RAW Sector file format)\t*.img\n"
					"CPC DSK file\0*.dsk\0IMD file\t*.imd\n"
					"HFE file (Rev 2 - Experimental)\t*.hfe\n");


		//fnfc.directory("/var/tmp"); // default directory to use
		
		// Show native chooser
		switch ( fnfc.show() ) {
			case -1:
			{
				//printf("ERROR: %s\n", fnfc.errmsg());
				break; // ERROR
			}
			case 1:
			{
				//printf("CANCEL\n");
				break; // CANCEL
			}
			default:
			{
				i=fnfc.filter_value();
				if(hxcfe_selectContainer(flopemu,(char*)plugid_lst[i])==HXCFE_NOERROR)
				{
	//				hxcfe_floppyGetSetParams(flopemu,thefloppydisk,SET,DOUBLESTEP,&hwif->double_step);
	//				hxcfe_floppyGetSetParams(flopemu,thefloppydisk,SET,INTERFACEMODE,&hwif->interface_mode);
					hxcfe_floppyExport(flopemu,thefloppydisk,(char*)fnfc.filename());
				}

				//printf("PICKED: %s\n", fnfc.filename());
				//loadfloppy((char*)fnfc.filename());
				break; // FILE CHOSEN
			}
		}
	}
}


void load_file_image_pb(Fl_Widget * widget, void * ptr) 
{
	load_file_image(widget,ptr);
}


void export_file_image_pb(Fl_Widget * widget, void * ptr) 
{

	
}


void fc_callback(Fl_File_Chooser * fc, void *) 
{
	const char * fp;
	fp=fc->value();
}

void save_fc_callback(Fl_File_Chooser * fc, void *) 
{
	const char * fp;

	fp=fc->value();
	//loadfloppy((char*)fp,0);
}


#define BUTTON_XPOS 5
#define BUTTON_YPOS 30
#define BUTTON_XSIZE 100
#define BUTTON_YSIZE 35
#define BUTTON_YSTEP 35

#define WINDOW_XSIZE 341

Fl_Menu_Item menutable[] = {
  {"&Floppy image",0,0,0,FL_SUBMENU},
    {"&Load",	FL_ALT+'l',load_file_image_pb,0},
    {"Load custom RAW file/Create custom floppy",	FL_ALT+'r',menu_clicked,(void*)1},
    {"&Create DOS/AmigaDOS disk",	FL_ALT+'c',	menu_clicked,(void*)3},
    {"&Export disk/Save As",	FL_ALT+'e', save_file_image, 0},
    {"Batch convert files images",	FL_ALT+'o', menu_clicked,(void*)2},
	{"Dump a Floppy disk",	FL_ALT+'o', menu_clicked,(void*)7},
    {0},
  {"&Settings",FL_F+2,0,0,FL_SUBMENU},
    {"SDCard HxC FLoppy Emulator settings",	FL_ALT+'l',menu_clicked,(void*)5},
    {"USB HxC Floppy Emulator settings",	FL_ALT+'s',menu_clicked,(void*)6},
    {0},
  {"&Log",FL_F+3,0,0,FL_SUBMENU},
    {"&Log",	FL_ALT+'l',menu_clicked,(void*)8},
    {0},
  {"&About",0,0,0,FL_SUBMENU},
  {"&HxCFloppyEmulator",	FL_ALT+'h',create_about_window,0},
    {0},
  {0}
};


void format_choice_cb(Fl_Widget *, void *v) {
  //cursor = (Fl_Cursor)(long)v;
  //cursor_slider->value(cursor);
  //fl_cursor(cursor,fg,bg);
}

	enum
	{
		FS_720KB_ATARI_FAT12=0,
		FS_902KB_ATARI_FAT12,
		FS_360KB_ATARI_FAT12,
		FS_880KB_AMIGADOS,
			
		FS_5P25_300RPM_160KB_MSDOS_FAT12,
		FS_5P25_360RPM_160KB_MSDOS_FAT12,

		FS_5P25_300RPM_180KB_MSDOS_FAT12,
		FS_5P25_360RPM_180KB_MSDOS_FAT12,

		FS_5P25_SS_300RPM_320KB_MSDOS_FAT12,
		FS_5P25_SS_360RPM_320KB_MSDOS_FAT12,

		FS_5P25_DS_300RPM_320KB_MSDOS_FAT12,
		FS_5P25_DS_360RPM_320KB_MSDOS_FAT12,

		FS_5P25_DS_300RPM_360KB_MSDOS_FAT12,
		FS_5P25_DS_360RPM_360KB_MSDOS_FAT12,

		FS_3P5_DS_300RPM_640KB_MSDOS_FAT12,

		FS_720KB_MSDOS_FAT12,

		FS_5P25_300RPM_1200KB_MSDOS_FAT12,

		FS_1_44MB_MSDOS_FAT12,
		FS_1_68MB_MSDOS_FAT12,
		FS_2_88MB_MSDOS_FAT12,
		FS_3_38MB_MSDOS_FAT12,
		FS_4_23MB_ATARI_FAT12,
		FS_6_78MB_MSDOS_FAT12,
		FS_16MB_MSDOS_FAT12
	};



Fl_Menu_Item format_choices[]=
{
	{ "HFE - SDCard HxC Floppy Emulator file format",0,format_choice_cb,(void*)PLUGIN_HXC_HFE},
	{ "MFM - MFM/FM track file format",0,format_choice_cb,(void*)PLUGIN_HXC_MFM},
	{ "AFI - Advanced file image format",0,format_choice_cb,(void*)PLUGIN_HXC_AFI},
	{ "VTR - VTrucco Floppy Emulator file format",0,format_choice_cb,(void*)PLUGIN_VTR_IMG},
	{ "RAW - RAW sectors file format",0,format_choice_cb,(void*)PLUGIN_RAW_IMG},
	{ "IMD - IMD sectors file format",0,format_choice_cb,(void*)PLUGIN_IMD_IMG},
	{ "HFE - Rev 2 - Experimental",0,format_choice_cb,(void*)PLUGIN_HXC_EXTHFE},
	{0}
};


Fl_Menu_Item fs_choices[]=
{
	{ "5\"25 & 8\" 160KB SSDD 300RPM FAT12",0,format_choice_cb,(void*)FS_5P25_300RPM_160KB_MSDOS_FAT12},
	{ "5\"25 & 8\" 160KB SSDD 360RPM FAT12",0,format_choice_cb,(void*)FS_5P25_360RPM_160KB_MSDOS_FAT12},

	{ "5\"25       180KB SSDD 300RPM FAT12",0,format_choice_cb,(void*)FS_5P25_300RPM_180KB_MSDOS_FAT12},
	{ "5\"25       180KB SSDD 360RPM FAT12",0,format_choice_cb,(void*)FS_5P25_360RPM_180KB_MSDOS_FAT12},

	{ "5\"25       320KB SSDD 300RPM FAT12",0,format_choice_cb,(void*)FS_5P25_SS_300RPM_320KB_MSDOS_FAT12},
	{ "5\"25       320KB SSDD 360RPM FAT12",0,format_choice_cb,(void*)FS_5P25_SS_360RPM_320KB_MSDOS_FAT12},

	{ "5\"25       320KB DSDD 300RPM FAT12",0,format_choice_cb,(void*)FS_5P25_DS_300RPM_320KB_MSDOS_FAT12},
	{ "5\"25       320KB DSDD 360RPM FAT12",0,format_choice_cb,(void*)FS_5P25_DS_360RPM_320KB_MSDOS_FAT12},

	{ "5\"25 & 8\" 360KB DSDD 300RPM FAT12",0,format_choice_cb,(void*)FS_5P25_DS_300RPM_360KB_MSDOS_FAT12},
	{ "5\"25 & 8\" 360KB DSDD 360RPM FAT12",0,format_choice_cb,(void*)FS_5P25_DS_360RPM_360KB_MSDOS_FAT12},

	{ "3\"5        640KB DSDD FAT12",0,format_choice_cb,(void*)FS_3P5_DS_300RPM_640KB_MSDOS_FAT12},
		
	{ "3\"5        720KB DSDD FAT12 ",0,format_choice_cb,(void*)FS_720KB_MSDOS_FAT12},

	{ "5\"25       1.2MB DSHD FAT12",0,format_choice_cb,(void*)FS_5P25_300RPM_1200KB_MSDOS_FAT12},

	{ "3\"5        1.44MB DSHD FAT12",0,format_choice_cb,(void*)FS_1_44MB_MSDOS_FAT12},
	{ "3\"5        1.68MB DSHD FAT12",0,format_choice_cb,(void*)FS_1_68MB_MSDOS_FAT12},
	{ "3\"5        2.88MB DSED FAT12",0,format_choice_cb,(void*)FS_2_88MB_MSDOS_FAT12},
	{ "3\"5        3.38MB DSHD FAT12",0,format_choice_cb,(void*)FS_3_38MB_MSDOS_FAT12},
		
	{ "3\"5        6.78MB DSHD FAT12",0,format_choice_cb,(void*)FS_6_78MB_MSDOS_FAT12},

	{ "3\"5        360KB SSDD Atari FAT12",0,format_choice_cb,(void*)FS_360KB_ATARI_FAT12},
	{ "3\"5        720KB DSDD Atari FAT12",0,format_choice_cb,(void*)FS_720KB_ATARI_FAT12},
	{ "3\"5        902KB DSDD Atari FAT12",0,format_choice_cb,(void*)FS_902KB_ATARI_FAT12},
	{ "3\"5        4.23MB DSDD Atari FAT12",0,format_choice_cb,(void*)FS_4_23MB_ATARI_FAT12},

	{ "3\"5        880KB DSDD AmigaDOS",0,format_choice_cb,(void*)FS_880KB_AMIGADOS},
		

	{0}			
};

Fl_Menu_Item sectorsize_choices[]=
{
	{ "128",0,raw_loader_window_datachanged,(void*)128},
	{ "256",0,raw_loader_window_datachanged,(void*)256},
	{ "512",0,raw_loader_window_datachanged,(void*)512},
	{ "1024",0,raw_loader_window_datachanged,(void*)1024},
	{ "2048",0,raw_loader_window_datachanged,(void*)2048},
	{ "4096",0,raw_loader_window_datachanged,(void*)4096},
	{ "8192",0,raw_loader_window_datachanged,(void*)8192},
	{ "16384",0,raw_loader_window_datachanged,(void*)16384},
	{0}
};


Fl_Menu_Item if_choices[]=
{
	{ "Amiga",0,format_choice_cb,(void*)AMIGA_DD_FLOPPYMODE},
	{ "Amiga HD",0,format_choice_cb,(void*)AMIGA_HD_FLOPPYMODE},
	{ "Atari ST",0,format_choice_cb,(void*)ATARIST_DD_FLOPPYMODE},
	{ "Atari ST HD",0,format_choice_cb,(void*)ATARIST_HD_FLOPPYMODE},
	{ "IBM PC 720kB",0,format_choice_cb,(void*)IBMPC_DD_FLOPPYMODE},
	{ "IBM PC 1.44MB",0,format_choice_cb,(void*)IBMPC_HD_FLOPPYMODE},
	{ "IBM PC 2.88MB",0,format_choice_cb,(void*)IBMPC_ED_FLOPPYMODE},
	{ "Amstrad CPC",0,format_choice_cb,(void*)CPC_DD_FLOPPYMODE},
	{ "MSX 2",0,format_choice_cb,(void*)MSX2_DD_FLOPPYMODE},
	{ "Generic Shugart",0,format_choice_cb,(void*)GENERIC_SHUGART_DD_FLOPPYMODE},
	{ "Emu Shugart",0,format_choice_cb,(void*)EMU_SHUGART_FLOPPYMODE},
	{ "C64 1541",0,format_choice_cb,(void*)C64_DD_FLOPPYMODE},

	{0}
};


Fl_Menu_Item track_type_choices[]=
{
	{ "FM",0,raw_loader_window_datachanged,(void*)ISOFORMAT_SD},
	{ "IBM FM",0,raw_loader_window_datachanged,(void*)IBMFORMAT_SD},
	{ "MFM",0,raw_loader_window_datachanged,(void*)ISOFORMAT_DD},
	{ "IBM MFM",0,raw_loader_window_datachanged,(void*)IBMFORMAT_DD},
	{0}
};

/*
platform platformlist[]=
{
	{ AMIGA_DD_FLOPPYMODE,"Amiga","DS0","DS1","DS2","MTRON"},
	{ AMIGA_HD_FLOPPYMODE,"Amiga HD","DS0","DS1","DS2","MTRON"},
	{ ATARIST_DD_FLOPPYMODE,"Atari ST","D0SEL","D1SEL","-","MTRON"},
	{ ATARIST_HD_FLOPPYMODE,"Atari ST HD","D0SEL","D1SEL","-","MTRON"},
	{ IBMPC_DD_FLOPPYMODE,"IBM PC 720kB","MOTEA","DRVSB","DRVSA","MOTEB"},
	{ IBMPC_HD_FLOPPYMODE,"IBM PC 1.44MB","MOTEA","DRVSB","DRVSA","MOTEB"},
	{ IBMPC_ED_FLOPPYMODE,"IBM PC 2.88MB","MOTEA","DRVSB","DRVSA","MOTEB"},
	{ CPC_DD_FLOPPYMODE,"Amstrad CPC","Drive Select 0","Drive Select 1","-","MOTOR ON"},
	{ MSX2_DD_FLOPPYMODE,"MSX 2","DS0","DS1","DS2","MTRON"},
	{ GENERIC_SHUGART_DD_FLOPPYMODE,"Generic Shugart","DS0","DS1","DS2","MTRON"},
	{ EMU_SHUGART_FLOPPYMODE,"Emu Shugart","DS0","DS1","DS2","MTRON"},
	{ C64_DD_FLOPPYMODE,"C64 1541","NA","NA","NA","NA"},
	{ -1,"?","DS0","DS1","DS2","MTRON"}
};

*/

void cb_read_disk(class Fl_Button *,void *)
{


}

void cb_ok(class Fl_Button *,void *)
{


}
////////////////////////////////////////////////////////////

class Fl_DND_Box : public Fl_Box
{
    public:

        static void callback_deferred(void *v)
        {
            Fl_DND_Box *w = (Fl_DND_Box*)v;

            w->do_callback();
        }

        Fl_DND_Box(int X, int Y, int W, int H, const char *L = 0)
                : Fl_Box(X,Y,W,H,L), evt(FL_NO_EVENT), evt_txt(0), evt_len(0)
        {
            labeltype(FL_NO_LABEL);
            box(FL_NO_BOX);
            clear_visible_focus();
        }

        virtual ~Fl_DND_Box()
        {
            delete [] evt_txt;
        }

        int event()
        {
            return evt;
        }

        const char* event_text()
        {
            return evt_txt;
        }

        int event_length()
        {
            return evt_len;
        }

        int handle(int e)
        {
            switch(e)
            {
                case FL_DND_ENTER:
                case FL_DND_RELEASE:
                case FL_DND_LEAVE:
                case FL_DND_DRAG:
                    evt = e;
                    return 1;


                case FL_PASTE:
                    evt = e;

                    // make a copy of the DND payload
                    evt_len = Fl::event_length();

                    //delete [] evt_txt;

                    evt_txt = new char[evt_len];
                    strcpy(evt_txt, Fl::event_text());

                    // If there is a callback registered, call it.
                    // The callback must access Fl::event_text() to
                    // get the string or file path that was dropped.
                    // Note that do_callback() is not called directly.
                    // Instead it will be executed by the FLTK main-loop
                    // once we have finished handling the DND event.
                    // This allows caller to popup a window or change widget focus.
                    if(callback() && ((when() & FL_WHEN_RELEASE) || (when() & FL_WHEN_CHANGED)))
                        Fl::add_timeout(0.0, Fl_DND_Box::callback_deferred, (void*)this);
                    return 1;
            }

            return Fl_Box::handle(e);
        }

    protected:
        // The event which caused Fl_DND_Box to execute its callback
        int evt;

        char *evt_txt;
        int evt_len;
};

// Widget that displays the image
Fl_Box *box = (Fl_Box*)0;




void dnd_open(const char *urls)
{
	loadfloppy((char*)urls);
}

void dnd_cb(Fl_Widget *o, void *v)
{
    Fl_DND_Box *dnd = (Fl_DND_Box*)o;

    if(dnd->event() == FL_PASTE)
        dnd_open(dnd->event_text());
}
////////////////////////////////////////////////////////////

static void tick_mw(void *v) {
	Main_Window *window;
	int i,j;
	char tempstr[1024];
	char tempstr2[1024];

	window=(Main_Window *)v;

	if(strlen(gui_context->bufferfilename))
	{
		sprintf(tempstr,"%s - %d track(s) %d side(s)     ",gui_context->bufferfilename,thefloppydisk->floppyNumberOfTrack,thefloppydisk->floppyNumberOfSide);
		if(txtindex>=strlen(tempstr))txtindex=0;
				
		i=0;
		j=txtindex;
		do
		{
			tempstr2[i]=tempstr[j];
			i++;
			j++;
			if(j>=strlen(tempstr)) j=0;
		}while(i<strlen(tempstr));
		tempstr2[i]=0;
		memcpy(&tempstr2[i],tempstr2,strlen(tempstr2));
			
		window->file_name_txt->value((const char*)tempstr2);
		//SetDlgItemText(hwndDlg,IDC_EDIT_STATUS,tempstr2);
			
		txtindex++;
	}
	else
	{
		if(gui_context->loadstatus!=HXCFE_NOERROR)
		{
			switch(gui_context->loadstatus)
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
					sprintf(tempstr2,"Load error! error %d",gui_context->loadstatus);
					window->file_name_txt->value((const char*)tempstr2);
				break;
			}
		}
	}
			
	if(thefloppydisk)
	{
		if(hwif)
		{
			sprintf(tempstr,"Track %d/%d",hwif->current_track,thefloppydisk->floppyNumberOfTrack);
			window->track_pos_str->value((const char*)tempstr);

			window->track_pos->minimum(0);

			window->track_pos->maximum(thefloppydisk->floppyNumberOfTrack);
			window->track_pos->value((int)hwif->current_track);
		}
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

	Fl::scheme("gtk+");

	Fl_Group group(0,0,WINDOW_XSIZE,392);

	group.image(new Fl_Tiled_Image(new Fl_BMP_Image("floppy.bmp")));
	group.align(FL_ALIGN_TEXT_OVER_IMAGE);

	for(i=0;i<8;i++)
	{
		txt_buttons_main[i].button = new Fl_Button(BUTTON_XPOS, BUTTON_YPOS+(BUTTON_YSTEP*i), BUTTON_XSIZE, BUTTON_YSIZE, txt_buttons_main[i].label);
		txt_buttons_main[i].button->labelsize(12);
		txt_buttons_main[i].button->callback(bt_clicked,(void*)i);	

		Fl_Box *box = new Fl_Box(FL_NO_BOX,BUTTON_XPOS+BUTTON_XSIZE,BUTTON_YPOS+(BUTTON_YSIZE/4)+(BUTTON_YSTEP*i),BUTTON_XSIZE*4,BUTTON_YSIZE/2,txt_buttons_main[i].desc);
		box->align(FL_ALIGN_INSIDE|FL_ALIGN_LEFT);
		box->labelfont(FL_BOLD);
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

	this->fdump_window=new floppy_dump_window();
	
	this->batchconv_window=new batch_converter_window();
	batchconv_window->choice_file_format->menu(format_choices);
	batchconv_window->choice_file_format->value(0);

	this->fs_window=new filesystem_generator_window();
	fs_window->choice_filesystype->menu(fs_choices);
	fs_window->choice_filesystype->value(0);

	this->rawloader_window=new rawfile_loader_window();
	rawloader_window->choice_sectorsize->menu(sectorsize_choices);
	rawloader_window->choice_sectorsize->value(2);
	rawloader_window->choice_tracktype->menu(track_type_choices);
	rawloader_window->choice_tracktype->value(3);
	rawloader_window->innum_rpm->value(300);
	rawloader_window->innum_sectoridstart->value(1);
	rawloader_window->innum_bitrate->value(250000);
	rawloader_window->innum_sectorpertrack->value(9);
	rawloader_window->numin_formatvalue->value(246);
	rawloader_window->innum_nbtrack->value(80);
	rawloader_window->numin_gap3->value(84);
	rawloader_window->numin_interleave->value(1);
	rawloader_window->numin_skew->value(0);

	this->log_box=new Log_box();

	backlight_tmr=20;
	standby_tmr=20;
	lcd_scroll=150;
	step_sound=0xD8;
	ui_sound=0x60;

	this->sdcfg_window=new sdhxcfecfg_window();
	sdcfg_window->choice_hfeifmode->menu(if_choices);
	sdcfg_window->slider_scrolltxt_speed->scrollvalue(0xFF-lcd_scroll,1,0,(255-64));
	sdcfg_window->slider_stepsound_level->scrollvalue(0xFF-step_sound,1,0x00, 0xFF-0xD8);
	sdcfg_window->slider_uisound_level->scrollvalue(ui_sound,1,0,128);
	sdcfg_window->valslider_device_backlight_timeout->scrollvalue(backlight_tmr,1,0,256);
	sdcfg_window->valslider_device_standby_timeout->scrollvalue(standby_tmr,1,0,256);
	
	this->usbcfg_window=new usbhxcfecfg_window();
	usbcfg_window->choice_ifmode->menu(if_choices);
	usbcfg_window->slider_process_priority->scrollvalue(0,1,0,5);


	txtindex=0;
	tick_mw(this);

//	Fl::dnd_text_ops(1);
	Fl::run();
	
	
}



Main_Window::~Main_Window()
{
	Fl::remove_timeout(tick_main,0); 
}

