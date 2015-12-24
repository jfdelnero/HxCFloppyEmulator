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
// File : ana_loader.c
// Contains: AnaDisk floppy image loader.
//
// Written by:	Jean-François DEL NERO
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

#include "ana_loader.h"
#include "ana_format.h"

#include "libhxcadaptor.h"

int AnaDisk_SanityCheck(HXCFE_IMGLDR * imgldr_ctx,char * imgfile,int32_t * mintrack,int32_t * maxtrack,int32_t * minhead,int32_t * maxhead)
{
	FILE *f;
	int32_t filesize,counted_size;
	AnaDisk_sectorheader sector_header;

	filesize = hxc_getfilesize(imgfile);

	if(mintrack)
		*mintrack = 255;

	if(maxtrack)
		*maxtrack = 0;

	if(minhead)
		*minhead = 255;

	if(maxhead)
		*maxhead = 0;

	if(filesize>0)
	{
		f=hxc_fopen(imgfile,"rb");
		if(f==NULL)
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Can't open %s !",imgfile);
			return -1;
		}

		counted_size = 0;
		do
		{
			fread(&sector_header,sizeof(sector_header),1,f);

			if(mintrack)
			{
				if(*mintrack > sector_header.cylinder )
					*mintrack = sector_header.cylinder;
			}

			if(maxtrack)
			{
				if(*maxtrack < sector_header.cylinder )
					*maxtrack = sector_header.cylinder;
			}

			if(minhead)
			{
				if(*minhead > sector_header.side )
					*minhead = sector_header.side;
			}

			if(maxtrack)
			{
				if(*maxhead < sector_header.side )
					*maxhead = sector_header.side;
			}

			counted_size += sizeof(sector_header);

			counted_size += sector_header.data_len;

			fseek(f,sector_header.data_len,SEEK_CUR);

		}while(counted_size<filesize);

		hxc_fclose(f);

		if(filesize == counted_size)
			return 1; // Seems valid.
		else
			return 0; // Something wrong...
	}

	return -1;
}

int ANA_libIsValidDiskFile(HXCFE_IMGLDR * imgldr_ctx,char * imgfile)
{
	int ret;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"ANA_libIsValidDiskFile");

	if(hxc_checkfileext(imgfile,"ana"))
	{
		ret = AnaDisk_SanityCheck(imgldr_ctx,imgfile,0,0,0,0);

		if( ret > 0 )
			return HXCFE_VALIDFILE;

		if( !ret )
			return HXCFE_BADFILE;

		return HXCFE_ACCESSERROR;
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"ANA_libIsValidDiskFile : non AnaDisk file !");
		return HXCFE_BADFILE;
	}
}

int getnextsector(int32_t track,int32_t side,AnaDisk_sectorheader * sectorheader,FILE * f)
{
	int ret;
	int lastdatapos;

	lastdatapos = 0;

	do
	{
		ret = fread(sectorheader,sizeof(AnaDisk_sectorheader),1,f);
		lastdatapos = ftell(f);
		fseek(f,sectorheader->data_len,SEEK_CUR);
	}while( ret && !feof(f) && ( sectorheader->cylinder != track || sectorheader->side != side) );

	if( !ret || feof(f) )
	{
		return 0;
	}

	fseek(f,lastdatapos,SEEK_SET);
	return 1;
}

