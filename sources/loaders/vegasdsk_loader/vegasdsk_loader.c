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
// File : vegasdsk_loader.c
// Contains: vegas floppy image loader
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

#include "vegasdsk_loader.h"

#include "libhxcadaptor.h"

int VEGASDSK_libIsValidDiskFile(HXCFE_IMGLDR * imgldr_ctx,char * imgfile)
{
	unsigned char buffer[256];
	FILE * f;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"VEGASDSK_libIsValidDiskFile");

	if( hxc_checkfileext(imgfile,"veg") || hxc_checkfileext(imgfile,"vegasdsk") )
	{

		f=hxc_fopen(imgfile,"rb");
		if(f==NULL)
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"VEGASDSK_libIsValidDiskFile : Cannot open %s !",imgfile);
			return HXCFE_ACCESSERROR;
		}

		fseek (f , 256*(3-1) , SEEK_SET);
		fread(buffer,256,1,f);

		hxc_fclose(f);
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"VEGASDSK_libIsValidDiskFile : Vegas DSK file ! %d tracks %d sectors/tracks",buffer[0x26]+1,buffer[0x27]+1);
		return HXCFE_VALIDFILE;
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"VEGASDSK_libIsValidDiskFile : non Vegas DSK file !");
		return HXCFE_BADFILE;
	}
}



