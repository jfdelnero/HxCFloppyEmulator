/*
//
// Copyright (C) 2006-2023 Jean-François DEL NERO
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
// File : woz_loader.c
// Contains: WOZ floppy image loader
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

#include "woz_loader.h"

#include "woz_format.h"

#include "libhxcadaptor.h"
#include "tracks/crc.h"
#include "tracks/std_crc32.h"

int WOZ_libIsValidDiskFile( HXCFE_IMGLDR * imgldr_ctx, HXCFE_IMGLDR_FILEINFOS * imgfile )
{
	woz_fileheader * fileheader;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"WOZ_libIsValidDiskFile");

	if(hxc_checkfileext(imgfile->path,"woz",SYS_PATH_TYPE))
	{
		fileheader = (woz_fileheader*)imgfile->file_header;

		if(!memcmp((char*)&fileheader->headertag,"WOZ", 3))
		{
			if( fileheader->version != '1' && fileheader->version != '2' )
			{
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"WOZ_libIsValidDiskFile : non WOZ file (bad header)!");
				return HXCFE_BADFILE;
			}

			if(   fileheader->pad != 0xFF ||
			    ( fileheader->lfcrlf[0] != 0xA || fileheader->lfcrlf[1] != 0xD || fileheader->lfcrlf[2] != 0xA  )
			  )
			{
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"WOZ_libIsValidDiskFile : non WOZ file (bad header)!");
				return HXCFE_BADFILE;
			}

			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"WOZ_libIsValidDiskFile : WOZ %d file !", fileheader->version - '0');

			return HXCFE_VALIDFILE;
		}

		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"WOZ_libIsValidDiskFile : non WOZ file (bad header)!");

		return HXCFE_BADFILE;
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"WOZ_libIsValidDiskFile : non WOZ file !");
		return HXCFE_BADFILE;
	}
}

int WOZ_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	FILE * f;
	woz_fileheader fileheader;
	int filesize;
	unsigned char * file_buffer;
	uint32_t crc32;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"WOZ_libLoad_DiskFile %s",imgfile);

	f = hxc_fopen(imgfile,"rb");
	if( f == NULL )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	memset( &fileheader, 0, sizeof(woz_fileheader) );

	hxc_fread( &fileheader, sizeof(woz_fileheader), f );

	if(memcmp((char*)&fileheader.headertag,"WOZ", 3))
	{
		hxc_fclose(f);
		return HXCFE_BADFILE;
	}

	if( fileheader.version != '1' && fileheader.version != '2' )
	{
		hxc_fclose(f);
		return HXCFE_BADFILE;
	}

	if(   fileheader.pad != 0xFF ||
	    ( fileheader.lfcrlf[0] != 0xA || fileheader.lfcrlf[1] != 0xD || fileheader.lfcrlf[2] != 0xA  )
	  )
	{
		hxc_fclose(f);
		return HXCFE_BADFILE;
	}

	filesize = hxc_fgetsize(f);
	if( filesize <= sizeof(woz_fileheader) )
	{
		hxc_fclose(f);
		return HXCFE_BADFILE;
	}

	file_buffer = malloc(filesize);
	if(!file_buffer)
	{
		hxc_fclose(f);
		return HXCFE_INTERNALERROR;
	}

	memset(file_buffer,0,filesize);

	fseek(f,0,SEEK_SET);

	hxc_fread( file_buffer, filesize, f );

	hxc_fclose(f);

	crc32 = std_crc32(0x00000000, &file_buffer[sizeof(woz_fileheader)], filesize - sizeof(woz_fileheader) );

	if(crc32 != fileheader.crc32)
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"BAD CRC32, Corrupted file !",imgfile);
		free(file_buffer);
		hxc_fclose(f);
		return HXCFE_BADFILE;
	}

	free(file_buffer);

	return HXCFE_INTERNALERROR;
}

int WOZ_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]="APPLEII_WOZ";
	static const char plug_desc[]="Apple II WOZ Loader";
	static const char plug_ext[]="woz";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   WOZ_libIsValidDiskFile,
		(LOADDISKFILE)      WOZ_libLoad_DiskFile,
		(WRITEDISKFILE)     NULL,
		(GETPLUGININFOS)    WOZ_libGetPluginInfo
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
