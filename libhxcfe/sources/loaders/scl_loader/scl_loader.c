/*
//
// Copyright (C) 2006-2025 Jean-Fran�ois DEL NERO
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
// File : scl_loader.c
// Contains: SCL floppy image loader
//
// Written by: Jean-Fran�ois DEL NERO
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

#include "scl_loader.h"

unsigned char dir_entry[34] =
{
	0x01, 0x16, 0x00, 0xF0,
	0x09, 0x10, 0x00, 0x00,
	0x20, 0x20, 0x20, 0x20,
	0x20, 0x20, 0x20, 0x20,
	0x20, 0x00, 0x00, 0x64,
	0x69, 0x73, 0x6B, 0x6E,
	0x61, 0x6D, 0x65, 0x00,
	0x00, 0x00, 0x46, 0x55
};

int SCL_libIsValidDiskFile( HXCFE_IMGLDR * imgldr_ctx, HXCFE_IMGLDR_FILEINFOS * imgfile )
{
	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"SCL_libIsValidDiskFile");

	if(hxc_checkfileext(imgfile->path,"scl",SYS_PATH_TYPE))
	{
		if(!strncmp((char*)&imgfile->file_header, "SINCLAIR", 8))
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"SCL_libIsValidDiskFile : Sinclair SCL file !");
			return HXCFE_VALIDFILE;
		}
		else
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"SCL_libIsValidDiskFile : non Sinclair SCL file !(bad header)");
			return HXCFE_BADFILE;
		}
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"SCL_libIsValidDiskFile : non Sinclair SCL file !");
		return HXCFE_BADFILE;
	}
}

unsigned int lsb2ui(unsigned char *mem)
{
	return (mem[0] + (mem[1] * 256) + (mem[2] * 256 * 256)
			+ (mem[3] * 256 * 256 * 256));
}


void ui2lsb(unsigned char *mem, unsigned int value)
{
	mem[0] = (value>>0 )&0xFF;
	mem[1] = (value>>8 )&0xFF;
	mem[2] = (value>>16)&0xFF;
	mem[3] = (value>>24)&0xFF;
}

int SCL_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	raw_iso_cfg rawcfg;
	FILE * f_img;
	int ret;
	int i;
	void *tmp;
	unsigned int size;
	unsigned char *trd_fsec;
	unsigned char *trd_ftrk;
	unsigned char *trd_files;
	char sclsignature[8];
	unsigned char number_of_blocks;
	char block_headers[256][14];
	unsigned char * trd_image;

	uint32_t left;
	uint32_t trd_offset;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"SCL_libLoad_DiskFile %s",imgfile);

	f_img = hxc_fopen(imgfile,"rb");
	if( f_img == NULL )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"SCL_libLoad_DiskFile : Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	hxc_fread(&sclsignature,8,f_img);
	if(strncmp(sclsignature, "SINCLAIR", 8))
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"non Sinclair SCL file !(bad header)");
		hxc_fclose(f_img);
		return HXCFE_BADFILE;
	}

	hxc_fread(&number_of_blocks,1,f_img);
	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"SCL_libLoad_DiskFile : %d block(s) in the file",number_of_blocks);

	for (i=0; i < number_of_blocks; i++)
	{
		hxc_fread(&(block_headers[i][0]),14,f_img);
	}

	// allocate and init a TR DOS disk.
	raw_iso_setdefcfg(&rawcfg);

	rawcfg.number_of_tracks = 80;
	rawcfg.number_of_sides = 2;
	rawcfg.number_of_sectors_per_track = 16;
	trd_image = (unsigned char*)malloc(rawcfg.number_of_tracks * rawcfg.number_of_sides * rawcfg.number_of_sectors_per_track * 256);
	if(!trd_image)
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"SCL_libLoad_DiskFile : Malloc error !");
		hxc_fclose(f_img);
		return HXCFE_INTERNALERROR;
	}

	memset(trd_image,0, rawcfg.number_of_tracks * rawcfg.number_of_sides * rawcfg.number_of_sectors_per_track * 256);
	memcpy(&trd_image[0x08E2], dir_entry, 32);
	strncpy((char*)&trd_image[0x08F5], "HxCFE", 8);

	tmp = (char *) trd_image + 0x8E5;
	trd_files = (unsigned char *) trd_image + 0x8E4;
	trd_fsec = (unsigned char *) trd_image + 0x8E1;
	trd_ftrk = (unsigned char *) trd_image + 0x8E2;

	// copy blocks to the trd disk
	for (i=0; i < number_of_blocks; i++)
	{
		size = block_headers[i][13];
		if (lsb2ui(tmp) < size)
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"SCL_libLoad_DiskFile : File too long to fit in the image *trd_free=%u < size=%u",lsb2ui(tmp),size);
			hxc_fclose(f_img);
			free(trd_image);
			return HXCFE_INTERNALERROR;
		}

		if (*trd_files > 127)
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"SCL_libLoad_DiskFile : Image File full!");
			hxc_fclose(f_img);
			free(trd_image);
			return HXCFE_INTERNALERROR;
		}

		memcpy((void *) ((char *) trd_image + *trd_files * 16),
			 (void *) block_headers[i], 14);

		memcpy((void *) ((char *) trd_image + *trd_files * 16 + 0x0E),
			 (void *) trd_fsec, 2);

		left = (uint32_t) ((unsigned char) block_headers[i][13]) * 256L;
		trd_offset = (*trd_ftrk) * 4096L + (*trd_fsec) * 256L;

		hxc_fread(&(trd_image[trd_offset]),left,f_img);

		(*trd_files)++;

		ui2lsb(tmp, lsb2ui(tmp) - size);

		while (size > 15)
		{
		  (*trd_ftrk)++;
		  size = size - 16;
		}


		(*trd_fsec) += size;

		while ((*trd_fsec) > 15)
		{
			(*trd_fsec) -= 16;
			(*trd_ftrk)++;
		}

	}
	hxc_fclose(f_img);

	rawcfg.rpm=300;
	rawcfg.sector_size = 256; // SCL file support only 256bytes/sector floppies.
	rawcfg.gap3 = 50;
	rawcfg.track_format = IBMFORMAT_DD;
	rawcfg.interleave = 1;
	rawcfg.bitrate = 250000;
	rawcfg.interface_mode = GENERIC_SHUGART_DD_FLOPPYMODE;

	ret = raw_iso_loader(imgldr_ctx, floppydisk, 0, trd_image, rawcfg.number_of_tracks * rawcfg.number_of_sides * rawcfg.number_of_sectors_per_track * 256, &rawcfg);

	free(trd_image);

	return ret;
}


int SCL_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]="ZXSPECTRUM_SCL";
	static const char plug_desc[]="ZX SPECTRUM SCL Loader";
	static const char plug_ext[]="scl";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   SCL_libIsValidDiskFile,
		(LOADDISKFILE)      SCL_libLoad_DiskFile,
		(WRITEDISKFILE)     0,
		(GETPLUGININFOS)    SCL_libGetPluginInfo
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
