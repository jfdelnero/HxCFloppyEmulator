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
// File : adl_ssd_dsd_loader.c
// Contains: BBC floppy image loader
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
#include "libhxcadaptor.h"
#include "floppy_loader.h"
#include "tracks/track_generator.h"

#include "loaders/common/raw_iso.h"

#include "adl_loader.h"

int ADL_libIsValidDiskFile( HXCFE_IMGLDR * imgldr_ctx, HXCFE_IMGLDR_FILEINFOS * imgfile )
{
	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"ADL_libIsValidDiskFile");

	if( hxc_checkfileext(imgfile->path,"adl",SYS_PATH_TYPE) || hxc_checkfileext(imgfile->path,"adm",SYS_PATH_TYPE) || hxc_checkfileext(imgfile->path,"adf",SYS_PATH_TYPE) )
	{
		if(imgfile->file_size&0x1FF || (imgfile->file_size > 82*16*2*256 ) )
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"ADL_libIsValidDiskFile : non Acorn BBC ADL IMG file - bad file size !");
			return HXCFE_BADFILE;
		}

		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"ADL_libIsValidDiskFile : Acorn BBC ADL file !");
		return HXCFE_VALIDFILE;
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"ADL_libIsValidDiskFile : non Acorn BBC ADL file !");
		return HXCFE_BADFILE;
	}
}

int ADL_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	FILE * f_img;
	raw_iso_cfg rawcfg;
	int ret;
	unsigned int filesize;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"ADL_libLoad_DiskFile %s",imgfile);

	f_img = hxc_fopen(imgfile,"rb");
	if( f_img == NULL )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	filesize = hxc_fgetsize(f_img);

	if( filesize )
	{
		raw_iso_setdefcfg(&rawcfg);

		rawcfg.sector_size = 256;

		rawcfg.number_of_tracks = 80;
		rawcfg.number_of_sides = 2;
		rawcfg.number_of_sectors_per_track = 16;
		rawcfg.gap3 = 255;
		rawcfg.interleave = 1;
		rawcfg.skew_per_track = 7;
		rawcfg.rpm = 300; // normal rpm

		switch( filesize )
		{
			case 80*16*2*256:
				rawcfg.number_of_tracks = 80;
				rawcfg.number_of_sides = 2;
				rawcfg.number_of_sectors_per_track = 16;
			break;
			case 80*16*1*256:
				rawcfg.number_of_tracks = 80;
				rawcfg.number_of_sides = 1;
				rawcfg.number_of_sectors_per_track = 16;
			break;
			case 40*16*1*256:
				rawcfg.number_of_tracks = 40;
				rawcfg.number_of_sides = 1;
				rawcfg.number_of_sectors_per_track = 16;
			break;
			default:
				imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Invalid file size : %d bytes",filesize);
				hxc_fclose(f_img);
				return HXCFE_FILECORRUPTED;
			break;
		}

		rawcfg.interface_mode = GENERIC_SHUGART_DD_FLOPPYMODE;
		rawcfg.bitrate = DEFAULT_DD_BITRATE;
		rawcfg.track_format = IBMFORMAT_DD;

		ret = raw_iso_loader(imgldr_ctx, floppydisk, f_img, 0, 0, &rawcfg);

		hxc_fclose(f_img);

		return ret;
	}

	hxc_fclose(f_img);
	return HXCFE_FILECORRUPTED;
}

int ADL_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]="BBC_ADL";
	static const char plug_desc[]="BBC ADL floppy image loader";
	static const char plug_ext[]="adl";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   ADL_libIsValidDiskFile,
		(LOADDISKFILE)      ADL_libLoad_DiskFile,
		(WRITEDISKFILE)     0,
		(GETPLUGININFOS)    ADL_libGetPluginInfo
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
