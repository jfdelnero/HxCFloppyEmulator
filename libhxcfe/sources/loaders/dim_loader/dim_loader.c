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
// File : dim_loader.c
// Contains: DIM floppy image loader
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

#include "dim_loader.h"
#include "dim_writer.h"

#include "dim_format.h"

int DIM_libIsValidDiskFile( HXCFE_IMGLDR * imgldr_ctx, HXCFE_IMGLDR_FILEINFOS * imgfile )
{
	dim_header * header;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"DIM_libIsValidDiskFile");

	if( hxc_checkfileext(imgfile->path,"dim",SYS_PATH_TYPE) )
	{
		header = (dim_header *)&imgfile->file_header;
		if( header->id_header==0x4242)
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"DIM_libIsValidDiskFile : DIM file !");
			return HXCFE_VALIDFILE;
		}
		else
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"DIM_libIsValidDiskFile : non DIM file ! Bad header!");
			return HXCFE_BADFILE;
		}
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"DIM_libIsValidDiskFile : non DIM file !");
		return HXCFE_BADFILE;
	}
}

int DIM_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	raw_iso_cfg rawcfg;
	int ret;
	FILE * f_img;
	dim_header header;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"DIM_libLoad_DiskFile %s",imgfile);

	f_img = hxc_fopen(imgfile,"rb");
	if( f_img == NULL )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	hxc_fread(&header,sizeof(dim_header), f_img);

	if(header.id_header != 0x4242)
	{
		hxc_fclose(f_img);

		return HXCFE_FILECORRUPTED;
	}

	raw_iso_setdefcfg(&rawcfg);

	rawcfg.bitrate = 250000;
	rawcfg.rpm = 300;
	rawcfg.sector_size = (header.sectorsizeh*256)+header.sectorsizel; // st file support only 512bytes/sector floppies.
	if(!rawcfg.sector_size || (rawcfg.sector_size&0xFF))
		rawcfg.sector_size = 512;
	rawcfg.gap3 = 30;
	rawcfg.interleave = 2;
	rawcfg.track_format = ISOFORMAT_DD;
	rawcfg.interface_mode = GENERIC_SHUGART_DD_FLOPPYMODE;
	rawcfg.number_of_sides = header.side + 1;
	rawcfg.number_of_tracks = header.end_track + 1;
	rawcfg.number_of_sectors_per_track = header.nbsector;

	if(!header.density)
	{
		rawcfg.bitrate = DEFAULT_DD_BITRATE;
		rawcfg.interface_mode = ATARIST_DD_FLOPPYMODE;
		rawcfg.skew_per_track = 4;
		rawcfg.skew_per_side = 2;
		rawcfg.number_of_sides = 1;
	}
	else
	{
		rawcfg.bitrate = DEFAULT_HD_BITRATE;
		rawcfg.interface_mode = ATARIST_HD_FLOPPYMODE;
		rawcfg.skew_per_track = 8;
		rawcfg.skew_per_side = 4;
		rawcfg.number_of_sides = 2;
	}

	if(floppydisk->floppySectorPerTrack==11)
	{
		rawcfg.track_format = ISOFORMAT_DD11S;
		rawcfg.gap3 = 3;
		rawcfg.interleave = 2;
	}

	fseek (f_img , 0x20 , SEEK_SET);

	ret = raw_iso_loader(imgldr_ctx, floppydisk, f_img, 0, 0, &rawcfg);

	hxc_fclose(f_img);

	return ret;
}

int DIM_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]="ATARIST_DIM";
	static const char plug_desc[]="ATARI ST DIM Loader";
	static const char plug_ext[]="dim";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   DIM_libIsValidDiskFile,
		(LOADDISKFILE)      DIM_libLoad_DiskFile,
		(WRITEDISKFILE)     DIM_libWrite_DiskFile,
		(GETPLUGININFOS)    DIM_libGetPluginInfo
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
