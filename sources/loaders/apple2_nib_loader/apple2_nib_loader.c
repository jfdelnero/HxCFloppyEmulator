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
// File : apple2_nib_loader.c
// Contains: Apple 2 nib floppy image loader
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

#include "apple2_nib_loader.h"

#include "libhxcadaptor.h"

#define NIB_TRACK_SIZE 6656

//#define HDDD_A2_SUPPORT 1

extern unsigned short ext_a2_bit_expander[];

int Apple2_nib_libIsValidDiskFile(HXCFE_IMGLDR * imgldr_ctx,char * imgfile)
{
	int filesize;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Apple2_nib_libIsValidDiskFile");

	if( hxc_checkfileext(imgfile,"nib") )
	{
		filesize=hxc_getfilesize(imgfile);
		if(filesize<0)
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Apple2_nib_libIsValidDiskFile : Cannot open %s !",imgfile);
			return HXCFE_ACCESSERROR;
		}

		if(filesize != (NIB_TRACK_SIZE*35) )
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Apple2_nib_libIsValidDiskFile : non Apple 2 NIB file - bad file size !");
			return HXCFE_BADFILE;
		}

		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Apple2_nib_libIsValidDiskFile : Apple 2 NIB file !");
		return HXCFE_VALIDFILE;
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Apple2_nib_libIsValidDiskFile : non Apple 2 NIB file !");
		return HXCFE_BADFILE;
	}
}

int Apple2_nib_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	FILE * f;
	unsigned int filesize;
	int i,j;
	unsigned int file_offset;
	unsigned char* trackdata;
	int trackformat;
	int rpm;
	HXCFE_CYLINDER* currentcylinder;

	unsigned short pulses;

#ifdef HDDD_A2_SUPPORT
	unsigned short fm_pulses;
#endif
	unsigned char  data_nibble;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Apple2_nib_libLoad_DiskFile %s",imgfile);

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
		trackformat=ISOFORMAT_DD;

		floppydisk->floppyNumberOfSide=2;
		floppydisk->floppySectorPerTrack=-1;
		floppydisk->floppyNumberOfTrack= filesize / NIB_TRACK_SIZE;

		floppydisk->floppyBitRate=250000;
#ifdef HDDD_A2_SUPPORT
		floppydisk->floppyBitRate = floppydisk->floppyBitRate * 2;
#endif
		floppydisk->floppyiftype=GENERIC_SHUGART_DD_FLOPPYMODE;
		floppydisk->tracks=(HXCFE_CYLINDER**)malloc(sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);

		rpm=300;

		imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"filesize:%dkB, %d tracks, %d side(s)",filesize/1024,floppydisk->floppyNumberOfTrack,floppydisk->floppyNumberOfSide);

		trackdata=(unsigned char*)malloc(NIB_TRACK_SIZE);

		for(j=0;j<floppydisk->floppyNumberOfTrack;j++)
		{
			hxcfe_imgCallProgressCallback(imgldr_ctx, j,floppydisk->floppyNumberOfTrack);

			floppydisk->tracks[j]=allocCylinderEntry(rpm,floppydisk->floppyNumberOfSide);
			currentcylinder=floppydisk->tracks[j];

			file_offset= NIB_TRACK_SIZE * j;

			fseek (f , file_offset , SEEK_SET);
			hxc_fread(trackdata,NIB_TRACK_SIZE,f);

#ifdef HDDD_A2_SUPPORT
			currentcylinder->sides[0] = tg_alloctrack(floppydisk->floppyBitRate,trackformat,rpm,NIB_TRACK_SIZE * 2 * 8 * 2,1000,0,0);
			currentcylinder->sides[1] = tg_alloctrack(floppydisk->floppyBitRate,trackformat,rpm,NIB_TRACK_SIZE * 2 * 8 * 2,1000,0,0);
#else
			currentcylinder->sides[0] = tg_alloctrack(floppydisk->floppyBitRate,trackformat,rpm,NIB_TRACK_SIZE * 2 * 8,1000,0,0);
			currentcylinder->sides[1] = tg_alloctrack(floppydisk->floppyBitRate,trackformat,rpm,NIB_TRACK_SIZE * 2 * 8,1000,0,0);
#endif
			for(i=0;i<NIB_TRACK_SIZE;i++)
			{
				data_nibble = trackdata[i];
				pulses = ext_a2_bit_expander[ data_nibble ];

#ifdef HDDD_A2_SUPPORT
				// Add the FM Clocks
				fm_pulses = ext_a2_bit_expander[pulses >> 8] | 0x2222;
				currentcylinder->sides[0]->databuffer[(i*4)+0] = fm_pulses >> 8;
				currentcylinder->sides[0]->databuffer[(i*4)+1] = fm_pulses &  0xFF;

				// Add the FM Clocks
				fm_pulses = ext_a2_bit_expander[pulses &  0xFF] | 0x2222;
				currentcylinder->sides[0]->databuffer[(i*4)+2] = fm_pulses >> 8;
				currentcylinder->sides[0]->databuffer[(i*4)+3] = fm_pulses &  0xFF;
#else
				currentcylinder->sides[0]->databuffer[(i*2)+0] = pulses >> 8;
				currentcylinder->sides[0]->databuffer[(i*2)+1] = pulses &  0xFF;
#endif
			}

#ifdef HDDD_A2_SUPPORT
			memset(currentcylinder->sides[1]->databuffer,0xAA,NIB_TRACK_SIZE * 2 * 2);
#else
			memset(currentcylinder->sides[1]->databuffer,0xAA,NIB_TRACK_SIZE * 2);
#endif
		}

		free(trackdata);

		imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"track file successfully loaded and encoded!");

		hxc_fclose(f);
		return HXCFE_NOERROR;

	}

	imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"file size=%d !?",filesize);
	hxc_fclose(f);
	return HXCFE_BADFILE;
}

int Apple2_nib_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{

	static const char plug_id[]="APPLE2_NIB";
	static const char plug_desc[]="Apple II NIB Loader";
	static const char plug_ext[]="nib";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)	Apple2_nib_libIsValidDiskFile,
		(LOADDISKFILE)		Apple2_nib_libLoad_DiskFile,
		(WRITEDISKFILE)		0,
		(GETPLUGININFOS)	Apple2_nib_libGetPluginInfo
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