int ANA_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	FILE * f;
	unsigned int i,j,k;
	HXCFE_SECTCFG* sectorconfig;
	HXCFE_CYLINDER* currentcylinder;
	HXCFE_SIDE* currentside;
	int32_t pregap;
	AnaDisk_sectorheader sector_header;
	int32_t interleave,tracktype;
	int32_t rpm;
	int32_t mintrack,maxtrack;
	int32_t minhead,maxhead;
	int ret;
	unsigned int sectorfound;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"ANA_libLoad_DiskFile %s",imgfile);

	pregap = 0;

	ret = AnaDisk_SanityCheck(imgldr_ctx,imgfile,&mintrack,&maxtrack,&minhead,&maxhead);

	if( ret > 0 )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Min Track : %d Max Track : %d Min Head : %d Max Head : %d",mintrack,maxtrack,minhead,maxhead);

		f=hxc_fopen(imgfile,"rb");
		if(f==NULL)
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
			return HXCFE_ACCESSERROR;
		}

		floppydisk->floppyNumberOfTrack = maxtrack+1;
		floppydisk->floppyNumberOfSide = maxhead+1;
		floppydisk->floppyBitRate = 250000;
		floppydisk->floppySectorPerTrack = -1;
		floppydisk->floppyiftype = GENERIC_SHUGART_DD_FLOPPYMODE;
		rpm = 300;
		pregap = 0;
		interleave=1;
		tracktype=IBMFORMAT_DD;

		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"AnaDisk File : %d track, %d side, %d bit/s, %d sectors, mode %d",
			floppydisk->floppyNumberOfTrack,
			floppydisk->floppyNumberOfSide,
			floppydisk->floppyBitRate,
			floppydisk->floppySectorPerTrack,
			floppydisk->floppyiftype);

		floppydisk->tracks=(HXCFE_CYLINDER**)malloc(sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);
		memset(floppydisk->tracks,0,sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);

		sectorconfig=(HXCFE_SECTCFG*)malloc(sizeof(HXCFE_SECTCFG)*128);
		memset(sectorconfig,0,sizeof(HXCFE_SECTCFG)*128);

		for(j=0;j<(unsigned int)(floppydisk->floppyNumberOfSide);j++)
		{
			for(i=0;i<(unsigned int)(floppydisk->floppyNumberOfTrack);i++)
			{
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"--- Track %d Side %d ---",i,j);

				sectorfound = 0;

				while(getnextsector(i,j,&sector_header,f))
				{
					imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Side %d Track %d Sector %d Data len %d",sector_header.logical_side,sector_header.logical_cylinder,sector_header.logical_sector,sector_header.data_len);

					memset(&sectorconfig[sectorfound],0,sizeof(HXCFE_SECTCFG));

					if(sector_header.data_len)
					{
						sectorconfig[sectorfound].input_data = malloc(sector_header.data_len);
						fread(sectorconfig[sectorfound].input_data,sector_header.data_len,1,f);
					}

					sectorconfig[sectorfound].trackencoding = IBMFORMAT_DD;
					sectorconfig[sectorfound].bitrate = floppydisk->floppyBitRate;
					sectorconfig[sectorfound].cylinder = sector_header.logical_cylinder;
					sectorconfig[sectorfound].head = sector_header.logical_side;
					sectorconfig[sectorfound].sector = sector_header.logical_sector;
					sectorconfig[sectorfound].sectorsize = sector_header.data_len;
					sectorconfig[sectorfound].use_alternate_sector_size_id = 0xFF;
					sectorconfig[sectorfound].alternate_sector_size_id = sector_header.sector_len_code;
					sectorconfig[sectorfound].gap3=255;
					sectorfound++;
				}

				if(!floppydisk->tracks[i])
				{
					floppydisk->tracks[i] = allocCylinderEntry(rpm,floppydisk->floppyNumberOfSide);
				}

				currentcylinder = floppydisk->tracks[i];

				currentside = tg_generateTrackEx((unsigned short)sectorfound,sectorconfig,interleave,0,floppydisk->floppyBitRate,rpm,tracktype,pregap,2500 | NO_SECTOR_UNDER_INDEX,-2500);
				currentcylinder->sides[j] = currentside;
				currentcylinder->floppyRPM = rpm;

				for(k=0;k<sectorfound;k++)
				{
					if(sectorconfig[k].input_data)
						free(sectorconfig[k].input_data);
				}

				fseek(f,0,SEEK_SET);
			}
		}

		free(sectorconfig);

		hxc_fclose(f);

		hxcfe_sanityCheck(imgldr_ctx->hxcfe,floppydisk);

		return HXCFE_NOERROR;
	}

	return HXCFE_BADFILE;
}

int ANA_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{

	static const char plug_id[]="ANA_IMG";
	static const char plug_desc[]="AnaDisk file Loader";
	static const char plug_ext[]="ana";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)	ANA_libIsValidDiskFile,
		(LOADDISKFILE)		ANA_libLoad_DiskFile,
		(WRITEDISKFILE)		0,
		(GETPLUGININFOS)	ANA_libGetPluginInfo
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
