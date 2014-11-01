/*
//
// Copyright (C) 2006-2014 Jean-François DEL NERO
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
// File : ti99pc99_loader.c
// Contains: TI99 PC99 floppy image loader
//
// Written by:	DEL NERO Jean Francois
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

#include "vdk_loader.h"
#include "vdk_format.h"

#include "libhxcadaptor.h"

int VDK_libIsValidDiskFile(HXCFE_IMGLDR * imgldr_ctx,char * imgfile)
{
	int filesize;
	vdk_header vdk_h;
	FILE * f;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"VDK_libIsValidDiskFile");
	if(imgfile)
	{

		f=hxc_fopen(imgfile,"rb");
		if(f==NULL)
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"VDK_libIsValidDiskFile : Cannot open %s !",imgfile);
			return -1;
		}

		fseek (f , 0 , SEEK_END);
		filesize=ftell(f);
		fseek (f , 0 , SEEK_SET);

		fread(&vdk_h,sizeof(vdk_header),1,f);

		hxc_fclose(f);

		if(vdk_h.signature==0x6B64)
		{
			filesize=filesize-vdk_h.header_size;

			if(filesize%256)
			{
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"VDK_libIsValidDiskFile : non VDK file !");
				return HXCFE_BADFILE;
			}

			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"VDK_libIsValidDiskFile : VDK file !");
			return HXCFE_VALIDFILE;
		}

		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"VDK_libIsValidDiskFile : non VDK file !");
		return HXCFE_BADFILE;
	}

	return HXCFE_BADPARAMETER;
}


int VDK_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{

	FILE * f;
	unsigned int filesize,filetracklen;
	unsigned int i,j,file_offset;
	unsigned char  gap3len,interleave;
	unsigned char* trackdata;
	vdk_header vdk_h;
	unsigned short rpm,sectorsize;
	unsigned char skew,trackformat;
	HXCFE_CYLINDER* currentcylinder;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"VDK_libLoad_DiskFile %s",imgfile);

	f=hxc_fopen(imgfile,"rb");
	if(f==NULL)
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}


	fseek (f , 0 , SEEK_END);
	filesize=ftell(f);
	fseek (f , 0 , SEEK_SET);

	memset(&vdk_h,0,sizeof(vdk_header));
	fread(&vdk_h,sizeof(vdk_header),1,f);

	fseek (f , vdk_h.header_size , SEEK_SET);

	if((vdk_h.signature!=0x6B64) || ((filesize-vdk_h.header_size)%256))
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"non VDK file !");
		hxc_fclose(f);
		return HXCFE_BADFILE;
	}


	floppydisk->floppyNumberOfTrack=vdk_h.number_of_track;
	floppydisk->floppyNumberOfSide=vdk_h.number_of_sides;
	floppydisk->floppySectorPerTrack=(filesize-vdk_h.header_size)/(floppydisk->floppyNumberOfTrack*floppydisk->floppyNumberOfSide*256);
	floppydisk->floppyBitRate=250000;
	floppydisk->floppyiftype=GENERIC_SHUGART_DD_FLOPPYMODE;
	floppydisk->tracks=(HXCFE_CYLINDER**)malloc(sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);

	rpm=300; // normal rpm
	skew=0;
	imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"VDK File : VDK version: 0x%.2x,Source ID: 0x%.2x, Source Version: 0x%.2x, Flags: 0x%.2x",vdk_h.version,vdk_h.file_source_id,vdk_h.file_source_ver,vdk_h.flags);
	imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"%d tracks, %d side(s), %d sectors/track, rpm:%d",floppydisk->floppyNumberOfTrack,floppydisk->floppyNumberOfSide,floppydisk->floppySectorPerTrack,rpm);

	sectorsize=256;
	gap3len=24;
	interleave=2;
	trackformat=ISOFORMAT_DD;

	filetracklen=sectorsize*floppydisk->floppySectorPerTrack;
	trackdata=(unsigned char*)malloc(filetracklen);


	for(j=0;j<floppydisk->floppyNumberOfTrack;j++)
	{

		floppydisk->tracks[j]=allocCylinderEntry(rpm,floppydisk->floppyNumberOfSide);
		currentcylinder=floppydisk->tracks[j];

		for(i=0;i<floppydisk->floppyNumberOfSide;i++)
		{
			file_offset=((filetracklen*j*floppydisk->floppyNumberOfSide)+(filetracklen*(i&1)))+vdk_h.header_size;
			fseek (f , file_offset , SEEK_SET);

			fread(trackdata,filetracklen,1,f);

			currentcylinder->sides[i]=tg_generateTrack(trackdata,sectorsize,floppydisk->floppySectorPerTrack,(unsigned char)j,(unsigned char)i,1,interleave,(unsigned char)(((j<<1)|(i&1))*skew),floppydisk->floppyBitRate,currentcylinder->floppyRPM,trackformat,gap3len,0,2500,-2500);
		}

	}

	free(trackdata);

	imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"track file successfully loaded and encoded!");

	hxc_fclose(f);
	return HXCFE_NOERROR;
}

int VDK_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,unsigned long infotype,void * returnvalue)
{

	static const char plug_id[]="DRAGON3264_VDK";
	static const char plug_desc[]="DRAGON32 & 64 VDK Loader";
	static const char plug_ext[]="vdk";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)	VDK_libIsValidDiskFile,
		(LOADDISKFILE)		VDK_libLoad_DiskFile,
		(WRITEDISKFILE)		0,
		(GETPLUGININFOS)	VDK_libGetPluginInfo
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

