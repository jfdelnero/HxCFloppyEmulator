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
// File : st_loader.c
// Contains: ST floppy image loader
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
#include "libhxcfe.h"
#include "libhxcadaptor.h"
#include "floppy_loader.h"
#include "tracks/track_generator.h"

#include "st_loader.h"
#include "st_writer.h"

#include "stfileformat.h"

#include "loaders/common/raw_iso.h"

static int getfloppyconfig(unsigned char bootsector[512],uint32_t filesize, raw_iso_cfg *rawcfg)
{
	int i;
	int32_t nb_of_side,nb_of_track,nb_of_sector;
	int32_t numberofsector;

	unsigned char  * uimg;
	unsigned char  conffound;

	uimg=(unsigned char *)bootsector;

	conffound=0;

	raw_iso_setdefcfg(rawcfg);

	if(uimg[0x18]<24 && uimg[0x18]>8)
	{

		rawcfg->number_of_sectors_per_track = uimg[0x18];
		rawcfg->number_of_sides = uimg[0x1A];

		rawcfg->gap3 = 84;
		rawcfg->interleave = 1;

		switch(rawcfg->number_of_sectors_per_track)
		{
			case 9:
				rawcfg->gap3 = 84;
				rawcfg->interleave = 1;
			break;
			case 10:
				rawcfg->gap3 = 30;
				rawcfg->interleave = 1;
			break;
			case 11:
				rawcfg->gap3 = 3;
				rawcfg->interleave = 2;
			break;
		}

		numberofsector = uimg[0x13]+(uimg[0x14]*256);
		if( rawcfg->number_of_sectors_per_track && rawcfg->number_of_sides )
		{
			rawcfg->number_of_tracks = (numberofsector/(rawcfg->number_of_sectors_per_track * rawcfg->number_of_sides));

			if((unsigned int)((rawcfg->number_of_sectors_per_track) * (rawcfg->number_of_tracks) * (rawcfg->number_of_sides) * 512) == filesize)
			{
				conffound=1;
			}
		}

	}

	if(!conffound)
	{
		i=0;
		do
		{
			if(stfileformats[i].filesize == filesize)
			{
				rawcfg->number_of_tracks = stfileformats[i].numberoftrack;
				rawcfg->number_of_sectors_per_track = stfileformats[i].sectorpertrack;
				rawcfg->number_of_sides = stfileformats[i].numberofside;
				rawcfg->gap3 = stfileformats[i].gap3len;
				rawcfg->interleave = stfileformats[i].interleave;
				conffound=1;
			}
			i++;

		}while(stfileformats[i].filesize!=0 && conffound==0);

		if(!conffound)
		{

			for(nb_of_side=1;nb_of_side<=2;nb_of_side++)
			{
				for(nb_of_track=0;nb_of_track<85;nb_of_track++)
				{
					for(nb_of_sector=8;nb_of_sector<=11;nb_of_sector++)
					{
						if(filesize==(unsigned int)(nb_of_side*nb_of_track*nb_of_sector*512))
						{
							rawcfg->number_of_tracks = nb_of_track;
							rawcfg->number_of_sectors_per_track = nb_of_sector;
							rawcfg->number_of_sides = nb_of_side;
							rawcfg->gap3 = 84;
							rawcfg->interleave = 1;

							switch( rawcfg->number_of_sectors_per_track )
							{
								case 9:
									rawcfg->gap3 = 84;
									rawcfg->interleave = 1;
								break;
								case 10:
									rawcfg->gap3 = 30;
									rawcfg->interleave = 1;
								break;
								case 11:
									rawcfg->gap3 = 3;
									rawcfg->interleave = 2;
								break;
							}

							conffound = 1;

							return conffound;
						}
					}
				}
			}
		}
	}

	rawcfg->fill_value = 0xE5;

	rawcfg->rpm = 300;
	rawcfg->sector_size = 512;
	rawcfg->start_sector_id = 1;
	rawcfg->track_format = ISOFORMAT_DD;

	if( rawcfg->number_of_sectors_per_track < 15 )
	{
		rawcfg->interface_mode = ATARIST_DD_FLOPPYMODE;
		rawcfg->bitrate = DEFAULT_DD_BITRATE;

		rawcfg->skew_per_track = 4;
		rawcfg->skew_per_side = 2;
	}
	else
	{
		rawcfg->interface_mode = ATARIST_HD_FLOPPYMODE;
		rawcfg->bitrate = DEFAULT_HD_BITRATE;

		rawcfg->skew_per_track = 8;
		rawcfg->skew_per_side = 4;
	}

	if( rawcfg->number_of_sectors_per_track == 11 )
	{
		rawcfg->gap3 = 3;
		rawcfg->track_format = ISOFORMAT_DD11S;
	}

	return conffound;
}


int ST_libIsValidDiskFile( HXCFE_IMGLDR * imgldr_ctx, HXCFE_IMGLDR_FILEINFOS * imgfile )
{
	return hxcfe_imgCheckFileCompatibility( imgldr_ctx, imgfile, "ST_libIsValidDiskFile", "st", 512);
}

int ST_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	raw_iso_cfg rawcfg;
	FILE * f_img;
	int ret;
	unsigned int filesize;
	unsigned char boot_sector[512];

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"ST_libLoad_DiskFile %s",imgfile);

	f_img = hxc_fopen(imgfile,"rb");
	if( f_img == NULL )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	filesize = hxc_fgetsize(f_img);

	memset(boot_sector,0,sizeof(boot_sector));
	hxc_fread(boot_sector,512,f_img);

	if( getfloppyconfig( boot_sector, filesize, &rawcfg) == 1 )
	{
		fseek(f_img,0,SEEK_SET);

		ret = raw_iso_loader(imgldr_ctx, floppydisk, f_img, 0, 0, &rawcfg);

		hxc_fclose(f_img);

		return ret;
	}

	hxc_fclose(f_img);

	return HXCFE_BADFILE;
}

int ST_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]="ATARIST_ST";
	static const char plug_desc[]="ATARI ST ST Loader";
	static const char plug_ext[]="st";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   ST_libIsValidDiskFile,
		(LOADDISKFILE)      ST_libLoad_DiskFile,
		(WRITEDISKFILE)     ST_libWrite_DiskFile,
		(GETPLUGININFOS)    ST_libGetPluginInfo
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
