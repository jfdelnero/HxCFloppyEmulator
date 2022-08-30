/*
//
// Copyright (C) 2006-2022 Jean-François DEL NERO
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
// File : fdx_loader.c
// Contains: FDX floppy image loader
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

#include "tracks/encoding/mfm_encoding.h"

#include "floppy_loader.h"
#include "floppy_utils.h"

#include "fdx_loader.h"
#include "fdx_writer.h"

#include "fdx_format.h"

#include "libhxcadaptor.h"
#include "tracks/crc.h"

#define FDX_DBG 1

int FDX_libIsValidDiskFile( HXCFE_IMGLDR * imgldr_ctx, HXCFE_IMGLDR_FILEINFOS * imgfile )
{
	fdxheader_t * fileheader;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"FDX_libIsValidDiskFile");

	if(hxc_checkfileext(imgfile->path,"fdx",SYS_PATH_TYPE))
	{
		fileheader = (fdxheader_t*)imgfile->file_header;

		if(memcmp((char*)&fileheader->fdx_signature,"FDX",3) || !(fileheader->nb_of_cylinders && fileheader->nb_of_cylinders <= 86 && fileheader->nb_of_heads >= 1 && fileheader->nb_of_heads <= 2 ) )
		{

			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"FDX_libIsValidDiskFile : non FDX file (bad header)!");
			return HXCFE_BADFILE;
		}
		else
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"FDX_libIsValidDiskFile : FDX file !");
			return HXCFE_VALIDFILE;
		}
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"FDX_libIsValidDiskFile : non FDX file !");
		return HXCFE_BADFILE;
	}
}

int FDX_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	FILE * f;
	int ret;
	int i,j,k,len;
	int tracknumber,sidenumber;
	HXCFE_CYLINDER* currentcylinder;
	HXCFE_SIDE* currentside;
	fdxheader_t fileheader;
	fdxtrack_t * fdxtrackheader;
	uint8_t * track_buffer;

	currentcylinder = 0;

	f = NULL;
	track_buffer = NULL;

	ret = HXCFE_NOERROR;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"FDX_libLoad_DiskFile %s",imgfile);

	f = hxc_fopen(imgfile,"rb");
	if( f == NULL )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	memset( &fileheader,0,sizeof(fdxheader_t));
	if(hxc_fread( &fileheader, sizeof(fdxheader_t), f ))
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Can't read the file header %s !",imgfile);
		fclose(f);
		return HXCFE_ACCESSERROR;
	}

	if(!memcmp((char*)&fileheader.fdx_signature,"FDX",3) && (fileheader.nb_of_cylinders && fileheader.nb_of_cylinders <= 86 && fileheader.nb_of_heads >= 1 && fileheader.nb_of_heads <= 2 && fileheader.track_block_size) )
	{
#ifdef FDX_DBG
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"FDX revision : %d",fileheader.revision);
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Disk name : %s",fileheader.disk_name);
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Disk type : 0x%X",fileheader.disk_type);
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Number of cylinders : %d",fileheader.nb_of_cylinders);
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Number of heads : %d",fileheader.nb_of_heads);
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Default bitrate : %d",fileheader.default_bitrate);
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Disk RPM : %d",fileheader.disk_rpm);
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Write Protect : %d",fileheader.write_protect);
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Options flags : 0x%.8X",fileheader.option);
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Track block size : 0x%.8X",fileheader.track_block_size);
#endif

		floppydisk->floppySectorPerTrack = -1;
		floppydisk->floppyNumberOfSide = fileheader.nb_of_heads;
		floppydisk->floppyNumberOfTrack = fileheader.nb_of_cylinders;
		floppydisk->floppyiftype = ATARIST_DD_FLOPPYMODE;
		floppydisk->floppyBitRate = (fileheader.default_bitrate / 2) * 1000;

		track_buffer = malloc(fileheader.track_block_size);
		if(!track_buffer)
		{
			ret = HXCFE_INTERNALERROR;
			goto error;
		}

		fdxtrackheader = (fdxtrack_t*)track_buffer;

		floppydisk->tracks = (HXCFE_CYLINDER**)malloc(sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);
		if(!floppydisk->tracks)
		{
			ret = HXCFE_INTERNALERROR;
			goto error;
		}

		memset(floppydisk->tracks,0,sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);

		for(i=0;i<floppydisk->floppyNumberOfTrack*floppydisk->floppyNumberOfSide;i++)
		{
			if( !hxc_fread( track_buffer, fileheader.track_block_size, f ) )
			{
				#ifdef FDX_DBG
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Track %d, Head %d",fdxtrackheader->cylinder,fdxtrackheader->head);
				#endif

				tracknumber = fdxtrackheader->cylinder;
				sidenumber = fdxtrackheader->head;

				if(!floppydisk->tracks[tracknumber])
				{
					floppydisk->tracks[tracknumber] = (HXCFE_CYLINDER*)malloc(sizeof(HXCFE_CYLINDER));
					if(!floppydisk->tracks[tracknumber])
					{
						ret = HXCFE_INTERNALERROR;
						goto error;
					}

					memset(floppydisk->tracks[tracknumber],0,sizeof(HXCFE_CYLINDER));

					floppydisk->tracks[tracknumber]->floppyRPM = fileheader.disk_rpm;
					currentcylinder = floppydisk->tracks[tracknumber];
					currentcylinder->number_of_side = 0;

					currentcylinder->sides = (HXCFE_SIDE**)malloc(sizeof(HXCFE_SIDE*)*2);
					if(!currentcylinder->sides)
					{
						ret = HXCFE_INTERNALERROR;
						goto error;
					}

					memset(currentcylinder->sides,0,sizeof(HXCFE_SIDE*)*2);
				}

				currentcylinder=floppydisk->tracks[tracknumber];

				currentcylinder->sides[sidenumber] = tg_alloctrack(floppydisk->floppyBitRate,ISOIBM_MFM_ENCODING,300,fdxtrackheader->bit_track_length,2500,0,TG_ALLOCTRACK_ALLOCTIMIMGBUFFER|TG_ALLOCTRACK_ALLOCFLAKEYBUFFER);
				if(!currentcylinder->sides[sidenumber])
				{
					ret = HXCFE_INTERNALERROR;
					goto error;
				}


				currentside=currentcylinder->sides[sidenumber];
				currentcylinder->number_of_side++;

				len = fdxtrackheader->bit_track_length/8;
				if( fdxtrackheader->bit_track_length & 7 )
					len++;

				memset(currentside->databuffer,0x00,len);

				k = (fdxtrackheader->index_bit_plase) % fdxtrackheader->bit_track_length;
				for(j=0;j<fdxtrackheader->bit_track_length;j++)
				{
					if( track_buffer[sizeof(fdxtrack_t) + (k>>3)] & (0x80>>(k&7)) )
					{
						currentside->databuffer[j>>3] |= (0x80>>(j&7));
					}

					k++;
					k %= fdxtrackheader->bit_track_length;
				}

				for(j=0;j<len;j++)
				{
					if(!currentside->databuffer[j])
					{
						currentside->databuffer[j] = 0x24;
						currentside->flakybitsbuffer[j] = 0xFF;
					}
				}
			}
		}
	}

	hxc_fclose(f);

	if(track_buffer)
		free(track_buffer);

	return ret;

error:
	if(f)
		hxc_fclose(f);

	if(track_buffer)
		free(track_buffer);

	return ret;
}

int FDX_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{

	static const char plug_id[]="PC88_FDX";
	static const char plug_desc[]="PC88 FDX Loader";
	static const char plug_ext[]="fdx";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   FDX_libIsValidDiskFile,
		(LOADDISKFILE)      FDX_libLoad_DiskFile,
		(WRITEDISKFILE)     NULL, //FDX_libWrite_DiskFile,
		(GETPLUGININFOS)    FDX_libGetPluginInfo
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

