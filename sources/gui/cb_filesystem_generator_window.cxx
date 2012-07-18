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
// File : cb_filesystem_generator_window.cxx
// Contains: Filesystem generator window callbacks
//
// Written by:	DEL NERO Jean Francois
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////


#include <stdio.h>
#include <stdlib.h>

#ifndef WIN32
#include <stdint.h>
#endif

#include "fl_includes.h"
#include "fl_dnd_box.h"

extern "C"
{
	#include "libhxcfe.h"
	#include "usb_hxcfloppyemulator.h"
}

#include "main.h"
#include "filesystem_generator_window.h"
#include "cb_filesystem_generator_window.h"
#include "loader.h"

#include "fileselector.h"

extern s_gui_context * guicontext;

#ifdef WIN32
#define intptr_t int
#endif


static intptr_t s=0;

void fs_choice_cb(Fl_Widget *, void *v)
{
	s=(intptr_t)v;
}

void filesystem_generator_window_bt_injectdir(Fl_Button* bt, void*)
{
	filesystem_generator_window *fgw;
	Fl_Window *dw;
	FLOPPY *floppy;

	dw=((Fl_Window*)(bt->parent()));
	fgw=(filesystem_generator_window *)dw->user_data();
	
	floppy=hxcfe_generateFloppy(guicontext->hxcfe,(char*)fgw->input_folder->value(),s,0);
	load_floppy(floppy);
}

void filesystem_generator_window_bt_selectdir(Fl_Button* bt, void*)
{
	char dirstr[512];
	filesystem_generator_window *fgw;
	Fl_Window *dw;

	dw=((Fl_Window*)(bt->parent()));
	fgw=(filesystem_generator_window *)dw->user_data();

	if(!select_dir((char*)"Select source",(char*)&dirstr))
	{
		fgw->input_folder->value(dirstr);
	}
}

void filesystem_generator_bt_cancel(Fl_Button* bt, void*)
{
	filesystem_generator_window *fgw;
	Fl_Window *dw;

	dw=((Fl_Window*)(bt->parent()));
	fgw=(filesystem_generator_window *)dw->user_data();
	
	fgw->window->hide();
}