int VEGASDSK_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{

	FILE * f;
	unsigned int filesize;
	int i,j,k;
	unsigned int file_offset,offset;
	unsigned char* floppy_data;
	int gap3len,interleave;
	int sectorsize,rpm;
	int bootnumberofsector;
	unsigned char  trackformat;
	unsigned char  buffer[256];
	unsigned char  ddmode;

	HXCFE_CYLINDER* currentcylinder;
	HXCFE_SECTCFG  sectorconfig[30];

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"VEGASDSK_libLoad_DiskFile %s",imgfile);

	f=hxc_fopen(imgfile,"rb");
	if(f==NULL)
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	fseek (f , 0 , SEEK_END);
	filesize=ftell(f);

	fseek (f , 256*(3-1) , SEEK_SET);
	fread(buffer,256,1,f);

	fseek (f , 0 , SEEK_SET);

	gap3len=255;


	switch(buffer[0x27])
	{
		case 10:
			floppydisk->floppySectorPerTrack=10;
			bootnumberofsector=10;
			sectorsize=256;
			floppydisk->floppyNumberOfSide=1;
			interleave=5;
			trackformat=ISOFORMAT_SD;
			break;
		case 20:
			floppydisk->floppySectorPerTrack=10;
			bootnumberofsector=10;
			sectorsize=256;
			floppydisk->floppyNumberOfSide=2;
			interleave=5;
			trackformat=ISOFORMAT_SD;
			break;
		case 18:
			floppydisk->floppySectorPerTrack=18;
			gap3len=20;
			bootnumberofsector=10;
			sectorsize=256;
			ddmode=0xFF;
			floppydisk->floppyNumberOfSide=1;
			interleave=9;
			trackformat=ISOFORMAT_DD;
			break;
		case 36:
			floppydisk->floppySectorPerTrack=18;
			gap3len=20;
			bootnumberofsector=10;
			sectorsize=256;
			ddmode=0xFF;
			floppydisk->floppyNumberOfSide=2;
			interleave=9;
			trackformat=ISOFORMAT_DD;
			break;
		default:
			floppydisk->floppySectorPerTrack=10;
			bootnumberofsector=10;
			sectorsize=256;
			floppydisk->floppyNumberOfSide=1;
			interleave=5;
			trackformat=ISOFORMAT_SD;
			break;
	}

	floppydisk->floppyNumberOfTrack=buffer[0x26]+1;




	floppydisk->floppyBitRate=250000;
	floppydisk->floppyiftype=GENERIC_SHUGART_DD_FLOPPYMODE;
	floppydisk->tracks=(HXCFE_CYLINDER**)malloc(sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);

	rpm=300; // normal rpm

	imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"filesize:%dkB, %d tracks, %d side(s), %d sectors/track, gap3:%d, interleave:%d,rpm:%d",filesize/1024,floppydisk->floppyNumberOfTrack,floppydisk->floppyNumberOfSide,floppydisk->floppySectorPerTrack,gap3len,interleave,rpm);

	j=0;
	floppydisk->tracks[j]=allocCylinderEntry(rpm,floppydisk->floppyNumberOfSide);
	currentcylinder=floppydisk->tracks[j];
	for(i=0;i<floppydisk->floppyNumberOfSide;i++)
	{
		hxcfe_imgCallProgressCallback(imgldr_ctx, (j<<1) + (i&1),floppydisk->floppyNumberOfTrack*2);

		file_offset=( (((bootnumberofsector)*256)) * i );
		memset(sectorconfig,0,sizeof(HXCFE_SECTCFG)*10);
		for(k=0;k<10;k++)
		{
			sectorconfig[k].head=i;
			sectorconfig[k].cylinder=j;
			sectorconfig[k].sector=k+1;//+ (numberofsector
			sectorconfig[k].sectorsize=256;
			sectorconfig[k].bitrate=floppydisk->floppyBitRate;
			sectorconfig[k].gap3=255;
			sectorconfig[k].trackencoding=ISOFORMAT_SD;
			sectorconfig[k].input_data=malloc(sectorconfig[k].sectorsize);
			fread(sectorconfig[k].input_data,256,1,f);

		}

		currentcylinder->sides[i]=tg_generateTrackEx(10,(HXCFE_SECTCFG *)&sectorconfig,5,0,floppydisk->floppyBitRate,rpm,ISOFORMAT_SD,0,2500,-2500);

		for(k=0;k<10;k++)
		{
			free(sectorconfig[k].input_data);
		}
	}

	offset=ftell(f);

	floppy_data=malloc(floppydisk->floppySectorPerTrack*sectorsize);

	for(j=1;j<floppydisk->floppyNumberOfTrack;j++)
	{
		floppydisk->tracks[j]=allocCylinderEntry(rpm,floppydisk->floppyNumberOfSide);
		currentcylinder=floppydisk->tracks[j];

		for(i=0;i<floppydisk->floppyNumberOfSide;i++)
		{
			hxcfe_imgCallProgressCallback(imgldr_ctx, (j<<1) + (i&1),floppydisk->floppyNumberOfTrack*2);

			file_offset=offset+ ( (((floppydisk->floppySectorPerTrack)*256)) * floppydisk->floppyNumberOfSide * (j-1) ) +
				                ( (((floppydisk->floppySectorPerTrack)*256)) * i );
			fread(floppy_data,floppydisk->floppySectorPerTrack*sectorsize,1,f);

			memset(sectorconfig,0,sizeof(HXCFE_SECTCFG)*floppydisk->floppySectorPerTrack);
			for(k=0;k<floppydisk->floppySectorPerTrack;k++)
			{
				sectorconfig[k].head=i;
				sectorconfig[k].cylinder=j;
				sectorconfig[k].sector=k+1 + (floppydisk->floppySectorPerTrack * i);
				sectorconfig[k].sectorsize=sectorsize;
				sectorconfig[k].bitrate=floppydisk->floppyBitRate;
				sectorconfig[k].gap3=gap3len;
				sectorconfig[k].trackencoding=trackformat;
				sectorconfig[k].input_data=&floppy_data[k*256];
			}

			currentcylinder->sides[i]=tg_generateTrackEx(floppydisk->floppySectorPerTrack,(HXCFE_SECTCFG *)&sectorconfig,interleave,0,floppydisk->floppyBitRate,rpm,trackformat,0,2500,-2500);
		}
	}

	free(floppy_data);

	imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"track file successfully loaded and encoded!");

	hxc_fclose(f);
	return HXCFE_NOERROR;
}

int VEGASDSK_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{

	static const char plug_id[]="VEGAS6809";
	static const char plug_desc[]="VEGAS6809 image Loader";
	static const char plug_ext[]="veg";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)	VEGASDSK_libIsValidDiskFile,
		(LOADDISKFILE)		VEGASDSK_libLoad_DiskFile,
		(WRITEDISKFILE)		0,
		(GETPLUGININFOS)	VEGASDSK_libGetPluginInfo
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

