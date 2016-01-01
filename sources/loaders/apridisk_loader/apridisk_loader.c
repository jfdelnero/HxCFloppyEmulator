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
// File : D88_loader.c
// Contains: D88 floppy image loader.
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

#include "apridisk_loader.h"
#include "apridisk_format.h"

#include "libhxcadaptor.h"

int ApriDisk_libIsValidDiskFile(HXCFE_IMGLDR * imgldr_ctx,char * imgfile)
{
	unsigned char HeaderBuffer[128];
	FILE * f;
	int pathlen;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"ApriDisk_libIsValidDiskFile");

	if(imgfile)
	{
		pathlen=strlen(imgfile);
		if(pathlen!=0)
		{
			f=hxc_fopen(imgfile,"r+b");
			if(f)
			{
				memset(HeaderBuffer,0,sizeof(HeaderBuffer));
				fread(HeaderBuffer,1,128,f);
				hxc_fclose(f);

				if(!strcmp(APRIDISK_HeaderString,(char*)HeaderBuffer))
				{
					imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"ApriDisk_libIsValidDiskFile : ApriDisk file !");
					return HXCFE_VALIDFILE;
				}
				else
				{
					imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"ApriDisk_libIsValidDiskFile : ApriDisk file !");
					return HXCFE_BADFILE;
				}
			}
		}
	}

	return HXCFE_BADPARAMETER;
}



