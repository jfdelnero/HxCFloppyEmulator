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
// File : d64_loader.c
// Contains: Commodore 64 floppy image loader
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
#include "libhxcfe.h"

#include "floppy_loader.h"
#include "floppy_utils.h"

#include "tracks/track_formats/c64_gcr_track.h"

#include "d64_loader.h"

#include "libhxcadaptor.h"

typedef struct d64trackpos_
{
	int number_of_sector;
	int bitrate;
	int fileoffset;
}d64trackpos;

int D64_libIsValidDiskFile( HXCFE_IMGLDR * imgldr_ctx, HXCFE_IMGLDR_FILEINFOS * imgfile )
{
	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"D64_libIsValidDiskFile");

	if(hxc_checkfileext(imgfile->path,"d64",SYS_PATH_TYPE))
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"D64_libIsValidDiskFile : D64 file !");
		return HXCFE_VALIDFILE;
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"D64_libIsValidDiskFile : non D64 file !");
		return HXCFE_BADFILE;
	}
}

int D64_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	FILE * f;
	unsigned int filesize;
	int i,j,k;
	unsigned char* trackdata;
	int tracklen;
	int rpm;
	int sectorsize;
	HXCFE_CYLINDER* currentcylinder;
	HXCFE_SIDE* currentside;
	unsigned char errormap[1024];
	d64trackpos * tracklistpos;
	int number_of_track;
	int errormap_size;

	tracklistpos = NULL;
	trackdata = NULL;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"D64_libLoad_DiskFile %s",imgfile);

	f = hxc_fopen(imgfile,"rb");
	if( f == NULL )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	filesize = hxc_fgetsize(f);

	errormap_size=0;
	switch(filesize)
	{
		case 174848:
			//35 track, no errors
			number_of_track=35;
			break;

		case 175531:
			//35 track, 683 error bytes
			number_of_track=35;

			errormap_size=683;
			memset(errormap,0,errormap_size);
			fseek(f,errormap_size,SEEK_END);
			hxc_fread(errormap,errormap_size,f);

			break;

		case 196608:
			//40 track, no errors
			number_of_track=40;
			break;

		case 197376:
			//40 track, 768 error bytes
			number_of_track=40;

			errormap_size=768;
			memset(errormap,0,errormap_size);
			fseek(f,errormap_size,SEEK_END);
			hxc_fread(errormap,errormap_size,f);

			break;

		default:
			// not supported !
			imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Unsupported D64 file size ! (%d Bytes)",filesize);
			hxc_fclose(f);
			return HXCFE_UNSUPPORTEDFILE;

			break;
	}

	tracklistpos=(d64trackpos*)malloc(number_of_track*sizeof(d64trackpos));
	if( !tracklistpos )
		goto alloc_error;

	memset(tracklistpos,0,number_of_track*sizeof(d64trackpos));

	i=0;
	k=0;
	do
	{
		tracklistpos[i].number_of_sector=21;
		tracklistpos[i].fileoffset=k;
		tracklistpos[i].bitrate=307693;//XTAL_16_MHz / 13

		k=k+(21*256);
		i++;
	}while(i<number_of_track && i<17);

	i=17;
	do
	{
		tracklistpos[i].number_of_sector=19;
		tracklistpos[i].fileoffset=k;
		tracklistpos[i].bitrate=285715; //XTAL_16_MHz / 14

		k=k+(19*256);
		i++;
	}while(i<number_of_track && i<24);

	i=24;
	do
	{
		tracklistpos[i].number_of_sector=18;
		tracklistpos[i].fileoffset=k;
		tracklistpos[i].bitrate=266667;//XTAL_16_MHz / 15

		k=k+(18*256);
		i++;
	}while(i<number_of_track && i<30);

	i=30;
	do
	{
		tracklistpos[i].number_of_sector=17;
		tracklistpos[i].fileoffset=k;
		tracklistpos[i].bitrate=250000;//XTAL_16_MHz / 16

		k=k+(17*256);
		i++;
	}while(i<number_of_track);


	floppydisk->floppyNumberOfTrack=number_of_track;
	floppydisk->floppyNumberOfSide=1;
	floppydisk->floppySectorPerTrack=-1;

	sectorsize=256; // c64 file support only 256bytes/sector floppies.

	floppydisk->floppyBitRate=VARIABLEBITRATE;
	floppydisk->floppyiftype=C64_DD_FLOPPYMODE;
	floppydisk->tracks=(HXCFE_CYLINDER**)malloc(sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);

	rpm=300;

	imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"filesize:%dkB, %d tracks, %d side(s), rpm:%d",filesize/1024,floppydisk->floppyNumberOfTrack,floppydisk->floppyNumberOfSide,rpm);

	for(j=0;j<floppydisk->floppyNumberOfTrack;j++)
	{
		trackdata = (unsigned char*)malloc(sectorsize*tracklistpos[j].number_of_sector);
		if( !trackdata )
			goto alloc_error;

		floppydisk->tracks[j]=allocCylinderEntry(rpm,floppydisk->floppyNumberOfSide);
		currentcylinder=floppydisk->tracks[j];

		for(i=0;i<floppydisk->floppyNumberOfSide;i++)
		{
			hxcfe_imgCallProgressCallback(imgldr_ctx,(j<<1) + (i&1),floppydisk->floppyNumberOfTrack*2 );

			tracklen=(tracklistpos[j].bitrate/(rpm/60))/4;

			currentcylinder->sides[i] = malloc(sizeof(HXCFE_SIDE));
			if( !currentcylinder->sides[i] )
				goto alloc_error;

			memset(currentcylinder->sides[i],0,sizeof(HXCFE_SIDE));
			currentside=currentcylinder->sides[i];

			currentside->number_of_sector=tracklistpos[j].number_of_sector;
			currentside->tracklen=tracklen;

			currentside->databuffer = malloc(currentside->tracklen);
			if( !currentside->databuffer )
				goto alloc_error;

			memset(currentside->databuffer,0,currentside->tracklen);

			currentside->flakybitsbuffer=0;

			currentside->indexbuffer = malloc(currentside->tracklen);
			if( !currentside->indexbuffer )
				goto alloc_error;

			memset(currentside->indexbuffer,0,currentside->tracklen);

			currentside->timingbuffer=0;
			currentside->bitrate=tracklistpos[j].bitrate;
			currentside->track_encoding=UNKNOWN_ENCODING;

			fseek (f , tracklistpos[j].fileoffset , SEEK_SET);

			hxc_fread(trackdata,sectorsize*currentside->number_of_sector,f);

			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Track:%d Size:%d File offset:%d Number of sector:%d Bitrate:%d",j,currentside->tracklen,tracklistpos[j].fileoffset,tracklistpos[j].number_of_sector,currentside->bitrate);

			BuildC64GCRTrack(currentside->number_of_sector,sectorsize,j,i,trackdata,currentside->databuffer,&currentside->tracklen);

			currentside->tracklen=currentside->tracklen*8;

		}

		free(trackdata);
		trackdata = NULL;
	}

	imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"track file successfully loaded and encoded!");

	hxc_fclose(f);
	return HXCFE_NOERROR;

alloc_error:

	if ( f )
		hxc_fclose( f );

	hxcfe_freeFloppy(imgldr_ctx->hxcfe, floppydisk );

	free( trackdata );
	free( tracklistpos );

	return HXCFE_INTERNALERROR;
}

int D64_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]="C64_D64";
	static const char plug_desc[]="C64 D64 file image loader";
	static const char plug_ext[]="d64";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   D64_libIsValidDiskFile,
		(LOADDISKFILE)      D64_libLoad_DiskFile,
		(WRITEDISKFILE)     0,
		(GETPLUGININFOS)    D64_libGetPluginInfo
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
