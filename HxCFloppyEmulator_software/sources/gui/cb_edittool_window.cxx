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

	hxc_entercriticalsection(guicontext->hxcfe,1);

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

	curside = hxcfe_getSide(guicontext->hxcfe,guicontext->loadedfloppy,track,side);
	if(curside)
	{

		startpulse = startpulse % (hxcfe_getTrackLength(guicontext->hxcfe,guicontext->loadedfloppy,track,side));
		endpulse = endpulse % hxcfe_getTrackLength(guicontext->hxcfe,guicontext->loadedfloppy,track,side);

		if(startpulse<endpulse)
		{
			loop = endpulse - startpulse;
		}
		else
		{
			loop = ( hxcfe_getTrackLength(guicontext->hxcfe,guicontext->loadedfloppy,track,side) - startpulse ) + endpulse;
		}

		hxcfe_setCellBitrate( guicontext->hxcfe,curside,startpulse,atoi(tew->edit_bitrate->value()),loop);

		guicontext->updatefloppyinfos = 1;
	}

	hxc_leavecriticalsection(guicontext->hxcfe,1);
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

	hxc_entercriticalsection(guicontext->hxcfe,1);

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
	tracklen = hxcfe_getTrackLength(guicontext->hxcfe,guicontext->loadedfloppy,track,side);

	curside = hxcfe_getSide(guicontext->hxcfe,guicontext->loadedfloppy,track,side);
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
			loop = ( hxcfe_getTrackLength(guicontext->hxcfe,guicontext->loadedfloppy,track,side) - startpulse ) + endpulse;
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

	hxc_leavecriticalsection(guicontext->hxcfe,1);
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

	hxc_entercriticalsection(guicontext->hxcfe,1);

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

	curside = hxcfe_getSide(guicontext->hxcfe,guicontext->loadedfloppy,track,side);
	if(curside)
	{

		startpulse = startpulse % (hxcfe_getTrackLength(guicontext->hxcfe,guicontext->loadedfloppy,track,side));
		endpulse = endpulse % hxcfe_getTrackLength(guicontext->hxcfe,guicontext->loadedfloppy,track,side);

		if(startpulse<endpulse)
		{
			loop = endpulse - startpulse;
		}
		else
		{
			loop = ( hxcfe_getTrackLength(guicontext->hxcfe,guicontext->loadedfloppy,track,side) - startpulse ) + endpulse;
		}

		hxcfe_removeCell(guicontext->hxcfe,curside,startpulse,loop);

		guicontext->updatefloppyinfos = 1;
	}

	hxc_leavecriticalsection(guicontext->hxcfe,1);
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

	hxc_entercriticalsection(guicontext->hxcfe,1);

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
	tracklen = hxcfe_getTrackLength(guicontext->hxcfe,guicontext->loadedfloppy,track,side);

	curside = hxcfe_getSide(guicontext->hxcfe,guicontext->loadedfloppy,track,side);
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
			loop = ( hxcfe_getTrackLength(guicontext->hxcfe, guicontext->loadedfloppy, track, side ) - startpulse ) + endpulse;
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

	hxc_leavecriticalsection(guicontext->hxcfe,1);
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

	hxc_entercriticalsection(guicontext->hxcfe,1);

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
	tracklen = hxcfe_getTrackLength(guicontext->hxcfe,guicontext->loadedfloppy,track,side);

	curside = hxcfe_getSide(guicontext->hxcfe,guicontext->loadedfloppy,track,side);
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
			loop = ( hxcfe_getTrackLength(guicontext->hxcfe,guicontext->loadedfloppy,track,side) - startpulse ) + endpulse;
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

	hxc_leavecriticalsection(guicontext->hxcfe,1);
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

	hxc_entercriticalsection(guicontext->hxcfe,1);

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

	curside = hxcfe_getSide(guicontext->hxcfe,guicontext->loadedfloppy,track,side);
	if(curside)
	{
		hxcfe_shiftTrackData(guicontext->hxcfe,curside, shiftpulse );

		guicontext->updatefloppyinfos = 1;
	}

	hxc_leavecriticalsection(guicontext->hxcfe,1);
}

