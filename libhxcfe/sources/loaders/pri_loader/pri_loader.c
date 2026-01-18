/*
//
// Copyright (C) 2006-2026 Jean-François DEL NERO
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
// File : pri_loader.c
// Contains: Hampa Hug's PCE pri floppy image loader
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

#include "pri_loader.h"

#include "pri_format.h"

#include "libhxcadaptor.h"

#include "thirdpartylibs/zlib/zlib.h"

#include "cache.h"

uint32_t pri_crc (const unsigned char *buf, int cnt, uint32_t crc)
{
	int i, j;

	for (i = 0; i < cnt; i++)
	{
		crc ^= ( (uint32_t) (buf[i]) << 24 );

		for (j = 0; j < 8; j++)
		{
			if ( crc & 0x80000000 )
			{
				crc = (crc << 1) ^ 0x1edc6f41;
			}
			else
			{
				crc = crc << 1;
			}
		}
	}

	return crc;
}

int PRI_libIsValidDiskFile( HXCFE_IMGLDR * imgldr_ctx, HXCFE_IMGLDR_FILEINFOS * imgfile )
{
	uint32_t crc32;
	pri_chunk_header * fileheader;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"PRI_libIsValidDiskFile");

	if(hxc_checkfileext(imgfile->path,"PRI",SYS_PATH_TYPE))
	{
		fileheader = (pri_chunk_header*)imgfile->file_header;

		if( ( fileheader->chunk_id != CHUNKID_PRI ) || ( BIGENDIAN_DWORD(fileheader->size) != 4 ) )
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"PRI_libIsValidDiskFile : non PRI file (bad header)!");
			return HXCFE_BADFILE;
		}
		else
		{
			crc32 = pri_crc ((void*)fileheader, sizeof(pri_chunk_header) + BIGENDIAN_DWORD(fileheader->size) + 4, 0x00000000);
			if(!crc32)
			{
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"PRI_libIsValidDiskFile : PRI file !");
				return HXCFE_VALIDFILE;
			}
			else
			{
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"PRI_libIsValidDiskFile : Corrupted PRI file ?");
				return HXCFE_BADFILE;
			}
		}
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"PRI_libIsValidDiskFile : non PRI file !");
		return HXCFE_BADFILE;
	}
}

int PRI_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	file_cache file;
	int ret;
	short rpm;
	int i,offset;
	HXCFE_CYLINDER* currentcylinder;
	HXCFE_SIDE * curside;
	pri_chunk_header chunk;
	pri_track track;
	int cont;
	uint32_t crc32;
	uint8_t b;
	int maxtrk,maxside;
	char textbuf[4*1024];

	maxtrk = 0;
	maxside = 0;

	rpm = -1;

	ret = HXCFE_NOERROR;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"PRI_libLoad_DiskFile %s",imgfile);

	if ( open_file(&file, imgfile, -1, 0x00) < 0 )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	floppydisk->floppyNumberOfTrack = 255;
	floppydisk->floppyNumberOfSide = 2;

	floppydisk->tracks = (HXCFE_CYLINDER**)calloc(1,sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);
	if(!floppydisk->tracks)
		goto error;

	currentcylinder = NULL;
	curside = NULL;
	memset(&track,0,sizeof(pri_track));

	cont = 1;
	offset = 0;
	while ( cont )
	{
		memset( &chunk,0,sizeof(pri_chunk_header));

		if ( read_buf( &file,offset, &chunk, sizeof(pri_chunk_header) ) !=  sizeof(pri_chunk_header) )
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Can't read the chunk header %s !",imgfile);
			return HXCFE_ACCESSERROR;
		}

		crc32 = 0;
		ret = 1;
		for(i=0;i<(sizeof(pri_chunk_header) + BIGENDIAN_DWORD(chunk.size) + 4) && ret;i++)
		{
			b = get_byte( &file, offset + i, &ret);
			crc32 = pri_crc ((void*)&b, 1, crc32);
		}

		if( !ret || crc32)
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Bad chunk CRC !");
			cont = 0;
			break;
		}

		switch(chunk.chunk_id)
		{
			case CHUNKID_PRI:
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Chunk : PRI");
			break;

			case CHUNKID_END:
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Chunk : END");
				cont = 0;
			break;

			case CHUNKID_TEXT:
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Chunk : TEXT");

				read_buf( &file, offset + sizeof(pri_chunk_header), &textbuf, sizeof(textbuf) );

				if( BIGENDIAN_DWORD(chunk.size) < sizeof(textbuf) )
					textbuf[BIGENDIAN_DWORD(chunk.size)] = 0;
				else
					textbuf[sizeof(textbuf) - 1] = 0;

				imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"%s", textbuf);
			break;

			case CHUNKID_TRAK:
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Chunk : TRAK");

				read_buf( &file, offset + sizeof(pri_chunk_header), &track, sizeof(pri_track) );

				track.cylinder = BIGENDIAN_DWORD(track.cylinder);
				track.head = BIGENDIAN_DWORD(track.head);
				track.track_len = BIGENDIAN_DWORD(track.track_len);
				track.bitclock_rate = BIGENDIAN_DWORD(track.bitclock_rate);

				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Chunk : Track");
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Cylinder : %d, Head : %d, Track lenght : %d, Rate : %d", track.cylinder, track.head, track.track_len, track.bitclock_rate );

				if( track.cylinder > 255 || track.head > 1)
				{
					imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Chunk : Bad track !");

					cont = 0;
				}

				if( track.cylinder > maxtrk )
					maxtrk = track.cylinder;

				if( track.head > maxside )
					maxside = track.head;

				if(!floppydisk->tracks[track.cylinder])
				{
					floppydisk->tracks[track.cylinder] = allocCylinderEntry(0,floppydisk->floppyNumberOfSide);
				}

				currentcylinder = floppydisk->tracks[track.cylinder];
				curside = currentcylinder->sides[track.head];
			break;

			case CHUNKID_DATA:
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Chunk : DATA, Size : %d", BIGENDIAN_DWORD(chunk.size));

				currentcylinder->sides[track.head] = tg_alloctrack(track.bitclock_rate / 2, UNKNOWN_ENCODING, rpm, BIGENDIAN_DWORD(chunk.size)*8,2500,-2500,TG_ALLOCTRACK_ALLOCFLAKEYBUFFER|TG_ALLOCTRACK_ALLOCTIMIMGBUFFER);
				curside = currentcylinder->sides[track.head];

				for(i=0;i<BIGENDIAN_DWORD(chunk.size);i++)
				{
					curside->databuffer[i] = get_byte( &file, offset + sizeof(pri_chunk_header) + i, &ret);
				}
			break;

			case CHUNKID_WEAK:
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Chunk : WEAK");

				uint32_t weakoffset = BIGENDIAN_DWORD( get_ulong( &file, offset + sizeof(pri_chunk_header), &ret) );
				uint32_t mask;

				if( curside )
				{
					for(i=0;i<(BIGENDIAN_DWORD(chunk.size) - 4);i++)
					{
						mask =  BIGENDIAN_DWORD(get_ulong( &file, offset + sizeof(pri_chunk_header) + 4 + i, &ret));
						for(int j=0;j<8;j++)
						{
							if( mask & ( 0x80 >> j ) )
								curside->flakybitsbuffer[((weakoffset + i + j)>>3) % curside->tracklen] |= ( mask & ( 0x80 >> ( (weakoffset + i + j) & 7 ) ) );
						}
					}
				}
			break;

			case CHUNKID_BCLK:
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Chunk : BCLK");

				uint32_t bclkoffset = BIGENDIAN_DWORD( get_ulong( &file, offset + sizeof(pri_chunk_header), &ret) );
				uint32_t bclk;

				if( curside )
				{
					for(i=0;i<(BIGENDIAN_DWORD(chunk.size) - 4) / 4;i++)
					{
						bclk =  BIGENDIAN_DWORD(get_ulong( &file, offset + sizeof(pri_chunk_header) + 4 + i, &ret));
						if(bclk)
						{
							curside->timingbuffer[((bclkoffset + i)>>3) % curside->tracklen] = (uint32_t)(((float)bclk / (float)65536) * (float)track.bitclock_rate);
						}
						else
						{
							curside->timingbuffer[((bclkoffset + i)>>3) % curside->tracklen] = track.bitclock_rate;
						}
					}
				}
			break;

			default:
				imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Chunk : Bad or unknown chunk ! (%.8X)", chunk.chunk_id);
			break;
		}

		offset += ( sizeof(pri_chunk_header) + BIGENDIAN_DWORD(chunk.size) + 4);
	}

	close_file(&file);

	floppydisk->floppyNumberOfTrack = maxtrk + 1;
	floppydisk->floppyNumberOfSide = maxside + 1;

	for(i=0;i<floppydisk->floppyNumberOfTrack;i++)
	{
		floppydisk->tracks[i]->number_of_side = floppydisk->floppyNumberOfSide;
	}

	hxcfe_sanityCheck(imgldr_ctx->hxcfe,floppydisk);

	return HXCFE_NOERROR;

error:

	close_file(&file);

	return ret;
}

int PRI_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]="PRI";
	static const char plug_desc[]="PCE Raw Image Loader";
	static const char plug_ext[]="pri";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   PRI_libIsValidDiskFile,
		(LOADDISKFILE)      PRI_libLoad_DiskFile,
		(WRITEDISKFILE)     0,//PRI_libWrite_DiskFile,
		(GETPLUGININFOS)    PRI_libGetPluginInfo
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
