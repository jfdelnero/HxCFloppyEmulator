/*
//
// Copyright (C) 2006-2018 Jean-François DEL NERO
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
// File : sdu_loader.c
// Contains: SAB Diskette Utility / SDU floppy image loader
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

#include "sdu_loader.h"

#include "sdu_format.h"

#include "libhxcadaptor.h"

int SDU_libIsValidDiskFile(HXCFE_IMGLDR * imgldr_ctx,char * imgfile)
{
	int filesize;
    sdu_header sduh;
    FILE *f;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"SDU_libIsValidDiskFile");

	if( hxc_checkfileext(imgfile,"sdu") )
	{
		filesize = hxc_getfilesize(imgfile);
		if(filesize<0)
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"SDU_libIsValidDiskFile : Cannot open %s !",imgfile);
			return HXCFE_ACCESSERROR;
		}

		f = hxc_fopen(imgfile,"rb");
		if(f == NULL)
			return HXCFE_ACCESSERROR;

        memset(&sduh, 0, sizeof(sduh));

		hxc_fread(&sduh,sizeof(sduh),f);
		hxc_fclose(f);

		if( !strncmp((const char*)&sduh.signature,"SAB ",4))
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"SDU_libIsValidDiskFile : SDU file !");
			return HXCFE_VALIDFILE;
		}
		else
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"SDU_libIsValidDiskFile : non SDU file !");
			return HXCFE_BADFILE;
		}
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"SDU_libIsValidDiskFile : non SDU file !");
		return HXCFE_BADFILE;
	}
}

int SDU_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{

	FILE * f;
	unsigned int filesize;
    sdu_header sduh;
	int i,j;
	unsigned int file_offset;
	unsigned char* trackdata;
	int32_t gap3len,interleave;
	int32_t rpm;
	int32_t sectorsize;
	unsigned char trackformat;
	HXCFE_CYLINDER* currentcylinder;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"SDU_libLoad_DiskFile %s",imgfile);

	f = hxc_fopen(imgfile,"rb");
	if( f == NULL )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	filesize = hxc_fgetsize(f);

	if(filesize!=0)
	{
        memset(&sduh, 0, sizeof(sduh));

		hxc_fread(&sduh,sizeof(sduh),f);

		if( strncmp((const char*)&sduh.signature,"SAB ",4))
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"SDU_libLoad_DiskFile : Bad file !");
			return HXCFE_BADFILE;
		}


		rpm = 300;
		gap3len = 30;
		interleave = 1;

        floppydisk->floppyNumberOfTrack = sduh.max_cylinder;
        floppydisk->floppyNumberOfSide  = sduh.max_head;
        floppydisk->floppySectorPerTrack = sduh.max_sector;
		sectorsize = sduh.sector_size;

		if( sectorsize *  floppydisk->floppySectorPerTrack > (512*11) )
		{
			floppydisk->floppyBitRate = 500000;
		}
		else
		{
			floppydisk->floppyBitRate = 250000;
		}

		floppydisk->tracks=(HXCFE_CYLINDER**)malloc(sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);

		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"rpm %d bitrate:%d track:%d side:%d sector:%d sectorsize: %d",rpm,floppydisk->floppyBitRate,floppydisk->floppyNumberOfTrack,floppydisk->floppyNumberOfSide,floppydisk->floppySectorPerTrack,sectorsize);

		trackformat = IBMFORMAT_DD;

		trackdata = (unsigned char*)malloc(sectorsize*floppydisk->floppySectorPerTrack);
        if( trackdata )
        {
		    for(j=0;j<floppydisk->floppyNumberOfTrack;j++)
		    {
			    floppydisk->tracks[j] = allocCylinderEntry(rpm,floppydisk->floppyNumberOfSide);
			    currentcylinder = floppydisk->tracks[j];

			    for(i=0;i<floppydisk->floppyNumberOfSide;i++)
			    {
				    hxcfe_imgCallProgressCallback(imgldr_ctx,(j<<1) | (i&1),floppydisk->floppyNumberOfTrack*2 );

				    file_offset = sizeof(sdu_header) + (sectorsize*(j*floppydisk->floppySectorPerTrack*floppydisk->floppyNumberOfSide))+
							    (sectorsize*(floppydisk->floppySectorPerTrack)*i);

				    fseek (f , file_offset , SEEK_SET);

					memset(trackdata, 0, sectorsize*floppydisk->floppySectorPerTrack);
				    hxc_fread(trackdata,sectorsize*floppydisk->floppySectorPerTrack,f);

				    currentcylinder->sides[i] = tg_generateTrack(trackdata,sectorsize,floppydisk->floppySectorPerTrack,(unsigned char)j,(unsigned char)i,1,interleave,(unsigned char)(0),floppydisk->floppyBitRate,currentcylinder->floppyRPM,trackformat,gap3len,0,2500,-2500);
			    }
		    }

    		free(trackdata);

			imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"track file successfully loaded and encoded!");
		}
		else
		{
			hxc_fclose(f);
			return HXCFE_INTERNALERROR;
		}

		hxc_fclose(f);
		return HXCFE_NOERROR;
	}

	imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"file size=%d !?",filesize);
	hxc_fclose(f);
	return HXCFE_BADFILE;
}

int SDU_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{

	static const char plug_id[]="SDU_IMG";
	static const char plug_desc[]="SAB Diskette Utility Loader";
	static const char plug_ext[]="sdu";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)	SDU_libIsValidDiskFile,
		(LOADDISKFILE)		SDU_libLoad_DiskFile,
		(WRITEDISKFILE)		0,
		(GETPLUGININFOS)	SDU_libGetPluginInfo
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
