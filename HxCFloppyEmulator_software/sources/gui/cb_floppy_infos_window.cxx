/*
//
// Copyright (C) 2006-2023 Jean-François DEL NERO
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
// Contains: Disk viewer callbacks.
//
// Written by: Jean-François DEL NERO
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include "floppy_infos_window.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#include <stdint.h>

#include "fl_includes.h"

#include "batch_converter_window.h"
#include "filesystem_generator_window.h"
#include "cb_filesystem_generator_window.h"
#include "cb_floppy_infos_window.h"

#include "floppy_dump_window.h"
#include "floppy_infos_window.h"
#include "floppy_streamer_window.h"
#include "rawfile_loader_window.h"
#include "sdhxcfecfg_window.h"
#include "usbhxcfecfg_window.h"
#include "log_gui.h"
#include "about_gui.h"
#include "edittool_window.h"
#include "parameters_gui.h"

#include "libhxcfe.h"
#include "libhxcadaptor.h"
#include "usb_hxcfloppyemulator.h"

#include "main.h"
#include "utils.h"
#include "loader.h"
#include "fl_mouse_box.h"

#include "main_gui.h"

extern s_gui_context * guicontext;

char fullstr[256*1024];

#define PI    ((float)  3.141592654f)

extern bmaptype * hxc2001_2_bmp;

float x_offset_track = 85;
float x_offset_stream = 0;

#define MAX_TRACK_MODES_INDEX 2

static int progress_callback(unsigned int current,unsigned int total,void * td,void * user)
{
	s_gui_context * guicontext;
	unsigned char *ptr1;
	unsigned char *ptr2;
	int total_pixels;


	guicontext = (s_gui_context *)user;

	ptr1 = (unsigned char*)hxcfe_td_getframebuffer(td);
	ptr2 = guicontext->flayoutframebuffer;

	total_pixels = hxcfe_td_getframebuffer_xres(td) * hxcfe_td_getframebuffer_yres(td);
	while(total_pixels--)
	{
		*ptr2++ = *ptr1++;
		*ptr2++ = *ptr1++;
		*ptr2++ = *ptr1++;
		ptr1++;
	}

	return 0;
}

static double adjust_timescale(double slide)
{
	if(slide >= 250 * 1000)
	{
		slide -= (250 * 1000);
		slide += 250;
	}
	else
	{
		slide = (slide * 250) / (double)(250 * 1000);
	}

	return slide;
}

int valuesanitycheck(int val,int min, int max, int * modif)
{
	int retval;

	if(modif)
		*modif = 0;


	retval = val;
	if(val >= max)
	{
		if(modif)
			*modif = 1;
		retval = max - 1;
	}

	if(retval<0)
	{
		if(modif)
			*modif = 1;
		retval = min;
	}

	return retval;
}

void update_graph(floppy_infos_window * w)
{
	HXCFE_TD * td;
	char tempstr[512];

	int track,side;
	int valmodif;

	if(w->window->shown())
	{
		w->window->make_current();

		w->object_txt->textsize(9);
		w->object_txt->textfont(FL_SCREEN);

		td = guicontext->td;
		if(td)
		{
			if(guicontext->trackviewerfloppy)
			{
				if(w->track_number_slide->value() >= hxcfe_getNumberOfTrack(guicontext->hxcfe,guicontext->trackviewerfloppy) )
					w->track_number_slide->value( hxcfe_getNumberOfTrack(guicontext->hxcfe,guicontext->trackviewerfloppy) - 1 );

				if(w->side_number_slide->value() >= hxcfe_getNumberOfSide(guicontext->hxcfe,guicontext->trackviewerfloppy))
					w->side_number_slide->value(hxcfe_getNumberOfSide(guicontext->hxcfe,guicontext->trackviewerfloppy) - 1);

				w->track_number_slide->scrollvalue((int)w->track_number_slide->value(),1,0,hxcfe_getNumberOfTrack(guicontext->hxcfe,guicontext->trackviewerfloppy));
				w->side_number_slide->scrollvalue((int)w->side_number_slide->value(),1,0,hxcfe_getNumberOfSide(guicontext->hxcfe,guicontext->trackviewerfloppy));

				/*if(w->track_view_bt->value())
				{
					hxcfe_td_draw_track(td,guicontext->trackviewerfloppy,(int)w->track_number_slide->value(),(int)w->side_number_slide->value());
				}
				else
				{
					if(w->disc_view_bt->value())
					{
						hxcfe_td_setProgressCallback(td,progress_callback,(void*)guicontext);
						hxcfe_td_draw_disk(td,guicontext->trackviewerfloppy);
					}
				}*/

				track = (int)w->track_number_slide->value();
				track = valuesanitycheck(track,0, hxcfe_getNumberOfTrack(guicontext->hxcfe,guicontext->trackviewerfloppy),&valmodif);
				if(valmodif)
					w->track_number_slide->value(track);

				side = (int)w->side_number_slide->value();
				side = valuesanitycheck(side,0, hxcfe_getNumberOfSide(guicontext->hxcfe,guicontext->trackviewerfloppy),&valmodif);
				if(valmodif)
					w->side_number_slide->value(side);

				if(w->view_mode->value() <= MAX_TRACK_MODES_INDEX)
				{
					snprintf(tempstr,sizeof(tempstr),"Track : %d Side : %d ",track,side);
					w->side_number_slide->activate();
					w->track_number_slide->activate();
					w->x_offset->activate();
					w->x_time->activate();
					w->y_time->activate();
				}
				else
				{
					strcpy(tempstr,"");
					w->side_number_slide->deactivate();
					w->track_number_slide->deactivate();
					w->x_offset->deactivate();
					w->x_time->deactivate();
					w->y_time->deactivate();
					w->x_pos->value("---");
					w->y_pos->value("---");
				}

				w->global_status->value(tempstr);

				if(hxcfe_getNumberOfTrack(guicontext->hxcfe,guicontext->trackviewerfloppy))
				{
					snprintf(tempstr,sizeof(tempstr),"Track RPM tag : %d\nBitrate flag : %d\nTrack encoding flag : %x\n",
						(int)hxcfe_getTrackRPM(guicontext->hxcfe,guicontext->trackviewerfloppy,track),
						hxcfe_getTrackBitrate(guicontext->hxcfe,guicontext->trackviewerfloppy,track,side),
						hxcfe_getTrackEncoding(guicontext->hxcfe,guicontext->trackviewerfloppy,track,side)
						);

					w->buf->remove(0,w->buf->length());

					snprintf(tempstr,sizeof(tempstr),"Track RPM : %d RPM\n",hxcfe_getTrackRPM(guicontext->hxcfe,guicontext->trackviewerfloppy,track));
					w->buf->append((char*)tempstr);


					if( hxcfe_getTrackBitrate(guicontext->hxcfe,guicontext->trackviewerfloppy,track,side) == -1)
					{
						snprintf(tempstr,sizeof(tempstr),"\nBitrate : VARIABLE\n");
					}
					else
					{
						snprintf(tempstr,sizeof(tempstr),"\nBitrate : %d bit/s\n",hxcfe_getTrackBitrate(guicontext->hxcfe,guicontext->trackviewerfloppy,track,side) );
					}

					w->buf->append((char*)tempstr);
					snprintf(tempstr,sizeof(tempstr),"\nTrack format : %s\n",hxcfe_getTrackEncodingName(guicontext->hxcfe, hxcfe_getTrackEncoding(guicontext->hxcfe,guicontext->trackviewerfloppy,track,side) ) );
					w->buf->append((char*)tempstr);
					snprintf(tempstr,sizeof(tempstr),"\nTrack len : %d cells\n",hxcfe_getTrackLength(guicontext->hxcfe,guicontext->trackviewerfloppy,track,side));
					w->buf->append((char*)tempstr);
					snprintf(tempstr,sizeof(tempstr),"Number of side : %d\n",hxcfe_getTrackNumberOfSide(guicontext->hxcfe,guicontext->trackviewerfloppy,track));
					w->buf->append((char*)tempstr);

					snprintf(tempstr,sizeof(tempstr),"\nInterface mode:\n%s\n%s\n",
						hxcfe_getFloppyInterfaceModeName(guicontext->hxcfe,guicontext->interfacemode),
						hxcfe_getFloppyInterfaceModeDesc(guicontext->hxcfe,guicontext->interfacemode)
						);
					w->buf->append((char*)tempstr);

					w->object_txt->buffer(w->buf);
				}
			}
		}
	}
}

