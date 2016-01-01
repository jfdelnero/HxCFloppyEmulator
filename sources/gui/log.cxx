/*
//
// Copyright (C) 2006-2016 Jean-François DEL NERO
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
// File : log.cxx
// Contains: log window
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
#include <FL/Fl_Timer.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_File_Browser.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Native_File_Chooser.H>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <stdint.h>

#include "log_gui.h"

#include "libhxcfe.h"
#include "usb_hxcfloppyemulator.h"
#include "libhxcadaptor.h"

#include "main.h"

extern s_gui_context * guicontext;

#define LOGFIFOSIZE 1024
#define LOGSTRINGSIZE 1024
typedef struct logfifo_
{
	int in;
	int out;
	unsigned char * fifotab[LOGFIFOSIZE];
	int debug_msg;
	int log_msg;
}logfifo;

logfifo logsfifo;
char * logs_buffer;

#ifndef WIN32
#define _vsnprintf vsnprintf
#define _snprintf snprintf
#endif

int CUI_affiche(int MSGTYPE,char * chaine, ...)
{
	FILE * debugfile;
	char lineheader[16];
	char fullline[LOGSTRINGSIZE];

	if(((MSGTYPE == MSG_DEBUG ) && logsfifo.debug_msg) || (logsfifo.log_msg && (MSGTYPE!=MSG_DEBUG)) )
	{
		va_list marker;
		va_start( marker, chaine );


		if(!logsfifo.fifotab[logsfifo.in&(LOGFIFOSIZE-1)])
		{
			logsfifo.fifotab[logsfifo.in&(LOGFIFOSIZE-1)]=(unsigned char*)malloc(LOGSTRINGSIZE+1);
		}

		switch(MSGTYPE)
		{
			case MSG_INFO_0:
				_snprintf(lineheader,sizeof(lineheader),"INFO 0 : ");
			break;
			case MSG_INFO_1:
				_snprintf(lineheader,sizeof(lineheader),"INFO 1 : ");
			break;
			case MSG_WARNING:
				_snprintf(lineheader,sizeof(lineheader),"WARNING: ");
			break;
			case MSG_ERROR:
				_snprintf(lineheader,sizeof(lineheader),"ERROR  : ");
			break;
			case MSG_DEBUG:
				_snprintf(lineheader,sizeof(lineheader),"DEBUG  : ");
			break;
			default:
				_snprintf(lineheader,sizeof(lineheader),"UNKNOW MESSAGE TYPE: ");
			break;
		}

		_vsnprintf(fullline,LOGSTRINGSIZE-sizeof(lineheader),chaine,marker);

		if(logsfifo.fifotab[logsfifo.in&(LOGFIFOSIZE-1)])
		{
			memset(logsfifo.fifotab[logsfifo.in&(LOGFIFOSIZE-1)],0,LOGSTRINGSIZE+1);

			logsfifo.fifotab[logsfifo.in&(LOGFIFOSIZE-1)][LOGSTRINGSIZE]=0;

			strcpy((char*)logsfifo.fifotab[logsfifo.in&(LOGFIFOSIZE-1)],lineheader);
			strcat((char*)logsfifo.fifotab[logsfifo.in&(LOGFIFOSIZE-1)],fullline);

			logsfifo.in=(logsfifo.in + 1) & (LOGFIFOSIZE-1);

			if(logsfifo.in == logsfifo.out)
				logsfifo.out=(logsfifo.out + 1) & (LOGFIFOSIZE-1);
		}

		if(guicontext->logfile)
		{
			debugfile=hxc_fopen(guicontext->logfile,"a");
			if(debugfile)
			{
				fprintf(debugfile,"%s",lineheader);
				fprintf(debugfile,"%s",fullline);
				fprintf(debugfile,"\n");
				hxc_fclose(debugfile);
			}
		}


		va_end( marker );
	}

	return 0;
}


void close_log(Fl_Widget *w, void * t)
{
	w->parent()->hide();
}

void savelog_log(Fl_Widget *w, void * t)
{
	char * logfile;
	Fl_Native_File_Chooser fnfc;

	fnfc.title("Set log file...");
	fnfc.type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
	fnfc.filter("log file\t*.log\n");
	fnfc.preset_file("hxclog.log");

	// Show native chooser
	switch ( fnfc.show() )
	{
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
			if(guicontext->logfile)
			{
				logfile=guicontext->logfile;
				guicontext->logfile=0;
				free(logfile);
			}
			logfile=(char*)malloc(1024);
			sprintf(logfile,"%s",(char*)fnfc.filename());
			guicontext->logfile=logfile;
			break;
		}
	}

}


Log_box::~Log_box()
{
}

static void tick_log(void *v) {

	Log_box *window;
	int i;

	window=(Log_box *)v;

	if(logsfifo.in!=window->old_index)
	{

		window->old_index=logsfifo.in;

		window->buf->remove(0,window->buf->length());
		i=logsfifo.out;
		do
		{
			if(logsfifo.fifotab[i&(LOGFIFOSIZE-1)])
			{
				window->buf->append((char*)(const char*)logsfifo.fifotab[i&(LOGFIFOSIZE-1)]);
				window->buf->append((char*)"\n");
			}
			i=(i+1)&(LOGFIFOSIZE-1);
		}while(logsfifo.in!=i);


		window->txt_displ->buffer(window->buf);
		window->txt_displ->scroll(window->buf->count_lines(0,window->buf->length()),0);

	}

	logsfifo.debug_msg = window->dbg_msg_bt->value();
	logsfifo.log_msg = window->log_msg_bt->value();

	Fl::repeat_timeout(0.40, tick_log, v);
}


Log_box::Log_box()
  : Fl_Window(640,400)
{
	int xsize,ysize;

	xsize=640;
	ysize=400;

	buf=new Fl_Text_Buffer;

	txt_displ=new Fl_Text_Display(5, 5, xsize-(5*2), ysize-((5*2)+30));
	txt_displ->textsize(10);
	txt_displ->color((Fl_Color)FL_LIGHT2);
	txt_displ->buffer(buf);

	button_ok=new Fl_Button(5, ysize-35, 80, 30, "Close" );
	button_ok->callback(close_log,0);

	button_savelog=new Fl_Button(10+80, ysize-35, 80, 30, "Set log file" );
	button_savelog->callback(savelog_log,0);

	guicontext->logfile=0;

	memset(&logsfifo,0,sizeof(logfifo));

	logsfifo.debug_msg = 0;
	logsfifo.log_msg = 0;

	dbg_msg_bt = new Fl_Light_Button(xsize - 155, ysize-35, 150, 30, "Debug");
	log_msg_bt = new Fl_Light_Button(xsize - ((155*2)), ysize-35, 150, 30, "Info/Warning/Error");

	old_index=0;

	tick_log(this);
	this->end();
	this->label("Logs");

	return ;

}


