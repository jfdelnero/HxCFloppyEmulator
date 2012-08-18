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
// File : license_gui.cxx
// Contains: "About" window
//
// Written by:	DEL NERO Jean Francois
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h> 
#include <time.h>

#include "fl_includes.h"

extern char * license_txt;

extern "C"
{
	#include "version.h"
}

#include "license_gui.h"


void close_license(Fl_Widget *w, void * t)
{
	delete(w->window());
}

void License_box::hide()
{
	delete(this);
}

License_box::~License_box()
{
}

License_box::License_box()
  : Fl_Window(600,400)
{
	int xsize,ysize;
	
	xsize=600;
	ysize=400;


	buf=new Fl_Text_Buffer;

	txt_displ=new Fl_Text_Display(5, 5, xsize-(5*2), ysize-((5*2)+40));
	buf->append((char*)license_txt);	
	txt_displ->buffer(buf);

	button_ok=new Fl_Button(xsize-100, ysize-35, 80, 30, "OK" ); // Fl_Button* o
	button_ok->callback(close_license,0);

	this->end();
	this->label(NOMFENETRE);
	this->show();

	return ;
}