void tick_infos(void *w) {

	HXCFE_TD * td;
	floppy_infos_window *window;
	trackedittool_window *tew;
	Main_Window *mwindow;

	window=(floppy_infos_window *)w;
	mwindow = (Main_Window *)guicontext->main_window;

	if(window->window->shown())
	{
		window->window->make_current();
		td = guicontext->td;
		if(td)
		{

			if( (window->floppy_map_disp->w() != hxcfe_td_getframebuffer_xres(td)) || (window->floppy_map_disp->h() != hxcfe_td_getframebuffer_yres(td)) )
			{
				// Resized... Realloc needed

				hxc_entercriticalsection(guicontext->hxcfe,1);

				hxcfe_td_deinit(guicontext->td);

				guicontext->td = NULL;
				td = NULL;

				guicontext->td = hxcfe_td_init(guicontext->hxcfe,window->floppy_map_disp->w(),window->floppy_map_disp->h());
				if(guicontext->td)
				{
					td = guicontext->td;
					if(guicontext->flayoutframebuffer)
						free(guicontext->flayoutframebuffer);

					tew = (trackedittool_window *)mwindow->trackedit_window;

					if(atoi(tew->edit_startpoint->value()) || atoi(tew->edit_endpoint->value()))
					{
						hxcfe_td_set_marker( td, atoi(tew->edit_startpoint->value()), 0, 0, 0, TD_MARKER_FLAG_ENABLE );
						hxcfe_td_set_marker( td, atoi(tew->edit_endpoint->value()), 1, 1, 0, TD_MARKER_FLAG_ENABLE );
					}

					guicontext->flayoutframebuffer = (unsigned char*)malloc( hxcfe_td_getframebuffer_xres(td) * hxcfe_td_getframebuffer_yres(td) * 3);
					if(guicontext->flayoutframebuffer)
					{
						memset(guicontext->flayoutframebuffer,0xFF,hxcfe_td_getframebuffer_xres(td)*hxcfe_td_getframebuffer_yres(td) * 3);
						splash_sprite(hxc2001_2_bmp,guicontext->flayoutframebuffer, hxcfe_td_getframebuffer_xres(td), hxcfe_td_getframebuffer_yres(td), hxcfe_td_getframebuffer_xres(td) / 2 - hxc2001_2_bmp->Xsize / 2, hxcfe_td_getframebuffer_yres(td) / 2 - hxc2001_2_bmp->Ysize / 2);

						guicontext->updatefloppyinfos = 1;
					}
				}

				hxc_leavecriticalsection(guicontext->hxcfe,1);
			}

			if(guicontext->updatefloppyinfos)
			{
				hxc_entercriticalsection(guicontext->hxcfe,1);
				guicontext->updatefloppyinfos=0;
				guicontext->trackviewerfloppy_updateneeded = 1;
				update_graph(window);
				hxc_setevent(guicontext->hxcfe,10);
				hxc_leavecriticalsection(guicontext->hxcfe,1);
			}

			fl_draw_image((unsigned char *)guicontext->flayoutframebuffer, window->floppy_map_disp->x(), window->floppy_map_disp->y(), hxcfe_td_getframebuffer_xres(td), hxcfe_td_getframebuffer_yres(td), 3, 0);
		}
	}

	Fl::repeat_timeout(0.1, tick_infos, w);
}

