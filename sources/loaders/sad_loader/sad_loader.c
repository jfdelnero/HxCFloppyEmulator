/*
//
// Copyright (C) 2006-2016 Jean-François DEL NERO
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
// File : sad_loader.c
// Contains: SAD floppy image loader
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

#include "sad_loader.h"
#include "sad_fileformat.h"

#include "libhxcadaptor.h"

int SAD_libIsValidDiskFile(HXCFE_IMGLDR * imgldr_ctx,char * imgfile)
{
	int pathlen;
	FILE * f;
	SAD_HEADER sadh;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"SAD_libIsValidDiskFile");
	if(imgfile)
	{
		pathlen=strlen(imgfile);
		if(pathlen!=0)
		{

			f=hxc_fopen(imgfile,"rb");
			if(f==NULL)
			{
				imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"SAD_libIsValidDiskFile : Cannot open %s !",imgfile);
				return HXCFE_ACCESSERROR;
			}

			memset(&sadh,0,sizeof(SAD_HEADER));
			hxc_fread(&sadh,sizeof(SAD_HEADER),f);

			hxc_fclose(f);

			if(!strncmp((char*)sadh.abSignature,SAD_SIGNATURE,sizeof SAD_SIGNATURE - 1))
			{
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"SAD_libIsValidDiskFile : SAD file !");
				return HXCFE_VALIDFILE;
			}
			else
			{
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"SAD_libIsValidDiskFile : non SAD file !");
				return HXCFE_BADFILE;
			}

		}
	}

	return HXCFE_BADPARAMETER;
}



int SAD_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	FILE * f;
	int i,j;
	unsigned int file_offset;
	unsigned char* trackdata;
	int gap3len,interleave;
	int rpm;
	int sectorsize;
	int trackformat,skew;
	HXCFE_CYLINDER* currentcylinder;
	SAD_HEADER sadh;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"SAD_libLoad_DiskFile %s",imgfile);

	f=hxc_fopen(imgfile,"rb");
	if(f==NULL)
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"SAD_libLoad_DiskFile : Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	memset(&sadh,0,sizeof(SAD_HEADER));
	hxc_fread(&sadh,sizeof(SAD_HEADER),f);
	if(strncmp((char*)sadh.abSignature,SAD_SIGNATURE,sizeof SAD_SIGNATURE - 1))
	{
		hxc_fclose(f);
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"SAD_libLoad_DiskFile : non SAD file !");
		return HXCFE_BADFILE;
	}

	floppydisk->floppyNumberOfTrack=sadh.bTracks;
	floppydisk->floppyNumberOfSide=sadh.bSides;
	floppydisk->floppySectorPerTrack=sadh.bSectors;
	sectorsize=sadh.bSectorSizeDiv64*64;
	gap3len=35;
	interleave=1;
	skew=0;
	trackformat=ISOFORMAT_DD;

	floppydisk->floppyBitRate=250000;
	floppydisk->floppyiftype=GENERIC_SHUGART_DD_FLOPPYMODE;
	floppydisk->tracks=(HXCFE_CYLINDER**)malloc(sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);

	rpm=300; // normal rpm

	imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"SAD_libLoad_DiskFile : %d tracks, %d side(s), %d sectors/track, gap3:%d, interleave:%d,rpm:%d",floppydisk->floppyNumberOfTrack,floppydisk->floppyNumberOfSide,floppydisk->floppySectorPerTrack,gap3len,interleave,rpm);

	trackdata=(unsigned char*)malloc(sectorsize*floppydisk->floppySectorPerTrack);

	for(j=0;j<floppydisk->floppyNumberOfTrack;j++)
	{
		floppydisk->tracks[j]=allocCylinderEntry(rpm,floppydisk->floppyNumberOfSide);
		currentcylinder=floppydisk->tracks[j];

		for(i=0;i<floppydisk->floppyNumberOfSide;i++)
		{
			hxcfe_imgCallProgressCallback(imgldr_ctx,(j<<1) + (i&1),floppydisk->floppyNumberOfTrack*2 );

			file_offset=(sectorsize*(j*floppydisk->floppySectorPerTrack))+
				(sectorsize*(floppydisk->floppyNumberOfTrack*floppydisk->floppySectorPerTrack)*i) + sizeof(SAD_HEADER) ;
			fseek (f , file_offset , SEEK_SET);
			hxc_fread(trackdata,sectorsize*floppydisk->floppySectorPerTrack,f);

			currentcylinder->sides[i]=tg_generateTrack(trackdata,sectorsize,floppydisk->floppySectorPerTrack,(unsigned char)j,(unsigned char)i,1,interleave,(unsigned char)(((j<<1)|(i&1))*skew),floppydisk->floppyBitRate,currentcylinder->floppyRPM,trackformat,gap3len,0,2500|NO_SECTOR_UNDER_INDEX,-2500);
		}
	}

	free(trackdata);

	imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"SAD_libLoad_DiskFile : track file successfully loaded and encoded!");

	hxc_fclose(f);
	return HXCFE_NOERROR;
}

int SAD_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{

	static const char plug_id[]="SAMCOUPE_SAD";
	static const char plug_desc[]="SAM COUPE SAD Loader";
	static const char plug_ext[]="sad";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)	SAD_libIsValidDiskFile,
		(LOADDISKFILE)		SAD_libLoad_DiskFile,
		(WRITEDISKFILE)		0,
		(GETPLUGININFOS)	SAD_libGetPluginInfo
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

