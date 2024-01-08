/*
//
// Copyright (C) 2006-2024 Jean-François DEL NERO
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
// File : parameters.cxx
// Contains: Parameters window
//
// Written by: Jean-François DEL NERO
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Box.H>
#include <FL/filename.H>
#include <FL/Fl_Button.H>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <stdint.h>

#include "parameters_gui.h"

#include "libhxcfe.h"
#include "usb_hxcfloppyemulator.h"
#include "libhxcadaptor.h"

#include "main.h"

extern s_gui_context * guicontext;

#ifndef WIN32
#define _vsnprintf vsnprintf
#endif

void input_cb(Fl_Widget *w, void *v);

void Parameters_box::delete_editbox()
{
	int i;

	if(editboxarray)
	{
		i = 0;
		while(editboxarray[i].label)
		{
			free(editboxarray[i].label);
			delete editboxarray[i].input;
			i++;
		}

		free(editboxarray);

		editboxarray = NULL;

		delete scroll;
	}
}

void Parameters_box::create_editbox()
{
	char tmp_value[512];
	char * param_name;
	int cnt,i;

	scroll = new Fl_Scroll(5,5, xsize - 20, ysize-((10)+35) );

	cnt = 0;
	while( hxcfe_getEnvVarIndex( guicontext->hxcfe, cnt, NULL) )
	{
		cnt++;
	}

	if(cnt)
	{
		editboxarray = (params_editbox *)malloc(sizeof(params_editbox)*(cnt+1));
		if(!editboxarray)
			return;

		memset(editboxarray,0,sizeof(params_editbox)*(cnt+1));

		for(i=0;i<cnt;i++)
		{
			param_name = hxcfe_getEnvVarIndex( guicontext->hxcfe, i, (char*)&tmp_value);
			editboxarray[i].label = (char*)malloc(strlen(param_name)+1);
			if(editboxarray[i].label)
			{
				strcpy(editboxarray[i].label,param_name);
				editboxarray[i].input = new Fl_Input(xsize/2, 25 + 25*i, 120, 20, editboxarray[i].label);
				editboxarray[i].input->value(tmp_value);
				editboxarray[i].input->callback(input_cb);
			}
		}
	}

	scroll->end();

	return;
}

void close_parameters(Fl_Widget *w, void * t)
{
	Parameters_box *pb;

	pb = (Parameters_box *)w->parent();
	w->parent()->hide();

	delete pb;
}

void input_cb(Fl_Widget *w, void *v) {
	Fl_Input *i = (Fl_Input*)w;
	hxcfe_setEnvVar( guicontext->hxcfe, (char*)i->label(), (char*)i->value() );
}

Parameters_box::~Parameters_box()
{
	delete_editbox();
}

Parameters_box::Parameters_box()
  : Fl_Double_Window(740,480)
{
	editboxarray = NULL;

	xsize = 740;
	ysize = 480;

	create_editbox();

	button_ok = new Fl_Button(5, ysize-35, 80, 30, "Close" );
	button_ok->callback(close_parameters,0);

	hlptxt = new Fl_Output(5 + 80 + 10, ysize-40, xsize-90, 40);
	hlptxt->box(FL_NO_BOX);
	hlptxt->labeltype(FL_NO_LABEL);
	hlptxt->textsize(12);
	hlptxt->align(Fl_Align(FL_ALIGN_CENTER));
	hlptxt->wrap(FL_INPUT_WRAP);
	hlptxt->readonly(FL_INPUT_READONLY);
	hlptxt->static_value("Advanced parameters to tweak the libhxcfe behavior.");

	label("Parameters");

	end();

	return;
}

