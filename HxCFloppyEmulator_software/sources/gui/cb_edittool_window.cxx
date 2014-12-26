/*
//
// Copyright (C) 2006-2014 Jean-François DEL NERO
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
// File : cb_edittool_window.cxx
// Contains:
//
// Written by:	DEL NERO Jean Francois
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#include <stdint.h>

#include "fl_includes.h"

#include "floppy_infos_window.h"

#include "batch_converter_window.h"
#include "filesystem_generator_window.h"
#include "cb_filesystem_generator_window.h"
#include "cb_floppy_infos_window.h"

#include "floppy_dump_window.h"
#include "floppy_infos_window.h"
#include "rawfile_loader_window.h"
#include "sdhxcfecfg_window.h"
#include "usbhxcfecfg_window.h"
#include "log_gui.h"
#include "about_gui.h"
#include "edittool_window.h"

#include "libhxcfe.h"
#include "libhxcadaptor.h"
#include "usb_hxcfloppyemulator.h"

#include "main.h"
#include "loader.h"
#include "fl_mouse_box.h"

#include "main_gui.h"

extern s_gui_context * guicontext;

extern int valuesanitycheck(int val,int min, int max, int * modif);

void edittool_window_bt_startpoint_callback(Fl_Button *o, void *v)
{
	guicontext->pointer_mode = 1;
}

void edittool_window_bt_endpoint_callback(Fl_Button *o, void *v)
{
	guicontext->pointer_mode = 2;
}

void edittool_window_bt_setbitrate_callback(Fl_Button *o, void *v)
{
	Main_Window *window;
	floppy_infos_window *fiw;
	HXCFE_SIDE * curside;
	int valmodif;
	int track,side;
	trackedittool_window *tew;
	int startpulse,endpulse;
	int loop;


	window = (Main_Window *)guicontext->main_window;
	tew = (trackedittool_window *)window->trackedit_window;
	fiw = (floppy_infos_window *)window->infos_window;

	track = (int)fiw->track_number_slide->value();
	track = valuesanitycheck(track,0, hxcfe_getNumberOfTrack(guicontext->hxcfe,guicontext->loadedfloppy),&valmodif);
	if(valmodif)
		fiw->track_number_slide->value(track);

	side=(int)fiw->side_number_slide->value();
	side = valuesanitycheck(side,0, hxcfe_getNumberOfSide(guicontext->hxcfe,guicontext->loadedfloppy),&valmodif);
	if(valmodif)
		fiw->side_number_slide->value(side);

	startpulse = atoi(tew->edit_startpoint->value());
	endpulse = atoi(tew->edit_endpoint->value());

	curside = hxcfe_getSide(guicontext->loadedfloppy,track,side);
	if(curside)
	{

		startpulse = startpulse % (hxcfe_getTrackLength(guicontext->loadedfloppy,track,side));
		endpulse = endpulse % hxcfe_getTrackLength(guicontext->loadedfloppy,track,side);

		if(startpulse<endpulse)
		{
			loop = endpulse - startpulse;
		}
		else
		{
			loop = ( hxcfe_getTrackLength(guicontext->loadedfloppy,track,side) - startpulse ) + endpulse;
		}

		hxcfe_setCellBitrate( guicontext->hxcfe,curside,startpulse,atoi(tew->edit_bitrate->value()),loop);

		guicontext->updatefloppyinfos = 1;
	}
}

void edittool_window_bt_flakeypattern_callback(Fl_Button *o, void *v)
{
	Main_Window *window;
	floppy_infos_window *fiw;
	HXCFE_SIDE * curside;
	int valmodif;
	int track,side;
	trackedittool_window *tew;
	int startpulse,endpulse;
	int i,j,tracklen,loop;
	char * copybuffer;

	window = (Main_Window *)guicontext->main_window;
	tew = (trackedittool_window *)window->trackedit_window;
	fiw = (floppy_infos_window *)window->infos_window;

	track = (int)fiw->track_number_slide->value();
	track = valuesanitycheck(track,0, hxcfe_getNumberOfTrack(guicontext->hxcfe,guicontext->loadedfloppy),&valmodif);
	if(valmodif)
		fiw->track_number_slide->value(track);

	side=(int)fiw->side_number_slide->value();
	side = valuesanitycheck(side,0, hxcfe_getNumberOfSide(guicontext->hxcfe,guicontext->loadedfloppy),&valmodif);
	if(valmodif)
		fiw->side_number_slide->value(side);

	startpulse = atoi(tew->edit_startpoint->value());
	endpulse = atoi(tew->edit_endpoint->value());
	tracklen = hxcfe_getTrackLength(guicontext->loadedfloppy,track,side);

	curside = hxcfe_getSide(guicontext->loadedfloppy,track,side);
	if(curside)
	{
		startpulse = startpulse % tracklen;
		endpulse = endpulse % tracklen;

		if(startpulse<endpulse)
		{
			loop = endpulse - startpulse;
		}
		else
		{
			loop = ( hxcfe_getTrackLength(guicontext->loadedfloppy,track,side) - startpulse ) + endpulse;
		}

		copybuffer = (char*)tew->edit_fillflakey->value();
		if(copybuffer)
		{
			i = 0;
			for(j=0;j<loop;j++)
			{

				if(copybuffer[i] == '0')
					hxcfe_setCellFlakeyState( guicontext->hxcfe, curside, (startpulse+j)%tracklen, 0 );
				else
					hxcfe_setCellFlakeyState( guicontext->hxcfe, curside, (startpulse+j)%tracklen, 1 );
				i++;

				if(copybuffer[i]==0)
					i = 0;
			}

			guicontext->updatefloppyinfos = 1;
		}
	}
}

void edittool_window_bt_delete_callback(Fl_Button *o, void *v)
{
	Main_Window *window;
	floppy_infos_window *fiw;
	HXCFE_SIDE * curside;
	int valmodif;
	int track,side;
	trackedittool_window *tew;
	int startpulse,endpulse;
	int loop;


	window = (Main_Window *)guicontext->main_window;
	tew = (trackedittool_window *)window->trackedit_window;
	fiw = (floppy_infos_window *)window->infos_window;

	track = (int)fiw->track_number_slide->value();
	track = valuesanitycheck(track,0, hxcfe_getNumberOfTrack(guicontext->hxcfe,guicontext->loadedfloppy),&valmodif);
	if(valmodif)
		fiw->track_number_slide->value(track);

	side=(int)fiw->side_number_slide->value();
	side = valuesanitycheck(side,0, hxcfe_getNumberOfSide(guicontext->hxcfe,guicontext->loadedfloppy),&valmodif);
	if(valmodif)
		fiw->side_number_slide->value(side);

	startpulse = atoi(tew->edit_startpoint->value());
	endpulse = atoi(tew->edit_endpoint->value());

	curside = hxcfe_getSide(guicontext->loadedfloppy,track,side);
	if(curside)
	{

		startpulse = startpulse % (hxcfe_getTrackLength(guicontext->loadedfloppy,track,side));
		endpulse = endpulse % hxcfe_getTrackLength(guicontext->loadedfloppy,track,side);

		if(startpulse<endpulse)
		{
			loop = endpulse - startpulse;
		}
		else
		{
			loop = ( hxcfe_getTrackLength(guicontext->loadedfloppy,track,side) - startpulse ) + endpulse;
		}

		hxcfe_removeCell(guicontext->hxcfe,curside,startpulse,loop);

		guicontext->updatefloppyinfos = 1;
	}

}

void edittool_window_bt_insert_callback(Fl_Button *o, void *v)
{
	Main_Window *window;
	floppy_infos_window *fiw;
	HXCFE_SIDE * curside;
	int valmodif;
	int track,side;
	trackedittool_window *tew;
	int startpulse,endpulse;
	int i,tracklen,loop;
	char * copybuffer;

	window = (Main_Window *)guicontext->main_window;
	tew = (trackedittool_window *)window->trackedit_window;
	fiw = (floppy_infos_window *)window->infos_window;

	track = (int)fiw->track_number_slide->value();
	track = valuesanitycheck(track,0, hxcfe_getNumberOfTrack(guicontext->hxcfe,guicontext->loadedfloppy),&valmodif);
	if(valmodif)
		fiw->track_number_slide->value(track);

	side=(int)fiw->side_number_slide->value();
	side = valuesanitycheck(side,0, hxcfe_getNumberOfSide(guicontext->hxcfe,guicontext->loadedfloppy),&valmodif);
	if(valmodif)
		fiw->side_number_slide->value(side);

	startpulse = atoi(tew->edit_startpoint->value());
	endpulse = atoi(tew->edit_endpoint->value());
	tracklen = hxcfe_getTrackLength(guicontext->loadedfloppy,track,side);

	curside = hxcfe_getSide(guicontext->loadedfloppy,track,side);
	if(curside)
	{
		startpulse = startpulse % tracklen;
		endpulse = endpulse % tracklen;

		if(startpulse<endpulse)
		{
			loop = endpulse - startpulse;
		}
		else
		{
			loop = ( hxcfe_getTrackLength( guicontext->loadedfloppy, track, side ) - startpulse ) + endpulse;
		}

		copybuffer = (char*)tew->edit_editbuffer->value();
		if(copybuffer)
		{
			hxcfe_insertCell( guicontext->hxcfe, curside, startpulse, 0, strlen(copybuffer) );

			i = 0;
			while(copybuffer[i])
			{
				if(copybuffer[i] == '0')
					hxcfe_setCellState( guicontext->hxcfe, curside, (startpulse+i)%tracklen, 0 );
				else
					hxcfe_setCellState( guicontext->hxcfe, curside, (startpulse+i)%tracklen, 1 );
				i++;
			}

			guicontext->updatefloppyinfos = 1;
		}
	}
}

void edittool_window_bt_fill_callback(Fl_Button *o, void *v)
{
	Main_Window *window;
	floppy_infos_window *fiw;
	HXCFE_SIDE * curside;
	int valmodif;
	int track,side;
	trackedittool_window *tew;
	int startpulse,endpulse;
	int i,j,tracklen,loop;
	char * copybuffer;

	window = (Main_Window *)guicontext->main_window;
	tew = (trackedittool_window *)window->trackedit_window;
	fiw = (floppy_infos_window *)window->infos_window;

	track = (int)fiw->track_number_slide->value();
	track = valuesanitycheck(track,0, hxcfe_getNumberOfTrack(guicontext->hxcfe,guicontext->loadedfloppy),&valmodif);
	if(valmodif)
		fiw->track_number_slide->value(track);

	side=(int)fiw->side_number_slide->value();
	side = valuesanitycheck(side,0, hxcfe_getNumberOfSide(guicontext->hxcfe,guicontext->loadedfloppy),&valmodif);
	if(valmodif)
		fiw->side_number_slide->value(side);

	startpulse = atoi(tew->edit_startpoint->value());
	endpulse = atoi(tew->edit_endpoint->value());
	tracklen = hxcfe_getTrackLength(guicontext->loadedfloppy,track,side);

	curside = hxcfe_getSide(guicontext->loadedfloppy,track,side);
	if(curside)
	{
		startpulse = startpulse % tracklen;
		endpulse = endpulse % tracklen;

		if(startpulse<endpulse)
		{
			loop = endpulse - startpulse;
		}
		else
		{
			loop = ( hxcfe_getTrackLength(guicontext->loadedfloppy,track,side) - startpulse ) + endpulse;
		}

		copybuffer = (char*)tew->edit_editbuffer->value();
		if(copybuffer)
		{
			i = 0;
			for(j=0;j<loop;j++)
			{

				if(copybuffer[i] == '0')
					hxcfe_setCellState( guicontext->hxcfe, curside, (startpulse+j)%tracklen, 0 );
				else
					hxcfe_setCellState( guicontext->hxcfe, curside, (startpulse+j)%tracklen, 1 );
				i++;

				if(copybuffer[i]==0)
					i = 0;
			}

			guicontext->updatefloppyinfos = 1;
		}
	}
}

void edittool_window_bt_shift_callback(Fl_Button *o, void *v)
{
	Main_Window *window;
	floppy_infos_window *fiw;
	HXCFE_SIDE * curside;
	int valmodif;
	int track,side;
	trackedittool_window *tew;
	int shiftpulse;

	window = (Main_Window *)guicontext->main_window;
	tew = (trackedittool_window *)window->trackedit_window;
	fiw = (floppy_infos_window *)window->infos_window;

	track = (int)fiw->track_number_slide->value();
	track = valuesanitycheck(track,0, hxcfe_getNumberOfTrack(guicontext->hxcfe,guicontext->loadedfloppy),&valmodif);
	if(valmodif)
		fiw->track_number_slide->value(track);

	side=(int)fiw->side_number_slide->value();
	side = valuesanitycheck(side,0, hxcfe_getNumberOfSide(guicontext->hxcfe,guicontext->loadedfloppy),&valmodif);
	if(valmodif)
		fiw->side_number_slide->value(side);

	shiftpulse = atoi(tew->edit_shiftbit->value());

	curside = hxcfe_getSide(guicontext->loadedfloppy,track,side);
	if(curside)
	{
		hxcfe_shiftTrackData(curside, shiftpulse );

		guicontext->updatefloppyinfos = 1;
	}
}

void edittool_window_bt_paste_callback(Fl_Button *o, void *v)
{
	Main_Window *window;
	floppy_infos_window *fiw;
	HXCFE_SIDE * curside;
	int valmodif;
	int track,side;
	trackedittool_window *tew;
	int startpulse,endpulse;
	int i,tracklen;
	char * copybuffer;

	window = (Main_Window *)guicontext->main_window;
	tew = (trackedittool_window *)window->trackedit_window;
	fiw = (floppy_infos_window *)window->infos_window;

	track = (int)fiw->track_number_slide->value();
	track = valuesanitycheck(track,0, hxcfe_getNumberOfTrack(guicontext->hxcfe,guicontext->loadedfloppy),&valmodif);
	if(valmodif)
		fiw->track_number_slide->value(track);

	side=(int)fiw->side_number_slide->value();
	side = valuesanitycheck(side,0, hxcfe_getNumberOfSide(guicontext->hxcfe,guicontext->loadedfloppy),&valmodif);
	if(valmodif)
		fiw->side_number_slide->value(side);

	startpulse = atoi(tew->edit_startpoint->value());
	endpulse = atoi(tew->edit_endpoint->value());
	tracklen = hxcfe_getTrackLength(guicontext->loadedfloppy,track,side);

	curside = hxcfe_getSide(guicontext->loadedfloppy,track,side);
	if(curside)
	{

		startpulse = startpulse % tracklen;
		endpulse = endpulse % tracklen;

		copybuffer = (char*)tew->edit_editbuffer->value();
		if(copybuffer)
		{
			i = 0;
			while(copybuffer[i])
			{
				if(copybuffer[i] == '0')
					hxcfe_setCellState( guicontext->hxcfe, curside, (startpulse+i)%tracklen, 0 );
				else
					hxcfe_setCellState( guicontext->hxcfe, curside, (startpulse+i)%tracklen, 1 );
				i++;
			}

			guicontext->updatefloppyinfos = 1;
		}
	}
}

void edittool_window_bt_copy_callback(Fl_Button *o, void *v)
{
	Main_Window *window;
	floppy_infos_window *fiw;
	HXCFE_SIDE * curside;
	int valmodif;
	int track,side;
	trackedittool_window *tew;
	int startpulse,endpulse;
	int loop,i,tracklen;
	char * copybuffer;

	window = (Main_Window *)guicontext->main_window;
	tew = (trackedittool_window *)window->trackedit_window;
	fiw = (floppy_infos_window *)window->infos_window;

	track = (int)fiw->track_number_slide->value();
	track = valuesanitycheck(track,0, hxcfe_getNumberOfTrack(guicontext->hxcfe,guicontext->loadedfloppy),&valmodif);
	if(valmodif)
		fiw->track_number_slide->value(track);

	side=(int)fiw->side_number_slide->value();
	side = valuesanitycheck(side,0, hxcfe_getNumberOfSide(guicontext->hxcfe,guicontext->loadedfloppy),&valmodif);
	if(valmodif)
		fiw->side_number_slide->value(side);

	startpulse = atoi(tew->edit_startpoint->value());
	endpulse = atoi(tew->edit_endpoint->value());
	tracklen = hxcfe_getTrackLength(guicontext->loadedfloppy,track,side);

	curside = hxcfe_getSide(guicontext->loadedfloppy,track,side);
	if(curside)
	{

		startpulse = startpulse % tracklen;
		endpulse = endpulse % tracklen;

		if(startpulse<endpulse)
		{
			loop = endpulse - startpulse;
		}
		else
		{
			loop = ( tracklen - startpulse ) + endpulse;
		}

		if(loop)
		{
			copybuffer = (char*)malloc(loop + 1);
			if(copybuffer)
			{
				memset(copybuffer,0,(loop + 1));

				for(i=0;i<loop;i++)
				{
					copybuffer[i] = hxcfe_getCellState( guicontext->hxcfe, curside, (startpulse+i)%tracklen ) + '0';
				}

				tew->edit_editbuffer->value(copybuffer);

				free(copybuffer);
			}
		}
	}
}
