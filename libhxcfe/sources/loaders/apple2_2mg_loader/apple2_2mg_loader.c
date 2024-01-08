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
// File : Apple2_2mg_loader.c
// Contains: 2MG Apple II floppy image loader
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
#include "tracks/track_generator.h"
#include "libhxcfe.h"

#include "floppy_loader.h"
#include "floppy_utils.h"

#include "apple2_2mg_loader.h"
#include "apple2_2mg_format.h"

#include "libhxcadaptor.h"

int Apple2_2mg_libIsValidDiskFile( HXCFE_IMGLDR * imgldr_ctx, HXCFE_IMGLDR_FILEINFOS * imgfile )
{
	A2_2MG_header * header;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Apple2_2mg_libIsValidDiskFile");

	if( hxc_checkfileext(imgfile->path,"2mg",SYS_PATH_TYPE) || hxc_checkfileext(imgfile->path,"2img",SYS_PATH_TYPE) )
	{

		header = (A2_2MG_header *)&imgfile->file_header;

		if( !strncmp((char*)header->sign,"2IMG",4))
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Apple2_2mg_libIsValidDiskFile : 2MG file !");
			return HXCFE_VALIDFILE;
		}
		else
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Apple2_2mg_libIsValidDiskFile : non 2MG file !");
			return HXCFE_BADFILE;
		}

		return HXCFE_BADFILE;
	}

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Apple2_2mg_libIsValidDiskFile : non 2MG file !");
	return HXCFE_BADFILE;
}

