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
// File : cb_floppy_infos_window.cxx
// Contains: 
//
// Written by:	DEL NERO Jean Francois
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include "floppy_infos_window.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <direct.h>

#include "fl_includes.h"

extern "C"
{
	#include "libhxcfe.h"
	#include "tracks/display_track.h"
	#include "usb_hxcfloppyemulator.h"
	#include "os_api.h"
	#include "thirdpartylibs/fdrawcmd/fdrawcmd.h"
}

#include "main.h"
#include "loader.h"
#include "fl_mouse_box.h"

extern s_gui_context * guicontext;

void update_graph(floppy_infos_window * w)
{
	s_trackdisplay * td;
	unsigned char * ptr1;
	int i,j,k;
	int disp_xsize;
	int disp_ysize;
	char tempstr[512];

	int track,side;

	disp_xsize=w->floppy_map_disp->w();
	disp_ysize=w->floppy_map_disp->h();
	
	if(w->window->shown())
	{
		w->window->make_current();
		
		td=guicontext->td;
		if(td)
		{
			hxcfe_td_setparams(guicontext->hxcfe,td,(int)(w->x_time->value()*1000),(int)w->y_time->value(),(int)(w->x_offset->value()*1000));

			if(guicontext->loadedfloppy)
			{
				if(w->track_number_slide->value()>=guicontext->loadedfloppy->floppyNumberOfTrack)
					w->track_number_slide->value(guicontext->loadedfloppy->floppyNumberOfTrack-1);
				
				if(w->side_number_slide->value()>=guicontext->loadedfloppy->floppyNumberOfSide)
					w->side_number_slide->value(guicontext->loadedfloppy->floppyNumberOfSide-1);

				w->track_number_slide->scrollvalue((int)w->track_number_slide->value(),1,0,guicontext->loadedfloppy->floppyNumberOfTrack);
				w->side_number_slide->scrollvalue((int)w->side_number_slide->value(),1,0,guicontext->loadedfloppy->floppyNumberOfSide);
				
				if(w->track_view_bt->value())
				{
					hxcfe_td_draw_track(guicontext->hxcfe,td,guicontext->loadedfloppy,(int)w->track_number_slide->value(),(int)w->side_number_slide->value());
				}
				else
				{
					if(w->disc_view_bt->value())
					{
						hxcfe_td_draw_disk(guicontext->hxcfe,td,guicontext->loadedfloppy);
					}
				}

				ptr1=(unsigned char*)td->framebuffer;
				k=0;
				j=0;
				for(i=0;i<td->xsize*td->ysize;i++)
				{
					ptr1[j++]=ptr1[k+0];
					ptr1[j++]=ptr1[k+1];
					ptr1[j++]=ptr1[k+2];
					k=k+4;
				}

				fl_draw_image((unsigned char *)td->framebuffer, w->floppy_map_disp->x(), w->floppy_map_disp->y(), td->xsize, td->ysize, 3, 0);

				track=(int)w->track_number_slide->value();
				side=(int)w->side_number_slide->value();

				sprintf(tempstr,"Track : %d Side : %d ",track,side);
				w->global_status->value(tempstr);

				sprintf(tempstr,"Track RPM tag : %d\nBitrate flag : %d\nTrack encoding flag : %x\n",
					guicontext->loadedfloppy->tracks[track]->floppyRPM,
					guicontext->loadedfloppy->tracks[track]->sides[side]->bitrate,
					guicontext->loadedfloppy->tracks[track]->sides[side]->track_encoding
					);
				
				w->buf->remove(0,w->buf->length());

				sprintf(tempstr,"Track RPM : %d RPM - ",guicontext->loadedfloppy->tracks[track]->floppyRPM);
				w->buf->append((char*)tempstr);
				
				if(guicontext->loadedfloppy->tracks[track]->sides[side]->bitrate==-1)
				{
					sprintf(tempstr,"Bitrate : VARIABLE\n");
				}
				else
				{
					sprintf(tempstr,"Bitrate : %d bit/s\n",guicontext->loadedfloppy->tracks[track]->sides[side]->bitrate);
				}
				w->buf->append((char*)tempstr);
				sprintf(tempstr,"Track format : %s\n",hxcfe_getTrackEncodingName(guicontext->hxcfe,guicontext->loadedfloppy->tracks[track]->sides[side]->track_encoding));
				w->buf->append((char*)tempstr);
				sprintf(tempstr,"Track len : %d cells\n",guicontext->loadedfloppy->tracks[track]->sides[side]->tracklen);
				w->buf->append((char*)tempstr);
				sprintf(tempstr,"Number of side : %d\n",guicontext->loadedfloppy->tracks[track]->number_of_side);
				w->buf->append((char*)tempstr);	

				sprintf(tempstr,"Interface mode: %s - %s\n",
					hxcfe_getFloppyInterfaceModeName(guicontext->hxcfe,guicontext->interfacemode),
					hxcfe_getFloppyInterfaceModeDesc(guicontext->hxcfe,guicontext->interfacemode)
					);
				w->buf->append((char*)tempstr);	

				w->object_txt->buffer(w->buf);
			}
		}
	}
}

