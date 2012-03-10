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

extern "C"
{
	#include "./microintro/microintro.h"
}

// About class...
class About_box : public Fl_Window {
  

  Fl_Button *button_wesite;
  Fl_Button *button_releasenotes;
  Fl_Button *button_license;
  Fl_Button *button_ok;
  Fl_Button *button_forum;
  Fl_Progress* track_pos;
  Fl_Text_Display* file_name_txt;
  Fl_Box*  o;
  Fl_Window *window;


  public:
	  int	xsize;
	  int	ysize;
	  int	xpos_size;
	  int	ypos_size;
	  int	window_active;

	  uintro_context * ui_context;

	  About_box();
	~About_box();

};


void create_about_window(Fl_Widget *, void *);

