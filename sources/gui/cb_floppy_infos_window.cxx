/*
//
// Copyright (C) 2006 - 2013 Jean-François DEL NERO
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

#include "fl_includes.h"

extern "C"
{
	#include "libhxcfe.h"
	#include "tracks/display_track.h"
	#include "usb_hxcfloppyemulator.h"
}

#include "main.h"
#include "loader.h"
#include "fl_mouse_box.h"

extern s_gui_context * guicontext;

char fullstr[256*1024];

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
		
		w->object_txt->textsize(9);
		w->object_txt->textfont(FL_SCREEN);

		td=guicontext->td;
		if(td)
		{
			hxcfe_td_activate_analyzer(guicontext->hxcfe,td,ISOIBM_MFM_ENCODING,w->iso_mfm_bt->value());
			hxcfe_td_activate_analyzer(guicontext->hxcfe,td,ISOIBM_FM_ENCODING,w->iso_fm_bt->value());
			hxcfe_td_activate_analyzer(guicontext->hxcfe,td,AMIGA_MFM_ENCODING,w->amiga_mfm_bt->value());
			hxcfe_td_activate_analyzer(guicontext->hxcfe,td,EMU_FM_ENCODING,w->eemu_bt->value());
			hxcfe_td_activate_analyzer(guicontext->hxcfe,td,MEMBRAIN_MFM_ENCODING,w->membrain_bt->value());
			hxcfe_td_activate_analyzer(guicontext->hxcfe,td,TYCOM_FM_ENCODING,w->tycom_bt->value());
			hxcfe_td_activate_analyzer(guicontext->hxcfe,td,APPLEII_GCR1_ENCODING,w->apple2_bt->value());
			hxcfe_td_activate_analyzer(guicontext->hxcfe,td,APPLEII_GCR2_ENCODING,w->apple2_bt->value());
			hxcfe_td_activate_analyzer(guicontext->hxcfe,td,ARBURG_ENCODING,w->arburg_bt->value());
			hxcfe_td_activate_analyzer(guicontext->hxcfe,td,ARBURGSYS_ENCODING,w->arburg_bt->value());

			hxcfe_td_setparams(guicontext->hxcfe,td,(int)(w->x_time->value()),(int)w->y_time->value(),(int)(w->x_offset->value()*1000));

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
					(int)guicontext->loadedfloppy->tracks[track]->floppyRPM,
					(int)guicontext->loadedfloppy->tracks[track]->sides[side]->bitrate,
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
					sprintf(tempstr,"Bitrate : %d bit/s\n",(int)guicontext->loadedfloppy->tracks[track]->sides[side]->bitrate);
				}
				w->buf->append((char*)tempstr);
				sprintf(tempstr,"Track format : %s\n",hxcfe_getTrackEncodingName(guicontext->hxcfe,guicontext->loadedfloppy->tracks[track]->sides[side]->track_encoding));
				w->buf->append((char*)tempstr);
				sprintf(tempstr,"Track len : %d cells\n",(int)guicontext->loadedfloppy->tracks[track]->sides[side]->tracklen);
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
			else
			{
				ptr1=(unsigned char*)td->framebuffer;
				k=0;
				j=0;
				for(i=0;i<td->xsize*td->ysize;i++)
				{
					ptr1[j++]=0xFF;//ptr1[k+0];
					ptr1[j++]=0xFF;//ptr1[k+1];
					ptr1[j++]=0xFF;//ptr1[k+2];
					k=k+4;
				}

				fl_draw_image((unsigned char *)td->framebuffer, w->floppy_map_disp->x(), w->floppy_map_disp->y(), td->xsize, td->ysize, 3, 0);
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

	if(window->x_time->value()>1000)
	{
		window->x_time->step(1000);
	}
	else
	{
		if(window->x_time->value()>100)
		{
			window->x_time->step(64);
		}
		else
		{
			window->x_time->step(1);
		}

	}

	update_graph(window);
}

void mouse_di_cb(Fl_Widget *o, void *v)
{
	unsigned int i;
	floppy_infos_window *fiw;
	Fl_Window *dw;
	Fl_Mouse_Box *dnd = (Fl_Mouse_Box*)o;
	double stepperpix_x;
	double stepperpix_y;
	int xpos,ypos;
	char str[512];
	char str2[512];
	int track,side;

	unsigned char c;
	s_sectorlist * sl;
	int disp_xsize,disp_ysize,disp_xpos,disp_ypos;
	
	dw=((Fl_Window*)(o->parent()));
	fiw=(floppy_infos_window *)dw->user_data();

	disp_xsize=fiw->floppy_map_disp->w();
	disp_ysize=fiw->floppy_map_disp->h();
	disp_xpos=fiw->floppy_map_disp->x();
	disp_ypos=fiw->floppy_map_disp->y();

	if(dnd->event() == FL_ENTER)
	{
		stepperpix_x=(fiw->x_time->value())/disp_xsize;
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

		fiw->buf->remove(0,fiw->buf->length());

		ypos = disp_ysize - ypos;

		fiw->object_txt->textsize(9);
		fiw->object_txt->textfont(FL_SCREEN);

		sl=guicontext->td->sl;
		fullstr[0]=0;

		while(sl)
		{
			if(sl->sectorconfig)
			{
				fullstr[0]=0;
				if( ( xpos>=sl->x_pos1 && xpos<=sl->x_pos2 ) && 
					( ypos>=(sl->y_pos1) && ypos<=(sl->y_pos2) ) )
				{

					switch(sl->sectorconfig->trackencoding)
					{
						case ISOFORMAT_SD:
							sprintf(str,"FM   ");
						break;
						case ISOFORMAT_DD:
							sprintf(str,"MFM  ");
						break;
						case AMIGAFORMAT_DD:
							sprintf(str,"AMFM ");
						break;
						case TYCOMFORMAT_SD:
							sprintf(str,"TYCOM ");
						break;
						case MEMBRAINFORMAT_DD:
							sprintf(str,"MEMBRAIN ");
						break;
						case EMUFORMAT_SD:
							sprintf(str,"E-mu ");
						break;
						case APPLE2_GCR5A3:
							sprintf(str,"AppleII 5A3 ");
						break;
						case APPLE2_GCR6A2:
							sprintf(str,"AppleII 6A2 ");
						break;
						case ARBURG_SD:
							sprintf(str,"Arburg SD ");
						break;
						case ARBURG_SYS:
							sprintf(str,"Arburg SYS ");
						break;
						default:
							sprintf(str,"Unknow ");
						break;
					}
					fiw->buf->append((char*)str);
	
					sprintf(str,"Sector ID %.3d - Size %.5d - Size ID 0x%.2x - Track ID %.3d - Side ID %.3d\n",
						sl->sectorconfig->sector,
						sl->sectorconfig->sectorsize,
						sl->sectorconfig->alternate_sector_size_id,
						sl->sectorconfig->cylinder,
						sl->sectorconfig->head);
					fiw->buf->append((char*)str);

					sprintf(str,"DataMark:0x%.2X - Head CRC 0x%.4X (%s) - Data CRC 0x%.4X (%s)\n",
						sl->sectorconfig->alternate_datamark,
						sl->sectorconfig->header_crc,sl->sectorconfig->use_alternate_header_crc?"BAD CRC!":"Ok",
						sl->sectorconfig->data_crc,sl->sectorconfig->use_alternate_data_crc?"BAD CRC!":"Ok");
					fiw->buf->append((char*)str);

					sprintf(str,"Start Sector cell : %lu, Start Sector Data cell %lu, End Sector cell %lu\n",
						sl->sectorconfig->startsectorindex,
						sl->sectorconfig->startdataindex,
						sl->sectorconfig->endsectorindex);
					fiw->buf->append((char*)str);

					if(sl->sectorconfig->startsectorindex <= sl->sectorconfig->endsectorindex)
					{
						sprintf(str,"Number of cells : %lu\n",
							sl->sectorconfig->endsectorindex-sl->sectorconfig->startsectorindex);
					}
					else
					{
						track=(int)fiw->track_number_slide->value();
						side=(int)fiw->side_number_slide->value();
						sprintf(str,"Number of cells : %lu\n",
							( (guicontext->loadedfloppy->tracks[track]->sides[side]->tracklen - sl->sectorconfig->startsectorindex ) + sl->sectorconfig->endsectorindex ) );
					}

					fiw->buf->append((char*)str);


					if(sl->sectorconfig->input_data)
					{
						for(i=0;i<sl->sectorconfig->sectorsize;i++)
						{
							if(!(i&0xF))
							{
								if(i)
								{
									str2[16]=0;
									strcat(fullstr,"| ");
									strcat(fullstr,str2);
								}

								sprintf(str,"\n0x%.4X | ",i);
								strcat(fullstr,str);
							}

							sprintf(str,"%.2X ",sl->sectorconfig->input_data[i]);
							strcat(fullstr,str);				

							c='.';
							if( (sl->sectorconfig->input_data[i]>=32 && sl->sectorconfig->input_data[i]<=126) )
								c=sl->sectorconfig->input_data[i];

							str2[i&0xF]=c;

						}
					
						str2[16]=0;
						strcat(fullstr,"| ");
						strcat(fullstr,str2);
					}
			
					strcat(fullstr,"\n\n");				


					fiw->buf->append((char*)fullstr);					
					
					fiw->object_txt->buffer(fiw->buf);
				}
			}

			sl = sl->next_element;
		}

		if(guicontext->loadedfloppy)
		{
			track=(int)fiw->track_number_slide->value();
			side=(int)fiw->side_number_slide->value();

			sprintf(str,"Track RPM : %d RPM - ",guicontext->loadedfloppy->tracks[track]->floppyRPM);
			strcat(fullstr,str);
				
			if(guicontext->loadedfloppy->tracks[track]->sides[side]->bitrate==-1)
			{
				sprintf(str,"Bitrate : VARIABLE\n");
			}
			else
			{
				sprintf(str,"Bitrate : %d bit/s\n",(int)guicontext->loadedfloppy->tracks[track]->sides[side]->bitrate);
			}
			strcat(fullstr,str);

			sprintf(str,"Track format : %s\n",hxcfe_getTrackEncodingName(guicontext->hxcfe,guicontext->loadedfloppy->tracks[track]->sides[side]->track_encoding));
			strcat(fullstr,str);
			sprintf(str,"Track len : %d cells\n",(int)guicontext->loadedfloppy->tracks[track]->sides[side]->tracklen);
			strcat(fullstr,str);
			sprintf(str,"Number of side : %d\n",guicontext->loadedfloppy->tracks[track]->number_of_side);
			strcat(fullstr,str);

			sprintf(str,"Interface mode: %s - %s\n",
				hxcfe_getFloppyInterfaceModeName(guicontext->hxcfe,guicontext->interfacemode),
				hxcfe_getFloppyInterfaceModeDesc(guicontext->hxcfe,guicontext->interfacemode));
			
			strcat(fullstr,str);

			fiw->buf->append((char*)fullstr);

			fiw->object_txt->buffer(fiw->buf);
		}

	}
}