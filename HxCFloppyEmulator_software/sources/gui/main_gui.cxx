/*
//
// Copyright (C) 2006-2023 Jean-François DEL NERO
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
// Written by: Jean-François DEL NERO
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include "fl_includes.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <stdint.h>

#include "sdhxcfe_cfg.h"
#include "batch_converter_window.h"
#include "filesystem_generator_window.h"
#include "cb_filesystem_generator_window.h"

#include "floppy_dump_window.h"
#include "floppy_infos_window.h"
#include "floppy_streamer_window.h"
#include "rawfile_loader_window.h"
#include "sdhxcfecfg_window.h"
#include "usbhxcfecfg_window.h"
#include "edittool_window.h"
#include "log_gui.h"
#include "about_gui.h"
#include "parameters_gui.h"

#include "soft_cfg_file.h"
#include "fl_dnd_box.h"

#include "gui_strings.h"

#include "plugins_id.h"

#ifdef WIN32
#include "win32/resource.h"
 #if !defined(__MINGW32__) && !defined(__MINGW64__)
  #define intptr_t int
 #endif
#endif

extern "C"
{
	#include "microintro/data/data_bmp_hxc2001_backgnd_bmp.h"
	#include "microintro/data/data_COPYING_FULL.h"
	#include "microintro/data/data_bmp_pauline_bmp.h"
	#include "microintro/data/data_bmp_hxc2001_2_bmp.h"

	#include "microintro/packer/pack.h"

	extern void convert8b24b(bmaptype * img,unsigned short transcolor);

	#include "version.h"
}

#include "libhxcfe.h"
#include "libhxcadaptor.h"
#include "usb_hxcfloppyemulator.h"

#include "loader.h"
#include "main_gui.h"
#include "main.h"
#include "utils.h"

#include "cb_floppy_dump_window.h"
#include "cb_floppy_infos_window.h"
#include "cb_floppy_streamer_window.h"

#include "msg_txt.h"

extern s_gui_context * guicontext;

char * license_txt;
bmaptype * pauline_bmp;
bmaptype * hxc2001_2_bmp;

const char * plugid_lst[]=
{
	PLUGIN_HXC_HFE,
	PLUGIN_VTR_IMG,
	PLUGIN_HXC_MFM,
	PLUGIN_HXC_AFI,
	PLUGIN_RAW_LOADER,
	PLUGIN_AMSTRADCPC_DSK,
	PLUGIN_ORIC_DSK,
	PLUGIN_IMD_IMG,
	PLUGIN_AMIGA_ADF,
	PLUGIN_AMIGA_ADZ,
	PLUGIN_TI994A_V9T9,
	PLUGIN_TRS80_JV3,
	PLUGIN_TRS80_DMK,
	PLUGIN_DRAGON3264_VDK,
	PLUGIN_ZXSPECTRUM_TRD,
	PLUGIN_SPECCYSDD,
	PLUGIN_NEC_D88,
	PLUGIN_ATARIST_ST,
	PLUGIN_ATARIST_MSA,
	PLUGIN_ATARIST_DIM,
	PLUGIN_ATARIST_STX,
	PLUGIN_ATARIST_STW,
	PLUGIN_THOMSON_FD,
	PLUGIN_HXC_HDDD_A2,
	PLUGIN_HXC_EXTHFE,
	PLUGIN_HXC_HFEV3,
	PLUGIN_HXC_HFESTREAM,
	PLUGIN_ARBURG,
	PLUGIN_SKF,
	PLUGIN_IPF,
	PLUGIN_SCP,
	PLUGIN_BMP,
	PLUGIN_STREAM_BMP,
	PLUGIN_DISK_BMP,
	PLUGIN_GENERIC_XML,
	PLUGIN_NORTHSTAR,
	PLUGIN_HEATHKIT,
	PLUGIN_HXC_QD,
	PLUGIN_APPLE2_DO,
	PLUGIN_APPLE2_PO,
	PLUGIN_FDX68_FDX
};

#ifdef GUI_DEBUG
void print_dbg(char * str)
{
	printf("%s\n",str);
	fflush(stdout);
}
#endif

void save_ui_state(HXCFE* hxcfe)
{
	char * savefilepath;
	char * param_name;
	char tmp_value[512];
	int i;
	FILE *f;

	savefilepath = hxcfe_getEnvVar( hxcfe, (char*)"UISTATE_SAVE_FILE", 0 );
	if(savefilepath)
	{
		if(strlen(savefilepath))
		{
			f = hxc_fopen(savefilepath,"wb");
			if(f)
			{
				fprintf(f,"#\n# HxC Floppy Emulator user interface save file\n#\n\n");

				i = 0;
				while( hxcfe_getEnvVarIndex( hxcfe, i, NULL ) )
				{
					tmp_value[0] = 0;
					param_name = hxcfe_getEnvVarIndex( hxcfe, i, (char*)&tmp_value );
					if(param_name)
					{
						if( !strncmp(param_name,"LASTSTATE_", 10) )
						{
							fprintf(f,"set %s \"%s\"\n", param_name,tmp_value  );
						}
					}
					i++;
				}

				fprintf(f,"\n");

				fclose(f);
			}
		}
	}
}

void menu_clicked(Fl_Widget * w, void * fc_ptr)
{
	intptr_t i;
	Main_Window *mw;
	Fl_Window *dw;
	i=(intptr_t)fc_ptr;

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

		case 11:
			Fl::scheme("none");
		break;
		case 12:
			Fl::scheme("plastic");
		break;
		case 13:
			Fl::scheme("gtk+");
		break;
		case 14:
			Fl::scheme("gleam");
		break;
		case 15:
			mw->parameters_box = new Parameters_box();

			mw->parameters_box->show();
		break;
		case 16:
			mw->streamer_window->window->show();
		break;

	}

}

void bt_clicked(Fl_Widget * w, void * fc_ptr)
{
	intptr_t i;
	Main_Window *mw;
	Fl_Window *dw;
	i=(intptr_t)fc_ptr;

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
			mw->infos_window->window->show();
		break;
	}
}

typedef struct _exportthread
{
	char * urls;
	HXCFE_FLOPPY * floppy;
	char * type;
}exportthread;

int export_thread(void* floppycontext,void* context)
{
	exportthread * exportth;
	HXCFE_IMGLDR * imgldr_ctx;
	HXCFE_FLOPPY * fp;
	int loaderid,i;
	char filepath[512];
	char * extptr;

	exportth = (exportthread*)context;

	imgldr_ctx = hxcfe_imgInitLoader(guicontext->hxcfe);
	if(imgldr_ctx)
	{
		guicontext->exporting++;

		guicontext->loadingprogess = 0;
		hxcfe_imgSetProgressCallback(imgldr_ctx,progress_callback,(void*)guicontext);

		loaderid = hxcfe_imgGetLoaderID(imgldr_ctx,(char*)exportth->type);

		if(loaderid>=0)
		{
			fp = hxcfe_floppyDuplicate(guicontext->hxcfe,guicontext->loadedfloppy);
			if(fp)
			{
				if(!guicontext->autoselectmode)
				{
					hxcfe_floppySetInterfaceMode(guicontext->hxcfe,fp,guicontext->interfacemode);
				}

				strcpy(filepath,exportth->urls);

				extptr = (char*)hxcfe_imgGetLoaderExt( imgldr_ctx, loaderid );
				if(extptr)
				{


					i = strlen(filepath);
					while( i && filepath[i]!='.' && filepath[i]!='\\' && filepath[i]!='/')
					{
						i--;
					}

					if( i && filepath[i] != '.' )
					{
						// no file extension found. Add the default module extension.
						strcat(filepath,".");
						strcat(filepath,extptr);
					}
				}

				hxcfe_floppySetDoubleStep(guicontext->hxcfe,fp,guicontext->doublestep);
				hxcfe_imgExport(imgldr_ctx,fp,(char*)filepath,loaderid);
				hxcfe_imgUnload(imgldr_ctx,fp);
			}
		}

		hxcfe_imgDeInitLoader(imgldr_ctx);

		guicontext->exporting--;
	}

	free(exportth->type);
	free(exportth->urls);
	free(exportth);

	return 0;
}

int launchexport(char * urls,HXCFE_FLOPPY * fp, char * type)
{
	exportthread * exportthread_params;

	exportthread_params = (exportthread *)malloc(sizeof(exportthread));
	if(exportthread_params)
	{
		if(strlen(urls))
		{
			exportthread_params->urls =(char*)malloc(strlen(urls)+1);
			memset(exportthread_params->urls,0,strlen(urls)+1);
			memcpy(exportthread_params->urls,urls,strlen(urls));

			exportthread_params->type =(char*)malloc(strlen(type)+1);
			memset(exportthread_params->type,0,strlen(type)+1);
			memcpy(exportthread_params->type,type,strlen(type));

			exportthread_params->floppy = fp;

			hxc_createthread(guicontext->hxcfe,(void*)exportthread_params,&export_thread,0);
		}

	}

	return 0;
}

typedef struct _loadthread
{
	char * urls;
}loadthread;

int loading_thread(void* floppycontext,void* context)
{
	loadthread * loadth;

	loadth = (loadthread*)context;
	load_floppy_image(loadth->urls);

	guicontext->updatefloppyinfos++;
	guicontext->updatefloppyfs++;

	guicontext->loading = 0;

	free(loadth->urls);
	free(loadth);

	return 0;
}

void load_file(const char *urls)
{
	loadthread * loadthread_params;

	if(!guicontext->loading)
	{
		loadthread_params = (loadthread *)malloc(sizeof(loadthread));
		if(loadthread_params)
		{
			guicontext->loading = 1;

			if(strlen(urls))
			{
				loadthread_params->urls =(char*)malloc(strlen(urls)+1);
				memset(loadthread_params->urls,0,strlen(urls)+1);
				memcpy(loadthread_params->urls,urls,strlen(urls));
			}
			else
				loadthread_params->urls = 0;

			hxc_createthread(guicontext->hxcfe,(void*)loadthread_params,&loading_thread,0);
		}
		else
		{
			guicontext->loading = 0;
		}
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
#ifdef STANDALONEFSBROWSER
			write_back_fileimage();
#endif
			load_file((char*)fnfc.filename());

			break; // FILE CHOSEN
		}
	}
}

void save_file_image(Fl_Widget * w, void * fc_ptr)
{
	int i,keepsrcext;
	Fl_Native_File_Chooser fnfc;
	unsigned char deffilename[DEFAULT_TEXT_BUFFER_SIZE + 128];

	if(!guicontext->loadedfloppy)
	{
		fl_alert("No floppy loaded !\nPlease drag and drop your disk file into the window\n");
	}
	else
	{
		keepsrcext = hxcfe_getEnvVarValue( guicontext->hxcfe, (char*)"BATCHCONVERT_KEEP_SOURCE_FILE_NAME_EXTENSION");

		snprintf((char*)deffilename, sizeof(deffilename), "%s", guicontext->bufferfilename);

		i=0;
		while(deffilename[i]!=0)
		{
			if(deffilename[i]=='.')
			{
				if(keepsrcext)
					deffilename[i] = '_';
				else
					deffilename[i] = 0;
			}
			i++;
		}

		fnfc.title("Export disk/Save As");
		fnfc.preset_file((char*)deffilename);
		fnfc.type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
		fnfc.options(Fl_Native_File_Chooser::SAVEAS_CONFIRM|Fl_Native_File_Chooser::NEW_FOLDER|Fl_Native_File_Chooser::USE_FILTER_EXT);
		fnfc.filter("HFE file (SDCard HxC Floppy Emulator file format)\t*.hfe\n"
					"VTR file (VTrucco Floppy Emulator file format)\t*.vtr\n"
					"MFM file (MFM/FM track file format)\t*.mfm\n"
					"AFI file (Advanced File image format)\t*.afi\n"
					"IMG file (RAW Sector file format)\t*.img\n"
					"CPC DSK file\t*.dsk\n"
					"Oric DSK file\t*.dsk\n"
					"IMD file\t*.imd\n"
					"ADF file\t*.adf\n"
					"ADZ file\t*.adz\n"
					"TI99/4A V9T9 DSK file\t*.dsk\n"
					"TRS80 JV3 file\t*.jv3\n"
					"TRS80 DMK file\t*.dmk\n"
					"Dragon VDK file\t*.vdk\n"
					"Zx Spectrum TRD file\t*.trd\n"
					"Speccy SDD file\t*.sdd\n"
					"PC88  D88 file\t*.d88\n"
					"ATARI ST ST file\t*.st\n"
					"ATARI ST MSA file\t*.msa\n"
					"ATARI ST DIM file\t*.dim\n"
					"ATARI ST STX file\t*.stx\n"
					"ATARI ST STW file\t*.stw\n"
					"Thomson FD file\t*.fd\n"
					"HFE file (HDDD A2 Encoding support)\t*.hfe\n"
					"HFE file (Rev 2 - Experimental)\t*.hfe\n"
					"HFE file (Rev 3 - Experimental)\t*.hfe\n"
					"HFE file (Stream - Experimental)\t*.hfe\n"
					"Arburg file\t*.arburgfd\n"
					"KF Stream file\t*.raw\n"
					"SPS IPF file (WIP)\t*.ipf\n"
					"SCP file\t*.scp\n"
					"Tracks BMP file\t*.bmp\n"
					"Stream Tracks BMP file\t*.bmp\n"
					"Disk BMP file\t*.bmp\n"
					"XML file\t*.xml\n"
					"NSI file\t*.nsi\n"
					"H8D file\t*.h8d\n"
					"QD file (Quickdisk HxC Floppy Emulator file format)\t*.qd\n"
					"Apple II DO (Dos 3.3) file\t*.do\n"
					"Apple II PO (ProDos) file\t*.po\n"
					"FDX68 file (raw)\t*.fdx\n"
					);


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

				launchexport((char*)fnfc.filename(),guicontext->loadedfloppy,(char*)plugid_lst[i]);

				break; // FILE CHOSEN
			}
		}
	}
}

void execute_script(Fl_Widget * w, void * fc_ptr)
{
	Fl_Native_File_Chooser fnfc;

	fnfc.title("Execute script");
	fnfc.type(Fl_Native_File_Chooser::BROWSE_FILE);
	fnfc.filter("Script file\t*.script\n");

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
			hxcfe_execScriptFile( guicontext->hxcfe, (char*)fnfc.filename() );
			break; // FILE CHOSEN
		}
	}
}

void execute_script_pb(Fl_Widget * widget, void * ptr)
{
	execute_script(widget,ptr);
}

void load_file_image_pb(Fl_Widget * widget, void * ptr)
{
	load_file_image(widget,ptr);
}

#define BUTTON_XPOS 5
#define BUTTON_YPOS 30
#define BUTTON_XSIZE 110
#define BUTTON_YSIZE 35
#define BUTTON_YSTEP 35

#define WINDOW_XSIZE 345

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

#if !defined(STANDALONEFSBROWSER) && !defined(HXC_STREAMER_MODE)
	if(guicontext->usbhxcfe)
		libusbhxcfe_setInterfaceMode(guicontext->hxcfe,guicontext->usbhxcfe,guicontext->interfacemode,guicontext->doublestep,guicontext->driveid);
#endif
}

void format_choice_cb(Fl_Widget *, void *v)
{
	guicontext->interfacemode =  (intptr_t)v;

	sync_if_config();
}


void dnd_cb(Fl_Widget *o, void *v)
{
	char * dnd_str,*path;
	Fl_DND_Box *dnd = (Fl_DND_Box*)o;

	if(dnd->event() == FL_PASTE)
	{
		if(strlen(dnd->event_text()))
		{
			dnd_str = (char*)malloc(strlen(dnd->event_text())+1);
			if(dnd_str)
			{
				strcpy(dnd_str,dnd->event_text());

				if(strchr(dnd_str,'\n'))
				{
					*strchr(dnd_str,'\n') = 0;
				}

				path = URIfilepathparser((char*)dnd_str,strlen(dnd_str));
				if(path)
				{
					load_file(path);
					free(path);
				}

				free(dnd_str);
			}
		}
	}
}

static void tick_mw(void *v) {
	Main_Window *window;
	int i,j;
	char tempstr[DEFAULT_TEXT_BUFFER_SIZE + 128];
	char tempstr2[DEFAULT_TEXT_BUFFER_SIZE + 128];

	window=(Main_Window *)v;

	if(guicontext->loadedfloppy && strlen(guicontext->bufferfilename))
	{
		snprintf(tempstr, sizeof(tempstr), "%s - %d track(s) %d side(s)     ",guicontext->bufferfilename,hxcfe_getNumberOfTrack(guicontext->hxcfe,guicontext->loadedfloppy),hxcfe_getNumberOfSide(guicontext->hxcfe,guicontext->loadedfloppy));

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
		j = strlen(tempstr2);
		memcpy(&tempstr2[i],tempstr2,j);
		tempstr2[i + j] = 0;

		window->file_name_txt->value((const char*)tempstr2);

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
					snprintf(tempstr2, sizeof(tempstr2), "Load error! error %d",guicontext->loadstatus);
					window->file_name_txt->value((const char*)tempstr2);
				break;
			}
		}
	}

#if !defined(STANDALONEFSBROWSER) && !defined(HXC_STREAMER_MODE)
	if(!guicontext->exporting)
	{

		if(guicontext->loading)
		{
			snprintf(tempstr,sizeof(tempstr),"Loading : %s",guicontext->bufferfilename);

			window->file_name_txt->value((const char*)tempstr);
			window->track_pos->minimum(0);
			window->track_pos->maximum( (float)100);
			window->track_pos->value(   (float)guicontext->loadingprogess);
		}
		else
		{
			if(guicontext->loadedfloppy)
			{
				snprintf(tempstr,sizeof(tempstr),"Track %d/%d",libusbhxcfe_getCurTrack(guicontext->hxcfe,guicontext->usbhxcfe),hxcfe_getNumberOfTrack(guicontext->hxcfe,guicontext->loadedfloppy));

				window->track_pos_str->value((const char*)tempstr);
				window->track_pos->minimum(0);
				window->track_pos->maximum( (float)hxcfe_getNumberOfTrack(guicontext->hxcfe,guicontext->loadedfloppy) );
				window->track_pos->value((float)libusbhxcfe_getCurTrack(guicontext->hxcfe,guicontext->usbhxcfe));
			}
		}
	}
	else
	{
		snprintf(tempstr,sizeof(tempstr),"Exporting : %s",guicontext->bufferfilename);
		window->file_name_txt->value((const char*)tempstr);
		//window->track_pos->color(0x000000FF);
		window->track_pos->minimum(0);
		window->track_pos->maximum( (float)100);
		window->track_pos->value(   (float)guicontext->loadingprogess);
	}

#endif

	if(guicontext->autoselectmode)
	{
		if(!window->sdcfg_window->chk_hfr_autoifmode->value())
			window->sdcfg_window->chk_hfr_autoifmode->value(1);

		if(!window->usbcfg_window->chk_autoifmode->value())
			window->usbcfg_window->chk_autoifmode->value(1);

		window->usbcfg_window->choice_ifmode->deactivate();
		window->sdcfg_window->choice_hfeifmode->deactivate();

		i=0;
		while( if_choices[i].text && ((intptr_t)if_choices[i].user_data_ != guicontext->interfacemode))
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
		while( if_choices[i].text && ((intptr_t)if_choices[i].user_data_ != guicontext->interfacemode))
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

	if(window->sdcfg_window->choice_interfacemode_drva_cfg->value() != 13)
	{
		window->sdcfg_window->choice_pin02_drva->deactivate();
		window->sdcfg_window->choice_pin34_drva->deactivate();
		window->sdcfg_window->choice_pin02_drva->value(0);
		window->sdcfg_window->choice_pin34_drva->value(0);
	}
	else
	{
		window->sdcfg_window->choice_pin02_drva->activate();
		window->sdcfg_window->choice_pin34_drva->activate();
	}

	if( !window->sdcfg_window->chk_enable_twodrives_emu->value() )
	{
		window->sdcfg_window->choice_interfacemode_drvb_cfg->value( window->sdcfg_window->choice_interfacemode_drva_cfg->value() );
		window->sdcfg_window->choice_pin02_drvb->value(window->sdcfg_window->choice_pin02_drva->value());
		window->sdcfg_window->choice_pin34_drvb->value(window->sdcfg_window->choice_pin34_drva->value());

		window->sdcfg_window->choice_interfacemode_drvb_cfg->deactivate();
		window->sdcfg_window->choice_pin02_drvb->deactivate();
		window->sdcfg_window->choice_pin34_drvb->deactivate();
	}
	else
	{
		window->sdcfg_window->choice_interfacemode_drvb_cfg->activate();
	}

	if(window->sdcfg_window->choice_interfacemode_drvb_cfg->value() != 13 )
	{
		window->sdcfg_window->choice_pin02_drvb->deactivate();
		window->sdcfg_window->choice_pin34_drvb->deactivate();
		window->sdcfg_window->choice_pin02_drvb->value(0);
		window->sdcfg_window->choice_pin34_drvb->value(0);
	}
	else
	{
		if( window->sdcfg_window->chk_enable_twodrives_emu->value() )
		{
			window->sdcfg_window->choice_pin02_drvb->activate();
			window->sdcfg_window->choice_pin34_drvb->activate();
		}
		else
		{
			window->sdcfg_window->choice_pin02_drvb->deactivate();
			window->sdcfg_window->choice_pin34_drvb->deactivate();
		}
	}

	window->batchconv_window->progress_indicator->redraw();
	window->batchconv_window->strout_convert_status->redraw();
	window->batchconv_window->bt_convert->redraw();

	Fl::repeat_timeout(0.10, tick_mw, v);
}

Main_Window::Main_Window()
  : Fl_Window(WINDOW_XSIZE,428)
{
	int i,j;
	HXCFE_XMLLDR* rfb;
	char * temp;
#ifndef HXC_STREAMER_MODE
	infothread * infoth;
#endif
	streamthread * streamth;
	USBStats stats;
	intptr_t tmp_intptr;

	txtindex=0;
	i=0;
	evt_txt=0;

	#ifdef GUI_DEBUG
	print_dbg((char*)"Main_Window : Entering Main_Window");
	#endif

	// First lock() call to Enable lock/unlock threads support.
	Fl::lock();
	Fl::visual(FL_RGB);

	hxc2001_2_bmp = NULL;

	guicontext->loading = 0;
	guicontext->exporting = 0;

	guicontext->loadedfloppy=0;
	guicontext->autoselectmode=0xFF;
	guicontext->driveid=0x00;
	guicontext->doublestep=0x00;
	guicontext->interfacemode=GENERIC_SHUGART_DD_FLOPPYMODE;
	guicontext->hxcfe=hxcfe_init();
	hxcfe_setOutputFunc(guicontext->hxcfe,CUI_affiche);

#if !defined(STANDALONEFSBROWSER) && !defined(HXC_STREAMER_MODE)
	guicontext->usbhxcfe = libusbhxcfe_init(guicontext->hxcfe);
#endif

	if(!guicontext->usbhxcfe)
	{
		printf("Error while loading libusbhxcfe ! USB HxC Floppy Emulator support disabled !\n");
	}
	else
	{
		#ifdef GUI_DEBUG
		print_dbg((char*)"Main_Window : libusbhxcfe init done !");
		#endif
	}

	Fl::scheme("gtk+");

	Fl_Group group(0,0,WINDOW_XSIZE,392);

#ifdef WIN32
	this->icon((char *)LoadIcon(fl_display, MAKEINTRESOURCE(101)));
#endif

#ifndef OEM_MODE
	bitmap_hxc2001_backgnd_bmp->unpacked_data=data_unpack(bitmap_hxc2001_backgnd_bmp->data,bitmap_hxc2001_backgnd_bmp->csize ,bitmap_hxc2001_backgnd_bmp->data, bitmap_hxc2001_backgnd_bmp->size);
	convert8b24b(bitmap_hxc2001_backgnd_bmp,0x00);
	group.image(new Fl_Tiled_Image(new Fl_RGB_Image((const unsigned char*)bitmap_hxc2001_backgnd_bmp->unpacked_data,bitmap_hxc2001_backgnd_bmp->Xsize, bitmap_hxc2001_backgnd_bmp->Ysize, 3, 0)));
	group.align(FL_ALIGN_TEXT_OVER_IMAGE);
#endif

	j = 0;
	for(i=0;i<9;i++)
	{
		if(strlen(getString(txt_buttons_main[i].label_id)))
		{
			txt_buttons_main[i].button = new Fl_Button(BUTTON_XPOS, BUTTON_YPOS+(BUTTON_YSTEP*j), BUTTON_XSIZE, BUTTON_YSIZE, getString(txt_buttons_main[i].label_id));
			if(txt_buttons_main[i].button)
			{
				txt_buttons_main[i].button->labelsize(12);
				tmp_intptr = i;
				txt_buttons_main[i].button->callback(bt_clicked,(void*)tmp_intptr);
			}

			Fl_Box *box = new Fl_Box(FL_NO_BOX,BUTTON_XPOS+BUTTON_XSIZE,BUTTON_YPOS+(BUTTON_YSIZE/4)+(BUTTON_YSTEP*j),BUTTON_XSIZE*4,BUTTON_YSIZE/2,getString(txt_buttons_main[i].desc_id));
			if(box)
			{
				box->align(FL_ALIGN_INSIDE|FL_ALIGN_LEFT);
				box->labelsize(12);
				box->labeltype(FL_ENGRAVED_LABEL);
			}
			j++;
		}
	}

	#ifdef GUI_DEBUG
	print_dbg((char*)"Main_Window : Buttons and background are set !");
	#endif

	file_name_txt = new Fl_Output(BUTTON_XPOS, BUTTON_YPOS+(BUTTON_YSTEP*j++), WINDOW_XSIZE-(BUTTON_XPOS*2), 25);
	if(file_name_txt)
	{
		file_name_txt->labelsize(12);
		file_name_txt->textsize(12);
		file_name_txt->align(FL_ALIGN_TOP_LEFT);
		file_name_txt->value("No disk loaded.");
		file_name_txt->box(FL_PLASTIC_UP_BOX);
	}
	else
	{
		printf("Error while allocation file_name_txt !\n");
	}

	track_pos_str = new Fl_Output(BUTTON_XPOS, BUTTON_YPOS+(BUTTON_YSTEP*j++)-10, WINDOW_XSIZE-(BUTTON_XPOS*2), 25);
	if(track_pos_str)
	{
		track_pos_str->labelsize(12);
		track_pos_str->textsize(12);
		track_pos_str->align(FL_ALIGN_TOP_LEFT);
		track_pos_str->box(FL_PLASTIC_UP_BOX);
	}
	else
	{
		printf("Error while allocation track_pos_str !\n");
	}

	track_pos = new Fl_Progress(BUTTON_XPOS, BUTTON_YPOS+(BUTTON_YSTEP*j++)-18, WINDOW_XSIZE-(BUTTON_XPOS*2), 25);
	if(track_pos)
	{
		track_pos->box(FL_THIN_UP_BOX);
		track_pos->selection_color((Fl_Color)137);
		track_pos->minimum(0);
		track_pos->maximum(255);
		track_pos->value(0);

		track_pos->box(FL_PLASTIC_UP_BOX);
	}
	else
	{
		printf("Error while allocation track_pos !\n");
	}

	group.end();

	#ifdef GUI_DEBUG
	print_dbg((char*)"Main_Window : Track position indicator set !");
	#endif

	txt_buttons_main[0].button->callback(load_file_image,0);
	txt_buttons_main[4].button->callback(save_file_image,0);

	menutable[1].user_data_ = fc_load;
	menutable[4].user_data_ = fc_save;
	Fl_Menu_Bar menubar(0,0,WINDOW_XSIZE,24);
	menubar.menu(menutable);

	// Fl_DND_Box is constructed with the same dimensions and at the same position as Fl_Scroll
	Fl_DND_Box *o = new Fl_DND_Box(0, 0,WINDOW_XSIZE, 400, 0);
	if(o)
	{
		o->callback(dnd_cb);
	}
	else
	{
		printf("Error while allocation Drag & drop box !\n");
	}

	end();

	#ifdef GUI_DEBUG
	print_dbg((char*)"Main_Window : Main window done !");
	#endif

#ifndef OEM_MODE
	label(NOMFENETRE);
#else
	label("Floppy Emulator Toolbox v" STR_FILE_VERSION2);
#endif

#if !defined(STANDALONEFSBROWSER) && !defined(HXC_STREAMER_MODE)
	show();
#endif

	user_data((void*)(this));

	load_last_cfg();

	resize (this->x_root(),this->y_root(),WINDOW_XSIZE,BUTTON_YPOS+(BUTTON_YSTEP*j) - 25);

	guicontext->main_window = (Main_Window *)this;

	//////////////////////////////////////////////
	// Floppy dump window
	fdump_window = new floppy_dump_window();
	if(fdump_window)
	{
		fdump_window->side_0->value(1);
		fdump_window->side_1->value(1);
		fdump_window->double_step->value(0);
		fdump_window->start_track->value(0);
		fdump_window->end_track->value(79);
		fdump_window->sel_drive_a->value(1);
		fdump_window->sel_drive_b->value(0);

		fdump_window->FM125->value(1);
		fdump_window->FM150->value(1);
		fdump_window->FM250->value(1);
		fdump_window->FM500->value(0);
		fdump_window->MFM250->value(1);
		fdump_window->MFM300->value(1);
		fdump_window->MFM500->value(1);
		fdump_window->MFM1000->value(0);
		fdump_window->double_step->value(0);

		fdump_window->number_of_retry->value(3);

		guicontext->xsize = fdump_window->layout_area->w();
		guicontext->ysize = fdump_window->layout_area->h();
		guicontext->mapfloppybuffer = (unsigned char*)malloc(guicontext->xsize * guicontext->ysize * 4);
		if(!guicontext->mapfloppybuffer)
		{
			printf("Error while allocating the floppy dump frame buffer !\n");
		}
		else
		{
			if(!hxc2001_2_bmp)
			{
				bitmap_hxc2001_2_bmp->unpacked_data=data_unpack(bitmap_hxc2001_2_bmp->data,bitmap_hxc2001_2_bmp->csize ,bitmap_hxc2001_2_bmp->data, bitmap_hxc2001_2_bmp->size);
				convert8b24b(bitmap_hxc2001_2_bmp,0x00);
				hxc2001_2_bmp = bitmap_hxc2001_2_bmp;
			}

			memset(guicontext->mapfloppybuffer,0xFF,guicontext->xsize*guicontext->ysize*4);
#ifndef OEM_MODE
			splash_sprite(hxc2001_2_bmp,guicontext->mapfloppybuffer, guicontext->xsize, guicontext->ysize, guicontext->xsize / 2 - hxc2001_2_bmp->Xsize / 2, guicontext->ysize / 2 - hxc2001_2_bmp->Ysize / 2);
#endif
		}

		hxc_createcriticalsection(guicontext->hxcfe,1);
		hxc_createcriticalsection(guicontext->hxcfe,2);

		Fl::add_timeout(0.02, tick_dump, (void*)fdump_window);

		#ifdef GUI_DEBUG
		print_dbg((char*)"Main_Window : Floppy dump window done !");
		#endif
	}

	//////////////////////////////////////////////
	// Floppy view window
#ifndef HXC_STREAMER_MODE

	infos_window = new floppy_infos_window();
	if(infos_window)
	{
		if(!hxc2001_2_bmp)
		{
			bitmap_hxc2001_2_bmp->unpacked_data=data_unpack(bitmap_hxc2001_2_bmp->data,bitmap_hxc2001_2_bmp->csize ,bitmap_hxc2001_2_bmp->data, bitmap_hxc2001_2_bmp->size);
			convert8b24b(bitmap_hxc2001_2_bmp,0x00);
			hxc2001_2_bmp = bitmap_hxc2001_2_bmp;
		}

		infos_window->x_offset->bounds(0.0, 100);
		infos_window->x_offset->value(85);
		infos_window->x_time->scrollvalue((300*1000)+ (250 * 1000),1,1000,(1000*1000) + (250 * 1000));
		infos_window->x_time->step(1000);
		infos_window->y_time->scrollvalue(16,1,2,64);
		infos_window->view_mode->value(0);

		guicontext->td = hxcfe_td_init(guicontext->hxcfe,infos_window->floppy_map_disp->w(),infos_window->floppy_map_disp->h());
		guicontext->flayoutframebuffer = (unsigned char*)malloc( infos_window->floppy_map_disp->w() * infos_window->floppy_map_disp->h() * 3);
		if(guicontext->flayoutframebuffer)
		{
			memset(guicontext->flayoutframebuffer,0xFF,infos_window->floppy_map_disp->w()*infos_window->floppy_map_disp->h() * 3);
#ifndef OEM_MODE
			splash_sprite(hxc2001_2_bmp,guicontext->flayoutframebuffer, infos_window->floppy_map_disp->w(), infos_window->floppy_map_disp->h(), infos_window->floppy_map_disp->w() / 2 - hxc2001_2_bmp->Xsize / 2, infos_window->floppy_map_disp->h() / 2 - hxc2001_2_bmp->Ysize / 2);
#endif
			hxc_createevent(guicontext->hxcfe,10);

			infoth = (infothread *)malloc(sizeof(infothread));
			if(infoth)
			{
				memset(infoth,0,sizeof(infothread));
				infoth->window = (floppy_infos_window*)(infos_window);
				infoth->guicontext = guicontext;
				hxc_createthread(guicontext->hxcfe,(void*)infoth,&InfosThreadProc,0);
			}
		}

		i = 0;
		while( hxcfe_td_get_view_mode_name(guicontext->td,i) )
		{
			temp = (char*)hxcfe_td_get_view_mode_name(guicontext->td,i);
			if(temp)
			{
				track_display_view_modes_choices[i].text = (const char*)malloc(strlen(temp)+1);
				if(track_display_view_modes_choices[i].text)
					strcpy((char*)track_display_view_modes_choices[i].text, temp);
			}
			i++;
		}

		infos_window->view_mode->menu(track_display_view_modes_choices);
		infos_window->view_mode->value(0);

		infos_window->buf = new Fl_Text_Buffer;
		if(infos_window->buf)
			infos_window->object_txt->buffer(infos_window->buf);

		infos_window->amiga_mfm_bt->value(1);
		infos_window->iso_fm_bt->value(1);
		infos_window->iso_mfm_bt->value(1);

		Fl::add_timeout(0.1, tick_infos, (void*)infos_window);

		#ifdef GUI_DEBUG
		print_dbg((char*)"Main_Window : Floppy viewer window done !");
		#endif
	}

	guicontext->updatefloppyinfos++;
	guicontext->updatefloppyfs++;
#endif

	//////////////////////////////////////////////
	// Streamer / Pauline view window
	streamer_window = new floppy_streamer_window();
	if(streamer_window)
	{
		bitmap_pauline_bmp->unpacked_data=data_unpack(bitmap_pauline_bmp->data,bitmap_pauline_bmp->csize ,bitmap_pauline_bmp->data, bitmap_pauline_bmp->size);
		convert8b24b(bitmap_pauline_bmp,0x00);
		pauline_bmp = bitmap_pauline_bmp;

		streamer_window->x_offset->bounds(0.0, 100);
		streamer_window->x_offset->value(85);
		streamer_window->x_time->scrollvalue((300*1000)+ (250 * 1000),1,1000,(1000*1000) + (250 * 1000));
		streamer_window->x_time->step(1000);
		streamer_window->y_time->scrollvalue(16,1,2,64);

		guicontext->td_stream = hxcfe_td_init(guicontext->hxcfe,streamer_window->floppy_map_disp->w(),streamer_window->floppy_map_disp->h());
		guicontext->stream_frame_buffer = (unsigned char*)malloc( streamer_window->floppy_map_disp->w() * streamer_window->floppy_map_disp->h() * 4);
		if(guicontext->stream_frame_buffer)
		{
			memset(guicontext->stream_frame_buffer,0xFF,streamer_window->floppy_map_disp->w()*streamer_window->floppy_map_disp->h() * 4);
#ifndef OEM_MODE
			splash_sprite(pauline_bmp,guicontext->stream_frame_buffer, streamer_window->floppy_map_disp->w(), streamer_window->floppy_map_disp->h(), streamer_window->floppy_map_disp->w() / 2 - pauline_bmp->Xsize / 2, streamer_window->floppy_map_disp->h() / 2 - pauline_bmp->Ysize / 2);
#endif
			hxc_createevent(guicontext->hxcfe,10);

			streamth = (streamthread *)malloc(sizeof(infothread));
			if(streamth)
			{
				memset(streamth,0,sizeof(infothread));
				streamth->window = (floppy_streamer_window*)(streamer_window);
				streamth->guicontext = guicontext;
				hxc_createthread(guicontext->hxcfe,(void*)streamth,&StreamerThreadProc,0);
				hxc_createthread(guicontext->hxcfe,(void*)streamth,&StreamerThreadRxStatusProc,0);
				hxc_createthread(guicontext->hxcfe,(void*)streamth,&StreamerThreadRxDataProc,0);
			}
		}

/*		streamer_window->buf = new Fl_Text_Buffer;
		if(streamer_window->buf)
			streamer_window->object_txt->buffer(infos_window->buf);
*/
		streamer_window->amiga_mfm_bt->value(1);
		streamer_window->iso_fm_bt->value(1);
		streamer_window->iso_mfm_bt->value(1);

		memset(guicontext->pauline_ip_address,0,sizeof(guicontext->pauline_ip_address));

		hxcfe_getEnvVar( guicontext->hxcfe, (char*)"PAULINE_DEFAULT_IP_ADDRESS", guicontext->pauline_ip_address );

		streamer_window->server_address->value(guicontext->pauline_ip_address);

		streamer_window->track_number_slide->scrollvalue((int)0,1,0,86);
		streamer_window->side_number_slide->scrollvalue((int)0,1,0,2);

		streamer_window->index_delay->value("50000");
		streamer_window->dump_lenght->value("800");
		streamer_window->max_track->value("82");
		streamer_window->min_track->value("0");
		streamer_window->Side_0->value(1);
		streamer_window->Side_1->value(1);

		streamer_window->dump_name->value("untitled");
		streamer_window->index_name->value("AUTO");

		streamer_window->drive_choice->menu(drives_choices);
		streamer_window->drive_choice->value(0);

		Fl::add_timeout(0.1, streamer_tick_infos, (void*)streamer_window);

		#ifdef GUI_DEBUG
		print_dbg((char*)"Main_Window : Streamer window done !");
		#endif
	}