void edittool_window_bt_shifttracks_callback(Fl_Button *o, void *v)
{
	Main_Window *window;
	floppy_infos_window *fiw;
	trackedittool_window *tew;
	int shiftpulse;

	hxc_entercriticalsection(guicontext->hxcfe,1);

	window = (Main_Window *)guicontext->main_window;
	tew = (trackedittool_window *)window->trackedit_window;
	fiw = (floppy_infos_window *)window->infos_window;

	shiftpulse = atoi(tew->edit_shiftbittracks->value());

	shiftpulse = shiftpulse % 1000;

	if(shiftpulse<0)
		shiftpulse = 1000 + shiftpulse;

	hxcfe_rotateFloppy( guicontext->hxcfe, guicontext->loadedfloppy, shiftpulse, 1000 );

	guicontext->updatefloppyinfos = 1;

	hxc_leavecriticalsection(guicontext->hxcfe,1);
}

void edittool_window_bt_repair_callback(Fl_Button *o, void *v)
{
	Main_Window *window;
	floppy_infos_window *fiw;
	HXCFE_SIDE * curside;
	int valmodif;
	int track,side;
	trackedittool_window *tew;
	int startpulse,endpulse;
	int tracklen,loop;

	hxc_entercriticalsection(guicontext->hxcfe,1);

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
	tracklen = hxcfe_getTrackLength(guicontext->hxcfe,guicontext->loadedfloppy,track,side);

	curside = hxcfe_getSide(guicontext->hxcfe,guicontext->loadedfloppy,track,side);
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
			loop = ( hxcfe_getTrackLength(guicontext->hxcfe, guicontext->loadedfloppy, track, side ) - startpulse ) + endpulse;
		}

		//hxcfe_sectorRepair( guicontext->hxcfe, guicontext->loadedfloppy, track, side, startpulse);
		hxcfe_localRepair( guicontext->hxcfe, guicontext->loadedfloppy, track, side, startpulse, loop );

		guicontext->updatefloppyinfos = 1;
	}

	hxc_leavecriticalsection(guicontext->hxcfe,1);
}

void edittool_window_bt_swapsides_callback(Fl_Button *o, void *v)
{
	Main_Window *window;
	floppy_infos_window *fiw;
	HXCFE_SIDE * side0,* side1;
	int track;
	trackedittool_window *tew;

	window = (Main_Window *)guicontext->main_window;
	tew = (trackedittool_window *)window->trackedit_window;
	fiw = (floppy_infos_window *)window->infos_window;

	hxc_entercriticalsection(guicontext->hxcfe,1);

	if(hxcfe_getNumberOfSide(guicontext->hxcfe,guicontext->loadedfloppy) == 2)
	{
		for(track=0;track<hxcfe_getNumberOfTrack(guicontext->hxcfe,guicontext->loadedfloppy);track++)
		{
			side0 = hxcfe_getSide( guicontext->hxcfe, guicontext->loadedfloppy, track, 0 );
			side1 = hxcfe_getSide( guicontext->hxcfe, guicontext->loadedfloppy, track, 1 );

			if(side0 && side1)
			{
				side0 = hxcfe_duplicateSide(guicontext->hxcfe,side0);
				hxcfe_replaceSide( guicontext->hxcfe, guicontext->loadedfloppy, track, 0, side1 );
				hxcfe_replaceSide( guicontext->hxcfe, guicontext->loadedfloppy, track, 1, side0 );
				hxcfe_freeSide( guicontext->hxcfe, side0 );
			}
		}

		guicontext->updatefloppyinfos = 1;
	}

	hxc_leavecriticalsection(guicontext->hxcfe,1);
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

	hxc_entercriticalsection(guicontext->hxcfe,1);

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
	tracklen = hxcfe_getTrackLength(guicontext->hxcfe,guicontext->loadedfloppy,track,side);

	curside = hxcfe_getSide(guicontext->hxcfe,guicontext->loadedfloppy,track,side);
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

	hxc_leavecriticalsection(guicontext->hxcfe,1);
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

	hxc_entercriticalsection(guicontext->hxcfe,1);

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
	tracklen = hxcfe_getTrackLength(guicontext->hxcfe,guicontext->loadedfloppy,track,side);

	curside = hxcfe_getSide(guicontext->hxcfe,guicontext->loadedfloppy,track,side);
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

	hxc_leavecriticalsection(guicontext->hxcfe,1);
}

