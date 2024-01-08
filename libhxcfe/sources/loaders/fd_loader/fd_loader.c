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
// File : fd_loader.c
// Contains: Thomson FD floppy image loader
//
// Written by: Jean-François DEL NERO
//
// Change History (most recent first):
// Jan. 2017 :  T. Missonier (sourcezax@users.sourceforge.net) : Correction for double sided fd files. Added fd_writer support
///////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "types.h"

#include "internal_libhxcfe.h"
#include "libhxcfe.h"
#include "libhxcadaptor.h"
#include "floppy_loader.h"
#include "tracks/track_generator.h"
#include "loaders/common/raw_iso.h"

#include "fd_loader.h"
#include "fd_writer.h"

int FD_libIsValidDiskFile( HXCFE_IMGLDR * imgldr_ctx, HXCFE_IMGLDR_FILEINFOS * imgfile )
{
	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"fd_libIsValidDiskFile");

	if( hxc_checkfileext(imgfile->path,"fd",SYS_PATH_TYPE) )
	{
		if((imgfile->file_size == 327680) || (imgfile->file_size == 655360))
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"FD_libIsValidDiskFile : TO8D FD file !");
			return HXCFE_VALIDFILE;
		}
		else
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"FD_libIsValidDiskFile : non TO8D FD file ! - bad file size! ");
			return HXCFE_BADFILE;
		}
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"FD_libIsValidDiskFile : non TO8D FD file !");
		return HXCFE_BADFILE;
	}
}

int FD_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	FILE * f_img;
	raw_iso_cfg rawcfg;
	int ret,filesize;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"fd_libLoad_DiskFile %s",imgfile);

	f_img = hxc_fopen(imgfile,"rb");
	if( f_img == NULL )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	raw_iso_setdefcfg(&rawcfg);

	rawcfg.sector_size = 256; // FD file support only 256bytes/sector floppies.
	rawcfg.bitrate = 250000;
	rawcfg.rpm = 300;
	rawcfg.interleave = 7;
	rawcfg.skew_per_track = 0;
	rawcfg.gap3 = 50;
	rawcfg.track_format = ISOFORMAT_DD;
	rawcfg.number_of_sectors_per_track = 16;
	rawcfg.number_of_tracks = 80;
	rawcfg.interface_mode = GENERIC_SHUGART_DD_FLOPPYMODE;

	rawcfg.trk_grouped_by_sides = 1;
	rawcfg.force_side_id = 0;

	filesize = hxc_fgetsize(f_img);

	switch(filesize)
	{
		case 327680:
			rawcfg.number_of_sides = 1;
		break;
		case 655360:
			rawcfg.number_of_sides = 2;
		break;
		default:
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"non TO8D FD file ! - bad file size! ");
			hxc_fclose(f_img);
			return HXCFE_BADFILE;
		break;
	}

	ret = raw_iso_loader(imgldr_ctx, floppydisk, f_img, 0, 0, &rawcfg);

	hxc_fclose(f_img);

	return ret;
}

int FD_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]="THOMSON_FD";
	static const char plug_desc[]="THOMSON FD Loader";
	static const char plug_ext[]="fd";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   FD_libIsValidDiskFile,
		(LOADDISKFILE)      FD_libLoad_DiskFile,
		(WRITEDISKFILE)     FD_libWrite_DiskFile,
		(GETPLUGININFOS)    FD_libGetPluginInfo
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