#ifndef HXC_STREAMER_MODE
	//////////////////////////////////////////////
	// Track editor window
	trackedit_window = new trackedittool_window();
	if(trackedit_window)
	{
		infos_window->x_offset->bounds(0.0, 100);
		trackedit_window->edit_startpoint->value("0");
		trackedit_window->edit_endpoint->value("0");
		trackedit_window->edit_bitrate->value("250000");
		trackedit_window->edit_bitrate2->value("250000");
		trackedit_window->edit_rpm->value("300");
		trackedit_window->edit_fillflakey->value("1");
		trackedit_window->edit_shiftbit->value("0");
		trackedit_window->edit_editbuffer->value("010");

		#ifdef GUI_DEBUG
		print_dbg((char*)"Main_Window : Track editor window done !");
		#endif
	}
#endif

	//////////////////////////////////////////////
	// Batch convert window
	batchconv_window = new batch_converter_window();
	if(batchconv_window)
	{
		batchconv_window->choice_file_format->menu(format_choices);
		batchconv_window->choice_file_format->value(0);
		batchconv_window->hlptxt->wrap(FL_INPUT_WRAP);
		batchconv_window->hlptxt->textsize(10);
		batchconv_window->hlptxt->readonly(FL_INPUT_READONLY);
		batchconv_window->hlptxt->static_value("To convert a large quantity of floppy images, set the source directory and the target directory (the SDCard). Drag&Drop mode : Just set the target directory and drag&drop the floppy images on this window.");
		batchconv_window->progress_indicator->minimum(0);
		batchconv_window->progress_indicator->maximum(100);

		batchconv_window->strin_src_dir->value( hxcfe_getEnvVar( guicontext->hxcfe, (char*)"LASTSTATE_BATCHCONVERTER_SRC_DIR", NULL ) );
		batchconv_window->strin_dst_dir->value( hxcfe_getEnvVar( guicontext->hxcfe, (char*)"LASTSTATE_BATCHCONVERTER_DST_DIR", NULL ) );
		batchconv_window->choice_file_format->value( hxcfe_getEnvVarValue( guicontext->hxcfe, (char*)"LASTSTATE_BATCHCONVERTER_TARGETFORMAT") );

		#ifdef GUI_DEBUG
		print_dbg((char*)"Main_Window : Batch converter window done !");
		#endif
	}

	//////////////////////////////////////////////
	// File system window
	fs_window = new filesystem_generator_window();
	if(fs_window)
	{
#ifdef STANDALONEFSBROWSER
	#ifdef WIN32
		fs_window->window->icon((char *)LoadIcon(fl_display, MAKEINTRESOURCE(101)));
	#endif
#endif

		fs_window->choice_filesystype->menu(fs_choices);
		fs_window->choice_filesystype->value(11);
		fs_window->disk_selector->lstep(10);
#ifndef STANDALONEFSBROWSER
		fs_window->disk_selector->hide();
#endif
		fs_window->hlptxt->wrap(FL_INPUT_WRAP);
		fs_window->hlptxt->textsize(10);
		fs_window->hlptxt->readonly(FL_INPUT_READONLY);

		fs_window->hlptxt->static_value("To add your files to the disk just Drag&Drop them on the file browser to the left !");
		guicontext->last_loaded_image_path[0] = 0;
		guicontext->loaded_img_modified = 0;
#ifdef STANDALONEFSBROWSER
		fs_window->choice_filesystype->deactivate();
		fs_window->bt_injectdir->deactivate();
		load_indexed_fileimage(0);
#endif
		fs_window->fs_browser->clear();
		fs_window->fs_browser->selectmode(FL_TREE_SELECT_MULTI);
		fs_window->fs_browser->root_label("/");
		fs_window->fs_browser->showroot(1);
		fs_window->fs_browser->redraw();
		fs_window->fs_browser->show_self();

		Fl::add_timeout(0.1, tick_fs, (void*)fs_window);

		#ifdef GUI_DEBUG
		print_dbg((char*)"Main_Window : File system window done !");
		#endif
	}

	//////////////////////////////////////////////
	// Raw floppy window
	rawloader_window = new rawfile_loader_window();
	if(rawloader_window)
	{
		rawloader_window->choice_sectorsize->menu(sectorsize_choices);
		rawloader_window->choice_sectorsize->value(2);
		rawloader_window->choice_numberofside->menu(nbside_choices);
		rawloader_window->choice_numberofside->value(1);
		rawloader_window->choice_tracktype->menu(track_type_choices);
		rawloader_window->choice_tracktype->value(3);
		rawloader_window->choice_numberofside->value(1);
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

		rawloader_window->hlptxt->wrap(FL_INPUT_WRAP);
		rawloader_window->hlptxt->textsize(10);
		rawloader_window->hlptxt->readonly(FL_INPUT_READONLY);

		rawloader_window->hlptxt->static_value("To batch convert RAW files you can use the Batch Converter function and check the RAW files mode check box.");

		raw_loader_window_datachanged(rawloader_window->numin_skew, 0);

		#ifdef GUI_DEBUG
		print_dbg((char*)"Main_Window : Adding xml format descriptions");
		#endif

		rfb = hxcfe_initXmlFloppy(guicontext->hxcfe);
		if(rfb)
		{
			i = 0;
			while(i< hxcfe_numberOfXmlLayout(rfb) )
			{
				temp = (char*)hxcfe_getXmlLayoutDesc(rfb,i);
				if(temp)
				{
					disklayout_choices[i+2].text = (const char*)malloc(strlen(temp)+1);
					if(disklayout_choices[i+2].text)
						strcpy((char*)disklayout_choices[i+2].text, temp);
				}
				i++;
			}
			hxcfe_deinitXmlFloppy(rfb);
		}

		rawloader_window->choice_disklayout->menu(disklayout_choices);
		rawloader_window->choice_disklayout->value(0);

		#ifdef GUI_DEBUG
		print_dbg((char*)"Main_Window : Raw floppy window done !");
		#endif
	}

	//////////////////////////////////////////////
	// Log window
	log_box = new Log_box();
	if(log_box)
	{
#ifdef GUI_DEBUG
		print_dbg((char*)"Main_Window : Log window done !");
#endif
	}
	//////////////////////////////////////////////
	// SD FE CFG window
	guicontext->backlight_tmr = 20;
	guicontext->standby_tmr = 20;
	guicontext->lcd_scroll = 150;
	guicontext->step_sound = 0xE8;
	guicontext->ui_sound = 0x40;

	sdcfg_window = new sdhxcfecfg_window();
	if(sdcfg_window)
	{
		sdcfg_window->choice_hfeifmode->menu(if_choices);
		sdcfg_window->slider_scrolltxt_speed->scrollvalue(0xFF-guicontext->lcd_scroll,1,0,(255-64));
		sdcfg_window->slider_stepsound_level->scrollvalue(0xFF-guicontext->step_sound,1,0x00, 0xFF-0xD8);
		sdcfg_window->slider_uisound_level->scrollvalue(guicontext->ui_sound,1,0,128);
		sdcfg_window->valslider_device_backlight_timeout->scrollvalue(guicontext->backlight_tmr,1,0,256);
		sdcfg_window->valslider_device_standby_timeout->scrollvalue(guicontext->standby_tmr,1,0,256);
		sdcfg_window->chk_loadlastloaded->set();
		sdcfg_window->chk_enable_twodrives_emu->set();
		sdcfg_window->choice_pin02_drva->menu(pincfg_choices);
		sdcfg_window->choice_pin34_drva->menu(pincfg_choices);
		sdcfg_window->choice_pin02_drvb->menu(pincfg_choices);
		sdcfg_window->choice_pin34_drvb->menu(pincfg_choices);
		sdcfg_window->choice_interfacemode_drva_cfg->menu(feifcfg_choices);
		sdcfg_window->choice_interfacemode_drvb_cfg->menu(feifcfg_choices);

		#ifdef GUI_DEBUG
		print_dbg((char*)"Main_Window : SD settings window done !");
		#endif
	}

	//////////////////////////////////////////////
	// USB FE CFG window
