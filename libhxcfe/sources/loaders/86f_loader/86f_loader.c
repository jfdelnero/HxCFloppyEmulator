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
// File : 86f_loader.c
// Contains: 86F floppy image loader
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

#include "86f_loader.h"
#include "86f_writer.h"

#include "86f_format.h"

#include "libhxcadaptor.h"
#include "tracks/crc.h"

#define F86_DBG 1

int F86_libIsValidDiskFile( HXCFE_IMGLDR * imgldr_ctx, HXCFE_IMGLDR_FILEINFOS * imgfile )
{
	f86header_t * fileheader;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"86F_libIsValidDiskFile");

	if(hxc_checkfileext(imgfile->path,"86F",SYS_PATH_TYPE))
	{
		fileheader = (f86header_t*)imgfile->file_header;

		if( memcmp((char*)&fileheader->f86_signature,"86BF",4) )
		{

			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"86F_libIsValidDiskFile : non 86F file (bad header)!");
			return HXCFE_BADFILE;
		}
		else
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"86F_libIsValidDiskFile : 86F file !");
			return HXCFE_VALIDFILE;
		}
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"86F_libIsValidDiskFile : non 86F file !");
		return HXCFE_BADFILE;
	}
}

int F86_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	FILE * f;
	int ret;
	short rpm;
	int i,j,k,len;
	int tracknumber,sidenumber;
	HXCFE_CYLINDER* currentcylinder;
	HXCFE_SIDE* currentside;
	f86header_t fileheader;
	uint8_t * track_buffer;
	uint32_t tracks_offset_array[512];
	f86track_t cur_track_header;

	currentcylinder = 0;

	f = NULL;
	track_buffer = NULL;

	ret = HXCFE_NOERROR;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"86F_libLoad_DiskFile %s",imgfile);

	f = hxc_fopen(imgfile,"rb");
	if( f == NULL )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	memset( &fileheader,0,sizeof(f86header_t));
	if( hxc_fread( &fileheader, sizeof(f86header_t), f ) <= 0 )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Can't read the file header %s !",imgfile);
		fclose(f);
		return HXCFE_ACCESSERROR;
	}

	if(!memcmp((char*)&fileheader.f86_signature,"86BF",4) )
	{
#ifdef F86_DBG
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"86F revision : %d.%d",fileheader.major_revision,fileheader.minor_revision);
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"86F Flags : 0x%.8X",fileheader.disk_flags);

		if(fileheader.disk_flags & DISK_FLAG_HAS_DESCRIPTION_DATA)
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"DISK_FLAG_HAS_DESCRIPTION_DATA is set");
		else
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"DISK_FLAG_HAS_DESCRIPTION_DATA not set");

		if(fileheader.disk_flags & DISK_FLAG_DOUBLE_SIDED)
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"DISK_FLAG_DOUBLE_SIDED is set");
		else
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"DISK_FLAG_DOUBLE_SIDED not set");

		if(fileheader.disk_flags & DISK_FLAG_WRITE_PROTECTED)
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"DISK_FLAG_WRITE_PROTECTED is set");
		else
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"DISK_FLAG_WRITE_PROTECTED not set");

		if(fileheader.disk_flags & DISK_FLAG_BITCELL_MODE)
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"DISK_FLAG_BITCELL_MODE is set");
		else
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"DISK_FLAG_BITCELL_MODE not set");

		if(fileheader.disk_flags & DISK_FLAG_MULTIPLE_REVOLUTIONS)
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"DISK_FLAG_MULTIPLE_REVOLUTIONS is set");
		else
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"DISK_FLAG_MULTIPLE_REVOLUTIONS not set");

		switch( DISK_FLAG_GET_DISK_TYPE(fileheader.disk_flags) )
		{
			case 0:
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"DD Disk type");
			break;
			case 1:
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"HD Disk type");
			break;
			case 2:
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"ED Disk type");
			break;
			case 3:
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"ED + 2000 kbps Disk type");
			break;
		}