void edittool_window_bt_setdiskrpm_callback(Fl_Button *o, void *v)
{
	Main_Window *window;
	floppy_infos_window *fiw;
	HXCFE_SIDE * curside;
	int track,side;
	trackedittool_window *tew;

	hxc_entercriticalsection(guicontext->hxcfe,1);

	window = (Main_Window *)guicontext->main_window;
	tew = (trackedittool_window *)window->trackedit_window;
	fiw = (floppy_infos_window *)window->infos_window;

	for(side=0;side<hxcfe_getNumberOfSide(guicontext->hxcfe,guicontext->loadedfloppy);side++)
	{
		for(track=0;track<hxcfe_getNumberOfTrack(guicontext->hxcfe,guicontext->loadedfloppy);track++)
		{
			curside = hxcfe_getSide(guicontext->hxcfe,guicontext->loadedfloppy,track,side);
			if(curside)
			{
				hxcfe_setTrackRPM( guicontext->hxcfe, curside, atoi(tew->edit_rpm->value()) );
			}
		}
	}

	guicontext->updatefloppyinfos = 1;

	hxc_leavecriticalsection(guicontext->hxcfe,1);
}

void edittool_window_bt_setdiskbitrate_callback(Fl_Button *o, void *v)
{
	Main_Window *window;
	floppy_infos_window *fiw;
	HXCFE_SIDE * curside;
	int track,side;
	trackedittool_window *tew;
	int tracklen;

	hxc_entercriticalsection(guicontext->hxcfe,1);

	window = (Main_Window *)guicontext->main_window;
	tew = (trackedittool_window *)window->trackedit_window;
	fiw = (floppy_infos_window *)window->infos_window;

	for(side=0;side<hxcfe_getNumberOfSide(guicontext->hxcfe,guicontext->loadedfloppy);side++)
	{
		for(track=0;track<hxcfe_getNumberOfTrack(guicontext->hxcfe,guicontext->loadedfloppy);track++)
		{
			curside = hxcfe_getSide(guicontext->hxcfe,guicontext->loadedfloppy,track,side);
			if(curside)
			{
				tracklen = hxcfe_getTrackLength(guicontext->hxcfe,guicontext->loadedfloppy,track,side);

				hxcfe_setCellBitrate(guicontext->hxcfe,curside,0,atoi(tew->edit_bitrate2->value()),tracklen);
			}
		}
	}

	guicontext->updatefloppyinfos = 1;

	hxc_leavecriticalsection(guicontext->hxcfe,1);

}