int ApriDisk_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	FILE * f;
	apridisk_data_record * data_record;
	apridisk_compressed_data * compressed_dataitem;
	int i,j;
	HXCFE_SECTCFG* sectorconfig;
	HXCFE_CYLINDER* currentcylinder;
	HXCFE_SIDE* currentside;
	int rpm;
	int interleave;

	int number_of_track,number_of_sector;
	int totalfilesize,k;
	unsigned char * file_buffer;
	int fileindex,newtrack;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"ApriDisk_libLoad_DiskFile %s",imgfile);

	f=hxc_fopen(imgfile,"rb");
	if(f==NULL)
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	fseek(f,0,SEEK_END);
	totalfilesize=ftell(f);
	fseek(f,0,SEEK_SET);
	file_buffer=(unsigned char *) malloc(totalfilesize);
	memset(file_buffer,0,totalfilesize);
	fread(file_buffer,1,totalfilesize,f);

	fileindex=0;
	//////////////////////////////////////////////////////
	// Header check
	if(strcmp(APRIDISK_HeaderString,(char*)&file_buffer[fileindex]))
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"ApriDisk file !");
		return HXCFE_BADFILE;
	}

	number_of_track=0;
	i=0;

	floppydisk->floppyBitRate=500000;
	floppydisk->floppyiftype=GENERIC_SHUGART_DD_FLOPPYMODE;
	floppydisk->floppyNumberOfTrack=0;
	floppydisk->floppyNumberOfSide=0;
	floppydisk->floppySectorPerTrack=-1; // default value

	for(i=0;i<128;i++)
	{
		hxcfe_imgCallProgressCallback(imgldr_ctx,i,128);

		for(j=0;j<=1;j++)
		{
			number_of_sector=0;
			sectorconfig=0;
			fileindex=128;
			rpm=600;
			newtrack=0;
			interleave=1;

			do
			{

				data_record=(apridisk_data_record *)&file_buffer[fileindex];
				fileindex=fileindex+sizeof(apridisk_data_record);

				switch(data_record->item_type)
				{
				case DATA_RECORD_DELETED:
					if(data_record->header_size>sizeof(apridisk_data_record))
					{
						fileindex=fileindex+(data_record->header_size-sizeof(apridisk_data_record));
					}
					fileindex=fileindex+(data_record->data_size);
					break;

				case DATA_RECORD_SECTOR:
					if(data_record->header_size>sizeof(apridisk_data_record))
					{
						fileindex=fileindex+(data_record->header_size-sizeof(apridisk_data_record));
					}
					if((data_record->cylinder==i) && (data_record->head==j))
					{

						if((j+1)>(floppydisk->floppyNumberOfSide))
						{
							floppydisk->floppyNumberOfSide=j+1;
						}

						if((i+1)>(floppydisk->floppyNumberOfTrack))
						{
							floppydisk->floppyNumberOfTrack=i+1;
							newtrack=1;
						}

						data_record->item_type=DATA_RECORD_DELETED;


						sectorconfig=(HXCFE_SECTCFG*)realloc(sectorconfig,sizeof(HXCFE_SECTCFG)*(number_of_sector+1));
						memset(&sectorconfig[number_of_sector],0,sizeof(HXCFE_SECTCFG));

						sectorconfig[number_of_sector].cylinder=i;
						sectorconfig[number_of_sector].head=j;
						sectorconfig[number_of_sector].sector=data_record->sector;
						sectorconfig[number_of_sector].trackencoding=IBMFORMAT_DD;
						sectorconfig[number_of_sector].bitrate=floppydisk->floppyBitRate;
						sectorconfig[number_of_sector].gap3=255;

						switch(data_record->compression)
						{
						case DATA_NOT_COMPRESSED:
							sectorconfig[number_of_sector].sectorsize=data_record->data_size;
							sectorconfig[number_of_sector].input_data=malloc(data_record->data_size);
							memcpy(sectorconfig[number_of_sector].input_data,&file_buffer[fileindex],data_record->data_size);
							fileindex=fileindex+data_record->data_size;
							break;

						case DATA_COMPRESSED:
							compressed_dataitem=(apridisk_compressed_data *)&file_buffer[fileindex];

							sectorconfig[number_of_sector].sectorsize=compressed_dataitem->count;
							sectorconfig[number_of_sector].input_data=malloc(compressed_dataitem->count);

							memset(sectorconfig[number_of_sector].input_data,compressed_dataitem->byte,compressed_dataitem->count);
							fileindex=fileindex+data_record->data_size;

							break;

						default:
							imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Unknow compression id (%.4x) !",data_record->compression);
							sectorconfig[number_of_sector].input_data=0;
							break;
						}

						number_of_sector++;

					}
					else
					{
						fileindex=fileindex+data_record->data_size;
					}

					imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"ApriDisk_libLoad_DiskFile: item DATA_RECORD_SECTOR found. Header size=%d, Data size=%d, Sector=%d Head=%d Cylinder=%d",data_record->header_size,data_record->data_size,data_record->sector,data_record->head,data_record->cylinder);
					break;

				case DATA_RECORD_COMMENT:
					if(data_record->header_size>sizeof(apridisk_data_record))
					{
						fileindex=fileindex+(data_record->header_size-sizeof(apridisk_data_record));
					}
					fileindex=fileindex+data_record->data_size;
					break;

				case DATA_RECORD_CREATOR:

					if(data_record->header_size>sizeof(apridisk_data_record))
					{
						fileindex=fileindex+(data_record->header_size-sizeof(apridisk_data_record));
					}
					fileindex=fileindex+data_record->data_size;
					break;

				default:
					return HXCFE_BADFILE;
					break;

			}

		}while(fileindex<totalfilesize);

		if(newtrack)
		{
			floppydisk->tracks=(HXCFE_CYLINDER**)realloc(floppydisk->tracks,sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);
			floppydisk->tracks[i]=(HXCFE_CYLINDER*)malloc(sizeof(HXCFE_CYLINDER));
			memset(floppydisk->tracks[i],0,sizeof(HXCFE_CYLINDER));
		}

		currentcylinder=floppydisk->tracks[floppydisk->floppyNumberOfTrack-1];
		currentcylinder->number_of_side=floppydisk->floppyNumberOfSide;
		currentcylinder->sides=(HXCFE_SIDE**)realloc(currentcylinder->sides,sizeof(HXCFE_SIDE*)*currentcylinder->number_of_side);
		//memset(currentcylinder->sides,0,sizeof(HXCFE_SIDE*)*currentcylinder->number_of_side);

		currentcylinder->floppyRPM=rpm;

		currentside=tg_generateTrackEx((unsigned short)number_of_sector,sectorconfig,interleave,0,floppydisk->floppyBitRate,rpm,IBMFORMAT_DD,0,2500 | NO_SECTOR_UNDER_INDEX,-2500);
		if(currentcylinder->number_of_side>j)
			currentcylinder->sides[j]=currentside;

		for(k=0;k<number_of_sector;k++)
		{
			free(sectorconfig[k].input_data);
		}

		if(number_of_sector)
				free(sectorconfig);

		}

		number_of_sector=0;
	}


	// Comment & creator extraction.
	fileindex=128;
	do
	{

		data_record=(apridisk_data_record *)&file_buffer[fileindex];
		fileindex=fileindex+sizeof(apridisk_data_record);

		switch(data_record->item_type)
		{
			case DATA_RECORD_DELETED:
				if(data_record->header_size>sizeof(apridisk_data_record))
				{
					fileindex=fileindex+(data_record->header_size-sizeof(apridisk_data_record));
				}
				fileindex=fileindex+(data_record->data_size);
				break;

			case DATA_RECORD_SECTOR:
				if(data_record->header_size>sizeof(apridisk_data_record))
				{
					fileindex=fileindex+(data_record->header_size-sizeof(apridisk_data_record));
				}
				fileindex=fileindex+data_record->data_size;
				break;

			case DATA_RECORD_COMMENT:
				if(data_record->header_size>sizeof(apridisk_data_record))
				{
					fileindex=fileindex+(data_record->header_size-sizeof(apridisk_data_record));
				}
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"ApriDisk_libLoad_DiskFile: item DATA_RECORD_COMMENT found: %s",&file_buffer[fileindex]);
				fileindex=fileindex+data_record->data_size;
				break;

			case DATA_RECORD_CREATOR:
				if(data_record->header_size>sizeof(apridisk_data_record))
				{
					fileindex=fileindex+(data_record->header_size-sizeof(apridisk_data_record));
				}
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"ApriDisk_libLoad_DiskFile: item DATA_RECORD_CREATOR found: %s",&file_buffer[fileindex]);
				fileindex=fileindex+data_record->data_size;
				break;

				default:
				return HXCFE_BADFILE;
				break;

			}

		}while(fileindex<totalfilesize);



	free(file_buffer);

	hxcfe_sanityCheck(imgldr_ctx->hxcfe,floppydisk);

	return HXCFE_NOERROR;
}

int ApriDisk_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{

	static const char plug_id[]="APRIDISK";
	static const char plug_desc[]="APRIDISK Loader";
	static const char plug_ext[]="dsk";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)	ApriDisk_libIsValidDiskFile,
		(LOADDISKFILE)		ApriDisk_libLoad_DiskFile,
		(WRITEDISKFILE)		0,
		(GETPLUGININFOS)	ApriDisk_libGetPluginInfo
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

