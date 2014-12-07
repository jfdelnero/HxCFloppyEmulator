/*
//
// Copyright (C) 2006-2014 Jean-François DEL NERO
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
// File : jv1_loader.c
// Contains: JV1 TRS80 floppy image loader
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

#include "jv1_loader.h"

#include "libhxcadaptor.h"

int JV1_libIsValidDiskFile(HXCFE_IMGLDR * imgldr_ctx,char * imgfile)
{
	int filesize;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"JV1_libIsValidDiskFile");

	if( hxc_checkfileext(imgfile,"jv1") || hxc_checkfileext(imgfile,"dsk"))
	{

		if(hxc_checkfileext(imgfile,"dsk"))
		{
			filesize=hxc_getfilesize(imgfile);
			if(filesize<0)
			{
				imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"JV1_libIsValidDiskFile : Cannot open %s !",imgfile);
				return HXCFE_ACCESSERROR;
			}

			if( filesize%(10*1*256) || ( (filesize/(10*1*256)) > 120 ) )
			{
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"JV1_libIsValidDiskFile : non JV1 file !");
				return HXCFE_BADFILE;
			}
		}

		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"JV1_libIsValidDiskFile : JV1 file !");
		return HXCFE_VALIDFILE;
	}

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"JV1_libIsValidDiskFile : non JV1 file !");
	return HXCFE_BADFILE;
}



int JV1_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{

	FILE * f;
	unsigned int filesize;
	int i,j,k;
	unsigned int file_offset;
	unsigned char* trackdata;
	int trackformat;
	int gap3len,interleave;
	int sectorsize,rpm;
	int bitrate;

	HXCFE_SECTCFG* sectorconfig;
	HXCFE_CYLINDER* currentcylinder;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"JV1_libLoad_DiskFile %s",imgfile);

	f=hxc_fopen(imgfile,"rb");
	if(f==NULL)
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	fseek (f , 0 , SEEK_END);
	filesize=ftell(f);
	fseek (f , 0 , SEEK_SET);

	if(filesize!=0)
	{

			sectorsize=256; // JV1 file support only 256bytes/sector floppies.

            bitrate=250000;
            rpm=300;
            interleave=3;
            gap3len=15;
			trackformat=IBMFORMAT_SD;
            floppydisk->floppyNumberOfSide=1;
            floppydisk->floppySectorPerTrack=10;
            floppydisk->floppyNumberOfTrack=filesize/( floppydisk->floppyNumberOfSide*floppydisk->floppySectorPerTrack*256);

			floppydisk->floppyBitRate=bitrate;
			floppydisk->floppyiftype=GENERIC_SHUGART_DD_FLOPPYMODE;
			floppydisk->tracks=(HXCFE_CYLINDER**)malloc(sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);

			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"rpm %d bitrate:%d track:%d side:%d sector:%d",rpm,bitrate,floppydisk->floppyNumberOfTrack,floppydisk->floppyNumberOfSide,floppydisk->floppySectorPerTrack);

			sectorconfig=(HXCFE_SECTCFG*)malloc(sizeof(HXCFE_SECTCFG)*floppydisk->floppySectorPerTrack);
			memset(sectorconfig,0,sizeof(HXCFE_SECTCFG)*floppydisk->floppySectorPerTrack);

			trackdata=(unsigned char*)malloc(sectorsize*floppydisk->floppySectorPerTrack);

			for(j=0;j<floppydisk->floppyNumberOfTrack;j++)
			{

				floppydisk->tracks[j]=allocCylinderEntry(rpm,floppydisk->floppyNumberOfSide);
				currentcylinder=floppydisk->tracks[j];

				for(i=0;i<floppydisk->floppyNumberOfSide;i++)
				{
					hxcfe_imgCallProgressCallback(imgldr_ctx,(j<<1) + (i&1),floppydisk->floppyNumberOfTrack*2 );

					file_offset=(sectorsize*(j*floppydisk->floppySectorPerTrack*floppydisk->floppyNumberOfSide))+
						(sectorsize*(floppydisk->floppySectorPerTrack)*i);

					fseek (f , file_offset , SEEK_SET);
					fread(trackdata,sectorsize*floppydisk->floppySectorPerTrack,1,f);

					memset(sectorconfig,0,sizeof(HXCFE_SECTCFG)*floppydisk->floppySectorPerTrack);
					for(k=0;k<floppydisk->floppySectorPerTrack;k++)
					{
						sectorconfig[k].cylinder=j;
						sectorconfig[k].head=i;
						sectorconfig[k].sector=k;
						if(j==17)
						{
							sectorconfig[k].use_alternate_datamark=1;
							sectorconfig[k].alternate_datamark=0xFA;
						}
						sectorconfig[k].bitrate=floppydisk->floppyBitRate;
						sectorconfig[k].gap3=gap3len;
						sectorconfig[k].sectorsize=sectorsize;
						sectorconfig[k].input_data=&trackdata[k*sectorsize];
						sectorconfig[k].trackencoding=trackformat;
					}

					currentcylinder->sides[i]=tg_generateTrackEx(floppydisk->floppySectorPerTrack,sectorconfig,interleave,0,floppydisk->floppyBitRate,rpm,trackformat,0,2500,-2500);
				}
			}

			free(sectorconfig);
			imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"track file successfully loaded and encoded!");

			hxc_fclose(f);
			return HXCFE_NOERROR;
	}

	imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"file size=%d !?",filesize);
	hxc_fclose(f);
	return HXCFE_BADFILE;
}


int JV1_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{

	static const char plug_id[]="TRS80_JV1";
	static const char plug_desc[]="TRS80 JV1 Loader";
	static const char plug_ext[]="jv1";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)	JV1_libIsValidDiskFile,
		(LOADDISKFILE)		JV1_libLoad_DiskFile,
		(WRITEDISKFILE)		0,
		(GETPLUGININFOS)	JV1_libGetPluginInfo
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
