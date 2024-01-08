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
// File : vdk_loader.c
// Contains: Dragon 32 /64 VDK floppy image loader
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

#include "vdk_loader.h"
#include "vdk_writer.h"

#include "vdk_format.h"

int VDK_libIsValidDiskFile( HXCFE_IMGLDR * imgldr_ctx, HXCFE_IMGLDR_FILEINFOS * imgfile )
{
	int filesize;
	vdk_header *vdk_h;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"VDK_libIsValidDiskFile");
	if(imgfile)
	{
		vdk_h = (vdk_header *)&imgfile->file_header;

		if(vdk_h->signature==0x6B64)
		{
			filesize = imgfile->file_size - vdk_h->header_size;

			if(filesize%256)
			{
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"VDK_libIsValidDiskFile : non VDK file !");
				return HXCFE_BADFILE;
			}

			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"VDK_libIsValidDiskFile : VDK file !");
			return HXCFE_VALIDFILE;
		}

		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"VDK_libIsValidDiskFile : non VDK file !");
		return HXCFE_BADFILE;
	}

	return HXCFE_BADPARAMETER;
}


int VDK_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	raw_iso_cfg rawcfg;
	int ret;
	FILE * f_img;
	vdk_header vdk_h;

	unsigned int filesize;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"VDK_libLoad_DiskFile %s",imgfile);

	f_img = hxc_fopen(imgfile,"rb");
	if( f_img == NULL )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	filesize = hxc_fgetsize(f_img);

	memset(&vdk_h,0,sizeof(vdk_header));
	hxc_fread(&vdk_h,sizeof(vdk_header),f_img);

	fseek (f_img , vdk_h.header_size , SEEK_SET);

	if((vdk_h.signature!=0x6B64) || ((filesize-vdk_h.header_size)%256))
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"non VDK file !");
		hxc_fclose(f_img);
		return HXCFE_BADFILE;
	}

	raw_iso_setdefcfg(&rawcfg);

	rawcfg.bitrate = 250000;
	rawcfg.rpm = 300;
	rawcfg.skew_per_track = 0;
	rawcfg.sector_size = 256;
	rawcfg.gap3 = 24;
	rawcfg.interleave = 2;
	rawcfg.track_format = ISOFORMAT_DD;
	rawcfg.interface_mode = GENERIC_SHUGART_DD_FLOPPYMODE;
	rawcfg.number_of_sides = vdk_h.number_of_sides;
	rawcfg.number_of_tracks = vdk_h.number_of_track;
	rawcfg.number_of_sectors_per_track = (filesize - vdk_h.header_size) / (rawcfg.number_of_tracks * rawcfg.number_of_sides * 256);

	imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"VDK File : VDK version: 0x%.2x,Source ID: 0x%.2x, Source Version: 0x%.2x, Flags: 0x%.2x",vdk_h.version,vdk_h.file_source_id,vdk_h.file_source_ver,vdk_h.flags);
	imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"%d tracks, %d side(s), %d sectors/track, rpm:%d",rawcfg.number_of_tracks,rawcfg.number_of_sides,rawcfg.number_of_sectors_per_track,rawcfg.rpm);

	ret = raw_iso_loader(imgldr_ctx, floppydisk, f_img, 0, 0, &rawcfg);

	hxc_fclose(f_img);

	return ret;
}

int VDK_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]="DRAGON3264_VDK";
	static const char plug_desc[]="DRAGON32 & 64 VDK Loader";
	static const char plug_ext[]="vdk";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   VDK_libIsValidDiskFile,
		(LOADDISKFILE)      VDK_libLoad_DiskFile,
		(WRITEDISKFILE)     VDK_libWrite_DiskFile,
		(GETPLUGININFOS)    VDK_libGetPluginInfo
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
