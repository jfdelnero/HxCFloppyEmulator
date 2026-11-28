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
// File : fs_cpc.c
// Contains: Amstrad CPC (AMSDOS) File system manager functions
//
// Written by: Jean-François DEL NERO
// Assisted-by: Claude Opus 5.5
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////
//
// AMSDOS uses a CP/M 2.2 file system on single sided 40 tracks disks,
// 9 sectors of 512 bytes per track :
//
//  - DATA format   : sector IDs 0xC1-0xC9, no reserved track, 180 blocks.
//  - SYSTEM format : sector IDs 0x41-0x49, 2 reserved tracks, 171 blocks.
//
// Both formats use 1KB blocks, 64 directory entries (blocks 0 and 1) and
// 8 bits block numbers (16 blocks / 16KB per directory entry/extent).
//
// Files are stored in 128 bytes records : the size of a file is always a
// multiple of 128 bytes. File contents are read and written as-is (any AMSDOS
// header is kept in the file data).
//
// CP/M user areas 1 to 15 are shown as virtual "USERx" folders. User 0 files
// are at the root.
//
///////////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "types.h"

#include "internal_libhxcfe.h"
#include "tracks/track_generator.h"
#include "sector_search.h"
#include "fdc_ctrl.h"
#include "fs_manager/fs_manager.h"
#include "libhxcfe.h"
#include "fs_manager/fs_manager_access.h"

#include "floppy_loader.h"
#include "floppy_utils.h"

#include "fs_cpc.h"

#define CPC_SECTOR_SIZE         512
#define CPC_SECTORS_PER_TRACK   9
#define CPC_BLOCK_SIZE          1024
#define CPC_SECTORS_PER_BLOCK   ( CPC_BLOCK_SIZE / CPC_SECTOR_SIZE )
#define CPC_RECORD_SIZE         128
#define CPC_DIR_ENTRIES         64
#define CPC_DIR_ENTRY_SIZE      32
#define CPC_DIR_BLOCKS          2
#define CPC_BLOCKS_PER_EXTENT   16
#define CPC_RECORDS_PER_EXTENT  128
#define CPC_EXTENT_SIZE         ( CPC_BLOCKS_PER_EXTENT * CPC_BLOCK_SIZE )
#define CPC_MAX_USER            15
#define CPC_UNUSED_ENTRY        0xE5
#define CPC_EOF_CHAR            0x1A
#define CPC_MAX_HANDLES         128

typedef struct cpcfs_ctx_
{
	const char * format_name;
	uint8_t first_sector_id;
	int32_t reserved_tracks;
	int32_t nb_tracks;
	int32_t dsm;                                // Last block number

	uint8_t dir[CPC_DIR_ENTRIES * CPC_DIR_ENTRY_SIZE];
	uint8_t block_used[256];
}cpcfs_ctx;

typedef struct cpcfs_file_
{
	int32_t user;
	uint8_t name[11];
	int32_t write_mode;

	uint8_t * buffer;
	int32_t size;
	int32_t buffer_size;
	int32_t pos;
}cpcfs_file;

typedef struct cpcfs_dir_
{
	int32_t user;
	int32_t is_root;
	int32_t index;
	int32_t user_index;
}cpcfs_dir;

static const char cpc_invalid_chars[] = "<>.,;:=?*[]|/\\\"";

///////////////////////////////////////////////////////////////////////////////
// Low level sector / block access
///////////////////////////////////////////////////////////////////////////////

static cpcfs_ctx * get_ctx(HXCFE_FSMNG * fsmng)
{
	if(!fsmng)
		return NULL;

	return (cpcfs_ctx *)fsmng->internal;
}

// Logical sector number (from the start of the data area) access.
static int cpc_access_sector(HXCFE_FSMNG * fsmng, cpcfs_ctx * ctx, int32_t lsector, uint8_t * buffer, int write)
{
	int32_t track,sector_id,fdcstatus,ret;

	track = ctx->reserved_tracks + ( lsector / CPC_SECTORS_PER_TRACK );
	sector_id = ctx->first_sector_id + ( lsector % CPC_SECTORS_PER_TRACK );

	if( track >= ctx->nb_tracks || track > 255 )
		return 0;

	fdcstatus = FDC_ACCESS_ERROR;

	if(write)
		ret = hxcfe_writeSectorFDC(fsmng->fdc,(uint8_t)track,0,(uint8_t)sector_id,CPC_SECTOR_SIZE,ISOIBM_MFM_ENCODING,1,buffer,CPC_SECTOR_SIZE,&fdcstatus);
	else
		ret = hxcfe_readSectorFDC(fsmng->fdc,(uint8_t)track,0,(uint8_t)sector_id,CPC_SECTOR_SIZE,ISOIBM_MFM_ENCODING,1,buffer,CPC_SECTOR_SIZE,&fdcstatus);

	if( ret == 1 && fdcstatus == FDC_NOERROR )
		return 1;

	fsmng->hxcfe->hxc_printf(MSG_DEBUG,"CPCFS : %s error ! Track %d, Sector ID 0x%.2X, FDC Status : 0x%.2X",write?"write":"read",track,sector_id,fdcstatus);

	return 0;
}