int Apple2_2mg_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	FILE * f;
	unsigned int filesize;
	int i,j,k;
	unsigned int file_offset;
	unsigned char* trackdata;
	int trackformat;
	int gap3len,interleave;
	int sectorsize,rpm;
	int bitrate;
	unsigned char * sector_order;
	A2_2MG_header header;
	char tmpstr[1024];


	HXCFE_SECTCFG* sectorconfig;
	HXCFE_CYLINDER* currentcylinder;

	f = NULL;
	trackdata = NULL;
	sector_order = NULL;
	sectorconfig = NULL;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Apple2_2mg_libLoad_DiskFile %s",imgfile);

	sector_order = (unsigned char *)&PhysicalToLogicalSectorMap_Dos33;
	if(hxc_checkfileext(imgfile,"po",SYS_PATH_TYPE))
	{
		sector_order = (unsigned char *)&PhysicalToLogicalSectorMap_ProDos;
	}

	f = hxc_fopen(imgfile,"rb");
	if( f == NULL )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	sectorsize = 256;

	bitrate = 250000;
	rpm = 283;
	interleave = 1;
	gap3len = 0;
	trackformat = APPLE2_GCR6A2;
	floppydisk->floppyNumberOfSide = 1;
	floppydisk->floppySectorPerTrack = 16;

	filesize = hxc_fgetsize(f);

	if( filesize )
	{
		memset(&header,0,sizeof(A2_2MG_header));
		hxc_fread(&header,sizeof(A2_2MG_header),f);

		if( strncmp((char*)&header.sign,"2IMG",4))
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Apple2_2mg_libLoad_DiskFile : Bad file !");
			hxc_fclose(f);
			return HXCFE_BADFILE;
		}

		memset(tmpstr,0,sizeof(tmpstr));
		memcpy(tmpstr,&header.creator,4);

		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"2mg : creator = %s",tmpstr);
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"2mg : header_size = %d",header.header_size);
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"2mg : version = 0x%.4X",header.version);
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"2mg : format = 0x%.8X",header.format);

		switch(header.format)
		{
			case 0:
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"DOS 3.3 sector order");
			break;
			case 1:
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"ProDOS sector order");
				if(header.data_size >= 400 * 1024)
				{
					sectorsize = 512;
					floppydisk->floppySectorPerTrack=12;

					if(header.data_size > 432*1024)
						floppydisk->floppyNumberOfSide=2;
					else
						floppydisk->floppyNumberOfSide=1;
				}
				else
				{
					sectorsize = 256;
					floppydisk->floppyNumberOfSide=1;
					floppydisk->floppySectorPerTrack=16;
				}
			break;
			case 2:
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"NIB data");
			break;
			default:
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Unknown format!");
			break;
		}

		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"2mg : flags = 0x%.8X",header.flags);
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"2mg : prodos_blocks = %d",header.prodos_blocks);
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"2mg : data_offset = 0x%.8X",header.data_offset);
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"2mg : data_size = 0x%.8X",header.data_size);
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"2mg : comment_offset = 0x%.8X",header.comment_offset);
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"2mg : comment_size = 0x%.8X",header.comment_size);
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"2mg : creatordata_offset = 0x%.8X",header.creatordata_offset);
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"2mg : creatordata_size = 0x%.8X",header.creatordata_size);

		if(header.comment_size && header.comment_offset)
		{
			fseek (f , header.comment_offset , SEEK_SET);
			memset(tmpstr,0,sizeof(tmpstr));

			i = header.comment_size;
			if(i>=sizeof(tmpstr) - 1)
				i = sizeof(tmpstr) - 1;

			hxc_fread(tmpstr,i,f);

			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"2mg : comment = %s",tmpstr);
		}

		floppydisk->floppyNumberOfTrack=filesize/( floppydisk->floppyNumberOfSide*floppydisk->floppySectorPerTrack*256);

		floppydisk->floppyBitRate=bitrate;
		floppydisk->floppyiftype=GENERIC_SHUGART_DD_FLOPPYMODE;
		floppydisk->tracks = (HXCFE_CYLINDER**)malloc(sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);
		if( !floppydisk->tracks )
			goto alloc_error;

		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"rpm %d bitrate:%d track:%d side:%d sector:%d",rpm,bitrate,floppydisk->floppyNumberOfTrack,floppydisk->floppyNumberOfSide,floppydisk->floppySectorPerTrack);

		sectorconfig = (HXCFE_SECTCFG*)malloc(sizeof(HXCFE_SECTCFG)*floppydisk->floppySectorPerTrack);
		if( !sectorconfig )
			goto alloc_error;

		memset(sectorconfig,0,sizeof(HXCFE_SECTCFG)*floppydisk->floppySectorPerTrack);

		trackdata = (unsigned char*)malloc(sectorsize*floppydisk->floppySectorPerTrack);
		if( !trackdata )
			goto alloc_error;

		for(j=0;j<floppydisk->floppyNumberOfTrack;j++)
		{

			floppydisk->tracks[j]=allocCylinderEntry(rpm,floppydisk->floppyNumberOfSide);
			currentcylinder=floppydisk->tracks[j];

			for(i=0;i<floppydisk->floppyNumberOfSide;i++)
			{
				hxcfe_imgCallProgressCallback(imgldr_ctx, (j<<1) + (i&1),floppydisk->floppyNumberOfTrack*2);

				file_offset = header.data_offset +
							(sectorsize*(j*floppydisk->floppySectorPerTrack*floppydisk->floppyNumberOfSide)) +
							(sectorsize*(floppydisk->floppySectorPerTrack)*i);

				fseek (f , file_offset , SEEK_SET);
				hxc_fread(trackdata,sectorsize*floppydisk->floppySectorPerTrack,f);

				memset(sectorconfig,0,sizeof(HXCFE_SECTCFG)*floppydisk->floppySectorPerTrack);
				for(k=0;k<floppydisk->floppySectorPerTrack;k++)
				{
					sectorconfig[k].cylinder=j;
					sectorconfig[k].head=i;
					sectorconfig[k].sector=k;
					sectorconfig[k].bitrate=floppydisk->floppyBitRate;
					sectorconfig[k].gap3=gap3len;
					sectorconfig[k].sectorsize=sectorsize;
					sectorconfig[k].input_data=&trackdata[sector_order[k]*sectorsize];
					sectorconfig[k].trackencoding=trackformat;
				}

				currentcylinder->sides[i]=tg_generateTrackEx(floppydisk->floppySectorPerTrack,sectorconfig,interleave,0,floppydisk->floppyBitRate,rpm,trackformat,20,2500,-2500);
			}
		}

		free(sectorconfig);
		free(trackdata);

		imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"track file successfully loaded and encoded!");

		hxc_fclose(f);

		return HXCFE_NOERROR;
	}

	imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"file size=%d !?",filesize);
	hxc_fclose(f);

	return HXCFE_BADFILE;

alloc_error:

	free(sectorconfig);
	free(trackdata);

	if(f)
		hxc_fclose(f);

	hxcfe_freeFloppy(imgldr_ctx->hxcfe, floppydisk );

	return HXCFE_INTERNALERROR;
}

int Apple2_2mg_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[] = "APPLE2_2MG";
	static const char plug_desc[] = "Apple II 2MG Loader";
	static const char plug_ext[] = "2mg";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   Apple2_2mg_libIsValidDiskFile,
		(LOADDISKFILE)      Apple2_2mg_libLoad_DiskFile,
		(WRITEDISKFILE)     NULL,
		(GETPLUGININFOS)    Apple2_2mg_libGetPluginInfo
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
