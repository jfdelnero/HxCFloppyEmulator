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
// File : cb_floppy_streamer_window.cxx
// Contains: Floppy Streamer GUI.
//
// Written by: Jean-François DEL NERO
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include "floppy_streamer_window.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "fl_includes.h"

#include "batch_converter_window.h"
#include "filesystem_generator_window.h"
#include "cb_filesystem_generator_window.h"
#include "cb_floppy_streamer_window.h"

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
void * cmd_connection;
void * dat_connection;

extern bmaptype * pauline_bmp;

#pragma pack(1)
typedef struct _chunk_header
{
	uint32_t header;       // CHKH -  0x43, 0x48, 0x4B, 0x48 - 484B4843
	uint32_t size;         // Header + data +CRC
	uint32_t packet_number;
}chunk_header;

#define HXCSTREAM_HEADERSIGN 0x484B4843

FILE * fstreamin = NULL;
char tmp_stream_file_name[256];

#pragma pack()

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

void update_graph(floppy_streamer_window * w)
{

}

void streamer_tick_infos(void *w)
{
	HXCFE_TD * td;
	uint32_t flags;
	floppy_streamer_window *window;
	window=(floppy_streamer_window *)w;

	if(window->window->shown())
	{
		window->window->make_current();
		td = guicontext->td_stream;
		if(td)
		{
			if( (window->floppy_map_disp->w() != hxcfe_td_getframebuffer_xres(td)) || (window->floppy_map_disp->h() != hxcfe_td_getframebuffer_yres(td)) )
			{
				// Resized... Realloc needed

				hxcfe_td_deinit(guicontext->td_stream);

				guicontext->td_stream = NULL;
				td = NULL;

				guicontext->td_stream = hxcfe_td_init(guicontext->hxcfe,window->floppy_map_disp->w(),window->floppy_map_disp->h());
				if(guicontext->td_stream)
				{
					td = guicontext->td_stream;
					if(guicontext->stream_frame_buffer)
						free(guicontext->stream_frame_buffer);

					flags = 0;

					if(window->high_contrast->value())
						flags |= TD_FLAG_HICONTRAST;

					if(window->fat_dots->value())
						flags |= TD_FLAG_BIGDOT;

					hxcfe_td_setparams(td,(int)(adjust_timescale(window->x_time->value())),(int)window->y_time->value(),0,flags);

					guicontext->stream_frame_buffer = (unsigned char*)malloc( hxcfe_td_getframebuffer_xres(td) * hxcfe_td_getframebuffer_yres(td) * 3);
					if(guicontext->stream_frame_buffer)
					{
						memset(guicontext->stream_frame_buffer,0xFF,hxcfe_td_getframebuffer_xres(td)*hxcfe_td_getframebuffer_yres(td) * 3);
						splash_sprite(pauline_bmp,guicontext->stream_frame_buffer, hxcfe_td_getframebuffer_xres(td), hxcfe_td_getframebuffer_yres(td), hxcfe_td_getframebuffer_xres(td) / 2 - pauline_bmp->Xsize / 2, hxcfe_td_getframebuffer_yres(td) / 2 - pauline_bmp->Ysize / 2);
					}
				}
			}

			if(td)
				 fl_draw_image((unsigned char *)guicontext->stream_frame_buffer, window->floppy_map_disp->x(), window->floppy_map_disp->y(), hxcfe_td_getframebuffer_xres(td), hxcfe_td_getframebuffer_yres(td), 3, 0);
		}
	}

	Fl::repeat_timeout(0.1, streamer_tick_infos, w);
}

static void clean_string(char * str)
{
	int i;

	i = 0;

	while(str[i])
	{
		if(str[i] == '\r' || str[i] == '\n')
		{
			str[i] = ' ';
		}
		i++;
	}

}
int StreamerThreadRxStatusProc(void* floppycontext,void* context)
{
	char tmpstr[1024];
	streamthread * streamth;
	floppy_streamer_window *w;

	streamth = (streamthread*)context;
	w = streamth->window;

	while(1)
	{
		if(cmd_connection && dat_connection)
		{
			memset(tmpstr,0,sizeof(tmpstr));
			network_read2(cmd_connection, (unsigned char*)tmpstr, sizeof(tmpstr),0);
			clean_string(tmpstr);
			w->global_status->value(tmpstr);
		}
		else
		{
			hxc_pause(100);
		}
	}

	return 0;
}