static int cpc_access_block(HXCFE_FSMNG * fsmng, cpcfs_ctx * ctx, int32_t block, uint8_t * buffer, int write)
{
	int i;

	if( block < 0 || block > ctx->dsm )
		return 0;

	for(i=0;i<CPC_SECTORS_PER_BLOCK;i++)
	{
		if(!cpc_access_sector(fsmng, ctx, (block * CPC_SECTORS_PER_BLOCK) + i, &buffer[i*CPC_SECTOR_SIZE], write))
			return 0;
	}

	return 1;
}

static int cpc_load_dir(HXCFE_FSMNG * fsmng, cpcfs_ctx * ctx)
{
	int i;

	for(i=0;i<CPC_DIR_BLOCKS;i++)
	{
		if(!cpc_access_block(fsmng, ctx, i, &ctx->dir[i*CPC_BLOCK_SIZE], 0))
			return 0;
	}

	return 1;
}

static int cpc_flush_dir(HXCFE_FSMNG * fsmng, cpcfs_ctx * ctx)
{
	int i;

	for(i=0;i<CPC_DIR_BLOCKS;i++)
	{
		if(!cpc_access_block(fsmng, ctx, i, &ctx->dir[i*CPC_BLOCK_SIZE], 1))
			return 0;
	}

	return 1;
}

///////////////////////////////////////////////////////////////////////////////
// Directory helpers
///////////////////////////////////////////////////////////////////////////////

static uint8_t * dir_entry(cpcfs_ctx * ctx, int index)
{
	return &ctx->dir[index * CPC_DIR_ENTRY_SIZE];
}

static int entry_is_file(uint8_t * entry)
{
	return ( entry[0] <= CPC_MAX_USER );
}

static int entry_match(uint8_t * entry, int32_t user, uint8_t * name)
{
	int i;

	if( entry[0] != user )
		return 0;

	for(i=0;i<11;i++)
	{
		if( (entry[1+i] & 0x7F) != (name[i] & 0x7F) )
			return 0;
	}

	return 1;
}

static int32_t entry_extent(uint8_t * entry)
{
	return (entry[12] & 0x1F) + ((entry[14] & 0x3F) * 32);
}

static int32_t entry_records(uint8_t * entry)
{
	if( entry[15] > CPC_RECORDS_PER_EXTENT )
		return CPC_RECORDS_PER_EXTENT;

	return entry[15];
}

static void cpc_build_alloc_map(cpcfs_ctx * ctx)
{
	int i,j;
	uint8_t * entry;

	memset(ctx->block_used,0,sizeof(ctx->block_used));

	for(i=0;i<CPC_DIR_BLOCKS;i++)
		ctx->block_used[i] = 1;

	for(i=0;i<CPC_DIR_ENTRIES;i++)
	{
		entry = dir_entry(ctx,i);

		// Any non free entry may own blocks.
		if( entry[0] == CPC_UNUSED_ENTRY )
			continue;

		for(j=0;j<CPC_BLOCKS_PER_EXTENT;j++)
		{
			if( entry[16+j] && entry[16+j] <= ctx->dsm )
				ctx->block_used[entry[16+j]] = 1;
		}
	}
}

static int32_t cpc_free_blocks(cpcfs_ctx * ctx)
{
	int32_t i,cnt;

	cnt = 0;
	for(i=0;i<=ctx->dsm;i++)
	{
		if(!ctx->block_used[i])
			cnt++;
	}

	return cnt;
}

static int32_t cpc_free_entries(cpcfs_ctx * ctx)
{
	int32_t i,cnt;

	cnt = 0;
	for(i=0;i<CPC_DIR_ENTRIES;i++)
	{
		if( dir_entry(ctx,i)[0] == CPC_UNUSED_ENTRY )
			cnt++;
	}

	return cnt;
}

static int cpc_file_exists(cpcfs_ctx * ctx, int32_t user, uint8_t * name)
{
	int i;

	for(i=0;i<CPC_DIR_ENTRIES;i++)
	{
		if( entry_match(dir_entry(ctx,i), user, name) )
			return 1;
	}

	return 0;
}

static int32_t cpc_file_size(cpcfs_ctx * ctx, int32_t user, uint8_t * name)
{
	int i;
	int32_t last_extent,size;
	uint8_t * entry;

	last_extent = -1;
	size = 0;

	for(i=0;i<CPC_DIR_ENTRIES;i++)
	{
		entry = dir_entry(ctx,i);
		if( entry_match(entry, user, name) && entry_extent(entry) >= last_extent )
		{
			last_extent = entry_extent(entry);
			size = (last_extent * CPC_EXTENT_SIZE) + (entry_records(entry) * CPC_RECORD_SIZE);
		}
	}

	return size;
}

static int cpc_delete_entries(cpcfs_ctx * ctx, int32_t user, uint8_t * name)
{
	int i,cnt;
	uint8_t * entry;

	cnt = 0;
	for(i=0;i<CPC_DIR_ENTRIES;i++)
	{
		entry = dir_entry(ctx,i);
		if( entry_match(entry, user, name) )
		{
			entry[0] = CPC_UNUSED_ENTRY;
			cnt++;
		}
	}

	cpc_build_alloc_map(ctx);

	return cnt;
}

