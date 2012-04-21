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
// File : cb_floppy_infos_window.cxx
// Contains: 
//
// Written by:	DEL NERO Jean Francois
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include "floppy_infos_window.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <direct.h>

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/filename.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Progress.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_File_Browser.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/Fl_Timer.H>


#include <windows.h>

extern "C"
{
	#include "libhxcfe.h"
	#include "tracks/display_track.h"
	#include "usb_hxcfloppyemulator.h"
	#include "os_api.h"
	#include "thirdpartylibs/fdrawcmd/fdrawcmd.h"
}


#include "loader.h"

unsigned char * floppyinfobuffer;
extern HXCFLOPPYEMULATOR * flopemu;
extern FLOPPY * thefloppydisk;


void update_graph(floppy_infos_window * w)
{
	s_trackdisplay * td;
	unsigned char * ptr1;
	int i,j,k;
	
	if(w->window->shown())
	{
		w->window->make_current();
		
		td=hxcfe_td_init(flopemu,880,365,(int)w->x_time->value()*1000,(int)w->y_time->value(),(int)w->x_offset->value()*1000);
		if(td)
		{
			if(thefloppydisk)
			{
				w->track_number_slide->scrollvalue((int)w->track_number_slide->value(),1,0,thefloppydisk->floppyNumberOfTrack);
				w->side_number_slide->scrollvalue((int)w->side_number_slide->value(),1,0,thefloppydisk->floppyNumberOfSide);
				
				hxcfe_td_draw_track(flopemu,td,thefloppydisk,(int)w->track_number_slide->value(),(int)w->side_number_slide->value());
				
				ptr1=(unsigned char*)td->framebuffer;
				k=0;
				j=0;
				for(i=0;i<td->xsize*td->ysize;i++)
				{
					ptr1[j++]=ptr1[k+0];
					ptr1[j++]=ptr1[k+1];
					ptr1[j++]=ptr1[k+2];
					k=k+4;
				}
				
				fl_draw_image((unsigned char *)td->framebuffer, 5, 20, td->xsize, td->ysize, 3, 0);
				hxcfe_td_deinit(flopemu,td);
			}
		}
	}
}

void tick_infos(void *w) {
	
	floppy_infos_window *window;
	
	window=(floppy_infos_window *)w;
	
	update_graph(window);
	
	Fl::repeat_timeout(2, tick_infos, w);  
}

int InfosThreadProc(void* floppycontext,void* hw_context)
{
	return 0;	
}


void floppy_infos_window_bt_read(Fl_Button* bt, void*)
{
	floppy_infos_window *fdw;
	Fl_Window *dw;
	
	dw=((Fl_Window*)(bt->parent()));
	fdw=(floppy_infos_window *)dw->user_data();
}

void floppy_infos_ok(Fl_Button*, void* w)
{
	floppy_infos_window *fdw;
	
	fdw=(floppy_infos_window *)w;
	
	fdw->window->hide();
}

void disk_infos_window_callback(Fl_Widget *o, void *v)
{
	floppy_infos_window *window;
	
	window=((floppy_infos_window*)(o->user_data()));	
	update_graph(window);
}