int InfosThreadProc(void* floppycontext,void* context)
{
	int i,nbpixel;
	HXCFE_TD * td;
	infothread * infoth;
	s_gui_context * guicontext;
	floppy_infos_window *w;
	unsigned char *ptr1;
	unsigned char *ptr2;
	int view_mode, old_view_mode;

	infoth = (infothread*)context;
	w = infoth->window;

	old_view_mode = w->view_mode->value();
	view_mode = old_view_mode;

	do
	{
		guicontext = (s_gui_context *)infoth->guicontext;

		if(!hxc_waitevent(guicontext->hxcfe,10,1000))
		{
			hxc_entercriticalsection(guicontext->hxcfe,1);

			td = (HXCFE_TD *)guicontext->td;

			if(guicontext->trackviewerfloppy_updateneeded)
			{
				guicontext->trackviewerfloppy_updateneeded = 0;
				if(guicontext->trackviewerfloppy)
				{
					hxc_entercriticalsection(guicontext->hxcfe,2);
					HXCFE_FLOPPY * tmp_floppy;
					tmp_floppy = guicontext->trackviewerfloppy;
					guicontext->trackviewerfloppy = 0;
					hxcfe_floppyUnload(guicontext->hxcfe,tmp_floppy);
					hxc_leavecriticalsection(guicontext->hxcfe,2);
				}

				if(guicontext->loadedfloppy)
				{
					guicontext->trackviewerfloppy = hxcfe_floppyDuplicate(guicontext->hxcfe,guicontext->loadedfloppy);
					if(strlen(guicontext->bufferfilename))
					{
						hxcfe_td_setName( td, guicontext->bufferfilename );
					}
				}
			}

			hxcfe_td_activate_analyzer(td,ISOIBM_MFM_ENCODING,w->iso_mfm_bt->value());
			hxcfe_td_activate_analyzer(td,ISOIBM_FM_ENCODING,w->iso_fm_bt->value());
			hxcfe_td_activate_analyzer(td,AMIGA_MFM_ENCODING,w->amiga_mfm_bt->value());
			hxcfe_td_activate_analyzer(td,EMU_FM_ENCODING,w->eemu_bt->value());
			hxcfe_td_activate_analyzer(td,MEMBRAIN_MFM_ENCODING,w->membrain_bt->value());
			hxcfe_td_activate_analyzer(td,TYCOM_FM_ENCODING,w->tycom_bt->value());
			hxcfe_td_activate_analyzer(td,APPLEII_GCR1_ENCODING,w->apple2_bt->value());
			hxcfe_td_activate_analyzer(td,APPLEII_GCR2_ENCODING,w->apple2_bt->value());
			hxcfe_td_activate_analyzer(td,APPLEMAC_GCR_ENCODING,w->apple2_bt->value());
			hxcfe_td_activate_analyzer(td,ARBURGDAT_ENCODING,w->arburg_bt->value());
			hxcfe_td_activate_analyzer(td,ARBURGSYS_ENCODING,w->arburg_bt->value());
			hxcfe_td_activate_analyzer(td,AED6200P_MFM_ENCODING,w->aed6200p_bt->value());
			hxcfe_td_activate_analyzer(td,NORTHSTAR_HS_MFM_ENCODING,w->northstar_bt->value());
			hxcfe_td_activate_analyzer(td,HEATHKIT_HS_FM_ENCODING,w->heathkit_bt->value());
			hxcfe_td_activate_analyzer(td,DEC_RX02_M2FM_ENCODING,w->decrx02_bt->value());
			hxcfe_td_activate_analyzer(td,QD_MO5_ENCODING,w->qd_mo5_bt->value());
			hxcfe_td_activate_analyzer(td,C64_GCR_ENCODING,w->c64_bt->value());
			hxcfe_td_activate_analyzer(td,VICTOR9K_GCR_ENCODING,w->victor9k_bt->value());
			hxcfe_td_activate_analyzer(td,MICRALN_HS_FM_ENCODING,w->heathkit_bt->value());

			hxcfe_td_setparams(td,(int)(adjust_timescale(w->x_time->value())),(int)w->y_time->value(),(int)(w->x_offset->value()*1000),0);

			if(guicontext->trackviewerfloppy)
			{
				view_mode = w->view_mode->value();

				if(old_view_mode != view_mode)
				{
					if(old_view_mode == 0)
					{
						x_offset_track = w->x_offset->value();
					}

					if(old_view_mode == 1 || old_view_mode == 2)
					{
						x_offset_stream = w->x_offset->value();
					}
				}

				switch(view_mode)
				{
					case 0:
						hxcfe_td_select_view_type( td, w->view_mode->value());
						hxcfe_td_draw_track(td,guicontext->trackviewerfloppy,(int)w->track_number_slide->value(),(int)w->side_number_slide->value());

						if(old_view_mode != view_mode)
						{
							w->x_offset->value(x_offset_track);

							guicontext->updatefloppyinfos = 1;
						}
					break;
					case 1:
					case 2:
						hxcfe_td_select_view_type( td, w->view_mode->value());
						hxcfe_td_draw_stream_track(td,guicontext->trackviewerfloppy,(int)w->track_number_slide->value(),(int)w->side_number_slide->value());

						if(old_view_mode != view_mode)
						{
							w->x_offset->value(x_offset_stream);

							guicontext->updatefloppyinfos = 1;
						}
					break;
					default:
						hxcfe_td_select_view_type( td, w->view_mode->value());
						hxcfe_td_setProgressCallback(td,progress_callback,(void*)guicontext);
						hxcfe_td_draw_disk(td,guicontext->trackviewerfloppy);
					break;
				}

				old_view_mode = view_mode;
			}

			if(guicontext->loadedfloppy)
			{
				ptr1 = (unsigned char*)hxcfe_td_getframebuffer(td);
				ptr2 = (unsigned char*)guicontext->flayoutframebuffer;
				nbpixel = hxcfe_td_getframebuffer_xres(td)*hxcfe_td_getframebuffer_yres(td);
				for(i=0;i<nbpixel;i++)
				{
					*ptr2++ = *ptr1++;
					*ptr2++ = *ptr1++;
					*ptr2++ = *ptr1++;
					ptr1++;
				}
			}

			hxc_leavecriticalsection(guicontext->hxcfe,1);

		}

	}while(!guicontext->exit);

	free(infoth);

	return 0;
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

	hxc_setevent(guicontext->hxcfe,10);
}