static int cpc_user_has_files(cpcfs_ctx * ctx, int32_t user)
{
	int i;

	for(i=0;i<CPC_DIR_ENTRIES;i++)
	{
		if( dir_entry(ctx,i)[0] == user )
			return 1;
	}

	return 0;
}

static void cpc_format_name(uint8_t * entry, char * outname)
{
	int i,j,len;

	j = 0;
	len = 8;
	while( len && (entry[len] & 0x7F) == ' ' )
		len--;

	for(i=0;i<len;i++)
		outname[j++] = (char)(entry[1+i] & 0x7F);

	len = 3;
	while( len && (entry[8+len] & 0x7F) == ' ' )
		len--;

	if(len)
	{
		outname[j++] = '.';
		for(i=0;i<len;i++)
			outname[j++] = (char)(entry[9+i] & 0x7F);
	}

	outname[j] = 0;
}

///////////////////////////////////////////////////////////////////////////////
// Path parsing
///////////////////////////////////////////////////////////////////////////////

static int is_separator(char c)
{
	return ( c == '/' || c == '\\' );
}

static const char * skip_separators(const char * path)
{
	while( *path && is_separator(*path) )
		path++;

	return path;
}

// Parse a "USERx" folder name. Returns the number of chars used, 0 if not a user folder.
static int parse_user_folder(const char * path, int32_t * user)
{
	int i;
	int32_t value;

	if( strlen(path) < 5 )
		return 0;

	if( ( path[0] != 'U' && path[0] != 'u' ) ||
		( path[1] != 'S' && path[1] != 's' ) ||
		( path[2] != 'E' && path[2] != 'e' ) ||
		( path[3] != 'R' && path[3] != 'r' ) )
		return 0;

	i = 4;
	value = 0;
	while( path[i] >= '0' && path[i] <= '9' && i < 6 )
	{
		value = (value * 10) + (path[i] - '0');
		i++;
	}

	if( i == 4 || value > CPC_MAX_USER )
		return 0;

	if( path[i] && !is_separator(path[i]) )
		return 0;

	*user = value;

	return i;
}

// Parse a directory path : "/" (user 0) or "/USERx".
static int parse_dir_path(const char * path, int32_t * user, int32_t * is_root)
{
	int len;

	if(!path)
		return 0;

	path = skip_separators(path);

	if( !*path )
	{
		*user = 0;
		*is_root = 1;
		return 1;
	}

	len = parse_user_folder(path, user);
	if( !len )
		return 0;

	if( *skip_separators(&path[len]) )
		return 0;

	*is_root = 0;

	return 1;
}

static int is_valid_name_char(char c)
{
	if( (unsigned char)c <= ' ' || (unsigned char)c >= 0x7F )
		return 0;

	if( strchr(cpc_invalid_chars, c) )
		return 0;

	return 1;
}

// Parse a file path : "/NAME.EXT" or "/USERx/NAME.EXT"
static int parse_file_path(const char * path, int32_t * user, uint8_t * name)
{
	int len,i;
	const char * dot;
	const char * filename;
	char c;

	if(!path)
		return 0;

	path = skip_separators(path);

	*user = 0;

	len = parse_user_folder(path, user);
	if( len )
		path = skip_separators(&path[len]);

	filename = path;

	for(i=0;filename[i];i++)
	{
		if( is_separator(filename[i]) )
			return 0;
	}

	dot = strrchr(filename,'.');
	if(!dot)
		dot = filename + strlen(filename);

	len = (int)(dot - filename);
	if( len < 1 || len > 8 )
		return 0;

	if( *dot && ( strlen(dot + 1) > 3 ) )
		return 0;

	memset(name,' ',11);

	for(i=0;i<len;i++)
	{
		c = filename[i];
		if(!is_valid_name_char(c))
			return 0;

		if( c >= 'a' && c <= 'z' )
			c = c - 'a' + 'A';

		name[i] = (uint8_t)c;
	}

	if(*dot)
	{
		dot++;
		for(i=0;dot[i];i++)
		{
			c = dot[i];
			if(!is_valid_name_char(c))
				return 0;

			if( c >= 'a' && c <= 'z' )
				c = c - 'a' + 'A';

			name[8+i] = (uint8_t)c;
		}
	}

	return 1;
}

///////////////////////////////////////////////////////////////////////////////
// File data load / store
///////////////////////////////////////////////////////////////////////////////

