/*
//
// Copyright (C) 2006 - 2012 Jean-François DEL NERO
//
// This file is part of the HxCFloppyEmulator library
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
// File : cpcfs_loader.c
// Contains: CPCFSDK floppy image loader
//
// Written by:	DEL NERO Jean Francois
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>

#include "libhxcfe.h"

#include "floppy_loader.h"
#include "floppy_utils.h"

#include "cpcfs_loader.h"

#include "libhxcadaptor.h"

HXCFLOPPYEMULATOR* global_floppycontext;
extern int ScanFile(HXCFLOPPYEMULATOR* floppycontext,struct Volume * adfvolume,char * folder,char * file);


int CPCFSDK_libIsValidDiskFile(HXCFLOPPYEMULATOR* floppycontext,char * imgfile)
{
	
	int pathlen;
	char * filepath;
    struct stat staterep;

	floppycontext->hxc_printf(MSG_DEBUG,"CPCFSDK_libIsValidDiskFile %s",imgfile);
	if(imgfile)
	{
		pathlen=strlen(imgfile);
		if(pathlen!=0)
		{		
			memset(&staterep,0,sizeof(struct stat));
			hxc_stat(imgfile,&staterep);

			if(staterep.st_mode&S_IFDIR)
			{
				filepath=malloc(pathlen+1);
				if(filepath!=0)
				{
					sprintf(filepath,"%s",imgfile);
					strlower(filepath);

					if(strstr( filepath,".cpcfs" )!=NULL)
					{
						floppycontext->hxc_printf(MSG_DEBUG,"CPCFSDK file !");
						free(filepath);
						return HXCFE_VALIDFILE;
					}
					else
					{
						floppycontext->hxc_printf(MSG_DEBUG,"non CPCFSDK file ! (.cpcfs missing)");
						free(filepath);
						return HXCFE_BADFILE;
					}
				}
			}
			else
			{
				floppycontext->hxc_printf(MSG_DEBUG,"non CPCFSDK file ! (it's not a directory)");
				return HXCFE_BADFILE;
			}
		}
		floppycontext->hxc_printf(MSG_DEBUG,"0 byte string ?");
	}
	return HXCFE_BADPARAMETER;


}


int ScanCpcFile(HXCFLOPPYEMULATOR* floppycontext,struct Volume * adfvolume,char * folder,char * file)
{

	return 0;
}

int CPCFSDK_libLoad_DiskFile(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * floppydisk,char * imgfile,void * parameters)
{
		return HXCFE_BADFILE;
}


int CPCFSDK_libGetPluginInfo(HXCFLOPPYEMULATOR* floppycontext,unsigned long infotype,void * returnvalue)
{

	static const char plug_id[]="CPC_FS";
	static const char plug_desc[]="Amstrad CPC FS Loader";
	static const char plug_ext[]="cpcfs";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)	CPCFSDK_libIsValidDiskFile,
		(LOADDISKFILE)		CPCFSDK_libLoad_DiskFile,
		(WRITEDISKFILE)		0,
		(GETPLUGININFOS)	CPCFSDK_libGetPluginInfo
	};

	return libGetPluginInfo(
			floppycontext,
			infotype,
			returnvalue,
			plug_id,
			plug_desc,
			&plug_funcs,
			plug_ext
			);
}