#if !defined(STANDALONEFSBROWSER) && !defined(HXC_STREAMER_MODE)
	libusbhxcfe_getStats(guicontext->hxcfe,guicontext->usbhxcfe,&stats,0);
#endif
	usbcfg_window = new usbhxcfecfg_window();
	if(usbcfg_window)
	{
		usbcfg_window->choice_ifmode->menu(if_choices);

#if !defined(STANDALONEFSBROWSER) && !defined(HXC_STREAMER_MODE)
		usbcfg_window->slider_usbpacket_size->scrollvalue(stats.packetsize,128,512,4096-(512-128));
#endif
		usbcfg_window->rbt_ds0->value(1);

		#ifdef GUI_DEBUG
		print_dbg((char*)"Main_Window : USB HxC settings window done !");
		#endif
	}

	//////////////////////////////////////////////
	// About window
	about_window = new About_box();
	if(about_window)
	{
		data_COPYING_FULL->unpacked_data=data_unpack(data_COPYING_FULL->data,data_COPYING_FULL->csize ,data_COPYING_FULL->data, data_COPYING_FULL->size);

		license_txt=(char*)data_COPYING_FULL->unpacked_data;
		license_txt[data_COPYING_FULL->size - 1] = 0;

		#ifdef GUI_DEBUG
		print_dbg((char*)"Main_Window : About window done !");
		#endif
	}

	sync_if_config();

	txtindex = 0;

	Fl::add_timeout( 0.1, tick_mw, (void*)this);

#ifdef STANDALONEFSBROWSER
	fs_window->window->show();
#endif

#ifdef HXC_STREAMER_MODE
	streamer_window->window->show();
#endif

	#ifdef GUI_DEBUG
	print_dbg((char*)"Main_Window : All done !");
	#endif

	Fl::run();
}

Main_Window::~Main_Window()
{
}

