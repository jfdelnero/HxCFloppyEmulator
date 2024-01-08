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
// File : msx_loader.c
// Contains: MSX IMG floppy image loader
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

#include "msx_loader.h"
#include "msxfileformat.h"

int msx_imggetfloppyconfig(char * filename,unsigned char * img,uint32_t filesize, raw_iso_cfg *rawcfg)
{
	int i;
	unsigned char * uimg;
	int conffound,numberofsector;

	conffound=0;

	uimg=(unsigned char *)img;

	raw_iso_setdefcfg(rawcfg);
	rawcfg->track_format = IBMFORMAT_DD;
	rawcfg->interface_mode = MSX2_DD_FLOPPYMODE;

	if(!strstr( filename,".img" ))
	{
		img++;
	}

	conffound=0;
	if(uimg[0x18]<24 && uimg[0x18]>7)
	{

		rawcfg->rpm=300;
		rawcfg->number_of_sectors_per_track = uimg[0x18];
		rawcfg->number_of_sides = uimg[0x1A];
		rawcfg->gap3 = 60;
		rawcfg->interleave = 1;
		rawcfg->bitrate = 250000;
		rawcfg->sector_size = 512;

		numberofsector = uimg[0x13]+(uimg[0x14]*256);
		rawcfg->number_of_tracks = (numberofsector/(rawcfg->number_of_sectors_per_track * rawcfg->number_of_sides));

		//	if((unsigned int)((*numberofsectorpertrack) * (*numberoftrack) * (*numberofside) *512)==filesize)
		{
			conffound=1;
		}

	}

	if(conffound==0)
	{
		i=0;
		do
		{

			if(msxfileformats[i].filesize == filesize)
			{
				rawcfg->number_of_tracks = msxfileformats[i].numberoftrack;
				rawcfg->number_of_sectors_per_track = msxfileformats[i].sectorpertrack;
				rawcfg->number_of_sides = msxfileformats[i].numberofside;
				rawcfg->gap3 = msxfileformats[i].gap3len;
				rawcfg->interleave = msxfileformats[i].interleave;
				rawcfg->rpm = msxfileformats[i].RPM;
				rawcfg->bitrate = msxfileformats[i].bitrate;
				rawcfg->sector_size = msxfileformats[i].sectorsize;
				conffound=1;
			}
			i++;

		}while(msxfileformats[i].filesize!=0 && conffound==0);

	}
	return conffound;
}


int MSX_libIsValidDiskFile( HXCFE_IMGLDR * imgldr_ctx, HXCFE_IMGLDR_FILEINFOS * imgfile )
{
	int conffound,i;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"MSX_libIsValidDiskFile");

	if( hxc_checkfileext(imgfile->path,"img",SYS_PATH_TYPE) || hxc_checkfileext(imgfile->path,"dsk",SYS_PATH_TYPE) )
	{
		if(imgfile->file_size&0x1FF || !imgfile->file_size )
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"MSX_libIsValidDiskFile : non MSX IMG file - bad file size !");
			return HXCFE_BADFILE;
		}

		i=0;
		conffound=0;
		do
		{
			if((int)msxfileformats[i].filesize == imgfile->file_size)
			{
				conffound=1;
			}
			i++;
		}while(msxfileformats[i].filesize!=0 && conffound==0);

		if(conffound)
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"MSX_libIsValidDiskFile : MSX IMG file !");
		}
		else
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"MSX_libIsValidDiskFile : non MSX IMG file - bad file size !");
			return HXCFE_BADFILE;
		}
		return HXCFE_VALIDFILE;
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"MSX_libIsValidDiskFile : non MSX IMG file !");
		return HXCFE_BADFILE;
	}
}

int MSX_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	raw_iso_cfg rawcfg;
	int ret;
	unsigned char boot_sector[512];
	FILE * f_img;
	unsigned int filesize;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"MSX_libLoad_DiskFile %s",imgfile);

	f_img = hxc_fopen(imgfile,"rb");
	if( f_img == NULL )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	filesize = hxc_fgetsize(f_img);

	memset(boot_sector,0,sizeof(boot_sector));
	hxc_fread(&boot_sector,sizeof(boot_sector),f_img);

	if( msx_imggetfloppyconfig(imgfile, boot_sector, filesize, &rawcfg) == 1 )
	{
		fseek(f_img,0,SEEK_SET);

		ret = raw_iso_loader(imgldr_ctx, floppydisk, f_img, 0, 0, &rawcfg);

		hxc_fclose(f_img);

		return ret;
	}

	hxc_fclose(f_img);
	return HXCFE_BADFILE;
}

int MSX_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]="MSX_DSK";
	static const char plug_desc[]="MSX DSK Loader";
	static const char plug_ext[]="dsk";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   MSX_libIsValidDiskFile,
		(LOADDISKFILE)      MSX_libLoad_DiskFile,
		(WRITEDISKFILE)     0,
		(GETPLUGININFOS)    MSX_libGetPluginInfo
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
