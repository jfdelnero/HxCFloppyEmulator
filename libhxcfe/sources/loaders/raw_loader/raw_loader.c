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
// File : raw_loader.c
// Contains: RAW floppy image loader
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

#include "raw_loader.h"

#include "libhxcadaptor.h"

int RAW_libIsValidFormat(HXCFE* floppycontext,cfgrawfile * imgformatcfg)
{
	if(imgformatcfg->rpm == 0)
		return HXCFE_BADPARAMETER;

	return HXCFE_VALIDFILE;
}

int RAW_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,cfgrawfile * imgformatcfg)
{
	FILE * f;
	int i,j,fileside,bitrate;
	unsigned int file_offset;
	unsigned char* trackdata;
	int gap3len,interleave,skew,curskew,tracktype,firstsectorid;
	int sectorsize,rpm;

	trackdata = NULL;
	f = NULL;

	if(imgfile)
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"RAW_libLoad_DiskFile %s",imgfile);

		f = hxc_fopen(imgfile,"rb");
		if( f == NULL )
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
			return HXCFE_ACCESSERROR;
		}
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"RAW_libLoad_DiskFile empty floppy");
	}

	sectorsize=128<<(imgformatcfg->sectorsize&7);

	if(!imgformatcfg->autogap3)
		gap3len=(unsigned char)imgformatcfg->gap3;
	else
		gap3len=255;

	rpm=(unsigned short)imgformatcfg->rpm;
	bitrate=imgformatcfg->bitrate;
	interleave=imgformatcfg->interleave;
	skew=imgformatcfg->skew;

	floppydisk->floppyNumberOfTrack=(unsigned short)imgformatcfg->numberoftrack;

	if((imgformatcfg->sidecfg&TWOSIDESFLOPPY) || (imgformatcfg->sidecfg&SIDE_INVERTED))
	{
		floppydisk->floppyNumberOfSide=2;
	}
	else
	{
		floppydisk->floppyNumberOfSide=1;
	}

	floppydisk->floppySectorPerTrack=imgformatcfg->sectorpertrack;

	floppydisk->floppyBitRate=bitrate;
	floppydisk->floppyiftype=GENERIC_SHUGART_DD_FLOPPYMODE;
	floppydisk->tracks=(HXCFE_CYLINDER**)malloc(sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);
	if(!floppydisk->tracks)
		goto error;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"%d bytes sectors, %d sectors/tracks,interleaving %d, skew %d, %d tracks, %d side(s), gap3 %d, %d rpm, %d bits/s",
		sectorsize,
		floppydisk->floppySectorPerTrack,
		interleave,
		skew,
		floppydisk->floppyNumberOfTrack,
		floppydisk->floppyNumberOfSide,
		gap3len,
		rpm,
		bitrate
		);

	switch(imgformatcfg->tracktype)
	{
		case FM_TRACK_TYPE:
			tracktype=ISOFORMAT_SD;
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"FM ISO tracks format");
		break;

		case FMIBM_TRACK_TYPE:
			tracktype=IBMFORMAT_SD;
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"FM IBM tracks format");
		break;

		case MFM_TRACK_TYPE:
			tracktype=ISOFORMAT_DD;
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"MFM ISO tracks format");
		break;

		case MFMIBM_TRACK_TYPE:
			tracktype=IBMFORMAT_DD;
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"MFM IBM tracks format");
		break;

		case GCR_TRACK_TYPE:
			tracktype=0;
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"GCR tracks format");
		break;
		default:
			tracktype=ISOFORMAT_DD;
		break;

	};

	trackdata = (unsigned char*)malloc(sectorsize*floppydisk->floppySectorPerTrack);
	if(!trackdata)
		goto error;

	for(j=0;j<floppydisk->floppyNumberOfTrack;j++)
	{
		floppydisk->tracks[j] = allocCylinderEntry(rpm,floppydisk->floppyNumberOfSide);

		for(i=0;i<floppydisk->floppyNumberOfSide;i++)
		{
			hxcfe_imgCallProgressCallback(imgldr_ctx,(j<<1) + (i&1),floppydisk->floppyNumberOfTrack*2 );

			if(imgformatcfg->sidecfg&TWOSIDESFLOPPY)
			{
				if(imgformatcfg->sidecfg&SIDE_INVERTED)
				{
					fileside=i^0x1;
				}
				else
				{
					fileside=i;
				}
			}
			else
			{
				fileside=0;
			}

			if(imgformatcfg->sidecfg&SIDE0_FIRST)
			{
				file_offset=(sectorsize*(j*floppydisk->floppySectorPerTrack))+
							(sectorsize*floppydisk->floppySectorPerTrack*floppydisk->floppyNumberOfTrack*fileside);
			}
			else
			{
				file_offset=(sectorsize*(j*floppydisk->floppySectorPerTrack*floppydisk->floppyNumberOfSide))+
							(sectorsize*(floppydisk->floppySectorPerTrack)*fileside);
			}

			if(f)
			{
				fseek (f , file_offset , SEEK_SET);

				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Track %d Head %d : Reading %d bytes at %.8X",j,i,sectorsize*floppydisk->floppySectorPerTrack,file_offset);
				hxc_fread(trackdata,sectorsize*floppydisk->floppySectorPerTrack,f);

			}
			else
			{
				memset(trackdata,imgformatcfg->fillvalue,sectorsize*floppydisk->floppySectorPerTrack);
			}

			firstsectorid=imgformatcfg->firstidsector;
			if(imgformatcfg->intersidesectornumbering)
			{
				if(i)
				{
					firstsectorid=firstsectorid+floppydisk->floppySectorPerTrack;
				}
			}

			if(tracktype)
			{
				if(imgformatcfg->sideskew)
					curskew=(((j<<1)|(i&1))*skew);
				else
					curskew=(j*skew);

				floppydisk->tracks[j]->sides[i] = tg_generateTrack(trackdata,sectorsize,floppydisk->floppySectorPerTrack,(unsigned char)j,(unsigned char)i,(unsigned char)firstsectorid,interleave,curskew,floppydisk->floppyBitRate,rpm,tracktype,gap3len,0,2500|NO_SECTOR_UNDER_INDEX,-2500);
			}
		}
	}

	free(trackdata);

	if(f)
		hxc_fclose(f);

	imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"track file successfully loaded and encoded!");

	return HXCFE_NOERROR;

error:
	imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Alloc / Internal error !",imgfile);

	free(trackdata);

	if(f)
		hxc_fclose(f);

	hxcfe_freeFloppy(imgldr_ctx->hxcfe, floppydisk );

	return HXCFE_INTERNALERROR;
}

int RAW_libWrite_DiskFile(HXCFE_IMGLDR* imgldr_ctx,HXCFE_FLOPPY * floppy,char * filename);

int32_t RAW_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]="RAW_LOADER";
	static const char plug_desc[]="RAW Sector loader";
	static const char plug_ext[]="img";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   0,//RAW_libIsValidDiskFile,
		(LOADDISKFILE)      RAW_libLoad_DiskFile,
		(WRITEDISKFILE)     RAW_libWrite_DiskFile,
		(GETPLUGININFOS)    RAW_libGetPluginInfo
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

