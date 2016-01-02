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
// File : cb_rawfile_loader_window.cxx
// Contains: RAW image loader window
//
// Written by:	DEL NERO Jean Francois
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include "fl_includes.h"

#include "rawfile_loader_window.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "fileselector.h"

extern "C"
{
	#include "libhxcfe.h"
	#include "usb_hxcfloppyemulator.h"
	#include "libhxcadaptor.h"
}

#include "loader.h"
#include "main.h"

extern s_gui_context * guicontext;



void getWindowState(rawfile_loader_window *rlw,cfgrawfile *rfc)
{
	memset(rfc,0,sizeof(cfgrawfile));
	
	if(rlw->chk_autogap3->value())
		rfc->autogap3=0xff;
	else
		rfc->autogap3=0x00;

	if(rlw->choice_numberofside->value())
		rfc->sidecfg=2;
	else
		rfc->sidecfg=1;

	if(rlw->choice_numberofside->value())
		rfc->sidecfg=rfc->sidecfg|TWOSIDESFLOPPY;

	if(rlw->chk_reversesides->value())
		rfc->sidecfg=rfc->sidecfg|SIDE_INVERTED;

	if(rlw->chk_side0track_first->value())
		rfc->sidecfg=rfc->sidecfg|SIDE0_FIRST;

	if(rlw->chk_sidebasedskew->value())
		rfc->sideskew=0xFF;
	else
		rfc->sideskew=0x00;

	if(rlw->chk_intersidesectornum->value())
		rfc->intersidesectornumbering=0xff;
	else
		rfc->intersidesectornumbering=0x00;

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
	rfc->pregap=(unsigned long)rlw->numin_pregap->value();
}

void setWindowState(rawfile_loader_window *rlw,cfgrawfile *rfc)
{	
	if(rfc->autogap3)
		rlw->chk_autogap3->value(1);
	else
		rlw->chk_autogap3->value(0);

	if(rfc->sidecfg&TWOSIDESFLOPPY)
		rlw->choice_numberofside->value(1);
	else
		rlw->choice_numberofside->value(0);

	if(rfc->sidecfg&SIDE_INVERTED)
		rlw->chk_reversesides->value(1);
	else
		rlw->chk_reversesides->value(0);

	if(rfc->sidecfg&SIDE0_FIRST)
		rlw->chk_side0track_first->value(1);
	else
		rlw->chk_side0track_first->value(0);

	if(rfc->sideskew)
		rlw->chk_sidebasedskew->value(1);
	else
		rlw->chk_sidebasedskew->value(0);

	if(rfc->intersidesectornumbering)
		rlw->chk_intersidesectornum->value(1);
	else
		rlw->chk_intersidesectornum->value(0);

	rlw->innum_bitrate->value(rfc->bitrate);
	rlw->numin_formatvalue->value(rfc->fillvalue);
	rlw->innum_nbtrack->value(rfc->numberoftrack);
	rlw->numin_gap3->value(rfc->gap3);
	rlw->innum_sectoridstart->value(rfc->firstidsector);
	rlw->numin_interleave->value(rfc->interleave);
	rlw->innum_sectorpertrack->value(rfc->sectorpertrack);
	rlw->innum_rpm->value(rfc->rpm);
	rlw->choice_sectorsize->value(rfc->sectorsize);
	rlw->choice_tracktype->value(rfc->tracktype);
	rlw->numin_skew->value(rfc->skew);
	rlw->numin_pregap->value(rfc->pregap);
	
}