#define MAX_CHUNKSIZE 512*1024
#define MAX_CHUNK_BLOCKS_PER_PICTURE 16

int StreamerThreadRxDataProc(void* floppycontext,void* context)
{
	unsigned char * buffer;
	chunk_header * ch;
	int ret,failure;
	time_t time_now;
	struct tm * time_info;

	buffer =(unsigned char*) malloc(MAX_CHUNKSIZE);
	if(!buffer)
		return -1;

	memset(tmp_stream_file_name,0,sizeof(tmp_stream_file_name));

	// Get system time
	time(&time_now);
	time_info = localtime(&time_now);
	snprintf(tmp_stream_file_name,sizeof(tmp_stream_file_name),"pauline_tmp_buffer_%.4d-%.2d-%.2d_%.2dh%.2dm%.2ds.hxcstream",time_info->tm_year + 1900, \
	                                                                                       time_info->tm_mon+1, \
	                                                                                       time_info->tm_mday, \
	                                                                                       time_info->tm_hour, \
	                                                                                       time_info->tm_min, \
	                                                                                       time_info->tm_sec );

	ret = 0;
	ch = (chunk_header *)buffer;

	while( ret >= 0 )
	{
		if(cmd_connection && dat_connection)
		{
			failure = 1;
			memset(ch,0,sizeof(chunk_header));
			ret = network_read(dat_connection, (unsigned char*)ch, sizeof(chunk_header),0);
			if(ch->header == HXCSTREAM_HEADERSIGN)
			{
				if( (ch->size >= sizeof(chunk_header)) && ch->size < MAX_CHUNKSIZE)
				{
					ret = network_read(dat_connection, (unsigned char*)&buffer[sizeof(chunk_header)], ch->size - sizeof(chunk_header),0);

					if(!fstreamin)
						fstreamin = fopen(tmp_stream_file_name,"wb");

					if(fstreamin)
					{
						fwrite(buffer,ch->size,1,fstreamin);
						fflush(fstreamin);
					}
					failure = 0;
				}
			}

			if(failure)
			{
				hxc_pause(100);
			}
		}
		else
		{
			hxc_pause(100);
		}
	}

	free(buffer);

	if(fstreamin)
		fclose(fstreamin);

	return 0;
}

