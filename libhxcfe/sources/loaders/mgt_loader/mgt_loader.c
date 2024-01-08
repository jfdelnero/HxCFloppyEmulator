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
// File : mgt_loader.c
// Contains: MGT floppy image loader
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

#include "mgt_loader.h"

int MGT_libIsValidDiskFile( HXCFE_IMGLDR * imgldr_ctx, HXCFE_IMGLDR_FILEINFOS * imgfile )
{
	return hxcfe_imgCheckFileCompatibility( imgldr_ctx, imgfile, "MGT_libIsValidDiskFile", "sad,mgt", 512 );
}

int MGT_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	raw_iso_cfg rawcfg;
	int ret;
	FILE * f_img;
	unsigned int filesize;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"MGT_libLoad_DiskFile %s",imgfile);

	f_img = hxc_fopen(imgfile,"rb");
	if( f_img == NULL )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	raw_iso_setdefcfg(&rawcfg);

	rawcfg.bitrate = 250000;
	rawcfg.rpm = 300;
	rawcfg.interleave = 1;
	rawcfg.sector_size = 512;
	rawcfg.track_format = ISOFORMAT_DD;
	rawcfg.interface_mode = GENERIC_SHUGART_DD_FLOPPYMODE;

	filesize = hxc_fgetsize(f_img);

	switch( filesize )
	{
		case 819200:
			rawcfg.number_of_tracks = 80;
			rawcfg.number_of_sides = 2;
			rawcfg.number_of_sectors_per_track = 10;
			rawcfg.gap3 = 35;
			rawcfg.interleave = 1;
			rawcfg.skew_per_track = 0;
			rawcfg.skew_per_side = 0;
		break;

		default:
			hxc_fclose(f_img);
			imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Bad file size:  %d bytes !",filesize);
			return HXCFE_BADFILE;
		break;

	}

	ret = raw_iso_loader(imgldr_ctx, floppydisk, f_img, 0, 0, &rawcfg);

	hxc_fclose(f_img);

	return ret;
}

int MGT_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]="SAMCOUPE_MGT";
	static const char plug_desc[]="SAM COUPE MGT Loader";
	static const char plug_ext[]="mgt";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   MGT_libIsValidDiskFile,
		(LOADDISKFILE)      MGT_libLoad_DiskFile,
		(WRITEDISKFILE)     0,
		(GETPLUGININFOS)    MGT_libGetPluginInfo
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

