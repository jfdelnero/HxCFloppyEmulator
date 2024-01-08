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
// File : flppcm_loader.c
// Contains: FLP PC Magazine floppy image loader
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

#include "flppcm_loader.h"

#include "flppcmfileformat.h"

int FLPPCM_libIsValidDiskFile( HXCFE_IMGLDR * imgldr_ctx, HXCFE_IMGLDR_FILEINFOS * imgfile )
{
	flp_header_t * flp_header;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"FLPPCM_libIsValidDiskFile");

	if(hxc_checkfileext(imgfile->path,"flp",SYS_PATH_TYPE))
	{
		flp_header = (flp_header_t *)&imgfile->file_header;

		if(strncmp((char*)flp_header->hsign,"PCM",sizeof(flp_header->hsign)))
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"FLPPCM_libIsValidDiskFile : non FLP file - bad header !");
			return HXCFE_BADFILE;
		}

		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"FLPPCM_libIsValidDiskFile : FLP file !");
		return HXCFE_VALIDFILE;
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"FLPPCM_libIsValidDiskFile : non FLP file !");
		return HXCFE_BADFILE;
	}
}

int FLPPCM_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	raw_iso_cfg rawcfg;

	FILE * f_img;
	int ret;
	flp_header_t flp_header;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"FLPPCM_libLoad_DiskFile %s",imgfile);

	f_img = hxc_fopen(imgfile,"rb");
	if( f_img == NULL )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	memset(&flp_header,0,sizeof(flp_header_t));

	hxc_fread(&flp_header,sizeof(flp_header_t),f_img);

	raw_iso_setdefcfg(&rawcfg);

	if(!strncmp((char*)flp_header.hsign,"PCM",sizeof(flp_header.hsign)))
	{
		rawcfg.sector_size = flp_header.sectorsize;
		rawcfg.number_of_tracks = flp_header.nbtracks;
		rawcfg.number_of_sides = (unsigned char)flp_header.nbsides;
		rawcfg.number_of_sectors_per_track = flp_header.nbsectors;
		rawcfg.bitrate = 250000;
		rawcfg.interface_mode = IBMPC_DD_FLOPPYMODE;
		if( rawcfg.number_of_sectors_per_track >= 15 && rawcfg.sector_size >=512 )
		{
			rawcfg.bitrate = 500000;
			rawcfg.interface_mode = IBMPC_HD_FLOPPYMODE;
		}

		rawcfg.rpm = 300;
		rawcfg.gap3 = 0xFF;
		rawcfg.interleave = 1;

		rawcfg.track_format = IBMFORMAT_DD;

		ret = raw_iso_loader(imgldr_ctx, floppydisk, f_img, 0, 0, &rawcfg);

		hxc_fclose(f_img);

		return ret;
	}

	imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Bad FLP header");
	hxc_fclose(f_img);

	return HXCFE_BADFILE;
}

int FLPPCM_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]="FLP_IMG";
	static const char plug_desc[]="FLP PC Magazine image Loader";
	static const char plug_ext[]="flp";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   FLPPCM_libIsValidDiskFile,
		(LOADDISKFILE)      FLPPCM_libLoad_DiskFile,
		(WRITEDISKFILE)     0,
		(GETPLUGININFOS)    FLPPCM_libGetPluginInfo
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
