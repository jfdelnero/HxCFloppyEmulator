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
// File : arburg_raw_loader.c
// Contains: Arburg floppy image loader
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

#include "tracks/arburg_track.h"
#include "arburg_raw_loader.h"
#include "arburg_raw_writer.h"

#include "libhxcadaptor.h"

int ARBURG_RAW_libIsValidDiskFile(HXCFE_IMGLDR * imgldr_ctx,char * imgfile)
{
	int filesize;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"ARBURG_RAW_libIsValidDiskFile");

	if( hxc_checkfileext(imgfile,"arburgfd") )
	{

		filesize=hxc_getfilesize(imgfile);
		if(filesize<0)
			return HXCFE_ACCESSERROR;

		if(filesize==(ARBURB_DATATRACK_SIZE*2*80))
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"ARBURG_RAW_libIsValidDiskFile : Arburg Data raw file !");
			return HXCFE_VALIDFILE;
		}
		else
		{

			if(filesize==( (ARBURB_DATATRACK_SIZE*10)+(ARBURB_SYSTEMTRACK_SIZE*70)+(ARBURB_SYSTEMTRACK_SIZE*80) ))
			{
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"ARBURG_RAW_libIsValidDiskFile : Arburg System raw file !");
				return HXCFE_VALIDFILE;
			}
			else
			{
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"ARBURG_RAW_libIsValidDiskFile : non Arburg raw file !");
				return HXCFE_BADFILE;
			}
		}
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"ARBURG_RAW_libIsValidDiskFile : non Arburg raw file !");
		return HXCFE_BADFILE;
	}
}

int ARBURG_RAW_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	FILE * f;
	unsigned short i;
	int filesize;
	HXCFE_CYLINDER*      currentcylinder;
	HXCFE_SIDE*          currentside;
	unsigned char  sector_data[ARBURB_SYSTEMTRACK_SIZE + 2];
	unsigned short tracknumber,sidenumber;
	int systemdisk;
	int fileoffset,blocksize;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"ARBURG_RAW_libLoad_DiskFile %s",imgfile);

	filesize=hxc_getfilesize(imgfile);

	if(filesize<0)
		return HXCFE_ACCESSERROR;

	systemdisk = 0;

	if(filesize==( (ARBURB_DATATRACK_SIZE*10)+(ARBURB_SYSTEMTRACK_SIZE*70)+(ARBURB_SYSTEMTRACK_SIZE*80) ))
	{
		systemdisk = 1;
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"ARBURG_RAW_libLoad_DiskFile : Arburg Data raw file !");
	}


	f=hxc_fopen(imgfile,"rb");
	if(f==NULL)
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	floppydisk->floppyNumberOfTrack=80;
	floppydisk->floppyNumberOfSide=2;
	floppydisk->floppyBitRate=250000;
	floppydisk->floppySectorPerTrack=1;
	floppydisk->floppyiftype=GENERIC_SHUGART_DD_FLOPPYMODE;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Arburg File : %d track, %d side, %d bit/s, %d sectors, mode %d",
		floppydisk->floppyNumberOfTrack,
		floppydisk->floppyNumberOfSide,
		floppydisk->floppyBitRate,
		floppydisk->floppySectorPerTrack,
		floppydisk->floppyiftype);


	floppydisk->tracks=(HXCFE_CYLINDER**)malloc(sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);
	memset(floppydisk->tracks,0,sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);

	for(i=0;i<floppydisk->floppyNumberOfTrack*floppydisk->floppyNumberOfSide;i++)
	{
		hxcfe_imgCallProgressCallback(imgldr_ctx, i,floppydisk->floppyNumberOfTrack*2);

		tracknumber = i % floppydisk->floppyNumberOfTrack;
		if(i>=floppydisk->floppyNumberOfTrack)
			sidenumber = 1;
		else
			sidenumber = 0;

		if(i<10 || !systemdisk )
		{
			blocksize = ARBURB_DATATRACK_SIZE;
			fileoffset = i * blocksize;
		}
		else
		{
			blocksize = ARBURB_SYSTEMTRACK_SIZE;
			fileoffset = (10*ARBURB_DATATRACK_SIZE) + ((i-10)*blocksize);
		}

		fseek(f,fileoffset,SEEK_SET);
		hxc_fread(&sector_data,blocksize,f);

		if(!floppydisk->tracks[tracknumber])
		{
			floppydisk->tracks[tracknumber]=allocCylinderEntry(300,floppydisk->floppyNumberOfSide);
		}

		currentcylinder=floppydisk->tracks[tracknumber];


		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"read track %d side %d at offset 0x%x (0x%x bytes)",
			tracknumber,
			sidenumber,
			fileoffset,
			blocksize);

		currentcylinder->sides[sidenumber]=tg_alloctrack(floppydisk->floppyBitRate,ARBURGDAT_ENCODING,currentcylinder->floppyRPM,256*49 * 8,2000,-2000,0x00);
		currentside=currentcylinder->sides[sidenumber];
		currentside->number_of_sector=floppydisk->floppySectorPerTrack;

		if(i<10 || !systemdisk )
		{
			BuildArburgTrack(imgldr_ctx->hxcfe,tracknumber,sidenumber,sector_data,currentside->databuffer,&currentside->tracklen,2);
			currentside->track_encoding = ARBURGDAT_ENCODING;
		}
		else
		{
			BuildArburgSysTrack(imgldr_ctx->hxcfe,tracknumber,sidenumber,sector_data,currentside->databuffer,&currentside->tracklen,2);
			currentside->track_encoding = ARBURGSYS_ENCODING;
		}

	}

	hxc_fclose(f);
	return HXCFE_NOERROR;
}

int ARBURG_RAW_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{

	static const char plug_id[]="ARBURG";
	static const char plug_desc[]="ARBURG RAW Loader";
	static const char plug_ext[]="arburgfd";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)	ARBURG_RAW_libIsValidDiskFile,
		(LOADDISKFILE)		ARBURG_RAW_libLoad_DiskFile,
		(WRITEDISKFILE)		ARBURG_RAW_libWrite_DiskFile,
		(GETPLUGININFOS)	ARBURG_RAW_libGetPluginInfo
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

