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
// File : ensoniq_mirage_loader.c
// Contains: Ensoniq mirage floppy image loader
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

#include "ensoniq_mirage_loader.h"

#include "libhxcadaptor.h"

int Ensoniq_mirage_libIsValidDiskFile( HXCFE_IMGLDR * imgldr_ctx, HXCFE_IMGLDR_FILEINFOS * imgfile )
{
	return hxcfe_imgCheckFileCompatibility( imgldr_ctx, imgfile, "Ensoniq_mirage_libIsValidDiskFile", "edm", ((5*1024) + (1*512)) );
}

int Ensoniq_mirage_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{

	FILE * f;
	unsigned int filesize;
	int i,j,k;
	unsigned int file_offset;
	unsigned char* trackdata;
	int  gap3len,interleave;
	int  trackformat;
	int  rpm;
	int  numberofsector;
	HXCFE_CYLINDER* currentcylinder;
	HXCFE_SECTCFG  sectorconfig[6];

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Ensoniq_mirage_libLoad_DiskFile %s",imgfile);

	f = hxc_fopen(imgfile,"rb");
	if( f == NULL )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	filesize = hxc_fgetsize(f);

	numberofsector=6;

	if(filesize!=0)
	{

		gap3len=30;
		interleave=1;
		trackformat=ISOFORMAT_DD;

		floppydisk->floppyNumberOfSide=1;
		floppydisk->floppySectorPerTrack=6;
		floppydisk->floppyNumberOfTrack=(filesize/(((numberofsector-1)*1024)+512)) / floppydisk->floppyNumberOfSide;

		if(1)
		{

			floppydisk->floppyBitRate=250000;
			floppydisk->floppyiftype=GENERIC_SHUGART_DD_FLOPPYMODE;
			floppydisk->tracks=(HXCFE_CYLINDER**)malloc(sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);

			rpm=300; // normal rpm

			imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"filesize:%dkB, %d tracks, %d side(s), %d sectors/track, gap3:%d, interleave:%d,rpm:%d",filesize/1024,floppydisk->floppyNumberOfTrack,floppydisk->floppyNumberOfSide,floppydisk->floppySectorPerTrack,gap3len,interleave,rpm);

			trackdata=(unsigned char*)malloc(((numberofsector-1)*1024)+512);

			for(j=0;j<floppydisk->floppyNumberOfTrack;j++)
			{
				floppydisk->tracks[j]=allocCylinderEntry(rpm,floppydisk->floppyNumberOfSide);
				currentcylinder=floppydisk->tracks[j];

				for(i=0;i<floppydisk->floppyNumberOfSide;i++)
				{
					hxcfe_imgCallProgressCallback(imgldr_ctx,(j<<1) + (i&1),floppydisk->floppyNumberOfTrack*2 );

					memset(sectorconfig,0,sizeof(HXCFE_SECTCFG)*6);
					for(k=0;k<6;k++)
					{
						sectorconfig[k].head=i;
						sectorconfig[k].cylinder=j;
						sectorconfig[k].sector=k;
						sectorconfig[k].sectorsize=1024;
						sectorconfig[k].bitrate=floppydisk->floppyBitRate;
						sectorconfig[k].gap3=gap3len;
						sectorconfig[k].trackencoding=trackformat;
						sectorconfig[k].input_data=&trackdata[k*1024];
					}

					sectorconfig[numberofsector-1].sectorsize=512;

					file_offset=( (((numberofsector-1)*1024)+512) * floppydisk->floppyNumberOfSide * j ) +
								( (((numberofsector-1)*1024)+512) * i );
					fseek (f , file_offset , SEEK_SET);
					hxc_fread(trackdata,(((numberofsector-1)*1024)+512),f);

					currentcylinder->sides[i]=tg_generateTrackEx(floppydisk->floppySectorPerTrack,sectorconfig,interleave,0,floppydisk->floppyBitRate,rpm,trackformat,0,2500,-2500);
				}
			}

			free(trackdata);

			imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"track file successfully loaded and encoded!");

			hxc_fclose(f);
			return HXCFE_NOERROR;

		}
		hxc_fclose(f);
		return HXCFE_FILECORRUPTED;
	}

	imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"file size=%d !?",filesize);
	hxc_fclose(f);
	return HXCFE_BADFILE;
}

int Ensoniq_mirage_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]="ENSONIQ_EDM";
	static const char plug_desc[]="Ensoniq mirage EDM Loader";
	static const char plug_ext[]="edm";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   Ensoniq_mirage_libIsValidDiskFile,
		(LOADDISKFILE)      Ensoniq_mirage_libLoad_DiskFile,
		(WRITEDISKFILE)     0,
		(GETPLUGININFOS)    Ensoniq_mirage_libGetPluginInfo
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
