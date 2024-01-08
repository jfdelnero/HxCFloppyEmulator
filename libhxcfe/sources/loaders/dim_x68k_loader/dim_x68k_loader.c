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
// File : dim_x68k_loader.c
// Contains: X68000 DIM floppy image loader
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

#include "dim_x68k_loader.h"
#include "dim_x68k_format.h"

int DIM_x68k_libIsValidDiskFile( HXCFE_IMGLDR * imgldr_ctx, HXCFE_IMGLDR_FILEINFOS * imgfile )
{
	dim_x68k_header * header;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"DIM_x68k_libIsValidDiskFile");

	if( hxc_checkfileext(imgfile->path,"dim",SYS_PATH_TYPE) )
	{
		header = (dim_x68k_header *)&imgfile->file_header;
		if( header->media_byte < 4 && !memcmp(header->difc_sign, "DIFC HEADER  ", 13) )
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"DIM_x68k_libIsValidDiskFile : x68000 DIM file !");
			return HXCFE_VALIDFILE;
		}
		else
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"DIM_x68k_libIsValidDiskFile : non DIM file ! Bad header!");
			return HXCFE_BADFILE;
		}
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"DIM_x68k_libIsValidDiskFile : non DIM file !");
		return HXCFE_BADFILE;
	}
}

int DIM_x68k_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	raw_iso_cfg rawcfg;
	int ret,i;
	FILE * f_img;
	dim_x68k_header header;
	unsigned char *dsk_raw_buf;
	int dsk_buf_size, trk_raw_size;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"DIM_x68k_libLoad_DiskFile %s",imgfile);

	f_img = hxc_fopen(imgfile,"rb");
	if( f_img == NULL )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	hxc_fread(&header,sizeof(dim_x68k_header), f_img);

	if( header.media_byte >= 4 || memcmp(&header.difc_sign, "DIFC HEADER  ", 13) )
	{
		hxc_fclose(f_img);

		return HXCFE_FILECORRUPTED;
	}

	raw_iso_setdefcfg(&rawcfg);

	rawcfg.bitrate = 500000;
	rawcfg.sector_size = 1024;
	rawcfg.gap3 = 116;
	rawcfg.interleave = 1;
	rawcfg.skew_per_track = 0;
	rawcfg.skew_per_side = 0;
	rawcfg.track_format = ISOFORMAT_DD;
	rawcfg.interface_mode = GENERIC_SHUGART_DD_FLOPPYMODE;

	switch(header.media_byte)
	{
		case 0: // (2HS)   (8sec/trk 1232k)
			rawcfg.number_of_sides = 2;
			rawcfg.number_of_tracks = 80;
			rawcfg.number_of_sectors_per_track = 8;
			rawcfg.rpm = 360;
		break;
		case 1: // (2HS)   (9sec/trk 1440k)
			rawcfg.number_of_sides = 2;
			rawcfg.number_of_tracks = 80;
			rawcfg.number_of_sectors_per_track = 9;
			rawcfg.rpm = 300;
		break;
		case 2: // (2HC)   (15sec/trk 1200k) [80/2/15/512]
			rawcfg.number_of_sides = 2;
			rawcfg.number_of_tracks = 80;
			rawcfg.number_of_sectors_per_track = 15;
			rawcfg.sector_size = 512;
			rawcfg.rpm = 360;
		break;
		case 3: // (2HQ)   (18sec/trk 1440k) IBM 1.44MB 2HD format
			rawcfg.number_of_sides = 2;
			rawcfg.number_of_tracks = 80;
			rawcfg.number_of_sectors_per_track = 18;
			rawcfg.sector_size = 512;
			rawcfg.rpm = 300;
		break;
		default:
			hxc_fclose(f_img);
			return HXCFE_FILECORRUPTED;
		break;
	}

	i = 80;
	while( i > 77 && ( (header.sectors_present[(i-1)*2] != 0x01) && (header.sectors_present[((i-1)*2)+1] != 0x01) ) )
	{
		i--;
	}

	rawcfg.number_of_tracks = i;

	trk_raw_size = rawcfg.number_of_sectors_per_track * rawcfg.sector_size;
	dsk_buf_size = rawcfg.number_of_tracks * rawcfg.number_of_sides * trk_raw_size;

	dsk_raw_buf = malloc( dsk_buf_size );
	if(!dsk_raw_buf)
	{
		hxc_fclose(f_img);

		return HXCFE_INTERNALERROR;
	}

	memset( dsk_raw_buf, 0xE5, dsk_buf_size );

	fseek (f_img , 0x100 , SEEK_SET);
	for(i=0;i<160;i++)
	{
		if( header.sectors_present[i] == 0x01 )
		{
			fseek( f_img , 0x100 + (i * trk_raw_size), SEEK_SET);
			hxc_fread( &dsk_raw_buf[i * trk_raw_size], trk_raw_size, f_img);
		}
	}

	ret = raw_iso_loader(imgldr_ctx, floppydisk, NULL, dsk_raw_buf, dsk_buf_size, &rawcfg);

	free(dsk_raw_buf);

	hxc_fclose(f_img);

	return ret;
}

int DIM_x68k_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]="X68000_DIM";
	static const char plug_desc[]="X68000 DIM file loader";
	static const char plug_ext[]="dim";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   DIM_x68k_libIsValidDiskFile,
		(LOADDISKFILE)      DIM_x68k_libLoad_DiskFile,
		(WRITEDISKFILE)     NULL,
		(GETPLUGININFOS)    DIM_x68k_libGetPluginInfo
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