static int cpc_load_file(HXCFE_FSMNG * fsmng, cpcfs_ctx * ctx, cpcfs_file * file)
{
	int i,j;
	int32_t offset,bytes,chunk;
	uint8_t * entry;
	uint8_t blockbuf[CPC_BLOCK_SIZE];

	file->size = cpc_file_size(ctx, file->user, file->name);
	file->buffer_size = file->size;
	file->buffer = NULL;

	if(!file->size)
		return 1;

	file->buffer = calloc(1, file->size);
	if(!file->buffer)
		return 0;

	for(i=0;i<CPC_DIR_ENTRIES;i++)
	{
		entry = dir_entry(ctx,i);
		if( !entry_match(entry, file->user, file->name) )
			continue;

		offset = entry_extent(entry) * CPC_EXTENT_SIZE;
		bytes = entry_records(entry) * CPC_RECORD_SIZE;

		for(j=0;j<CPC_BLOCKS_PER_EXTENT && bytes > 0;j++)
		{
			chunk = bytes;
			if( chunk > CPC_BLOCK_SIZE )
				chunk = CPC_BLOCK_SIZE;

			if( offset + chunk > file->size )
				chunk = file->size - offset;

			if( chunk <= 0 )
				break;

			if( entry[16+j] )
			{
				if(!cpc_access_block(fsmng, ctx, entry[16+j], blockbuf, 0))
				{
					free(file->buffer);
					file->buffer = NULL;
					return 0;
				}

				memcpy(&file->buffer[offset], blockbuf, chunk);
			}

			offset += chunk;
			bytes -= chunk;
		}
	}

	return 1;
}

// Maximum file size which can be stored for a file being written.
static int32_t cpc_max_write_size(cpcfs_ctx * ctx)
{
	int32_t blocks,entries;

	blocks = cpc_free_blocks(ctx);
	// The file own entry (created by createFile) is reused.
	entries = cpc_free_entries(ctx) + 1;

	if( blocks > entries * CPC_BLOCKS_PER_EXTENT )
		blocks = entries * CPC_BLOCKS_PER_EXTENT;

	return blocks * CPC_BLOCK_SIZE;
}

static int cpc_store_file(HXCFE_FSMNG * fsmng, cpcfs_ctx * ctx, cpcfs_file * file)
{
	int32_t records,nb_blocks,nb_extents;
	int32_t i,j,k,block,offset,chunk,remaining_records,entry_idx;
	uint8_t * entry;
	uint8_t * blocks;
	uint8_t blockbuf[CPC_BLOCK_SIZE];
	uint8_t saved_dir[sizeof(ctx->dir)];

	records = ( file->size + (CPC_RECORD_SIZE - 1) ) / CPC_RECORD_SIZE;
	nb_blocks = ( (records * CPC_RECORD_SIZE) + (CPC_BLOCK_SIZE - 1) ) / CPC_BLOCK_SIZE;
	nb_extents = ( nb_blocks + (CPC_BLOCKS_PER_EXTENT - 1) ) / CPC_BLOCKS_PER_EXTENT;
	if(!nb_extents)
		nb_extents = 1;

	memcpy(saved_dir, ctx->dir, sizeof(ctx->dir));

	// Release the current file entries
	cpc_delete_entries(ctx, file->user, file->name);

	if( cpc_free_blocks(ctx) < nb_blocks || cpc_free_entries(ctx) < nb_extents )
	{
		fsmng->hxcfe->hxc_printf(MSG_ERROR,"CPCFS : Not enough space to store the file !");
		memcpy(ctx->dir, saved_dir, sizeof(ctx->dir));
		cpc_build_alloc_map(ctx);
		return 0;
	}

	blocks = calloc(1, nb_blocks + 1);
	if(!blocks)
	{
		memcpy(ctx->dir, saved_dir, sizeof(ctx->dir));
		cpc_build_alloc_map(ctx);
		return 0;
	}

	// Allocate and write the data blocks
	block = CPC_DIR_BLOCKS;
	offset = 0;
	for(i=0;i<nb_blocks;i++)
	{
		while( block <= ctx->dsm && ctx->block_used[block] )
			block++;

		blocks[i] = (uint8_t)block;
		ctx->block_used[block] = 1;

		memset(blockbuf, CPC_EOF_CHAR, CPC_BLOCK_SIZE);

		chunk = file->size - offset;
		if( chunk > CPC_BLOCK_SIZE )
			chunk = CPC_BLOCK_SIZE;

		if( chunk > 0 )
			memcpy(blockbuf, &file->buffer[offset], chunk);

		if(!cpc_access_block(fsmng, ctx, block, blockbuf, 1))
		{
			free(blocks);
			memcpy(ctx->dir, saved_dir, sizeof(ctx->dir));
			cpc_build_alloc_map(ctx);
			return 0;
		}

		offset += CPC_BLOCK_SIZE;
	}

	// Create the directory entries
	remaining_records = records;
	entry_idx = 0;
	k = 0;
	for(i=0;i<nb_extents;i++)
	{
		while( dir_entry(ctx,entry_idx)[0] != CPC_UNUSED_ENTRY )
			entry_idx++;

		entry = dir_entry(ctx,entry_idx);
		memset(entry, 0, CPC_DIR_ENTRY_SIZE);

		entry[0] = (uint8_t)file->user;
		memcpy(&entry[1], file->name, 11);
		entry[12] = (uint8_t)(i & 0x1F);
		entry[13] = 0x00;
		entry[14] = (uint8_t)((i >> 5) & 0x3F);

		if( remaining_records > CPC_RECORDS_PER_EXTENT )
			entry[15] = CPC_RECORDS_PER_EXTENT;
		else
			entry[15] = (uint8_t)remaining_records;

		remaining_records -= entry[15];

		for(j=0;j<CPC_BLOCKS_PER_EXTENT && k<nb_blocks;j++)
		{
			entry[16+j] = blocks[k++];
		}
	}

	free(blocks);

	if(!cpc_flush_dir(fsmng, ctx))
		return 0;

	return 1;
}