void raw_loader_window_datachanged(Fl_Widget* w, void*)
{
	int totalsector,totalsize;
	int temp[256],v;
	char file[512];
	rawfile_loader_window *rlw;
	Fl_Window *dw;
	HXCFE_XMLLDR* rfb;
	int xmlload;

	dw = ((Fl_Window*)(w->parent()));
	rlw = (rawfile_loader_window *)dw->user_data();

	totalsector = (int)(rlw->innum_nbtrack->value() * rlw->innum_sectorpertrack->value());

	if(rlw->choice_numberofside->value())
		totalsector=totalsector*2;

	sprintf((char*)temp,"%d",totalsector);

	rlw->strout_totalsector->value((const char*)temp);
	totalsize = totalsector * (128<<rlw->choice_sectorsize->value());
	sprintf((char*)temp,"%d",totalsize);
	rlw->strout_totalsize->value((const char*)temp);

	if(!rlw->chk_autogap3->value())
	{
		rlw->numin_gap3->activate();
	}
	else
	{
		rlw->numin_gap3->deactivate();
	}

	xmlload = -1;

	v = rlw->choice_disklayout->value(); 
	if(v > 0)
	{
		if(v==1)
		{
			if(rlw->choice_disklayout->changed())
			{
				xmlload = 1;
				if(!fileselector((char*)"Load XML file",(char*)file,(char*)"*.xml",(char*)"*.xml",0,0))
				{
					rfb=hxcfe_initXmlFloppy(guicontext->hxcfe);
					if(rfb)
					{
						if(hxcfe_setXmlFloppyLayoutFile(rfb,file) == HXCFE_NOERROR)
						{
							xmlload = 0;
							strcpy(guicontext->xml_file_path,file);
						}
						else
						{
							fl_alert("XML file verification failed !");
						}

						hxcfe_deinitXmlFloppy(rfb);
					}
				}
				
				if(xmlload>0)
				{
					memset(guicontext->xml_file_path,0,sizeof(guicontext->xml_file_path));

					rlw->choice_disklayout->value(0);

					rlw->chk_autogap3->activate();
					rlw->chk_intersidesectornum->activate();
					rlw->chk_reversesides->activate();
					rlw->chk_side0track_first->activate();
					rlw->chk_sidebasedskew->activate();
					rlw->choice_numberofside->activate();
					rlw->choice_sectorsize->activate();
					rlw->choice_tracktype->activate();
					rlw->innum_bitrate->activate();
					rlw->innum_nbtrack->activate();
					rlw->innum_rpm->activate();
					rlw->innum_sectoridstart->activate();
					rlw->innum_sectorpertrack->activate();
					rlw->numin_formatvalue->activate();
					rlw->numin_gap3->activate();
					rlw->numin_interleave->activate();
					rlw->numin_pregap->activate();
					rlw->numin_skew->activate();
					rlw->strout_totalsector->activate();
					rlw->strout_totalsize->activate();

					if(!rlw->chk_autogap3->value())
					{
						rlw->numin_gap3->activate();
					}
					else
					{
						rlw->numin_gap3->deactivate();
					}

					return;
				}
			}
		}

		rlw->chk_autogap3->deactivate();
		rlw->chk_intersidesectornum->deactivate();
		rlw->chk_reversesides->deactivate();
		rlw->chk_side0track_first->deactivate();
		rlw->chk_sidebasedskew->deactivate();
		rlw->choice_numberofside->deactivate();
		rlw->choice_sectorsize->deactivate();
		rlw->choice_tracktype->deactivate();
		rlw->innum_bitrate->deactivate();
		rlw->innum_nbtrack->deactivate();
		rlw->innum_rpm->deactivate();
		rlw->innum_sectoridstart->deactivate();
		rlw->innum_sectorpertrack->deactivate();
		rlw->numin_formatvalue->deactivate();
		rlw->numin_gap3->deactivate();
		rlw->numin_interleave->deactivate();
		rlw->numin_pregap->deactivate();
		rlw->numin_skew->deactivate();
		rlw->strout_totalsector->deactivate();
		rlw->strout_totalsize->deactivate();
	}
	else
	{
		rlw->chk_autogap3->activate();
		rlw->chk_intersidesectornum->activate();
		rlw->chk_reversesides->activate();
		rlw->chk_side0track_first->activate();
		rlw->chk_sidebasedskew->activate();
		rlw->choice_numberofside->activate();
		rlw->choice_sectorsize->activate();
		rlw->choice_tracktype->activate();
		rlw->innum_bitrate->activate();
		rlw->innum_nbtrack->activate();
		rlw->innum_rpm->activate();
		rlw->innum_sectoridstart->activate();
		rlw->innum_sectorpertrack->activate();
		rlw->numin_formatvalue->activate();
		rlw->numin_gap3->activate();
		rlw->numin_interleave->activate();
		rlw->numin_pregap->activate();
		rlw->numin_skew->activate();
		rlw->strout_totalsector->activate();
		rlw->strout_totalsize->activate();

		if(!rlw->chk_autogap3->value())
		{
			rlw->numin_gap3->activate();
		}
		else
		{
			rlw->numin_gap3->deactivate();
		}

	}
}

