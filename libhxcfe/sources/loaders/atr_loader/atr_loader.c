/*
//
// Copyright (C) 2006-2024 Jean-François DEL NERO
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
// File : atr_loader.c
// Contains: ATR Atari floppy image loader
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

#include "atr_loader.h"

#include "atr_format.h"

#include "libhxcadaptor.h"

#include "tracks/luts.h"

void negate_buffer(unsigned char * buffer, int size)
{
	int i;

	for(i=0;i<size;i++)
	{
		buffer[i] = ~buffer[i];
	}
}

int ATR_libIsValidDiskFile( HXCFE_IMGLDR * imgldr_ctx, HXCFE_IMGLDR_FILEINFOS * imgfile )
{
	atr_header * fileheader;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"ATR_libIsValidDiskFile");

	if(hxc_checkfileext(imgfile->path,"atr",SYS_PATH_TYPE))
	{
		fileheader = (atr_header *)imgfile->file_header;

		if( fileheader->sign != 0x0296 && ( (imgfile->file_size - 16) %128 ) )
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"ATR_libIsValidDiskFile : non ATR file !");
			return HXCFE_BADFILE;
		}

		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"ATR_libIsValidDiskFile : ATR file !");
		return HXCFE_VALIDFILE;
	}

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"ATR_libIsValidDiskFile : non ATR file !");
	return HXCFE_BADFILE;
}


int ATR_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{

	FILE * f;
	unsigned int filesize;
	int i,j,k;
	unsigned int file_offset,track_offset;
	unsigned char* trackdata = NULL;
	int trackformat;
	int gap3len,interleave;
	int sectorsize,rpm;
	int bitrate,mixedsectorsize;
	int image_tracks_size;
	int image_track0_size;
	atr_header fileheader;
	unsigned char tmp_sector[256*3];

	HXCFE_SECTCFG* sectorconfig = NULL;
	HXCFE_CYLINDER* currentcylinder = NULL;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"ATR_libLoad_DiskFile %s",imgfile);

	f = hxc_fopen(imgfile,"rb");
	if( f == NULL )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	filesize = hxc_fgetsize(f);

	memset(&fileheader,0,sizeof(fileheader));

	hxc_fread(&fileheader,sizeof(fileheader),f);

	if( fileheader.sign == 0x0296 && !( (filesize - 16) %128 ) )
	{

		bitrate = 250000;
		rpm = 300;
		interleave = 9;
		gap3len = 30;

		sectorsize = fileheader.sector_size;
		mixedsectorsize=0;
		floppydisk->floppyNumberOfSide = 1;
		floppydisk->floppyNumberOfTrack = 40;
		floppydisk->floppySectorPerTrack = 18;
		trackformat=IBMFORMAT_DD;

		image_tracks_size = floppydisk->floppySectorPerTrack * sectorsize;
		image_track0_size = image_tracks_size;

		if( filesize == 183952 || filesize == 368272 )
		{
			interleave = 15;

			trackformat=IBMFORMAT_DD;

			floppydisk->floppyNumberOfSide = 1;

			if(filesize == 368272)
				floppydisk->floppyNumberOfSide = 2;

			mixedsectorsize=1;

			if( sectorsize == 256 )
			{
				if( !((filesize-16) % 128) && ((filesize-16) % 256) )
				{
					//Logical format.
					imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Logical format");
				}
				else
				{
					//Physical or Weird formats.
					imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Physical or Weird formats.");
				}
			}

			image_tracks_size = floppydisk->floppySectorPerTrack * sectorsize;
			image_track0_size = (image_tracks_size - (3*sectorsize)) + (3*128);
		}

		if( filesize < 183952 )
		{
			trackformat=IBMFORMAT_SD;
			floppydisk->floppySectorPerTrack = 26;
			floppydisk->floppyNumberOfTrack = 40;

			image_tracks_size = floppydisk->floppySectorPerTrack * sectorsize;
			image_track0_size = image_tracks_size;
		}

		if( filesize < 131088 )
		{
			trackformat=IBMFORMAT_SD;
			floppydisk->floppySectorPerTrack = 18;
			floppydisk->floppyNumberOfTrack = 40;

			image_tracks_size = floppydisk->floppySectorPerTrack * sectorsize;
			image_track0_size = image_tracks_size;
		}

		floppydisk->floppyBitRate = bitrate;
		floppydisk->floppyiftype = GENERIC_SHUGART_DD_FLOPPYMODE;
		floppydisk->tracks=(HXCFE_CYLINDER**)malloc(sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);
		if( !floppydisk->tracks )
			goto alloc_error;

		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"rpm %d bitrate:%d track:%d side:%d sector:%d",rpm,bitrate,floppydisk->floppyNumberOfTrack,floppydisk->floppyNumberOfSide,floppydisk->floppySectorPerTrack);

		sectorconfig=(HXCFE_SECTCFG*)malloc(sizeof(HXCFE_SECTCFG)*floppydisk->floppySectorPerTrack);
		if( !sectorconfig )
			goto alloc_error;

		memset(sectorconfig,0,sizeof(HXCFE_SECTCFG)*floppydisk->floppySectorPerTrack);

		trackdata = (unsigned char*)malloc(sectorsize*floppydisk->floppySectorPerTrack);
		if( !trackdata )
			goto alloc_error;

		file_offset = 16;
		for(j=0;j<floppydisk->floppyNumberOfTrack;j++)
		{
			floppydisk->tracks[j]=allocCylinderEntry(rpm,floppydisk->floppyNumberOfSide);
			currentcylinder=floppydisk->tracks[j];

			for(i=0;i<floppydisk->floppyNumberOfSide;i++)
			{
				hxcfe_imgCallProgressCallback(imgldr_ctx,(j<<1) + (i&1),floppydisk->floppyNumberOfTrack*2 );

				fseek (f, file_offset , SEEK_SET);

				if( (j == 0) && mixedsectorsize)
					hxc_fread(trackdata,(sectorsize*(floppydisk->floppySectorPerTrack-3)) + 128*3,f);
				else
					hxc_fread(trackdata,sectorsize*floppydisk->floppySectorPerTrack,f);

				track_offset = 0;
				memset(sectorconfig,0,sizeof(HXCFE_SECTCFG)*floppydisk->floppySectorPerTrack);
				memset(tmp_sector,0x00,sizeof(tmp_sector));

				for(k=0;k<floppydisk->floppySectorPerTrack;k++)
				{
					sectorconfig[k].cylinder = j;
					sectorconfig[k].head = i;
					sectorconfig[k].sector = k + 1;
					sectorconfig[k].bitrate = floppydisk->floppyBitRate;
					sectorconfig[k].gap3 = gap3len;

					if( (j == 0) && k < 3 && mixedsectorsize)
					{
						memcpy(&tmp_sector[k*256],&trackdata[track_offset],128);
						sectorconfig[k].input_data=&tmp_sector[k*256];
						sectorconfig[k].sectorsize = 256;
						track_offset += 128;
					}
					else
					{
						sectorconfig[k].input_data=&trackdata[track_offset];
						sectorconfig[k].sectorsize=sectorsize;
						track_offset += sectorsize;
					}

					sectorconfig[k].trackencoding = trackformat;

					negate_buffer(sectorconfig[k].input_data, sectorconfig[k].sectorsize);
				}

				currentcylinder->sides[i]=tg_generateTrackEx(floppydisk->floppySectorPerTrack,sectorconfig,interleave,0,floppydisk->floppyBitRate,rpm,trackformat,0,2500,-2500);

				if(!i && !j)
					file_offset += image_track0_size;
				else
					file_offset += image_tracks_size;
			}
		}

		free(trackdata);
		free(sectorconfig);
		imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"track file successfully loaded and encoded!");

		hxc_fclose(f);
		return HXCFE_NOERROR;
	}

	imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"file size=%d !?",filesize);
	hxc_fclose(f);
	return HXCFE_BADFILE;

alloc_error:

	if ( f )
		hxc_fclose( f );

	free( floppydisk->tracks );
	free( trackdata );
	free( sectorconfig );

	hxcfe_freeFloppy(imgldr_ctx->hxcfe, floppydisk );

	return HXCFE_INTERNALERROR;
}


int ATR_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]="ATARI_ATR";
	static const char plug_desc[]="Atari ATR Loader";
	static const char plug_ext[]="atr";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   ATR_libIsValidDiskFile,
		(LOADDISKFILE)      ATR_libLoad_DiskFile,
		(WRITEDISKFILE)     0,
		(GETPLUGININFOS)    ATR_libGetPluginInfo
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