int StreamerThreadProc(void* floppycontext,void* context)
{
	int i,nbpixel;
	HXCFE_TD * td;
	streamthread * streamth;
	s_gui_context * guicontext;
	floppy_streamer_window *w;
	unsigned char * ptr1;
	unsigned char * ptr2;
	unsigned char * buffer;
	unsigned char * full_track_buffer;
	int file_offset,tmp_file_offset,current_file_size;
	unsigned long * offset_table;
	int offset_table_index;
	FILE * f;
	uint32_t flags;
	chunk_header * ch;
	int ret;
	HXCFE_FXSA * fxsa;
	HXCFE_TRKSTREAM* trkstream;
#if 0
	uint16_t * wavebuf;
	int snd_stream_index,snd_stream_index_old;
#endif

	streamth = (streamthread*)context;
	guicontext = (s_gui_context *)streamth->guicontext;
	w = streamth->window;

	buffer = NULL;
	offset_table = NULL;
	full_track_buffer = NULL;

	buffer = (unsigned char*) malloc(1024*1024);
	if(!buffer)
		goto error;

	offset_table = (unsigned long *)malloc(128*1024);
	if(!offset_table)
		goto error;

	memset(offset_table, 0, 128*1024);

	full_track_buffer = (unsigned char*) malloc(1024*1024);
	if(!full_track_buffer)
		goto error;

	memset(full_track_buffer,0,1024*1024);

	ret = 0;
	ch = (chunk_header *)buffer;

	offset_table_index = 0;

	file_offset = 0;
	while( ret >= 0 )
	{
		if(cmd_connection && dat_connection)
		{
			do
			{
				hxc_pause(100);
			}while(!fstreamin);

			hxc_pause(50);

			f = fopen(tmp_stream_file_name,"rb");
			if(f)
			{
				do
				{
					fseek(f,0,SEEK_END);
					current_file_size = ftell(f);
					fseek(f,file_offset,SEEK_SET);

					ret = fread(buffer,sizeof(chunk_header), 1, f);
					if(ret == 1)
					{
						if( (int)(file_offset + ch->size) <= current_file_size )
						{
							ret = fread(&buffer[sizeof(chunk_header)],ch->size - sizeof(chunk_header), 1, f);
							if(ret == 1)
							{
								fxsa = hxcfe_initFxStream(guicontext->hxcfe);

								if(fxsa)
								{
									trkstream = hxcfe_FxStream_ImportHxCStreamBuffer(fxsa,buffer,ch->size);
									if(trkstream)
									{
										if(ch->packet_number == 0)
										{
											offset_table_index=0;
										}

										offset_table[offset_table_index] = file_offset;
										offset_table_index++;

										hxcfe_FxStream_FreeStream( fxsa, trkstream );

										if(offset_table_index > MAX_CHUNK_BLOCKS_PER_PICTURE)
											tmp_file_offset = offset_table[offset_table_index - MAX_CHUNK_BLOCKS_PER_PICTURE];
										else
											tmp_file_offset = offset_table[0];

										fseek(f,tmp_file_offset,SEEK_SET);
										if( fread(full_track_buffer,(file_offset - tmp_file_offset) + ch->size,1, f) == 1)
										{
											trkstream = hxcfe_FxStream_ImportHxCStreamBuffer(fxsa,full_track_buffer,(file_offset - tmp_file_offset) + ch->size);

											file_offset += ch->size;

											td = (HXCFE_TD *)guicontext->td_stream;

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

											flags = 0;

											if(w->high_contrast->value())
												flags |= TD_FLAG_HICONTRAST;

											if(w->fat_dots->value())
												flags |= TD_FLAG_BIGDOT;

											hxcfe_td_setparams(td,(int)(adjust_timescale(w->x_time->value())),(int)w->y_time->value(),0,flags);

											if(trkstream)
											{
												hxcfe_td_draw_trkstream( td, trkstream );

												ptr1 = (unsigned char*)hxcfe_td_getframebuffer(td);
												ptr2 = (unsigned char*)guicontext->stream_frame_buffer;
												nbpixel = hxcfe_td_getframebuffer_xres(td)*hxcfe_td_getframebuffer_yres(td);
												for(i=0;i<nbpixel;i++)
												{
													*ptr2++ = *ptr1++;
													*ptr2++ = *ptr1++;
													*ptr2++ = *ptr1++;
													ptr1++;
												}

												hxcfe_FxStream_FreeStream( fxsa, trkstream );
											}
										}
									}
									else
									{
										hxc_pause(10);
									}

									hxcfe_deinitFxStream( fxsa );
								}
								else
								{
									hxc_pause(10);
								}
							}
							else
							{
								hxc_pause(10);
							}
						}
						else
						{
							fseek(f,file_offset,SEEK_SET);
							hxc_pause(10);
						}
					}
					else
					{
						hxc_pause(50);
					}

					fseek(f,file_offset,SEEK_SET);

				}while(1);
			}
			else
			{
				printf("Error!!!!\n");
			}
		}
		else
		{
			hxc_pause(200);
		}
	}
	return 0;

error:
	free(buffer);
	free(offset_table);
	free(full_track_buffer);

	return -1;
}

void floppy_streamer_ok(Fl_Button*, void* w)
{
	floppy_streamer_window *fdw;

	fdw=(floppy_streamer_window *)w;

	fdw->window->hide();
}

void floppy_streamer_connect(Fl_Button*, void* w)
{
	floppy_streamer_window *fdw;

	fdw=(floppy_streamer_window *)w;

	cmd_connection = network_connect((char*)fdw->server_address->value(),600);
	if(cmd_connection)
	{

		dat_connection = network_connect((char*)fdw->server_address->value(),601);
		if(dat_connection)
		{
			fdw->global_status->value("Connected");
		}
	}
}