void edittool_window_bt_erase_side0_callback(Fl_Button *o, void *v)
{
	Main_Window *window;
	floppy_infos_window *fiw;
	HXCFE_SIDE * curside;
	int track;
	trackedittool_window *tew;
	int j,tracklen;

	window = (Main_Window *)guicontext->main_window;
	tew = (trackedittool_window *)window->trackedit_window;
	fiw = (floppy_infos_window *)window->infos_window;

	hxc_entercriticalsection(guicontext->hxcfe,1);

	if(hxcfe_getNumberOfSide(guicontext->hxcfe,guicontext->loadedfloppy))
	{
		for(track=0;track<hxcfe_getNumberOfTrack(guicontext->hxcfe,guicontext->loadedfloppy);track++)
		{
			curside = hxcfe_getSide(guicontext->hxcfe,guicontext->loadedfloppy,track,0);
			if(curside)
			{
				tracklen = hxcfe_getTrackLength(guicontext->hxcfe,guicontext->loadedfloppy,track,0);

				for(j=0;j<tracklen;j++)
				{
					if(!(j&0x3))
						hxcfe_setCellState( guicontext->hxcfe, curside, j, 1 );
					else
						hxcfe_setCellState( guicontext->hxcfe, curside, j, 0 );

					hxcfe_setCellFlakeyState( guicontext->hxcfe, curside, j, 0 );
				}
			}
		}

		guicontext->updatefloppyinfos = 1;
	}

	hxc_leavecriticalsection(guicontext->hxcfe,1);
}

void edittool_window_bt_erase_side1_callback(Fl_Button *o, void *v)
{
	Main_Window *window;
	floppy_infos_window *fiw;
	HXCFE_SIDE * curside;
	int track;
	trackedittool_window *tew;
	int j,tracklen;

	window = (Main_Window *)guicontext->main_window;
	tew = (trackedittool_window *)window->trackedit_window;
	fiw = (floppy_infos_window *)window->infos_window;

	hxc_entercriticalsection(guicontext->hxcfe,1);

	if(hxcfe_getNumberOfSide(guicontext->hxcfe,guicontext->loadedfloppy) == 2)
	{
		for(track=0;track<hxcfe_getNumberOfTrack(guicontext->hxcfe,guicontext->loadedfloppy);track++)
		{
			curside = hxcfe_getSide(guicontext->hxcfe,guicontext->loadedfloppy,track,1);
			if(curside)
			{
				tracklen = hxcfe_getTrackLength(guicontext->hxcfe,guicontext->loadedfloppy,track,1);

				for(j=0;j<tracklen;j++)
				{
					if(!(j&0x3))
						hxcfe_setCellState( guicontext->hxcfe, curside, j, 1 );
					else
						hxcfe_setCellState( guicontext->hxcfe, curside, j, 0 );

					hxcfe_setCellFlakeyState( guicontext->hxcfe, curside, j, 0 );
				}
			}
		}

		guicontext->updatefloppyinfos = 1;
	}

	hxc_leavecriticalsection(guicontext->hxcfe,1);

}

void edittool_window_bt_addtrack_callback(Fl_Button *o, void *v)
{
	Main_Window *window;
	floppy_infos_window *fiw;
	trackedittool_window *tew;

	window = (Main_Window *)guicontext->main_window;
	tew = (trackedittool_window *)window->trackedit_window;
	fiw = (floppy_infos_window *)window->infos_window;

	hxc_entercriticalsection(guicontext->hxcfe,1);

	hxcfe_addTrack( guicontext->hxcfe, guicontext->loadedfloppy, atoi(tew->edit_bitrate2->value()), atoi(tew->edit_rpm->value()) );

	guicontext->updatefloppyinfos = 1;

	hxc_leavecriticalsection(guicontext->hxcfe,1);
}

void edittool_window_bt_removetrack_callback(Fl_Button *o, void *v)
{
	hxc_entercriticalsection(guicontext->hxcfe,1);

	hxcfe_removeLastTrack(guicontext->hxcfe, guicontext->loadedfloppy);

	guicontext->updatefloppyinfos = 1;

	hxc_leavecriticalsection(guicontext->hxcfe,1);
}

void edittool_window_bt_removeoddtracks_callback(Fl_Button *o, void *v)
{
	hxc_entercriticalsection(guicontext->hxcfe,1);

	hxcfe_removeOddTracks( guicontext->hxcfe, guicontext->loadedfloppy );

	guicontext->updatefloppyinfos = 1;

	hxc_leavecriticalsection(guicontext->hxcfe,1);

}