void raw_loader_window_bt_loadrawfile(Fl_Button* bt, void*)
{
	int totalsector,totalsize;
	int temp[256];
	char file[1024];
	int disklayout;
	rawfile_loader_window *rlw;
	Fl_Window *dw;
	cfgrawfile rfc;
	HXCFE_XMLLDR* rfb;

	memset(file,0,sizeof(file));

	dw=((Fl_Window*)(bt->parent()));
	rlw=(rawfile_loader_window *)dw->user_data();

	getWindowState(rlw,&rfc);

	totalsector=(int)(rlw->innum_nbtrack->value() * 	rlw->innum_sectorpertrack->value());
	if(rlw->choice_numberofside->value())
		totalsector=totalsector*2;

	sprintf((char*)temp,"%d",totalsector);

	rlw->strout_totalsector->value((const char*)temp);
	totalsize=totalsector * (128<<rlw->choice_sectorsize->value());
	sprintf((char*)temp,"%d",totalsize);
	rlw->strout_totalsize->value((const char*)temp);

	if(!fileselector((char*)"Select raw file",(char*)file,0,(char*)"*.img",0,0))
	{	
		disklayout = rlw->choice_disklayout->value();
		if(disklayout>=1)
		{
			rfb=hxcfe_initXmlFloppy(guicontext->hxcfe);

			if(disklayout ==  1)
				hxcfe_setXmlFloppyLayoutFile(rfb,guicontext->xml_file_path);
			else
				hxcfe_selectXmlFloppyLayout(rfb,disklayout-2);
			load_floppy( hxcfe_generateXmlFileFloppy(rfb,(char*)file),(char*)"Raw Image");
			hxcfe_deinitXmlFloppy(rfb);
		}
		else
		{
			loadrawfile(guicontext->hxcfe,&rfc,file);
		}
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
	int disklayout;
	HXCFE_XMLLDR* rfb;

	dw=((Fl_Window*)(bt->parent()));
	rlw=(rawfile_loader_window *)dw->user_data();


	totalsector=(int)(rlw->innum_nbtrack->value() * 	rlw->innum_sectorpertrack->value());
	if(rlw->choice_numberofside->value())
		totalsector=totalsector*2;

	sprintf((char*)temp,"%d",totalsector);

	rlw->strout_totalsector->value((const char*)temp);
	totalsize=totalsector * (128<<rlw->choice_sectorsize->value());
	sprintf((char*)temp,"%d",totalsize);
	rlw->strout_totalsize->value((const char*)temp);

	getWindowState(rlw,&rfc);

	disklayout = rlw->choice_disklayout->value();
	if(disklayout>=1)
	{
		rfb=hxcfe_initXmlFloppy(guicontext->hxcfe);
		if(disklayout ==  1)
			hxcfe_setXmlFloppyLayoutFile(rfb,guicontext->xml_file_path);
		else
			hxcfe_selectXmlFloppyLayout(rfb,disklayout-2);
		load_floppy( hxcfe_generateXmlFloppy(rfb,0,0),(char*)"Raw Image" );
		hxcfe_deinitXmlFloppy(rfb);
	}
	else
	{
		loadrawfile(guicontext->hxcfe,&rfc,NULL);
	}

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

	if(!fileselector((char*)"Save config file",(char*)file,(char*)"floppy_profile.fpf",(char*)"*.fpf",1,0))
	{
		getWindowState(rlw,&rfc);

		fpf_file=hxc_fopen(file,"wb");
		if(fpf_file)
		{
			fprintf(fpf_file,"FPF_V0.1");
			fwrite(&rfc,sizeof(cfgrawfile),1,fpf_file);
			hxc_fclose(fpf_file);
			raw_loader_window_datachanged(bt, 0);
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

	if(!fileselector((char*)"Load config file",(char*)file,(char*)"*.fpf",(char*)"*.fpf",0,0))
	{
		fpf_file=hxc_fopen(file,"rb");
		if(fpf_file)
		{
			if(fread(header,sizeof(header),1,fpf_file))
			{
				if( !strncmp(header,"FPF_V0.1",sizeof(header)) )
				{
					memset(&rfc,0,sizeof(cfgrawfile));
					if(fread(&rfc,sizeof(cfgrawfile),1,fpf_file))
					{
						setWindowState(rlw,&rfc);
						raw_loader_window_datachanged(bt, 0);
					}
				}
			}

			hxc_fclose(fpf_file);
		}
	}
}