void disk_infos_window_bt_edit_callback(Fl_Widget *o, void *v)
{
	Main_Window *mwindow;

	mwindow = (Main_Window *)guicontext->main_window;

	mwindow->trackedit_window->window->show();
}

float getangle(int xsize,int ysize,int x_pos,int y_pos)
{
	float x1,y1,x2,y2,x3,y3;
	float angle;
	float dx21,dx31,dy21,dy31;
	float m12,m13;

	x1 = (float)(xsize / 2);
	y1 = (float)(ysize / 2);

	x2 = (float)xsize;
	y2 = (float)(ysize / 2);

	x3 = (float)x_pos;
	y3 = (float)y_pos;

	dx21 = x2-x1;
	dx31 = x3-x1;
	dy21 = y2-y1;
	dy31 = y3-y1;

	m12 = (float)sqrt( dx21*dx21 + dy21*dy21 );
	m13 = (float)sqrt( dx31*dx31 + dy31*dy31 );

	angle = (float)acos( (dx21*dx31 + dy21*dy31) / (m12 * m13) );

	if(dy31>=0)
	{
		angle = PI + (PI - angle);
	}

	return angle;
}


int getdistance(int xsize,int ysize,int x_pos,int y_pos)
{
	int distance;

	distance = (int)sqrt( pow( (float)((xsize/2) - x_pos), 2 ) + pow( (float)((ysize/2) - y_pos), 2 ) );

	return distance;
}


