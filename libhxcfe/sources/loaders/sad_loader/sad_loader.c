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
// File : sad_loader.c
// Contains: SAD floppy image loader
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

#include "sad_loader.h"
#include "sad_fileformat.h"

int SAD_libIsValidDiskFile( HXCFE_IMGLDR * imgldr_ctx, HXCFE_IMGLDR_FILEINFOS * imgfile )
{
	SAD_HEADER * sadh;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"SAD_libIsValidDiskFile");
	if(imgfile)
	{
		sadh = (SAD_HEADER *)imgfile->file_header;
		if(!strncmp((char*)sadh->abSignature,SAD_SIGNATURE,sizeof SAD_SIGNATURE - 1))
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"SAD_libIsValidDiskFile : SAD file !");
			return HXCFE_VALIDFILE;
		}
		else
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"SAD_libIsValidDiskFile : non SAD file !");
			return HXCFE_BADFILE;
		}
	}

	return HXCFE_BADPARAMETER;
}

int SAD_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	raw_iso_cfg rawcfg;
	FILE * f_img;
	int ret;
	SAD_HEADER sadh;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"SAD_libLoad_DiskFile %s",imgfile);

	f_img = hxc_fopen(imgfile,"rb");
	if( f_img == NULL )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"SAD_libLoad_DiskFile : Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	memset(&sadh,0,sizeof(SAD_HEADER));
	hxc_fread(&sadh,sizeof(SAD_HEADER),f_img);
	if(strncmp((char*)sadh.abSignature,SAD_SIGNATURE,sizeof SAD_SIGNATURE - 1))
	{
		hxc_fclose(f_img);
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"SAD_libLoad_DiskFile : non SAD file !");
		return HXCFE_BADFILE;
	}

	raw_iso_setdefcfg(&rawcfg);

	rawcfg.number_of_tracks = sadh.bTracks;
	rawcfg.number_of_sides = sadh.bSides;
	rawcfg.number_of_sectors_per_track = sadh.bSectors;
	rawcfg.sector_size = sadh.bSectorSizeDiv64*64;
	rawcfg.gap3 = 35;
	rawcfg.interleave = 1;
	rawcfg.track_format = ISOFORMAT_DD;
	rawcfg.bitrate = 250000;
	rawcfg.interface_mode = GENERIC_SHUGART_DD_FLOPPYMODE;
	rawcfg.rpm = 300; // normal rpm
	rawcfg.trk_grouped_by_sides = 1;

	ret = raw_iso_loader(imgldr_ctx, floppydisk, f_img, 0, 0, &rawcfg);

	hxc_fclose(f_img);

	return ret;
}

int SAD_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]="SAMCOUPE_SAD";
	static const char plug_desc[]="SAM COUPE SAD Loader";
	static const char plug_ext[]="sad";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   SAD_libIsValidDiskFile,
		(LOADDISKFILE)      SAD_libLoad_DiskFile,
		(WRITEDISKFILE)     0,
		(GETPLUGININFOS)    SAD_libGetPluginInfo
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
