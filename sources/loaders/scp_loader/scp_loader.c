/*
//
// Copyright (C) 2006 - 2013 Jean-François DEL NERO
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
// File : scp_loader.c
// Contains: SCP Stream floppy image loader
//
// Written by: DEL NERO Jean Francois
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////


#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

#include <math.h>

#include "types.h"
#include "libhxcfe.h"

#include "floppy_loader.h"
#include "floppy_utils.h"

#include "scp_loader.h"
#include "scp_writer.h"
#include "scp_format.h"

#include "libhxcadaptor.h"

int SCP_libIsValidDiskFile(HXCFLOPPYEMULATOR* floppycontext,char * imgfile)
{
	scp_header scph;
	FILE *f;

	floppycontext->hxc_printf(MSG_DEBUG,"SCP_libIsValidDiskFile");

	if( hxc_checkfileext(imgfile,"scp") )
	{
		f=hxc_fopen(imgfile,"rb");
		if(f==NULL)
		{
			floppycontext->hxc_printf(MSG_ERROR,"SCP_libIsValidDiskFile : Cannot open %s !",imgfile);
			return HXCFE_ACCESSERROR;
		}

		fread(&scph, sizeof(scp_header), 1, f);

		if( strncmp(scph.sign,"SCP",3) )
		{
			floppycontext->hxc_printf(MSG_ERROR,"SCP_libIsValidDiskFile : Bad file header !");
			fclose(f);
			return HXCFE_BADFILE;
		}

		floppycontext->hxc_printf(MSG_DEBUG,"SCP_libIsValidDiskFile : SCP file !");
		return HXCFE_VALIDFILE;
	}
	else
	{
		floppycontext->hxc_printf(MSG_DEBUG,"SCP_libIsValidDiskFile : non SCP file !");
		return HXCFE_BADFILE;
	}

	return HXCFE_BADPARAMETER;
}

int SCP_libLoad_DiskFile(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	return HXCFE_BADFILE;
}

int SCP_libGetPluginInfo(HXCFLOPPYEMULATOR* floppycontext,unsigned long infotype,void * returnvalue)
{
	static const char plug_id[]="SCP_FLUX_STREAM";
	static const char plug_desc[]="SCP Stream Loader";
	static const char plug_ext[]="scp";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)	SCP_libIsValidDiskFile,
		(LOADDISKFILE)		SCP_libLoad_DiskFile,
		(WRITEDISKFILE)		SCP_libWrite_DiskFile,
		(GETPLUGININFOS)	SCP_libGetPluginInfo
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
