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
// File : st_loader.c
// Contains: ST floppy image loader
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

#include "st_loader.h"
#include "st_writer.h"

#include "stfileformat.h"

#include "libhxcadaptor.h"

int getfloppyconfig(unsigned char bootsector[512],uint32_t filesize, HXCFE_FLPGEN * fb_ctx)
{
	int i;
	int32_t nb_of_side,nb_of_track,nb_of_sector;
	int32_t numberofsector;

	int numberofsectorpertrack;
	int numberofside;
	int gap3len;
	int numberoftrack;
	int interleave;

	unsigned char  * uimg;
	unsigned char  conffound;

	uimg=(unsigned char *)bootsector;

	conffound=0;
	if(uimg[0x18]<24 && uimg[0x18]>8)
	{

		numberofsectorpertrack=uimg[0x18];
		numberofside=uimg[0x1A];

		gap3len=84;
		interleave=1;

		switch(numberofsectorpertrack)
		{
			case 9:
				gap3len=84;
				interleave=1;
			break;
			case 10:
				gap3len=30;
				interleave=1;
			break;
			case 11:
				interleave=2;
				gap3len=3;
			break;
		}

		numberofsector=uimg[0x13]+(uimg[0x14]*256);
		if(numberofsectorpertrack && numberofside )
		{
			numberoftrack=(numberofsector/(numberofsectorpertrack*numberofside));

			if((unsigned int)((numberofsectorpertrack) * (numberoftrack) * (numberofside) * 512) == filesize)
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
			if(stfileformats[i].filesize==filesize)
			{
				numberoftrack = stfileformats[i].numberoftrack;
				numberofsectorpertrack = stfileformats[i].sectorpertrack;
				numberofside = stfileformats[i].numberofside;
				gap3len = stfileformats[i].gap3len;
				interleave = stfileformats[i].interleave;
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
							numberoftrack = nb_of_track;
							numberofsectorpertrack = nb_of_sector;
							numberofside = nb_of_side;

							gap3len=84;
							interleave=1;

							switch( numberofsectorpertrack )
							{
								case 9:
									gap3len=84;
									interleave=1;
								break;
								case 10:
									gap3len=30;
									interleave=1;
								break;
								case 11:
									interleave=2;
									gap3len=3;
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

	hxcfe_setNumberOfTrack ( fb_ctx, numberoftrack );
	hxcfe_setNumberOfSide ( fb_ctx, numberofside );
	hxcfe_setNumberOfSector ( fb_ctx, numberofsectorpertrack );
	hxcfe_setTrackInterleave( fb_ctx, interleave );
	hxcfe_setSectorSize( fb_ctx, 512 );
	hxcfe_setRPM( fb_ctx, 300 ); // normal rpm

	hxcfe_setSectorGap3 ( fb_ctx, gap3len );
	hxcfe_setTrackType( fb_ctx, ISOFORMAT_DD);

	if( numberofsectorpertrack < 15 )
	{
		hxcfe_setInterfaceMode( fb_ctx, ATARIST_DD_FLOPPYMODE);
		hxcfe_setTrackBitrate( fb_ctx, DEFAULT_DD_BITRATE );
		hxcfe_setTrackSkew( fb_ctx, 2 );
		hxcfe_setSideSkew( fb_ctx, 1 );
	}
	else
	{
		hxcfe_setInterfaceMode( fb_ctx, ATARIST_HD_FLOPPYMODE);
		hxcfe_setTrackBitrate( fb_ctx, DEFAULT_HD_BITRATE );
		hxcfe_setTrackSkew( fb_ctx, 4 );
		hxcfe_setSideSkew( fb_ctx, 2 );
	}

	if(numberofsectorpertrack==11)
	{
		hxcfe_setSectorGap3 ( fb_ctx, 3 );
		hxcfe_setTrackType( fb_ctx, ISOFORMAT_DD11S);
	}

	return conffound;
}


int ST_libIsValidDiskFile(HXCFE_IMGLDR * imgldr_ctx,char * imgfile)
{
	int filesize;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"ST_libIsValidDiskFile");

	if(hxc_checkfileext(imgfile,"st"))
	{
		filesize=hxc_getfilesize(imgfile);
		if(filesize<0)
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"ST_libIsValidDiskFile : Cannot open %s !",imgfile);
			return HXCFE_ACCESSERROR;
		}

		if(filesize&0x1FF)
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"ST_libIsValidDiskFile : non ST IMG file - bad file size !");
			return HXCFE_BADFILE;
		}

		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"ST_libIsValidDiskFile : ST file !");
		return HXCFE_VALIDFILE;
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"ST_libIsValidDiskFile : non ST file !");
		return HXCFE_BADFILE;
	}
}

int ST_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	HXCFE_FLPGEN * fb_ctx;
	FILE * f_img;
	int ret;
	unsigned int filesize;
	char boot_sector[512];

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"ST_libLoad_DiskFile %s",imgfile);

	f_img = hxc_fopen(imgfile,"rb");
	if( f_img == NULL )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	filesize = hxc_fgetsize(f_img);

	if( !filesize )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"file size=%d !?",filesize);
		hxc_fclose(f_img);
		return HXCFE_BADFILE;
	}

	fb_ctx = hxcfe_initFloppy( imgldr_ctx->hxcfe, 86, 2 );
	if( !fb_ctx )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Alloc Error !");
		hxc_fclose(f_img);
		return HXCFE_INTERNALERROR;
	}

	hxc_fread(boot_sector,512,f_img);

	if( getfloppyconfig( boot_sector, filesize, fb_ctx) == 1 )
	{
		fseek(f_img,0,SEEK_SET);

		ret = hxcfe_generateDisk( fb_ctx, floppydisk, f_img, 0, 0 );

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
		(ISVALIDDISKFILE)	ST_libIsValidDiskFile,
		(LOADDISKFILE)		ST_libLoad_DiskFile,
		(WRITEDISKFILE)		ST_libWrite_DiskFile,
		(GETPLUGININFOS)	ST_libGetPluginInfo
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
