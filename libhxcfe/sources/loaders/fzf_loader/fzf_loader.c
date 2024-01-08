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
// File : fzf_loader.c
// Contains: Casio FZF floppy image loader
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

#include "fzf_loader.h"

char * fzffiletype[]={
	"full",
	"bank",
	"voice",
	"effects"
};

int FZF_libIsValidDiskFile( HXCFE_IMGLDR * imgldr_ctx, HXCFE_IMGLDR_FILEINFOS * imgfile )
{
	return hxcfe_imgCheckFileCompatibility( imgldr_ctx, imgfile, "FZF_libIsValidDiskFile", "fzf", 0 );
}

int FZF_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	raw_iso_cfg rawcfg;
	int ret;
	FILE * f = NULL;
	unsigned int  filesize;
	int  i,j;
	unsigned char * fzf_file = NULL;
	int c;

	unsigned char  number_of_banks;
	unsigned char  number_of_voices;
	unsigned short pcm_size;
	unsigned char  file_type;
	unsigned char* floppy_data = NULL;

	int  nbblock;
	char filename[512];

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"FZF_libLoad_DiskFile %s",imgfile);

	f = hxc_fopen(imgfile,"rb");
	if( f == NULL )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	filesize = hxc_fgetsize(f);

	nbblock = filesize / 1024;
	if(filesize & 0x3FF ) nbblock++;

	fzf_file = malloc ( nbblock * 1024 );
	if(!fzf_file)
		goto alloc_error;

	memset(fzf_file,0,nbblock * 1024);
	hxc_fread(fzf_file,filesize,f);

	hxc_fclose(f);
	f = NULL;

	raw_iso_setdefcfg(&rawcfg);

	rawcfg.number_of_tracks = 80;
	rawcfg.number_of_sides = 2;
	rawcfg.number_of_sectors_per_track = 8;
	rawcfg.bitrate = 500000;
	rawcfg.sector_size = 1024;
	rawcfg.interface_mode = GENERIC_SHUGART_DD_FLOPPYMODE;
	rawcfg.track_format = IBMFORMAT_DD;
	rawcfg.interleave = 3;
	rawcfg.gap3 = 255;
	rawcfg.rpm = 360;

	filesize = rawcfg.number_of_tracks * rawcfg.number_of_sides * rawcfg.number_of_sectors_per_track * rawcfg.bitrate;

	floppy_data = malloc(filesize);
	if(!floppy_data)
		goto alloc_error;

	memset(floppy_data, 0x5A, filesize);

	if (nbblock>1279)
	{
		nbblock=1276;
	}

	file_type = fzf_file[ 0x3e8 + 5 ];
	number_of_banks = fzf_file[ 0x3e8 + 7 ];
	number_of_voices = fzf_file[ 0x3e8 + 8 ];
	pcm_size = (fzf_file[ 0x3e8 + 0x0d ]<<8) | fzf_file[ 0x3e8 + 0x0c ];

	for( i = 0; i  < nbblock; i++)
	{
		memcpy(&floppy_data[(i+4)*rawcfg.sector_size],&fzf_file[i*rawcfg.sector_size],rawcfg.sector_size);
	}

	// Fragment list - Sector 3
	memset(&floppy_data[(rawcfg.sector_size*3)+0],0,rawcfg.sector_size);
	floppy_data[(rawcfg.sector_size*3)+0] = 3; // Sector
	floppy_data[(rawcfg.sector_size*3)+2] = (3+nbblock)&0xff;
	floppy_data[(rawcfg.sector_size*3)+3] = (3+nbblock)>>8;
	floppy_data[(rawcfg.sector_size*3)+1018] = number_of_banks;
	floppy_data[(rawcfg.sector_size*3)+1020] = number_of_voices;
	floppy_data[(rawcfg.sector_size*3)+1022] = (pcm_size&0xff);
	floppy_data[(rawcfg.sector_size*3)+1023] = (pcm_size>>8)&0xff;

	// Directory - Sector 2
	memset(&floppy_data[(rawcfg.sector_size*2)+0],0,rawcfg.sector_size);

	// Sector 1
	memset(&floppy_data[(rawcfg.sector_size)+0],0,rawcfg.sector_size);
	memcpy(&floppy_data[(rawcfg.sector_size)+0],"FULL-DATA-FZ",12);
	floppy_data[(rawcfg.sector_size)+12] = file_type;
	floppy_data[(rawcfg.sector_size)+13] = 0;
	floppy_data[(rawcfg.sector_size)+14] = 3;
	floppy_data[(rawcfg.sector_size)+15] = 0;

	// BAM - Sector 0
	memset(&floppy_data[0],0,rawcfg.sector_size);
	memset(&floppy_data[288],0xFF,736);

	for(i=0;i<nbblock+3;i+=8)
	{
		for(j=0;j<8;j++)
		{
			if((i>>3)*8+j<nbblock+3)
			{
				floppy_data[ 0x80+ (i>>3) ] = floppy_data[ 0x80 + (i>>3) ] | (1<<j);
			}
		}
	}

	hxc_getfilenamewext(imgfile,filename,SYS_PATH_TYPE);

	//Disk name
	for(i=0;i<12;i++)
	{

		if (i>(int)strlen(filename))
		{
			c = 0x20;
		}
		else
		{
			if ( (c = filename[i])<0x20) c=0x20;
		}

		floppy_data[i] = c;
		floppy_data[i+0x10] = c;
	}
	floppy_data[0x0e] = 2;

	free(fzf_file);

	//////////////////////////////////////////////////////////////

	ret = raw_iso_loader(imgldr_ctx, floppydisk, 0, floppy_data, filesize, &rawcfg);

	free(floppy_data);

	return ret;

alloc_error:

	if ( f )
		hxc_fclose( f );

	hxcfe_freeFloppy(imgldr_ctx->hxcfe, floppydisk );

	free(fzf_file);
	free(floppy_data);

	return HXCFE_INTERNALERROR;
}

int FZF_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]="CASIO_FZF";
	static const char plug_desc[]="Casio FZF file Loader";
	static const char plug_ext[]="fzf";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   FZF_libIsValidDiskFile,
		(LOADDISKFILE)      FZF_libLoad_DiskFile,
		(WRITEDISKFILE)     0,
		(GETPLUGININFOS)    FZF_libGetPluginInfo
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
