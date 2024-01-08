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
// File : trd_loader.c
// Contains: Spectrum Beta Disk (TR-DOS) TRD floppy image loader
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

#include "trd_loader.h"
#include "trd_writer.h"

int TRD_libIsValidDiskFile( HXCFE_IMGLDR * imgldr_ctx, HXCFE_IMGLDR_FILEINFOS * imgfile )
{
	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"TRD_libIsValidDiskFile");

	if(hxc_checkfileext(imgfile->path,"trd",SYS_PATH_TYPE))
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"TRD_libIsValidDiskFile : TRD file !");
		return HXCFE_VALIDFILE;
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"TRD_libIsValidDiskFile : non TRD file !");
		return HXCFE_BADFILE;
	}
}

int TRD_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	raw_iso_cfg rawcfg;
	FILE * f_img;
	int ret;
	unsigned int filesize;
	unsigned char tempsector[256];

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"TRD_libLoad_DiskFile %s",imgfile);

	f_img = hxc_fopen(imgfile,"rb");
	if( f_img == NULL )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	raw_iso_setdefcfg(&rawcfg);

	filesize = hxc_fgetsize(f_img);

	fseek (f_img , 8*256 , SEEK_SET);
	hxc_fread(tempsector,sizeof(tempsector),f_img);

	fseek (f_img , 0 , SEEK_SET);

	switch(filesize)
	{
		case 16*256 * 40 * 1: // 40 track one side
			//35 track, no errors
			rawcfg.number_of_tracks = 40;
			rawcfg.number_of_sides = 1;
			rawcfg.number_of_sectors_per_track = 16;
			break;

		case 16*256 * 40 * 2: // 40 track two side // 80 track one side

			//22: double-sided, 80 tracks
			//23: double-sided, 40 tracks
			//24: single-sided, 80 tracks
			//25: single-sided, 40 tracks

			switch(tempsector[0xE3])
			{
				case 22:
					rawcfg.number_of_tracks = 80;
					rawcfg.number_of_sides = 2;
					rawcfg.number_of_sectors_per_track = 16;
					break;
				case 23:
					rawcfg.number_of_tracks = 40;
					rawcfg.number_of_sides = 2;
					rawcfg.number_of_sectors_per_track = 16;
					break;
				case 24:
					rawcfg.number_of_tracks = 80;
					rawcfg.number_of_sides = 1;
					rawcfg.number_of_sectors_per_track = 16;
					break;
				case 25:
					rawcfg.number_of_tracks = 40;
					rawcfg.number_of_sides = 1;
					rawcfg.number_of_sectors_per_track = 16;
					break;

				default:
					imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Unsupported TRD file size ! (%d Bytes)",filesize);
					hxc_fclose(f_img);
					return HXCFE_UNSUPPORTEDFILE;
					break;
			}

			break;

		case 16*256 * 80 * 2: // 80 track two side
			rawcfg.number_of_tracks = 80;
			rawcfg.number_of_sides = 2;
			rawcfg.number_of_sectors_per_track = 16;
			break;

		default:
			switch(tempsector[0xE3])
			{
				case 22:
					rawcfg.number_of_tracks = 80;
					rawcfg.number_of_sides = 2;
					rawcfg.number_of_sectors_per_track = 16;
					break;
				case 23:
					rawcfg.number_of_tracks = 40;
					rawcfg.number_of_sides = 2;
					rawcfg.number_of_sectors_per_track = 16;
					break;
				case 24:
					rawcfg.number_of_tracks = 80;
					rawcfg.number_of_sides = 1;
					rawcfg.number_of_sectors_per_track = 16;
					break;
				case 25:
					rawcfg.number_of_tracks = 40;
					rawcfg.number_of_sides = 1;
					rawcfg.number_of_sectors_per_track = 16;
					break;

				default:
					// not supported !
					imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Unsupported TRD file size ! (%d Bytes)",filesize);
					hxc_fclose(f_img);
					return HXCFE_UNSUPPORTEDFILE;
					break;
			}
			break;
	}

	rawcfg.rpm = 300;
	rawcfg.sector_size = 256;
	rawcfg.gap3 = 50;
	rawcfg.interleave = 1;
	rawcfg.bitrate = 250000;
	rawcfg.interface_mode = GENERIC_SHUGART_DD_FLOPPYMODE;
	rawcfg.track_format = IBMFORMAT_DD;

	fseek(f_img,0,SEEK_SET);

	ret = raw_iso_loader(imgldr_ctx, floppydisk, f_img, 0, 0, &rawcfg);

	hxc_fclose(f_img);

	return ret;
}

int TRD_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]="ZXSPECTRUM_TRD";
	static const char plug_desc[]="Zx Spectrum TRD Loader";
	static const char plug_ext[]="trd";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   TRD_libIsValidDiskFile,
		(LOADDISKFILE)      TRD_libLoad_DiskFile,
		(WRITEDISKFILE)     TRD_libWrite_DiskFile,
		(GETPLUGININFOS)    TRD_libGetPluginInfo
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