static int isdigit(char c)
{
	if(c>=0 && c<='9')
		return 1;

	return 0;
}

static int digits_only(const char *s)
{
	int len;

	len = 0;

	while (*s)
	{
		if (isdigit(*s++) == 0)
		{
			return 0;
		}
		else
		{
			len++;
		}
	}

	return len;
}

void floppy_streamer_readdisk(Fl_Button*, void* w)
{
	char tmp[256];
	int index;
	char index_mode[32];
	floppy_streamer_window *fdw;

	fdw=(floppy_streamer_window *)w;

	if(cmd_connection && dat_connection)
	{
		snprintf(tmp,sizeof(tmp),"index_to_dump %d\n",atoi(fdw->index_delay->value()));
		network_write(cmd_connection, (unsigned char*)tmp, strlen(tmp),2);

		snprintf(tmp,sizeof(tmp),"dump_time %d\n",atoi(fdw->dump_lenght->value()));
		network_write(cmd_connection, (unsigned char*)tmp, strlen(tmp),2);

		if(strlen(fdw->index_name->value()) && digits_only(fdw->index_name->value()))
		{
			index = atoi(fdw->index_name->value());
			strcpy(index_mode,"MANUAL_INDEX_NAME");
		}
		else
		{
			index = 1;
			strcpy(index_mode,"AUTO_INDEX_NAME");
		}

		snprintf(tmp,sizeof(tmp),"dump %d %d %d %d %d %d %d %d %d \"%s\" \"%s\" %d %s\n", \
													  fdw->drive_choice->value(),
													  atoi(fdw->min_track->value()), \
													  atoi(fdw->max_track->value()), \
													 (fdw->Side_0->value()^1)&1 , \
													  fdw->Side_1->value() , \
													  fdw->high_res->value(), \
													  fdw->double_step->value(), \
													  fdw->ignore_index->value(), \
													  0, \
													  fdw->dump_name->value(), \
													  fdw->comment->value(), \
													  index, \
													  index_mode \
													  );

		network_write(cmd_connection, (unsigned char*)tmp, strlen(tmp),2);
	}
}

void floppy_streamer_readtrack(Fl_Button*, void* w)
{
	char tmp[256];
	int index;
	char index_mode[32];
	floppy_streamer_window *fdw;

	fdw=(floppy_streamer_window *)w;

	if(cmd_connection)
	{
		snprintf(tmp,sizeof(tmp),"index_to_dump %d\n",atoi(fdw->index_delay->value()));
		network_write(cmd_connection, (unsigned char*)tmp, strlen(tmp),2);

		snprintf(tmp,sizeof(tmp),"dump_time %d\n",atoi(fdw->dump_lenght->value()));
		network_write(cmd_connection, (unsigned char*)tmp, strlen(tmp),2);

		if( digits_only(fdw->index_name->value()) )
		{
			index = atoi(fdw->index_name->value());
			strcpy(index_mode,"MANUAL_INDEX_NAME");
		}
		else
		{
			index = 1;
			strcpy(index_mode,"AUTO_INDEX_NAME");
		}

		snprintf(tmp,sizeof(tmp),"dump %d -1 -1 %g %g %d %d %d %d \"%s\" \"%s\" %d %s\n", \
														fdw->drive_choice->value(), \
														fdw->side_number_slide->value(), \
														fdw->side_number_slide->value(), \
														fdw->high_res->value(), \
														fdw->double_step->value(), \
														fdw->ignore_index->value(), \
														0, \
														fdw->dump_name->value(), \
														fdw->comment->value(), \
														index, \
														index_mode \
														);

		network_write(cmd_connection, (unsigned char*)tmp, strlen(tmp),2);
	}
}