int isTheRightSector(floppy_infos_window *fiw,s_sectorlist * sl,int xpos,int ypos)
{
	int disp_xsize,disp_ysize;
	float pos_angle;
	int distance,side;
	int min_distance,max_distance;

	if(fiw->view_mode->value() > MAX_TRACK_MODES_INDEX)
	{
		disp_xsize=fiw->floppy_map_disp->w();
		disp_ysize=fiw->floppy_map_disp->h();


		min_distance = sl->diameter - sl->thickness;
		max_distance = sl->diameter;

		if(min_distance < 0)
			min_distance = 0;

		if(min_distance > max_distance)
			max_distance = min_distance;

		if(xpos<(disp_xsize/2))
		{
			pos_angle = getangle(disp_xsize/2,disp_ysize,xpos,ypos);
			distance = getdistance(disp_xsize/2,disp_ysize,xpos,ypos);
			side = 0;
		}
		else
		{
			pos_angle = getangle(disp_xsize/2,disp_ysize,(xpos-(disp_xsize/2)),ypos);
			distance = getdistance(disp_xsize/2,disp_ysize,(xpos-(disp_xsize/2)),ypos);
			side = 1;
		}

		if(sl->end_angle < PI*2)
		{
			if(
				( ( pos_angle >= sl->start_angle ) && ( pos_angle <= sl->end_angle ) ) &&
				( ( distance >= min_distance ) && ( distance <= max_distance ) ) &&
				( sl->side == side )
				)
			{
				return 1;
			}
		}
		else
		{	// Sector over index case...
			if(
				( ( pos_angle >= sl->start_angle ) || ( pos_angle <= (sl->end_angle-(PI*2) ) ) ) &&
				( ( distance >= min_distance ) && ( distance <= max_distance ) ) &&
				( sl->side == side )
				)
			{
				return 1;
			}
		}

	}
	else
	{
		if( ( xpos>=sl->x_pos1 && xpos<=sl->x_pos2 ) &&
			( ypos>=(sl->y_pos1) && ypos<=(sl->y_pos2) ) )
		{
			return 1;
		}
	}

	return 0;
}

int isTheRightPulse(floppy_infos_window *fiw,s_pulseslist * pl,int xpos,int ypos)
{
	int disp_ysize;

	disp_ysize=fiw->floppy_map_disp->h();

	if(!fiw->view_mode->value())
	{
		if( ( xpos>=pl->x_pos1 && xpos<=pl->x_pos2 ) &&
			( ypos>=(disp_ysize - 250) && ypos<=(disp_ysize - 40)) )
		{
			return 1;
		}
	}

	return 0;
}

int isTheRightPulseFlakey(floppy_infos_window *fiw,s_pulseslist * pl,int xpos,int ypos)
{
	if(!fiw->view_mode->value())
	{
		if( ( xpos>=pl->x_pos1 && xpos<=pl->x_pos2 ) &&
			( ypos>=30 && ypos<=50) )
		{
			return 1;
		}
	}

	return 0;
}

int isTheRightPulseIndex(floppy_infos_window *fiw,s_pulseslist * pl,int xpos,int ypos)
{
	if(!fiw->view_mode->value())
	{
		if( ( xpos>=pl->x_pos1 && xpos<=pl->x_pos2 ) &&
			( ypos>=10 && ypos<=27) )
		{
			return 1;
		}
	}

	return 0;
}