void tick_infos(void *w) {
	
	s_trackdisplay * td;
	floppy_infos_window *window;	
	window=(floppy_infos_window *)w;
	
	if(window->window->shown())
	{
		window->window->make_current();
		td=guicontext->td;
		if(td)
		{
			if(guicontext->updatefloppyinfos)
			{
				guicontext->updatefloppyinfos=0;
				update_graph(window);
			}

			fl_draw_image((unsigned char *)td->framebuffer, window->floppy_map_disp->x(), window->floppy_map_disp->y(), td->xsize, td->ysize, 3, 0);
		}
	}
				
	Fl::repeat_timeout(0.1, tick_infos, w);  
}

int InfosThreadProc(void* floppycontext,void* hw_context)
{
	return 0;	
}

void floppy_infos_window_bt_read(Fl_Button* bt, void*)
{
	floppy_infos_window *fdw;
	Fl_Window *dw;
	
	dw=((Fl_Window*)(bt->parent()));
	fdw=(floppy_infos_window *)dw->user_data();
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
}

void mouse_di_cb(Fl_Widget *o, void *v)
{
	floppy_infos_window *fiw;
	Fl_Window *dw;
	Fl_Mouse_Box *dnd = (Fl_Mouse_Box*)o;
	double stepperpix_x;
	double stepperpix_y;
	int xpos,ypos;
	char str[512];
	int disp_xsize,disp_ysize,disp_xpos,disp_ypos;
	
	dw=((Fl_Window*)(o->parent()));
	fiw=(floppy_infos_window *)dw->user_data();

	disp_xsize=fiw->floppy_map_disp->w();
	disp_ysize=fiw->floppy_map_disp->h();
	disp_xpos=fiw->floppy_map_disp->x();
	disp_ypos=fiw->floppy_map_disp->y();

	if(dnd->event() == FL_ENTER)
	{
		stepperpix_x=(fiw->x_time->value()*1000)/disp_xsize;
		stepperpix_y=(fiw->y_time->value())/(double)disp_ysize;

		xpos=Fl::event_x() - disp_xpos;
		if(xpos<0) xpos=0;
		if(xpos>disp_xsize) xpos=disp_xsize-1;

		ypos=Fl::event_y() - disp_ypos;
		
		if(ypos<0) ypos=0;
		if(ypos>disp_ysize) ypos=disp_ysize-1;

		ypos=disp_ysize-ypos;

		sprintf(str,"x position : %.3f ms",	(xpos * stepperpix_x)/1000);
		fiw->x_pos->value(str);
		sprintf(str,"y position : %.3f us",	ypos*stepperpix_y);
		fiw->y_pos->value(str);
	}
}