///////////////////////////////////////////////////////////////////////////////
// File system manager interface
///////////////////////////////////////////////////////////////////////////////

void init_cpc(HXCFE_FSMNG * fsmng)
{
}

static int cpc_probe_format(HXCFE_FSMNG * fsmng, cpcfs_ctx * ctx, uint8_t first_sector_id, int32_t reserved_tracks, int32_t dsm, const char * name)
{
	int32_t i,fdcstatus,max_blocks;
	uint8_t sectorbuffer[CPC_SECTOR_SIZE];

	if( fsmng->trackperdisk <= reserved_tracks )
		return 0;

	// The whole directory track must be readable.
	for(i=0;i<CPC_SECTORS_PER_TRACK;i++)
	{
		fdcstatus = FDC_ACCESS_ERROR;
		if( hxcfe_readSectorFDC(fsmng->fdc,(uint8_t)reserved_tracks,0,(uint8_t)(first_sector_id + i),CPC_SECTOR_SIZE,ISOIBM_MFM_ENCODING,1,sectorbuffer,sizeof(sectorbuffer),&fdcstatus) != 1 )
			return 0;

		if( fdcstatus != FDC_NOERROR )
			return 0;
	}

	ctx->format_name = name;
	ctx->first_sector_id = first_sector_id;
	ctx->reserved_tracks = reserved_tracks;
	ctx->nb_tracks = fsmng->trackperdisk;

	// Limit the block count to the available tracks.
	max_blocks = ( (ctx->nb_tracks - reserved_tracks) * CPC_SECTORS_PER_TRACK ) / CPC_SECTORS_PER_BLOCK;
	if( dsm >= max_blocks )
		dsm = max_blocks - 1;

	ctx->dsm = dsm;

	return ( ctx->dsm >= CPC_DIR_BLOCKS );
}

int32_t cpc_umountImage(HXCFE_FSMNG * fsmng);

int32_t cpc_mountImage(HXCFE_FSMNG * fsmng, HXCFE_FLOPPY *floppy)
{
	cpcfs_ctx * ctx;

	if(!floppy || !fsmng)
		return HXCFE_BADPARAMETER;

	if(fsmng->internal)
		cpc_umountImage(fsmng);

	fsmng->fp = floppy;

	fsmng->trackperdisk = fsmng->fp->floppyNumberOfTrack;
	fsmng->sectorpertrack = CPC_SECTORS_PER_TRACK;
	fsmng->sidepertrack = 1;
	fsmng->sectorsize = CPC_SECTOR_SIZE;

	if(fsmng->fdc)
		hxcfe_deinitFDC (fsmng->fdc);

	fsmng->fdc = hxcfe_initFDC (fsmng->hxcfe);
	if(!fsmng->fdc)
		return HXCFE_INTERNALERROR;

	if(hxcfe_insertDiskFDC (fsmng->fdc,floppy) != HXCFE_NOERROR)
		goto error;

	ctx = calloc(1, sizeof(cpcfs_ctx));
	if(!ctx)
		goto error;

	if( !cpc_probe_format(fsmng, ctx, 0xC1, 0, 179, "DATA") &&
		!cpc_probe_format(fsmng, ctx, 0x41, 2, 170, "SYSTEM") )
	{
		fsmng->hxcfe->hxc_printf(MSG_DEBUG,"CPCFS : No AMSDOS DATA/SYSTEM format found");
		free(ctx);
		goto error;
	}

	if(!cpc_load_dir(fsmng, ctx))
	{
		fsmng->hxcfe->hxc_printf(MSG_DEBUG,"CPCFS : Directory read error");
		free(ctx);
		goto error;
	}

	cpc_build_alloc_map(ctx);

	// The handle tables are shared with the other file systems managers
	// (and may have been filled by a previous mount attempt) : reset them.
	memset(fsmng->handletable,0,sizeof(fsmng->handletable));
	memset(fsmng->dirhandletable,0,sizeof(fsmng->dirhandletable));

	fsmng->internal = ctx;

	fsmng->hxcfe->hxc_printf(MSG_DEBUG,"CPCFS : AMSDOS %s format mounted (%d tracks, %d blocks)",ctx->format_name,ctx->nb_tracks,ctx->dsm + 1);

	return HXCFE_NOERROR;

error:
	hxcfe_deinitFDC (fsmng->fdc);
	fsmng->fdc = 0;

	return HXCFE_INTERNALERROR;
}

int32_t cpc_closeFile( HXCFE_FSMNG * fsmng,int32_t filehandle);
int32_t cpc_closeDir(HXCFE_FSMNG * fsmng, int32_t dirhandle);

