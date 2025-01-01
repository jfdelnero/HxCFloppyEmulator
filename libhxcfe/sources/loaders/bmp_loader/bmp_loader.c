/*
//
// Copyright (C) 2006-2025 Jean-Fran�ois DEL NERO
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
// File : bmp_loader.c
// Contains: BMP / PNG floppy tracks layout image generator
//
// Written by: Jean-Fran�ois DEL NERO
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "types.h"

#include "internal_libhxcfe.h"
#include "libhxcfe.h"

#include "floppy_loader.h"
#include "floppy_utils.h"

#include "bmp_loader.h"

int BMP_Tracks_libWrite_DiskFile(HXCFE_IMGLDR* imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * filename);
int PNG_Tracks_libWrite_DiskFile(HXCFE_IMGLDR* imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * filename);

int BMP_StreamTracks_libWrite_DiskFile(HXCFE_IMGLDR* imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * filename);
int PNG_StreamTracks_libWrite_DiskFile(HXCFE_IMGLDR* imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * filename);

int BMP_Disk_libWrite_DiskFile(HXCFE_IMGLDR* imgldr_ctx,HXCFE_FLOPPY * floppy,char * filename);
int PNG_Disk_libWrite_DiskFile(HXCFE_IMGLDR* imgldr_ctx,HXCFE_FLOPPY * floppy,char * filename);

int BMP_Tracks_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]="BMP_IMAGE";
	static const char plug_desc[]="BMP floppy tracks layout image generator";
	static const char plug_ext[]="bmp";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   0,
		(LOADDISKFILE)      0,
		(WRITEDISKFILE)     BMP_Tracks_libWrite_DiskFile,
		(GETPLUGININFOS)    BMP_Tracks_libGetPluginInfo
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

int PNG_Tracks_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]="PNG_IMAGE";
	static const char plug_desc[]="PNG floppy tracks layout image generator";
	static const char plug_ext[]="png";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   0,
		(LOADDISKFILE)      0,
		(WRITEDISKFILE)     PNG_Tracks_libWrite_DiskFile,
		(GETPLUGININFOS)    PNG_Tracks_libGetPluginInfo
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

int BMP_StreamTracks_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]="BMP_STREAM_IMAGE";
	static const char plug_desc[]="BMP stream floppy tracks layout image generator";
	static const char plug_ext[]="bmp";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   0,
		(LOADDISKFILE)      0,
		(WRITEDISKFILE)     BMP_StreamTracks_libWrite_DiskFile,
		(GETPLUGININFOS)    BMP_StreamTracks_libGetPluginInfo
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

int PNG_StreamTracks_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]="PNG_STREAM_IMAGE";
	static const char plug_desc[]="PNG stream floppy tracks layout image generator";
	static const char plug_ext[]="png";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   0,
		(LOADDISKFILE)      0,
		(WRITEDISKFILE)     PNG_StreamTracks_libWrite_DiskFile,
		(GETPLUGININFOS)    PNG_StreamTracks_libGetPluginInfo
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

int BMP_Disk_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]="BMP_DISK_IMAGE";
	static const char plug_desc[]="BMP floppy layout (disk) image generator";
	static const char plug_ext[]="bmp";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   0,
		(LOADDISKFILE)      0,
		(WRITEDISKFILE)     BMP_Disk_libWrite_DiskFile,
		(GETPLUGININFOS)    BMP_Disk_libGetPluginInfo
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

int PNG_Disk_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]="PNG_DISK_IMAGE";
	static const char plug_desc[]="PNG floppy layout (disk) image generator";
	static const char plug_ext[]="png";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   0,
		(LOADDISKFILE)      0,
		(WRITEDISKFILE)     PNG_Disk_libWrite_DiskFile,
		(GETPLUGININFOS)    PNG_Disk_libGetPluginInfo
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
