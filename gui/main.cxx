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
// File : main.cxx
// Contains: Application entry point
//
// Written by:	DEL NERO Jean Francois
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////


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

#include <FL/Fl_File_Icon.H>
#include <FL/Fl_Shared_Image.H>
#include <FL/Fl_BMP_Image.H>

#include <FL/fl_ask.H>


extern "C"
{
	#include "hxc_floppy_emulator.h"
	#include "../usb_floppyemulator/usb_hxcfloppyemulator.h"
}

#include "gui_struct.h"
#include "batch_converter_window.h"
#include "filesystem_generator_window.h"
#include "floppy_dump_window.h"
#include "rawfile_loader_window.h"
#include "sdhxcfecfg_window.h"
#include "usbhxcfecfg_window.h"
#include "log_gui.h"
#include "about_gui.h"
#include "main_gui.h"

HXCFLOPPYEMULATOR * flopemu;
FLOPPY * thefloppydisk;
HWINTERFACE * hwif;

guicontext * gui_context;

extern int CUI_affiche(int MSGTYPE,char * chaine, ...);

	track_type track_type_list[]=
	{
		{ FM_TRACK_TYPE,"FM",ISOFORMAT_SD},
		{ FMIBM_TRACK_TYPE,"IBM FM",IBMFORMAT_SD},
		{ MFM_TRACK_TYPE,"MFM",ISOFORMAT_DD},
		{ MFMIBM_TRACK_TYPE,"IBM MFM",IBMFORMAT_DD},
		//{ GCR_TRACK_TYPE,"GCR"},
		{ -1,"",0}
	};


int main(int argc, char **argv) {

	gui_context=(guicontext *)malloc(sizeof(guicontext));
	if(gui_context)
	{
		memset(gui_context,0,sizeof(guicontext));

		thefloppydisk=0;
		flopemu=hxcfe_init();
		hxcfe_setOutputFunc(flopemu,CUI_affiche);
		hwif=(HWINTERFACE *)malloc(sizeof(HWINTERFACE));
		memset(hwif,0,sizeof(HWINTERFACE));
		HW_CPLDFloppyEmulator_init(flopemu,hwif);

		Main_Window * mw;
		mw=new Main_Window();

		return 0;
	}

	
	return -1;
}
