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
// File : sdu_loader.c
// Contains: SAB Diskette Utility / SDU floppy image loader
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

#include "sdu_loader.h"
#include "sdu_format.h"

int SDU_libIsValidDiskFile( HXCFE_IMGLDR * imgldr_ctx, HXCFE_IMGLDR_FILEINFOS * imgfile )
{
	sdu_header * sduh;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"SDU_libIsValidDiskFile");

	if( hxc_checkfileext(imgfile->path,"sdu",SYS_PATH_TYPE) )
	{
		sduh = (sdu_header *)imgfile->file_header;

		if( !strncmp((const char*)&sduh->signature,"SAB ",4))
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"SDU_libIsValidDiskFile : SDU file !");
			return HXCFE_VALIDFILE;
		}
		else
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"SDU_libIsValidDiskFile : non SDU file !");
			return HXCFE_BADFILE;
		}
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"SDU_libIsValidDiskFile : non SDU file !");
		return HXCFE_BADFILE;
	}
}

int SDU_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	raw_iso_cfg rawcfg;
	int ret;
	FILE * f_img;
	sdu_header sduh;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"SDU_libLoad_DiskFile %s",imgfile);

	f_img = hxc_fopen(imgfile,"rb");
	if( f_img == NULL )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	memset(&sduh, 0, sizeof(sduh));

	hxc_fread(&sduh,sizeof(sduh),f_img);

	if( strncmp((const char*)&sduh.signature,"SAB ",4))
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"SDU_libLoad_DiskFile : Bad file !");
		return HXCFE_BADFILE;
	}

	raw_iso_setdefcfg(&rawcfg);

	rawcfg.number_of_tracks = sduh.max_cylinder;
	rawcfg.number_of_sides = sduh.max_head;
	rawcfg.number_of_sectors_per_track = sduh.max_sector;
	rawcfg.sector_size = sduh.sector_size;
	rawcfg.interface_mode = GENERIC_SHUGART_DD_FLOPPYMODE;
	rawcfg.track_format = IBMFORMAT_DD;
	rawcfg.interleave = 1;
	rawcfg.gap3 = 30;
	rawcfg.rpm = 300;

	if( rawcfg.sector_size *  rawcfg.number_of_sectors_per_track > (512*11) )
	{
		rawcfg.bitrate = 500000;
	}
	else
	{
		rawcfg.bitrate = 250000;
	}

	ret = raw_iso_loader(imgldr_ctx, floppydisk, f_img, 0, 0, &rawcfg);

	hxc_fclose(f_img);

	return ret;
}

int SDU_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]="SDU_IMG";
	static const char plug_desc[]="SAB Diskette Utility Loader";
	static const char plug_ext[]="sdu";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   SDU_libIsValidDiskFile,
		(LOADDISKFILE)      SDU_libLoad_DiskFile,
		(WRITEDISKFILE)     0,
		(GETPLUGININFOS)    SDU_libGetPluginInfo
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