void floppy_streamer_spybus(Fl_Button*, void* w)
{
	char tmp[256];

	floppy_streamer_window *fdw;
	fdw=(floppy_streamer_window *)w;

	if(cmd_connection)
	{
		snprintf(tmp,sizeof(tmp),"index_to_dump %d\n",atoi(fdw->index_delay->value()));
		network_write(cmd_connection, (unsigned char*)tmp, strlen(tmp),2);

		snprintf(tmp,sizeof(tmp),"dump_time %d\n",atoi(fdw->dump_lenght->value()));
		network_write(cmd_connection, (unsigned char*)tmp, strlen(tmp),2);

		snprintf(tmp,sizeof(tmp),"dump %d -1 -1 %g %g %d %d %d 1\n",fdw->drive_choice->value(),fdw->side_number_slide->value(),fdw->side_number_slide->value(),fdw->high_res->value(),fdw->double_step->value(),fdw->ignore_index->value());
		network_write(cmd_connection, (unsigned char*)tmp, strlen(tmp),2);
	}
}

void floppy_streamer_clear_bt(Fl_Button*, void* w)
{
	floppy_streamer_window *fdw;

	fdw=(floppy_streamer_window *)w;

	fdw->dump_name->value("");
	fdw->comment->value("");
	fdw->index_name->value("");

	Fl::focus(fdw->dump_name);
}

void floppy_streamer_stop(Fl_Button*, void* w)
{
	char tmpstr[64];

	if(cmd_connection)
	{
		snprintf(tmpstr,sizeof(tmpstr),"stop\n");
		network_write(cmd_connection, (unsigned char*)tmpstr, strlen(tmpstr),2);
	}
}

void floppy_streamer_eject(Fl_Button*, void* w)
{
	char tmpstr[64];
	floppy_streamer_window *fdw;

	fdw=(floppy_streamer_window *)w;

	if(cmd_connection)
	{
		snprintf(tmpstr,sizeof(tmpstr),"ejectdisk %d\n",fdw->drive_choice->value());
		network_write(cmd_connection, (unsigned char*)tmpstr, strlen(tmpstr),2);
	}
}

void floppy_streamer_mode3(Fl_Light_Button* b, void* w)
{
	char tmpstr[64];

	if(cmd_connection)
	{
		if(b->value())
			snprintf(tmpstr,sizeof(tmpstr),"setio DRIVES_PORT_PIN02_OUT\n");
		else
			snprintf(tmpstr,sizeof(tmpstr),"cleario DRIVES_PORT_PIN02_OUT\n");

		network_write(cmd_connection, (unsigned char*)tmpstr, strlen(tmpstr),2);
	}
}

void floppy_streamer_trackup(Fl_Button*, void* w)
{
	char tmpstr[64];
	floppy_streamer_window *fdw;

	fdw=(floppy_streamer_window *)w;

	if(cmd_connection)
	{
		snprintf(tmpstr,sizeof(tmpstr),"headstep %d 1\n",fdw->drive_choice->value());
		network_write(cmd_connection, (unsigned char*)tmpstr, strlen(tmpstr),2);
	}
}

void floppy_streamer_down(Fl_Button*, void* w)
{
	char tmpstr[64];
	floppy_streamer_window *fdw;

	fdw=(floppy_streamer_window *)w;

	if(cmd_connection)
	{
		snprintf(tmpstr,sizeof(tmpstr),"headstep %d -1\n",fdw->drive_choice->value());
		network_write(cmd_connection, (unsigned char*)tmpstr, strlen(tmpstr),2);
	}
}

void floppy_streamer_recalibrate(Fl_Button*, void* w)
{
	char tmpstr[64];
	floppy_streamer_window *fdw;

	fdw=(floppy_streamer_window *)w;

	if(cmd_connection)
	{
		snprintf(tmpstr,sizeof(tmpstr),"recalibrate %d\n",fdw->drive_choice->value());
		network_write(cmd_connection, (unsigned char*)tmpstr, strlen(tmpstr),2);
	}
}

void floppy_streamer_movehead(Fl_Button*, void* w)
{
	char tmpstr[64];
	floppy_streamer_window *fdw;
	fdw=(floppy_streamer_window *)w;

	if(cmd_connection)
	{
		snprintf(tmpstr,sizeof(tmpstr),"movehead %d %g\n",fdw->drive_choice->value(),fdw->track_number_slide->value());
		network_write(cmd_connection, (unsigned char*)tmpstr, strlen(tmpstr),2);
	}
}

void disk_streamer_window_callback(Fl_Widget *o, void *v)
{

}
