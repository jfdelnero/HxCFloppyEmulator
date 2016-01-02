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
// File : stw_loader.c
// Contains: STW floppy image loader
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

#include "stw_loader.h"
#include "stw_format.h"

#include "libhxcadaptor.h"

int STW_libIsValidDiskFile(HXCFE_IMGLDR * imgldr_ctx,char * imgfile)
{
	FILE *f;
	STWIMG header;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"STW_libIsValidDiskFile");

	if(hxc_checkfileext(imgfile,"stw"))
	{
		f=hxc_fopen(imgfile,"rb");
		if(f==NULL)
			return HXCFE_ACCESSERROR;

		memset(&header,0,sizeof(STWIMG));

		hxc_fread(&header,sizeof(header),f);
		hxc_fclose(f);

		if( !strncmp((char*)header.headername,"STW",3))
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"STW_libIsValidDiskFile : STW file !");
			return HXCFE_VALIDFILE;
		}
		else
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"STW_libIsValidDiskFile : non STW file !");
			return HXCFE_BADFILE;
		}
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"STW_libIsValidDiskFile : non STW file !");
		return HXCFE_BADFILE;
	}
}

int STW_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	FILE * f;
	STWIMG header;
	STWTRACKIMG trackdesc;
	unsigned int i;
	HXCFE_CYLINDER* currentcylinder;
	HXCFE_SIDE* currentside;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"STW_libLoad_DiskFile %s",imgfile);

	f=hxc_fopen(imgfile,"rb");
	if(f==NULL)
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	memset(&header,0,sizeof(STWIMG));
	memset(&trackdesc,0,sizeof(STWTRACKIMG));

	hxc_fread(&header,sizeof(header),f);

	if(!strncmp((char*)header.headername,"STW",3))
	{

		floppydisk->floppyNumberOfTrack = header.number_of_track;
		floppydisk->floppyNumberOfSide = header.number_of_side;
		floppydisk->floppyBitRate = 250000;
		floppydisk->floppySectorPerTrack=-1;
		floppydisk->floppyiftype = ATARIST_DD_FLOPPYMODE;

		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"STW File : %d track, %d side, %d bit/s, %d sectors, mode %d",
			floppydisk->floppyNumberOfTrack,
			floppydisk->floppyNumberOfSide,
			floppydisk->floppyBitRate,
			floppydisk->floppySectorPerTrack,
			floppydisk->floppyiftype);

		floppydisk->tracks=(HXCFE_CYLINDER**)malloc(sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);
		memset(floppydisk->tracks,0,sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);

		for(i=0;i<(unsigned int)(floppydisk->floppyNumberOfTrack*floppydisk->floppyNumberOfSide);i++)
		{

			hxcfe_imgCallProgressCallback(imgldr_ctx,i,(floppydisk->floppyNumberOfTrack*floppydisk->floppyNumberOfSide) );

			fseek(f,sizeof(header)+(i*(sizeof(trackdesc)+(BIGENDIAN_WORD(header.bytes_per_tracks)*2))),SEEK_SET);
			hxc_fread(&trackdesc,sizeof(trackdesc),f);

			if(!floppydisk->tracks[trackdesc.track_number])
			{
				floppydisk->tracks[trackdesc.track_number]=allocCylinderEntry(300,floppydisk->floppyNumberOfSide);
			}

			currentcylinder = floppydisk->tracks[trackdesc.track_number];

			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"read track %d side %d at offset 0x%x (0x%x bytes)",
												trackdesc.track_number,
												trackdesc.side_number,
												ftell(f),
												BIGENDIAN_WORD(header.bytes_per_tracks));

			currentcylinder->sides[trackdesc.side_number] = tg_alloctrack(floppydisk->floppyBitRate,ISOIBM_MFM_ENCODING,currentcylinder->floppyRPM,BIGENDIAN_WORD(header.bytes_per_tracks)*8*2,2500,-2500,0x00);
			currentside=currentcylinder->sides[trackdesc.side_number];
			currentside->number_of_sector=floppydisk->floppySectorPerTrack;

			hxc_fread(currentside->databuffer,currentside->tracklen/8,f);
		}

		hxc_fclose(f);
		return HXCFE_NOERROR;
	}

	hxc_fclose(f);
	imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"bad header");
	return HXCFE_BADFILE;
}

int STW_libWrite_DiskFile(HXCFE_IMGLDR* imgldr_ctx,HXCFE_FLOPPY * floppy,char * filename);

int STW_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]="ATARIST_STW";
	static const char plug_desc[]="STW Loader";
	static const char plug_ext[]="stw";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)	STW_libIsValidDiskFile,
		(LOADDISKFILE)		STW_libLoad_DiskFile,
		(WRITEDISKFILE)		STW_libWrite_DiskFile,
		(GETPLUGININFOS)	STW_libGetPluginInfo
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
