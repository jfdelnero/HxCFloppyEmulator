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
// File : 86f_loader.c
// Contains: 86F floppy image loader
//
// Written by: Jean-François DEL NERO
// Assisted-by: Claude Opus 5.5
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

//#define F86_DBG 1

#define F86_THICK_TRACKS_AUTO_MIN_FILE_TRACKS 4

typedef struct f86_trkinfo_t_
{
	int        present;
	uint16_t   flags;
	uint32_t   nb_of_cells;
	uint32_t   index_position;
	uint32_t   array_size;      // data (and surface) size in bytes
	uint8_t  * data;
	uint8_t  * surface;
}f86_trkinfo_t;

static uint16_t f86_get_u16(uint8_t * p)
{
	return (uint16_t)( p[0] | (p[1] << 8) );
}

static uint32_t f86_get_u32(uint8_t * p)
{
	return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

// Track data array size in bytes. (86Box d86f_get_array_size())
static uint32_t f86_get_array_size(uint16_t disk_flags, int32_t extra_bit_cells)
{
	int rm,ssd,hole;
	int32_t array_size;

	rm   = F86_DISKFLAG_GET_RPM(disk_flags);
	ssd  = (disk_flags & F86_DISKFLAG_TOTAL_BITCELLS) ? 1 : 0;
	hole = F86_DISKFLAG_GET_HOLE(disk_flags);

	if( !rm && ssd )
	{
		// Special case - extra bit cells size specifies entire array size.
		array_size = 0;
	}
	else
	{
		switch( hole )
		{
			default:
			case 0:
			case 1:
				array_size = 12500;
				switch( rm )
				{
					case 1: array_size = ssd ? 12376 : 12625; break;
					case 2: array_size = ssd ? 12315 : 12687; break;
					case 3: array_size = ssd ? 12254 : 12750; break;
					default: break;
				}
			break;

			case 2:
				array_size = 25000;
				switch( rm )
				{
					case 1: array_size = ssd ? 24752 : 25250; break;
					case 2: array_size = ssd ? 24630 : 25375; break;
					case 3: array_size = ssd ? 24509 : 25500; break;
					default: break;
				}
			break;

			case 3:
				array_size = 50000;
				switch( rm )
				{
					case 1: array_size = ssd ? 49504 : 50500; break;
					case 2: array_size = ssd ? 49261 : 50750; break;
					case 3: array_size = ssd ? 49019 : 51000; break;
					default: break;
				}
			break;
		}
	}

	array_size <<= 4;
	array_size += extra_bit_cells;

	if( array_size <= 0 )
		return 0;

	if( array_size & 15 )
		array_size = (array_size >> 4) + 1;
	else
		array_size = (array_size >> 4);

	return (uint32_t)(array_size << 1);
}

static int32_t f86_get_rate(uint16_t track_flags)
{
	// MFM rate. (FM data rate is the half)
	switch( track_flags & F86_TRKFLAG_RATE_MASK )
	{
		case 0: return 500000;
		case 1: return 300000;
		case 2: return 250000;
		case 3: return 1000000;
		case 5: return 2000000;
		default: return 250000;
	}
}

// Track length in bit cells. (86Box common_get_raw_size())
static uint32_t f86_get_raw_size(uint16_t disk_flags, uint16_t track_flags, int32_t extra_bit_cells)
{
	double size,rate,rpm,rpm_diff;
	int rm,ssd;
	int32_t total;

	rm  = F86_DISKFLAG_GET_RPM(disk_flags);
	ssd = (disk_flags & F86_DISKFLAG_TOTAL_BITCELLS) ? 1 : 0;

	if( !rm && ssd && extra_bit_cells )
		return (uint32_t)extra_bit_cells;

	switch( rm )
	{
		case 1:  rpm_diff = 1.01;  break;
		case 2:  rpm_diff = 1.015; break;
		case 3:  rpm_diff = 1.02;  break;
		default: rpm_diff = 1.0;   break;
	}

	if( ssd )
		rpm_diff = 1.0 / rpm_diff;

	rate = (double)f86_get_rate(track_flags) / 1000.0;
	if( F86_TRKFLAG_GET_ENC(track_flags) == F86_TRK_ENC_FM )
		rate /= 2.0;

	rpm = ( (track_flags & F86_TRKFLAG_RPM_MASK) == F86_TRKFLAG_RPM_360 ) ? 360.0 : 300.0;

	size = 100000.0;
	size = (size / 250.0) * rate;
	size = (size * 300.0) / rpm;
	size *= rpm_diff;

	total = (int32_t)( ( ((uint32_t)size) >> 4 ) << 4 ) + extra_bit_cells;
	if( total < 0 )
		total = 0;

	return (uint32_t)total;
}

static int f86_get_bit(uint8_t * buf, uint32_t cell, int reversed)
{
	uint32_t byte_offset;

	// Big endian 16-bit words (MSB first byte per byte) or little endian words.
	byte_offset = cell >> 3;
	if( reversed )
		byte_offset ^= 1;

	return ( buf[byte_offset] >> ( 7 - (cell & 7) ) ) & 1;
}

static int f86_hxc_encoding(uint16_t track_flags)
{
	switch( F86_TRKFLAG_GET_ENC(track_flags) )
	{
		case F86_TRK_ENC_FM:   return ISOIBM_FM_ENCODING;
		case F86_TRK_ENC_MFM:  return ISOIBM_MFM_ENCODING;
		case F86_TRK_ENC_M2FM: return DEC_RX02_M2FM_ENCODING;
		default:               return UNKNOWN_ENCODING;
	}
}

static int f86_same_track(f86_trkinfo_t * t1, f86_trkinfo_t * t2)
{
	if( t1->present != t2->present )
		return 0;

	if( !t1->present )
		return 1;

	if( t1->flags != t2->flags || t1->nb_of_cells != t2->nb_of_cells || t1->array_size != t2->array_size )
		return 0;

	if( memcmp( t1->data, t2->data, t1->array_size ) )
		return 0;

	if( ( t1->surface && !t2->surface ) || ( !t1->surface && t2->surface ) )
		return 0;

	if( t1->surface && memcmp( t1->surface, t2->surface, t1->array_size ) )
		return 0;

	return 1;
}

static HXCFE_SIDE * f86_build_side(HXCFE_IMGLDR * imgldr_ctx, f86_trkinfo_t * trk, int reversed)
{
	HXCFE_SIDE * side;
	int32_t bitrate,rpm;
	uint32_t j,k,o,hxc_cells,step;
	int is_fm,data_bit,surface_bit,last_data_bit;

	last_data_bit = 0;

	is_fm = ( F86_TRKFLAG_GET_ENC(trk->flags) == F86_TRK_ENC_FM );

	bitrate = f86_get_rate(trk->flags);

	// HxC internal FM cells are MFM sized (0C0D) : 2 HxC cells per 86F FM cell.
	step = is_fm ? 2 : 1;
	hxc_cells = trk->nb_of_cells * step;

	rpm = (int32_t)( ( 60.0 * 2.0 * (double)bitrate / (double)hxc_cells ) + 0.5 );

	side = tg_alloctrack( bitrate, f86_hxc_encoding(trk->flags), (short)rpm, hxc_cells, 3800, 0, trk->surface ? TG_ALLOCTRACK_ALLOCFLAKEYBUFFER : 0 );
	if( !side )
		return NULL;

	data_bit = 0;

	// Rotate the track : the index hole position becomes the first bit cell.
	k = trk->index_position % trk->nb_of_cells;
	for( j = 0; j < trk->nb_of_cells; j++ )
	{
		o = ( j * step ) + ( step - 1 );

		last_data_bit = data_bit;
		data_bit = f86_get_bit( trk->data, k, reversed );

		if( trk->surface )
		{
			surface_bit = f86_get_bit( trk->surface, k, reversed );
			if( surface_bit )
			{
				// Weak bit (data bit = 1) or hole (data bit = 0) : noise on read.
				side->flakybitsbuffer[o>>3] |= ( 0x80 >> (o & 7) );
				if( last_data_bit & data_bit )
					data_bit = 0;
			}
		}

		if( data_bit )
			side->databuffer[o>>3] |= ( 0x80 >> (o & 7) );

		k++;
		if( k >= trk->nb_of_cells )
			k = 0;
	}

	return side;
}

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
	int i,entry,nb_entries,nb_sides;
	int file_track,side_number,nb_file_tracks;
	int cyl,nb_cyl,thick,thick_mode;
	int reversed,hole;
	uint8_t * file_buffer;
	uint8_t * p;
	uint32_t file_size,offset,header_size;
	uint16_t disk_flags;
	int32_t extra_bit_cells;
	f86_trkinfo_t * trks;
	f86_trkinfo_t * trk;
	HXCFE_CYLINDER * currentcylinder;

	f = NULL;
	file_buffer = NULL;
	trks = NULL;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"86F_libLoad_DiskFile %s",imgfile);

	f = hxc_fopen(imgfile,"rb");
	if( f == NULL )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	file_size = hxc_fgetsize(f);

	if( file_size < F86_HEADER_SIZE + ( ( F86_TRACK_TABLE_ENTRIES / 2 ) * 4 ) )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"86F file too small : %s !",imgfile);
		hxc_fclose(f);
		return HXCFE_BADFILE;
	}

	file_buffer = malloc( file_size );
	if( !file_buffer )
	{
		hxc_fclose(f);
		return HXCFE_INTERNALERROR;
	}

	if( hxc_fread( file_buffer, file_size, f ) <= 0 )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Can't read %s !",imgfile);
		ret = HXCFE_ACCESSERROR;
		goto error;
	}

	hxc_fclose(f);
	f = NULL;

	if( memcmp( file_buffer, "86BF", 4 ) )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Bad 86F signature !");
		ret = HXCFE_BADFILE;
		goto error;
	}

	disk_flags = f86_get_u16( &file_buffer[6] );

	imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"86F revision : %d.%.2d, disk flags : 0x%.4X",file_buffer[5],file_buffer[4],disk_flags);

	if( file_buffer[5] != F86_FORMAT_MAJOR_VERSION || file_buffer[4] != F86_FORMAT_MINOR_VERSION )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_WARNING,"86F : Unsupported revision %d.%.2d (expected %d.%.2d), trying anyway...",
			file_buffer[5],file_buffer[4],F86_FORMAT_MAJOR_VERSION,F86_FORMAT_MINOR_VERSION);
	}

	if( disk_flags & ( F86_DISKFLAG_ZONED | F86_DISKFLAG_ZONE_TYPE_MASK ) )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"86F : Zoned disks are not supported !");
		ret = HXCFE_UNSUPPORTEDFILE;
		goto error;
	}

	nb_sides = (disk_flags & F86_DISKFLAG_DOUBLE_SIDED) ? 2 : 1;
	nb_entries = (nb_sides == 2) ? F86_TRACK_TABLE_ENTRIES : (F86_TRACK_TABLE_ENTRIES / 2);
	reversed = (disk_flags & F86_DISKFLAG_REVERSE_ENDIAN) ? 1 : 0;
	hole = F86_DISKFLAG_GET_HOLE(disk_flags);
	header_size = (disk_flags & F86_DISKFLAG_BITCELL_COUNT) ? 10 : 6;

	if( file_size < F86_HEADER_SIZE + ( nb_entries * 4 ) )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"86F : Truncated tracks offsets table !");
		ret = HXCFE_FILECORRUPTED;
		goto error;
	}

	trks = calloc( nb_entries, sizeof(f86_trkinfo_t) );
	if( !trks )
	{
		ret = HXCFE_INTERNALERROR;
		goto error;
	}

	// Parse the tracks headers
	nb_file_tracks = 0;
	for( entry = 0; entry < nb_entries; entry++ )
	{
		offset = f86_get_u32( &file_buffer[ F86_HEADER_SIZE + (entry * 4) ] );
		if( !offset )
			continue;

		file_track  = (nb_sides == 2) ? (entry >> 1) : entry;
		side_number = (nb_sides == 2) ? (entry & 1) : 0;

		if( offset >= file_size || ( file_size - offset ) < header_size )
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_WARNING,"86F : Track %d:%d : bad offset (0x%.8X) !",file_track,side_number,offset);
			continue;
		}

		trk = &trks[entry];
		p = &file_buffer[offset];

		trk->flags = f86_get_u16( p );
		p += 2;

		extra_bit_cells = 0;
		if( disk_flags & F86_DISKFLAG_BITCELL_COUNT )
		{
			extra_bit_cells = (int32_t)f86_get_u32( p );
			p += 4;

			if( ( disk_flags & ( F86_DISKFLAG_TOTAL_BITCELLS | F86_DISKFLAG_RPM_MASK ) ) != F86_DISKFLAG_TOTAL_BITCELLS )
			{
				// Extra bit cells count (not total) : limited.
				if( extra_bit_cells < -32768 )
					extra_bit_cells = -32768;
				if( extra_bit_cells > 32768 )
					extra_bit_cells = 32768;
			}
		}

		trk->index_position = f86_get_u32( p );
		p += 4;

		trk->array_size = f86_get_array_size( disk_flags, extra_bit_cells );
		trk->nb_of_cells = f86_get_raw_size( disk_flags, trk->flags, extra_bit_cells );

		if( trk->array_size > F86_MAX_TRACK_WORDS * 2 )
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_WARNING,"86F : Track %d:%d : too long track (%d bytes) !",file_track,side_number,trk->array_size);
		}

		if( trk->nb_of_cells > trk->array_size * 8 )
			trk->nb_of_cells = trk->array_size * 8;

		if( !trk->nb_of_cells )
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_WARNING,"86F : Track %d:%d : empty track !",file_track,side_number);
			continue;
		}

		if( ( file_size - offset - header_size ) < ( trk->array_size * ( (disk_flags & F86_DISKFLAG_SURFACE_DATA) ? 2 : 1 ) ) )
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_WARNING,"86F : Track %d:%d : truncated track data !",file_track,side_number);
			continue;
		}

		trk->data = p;
		if( disk_flags & F86_DISKFLAG_SURFACE_DATA )
			trk->surface = p + trk->array_size;

		trk->present = 1;

		if( file_track + 1 > nb_file_tracks )
			nb_file_tracks = file_track + 1;