#endif

		if( hxc_fread( &tracks_offset_array, sizeof(tracks_offset_array), f ) <= 0 )
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Can't read the tracks offset %s !",imgfile);
			fclose(f);
			return HXCFE_ACCESSERROR;
		}

		floppydisk->floppySectorPerTrack = -1;
		floppydisk->floppyiftype = ATARIST_DD_FLOPPYMODE;
		floppydisk->floppyBitRate = 250 * 1000;

		switch( DISK_FLAG_GET_DISK_TYPE(fileheader.disk_flags) )
		{
			case 0:
				floppydisk->floppyBitRate = 250 * 1000;
			break;
			case 1:
				floppydisk->floppyBitRate = 500 * 1000;
			break;
			case 2:
				floppydisk->floppyBitRate = 1000 * 1000;
			break;
			case 3:
				floppydisk->floppyBitRate = 2000 * 1000;
			break;
		}

		if(fileheader.disk_flags & DISK_FLAG_DOUBLE_SIDED)
			floppydisk->floppyNumberOfSide = 2;
		else
			floppydisk->floppyNumberOfSide = 1;

		floppydisk->floppyNumberOfTrack = 0;
		for(i=0;i<512/2;i++)
		{
			if(tracks_offset_array[(i*2)] || tracks_offset_array[(i*2)+1])
			{
				floppydisk->floppyNumberOfTrack = (i+1);
			}
		}

		rpm = 300;

		floppydisk->tracks = (HXCFE_CYLINDER**)malloc(sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);
		if(!floppydisk->tracks)
		{
			ret = HXCFE_INTERNALERROR;
			goto error;
		}

		memset(floppydisk->tracks,0,sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);

		for(i=0;i<floppydisk->floppyNumberOfTrack*floppydisk->floppyNumberOfSide;i++)
		{
			tracknumber = i>>1;
			sidenumber = i&1;

			if(tracks_offset_array[i])
			{
				fseek(f,tracks_offset_array[i],SEEK_SET);

				if( hxc_fread( &cur_track_header, sizeof(cur_track_header), f ) > 0 )
				{
					#ifdef F86_DBG
					imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"86F Cylinder: %d, Head: %d, nb of bitcells: %d, Index position: %d",i>>1,i&1,cur_track_header.number_of_bit_cells,cur_track_header.index_position);
					#endif

					if(!floppydisk->tracks[tracknumber])
					{
						floppydisk->tracks[tracknumber] = (HXCFE_CYLINDER*)malloc(sizeof(HXCFE_CYLINDER));
						if(!floppydisk->tracks[tracknumber])
						{
							ret = HXCFE_INTERNALERROR;
							goto error;
						}

						memset(floppydisk->tracks[tracknumber],0,sizeof(HXCFE_CYLINDER));

						currentcylinder = floppydisk->tracks[tracknumber];

						currentcylinder->number_of_side = floppydisk->floppyNumberOfSide;

						currentcylinder->sides = (HXCFE_SIDE**)malloc(sizeof(HXCFE_SIDE*)*2);
						if(!currentcylinder->sides)
						{
							ret = HXCFE_INTERNALERROR;
							goto error;
						}

						memset(currentcylinder->sides,0,sizeof(HXCFE_SIDE*)*2);
					}

					floppydisk->tracks[tracknumber]->floppyRPM = rpm;

					currentcylinder = floppydisk->tracks[tracknumber];
				}

				currentcylinder=floppydisk->tracks[tracknumber];

				currentcylinder->sides[sidenumber] = tg_alloctrack(floppydisk->floppyBitRate,ISOIBM_MFM_ENCODING,rpm,cur_track_header.number_of_bit_cells,3800,0,0);
				if(!currentcylinder->sides[sidenumber])
				{
					ret = HXCFE_INTERNALERROR;
					goto error;
				}

				currentside=currentcylinder->sides[sidenumber];
				currentcylinder->number_of_side++;

				len = cur_track_header.number_of_bit_cells/8;
				if( cur_track_header.number_of_bit_cells & 7 )
					len++;

				if(len>0)
				{
					track_buffer = malloc(len);
					if(track_buffer)
					{
						memset(track_buffer,0,len);

						hxc_fread( track_buffer, len, f );
						memset(currentside->databuffer,0x00,len);

						k = (cur_track_header.index_position) % cur_track_header.number_of_bit_cells;
						for(j=0;j<cur_track_header.number_of_bit_cells;j++)
						{
							if( track_buffer[(k>>3)] & (0x80>>(k&7)) )
							{
								currentside->databuffer[j>>3] |= (0x80>>(j&7));
							}

							k++;
							k %= cur_track_header.number_of_bit_cells;
						}

						free(track_buffer);

						track_buffer = NULL;
					}

				}
			}

			hxcfe_sanityCheck(imgldr_ctx->hxcfe,floppydisk);

		}
	}

	hxc_fclose(f);

	if(track_buffer)
		free(track_buffer);

	return ret;

error:
	if(f)
		hxc_fclose(f);

	free(track_buffer);

	return ret;
}

int F86_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]="PC_86F";
	static const char plug_desc[]="86Box 86F Loader";
	static const char plug_ext[]="86f";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   F86_libIsValidDiskFile,
		(LOADDISKFILE)      F86_libLoad_DiskFile,
		(WRITEDISKFILE)     NULL, //F86_libWrite_DiskFile,
		(GETPLUGININFOS)    F86_libGetPluginInfo
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

