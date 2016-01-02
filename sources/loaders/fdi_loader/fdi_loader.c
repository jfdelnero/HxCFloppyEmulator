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
// File : fdi_loader.c
// Contains: FDI floppy image loader
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

#include "fdi_loader.h"
#include "fdi_format.h"

#include "libhxcadaptor.h"

int FDI_libIsValidDiskFile(HXCFE_IMGLDR * imgldr_ctx,char * imgfile)
{
	FILE *f;
	fdi_header f_header;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"FDI_libIsValidDiskFile");

	if( hxc_checkfileext(imgfile,"fdi") )
	{

		f=hxc_fopen(imgfile,"rb");
		if(f)
		{
			hxc_fread(&f_header,sizeof(fdi_header),f);
			hxc_fclose(f);

			if(f_header.signature[0]=='F' && f_header.signature[1]=='D' && f_header.signature[2]=='I')
			{
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"FDI_libIsValidDiskFile : FDI file !");
				return HXCFE_VALIDFILE;
			}

			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"FDI_libIsValidDiskFile : non FDI file !");
			return HXCFE_BADFILE;
		}

		return HXCFE_ACCESSERROR;
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"FDI_libIsValidDiskFile : non FDI file !");
		return HXCFE_BADFILE;
	}
}



int FDI_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{

	FILE * f;
	int  i,j,k,file_size;
	int interleave;
	int rpm;
	int number_of_track,number_of_side,number_of_sectorpertrack;
	unsigned char tempsector[256];
	int trackformat;
	int skew;
	int trackoffset,tempoffset,file_offset;
	HXCFE_SECTCFG* sectorconfig;
	HXCFE_CYLINDER* currentcylinder;

	fdi_header f_header;
	fdi_track_header track_header;
	fdi_sector_header sector_header;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"FDI_libLoad_DiskFile %s",imgfile);

	f=hxc_fopen(imgfile,"rb");
	if(f==NULL)
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}


	fseek(f,0,SEEK_END);
	file_size=ftell(f);
	fseek(f,0,SEEK_SET);

	hxc_fread(&f_header,sizeof(fdi_header),f);

	if(f_header.signature[0]!='F' || f_header.signature[1]!='D' || f_header.signature[2]!='I')
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Bad FDI file !");
		hxc_fclose(f);
		return HXCFE_BADFILE;
	}

	fseek(f,f_header.diskdescription_offset,SEEK_SET);
	hxc_fread(tempsector,f_header.data_offset - f_header.diskdescription_offset,f);
	imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"Disk:%s",tempsector);


	trackoffset=f_header.additionnal_infos_len+0xE;
	fseek(f,trackoffset,SEEK_SET);

	number_of_track=f_header.number_of_cylinders;
	number_of_side =f_header.number_of_heads;
	number_of_sectorpertrack=-1;

	rpm=300;
	interleave=1;
	skew=0;

	floppydisk->floppyBitRate=250000;
	floppydisk->floppyiftype=GENERIC_SHUGART_DD_FLOPPYMODE;
	floppydisk->floppyNumberOfTrack=number_of_track;
	floppydisk->floppyNumberOfSide=number_of_side;
	floppydisk->floppySectorPerTrack=number_of_sectorpertrack;
	floppydisk->tracks=(HXCFE_CYLINDER**)malloc(sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);
	trackformat=IBMFORMAT_DD;
	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"rpm %d bitrate:%d track:%d side:%d sector:%d",rpm,floppydisk->floppyBitRate,floppydisk->floppyNumberOfTrack,floppydisk->floppyNumberOfSide,floppydisk->floppySectorPerTrack);


	for(j=0;j<floppydisk->floppyNumberOfTrack;j++)
	{
		floppydisk->tracks[j]=allocCylinderEntry(rpm,floppydisk->floppyNumberOfSide);
		currentcylinder=floppydisk->tracks[j];

		for(i=0;i<floppydisk->floppyNumberOfSide;i++)
		{
			hxcfe_imgCallProgressCallback(imgldr_ctx, (j<<1) + (i&1),floppydisk->floppyNumberOfTrack*2);

			hxc_fread(&track_header,sizeof(fdi_track_header),f);
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"[%d:%d] %d sectors, Track Offset :0x%x:",j,i,track_header.number_of_sectors,track_header.track_offset+f_header.data_offset);

			sectorconfig=(HXCFE_SECTCFG*)malloc(sizeof(HXCFE_SECTCFG)*track_header.number_of_sectors);
			memset(sectorconfig,0,sizeof(HXCFE_SECTCFG)*track_header.number_of_sectors);

			for(k=0;k<track_header.number_of_sectors;k++)
			{
				hxc_fread(&sector_header,sizeof(fdi_sector_header),f);

				file_offset=f_header.data_offset+track_header.track_offset+sector_header.sector_offset;

				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"[%d:%d] Cyl:%d,Head:%d,Sec:%d,Size:%d,Flags:0x%.2X,Offset:0x%.8x",
					j,i,sector_header.cylinder_number,
					sector_header.head_number,
					sector_header.sector_number,
					128<<sector_header.sector_size,
					sector_header.flags,
					file_offset
					);

				tempoffset=ftell(f);

				sectorconfig[k].cylinder=sector_header.cylinder_number;
				sectorconfig[k].head=sector_header.head_number;
				sectorconfig[k].sector=sector_header.sector_number;
				sectorconfig[k].sectorsize=128<<sector_header.sector_size;
				sectorconfig[k].gap3=255;
				sectorconfig[k].fill_byte=246;
				sectorconfig[k].bitrate=floppydisk->floppyBitRate;
				sectorconfig[k].trackencoding=trackformat;

				if(!(sector_header.flags&0x1F))
				{
					sectorconfig[k].use_alternate_data_crc=0xFF;
					sectorconfig[k].data_crc=0xAAAA;
				}

				if(sector_header.flags&0x80)
				{
					sectorconfig[k].alternate_datamark=0xF8;
					sectorconfig[k].use_alternate_datamark=1;
				}

				if(file_offset<file_size)
				{
					sectorconfig[k].input_data=malloc(sectorconfig[k].sectorsize);
					fseek(f,file_offset,SEEK_SET);
					hxc_fread(sectorconfig[k].input_data,sectorconfig[k].sectorsize,f);
				}

				fseek(f,tempoffset,SEEK_SET);

			}

			currentcylinder->sides[i]=tg_generateTrackEx(track_header.number_of_sectors,sectorconfig,interleave,(unsigned char)(((j<<1)|(i&1))*skew),floppydisk->floppyBitRate,rpm,trackformat,0,2500|NO_SECTOR_UNDER_INDEX,-2500);

			for(k=0;k<track_header.number_of_sectors;k++)
			{
				free(sectorconfig[k].input_data);
			}
			free(sectorconfig);
		}
	}


	imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"track file successfully loaded and encoded!");

	hxc_fclose(f);

	hxcfe_sanityCheck(imgldr_ctx->hxcfe,floppydisk);

	return HXCFE_NOERROR;
}


int FDI_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{

	static const char plug_id[]="ZXSPECTRUM_FDI";
	static const char plug_desc[]="ZX SPECTRUM FDI Loader";
	static const char plug_ext[]="fdi";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)	FDI_libIsValidDiskFile,
		(LOADDISKFILE)		FDI_libLoad_DiskFile,
		(WRITEDISKFILE)		0,
		(GETPLUGININFOS)	FDI_libGetPluginInfo
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