void mouse_di_cb(Fl_Widget *o, void *v)
{
	unsigned int i;
	floppy_infos_window *fiw;
	trackedittool_window *tew;
	Fl_Window *dw;
	Fl_Mouse_Box *dnd = (Fl_Mouse_Box*)o;
	double stepperpix_x;
	double stepperpix_y;
	int xpos,ypos,buttons_state;
	char str[512];
	char str2[512];
	int track,side;
	HXCFE_SIDE * curside;
	unsigned char * sect_data;
	unsigned int sectorsize;

	int event,valmodif;

	unsigned char c;

	s_sectorlist * sl;
	s_pulseslist * pl;

	int disp_xsize,disp_ysize,disp_xpos,disp_ypos;

	dw=((Fl_Window*)(o->parent()));
	fiw=(floppy_infos_window *)dw->user_data();

	Main_Window *window;

	window = (Main_Window *)guicontext->main_window;
	tew = (trackedittool_window *)window->trackedit_window;

	disp_xsize=fiw->floppy_map_disp->w();
	disp_ysize=fiw->floppy_map_disp->h();
	disp_xpos=fiw->floppy_map_disp->x();
	disp_ypos=fiw->floppy_map_disp->y();

	event = dnd->event();

	if(event == FL_PUSH)
	{
		pl = hxcfe_td_getlastpulselist(guicontext->td);

		buttons_state = Fl::event_buttons();
		if(buttons_state)
		{
			xpos=Fl::event_x() - disp_xpos;
			if(xpos<0) xpos=0;
			if(xpos>disp_xsize) xpos=disp_xsize-1;

			ypos=Fl::event_y() - disp_ypos;

			if(ypos<0) ypos=0;
			if(ypos>disp_ysize) ypos=disp_ysize-1;

			track = (int)fiw->track_number_slide->value();
			track = valuesanitycheck(track,0, hxcfe_getNumberOfTrack(guicontext->hxcfe,guicontext->loadedfloppy),&valmodif);
			if(valmodif)
				fiw->track_number_slide->value(track);

			side=(int)fiw->side_number_slide->value();
			side = valuesanitycheck(side,0, hxcfe_getNumberOfSide(guicontext->hxcfe,guicontext->loadedfloppy),&valmodif);
			if(valmodif)
				fiw->side_number_slide->value(side);

			if(!fiw->view_mode->value())
			{
				curside = hxcfe_getSide(guicontext->hxcfe,guicontext->loadedfloppy,track,side);

				while(pl)
				{
					if(tew->bt_directedition->value())
					{
						if(isTheRightPulse(fiw,pl,xpos,ypos))
						{
							if(hxcfe_getCellState(guicontext->hxcfe,curside,pl->pulse_number))
								hxcfe_setCellState(guicontext->hxcfe,curside,pl->pulse_number,0);
							else
								hxcfe_setCellState(guicontext->hxcfe,curside,pl->pulse_number,1);
						}

						if(isTheRightPulseFlakey(fiw,pl,xpos,ypos))
						{
							if(hxcfe_getCellFlakeyState(guicontext->hxcfe,curside,pl->pulse_number))
								hxcfe_setCellFlakeyState(guicontext->hxcfe,curside,pl->pulse_number,0);
							else
								hxcfe_setCellFlakeyState(guicontext->hxcfe,curside,pl->pulse_number,1);
						}

						if(isTheRightPulseIndex(fiw,pl,xpos,ypos))
						{
							if(hxcfe_getCellIndexState(guicontext->hxcfe,curside,pl->pulse_number))
								hxcfe_setCellIndexState(guicontext->hxcfe,curside,pl->pulse_number,0);
							else
								hxcfe_setCellIndexState(guicontext->hxcfe,curside,pl->pulse_number,1);
						}

						guicontext->updatefloppyinfos = 1;
					}

					if(isTheRightPulse(fiw,pl,xpos,ypos))
					{
						snprintf(str,sizeof(str),"%d",pl->pulse_number);
						switch(guicontext->pointer_mode)
						{
							case 1:
								tew->edit_startpoint->value(str);
								hxcfe_td_set_marker( guicontext->td, atoi(tew->edit_startpoint->value()), 0, 0, 0, TD_MARKER_FLAG_ENABLE );
								guicontext->updatefloppyinfos = 1;
							break;
							case 2:
								tew->edit_endpoint->value(str);
								hxcfe_td_set_marker( guicontext->td, atoi(tew->edit_endpoint->value()), 1, 1, 0, TD_MARKER_FLAG_ENABLE );
								guicontext->updatefloppyinfos = 1;
							break;
						}
						guicontext->pointer_mode = 0;
					}

					pl = pl->next_element;
				}


			}
			else
			{
				if(fiw->view_mode->value()>MAX_TRACK_MODES_INDEX)
				{
					fiw->view_mode->value(0);
					fiw->side_number_slide->activate();
					fiw->track_number_slide->activate();
					fiw->x_offset->activate();
					fiw->x_time->activate();
					fiw->y_time->activate();
				}
				else
				{

				}

				hxc_setevent(guicontext->hxcfe,10);
			}
		}
	}

	if(event == FL_MOVE)
	{
		stepperpix_x=(adjust_timescale(fiw->x_time->value()))/disp_xsize;
		stepperpix_y=(adjust_timescale(fiw->y_time->value()))/(double)disp_ysize;

		xpos=Fl::event_x() - disp_xpos;
		if(xpos<0) xpos=0;
		if(xpos>disp_xsize) xpos=disp_xsize-1;

		ypos=Fl::event_y() - disp_ypos;

		if(ypos<0) ypos=0;
		if(ypos>disp_ysize) ypos=disp_ysize-1;

		ypos=disp_ysize-ypos;

		if(fiw->view_mode->value() <= MAX_TRACK_MODES_INDEX)
		{
			snprintf(str,sizeof(str),"x position : %.3f ms",	(xpos * stepperpix_x)/1000);
			fiw->x_pos->value(str);
			snprintf(str,sizeof(str),"y position : %.3f us",	ypos*stepperpix_y*1000);
			fiw->y_pos->value(str);
		}

		fiw->buf->remove(0,fiw->buf->length());

		ypos = disp_ysize - ypos;

		fiw->object_txt->textsize(9);
		fiw->object_txt->textfont(FL_SCREEN);

		if(fiw->view_mode->value()<=MAX_TRACK_MODES_INDEX)
			hxc_entercriticalsection(guicontext->hxcfe,1);

		sl = hxcfe_td_getlastsectorlist(guicontext->td);
		fullstr[0]=0;

		while(sl && guicontext->trackviewerfloppy)
		{
			if(sl->sectorconfig)
			{
				sectorsize = hxcfe_getSectorConfigSectorSize(guicontext->hxcfe,sl->sectorconfig);

				fullstr[0]=0;

				if( isTheRightSector(fiw,sl,xpos,ypos) )
				{
					if(fiw->view_mode->value() > MAX_TRACK_MODES_INDEX)
					{
						snprintf(str,sizeof(str),"Track : %d Side : %d ",sl->track,sl->side);
						fiw->global_status->value(str);

						fiw->track_number_slide->value(sl->track);
						fiw->side_number_slide->value(sl->side);
					}

					switch(hxcfe_getSectorConfigEncoding(guicontext->hxcfe,sl->sectorconfig))
					{
						case ISOFORMAT_SD:
							snprintf(str,sizeof(str),"FM Sector");
						break;
						case ISOFORMAT_DD:
							snprintf(str,sizeof(str),"MFM Sector");
						break;
						case UKNCFORMAT_DD:
							snprintf(str,sizeof(str),"UKNC Sector");
						break;
						case AMIGAFORMAT_DD:
							snprintf(str,sizeof(str),"Amiga MFM Sector");
						break;
						case TYCOMFORMAT_SD:
							snprintf(str,sizeof(str),"TYCOM Sector");
						break;
						case MEMBRAINFORMAT_DD:
							snprintf(str,sizeof(str),"MEMBRAIN Sector");
						break;
						case EMUFORMAT_SD:
							snprintf(str,sizeof(str),"E-mu Sector");
						break;
						case APPLE2_GCR5A3:
							snprintf(str,sizeof(str),"AppleII 5A3 Sector");
						break;
						case APPLE2_GCR6A2:
							snprintf(str,sizeof(str),"AppleII 6A2 Sector");
						break;
						case APPLEMAC_GCR6A2:
							snprintf(str,sizeof(str),"Apple Macintosh Sector");
						break;
						case ARBURG_DAT:
							snprintf(str,sizeof(str),"Arburg SD Sector");
						break;
						case ARBURG_SYS:
							snprintf(str,sizeof(str),"Arburg SYS Sector");
						break;
						case AED6200P_DD:
							snprintf(str,sizeof(str),"AED 6200P Sector");
						break;
						case NORTHSTAR_HS_DD:
							snprintf(str,sizeof(str),"Northstar Sector");
						break;
						case HEATHKIT_HS_SD:
							snprintf(str,sizeof(str),"Heathkit Sector");
						break;
						case DECRX02_SDDD:
							snprintf(str,sizeof(str),"DEC RX02 Sector");
						break;
						case QD_MO5_MFM:
							snprintf(str,sizeof(str),"QD MO5 Sector");
						break;
						case C64_GCR:
							snprintf(str,sizeof(str),"C64 GCR Sector");
						break;
						case MICRALN_HS_SD:
							snprintf(str,sizeof(str),"R2E Micral N");
						break;
						case VICTOR9K_GCR:
							snprintf(str,sizeof(str),"Victor 9000");
						break;
						default:
							snprintf(str,sizeof(str),"Unknown Sector type");
						break;
					}
					fiw->buf->append((char*)str);

					snprintf(str,sizeof(str),"\nSector ID:%.3d\nTrack ID:%.3d - Side ID:%.3d\nSize:%.5d (ID:0x%.2x)\n",
						hxcfe_getSectorConfigSectorID(guicontext->hxcfe,sl->sectorconfig),
						hxcfe_getSectorConfigTrackID(guicontext->hxcfe,sl->sectorconfig),
						hxcfe_getSectorConfigSideID(guicontext->hxcfe,sl->sectorconfig),
						sectorsize,
						hxcfe_getSectorConfigSizeID(guicontext->hxcfe,sl->sectorconfig)
						);
					fiw->buf->append((char*)str);

					snprintf(str,sizeof(str),"DataMark:0x%.2X\nHead CRC:0x%.4X (%s)\nData CRC:0x%.4X (%s)\n",
						hxcfe_getSectorConfigDataMark(guicontext->hxcfe,sl->sectorconfig),
						hxcfe_getSectorConfigHCRC(guicontext->hxcfe,sl->sectorconfig),hxcfe_getSectorConfigHCRCStatus(guicontext->hxcfe,sl->sectorconfig)?"BAD CRC!":"Ok",
						hxcfe_getSectorConfigDCRC(guicontext->hxcfe,sl->sectorconfig),hxcfe_getSectorConfigDCRCStatus(guicontext->hxcfe,sl->sectorconfig)?"BAD CRC!":"Ok");
					fiw->buf->append((char*)str);

					snprintf(str,sizeof(str),"Start Sector cell:%d\nStart Sector Data cell:%d\nEnd Sector cell:%d\n",
						hxcfe_getSectorConfigStartSectorIndex(guicontext->hxcfe,sl->sectorconfig),
						hxcfe_getSectorConfigStartDataIndex(guicontext->hxcfe,sl->sectorconfig),
						hxcfe_getSectorConfigEndSectorIndex(guicontext->hxcfe,sl->sectorconfig));
					fiw->buf->append((char*)str);

					if( hxcfe_getSectorConfigStartSectorIndex(guicontext->hxcfe,sl->sectorconfig) <= hxcfe_getSectorConfigEndSectorIndex(guicontext->hxcfe,sl->sectorconfig) )
					{
						snprintf(str,sizeof(str),"Number of cells:%d\n",
							hxcfe_getSectorConfigEndSectorIndex(guicontext->hxcfe,sl->sectorconfig) - hxcfe_getSectorConfigStartSectorIndex(guicontext->hxcfe,sl->sectorconfig));
					}
					else
					{
						track=(int)fiw->track_number_slide->value();
						side=(int)fiw->side_number_slide->value();

						hxc_entercriticalsection(guicontext->hxcfe,2);
						if(guicontext->trackviewerfloppy)
						{
							snprintf(str,sizeof(str),"Number of cells:%d\n",
								( ( hxcfe_getTrackLength(guicontext->hxcfe,guicontext->trackviewerfloppy,track,side) - hxcfe_getSectorConfigStartSectorIndex(guicontext->hxcfe,sl->sectorconfig) ) + hxcfe_getSectorConfigEndSectorIndex(guicontext->hxcfe,sl->sectorconfig) ) );
						}
						hxc_leavecriticalsection(guicontext->hxcfe,2);

					}

					fiw->buf->append((char*)str);

					sect_data = hxcfe_getSectorConfigInputData(guicontext->hxcfe,sl->sectorconfig);
					if(sect_data)
					{

						for(i=0;i< sectorsize ;i++)
						{
							if(!(i&0x7))
							{
								if(i)
								{
									str2[8]=0;
									strcat(fullstr,"| ");
									strcat(fullstr,str2);
								}

								snprintf(str,sizeof(str),"\n%.4X| ",i);
								strcat(fullstr,str);
							}

							snprintf(str,sizeof(str),"%.2X ",sect_data[i]);
							strcat(fullstr,str);

							c='.';
							if( (sect_data[i]>=32 && sect_data[i]<=126) )
								c = sect_data[i];

							str2[i&0x7]=c;

						}

						str2[8]=0;
						strcat(fullstr,"| ");
						strcat(fullstr,str2);
					}

					strcat(fullstr,"\n\n");

					fiw->buf->append((char*)fullstr);
			        //fiw->object_txt->textfont(FL_SCREEN);
					//fiw->object_txt->textsize(8);
					fiw->object_txt->buffer(fiw->buf);
				}
			}

			sl = sl->next_element;
		}

		hxc_entercriticalsection(guicontext->hxcfe,2);

		if(guicontext->trackviewerfloppy)
		{
			track = (int)fiw->track_number_slide->value();
			track = valuesanitycheck(track,0, hxcfe_getNumberOfTrack(guicontext->hxcfe,guicontext->trackviewerfloppy),&valmodif);
			if(valmodif)
				fiw->track_number_slide->value(track);

			side=(int)fiw->side_number_slide->value();
			side = valuesanitycheck(side,0, hxcfe_getNumberOfSide(guicontext->hxcfe,guicontext->trackviewerfloppy),&valmodif);
			if(valmodif)
				fiw->side_number_slide->value(side);

			if(hxcfe_getNumberOfTrack(guicontext->hxcfe,guicontext->trackviewerfloppy))
			{
				snprintf(str,sizeof(str),"Track RPM : %d RPM\n",hxcfe_getTrackRPM(guicontext->hxcfe,guicontext->trackviewerfloppy,track));
				strcat(fullstr,str);


				if( hxcfe_getTrackBitrate(guicontext->hxcfe,guicontext->trackviewerfloppy,track,side) == -1)
				{
					snprintf(str,sizeof(str),"\nBitrate : VARIABLE\n");
				}
				else
				{
					snprintf(str,sizeof(str),"\nBitrate : %d bit/s\n",(int)hxcfe_getTrackBitrate(guicontext->hxcfe,guicontext->trackviewerfloppy,track,side) );
				}
				strcat(fullstr,str);

				snprintf(str,sizeof(str),"\nTrack format :\n%s\n",hxcfe_getTrackEncodingName(guicontext->hxcfe,hxcfe_getTrackEncoding(guicontext->hxcfe,guicontext->trackviewerfloppy,track,side)));
				strcat(fullstr,str);
				snprintf(str,sizeof(str),"\nTrack len : %d cells\n",(int)hxcfe_getTrackLength(guicontext->hxcfe,guicontext->trackviewerfloppy,track,side));
				strcat(fullstr,str);
				snprintf(str,sizeof(str),"Number of side : %d\n",hxcfe_getTrackNumberOfSide(guicontext->hxcfe,guicontext->trackviewerfloppy,track));
				strcat(fullstr,str);

				snprintf(str,sizeof(str),"\nInterface mode:\n%s\n%s\n",
					hxcfe_getFloppyInterfaceModeName(guicontext->hxcfe,guicontext->interfacemode),
					hxcfe_getFloppyInterfaceModeDesc(guicontext->hxcfe,guicontext->interfacemode));

				strcat(fullstr,str);

				fiw->buf->append((char*)fullstr);

				fiw->object_txt->buffer(fiw->buf);
			}
		}

		hxc_leavecriticalsection(guicontext->hxcfe,2);

		if(fiw->view_mode->value() <= MAX_TRACK_MODES_INDEX)
			hxc_leavecriticalsection(guicontext->hxcfe,1);

	}

}