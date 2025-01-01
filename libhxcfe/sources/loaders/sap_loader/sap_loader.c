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
// File : sap_loader.c
// Contains: TO8D SAP floppy image loader
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

#include "thirdpartylibs/libsap/libsap.h"
#include "sap_loader.h"

#include "libhxcadaptor.h"

int SAP_libIsValidDiskFile( HXCFE_IMGLDR * imgldr_ctx, HXCFE_IMGLDR_FILEINFOS * imgfile )
{
	int floppyformat;
	sapID sapid;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"SAP_libIsValidDiskFile");

	if(hxc_checkfileext(imgfile->path,"sap",SYS_PATH_TYPE))
	{
		sapid = sap_OpenArchive(imgfile->path, &floppyformat);
		if(sapid!=SAP_ERROR)
		{
			sap_CloseArchive(sapid);
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"SAP_libIsValidDiskFile : SAP file !");
			return HXCFE_VALIDFILE;
		}
		else
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"SAP_libIsValidDiskFile : non SAP file !");
			return HXCFE_BADFILE;
		}

	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"SAP_libIsValidDiskFile : non SAP file !");
		return HXCFE_BADFILE;
	}
}

int SAP_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	int i,j,k;
	int gap3len,interleave;
	int skew;
	int rpm;
	int sectorsize;

	int trackformat;
	int floppyformat;
	sapID sapid;
	sapsector_t sapsector;
	HXCFE_SECTCFG  sectorconfig[SAP_NSECTS];
	HXCFE_CYLINDER* currentcylinder;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"SAP_libLoad_DiskFile %s",imgfile);

	sapid=sap_OpenArchive(imgfile, &floppyformat);
	if(sapid==SAP_ERROR)
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return -1;
	}

	gap3len = 255;
	interleave = 7;
	skew = 0;

	switch(floppyformat)
	{
		case SAP_FORMAT1:
			sectorsize=SAP_SECTSIZE1;
			floppydisk->floppyNumberOfTrack=SAP_NTRACKS1;
			floppydisk->floppySectorPerTrack=SAP_NSECTS;
			floppydisk->floppyNumberOfSide=1;
			trackformat=ISOFORMAT_DD;
		break;

		case SAP_FORMAT2:
			sectorsize=SAP_SECTSIZE2;
			floppydisk->floppyNumberOfTrack=SAP_NTRACKS2;
			floppydisk->floppySectorPerTrack=SAP_NSECTS;
			floppydisk->floppyNumberOfSide=1;
			trackformat=ISOFORMAT_SD;
		break;

		default:
			imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Unknow floppy format: %d !",floppyformat);
			sap_CloseArchive(sapid);
			return -1;
		break;
	}

	floppydisk->floppyBitRate=250000;
	floppydisk->floppyiftype=GENERIC_SHUGART_DD_FLOPPYMODE;
	floppydisk->tracks=(HXCFE_CYLINDER**)malloc(sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);
	rpm=300; // normal rpm

	imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"%d tracks, %d side(s), %d sectors/track,%d bytes/sector gap3:%d, interleave:%d,rpm:%d",floppydisk->floppyNumberOfTrack,floppydisk->floppyNumberOfSide,floppydisk->floppySectorPerTrack,sectorsize,gap3len,interleave,rpm);

	memset(sectorconfig,0,sizeof(HXCFE_SECTCFG)*SAP_NSECTS);
	for(j=0;j<floppydisk->floppyNumberOfTrack;j++)
	{
		floppydisk->tracks[j]=allocCylinderEntry(rpm,floppydisk->floppyNumberOfSide);
		currentcylinder=floppydisk->tracks[j];

		for(i=0;i<floppydisk->floppyNumberOfSide;i++)
		{
			hxcfe_imgCallProgressCallback(imgldr_ctx,(j<<1) + (i&1),floppydisk->floppyNumberOfTrack*2 );

			for(k=0;k<SAP_NSECTS;k++)
			{
				sap_ReadSector(sapid, j, k+1, &sapsector);
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"[%.2d:%.2d]: Sect %.2d, Track %.2d, Format: 0x%.2x, Protect 0x%.2x",j,k,sapsector.sector,sapsector.track,sapsector.format,sapsector.protection);
				sectorconfig[k].bitrate=250000;
				sectorconfig[k].gap3=255;
				sectorconfig[k].head=0;
				sectorconfig[k].trackencoding=trackformat;
				sectorconfig[k].sector=sapsector.sector;
				sectorconfig[k].cylinder=sapsector.track;
				sectorconfig[k].sectorsize=sectorsize;
				sectorconfig[k].input_data = malloc(sectorconfig[k].sectorsize);
				if( sectorconfig[k].input_data )
					memcpy(sectorconfig[k].input_data,sapsector.data,sectorconfig[k].sectorsize);
			}

			currentcylinder->sides[i]=tg_generateTrackEx(SAP_NSECTS,(HXCFE_SECTCFG *)&sectorconfig,interleave,((j<<1)|(i&1))*skew,floppydisk->floppyBitRate,rpm,trackformat,0,2500|NO_SECTOR_UNDER_INDEX,-2500);

			for(k=0;k<SAP_NSECTS;k++)
			{
				hxcfe_freeSectorConfigData( 0, &sectorconfig[k] );
			}
		}
	}

	sap_CloseArchive(sapid);

	imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"track file successfully loaded and encoded!");

	return HXCFE_NOERROR;
}

int SAP_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]="THOMSONTO8D_SAP";
	static const char plug_desc[]="THOMSON TO8D SAP Loader";
	static const char plug_ext[]="sap";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   SAP_libIsValidDiskFile,
		(LOADDISKFILE)      SAP_libLoad_DiskFile,
		(WRITEDISKFILE)     0,
		(GETPLUGININFOS)    SAP_libGetPluginInfo
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
