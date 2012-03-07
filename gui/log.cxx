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
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h> 
#include <time.h>

#include "mainrouts.h"
#include "log_gui.h"

extern "C"
{
	#include "hxc_floppy_emulator.h"
	#include "../usb_floppyemulator/usb_hxcfloppyemulator.h"
}


#define LOGFIFOSIZE 256
#define LOGSTRINGSIZE 1024
typedef struct logfifo_
{
	int in;
	int out;
	unsigned char * fifotab[LOGFIFOSIZE];
}logfifo;

logfifo logsfifo;
char * logs_buffer;
extern guicontext * gui_context;


int CUI_affiche(int MSGTYPE,char * chaine, ...)
{
	FILE * debugfile;
	char test_temp[LOGSTRINGSIZE];
	
	va_list marker;
	va_start( marker, chaine );     
	
	if(!logsfifo.fifotab[logsfifo.in&(LOGFIFOSIZE-1)])
	{
		logsfifo.fifotab[logsfifo.in&(LOGFIFOSIZE-1)]=(unsigned char*)malloc(LOGSTRINGSIZE);
		memset(logsfifo.fifotab[logsfifo.in&(LOGFIFOSIZE-1)],0,LOGSTRINGSIZE);
		_vsnprintf(test_temp,LOGSTRINGSIZE,chaine,marker);
		
		switch(MSGTYPE)
		{
			
		case MSG_INFO_0:
			_snprintf((char*)logsfifo.fifotab[logsfifo.in&(LOGFIFOSIZE-1)],LOGSTRINGSIZE,"INFO 0 : %s",test_temp);
			break;
			
		case MSG_INFO_1:
			_snprintf((char*)logsfifo.fifotab[logsfifo.in&(LOGFIFOSIZE-1)],LOGSTRINGSIZE,"INFO 1 : %s",test_temp);
			break;
			
		case MSG_WARNING:
			_snprintf((char*)logsfifo.fifotab[logsfifo.in&(LOGFIFOSIZE-1)],LOGSTRINGSIZE,"WARNING: %s",test_temp);
			break;
			
		case MSG_ERROR:
			_snprintf((char*)logsfifo.fifotab[logsfifo.in&(LOGFIFOSIZE-1)],LOGSTRINGSIZE,"ERROR  : %s",test_temp);
			break;
			
		case MSG_DEBUG:
			_snprintf((char*)logsfifo.fifotab[logsfifo.in&(LOGFIFOSIZE-1)],LOGSTRINGSIZE,"DEBUG  : %s",test_temp);
			break;
			
		default:
			break;
		}
		
		
		logsfifo.in=logsfifo.in+1;
	}
	else
	{
		memset(logsfifo.fifotab[logsfifo.in&(LOGFIFOSIZE-1)],0,LOGSTRINGSIZE);
		_vsnprintf(test_temp,LOGSTRINGSIZE,chaine,marker);
		
		switch(MSGTYPE)
		{
			
		case MSG_INFO_0:
			_snprintf((char*)logsfifo.fifotab[logsfifo.in&(LOGFIFOSIZE-1)],LOGSTRINGSIZE,"INFO 0 : %s",test_temp);
			break;
			
		case MSG_INFO_1:
			_snprintf((char*)logsfifo.fifotab[logsfifo.in&(LOGFIFOSIZE-1)],LOGSTRINGSIZE,"INFO 1 : %s",test_temp);
			break;
			
		case MSG_WARNING:
			_snprintf((char*)logsfifo.fifotab[logsfifo.in&(LOGFIFOSIZE-1)],LOGSTRINGSIZE,"WARNING: %s",test_temp);
			break;
			
		case MSG_ERROR:
			_snprintf((char*)logsfifo.fifotab[logsfifo.in&(LOGFIFOSIZE-1)],LOGSTRINGSIZE,"ERROR  : %s",test_temp);
			break;
			
		case MSG_DEBUG:
			_snprintf((char*)logsfifo.fifotab[logsfifo.in&(LOGFIFOSIZE-1)],LOGSTRINGSIZE,"DEBUG  : %s",test_temp);
			break;
			
		default:
			break;
		}
		
		logsfifo.in=(logsfifo.in+1)&(LOGFIFOSIZE-1);
		logsfifo.out=(logsfifo.out+1)&(LOGFIFOSIZE-1);
		
		
		
	}
	
	
	if(gui_context->logfile)
	{
		debugfile=fopen(gui_context->logfile,"a");
		if(debugfile)
		{
			switch(MSGTYPE)
			{
			case MSG_DEBUG:
				fprintf(debugfile,"DEBUG: ");
				break;
			default:
				break;
			}
			vfprintf(debugfile,chaine,marker);
			fprintf(debugfile,"\n");
			
			fclose(debugfile);		
		}
	}
	
	
	va_end( marker ); 
	
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
			if(gui_context->logfile)
			{
				logfile=gui_context->logfile;
				gui_context->logfile=0;
				free(logfile);
			}
			logfile=(char*)malloc(1024);
			sprintf(logfile,"%s",(char*)fnfc.filename());
			gui_context->logfile=logfile;
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
				window->buf->append((char*)"\r\n");
			}
			i=(i+1)&(LOGFIFOSIZE-1);
		}while(logsfifo.in!=i);
				
		
		window->txt_displ->buffer(window->buf);
		window->txt_displ->scroll(window->buf->count_lines(0,window->buf->length()),0);

	}
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

	gui_context->logfile=0;

	tick_log(this);
	this->end();
	this->label("Logs");
 	
	return ;

}


