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
// File : dim_loader.c
// Contains: DIM floppy image loader
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
#include "libhxcfe.h"
#include "./tracks/track_generator.h"

#include "floppy_loader.h"
#include "floppy_utils.h"

#include "dim_loader.h"
#include "dim_writer.h"

#include "dim_format.h"

#include "libhxcadaptor.h"

int DIM_libIsValidDiskFile(HXCFE_IMGLDR * imgldr_ctx,char * imgfile)
{
	FILE * f;
	dim_header header;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"DIM_libIsValidDiskFile");

	if( hxc_checkfileext(imgfile,"dim") )
	{

		f=hxc_fopen(imgfile,"rb");
		if(f==NULL)
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"DIM_libIsValidDiskFile : Cannot open %s !",imgfile);
			return HXCFE_ACCESSERROR;
		}

		fread(&header,sizeof(dim_header),1,f);

		hxc_fclose(f);

		if(	header.id_header==0x4242)
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"DIM_libIsValidDiskFile : DIM file !");
			return HXCFE_VALIDFILE;
		}
		else
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"DIM_libIsValidDiskFile : non DIM file ! Bad header!");
			return HXCFE_BADFILE;
		}
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"DIM_libIsValidDiskFile : non DIM file !");
		return HXCFE_BADFILE;
	}
}

int DIM_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{

	FILE * f;
	unsigned char  i,j,skew;
	unsigned int   file_offset;
	unsigned char  trackformat;
	unsigned char* trackdata;
	unsigned char  gap3len,interleave;
	unsigned short rpm;
	unsigned short sectorsize;
	HXCFE_CYLINDER* currentcylinder;
	dim_header header;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"DIM_libLoad_DiskFile %s",imgfile);

	f=hxc_fopen(imgfile,"rb");
	if(f==NULL)
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	fread(&header,sizeof(dim_header),1,f);

	if(header.id_header==0x4242)
	{
		sectorsize=(header.sectorsizeh*256)+header.sectorsizel; // st file support only 512bytes/sector floppies.
		if(!sectorsize || (sectorsize&0xFF)) sectorsize=512;
		// read the first sector
		interleave=1;
		gap3len=30;
		floppydisk->floppyNumberOfTrack=header.end_track+1;
		floppydisk->floppyNumberOfSide=header.side+1;
		floppydisk->floppySectorPerTrack=header.nbsector;

		if(!header.density)
		{
			floppydisk->floppyBitRate=DEFAULT_DD_BITRATE;
			floppydisk->floppyiftype=ATARIST_DD_FLOPPYMODE;
			skew=2;
		}
		else
		{
			floppydisk->floppyiftype=ATARIST_HD_FLOPPYMODE;
			floppydisk->floppyBitRate=DEFAULT_HD_BITRATE;
			skew=4;
		}

		trackformat=ISOFORMAT_DD;
		if(floppydisk->floppySectorPerTrack==11)
		{
			gap3len=3;
			trackformat=ISOFORMAT_DD11S;
			interleave=2;
		}

		floppydisk->tracks=(HXCFE_CYLINDER**)malloc(sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);

		rpm=300; // normal rpm

		imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"%d tracks, %d side(s), %d sectors/track, gap3:%d, interleave:%d,rpm:%d bitrate:%d",floppydisk->floppyNumberOfTrack,floppydisk->floppyNumberOfSide,floppydisk->floppySectorPerTrack,gap3len,interleave,rpm,floppydisk->floppyBitRate);

		trackdata=(unsigned char*)malloc(sectorsize*floppydisk->floppySectorPerTrack);

		for(j=0;j<floppydisk->floppyNumberOfTrack;j++)
		{

			floppydisk->tracks[j]=allocCylinderEntry(rpm,floppydisk->floppyNumberOfSide);
			currentcylinder=floppydisk->tracks[j];

			for(i=0;i<floppydisk->floppyNumberOfSide;i++)
			{
				hxcfe_imgCallProgressCallback(imgldr_ctx, (j<<1) + (i&1),floppydisk->floppyNumberOfTrack*2);

				file_offset=(sectorsize*(j*floppydisk->floppySectorPerTrack*floppydisk->floppyNumberOfSide))+
							(sectorsize*(floppydisk->floppySectorPerTrack)*i)+0x20;

				fseek (f , file_offset , SEEK_SET);
				fread(trackdata,sectorsize*floppydisk->floppySectorPerTrack,1,f);

				currentcylinder->sides[i]=tg_generateTrack(trackdata,sectorsize,floppydisk->floppySectorPerTrack,(unsigned char)j,(unsigned char)i,1,interleave,(unsigned char)(((j<<1)|(i&1))*skew),floppydisk->floppyBitRate,currentcylinder->floppyRPM,trackformat,gap3len,0,2500,-2500);
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

int DIM_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{

	static const char plug_id[]="ATARIST_DIM";
	static const char plug_desc[]="ATARI ST DIM Loader";
	static const char plug_ext[]="dim";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)	DIM_libIsValidDiskFile,
		(LOADDISKFILE)		DIM_libLoad_DiskFile,
		(WRITEDISKFILE)		DIM_libWrite_DiskFile,
		(GETPLUGININFOS)	DIM_libGetPluginInfo
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
