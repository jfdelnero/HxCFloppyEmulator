/*
//
// Copyright (C) 2006-2024 Jean-François DEL NERO
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
// File : adz_loader.c
// Contains: ADZ floppy image loader
//
// Written by: Jean-François DEL NERO
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "types.h"

#include "internal_libhxcfe.h"
#include "libhxcfe.h"
#include "./tracks/track_generator.h"

#include "floppy_loader.h"
#include "floppy_utils.h"

#include "adz_loader.h"
#include "adz_writer.h"

#include "loaders/common/raw_amiga.h"

#include "thirdpartylibs/zlib/zlib.h"

#include "libhxcadaptor.h"

#define UNPACKBUFFER 128*1024

int ADZ_libIsValidDiskFile( HXCFE_IMGLDR * imgldr_ctx, HXCFE_IMGLDR_FILEINFOS * imgfile )
{
	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"ADZ_libIsValidDiskFile");

	if(hxc_checkfileext(imgfile->path,"adz",SYS_PATH_TYPE) || hxc_checkfileext(imgfile->path,"gz",SYS_PATH_TYPE))
	{
		if(hxc_checkfileext(imgfile->path,"gz",SYS_PATH_TYPE))
		{
			if( !strstr(hxc_getfilenamebase(imgfile->path,0,SYS_PATH_TYPE),".adf.gz") )
			{
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"ADZ_libIsValidDiskFile : non ADZ file !");
				return HXCFE_BADFILE;
			}
		}

		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"ADZ_libIsValidDiskFile : ADZ file !");
		return HXCFE_VALIDFILE;
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"ADZ_libIsValidDiskFile : non ADZ file !");
		return HXCFE_BADFILE;
	}
}

int ADZ_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	int ret;
	int i;
	unsigned int filesize;
	unsigned char *flatimg, *tmp_ptr;
	gzFile file;
	int err;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"ADZ_libLoad_DiskFile %s",imgfile);

	file = gzopen(imgfile, "rb");
	if (!file)
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"gzopen: Error while reading the file!");
		return HXCFE_ACCESSERROR;
	}

	i = 0;
	filesize = 0;

	flatimg = (unsigned char*)malloc(UNPACKBUFFER);
	if( !flatimg )
		goto error;

	do
	{
		err = gzread(file, flatimg+filesize,UNPACKBUFFER );
		filesize += err;
		if(err>0)
		{
			tmp_ptr = (unsigned char *)realloc(flatimg,filesize+UNPACKBUFFER);
			if(!tmp_ptr)
				goto error;

			flatimg = tmp_ptr;
		}
		else
		{
			if(!gzeof(file))
			{
				goto error;
			}
		}
		i++;
	}while(err>0);

	gzclose(file);

	ret = raw_amiga_loader(imgldr_ctx, floppydisk, 0, flatimg, filesize );

	free(flatimg);

	return ret;

error:
	free(flatimg);

	if( file )
		gzclose(file);

	return HXCFE_BADFILE;
}

int ADZ_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]="AMIGA_ADZ";
	static const char plug_desc[]="AMIGA ADZ Loader";
	static const char plug_ext[]="adz";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   ADZ_libIsValidDiskFile,
		(LOADDISKFILE)      ADZ_libLoad_DiskFile,
		(WRITEDISKFILE)     ADZ_libWrite_DiskFile,
		(GETPLUGININFOS)    ADZ_libGetPluginInfo
	};

	return libGetPluginInfo(
			imgldr_ctx,
			infotype,
			returnvalue,
			plug_id,
			plug_desc,
			&plug_funcs,
			plug_ext
			);
}
