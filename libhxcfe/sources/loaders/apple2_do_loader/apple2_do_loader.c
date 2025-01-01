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
// File : apple2_do_loader.c
// Contains: DO Apple II floppy image loader
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
#include "tracks/track_generator.h"
#include "libhxcfe.h"

#include "floppy_loader.h"
#include "floppy_utils.h"

#include "apple2_do_loader.h"

#include "libhxcadaptor.h"

int Apple2_do_libIsValidDiskFile( HXCFE_IMGLDR * imgldr_ctx, HXCFE_IMGLDR_FILEINFOS * imgfile )
{
	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Apple2_do_libIsValidDiskFile");

	if( hxc_checkfileext(imgfile->path,"do",SYS_PATH_TYPE) || hxc_checkfileext(imgfile->path,"po",SYS_PATH_TYPE))
	{
		if( imgfile->file_size % (16*1*256) || ( (imgfile->file_size/(16*1*256)) > 50 ) )
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Apple2_do_libIsValidDiskFile : non DO file !");
			return HXCFE_BADFILE;
		}

		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Apple2_do_libIsValidDiskFile : DO file !");
		return HXCFE_VALIDFILE;
	}

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Apple2_do_libIsValidDiskFile : non DO file !");
	return HXCFE_BADFILE;
}

const unsigned char LogicalToPhysicalSectorMap_Dos33[16] =
{
	0x00, 0x0D, 0x0B, 0x09, 0x07, 0x05, 0x03, 0x01,
	0x0E, 0x0C, 0x0A, 0x08, 0x06, 0x04, 0x02, 0x0F
};

const unsigned char PhysicalToLogicalSectorMap_Dos33[16] =
{
	0x00, 0x07, 0x0E, 0x06, 0x0D, 0x05, 0x0C, 0x04,
	0x0B, 0x03, 0x0A, 0x02, 0x09, 0x01, 0x08, 0x0F,
};

const unsigned char LogicalToPhysicalSectorMap_ProDos[16] =
{
	0x00, 0x02, 0x04, 0x06, 0x08, 0x0A, 0x0C, 0x0E,
	0x01, 0x03, 0x05, 0x07, 0x09, 0x0B, 0x0D, 0x0F,
};

const unsigned char PhysicalToLogicalSectorMap_ProDos[16] =
{
	0x00, 0x08, 0x01, 0x09, 0x02, 0x0A, 0x03, 0x0B,
	0x04, 0x0C, 0x05, 0x0D, 0x06, 0x0E, 0x07, 0x0F
};

int Apple2_do_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
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

	HXCFE_SECTCFG* sectorconfig;
	HXCFE_CYLINDER* currentcylinder;

	sectorconfig = NULL;
	currentcylinder = NULL;
	trackdata = NULL;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Apple2_do_libLoad_DiskFile %s",imgfile);

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

	filesize = hxc_fgetsize(f);

	if( filesize )
	{
		sectorsize=256;

		bitrate=250000;
		rpm=283;
		interleave=1;
		gap3len=0;
		trackformat=APPLE2_GCR6A2;
		floppydisk->floppyNumberOfSide=1;
		floppydisk->floppySectorPerTrack=16;
		floppydisk->floppyNumberOfTrack=filesize/( floppydisk->floppyNumberOfSide*floppydisk->floppySectorPerTrack*256);

		floppydisk->floppyBitRate=bitrate;
		floppydisk->floppyiftype=GENERIC_SHUGART_DD_FLOPPYMODE;
		floppydisk->tracks=(HXCFE_CYLINDER**)malloc(sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);

		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"rpm %d bitrate:%d track:%d side:%d sector:%d",rpm,bitrate,floppydisk->floppyNumberOfTrack,floppydisk->floppyNumberOfSide,floppydisk->floppySectorPerTrack);

		sectorconfig=(HXCFE_SECTCFG*)malloc(sizeof(HXCFE_SECTCFG)*floppydisk->floppySectorPerTrack);
		if(!sectorconfig)
			goto alloc_error;

		memset(sectorconfig,0,sizeof(HXCFE_SECTCFG)*floppydisk->floppySectorPerTrack);

		trackdata = (unsigned char*)malloc(sectorsize*floppydisk->floppySectorPerTrack);
		if(!trackdata)
			goto alloc_error;

		for(j=0;j<floppydisk->floppyNumberOfTrack;j++)
		{

			floppydisk->tracks[j]=allocCylinderEntry(rpm,floppydisk->floppyNumberOfSide);
			currentcylinder=floppydisk->tracks[j];

			for(i=0;i<floppydisk->floppyNumberOfSide;i++)
			{
				hxcfe_imgCallProgressCallback(imgldr_ctx, (j<<1) + (i&1),floppydisk->floppyNumberOfTrack*2);

				file_offset=(sectorsize*(j*floppydisk->floppySectorPerTrack*floppydisk->floppyNumberOfSide))+
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

				currentcylinder->sides[i] = tg_generateTrackEx(floppydisk->floppySectorPerTrack,sectorconfig,interleave,0,floppydisk->floppyBitRate,rpm,trackformat,20,2500,-2500);
			}
		}

		free(trackdata);
		free(sectorconfig);

		imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"track file successfully loaded and encoded!");
		hxc_fclose(f);

		return HXCFE_NOERROR;
	}

	imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"file size=%d !?",filesize);
	hxc_fclose(f);

	return HXCFE_BADFILE;

alloc_error:
	imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Alloc / Internal error !",imgfile);

	free(trackdata);
	free(sectorconfig);

	if(f)
		hxc_fclose(f);

	hxcfe_freeFloppy(imgldr_ctx->hxcfe, floppydisk );

	return HXCFE_INTERNALERROR;
}


int Apple2_do_libWrite_DiskFile(HXCFE_IMGLDR* imgldr_ctx,HXCFE_FLOPPY * floppy,char * filename);

int Apple2_do_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[] = "APPLE2_DO";
	static const char plug_desc[] = "Apple II DO Loader";
	static const char plug_ext[] = "do";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   Apple2_do_libIsValidDiskFile,
		(LOADDISKFILE)      Apple2_do_libLoad_DiskFile,
		(WRITEDISKFILE)     Apple2_do_libWrite_DiskFile,
		(GETPLUGININFOS)    Apple2_do_libGetPluginInfo
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

int Apple2_po_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[] = "APPLE2_PO";
	static const char plug_desc[] = "Apple II PO Loader";
	static const char plug_ext[] = "po";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   Apple2_do_libIsValidDiskFile,
		(LOADDISKFILE)      Apple2_do_libLoad_DiskFile,
		(WRITEDISKFILE)     Apple2_do_libWrite_DiskFile,
		(GETPLUGININFOS)    Apple2_do_libGetPluginInfo
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
