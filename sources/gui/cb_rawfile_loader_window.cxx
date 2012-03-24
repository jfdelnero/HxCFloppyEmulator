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
// File : cb_rawfile_loader_window.cxx
// Contains: RAW image loader window
//
// Written by:	DEL NERO Jean Francois
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include "rawfile_loader_window.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "fileselector.h"

extern "C"
{
	#include "libhxcfe.h"
	#include "../usb_floppyemulator/usb_hxcfloppyemulator.h"
}

#include "loader.h"

extern HXCFLOPPYEMULATOR * flopemu;

void getWindowState(rawfile_loader_window *rlw,cfgrawfile *rfc)
{
	memset(rfc,0,sizeof(cfgrawfile));
	rfc->autogap3=0xff;
	if(rlw->chk_twosides->value())
		rfc->sidecfg=2;
	else
		rfc->sidecfg=1;
	rfc->bitrate=(unsigned long)rlw->innum_bitrate->value();
	rfc->fillvalue=(unsigned char)rlw->numin_formatvalue->value();
	rfc->numberoftrack=(unsigned long)rlw->innum_nbtrack->value();
	rfc->gap3=(unsigned long)rlw->numin_gap3->value();
	rfc->firstidsector=(unsigned char)rlw->innum_sectoridstart->value();
	rfc->interleave=(unsigned char)rlw->numin_interleave->value();
	rfc->sectorpertrack=(unsigned char)rlw->innum_sectorpertrack->value();
	rfc->rpm=(unsigned long)rlw->innum_rpm->value();
	rfc->sectorsize=rlw->choice_sectorsize->value();
	rfc->skew=(unsigned char)rlw->numin_skew->value();
	rfc->tracktype=rlw->choice_tracktype->value();
}

void setWindowState(rawfile_loader_window *rlw,cfgrawfile *rfc)
{	
	if(rfc->autogap3)
		rlw->chk_autogap3->value(1);
	else
		rlw->chk_autogap3->value(0);

	if(rfc->sidecfg==2)
		rlw->chk_twosides->value(1);
	else
		rlw->chk_twosides->value(0);

	rlw->innum_bitrate->value(rfc->bitrate);
	rlw->numin_formatvalue->value(rfc->fillvalue);
	rlw->innum_nbtrack->value(rfc->numberoftrack);
	rlw->numin_gap3->value(rfc->gap3);
	rlw->innum_sectoridstart->value(rfc->firstidsector);
	rlw->numin_interleave->value(rfc->interleave);
	rlw->innum_sectorpertrack->value(rfc->sectorpertrack);
	rlw->innum_rpm->value(rfc->rpm);
	rlw->choice_sectorsize->value(rfc->sectorsize);
	rlw->numin_skew->value(rfc->skew);
}

void raw_loader_window_datachanged(Fl_Widget* w, void*)
{
	int totalsector,totalsize;
	int temp[256];
	rawfile_loader_window *rlw;
	Fl_Window *dw;

	dw=((Fl_Window*)(w->parent()));
	rlw=(rawfile_loader_window *)dw->user_data();

	totalsector=(int)(rlw->innum_nbtrack->value() * 	rlw->innum_sectorpertrack->value());
	if(rlw->chk_twosides->value())
		totalsector=totalsector*2;

	sprintf((char*)temp,"%d",totalsector);

	rlw->strout_totalsector->value((const char*)temp);
	totalsize=totalsector * (128<<rlw->choice_sectorsize->value());
	sprintf((char*)temp,"%d",totalsize);
	rlw->strout_totalsize->value((const char*)temp);
}

void raw_loader_window_bt_loadrawfile(Fl_Button* bt, void*)
{
	int totalsector,totalsize;
	int temp[256];
	char file[1024];
	rawfile_loader_window *rlw;
	Fl_Window *dw;
	cfgrawfile rfc;

	memset(file,0,sizeof(file));

	dw=((Fl_Window*)(bt->parent()));
	rlw=(rawfile_loader_window *)dw->user_data();

	getWindowState(rlw,&rfc);

	totalsector=(int)(rlw->innum_nbtrack->value() * 	rlw->innum_sectorpertrack->value());
	if(rlw->chk_twosides->value())
		totalsector=totalsector*2;

	sprintf((char*)temp,"%d",totalsector);

	rlw->strout_totalsector->value((const char*)temp);
	totalsize=totalsector * (128<<rlw->choice_sectorsize->value());
	sprintf((char*)temp,"%d",totalsize);
	rlw->strout_totalsize->value((const char*)temp);

	if(!fileselector("Select raw file",(char*)file,0,"*.img",0,0))
	{	
		loadrawfile(flopemu,&rfc,file);
		dw->hide();
	}
}

void raw_loader_window_bt_createemptyfloppy(Fl_Button* bt, void*)
{
	int totalsector,totalsize;
	int temp[256];
	rawfile_loader_window *rlw;
	Fl_Window *dw;
	cfgrawfile rfc;

	dw=((Fl_Window*)(bt->parent()));
	rlw=(rawfile_loader_window *)dw->user_data();

	totalsector=(int)(rlw->innum_nbtrack->value() * 	rlw->innum_sectorpertrack->value());
	if(rlw->chk_twosides->value())
		totalsector=totalsector*2;

	sprintf((char*)temp,"%d",totalsector);

	rlw->strout_totalsector->value((const char*)temp);
	totalsize=totalsector * (128<<rlw->choice_sectorsize->value());
	sprintf((char*)temp,"%d",totalsize);
	rlw->strout_totalsize->value((const char*)temp);

	getWindowState(rlw,&rfc);

	loadrawfile(flopemu,&rfc,NULL);

	dw->hide();
}


void raw_loader_window_bt_savecfg(Fl_Button* bt, void*)
{
	FILE * fpf_file;
	char file[1024];
	cfgrawfile rfc;
	rawfile_loader_window *rlw;
	Fl_Window *dw;

	dw=((Fl_Window*)(bt->parent()));
	rlw=(rawfile_loader_window *)dw->user_data();

	if(!fileselector("Save config file",(char*)file,"floppy_profile.fpf","*.fpf",1,0))
	{
		getWindowState(rlw,&rfc);

		fpf_file=fopen(file,"wb");
		if(fpf_file)
		{
			fprintf(fpf_file,"FPF_V0.1");
			fwrite(&rfc,sizeof(cfgrawfile),1,fpf_file);
			fclose(fpf_file);
		}
	}
}

void raw_loader_window_bt_loadcfg(Fl_Button* bt, void*)
{
	FILE * fpf_file;
	char file[1024];
	cfgrawfile rfc;
	rawfile_loader_window *rlw;
	Fl_Window *dw;
	char header[8];

	dw=((Fl_Window*)(bt->parent()));
	rlw=(rawfile_loader_window *)dw->user_data();

	if(!fileselector("Load config file",(char*)file,"*.fpf","*.fpf",0,0))
	{
		fpf_file=fopen(file,"rb");
		if(fpf_file)
		{
			fread(header,sizeof(header),1,fpf_file);
			if( !strncmp(header,"FPF_V0.1",sizeof(header)) )
			{
				fread(&rfc,sizeof(cfgrawfile),1,fpf_file);
				setWindowState(rlw,&rfc);
			}
			fclose(fpf_file);
		}
	}
}