#ifdef F86_DBG
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"86F Track %d:%d : flags 0x%.4X, cells: %d, index: %d, array size: %d",
			file_track,side_number,trk->flags,trk->nb_of_cells,trk->index_position,trk->array_size);
#endif
	}

	if( !nb_file_tracks || !trks[0].present )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"86F : No track 0 !");
		ret = HXCFE_FILECORRUPTED;
		goto error;
	}

	// Thin / Thick tracks : 86F images are in thin tracks geometry.
	// 40 tracks disks have each track stored twice -> Keep one track out of two.
	thick_mode = hxcfe_getEnvVarValue( imgldr_ctx->hxcfe, "F86LOADER_THICK_TRACKS" );
	switch( thick_mode )
	{
		case F86_THICK_TRACKS_FORCE:
			thick = 1;
		break;

		case F86_THICK_TRACKS_NEVER:
			thick = 0;
		break;

		default:
			// Auto : all the tracks pairs must be identical.
			thick = 0;
			if( nb_file_tracks >= F86_THICK_TRACKS_AUTO_MIN_FILE_TRACKS && !(nb_file_tracks & 1) )
			{
				thick = 1;
				for( file_track = 0; file_track < nb_file_tracks && thick; file_track += 2 )
				{
					for( side_number = 0; side_number < nb_sides && thick; side_number++ )
					{
						if( nb_sides == 2 )
							thick = f86_same_track( &trks[ (file_track << 1) + side_number ], &trks[ ((file_track + 1) << 1) + side_number ] );
						else
							thick = f86_same_track( &trks[ file_track ], &trks[ file_track + 1 ] );
					}
				}
			}
		break;
	}

	nb_cyl = thick ? ( (nb_file_tracks + 1) / 2 ) : nb_file_tracks;

	if( thick )
		imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"86F : Thick tracks disk (%d file tracks -> %d cylinders)",nb_file_tracks,nb_cyl);

	floppydisk->floppyNumberOfTrack = nb_cyl;
	floppydisk->floppyNumberOfSide = nb_sides;
	floppydisk->floppySectorPerTrack = -1;
	floppydisk->floppyBitRate = f86_get_rate( trks[0].flags );

	switch( hole )
	{
		case 0:  floppydisk->floppyiftype = IBMPC_DD_FLOPPYMODE; break;
		case 1:  floppydisk->floppyiftype = IBMPC_HD_FLOPPYMODE; break;
		default: floppydisk->floppyiftype = IBMPC_ED_FLOPPYMODE; break;
	}

	if( disk_flags & F86_DISKFLAG_WRITE_PROTECT )
		hxcfe_floppySetFlags( imgldr_ctx->hxcfe, floppydisk, HXCFE_FLOPPY_WRPROTECTED_FLAG );

	floppydisk->tracks = (HXCFE_CYLINDER**)calloc( 1, sizeof(HXCFE_CYLINDER*) * nb_cyl );
	if( !floppydisk->tracks )
	{
		ret = HXCFE_INTERNALERROR;
		goto error;
	}

	for( cyl = 0; cyl < nb_cyl; cyl++ )
	{
		file_track = thick ? (cyl * 2) : cyl;

		for( side_number = 0; side_number < nb_sides; side_number++ )
		{
			hxcfe_imgCallProgressCallback( imgldr_ctx, (cyl * nb_sides) + side_number, nb_cyl * nb_sides );

			i = (nb_sides == 2) ? ( (file_track << 1) + side_number ) : file_track;
			trk = &trks[i];

			if( !trk->present )
				continue;

			if( !floppydisk->tracks[cyl] )
			{
				floppydisk->tracks[cyl] = allocCylinderEntry( 300, nb_sides );
				if( !floppydisk->tracks[cyl] )
				{
					ret = HXCFE_INTERNALERROR;
					goto error;
				}
			}

			currentcylinder = floppydisk->tracks[cyl];

			currentcylinder->sides[side_number] = f86_build_side( imgldr_ctx, trk, reversed );
			if( !currentcylinder->sides[side_number] )
			{
				ret = HXCFE_INTERNALERROR;
				goto error;
			}

			currentcylinder->floppyRPM = (int32_t)( ( 60.0 * 2.0 * (double)currentcylinder->sides[side_number]->bitrate / (double)currentcylinder->sides[side_number]->tracklen ) + 0.5 );
		}
	}

	hxcfe_sanityCheck(imgldr_ctx->hxcfe,floppydisk);

	free( trks );
	free( file_buffer );

	return HXCFE_NOERROR;

error:
	if( f )
		hxc_fclose(f);

	free( trks );
	free( file_buffer );

	return ret;
}

int F86_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]="PC_86F";
	static const char plug_desc[]="86Box 86F Loader/Writer";
	static const char plug_ext[]="86f";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   F86_libIsValidDiskFile,
		(LOADDISKFILE)      F86_libLoad_DiskFile,
		(WRITEDISKFILE)     F86_libWrite_DiskFile,
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

