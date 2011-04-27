/*
//
// Copyright (C) 2006, 2007, 2008, 2009 Jean-François DEL NERO
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
// File : loader.c
// Contains: Floppy Emulator Project
//
// Written by:	DEL NERO Jean Francois
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "mainrouts.h"

#include "hxc_floppy_emulator.h"
#include "internal_floppy.h"
#include "floppy_loader.h"
#include "./usb_floppyemulator/usb_hxcfloppyemulator.h"

#include "../../common/plugins/raw_loader/raw_loader.h"


extern HXCFLOPPYEMULATOR * flopemu;
extern FLOPPY * thefloppydisk;
extern guicontext * demo;
extern HWINTERFACE * hwif;

int loadfloppy(char *filename,FLOPPY * floppydisk,cfgrawfile * cfgrawfile)
{
	int ret;
	int i;
	int oldifmode;

	if(!floppydisk)
	{
		if(thefloppydisk)
		{
	
			floppy_unload(flopemu,thefloppydisk);
		}
		else
		{
			thefloppydisk=(FLOPPY *)malloc(sizeof(FLOPPY));
			memset(thefloppydisk,0,sizeof(FLOPPY));

		}
		if(!cfgrawfile)
		ret=floppy_load(flopemu,thefloppydisk,filename);
		else
		ret=RAW_libLoad_DiskFile(flopemu,thefloppydisk,filename,cfgrawfile);

		demo->loadstatus=ret;

		if(ret!=LOADER_NOERROR)
		{
			free(thefloppydisk);
			thefloppydisk=0;

		}
		else
		{	
			oldifmode=hwif->interface_mode;
			InjectFloppyImg(flopemu,thefloppydisk,hwif);
			if(!demo->autoselectmode)
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
				sprintf(demo->bufferfilename,"%s",&filename[i]);
			}
			else
			{
				sprintf(demo->bufferfilename,"Empty Floppy");
			}
		}	

	}
	else
	{

		if(thefloppydisk)
		{
	
			floppy_unload(flopemu,thefloppydisk);
		}

		thefloppydisk=(FLOPPY *)floppydisk;
	

		demo->loadstatus=LOADER_NOERROR;

		oldifmode=hwif->interface_mode;
		InjectFloppyImg(flopemu,thefloppydisk,hwif);
		if(!demo->autoselectmode)
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
			sprintf(demo->bufferfilename,"%s",&filename[i]);
		}
		else
		{
				sprintf(demo->bufferfilename,"Empty Floppy");
		}
	}	

	
	return ret;

}
