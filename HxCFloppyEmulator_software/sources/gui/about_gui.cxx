/*
//
// Copyright (C) 2006 - 2012 Jean-François DEL NERO
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

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h> 
#include <time.h>


extern "C"
{
	#include "libhxcfe.h"
	#include "version.h"
}

#include "fl_includes.h"

#include "about_gui.h"
#include "license_gui.h"


static void tick(void *v) 
{
	About_box *window;
	unsigned char * ptr1;
	int i,j,k;
	
	window=(About_box *)v;
	
	if(window->shown())
	{
		window->make_current();
		uintro_getnextframe(window->ui_context);
		
		ptr1=(unsigned char*)window->ui_context->framebuffer;
		k=0;
		j=0;
		for(i=0;i<window->xsize*window->ysize;i++)
		{
			ptr1[j++]=ptr1[k+2];
			ptr1[j++]=ptr1[k+1];
			ptr1[j++]=ptr1[k+0];
			k=k+4;
		}
		
		fl_draw_image((unsigned char *)window->ui_context->framebuffer, window->xpos_size, window->ypos_size, window->xsize, window->ysize, 3, 0);
	}
	
	Fl::repeat_timeout(0.02, tick, v);
	
}



void close(Fl_Widget *w, void * t)
{
	w->parent()->hide();
}

void create_license_window(Fl_Widget *, void *) 
{
	new License_box();
	return ;
}

void OpenURLInBrowser(Fl_Widget *,void* u)
{
	char * url;
	url=(char*)u;
	#if defined (WIN32)
		ShellExecute(HWND_DESKTOP, "open", url, NULL, NULL, SW_SHOW);
	#elif defined (OSX)
		char commandString[2048];
		sprintf(commandString, "open %s", url);
		system(commandString);
	#elif defined (__amigaos4__)
		char commandString[2048];
		sprintf(commandString, "ibrowse:ibrowse %s", url);
		system(commandString);
	#endif
}

About_box::~About_box()
{
	Fl::remove_timeout(tick,0); 
    uintro_deinit(this->ui_context);
}

About_box::About_box()
  : Fl_Window(530,240)
{
	o = new Fl_Box(40, 15, 180-40+5, 25, "HxCFLoppyEmulator");
	o->box(FL_DOWN_BOX);

	button_wesite=		new Fl_Button(5, 110, 180, 25, "Website");
	button_wesite->callback(OpenURLInBrowser,(void*)"http://www.hxc2001.com/");

	button_forum=		new Fl_Button(5, 110+25, 180, 25, "Support Forum");
	button_forum->callback(OpenURLInBrowser,(void*)"http://torlus.com/floppy/forum");

	button_releasenotes=new Fl_Button(5, 110+25*2, 180, 25, "Latest release notes");
	button_releasenotes->callback(OpenURLInBrowser,(void*)"http://hxc2001.free.fr/floppy_drive_emulator/hxcfloppyemulator_soft_release_notes.txt");

	button_license=		new Fl_Button(5, 110+25*3, 180, 25, "Under GPL License");
	button_license->callback(create_license_window,0);
	
	button_ok=new Fl_Button(5, 110+25*4, 180/2, 25, "OK" ); // Fl_Button* o
	button_ok->callback(close,0);

	o = new Fl_Box(200, 13, 320+6, 200+6);
	o->box(FL_UP_BOX);// Fl_Box* o
	o = new Fl_Box(5, 45, 180, 60, "Copyright © 2006-2012\nDEL NERO Jean François\nPowerOfAsm / HxC2001");
	o->box(FL_DOWN_BOX);

	xpos_size=200+3;
	ypos_size=13+3;

	xsize=320;
	ysize=200;

	ui_context=uintro_init(xsize,ysize);

	this->end();
	this->label(NOMFENETRE);

    tick(this);
	
	return ;
}