int32_t cpc_umountImage(HXCFE_FSMNG * fsmng)
{
	int i;

	if(fsmng->internal)
	{
		// Flush and close all the remaining handles.
		for(i=0;i<CPC_MAX_HANDLES;i++)
		{
			if(fsmng->handletable[i])
				cpc_closeFile(fsmng, i + 1);

			if(fsmng->dirhandletable[i])
				cpc_closeDir(fsmng, i + 1);
		}

		free(fsmng->internal);
		fsmng->internal = 0;
	}

	if(fsmng->fdc)
	{
		hxcfe_deinitFDC (fsmng->fdc);
		fsmng->fdc = 0;
	}

	return HXCFE_NOERROR;
}

int32_t cpc_getFreeSpace(HXCFE_FSMNG * fsmng)
{
	cpcfs_ctx * ctx;

	ctx = get_ctx(fsmng);
	if(!ctx)
		return HXCFE_ACCESSERROR;

	return cpc_free_blocks(ctx) * CPC_BLOCK_SIZE;
}

int32_t cpc_getTotalSpace(HXCFE_FSMNG * fsmng)
{
	cpcfs_ctx * ctx;

	ctx = get_ctx(fsmng);
	if(!ctx)
		return HXCFE_ACCESSERROR;

	return ( (ctx->dsm + 1) - CPC_DIR_BLOCKS ) * CPC_BLOCK_SIZE;
}

int32_t cpc_openDir(HXCFE_FSMNG * fsmng, char * path)
{
	cpcfs_dir * dir;
	int32_t i,user,is_root;

	if(!get_ctx(fsmng))
		return HXCFE_ACCESSERROR;

	if(!parse_dir_path(path, &user, &is_root))
		return HXCFE_ACCESSERROR;

	i = 0;
	while( i < CPC_MAX_HANDLES && fsmng->dirhandletable[i] )
	{
		i++;
	}

	if( i == CPC_MAX_HANDLES )
		return HXCFE_ACCESSERROR;

	dir = calloc(1, sizeof(cpcfs_dir));
	if(!dir)
		return HXCFE_ACCESSERROR;

	dir->user = user;
	dir->is_root = is_root;
	dir->index = 0;
	dir->user_index = 1;

	fsmng->dirhandletable[i] = dir;

	return i + 1;
}

int32_t cpc_readDir(HXCFE_FSMNG * fsmng,int32_t dirhandle,HXCFE_FSENTRY * dirent)
{
	cpcfs_ctx * ctx;
	cpcfs_dir * dir;
	uint8_t * entry;
	int i,duplicate;

	ctx = get_ctx(fsmng);

	if( !ctx || !dirent || dirhandle <= 0 || dirhandle > CPC_MAX_HANDLES )
		return HXCFE_ACCESSERROR;

	dir = (cpcfs_dir *)fsmng->dirhandletable[dirhandle-1];
	if(!dir)
		return HXCFE_ACCESSERROR;

	// Files of the current user area. A file may own several entries (extents) : list it once.
	while( dir->index < CPC_DIR_ENTRIES )
	{
		entry = dir_entry(ctx, dir->index);
		dir->index++;

		if( !entry_is_file(entry) || entry[0] != dir->user )
			continue;

		duplicate = 0;
		for(i=0;i<dir->index - 1;i++)
		{
			if( entry_match(dir_entry(ctx,i), entry[0], &entry[1]) )
			{
				duplicate = 1;
				break;
			}
		}

		if(duplicate)
			continue;

		memset(dirent, 0, sizeof(HXCFE_FSENTRY));
		cpc_format_name(entry, dirent->entryname);
		dirent->size = cpc_file_size(ctx, entry[0], &entry[1]);
		dirent->isdir = 0;
		dirent->flags = 0;

		return HXCFE_VALIDFILE;
	}

	// Root folder : the other non empty user areas as virtual folders.
	if( dir->is_root )
	{
		while( dir->user_index <= CPC_MAX_USER )
		{
			i = dir->user_index;
			dir->user_index++;

			if( cpc_user_has_files(ctx, i) )
			{
				memset(dirent, 0, sizeof(HXCFE_FSENTRY));
				snprintf(dirent->entryname, sizeof(dirent->entryname), "USER%d", i);
				dirent->size = 0;
				dirent->isdir = 1;
				dirent->flags = 0;

				return HXCFE_VALIDFILE;
			}
		}
	}

	return HXCFE_NOERROR;
}

int32_t cpc_closeDir(HXCFE_FSMNG * fsmng, int32_t dirhandle)
{
	if( dirhandle > 0 && dirhandle <= CPC_MAX_HANDLES )
	{
		if(fsmng->dirhandletable[dirhandle-1])
		{
			free(fsmng->dirhandletable[dirhandle-1]);
			fsmng->dirhandletable[dirhandle-1] = 0;
			return HXCFE_NOERROR;
		}
	}

	return HXCFE_ACCESSERROR;
}

