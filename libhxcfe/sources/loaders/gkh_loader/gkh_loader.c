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
// File : gkh_loader.c
// Contains: Ensoniq GKH floppy image loader
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

#include "gkh_loader.h"
#include "gkh_format.h"

int GKH_libIsValidDiskFile( HXCFE_IMGLDR * imgldr_ctx, HXCFE_IMGLDR_FILEINFOS * imgfile )
{
	gkh_header * header;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"GKH_libIsValidDiskFile");

	if( hxc_checkfileext(imgfile->path,"gkh",SYS_PATH_TYPE))
	{
		header = (gkh_header *)imgfile->file_header;

		if(!memcmp(&header->header_tag,"TDDFI",5))
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"GKH_libIsValidDiskFile : GKH file !");
		}
		else
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"GKH_libIsValidDiskFile : Bad header !!");
			return HXCFE_BADFILE;
		}

		return HXCFE_VALIDFILE;
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"GKH_libIsValidDiskFile : non GKH file !");
		return HXCFE_BADFILE;
	}
}

int GKH_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	FILE * f_img;
	raw_iso_cfg rawcfg;
	int ret;
	gkh_header header;

	int i;
	int data_offset;
	unsigned char tagbuffer[10];
	image_type_tag *img_type_tag;
	image_location_tag *img_location_tag;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"GKH_libLoad_DiskFile %s",imgfile);

	f_img = hxc_fopen(imgfile,"rb");
	if( f_img == NULL )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	data_offset = 58;

	memset(&header,0,sizeof(header));
	hxc_fread(&header,sizeof(header),f_img);

	if(!memcmp(&header.header_tag,"TDDFI",5) && header.version==1)
	{
		raw_iso_setdefcfg(&rawcfg);

		i=0;
		do
		{
			hxc_fread(&tagbuffer,10,f_img);
			switch(tagbuffer[0])
			{
				case 0x0A:
					img_type_tag = (image_type_tag *)&tagbuffer;
					rawcfg.sector_size = img_type_tag->sectorsize;
					rawcfg.number_of_tracks = img_type_tag->nboftrack;
					rawcfg.number_of_sides = (unsigned char)img_type_tag->nbofheads;
					rawcfg.number_of_sectors_per_track = img_type_tag->nbofsectors;
				break;

				case 0x0B:
					img_location_tag = (image_location_tag *)&tagbuffer;
					data_offset = img_location_tag->fileoffset;
				break;
			}
			i++;
		}while(i<header.numberoftags);

		rawcfg.interface_mode = GENERIC_SHUGART_DD_FLOPPYMODE;
		rawcfg.bitrate = 250000;
		rawcfg.rpm = 300;

		rawcfg.track_format = IBMFORMAT_DD;

		rawcfg.skew_per_track = 4;
		rawcfg.skew_per_side = 2;
		rawcfg.start_sector_id = 0;
		rawcfg.gap3 = 255;
		rawcfg.interleave = 1;

		fseek(f_img, data_offset,SEEK_SET);

		ret = raw_iso_loader(imgldr_ctx, floppydisk, f_img, 0, 0, &rawcfg);

		hxc_fclose(f_img);

		return ret;
	}

	imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"BAD GKH file!");
	hxc_fclose(f_img);
	return HXCFE_BADFILE;
}

int GKH_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]="ENSONIQ_GKH";
	static const char plug_desc[]="ENSONIQ GKH Loader";
	static const char plug_ext[]="gkh";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   GKH_libIsValidDiskFile,
		(LOADDISKFILE)      GKH_libLoad_DiskFile,
		(WRITEDISKFILE)     0,
		(GETPLUGININFOS)    GKH_libGetPluginInfo
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
