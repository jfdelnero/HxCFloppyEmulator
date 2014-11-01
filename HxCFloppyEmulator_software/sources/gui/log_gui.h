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


// License class...
class Log_box : public Fl_Window {
  
  Fl_Button *button_savelog;
  Fl_Button *button_ok;
  Fl_Window *window;

  
  public:
	
	int old_index;
	Fl_Text_Buffer* buf;
	Fl_Text_Display * txt_displ;

	Fl_Light_Button * dbg_msg_bt;
	Fl_Light_Button * log_msg_bt;

	Log_box();
	~Log_box();

};


int CUI_affiche(int MSGTYPE,char * chaine, ...);
