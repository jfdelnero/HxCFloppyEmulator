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
// File : fdi_nec_loader.c
// Contains: FDI floppy image loader
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

#include "fdi_nec_loader.h"
#include "fdi_nec_format.h"

int FDINEC_libIsValidDiskFile( HXCFE_IMGLDR * imgldr_ctx, HXCFE_IMGLDR_FILEINFOS * imgfile )
{
	fdi_nec_header * f_header;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"FDINEC_libIsValidDiskFile");

	if( hxc_checkfileext(imgfile->path,"fdi",SYS_PATH_TYPE) )
	{
		f_header = (fdi_nec_header *)imgfile->file_header;

		if( f_header->signature || f_header->cylinders > (uint32_t)90 || imgfile->file_size > 1265664 || f_header->sectorsize % 128 || !f_header->headersize || f_header->headersize > (uint32_t) imgfile->file_size )
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"FDINEC_libIsValidDiskFile : non FDI file !");
			return HXCFE_BADFILE;
		}

		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"FDINEC_libIsValidDiskFile : FDI file !");
		return HXCFE_VALIDFILE;
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"FDINEC_libIsValidDiskFile : non FDI file !");
		return HXCFE_BADFILE;
	}
}

int FDINEC_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	raw_iso_cfg rawcfg;
	FILE * f_img;
	int filesize,ret;

	fdi_nec_header f_header;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"FDINEC_libLoad_DiskFile %s",imgfile);

	f_img = hxc_fopen(imgfile,"rb");
	if( f_img == NULL )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	filesize = hxc_fgetsize(f_img);

	hxc_fread(&f_header,sizeof(fdi_nec_header),f_img);

	if( f_header.signature || f_header.cylinders > (uint32_t)90 || filesize > (int)1265664 || f_header.sectorsize % 128 || !f_header.headersize || f_header.headersize > (uint32_t)filesize )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Bad NEC FDI file !");
		hxc_fclose(f_img);
		return HXCFE_BADFILE;
	}

	fseek(f_img,f_header.headersize,SEEK_SET);

	raw_iso_setdefcfg(&rawcfg);

	rawcfg.rpm = 360;
	rawcfg.number_of_tracks = f_header.cylinders;
	rawcfg.number_of_sides = f_header.surfaces;
	rawcfg.number_of_sectors_per_track = f_header.sectors;
	rawcfg.sector_size = f_header.sectorsize;
	rawcfg.gap3 = 255;
	rawcfg.interface_mode = GENERIC_SHUGART_DD_FLOPPYMODE;
	rawcfg.track_format = IBMFORMAT_DD;

	if( rawcfg.sector_size * rawcfg.number_of_sectors_per_track > 6144 )
		rawcfg.bitrate = 500000;
	else
		rawcfg.bitrate = 250000;

	ret = raw_iso_loader(imgldr_ctx, floppydisk, f_img, 0, 0, &rawcfg);

	hxc_fclose(f_img);

	return ret;

}

int FDINEC_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]="NEC_FDI";
	static const char plug_desc[]="NEC FDI Loader";
	static const char plug_ext[]="fdi";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   FDINEC_libIsValidDiskFile,
		(LOADDISKFILE)      FDINEC_libLoad_DiskFile,
		(WRITEDISKFILE)     0,
		(GETPLUGININFOS)    FDINEC_libGetPluginInfo
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
