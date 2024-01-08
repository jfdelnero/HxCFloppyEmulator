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

static int get_woz_chunk(unsigned char * buf,  int buf_size, int offset, uint32_t chunk_id)
{
	woz_chunk * chunk;

	if( offset < sizeof(woz_fileheader))
		offset = sizeof(woz_fileheader);

	while( offset < buf_size)
	{
		chunk = (woz_chunk *)	&buf[offset];

		if (chunk->id == chunk_id)
		{
			return offset;
		}

		offset += (8 + chunk->size);
	}

	return -1;
}

int WOZ_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	HXCFE_CYLINDER* currentcylinder;
	HXCFE_SIDE* currentside;
	FILE * f;
	woz_fileheader fileheader;
	woz_info * info;
	woz_chunk * chunk;
	woz_trk * trks;
	woz_trk_v1 * trks_v1;
	int offset;
	int filesize;
	unsigned char * file_buffer;
	uint32_t crc32;
	char * tmp_str;
	char tmp_str_buf[64];
	unsigned char * tmap_array;
	int tmap_array_size;
	int i, j, k, rpm;
	int max_track;
	int intertracks;
	unsigned char woz_track;
	unsigned char * track_dat;

	intertracks = 0;

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
		return HXCFE_BADFILE;
	}

	offset = sizeof(woz_fileheader);
	while( offset < filesize)
	{
		chunk = (woz_chunk *)&file_buffer[offset];
		switch(chunk->id)
		{
			case CHUNK_INFO:
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"INFO Chunk id : 0x%.8X, Offset : 0x%.8X, Size : %d",chunk->id,offset,chunk->size);
			break;
			case CHUNK_TMAP:
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"TMAP Chunk id : 0x%.8X, Offset : 0x%.8X, Size : %d",chunk->id,offset,chunk->size);
			break;
			case CHUNK_TRKS:
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"TRKS Chunk id : 0x%.8X, Offset : 0x%.8X, Size : %d",chunk->id,offset,chunk->size);
			break;
			case CHUNK_META:
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"META Chunk id : 0x%.8X, Offset : 0x%.8X, Size : %d",chunk->id,offset,chunk->size);
			break;
			case CHUNK_WRIT:
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"WRIT Chunk id : 0x%.8X, Offset : 0x%.8X, Size : %d",chunk->id,offset,chunk->size);
			break;

			default:
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Unknown Chunk id : 0x%.8X, Offset : 0x%.8X, Size : %d",chunk->id,offset,chunk->size);
			break;
		}

		offset += (8 + chunk->size);
	}

	// Get and print the metadatas if available...
	offset = get_woz_chunk(file_buffer,  filesize, 0, CHUNK_META);
	if( offset > 0 )
	{
		chunk = (woz_chunk *)&file_buffer[offset];
		if(chunk->size > 0 &&  chunk->size < 1*1024*1024 )
		{
			tmp_str = calloc( 1, chunk->size + 1);
			if( tmp_str )
			{
				memcpy( tmp_str, chunk->data, chunk->size );
				imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"%s",tmp_str);

				free(tmp_str);
			}
		}
	}

	// Get the Info chunk
	offset = get_woz_chunk(file_buffer,  filesize, 0, CHUNK_INFO);
	if( offset < 0 )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"No INFO Chunk ?!");
		free(file_buffer);
		return HXCFE_BADFILE;
	}

	info = (woz_info *)&file_buffer[offset + 8];

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"INFO Version : %d",info->version);
	imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"Disk type : %d",info->disk_type);
	memset(tmp_str_buf,0,sizeof(tmp_str_buf));
	memcpy(tmp_str_buf,info->creator,32);
	imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"Creator : %s",tmp_str_buf);

	// Get the tracks map array
	offset = get_woz_chunk(file_buffer,  filesize, 0, CHUNK_TMAP);
	if( offset < 0 )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"No TMAPs array ?!");
		free(file_buffer);
		return HXCFE_BADFILE;
	}

	tmap_array_size = ((woz_chunk *)(&file_buffer[offset]))->size; // Should be 160.
	tmap_array = (unsigned char*)&file_buffer[offset + 8];

	// Get the tracks data area
	offset = get_woz_chunk(file_buffer,  filesize, 0, CHUNK_TRKS);
	if( offset < 0 )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"No TRKS area ?!");
		free(file_buffer);
		return HXCFE_BADFILE;
	}

	trks = (woz_trk*)&file_buffer[offset + 8];
	trks_v1 = (woz_trk_v1*)&file_buffer[offset + 8];

	max_track = 0;
	switch( info->disk_type )
	{
		case 1:
			// 5.25"
			for(i=0;i<tmap_array_size/4;i++)
			{
				if(tmap_array[i*4] != 0xFF)
				{
					max_track = i;
				}
			}

			if(	max_track < 36 )
				max_track = 36;

			floppydisk->floppyNumberOfSide = 1;
			rpm = 300;

			intertracks = hxcfe_getEnvVarValue( imgldr_ctx->hxcfe, "WOZLOADER_INTERTRACKSLOADING" );

			if(intertracks)
				max_track = max_track * 4;
		break;

		case 2:
			// 3.5"
			i = (tmap_array_size/2) - 1;
			while( i )
			{
				if(tmap_array[i*2] != 0xFF || tmap_array[(i*2)+1] != 0xFF )
				{
					if( max_track < i )
						max_track = i;
				}
				i--;
			}

			if(	max_track < 82 )
				max_track = 82;

			floppydisk->floppyNumberOfSide = 2;
			rpm = 300;

		break;

		default:
			imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Unsupported disk type : %d", info->disk_type );
			free(file_buffer);
			return HXCFE_BADFILE;
		break;
	}

	floppydisk->floppySectorPerTrack = -1;
	floppydisk->floppyNumberOfTrack = max_track;
	floppydisk->floppyBitRate = 250000;
	floppydisk->floppyiftype = GENERIC_SHUGART_DD_FLOPPYMODE;

	floppydisk->tracks = (HXCFE_CYLINDER**)calloc( 1, sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack );
	if( !floppydisk->tracks )
	{
		free(file_buffer);
		return HXCFE_INTERNALERROR;
	}

	for(j=0;j<floppydisk->floppyNumberOfTrack;j++)
	{
		floppydisk->tracks[j] = allocCylinderEntry(rpm,floppydisk->floppyNumberOfSide);
		currentcylinder=floppydisk->tracks[j];

		for(i=0;i<floppydisk->floppyNumberOfSide;i++)
		{
			hxcfe_imgCallProgressCallback(imgldr_ctx,(j<<1) + (i&1),floppydisk->floppyNumberOfTrack*2 );

			if( info->disk_type == 2 )
			{
				woz_track = tmap_array[(j*2)+i];
			}
			else
			{
				if(intertracks)
					woz_track = tmap_array[j];
				else
					woz_track = tmap_array[j*4];
			}

			if( woz_track != 0xFF )
			{
				if(info->version == 1)
				{
					currentcylinder->sides[i] = tg_alloctrack(floppydisk->floppyBitRate,UNKNOWN_ENCODING,rpm,trks_v1[woz_track].bit_count*2,3800,0,0);
					currentside = currentcylinder->sides[i];

					track_dat =(unsigned char*)&trks_v1[woz_track].bitstream;

					for(k=0;k<trks_v1[woz_track].bit_count;k++)
					{
						if( track_dat[(k>>3)] & (0x80>>(k&7)) )
						{
							currentside->databuffer[(k*2)>>3] |= (0x80>>((k*2)&7));
						}
					}
				}
				else
				{
					currentcylinder->sides[i] = tg_alloctrack(floppydisk->floppyBitRate,UNKNOWN_ENCODING,rpm,trks[woz_track].bit_count*2,3800,0,0);
					currentside = currentcylinder->sides[i];

					if( trks[woz_track].starting_block * 512 < filesize)
					{
						track_dat = &file_buffer[(trks[woz_track].starting_block * 512)];

						for(k=0;k<trks[woz_track].bit_count;k++)
						{
							if( track_dat[(k>>3)] & (0x80>>(k&7)) )
							{
								currentside->databuffer[(k*2)>>3] |= (0x80>>((k*2)&7));
							}
						}
					}

				}
			}
			else
			{
				currentcylinder->sides[i] = tg_alloctrack(floppydisk->floppyBitRate,ISOIBM_MFM_ENCODING,rpm,50000*2,3800,0,TG_ALLOCTRACK_ALLOCFLAKEYBUFFER);
				currentside = currentcylinder->sides[i];

				for(k=0;k<50000/2;k++)
				{
					currentside->databuffer[(k*4)>>3] |= (0x80>>((k*4)&7));
				}

				for(k=0;k<50000*2;k++)
				{
					currentside->flakybitsbuffer[k>>3] |= (0x80>>(k&7));
				}
			}
		}
	}

	hxcfe_sanityCheck(imgldr_ctx->hxcfe,floppydisk);

	imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"track file successfully loaded and encoded!");

	free(file_buffer);

	return HXCFE_NOERROR;
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