static int32_t cpc_alloc_file_handle(HXCFE_FSMNG * fsmng, char * filename, int write_mode)
{
	cpcfs_ctx * ctx;
	cpcfs_file * file;
	int32_t i;

	ctx = get_ctx(fsmng);
	if(!ctx)
		return HXCFE_ACCESSERROR;

	i = 0;
	while( i < CPC_MAX_HANDLES && fsmng->handletable[i] )
	{
		i++;
	}

	if( i == CPC_MAX_HANDLES )
		return HXCFE_ACCESSERROR;

	file = calloc(1, sizeof(cpcfs_file));
	if(!file)
		return HXCFE_ACCESSERROR;

	if(!parse_file_path(filename, &file->user, file->name))
	{
		fsmng->hxcfe->hxc_printf(MSG_DEBUG,"CPCFS : Invalid file name : %s",filename);
		free(file);
		return HXCFE_ACCESSERROR;
	}

	file->write_mode = write_mode;

	if(!write_mode)
	{
		if( !cpc_file_exists(ctx, file->user, file->name) || !cpc_load_file(fsmng, ctx, file) )
		{
			free(file);
			return HXCFE_ACCESSERROR;
		}
	}
	else
	{
		uint8_t * entry;
		int idx;

		// Truncate an existing file and create an empty entry for the new one.
		cpc_delete_entries(ctx, file->user, file->name);

		for(idx=0;idx<CPC_DIR_ENTRIES;idx++)
		{
			if( dir_entry(ctx,idx)[0] == CPC_UNUSED_ENTRY )
				break;
		}

		if( idx == CPC_DIR_ENTRIES )
		{
			fsmng->hxcfe->hxc_printf(MSG_ERROR,"CPCFS : Directory full !");
			cpc_flush_dir(fsmng, ctx);
			free(file);
			return HXCFE_ACCESSERROR;
		}

		entry = dir_entry(ctx,idx);
		memset(entry, 0, CPC_DIR_ENTRY_SIZE);
		entry[0] = (uint8_t)file->user;
		memcpy(&entry[1], file->name, 11);

		if(!cpc_flush_dir(fsmng, ctx))
		{
			free(file);
			return HXCFE_ACCESSERROR;
		}
	}

	fsmng->handletable[i] = file;

	return i + 1;
}

static cpcfs_file * get_file(HXCFE_FSMNG * fsmng, int32_t filehandle)
{
	if( !fsmng || filehandle <= 0 || filehandle > CPC_MAX_HANDLES )
		return NULL;

	return (cpcfs_file *)fsmng->handletable[filehandle-1];
}

int32_t cpc_openFile(HXCFE_FSMNG * fsmng, char * filename)
{
	return cpc_alloc_file_handle(fsmng, filename, 0);
}

int32_t cpc_createFile(HXCFE_FSMNG * fsmng, char * filename)
{
	return cpc_alloc_file_handle(fsmng, filename, 1);
}

int32_t cpc_writeFile(HXCFE_FSMNG * fsmng,int32_t filehandle,unsigned char * buffer,int32_t size)
{
	cpcfs_ctx * ctx;
	cpcfs_file * file;
	int32_t max_size,new_size;
	uint8_t * new_buffer;

	ctx = get_ctx(fsmng);
	file = get_file(fsmng, filehandle);

	if( !ctx || !file || !file->write_mode || !buffer || size < 0 )
		return HXCFE_ACCESSERROR;

	max_size = cpc_max_write_size(ctx);

	if( file->pos >= max_size )
		return 0;

	if( file->pos + size > max_size )
		size = max_size - file->pos;

	new_size = file->pos + size;

	if( new_size > file->buffer_size )
	{
		new_buffer = realloc(file->buffer, new_size);
		if(!new_buffer)
			return HXCFE_ACCESSERROR;

		memset(&new_buffer[file->buffer_size], 0, new_size - file->buffer_size);

		file->buffer = new_buffer;
		file->buffer_size = new_size;
	}

	memcpy(&file->buffer[file->pos], buffer, size);

	file->pos += size;
	if( file->pos > file->size )
		file->size = file->pos;

	return size;
}

int32_t cpc_readFile( HXCFE_FSMNG * fsmng,int32_t filehandle,unsigned char * buffer,int32_t size)
{
	cpcfs_file * file;

	file = get_file(fsmng, filehandle);

	if( !file || !buffer || size < 0 )
		return HXCFE_ACCESSERROR;

	if( file->pos >= file->size )
		return 0;

	if( file->pos + size > file->size )
		size = file->size - file->pos;

	memcpy(buffer, &file->buffer[file->pos], size);

	file->pos += size;

	return size;
}

int32_t cpc_deleteFile(HXCFE_FSMNG * fsmng, char * filename)
{
	cpcfs_ctx * ctx;
	int32_t user;
	uint8_t name[11];

	ctx = get_ctx(fsmng);
	if(!ctx)
		return HXCFE_ACCESSERROR;

	if(!parse_file_path(filename, &user, name))
		return HXCFE_ACCESSERROR;

	if( !cpc_delete_entries(ctx, user, name) )
		return HXCFE_ACCESSERROR;

	if(!cpc_flush_dir(fsmng, ctx))
		return HXCFE_ACCESSERROR;

	return HXCFE_NOERROR;
}

