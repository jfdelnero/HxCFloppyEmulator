/*
//
// Copyright (C) 2006-2018 Jean-François DEL NERO
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
// Written by:	DEL NERO Jean Francois
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

#include "thirdpartylibs/zlib/zlib.h"

#include "libhxcadaptor.h"

#define UNPACKBUFFER 128*1024


int ADZ_libIsValidDiskFile(HXCFE_IMGLDR * imgldr_ctx,char * imgfile)
{
	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"ADZ_libIsValidDiskFile");

	if(hxc_checkfileext(imgfile,"adz") || hxc_checkfileext(imgfile,"gz"))
	{
		if(hxc_checkfileext(imgfile,"gz"))
		{
			if( !strstr(hxc_getfilenamebase(imgfile,0),".adf.gz") )
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
	HXCFE_FLPGEN * fb_ctx;
	int ret;
	int i;
	unsigned int filesize;
	unsigned char* flatimg;
	gzFile file;
	int err;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"ADZ_libLoad_DiskFile %s",imgfile);

	file = gzopen(imgfile, "rb");
	if (!file)
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"gzopen: Error while reading the file!");
		return HXCFE_ACCESSERROR;
	}

	i=0;
	filesize=0;
	flatimg=(unsigned char*)malloc(UNPACKBUFFER);
	do
	{
		if(!flatimg)
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Unpack error!");
			gzclose(file);
			return HXCFE_BADFILE;
		}

		err = gzread(file, flatimg+filesize,UNPACKBUFFER );
		filesize += err;
		if(err>0)
		{
			flatimg = (unsigned char *)realloc(flatimg,filesize+UNPACKBUFFER);
		}
		i++;
	}while(err>0);

	gzclose(file);

	if(!flatimg)
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Alloc error!");
		return HXCFE_INTERNALERROR;
	}

	fb_ctx = hxcfe_initFloppy( imgldr_ctx->hxcfe, 86, 2 );
	if( !fb_ctx )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Alloc Error !");
		free(flatimg);
		return HXCFE_INTERNALERROR;
	}

	if(filesize<100*11*2*512)
	{
		hxcfe_setNumberOfSector ( fb_ctx, 11 );
		hxcfe_setRPM( fb_ctx, DEFAULT_AMIGA_RPM ); // normal rpm
		hxcfe_setInterfaceMode( fb_ctx, AMIGA_DD_FLOPPYMODE);
	}
	else
	{
		hxcfe_setNumberOfSector ( fb_ctx, 22 );
		hxcfe_setRPM( fb_ctx, DEFAULT_AMIGA_RPM / 2); // 150 rpm
		hxcfe_setInterfaceMode( fb_ctx, AMIGA_HD_FLOPPYMODE);
	}

	if(( filesize / ( 512 * hxcfe_getCurrentNumberOfTrack(fb_ctx) * hxcfe_getCurrentNumberOfSide(fb_ctx)))<80)
		hxcfe_setNumberOfTrack ( fb_ctx, 80 );
	else
		hxcfe_setNumberOfTrack ( fb_ctx, filesize/(512* hxcfe_getCurrentNumberOfTrack(fb_ctx) * hxcfe_getCurrentNumberOfSide(fb_ctx) ) );

	hxcfe_setNumberOfSide ( fb_ctx, 2 );
	hxcfe_setSectorSize( fb_ctx, 512 );

	hxcfe_setTrackType( fb_ctx, AMIGAFORMAT_DD);
	hxcfe_setTrackBitrate( fb_ctx, DEFAULT_AMIGA_BITRATE );

	hxcfe_setStartSectorID( fb_ctx, 0 );
	hxcfe_setSectorGap3 ( fb_ctx, 0 );

	ret = hxcfe_generateDisk( fb_ctx, floppydisk, 0, flatimg, filesize );

	free(flatimg);

	return ret;
}

int ADZ_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{

	static const char plug_id[]="AMIGA_ADZ";
	static const char plug_desc[]="AMIGA ADZ Loader";
	static const char plug_ext[]="adz";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)	ADZ_libIsValidDiskFile,
		(LOADDISKFILE)		ADZ_libLoad_DiskFile,
		(WRITEDISKFILE)		ADZ_libWrite_DiskFile,
		(GETPLUGININFOS)	ADZ_libGetPluginInfo
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
