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
// File : sdd_speccydos_loader.c
// Contains: SpeccyDos SDD floppy image loader
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

#include "sdd_speccydos_loader.h"
#include "sdd_speccydos_writer.h"

#include "sddfileformat.h"

int sdd_getfloppyconfig(unsigned char * img,raw_iso_cfg *rawcfg)
{
	sddfileformats_t  * uimg;

	uimg=(sddfileformats_t *)img;

	raw_iso_setdefcfg(rawcfg);

	if(!strncmp((char*)uimg->SIGN,"TRKY2",5))
	{
		rawcfg->gap3 = 64;
		rawcfg->interleave = 1;
		rawcfg->number_of_sectors_per_track = uimg->nb_sector_per_track;
		rawcfg->number_of_sides = ((uimg->nb_track_side >> 7) & 1) + 1;
		rawcfg->number_of_tracks = uimg->nb_track_side & 0x7F;
		rawcfg->interface_mode = GENERIC_SHUGART_DD_FLOPPYMODE;
		rawcfg->bitrate = DEFAULT_DD_BITRATE;
		rawcfg->rpm = 300;
		rawcfg->trk_grouped_by_sides = 1;

		if(!(uimg->density & 1))
			rawcfg->track_format = IBMFORMAT_DD;
		else
			rawcfg->track_format = IBMFORMAT_SD;

		return 1;
	}
	else
	{
		return 0;
	}
}


int SDDSpeccyDos_libIsValidDiskFile( HXCFE_IMGLDR * imgldr_ctx, HXCFE_IMGLDR_FILEINFOS * imgfile )
{
	raw_iso_cfg rawcfg;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"SDDSpeccyDos_libIsValidDiskFile");

	if(hxc_checkfileext(imgfile->path,"sdd",SYS_PATH_TYPE))
	{
		if( imgfile->file_size & 0xFF)
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"SDDSpeccyDos_libIsValidDiskFile : non SDD IMG file - bad file size !");
			return HXCFE_BADFILE;
		}

		if(sdd_getfloppyconfig( imgfile->file_header, &rawcfg) )
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"SDD file : filesize:%dkB, %d tracks, %d side(s), %d sectors/track",imgfile->file_size/1024,rawcfg.number_of_tracks,rawcfg.number_of_sides,rawcfg.number_of_sectors_per_track);
		}
		else
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"SDDSpeccyDos_libIsValidDiskFile : non SDD file : Wrong boot sector signature!");
		}

		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"SDDSpeccyDos_libIsValidDiskFile : SDD file !");
		return HXCFE_VALIDFILE;
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"SDDSpeccyDos_libIsValidDiskFile : non SDD file !");
		return HXCFE_BADFILE;
	}
}

int SDDSpeccyDos_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	raw_iso_cfg rawcfg;
	FILE * f_img;
	int ret;
	unsigned char bootsector[256];

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"SDDSpeccyDos_libLoad_DiskFile %s",imgfile);

	f_img = hxc_fopen(imgfile,"rb");
	if( f_img == NULL )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	// read the first sector
	memset(bootsector, 0,sizeof(bootsector));
	hxc_fread(bootsector,sizeof(bootsector),f_img);
	if( sdd_getfloppyconfig( bootsector, &rawcfg) )
	{
		fseek(f_img,0,SEEK_SET);

		ret = raw_iso_loader(imgldr_ctx, floppydisk, f_img, 0, 0, &rawcfg);

		hxc_fclose(f_img);

		return ret;
	}

	hxc_fclose(f_img);

	return HXCFE_FILECORRUPTED;
}

int SDDSpeccyDos_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]="SPECCYDOS_SDD";
	static const char plug_desc[]="Speccy DOS SDD File Loader";
	static const char plug_ext[]="sdd";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   SDDSpeccyDos_libIsValidDiskFile,
		(LOADDISKFILE)      SDDSpeccyDos_libLoad_DiskFile,
		(WRITEDISKFILE)     SDDSpeccyDos_libWrite_DiskFile,
		(GETPLUGININFOS)    SDDSpeccyDos_libGetPluginInfo
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