int32_t cpc_closeFile( HXCFE_FSMNG * fsmng,int32_t filehandle)
{
	cpcfs_ctx * ctx;
	cpcfs_file * file;
	int32_t ret;

	ctx = get_ctx(fsmng);
	file = get_file(fsmng, filehandle);

	if( !file )
		return HXCFE_ACCESSERROR;

	ret = HXCFE_NOERROR;

	if( file->write_mode )
	{
		if( !ctx || !cpc_store_file(fsmng, ctx, file) )
			ret = HXCFE_ACCESSERROR;
	}

	free(file->buffer);
	free(file);

	fsmng->handletable[filehandle-1] = 0;

	return ret;
}

int32_t cpc_createDir( HXCFE_FSMNG * fsmng,char * foldername)
{
	int32_t user,is_root;

	if(!get_ctx(fsmng))
		return HXCFE_ACCESSERROR;

	// Only the virtual user area folders can be "created".
	if( parse_dir_path(foldername, &user, &is_root) && !is_root )
		return HXCFE_NOERROR;

	return HXCFE_ACCESSERROR;
}

int32_t cpc_removeDir( HXCFE_FSMNG * fsmng,char * foldername)
{
	cpcfs_ctx * ctx;
	int32_t user,is_root;

	ctx = get_ctx(fsmng);
	if(!ctx)
		return HXCFE_ACCESSERROR;

	// A user area folder can be removed only once empty.
	if( parse_dir_path(foldername, &user, &is_root) && !is_root )
	{
		if( !cpc_user_has_files(ctx, user) )
			return HXCFE_NOERROR;
	}

	return HXCFE_ACCESSERROR;
}

int32_t cpc_ftell( HXCFE_FSMNG * fsmng,int32_t filehandle)
{
	cpcfs_file * file;

	file = get_file(fsmng, filehandle);
	if(!file)
		return HXCFE_ACCESSERROR;

	return file->pos;
}

int32_t cpc_fseek( HXCFE_FSMNG * fsmng,int32_t filehandle,int32_t offset,int32_t origin)
{
	cpcfs_file * file;
	int32_t newpos;

	file = get_file(fsmng, filehandle);
	if(!file)
		return HXCFE_ACCESSERROR;

	switch(origin)
	{
		case SEEK_SET:
			newpos = offset;
		break;
		case SEEK_CUR:
			newpos = file->pos + offset;
		break;
		case SEEK_END:
			newpos = file->size + offset;
		break;
		default:
			return HXCFE_ACCESSERROR;
	}

	if( newpos < 0 )
		return HXCFE_ACCESSERROR;

	if( !file->write_mode && newpos > file->size )
		newpos = file->size;

	file->pos = newpos;

	return HXCFE_NOERROR;
}

int cpc_libGetPluginInfo(HXCFE_FSMNG * fsmng_ctx, uint32_t infotype, void * returnvalue)
{
	static const char fs_plug_id[] = "FS_CPC";
	static const char fs_plug_desc[] = "Amstrad CPC AMSDOS";

	fs_plugins_ptr fs_plug_funcs=
	{
		.fsmng_init              = (fn_fsmng_init)            init_cpc,

		.fsmng_mountImage        = (fn_fsmng_mountImage)      cpc_mountImage,
		.fsmng_umountImage       = (fn_fsmng_umountImage)     cpc_umountImage,

		.fsmng_getFreeSpace      = (fn_fsmng_getFreeSpace)    cpc_getFreeSpace,
		.fsmng_getTotalSpace     = (fn_fsmng_getTotalSpace)   cpc_getTotalSpace,
		.fsmng_deleteFile        = (fn_fsmng_deleteFile)      cpc_deleteFile,

		.fsmng_openFile          = (fn_fsmng_openFile)        cpc_openFile,
		.fsmng_closeFile         = (fn_fsmng_closeFile)       cpc_closeFile,

		.fsmng_createFile        = (fn_fsmng_createFile)      cpc_createFile,
		.fsmng_writeFile         = (fn_fsmng_writeFile)       cpc_writeFile,
		.fsmng_readFile          = (fn_fsmng_readFile)        cpc_readFile,
		.fsmng_ftell             = (fn_fsmng_ftell)           cpc_ftell,
		.fsmng_fseek             = (fn_fsmng_fseek)           cpc_fseek,
		.fsmng_createDir         = (fn_fsmng_createDir)       cpc_createDir,
		.fsmng_removeDir         = (fn_fsmng_removeDir)       cpc_removeDir,
		.fsmng_openDir           = (fn_fsmng_openDir)         cpc_openDir,
		.fsmng_readDir           = (fn_fsmng_readDir)         cpc_readDir,
		.fsmng_closeDir          = (fn_fsmng_closeDir)        cpc_closeDir,

		.fsmng_getplugininfos    = (fn_fsmng_getplugininfos)  cpc_libGetPluginInfo
	};

	return libGetFSPluginInfo(
			fsmng_ctx,
			infotype,
			returnvalue,
			fs_plug_id,
			fs_plug_desc,
			&fs_plug_funcs
			);
}
