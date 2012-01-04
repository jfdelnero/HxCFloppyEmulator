/*
//
// Copyright (C) 2006, 2007, 2008, 2009, 2010, 2011 Jean-François DEL NERO
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
//-----------H----H--X----X-----CCCéCC----22222----0000-----0000------11----------//
//----------H----H----X-X-----C--------------2---0----0---0----0--1--1-----------//
//---------HHHHHH-----X------C----------22222---0----0---0----0-----1------------//
//--------H----H----X--X----C----------2-------0----0---0----0-----1-------------//
//-------H----H---X-----X---CCCCC-----222222----0000-----0000----1111------------//
//-------------------------------------------------------------------------------//
//----------------------------------------------------- http://hxc2001.free.fr --//
///////////////////////////////////////////////////////////////////////////////////
// File : loader.c
// Contains: Floppy Emulator Project
//
// Written by:	DEL NERO Jean Francois
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <string.h>
#include <stdlib.h>



#include "mainrouts.h"

extern "C"
{
	#include "hxc_floppy_emulator.h"
	#include "./usb_floppyemulator/usb_hxcfloppyemulator.h"
//	#include "../../common/plugins/raw_loader/raw_loader.h"
}

#include "loader.h"

extern HXCFLOPPYEMULATOR * flopemu;
extern FLOPPY * thefloppydisk;
extern guicontext * gui_context;
extern HWINTERFACE * hwif;

extern track_type track_type_list[];

int loadfloppy(char *filename)
{
	int ret;
	int i;
	int oldifmode;

	hxcfe_floppyUnload(flopemu,thefloppydisk);
	thefloppydisk=0;

	hxcfe_selectContainer(flopemu,"AUTOSELECT");
		
	thefloppydisk=hxcfe_floppyLoad(flopemu,filename,&ret);
		
	gui_context->loadstatus=ret;

	if(ret!=HXCFE_NOERROR || !thefloppydisk)
	{
		thefloppydisk=0;
	}
	else
	{	
		oldifmode=hwif->interface_mode;
		InjectFloppyImg(flopemu,thefloppydisk,hwif);
		if(!gui_context->autoselectmode)
		{	// keep the old interface mode
			hwif->interface_mode=oldifmode;
		};
			
		if(filename)
		{
			i=strlen(filename);
			while(i!=0 && filename[i]!='\\')
			{
				i--;
			}
			if(filename[i]=='\\') i++;
			sprintf(gui_context->bufferfilename,"%s",&filename[i]);
		}
		else
		{
			sprintf(gui_context->bufferfilename,"Empty Floppy");
		}
	}	

	return ret;
}

int loadrawfile(HXCFLOPPYEMULATOR* floppycontext,cfgrawfile * rfc)
{
	FBuilder* fb;
	unsigned int i,j,k,nbside;
	int ret;
	int oldifmode;


	if(thefloppydisk)
		hxcfe_floppyUnload(flopemu,thefloppydisk);

	nbside=(rfc->sidecfg)&2?2:1;

	fb=hxcfe_initFloppy(floppycontext,rfc->numberoftrack,nbside);

	hxcfe_setTrackInterleave(fb,rfc->interleave);	
	hxcfe_setSectorFill(fb,rfc->fillvalue);

	if(rfc->autogap3)
		hxcfe_setSectorGap3(fb,255);
	else
		hxcfe_setSectorGap3(fb,(unsigned char)rfc->gap3);

	hxcfe_setSectorGap3(fb,(unsigned char)rfc->gap3);
	hxcfe_setTrackBitrate(fb,rfc->bitrate);

	for(i=0;i<rfc->numberoftrack;i++)
	{
		for(j=0;j<nbside;j++)
		{
			// prepare a new track.
			hxcfe_pushTrack(fb,rfc->rpm,i,j,track_type_list[rfc->tracktype].tracktype);
			
			// Set the skew
			if(rfc->sideskew)
			{
				hxcfe_setTrackSkew(fb,(unsigned char)(((i<<1)|(j&1))*rfc->skew) );
			}
			else
			{
				hxcfe_setTrackSkew(fb,(unsigned char)i*rfc->skew);
			}

			// add all sectors
			for(k=0;k<rfc->sectorpertrack;k++)
			{
				hxcfe_addSector(fb,rfc->firstidsector+k,j,i,0,128<<rfc->sectorsize);
			}

			// generate the track
			hxcfe_popTrack(fb);
		}
	}

	thefloppydisk=hxcfe_getFloppy(fb);
	/*t=hxcfe_getfloppysize(floppycontext,thefloppydisk,0);

	test=malloc(512*18);

	ss=hxcfe_init_sectorsearch(floppycontext,thefloppydisk);
	t=hxcfe_readsectordata(ss,0,0,1,5,512,test);

	hxcfe_deinit_sectorsearch(ss);*/


	if(thefloppydisk)
		ret=HXCFE_NOERROR;

	gui_context->loadstatus=ret;

	oldifmode=hwif->interface_mode;
	InjectFloppyImg(flopemu,thefloppydisk,hwif);
	if(!gui_context->autoselectmode)
	{	// keep the old interface mode
		hwif->interface_mode=oldifmode;
	};

	sprintf(gui_context->bufferfilename,"Empty Floppy");
	


	return ret;
}

