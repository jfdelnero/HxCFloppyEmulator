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
// File : a2r_loader.c
// Contains: AppleSauce A2R Stream floppy image loader
//
// Written by: Jean-François DEL NERO
// Assisted-by: Claude Opus 5.5
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

#include <math.h>

#include "types.h"

#include "internal_libhxcfe.h"
#include "libhxcfe.h"

#include "floppy_loader.h"
#include "floppy_utils.h"

#include "a2r_loader.h"
#include "a2r_writer.h"

#include "a2r_format.h"

#include "libhxcadaptor.h"

#include "misc/env.h"
#include "misc/script_exec.h"

//#define A2RDEBUG 1

#define A2R_MAX_INDEX            ( MAX_NB_OF_INDEX - 8 )
#define A2R_SOLVED_LOOPS         4      // Number of times a solved track is repeated for the analyzer
#define A2R_SRC_STRM             1
#define A2R_SRC_RWCP             2
#define A2R_SRC_SLVD             3

// A2RLOADER_DOUBLE_STEP modes
#define A2R_DOUBLE_STEP_AUTO     0
#define A2R_DOUBLE_STEP_FORCE    1
#define A2R_DOUBLE_STEP_NEVER    2

typedef struct a2r_capt_t_
{
	int        source;          // A2R_SRC_xxx
	int        location;
	int        type;            // capture type
	uint32_t   tick_ps;         // tick period in picoseconds
	uint8_t  * data;            // packed flux data / bitstream (pointer in the file buffer)
	uint32_t   data_size;
	uint8_t  * index;           // 3.x : array of uint32 index signals (pointer in the file buffer)
	int        nb_index;
	uint32_t   loop_point;      // 2.x : estimated loop point (ticks)
	int        mirror_out;      // SLVD
	int        mirror_in;       // SLVD
}a2r_capt_t;

typedef struct a2r_ctx_t_
{
	a2r_capt_t * capts;
	int          nb_capts;
	int          max_capts;

	int          major_version;
	int          drive_type;     // 3.x drive type (2.x disk types are converted)
	int          write_protected;
	int          hard_sector_count;
	int          info_found;
}a2r_ctx_t;

static uint16_t a2r_get_u16(uint8_t * p)
{
	return (uint16_t)( p[0] | ( p[1] << 8 ) );
}

static uint32_t a2r_get_u32(uint8_t * p)
{
	return (uint32_t)p[0] | ( (uint32_t)p[1] << 8 ) | ( (uint32_t)p[2] << 16 ) | ( (uint32_t)p[3] << 24 );
}

int A2R_libIsValidDiskFile( HXCFE_IMGLDR * imgldr_ctx, HXCFE_IMGLDR_FILEINFOS * imgfile )
{
	a2r_header * a2rh;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"A2R_libIsValidDiskFile");

	if( hxc_checkfileext(imgfile->path,"a2r",SYS_PATH_TYPE) )
	{
		a2rh = (a2r_header *)&imgfile->file_header;

		if( ( strncmp((char*)&a2rh->sign,"A2R2",4) && strncmp((char*)&a2rh->sign,"A2R3",4) ) || (a2rh->ff_byte != 0xFF) )
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"A2R_libIsValidDiskFile : Bad file header !");
			return HXCFE_BADFILE;
		}

		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"A2R_libIsValidDiskFile : A2R file !");
		return HXCFE_VALIDFILE;
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"A2R_libIsValidDiskFile : non A2R file !");
		return HXCFE_BADFILE;
	}
}

static int a2r_add_capture(a2r_ctx_t * ctx, a2r_capt_t * capt)
{
	a2r_capt_t * tmp;

	if( ctx->nb_capts >= ctx->max_capts )
	{
		tmp = realloc( ctx->capts, ( ctx->max_capts + 256 ) * sizeof(a2r_capt_t) );
		if( !tmp )
			return -1;

		ctx->capts = tmp;
		ctx->max_capts += 256;
	}

	ctx->capts[ctx->nb_capts++] = *capt;

	return 0;
}

// 2.x STRM chunk
static int a2r_parse_strm(HXCFE * hxcfe, a2r_ctx_t * ctx, uint8_t * chunk, uint32_t size)
{
	uint32_t ofs;
	a2r_capt_t capt;

	ofs = 0;
	while( ofs < size && chunk[ofs] != A2R2_STRM_END )
	{
		if( size - ofs < A2R2_CAPTURE_HEADER_SIZE )
		{
			hxcfe->hxc_printf(MSG_WARNING,"A2R STRM : truncated capture header !");
			break;
		}

		memset( &capt, 0, sizeof(capt) );
		capt.source = A2R_SRC_STRM;
		capt.location = chunk[ofs];
		capt.type = chunk[ofs + 1];
		capt.data_size = a2r_get_u32( &chunk[ofs + 2] );
		capt.loop_point = a2r_get_u32( &chunk[ofs + 6] );
		capt.tick_ps = A2R2_TICK_PERIOD_PS;
		ofs += A2R2_CAPTURE_HEADER_SIZE;

		if( capt.data_size > size - ofs )
		{
			hxcfe->hxc_printf(MSG_WARNING,"A2R STRM : truncated capture data !");
			break;
		}

		capt.data = &chunk[ofs];
		ofs += capt.data_size;

#ifdef A2RDEBUG
		hxcfe->hxc_printf(MSG_DEBUG,"A2R STRM capture : location %d, type %d, %d bytes, loop point %d",capt.location,capt.type,capt.data_size,capt.loop_point);
#endif

		if( a2r_add_capture( ctx, &capt ) )
			return HXCFE_INTERNALERROR;
	}

	return HXCFE_NOERROR;
}

// 3.x RWCP / SLVD chunks
static int a2r_parse_rwcp_slvd(HXCFE * hxcfe, a2r_ctx_t * ctx, uint8_t * chunk, uint32_t size, int slvd)
{
	uint32_t ofs,tick_ps;
	a2r_capt_t capt;
	uint8_t mark;

	if( size < A2R3_CHUNK_SUBHEADER_SIZE )
		return HXCFE_NOERROR;

	tick_ps = a2r_get_u32( &chunk[1] );
	if( !tick_ps )
	{
		hxcfe->hxc_printf(MSG_WARNING,"A2R %s : null resolution ! Using %d ps.",slvd?"SLVD":"RWCP",A2R3_DEFAULT_TICK_PERIOD_PS);
		tick_ps = A2R3_DEFAULT_TICK_PERIOD_PS;
	}

	hxcfe->hxc_printf(MSG_DEBUG,"A2R %s chunk : version %d, resolution %d ps",slvd?"SLVD":"RWCP",chunk[0],tick_ps);

	ofs = A2R3_CHUNK_SUBHEADER_SIZE;
	while( ofs < size )
	{
		mark = chunk[ofs];

		if( mark == A2R3_MARK_END )
			break;

		if( mark != ( slvd ? A2R3_MARK_TRACK : A2R3_MARK_CAPTURE ) )
		{
			hxcfe->hxc_printf(MSG_WARNING,"A2R %s : bad entry mark (0x%.2X) !",slvd?"SLVD":"RWCP",mark);
			break;
		}

		memset( &capt, 0, sizeof(capt) );
		capt.tick_ps = tick_ps;

		if( slvd )
		{
			if( size - ofs < A2R3_SLVD_TRACK_HEADER_SIZE )
				break;

			capt.source = A2R_SRC_SLVD;
			capt.type = A2R_CAPTURE_TIMING;
			capt.location = a2r_get_u16( &chunk[ofs + 1] );
			capt.mirror_out = chunk[ofs + 3];
			capt.mirror_in = chunk[ofs + 4];
			capt.nb_index = chunk[ofs + 11];
			ofs += A2R3_SLVD_TRACK_HEADER_SIZE;
		}
		else
		{
			if( size - ofs < 5 )
				break;

			capt.source = A2R_SRC_RWCP;
			capt.type = chunk[ofs + 1];
			capt.location = a2r_get_u16( &chunk[ofs + 2] );
			capt.nb_index = chunk[ofs + 4];
			ofs += 5;
		}

		if( ( size - ofs ) < ( (uint32_t)capt.nb_index * 4 ) + 4 )
		{
			hxcfe->hxc_printf(MSG_WARNING,"A2R %s : truncated entry header !",slvd?"SLVD":"RWCP");
			break;
		}

		capt.index = &chunk[ofs];
		ofs += capt.nb_index * 4;

		capt.data_size = a2r_get_u32( &chunk[ofs] );
		ofs += 4;

		if( capt.data_size > size - ofs )
		{
			hxcfe->hxc_printf(MSG_WARNING,"A2R %s : truncated entry data !",slvd?"SLVD":"RWCP");
			break;
		}

		capt.data = &chunk[ofs];
		ofs += capt.data_size;

#ifdef A2RDEBUG
		hxcfe->hxc_printf(MSG_DEBUG,"A2R %s entry : location %d, type %d, %d index, %d bytes, mirror %d/%d",slvd?"SLVD":"RWCP",capt.location,capt.type,capt.nb_index,capt.data_size,capt.mirror_out,capt.mirror_in);
#endif

		if( a2r_add_capture( ctx, &capt ) )
			return HXCFE_INTERNALERROR;
	}

	return HXCFE_NOERROR;
}

// Location -> cylinder / side. Returns -1 if the location must be skipped.
static int a2r_location_to_track(a2r_ctx_t * ctx, int location, int skip_inter_tracks, int * cyl, int * side)
{
	if( ctx->drive_type == A2R_DRIVE_525_SS_40_QUARTER )
	{
		*side = 0;
		if( skip_inter_tracks )
		{
			if( location & 3 )
				return -1;

			*cyl = location >> 2;
		}
		else
		{
			*cyl = location;
		}
	}
	else
	{
		*cyl = location >> 1;
		*side = location & 1;
	}

	return 0;
}

static int a2r_capture_rank(a2r_capt_t * capt)
{
	int rank;

	switch( capt->type )
	{
		case A2R_CAPTURE_XTIMING: rank = 3; break;
		case A2R_CAPTURE_TIMING:  rank = 2; break;
		case A2R_CAPTURE_BITS:    rank = 1; break;
		default:                  return -1;
	}

	if( capt->source == A2R_SRC_SLVD )
		rank = 100;

	return rank;
}

// Nominal rotation period (ps) used when the capture doesn't give the index positions.
static double a2r_nominal_period_ps(a2r_ctx_t * ctx, int cyl)
{
	double rpm;

	rpm = 300.0;

	switch( ctx->drive_type )
	{
		case A2R_DRIVE_35_DS_80_APPLE:
			// Apple 3.5 CLV speed zones
			if( cyl < 16 )      rpm = 394.0;
			else if( cyl < 32 ) rpm = 429.0;
			else if( cyl < 48 ) rpm = 472.0;
			else if( cyl < 64 ) rpm = 525.0;
			else                rpm = 590.0;
		break;

		case A2R_DRIVE_8_DS:
			rpm = 360.0;
		break;
	}

	return ( 60.0 / rpm ) * 1E12;
}

// Convert a capture to a flux pulses array (ticks) + index pulse positions, then analyze it.
static HXCFE_SIDE * a2r_import_capture(HXCFE_IMGLDR * imgldr_ctx, a2r_ctx_t * ctx, a2r_capt_t * capt, int cyl, int side_number, float timecoef, int bmpexport, short * rpm)
{
	HXCFE * hxcfe;
	uint32_t * pulses;
	uint32_t * index_ticks;
	uint8_t  * index_main;
	uint32_t nb_pulses,max_pulses;
	uint32_t i,j,k,loop,acc,last;
	uint64_t total_ticks,loop_ticks,t,next_index;
	uint32_t ticks_per_bit;
	int nb_idx,hsc;
	double period_ps;
	HXCFE_FXSA * fxs;
	HXCFE_TRKSTREAM * track_dump;
	HXCFE_SIDE * currentside;
	char tmp_filename[512];

	hxcfe = imgldr_ctx->hxcfe;
	currentside = NULL;
	pulses = NULL;
	index_ticks = NULL;
	index_main = NULL;

	loop = ( capt->source == A2R_SRC_SLVD ) ? A2R_SOLVED_LOOPS : 1;

	// Pulses
	if( capt->type == A2R_CAPTURE_BITS )
		max_pulses = ( capt->data_size * 8 ) + 2;
	else
		max_pulses = ( capt->data_size * loop ) + 2;

	pulses = malloc( max_pulses * sizeof(uint32_t) );
	index_ticks = malloc( MAX_NB_OF_INDEX * sizeof(uint32_t) );
	index_main = malloc( MAX_NB_OF_INDEX * sizeof(uint8_t) );
	if( !pulses || !index_ticks || !index_main )
		goto done;

	nb_pulses = 0;
	total_ticks = 0;

	if( capt->type == A2R_CAPTURE_BITS )
	{
		// Bitstream normalized to 4us bit cells.
		ticks_per_bit = A2R_BITS_CAPTURE_CELL_PS / capt->tick_ps;
		acc = 0;
		for( i = 0; i < capt->data_size * 8; i++ )
		{
			acc += ticks_per_bit;
			if( capt->data[i >> 3] & ( 0x80 >> ( i & 7 ) ) )
			{
				pulses[nb_pulses++] = acc;
				total_ticks += acc;
				acc = 0;
			}
		}
	}
	else
	{
		for( k = 0; k < loop; k++ )
		{
			acc = 0;
			for( i = 0; i < capt->data_size; i++ )
			{
				acc += capt->data[i];
				if( capt->data[i] != 0xFF )
				{
					pulses[nb_pulses++] = acc;
					total_ticks += acc;
					acc = 0;
				}
			}
		}
	}

	if( nb_pulses < 2 )
	{
		hxcfe->hxc_printf(MSG_WARNING,"A2R : Track %d:%d (location %d) : empty capture !",cyl,side_number,capt->location);
		goto done;
	}

	loop_ticks = total_ticks / loop;

	// Index signals (absolute tick positions)
	nb_idx = 0;
	hsc = ctx->hard_sector_count;

	switch( capt->source )
	{
		case A2R_SRC_STRM:
			// Capture starts at the sync sensor trigger, next one at the loop point.
			index_ticks[nb_idx] = 0;
			index_main[nb_idx++] = 1;

			loop_ticks = capt->loop_point;
			if( !loop_ticks )
				loop_ticks = (uint64_t)( a2r_nominal_period_ps( ctx, cyl ) / capt->tick_ps );

			for( t = loop_ticks; loop_ticks && t < total_ticks && nb_idx < A2R_MAX_INDEX; t += loop_ticks )
			{
				index_ticks[nb_idx] = (uint32_t)t;
				index_main[nb_idx++] = 1;
			}
		break;

		case A2R_SRC_RWCP:
			// Capture started on an index signal (not included in the array).
			index_ticks[nb_idx] = 0;
			index_main[nb_idx++] = 1;

			if( capt->nb_index )
			{
				for( i = 0; i < (uint32_t)capt->nb_index && nb_idx < A2R_MAX_INDEX; i++ )
				{
					index_ticks[nb_idx] = a2r_get_u32( &capt->index[i * 4] );
					// Hard sectored disk : hsc + 1 signals per rotation.
					index_main[nb_idx++] = ( hsc ? ( ( i % ( hsc + 1 ) ) == (uint32_t)hsc ) : 1 );
				}
			}
			else
			{
				loop_ticks = (uint64_t)( a2r_nominal_period_ps( ctx, cyl ) / capt->tick_ps );
				for( t = loop_ticks; t < total_ticks && nb_idx < A2R_MAX_INDEX; t += loop_ticks )
				{
					index_ticks[nb_idx] = (uint32_t)t;
					index_main[nb_idx++] = 1;
				}
			}
		break;

		case A2R_SRC_SLVD:
			// Solved track : perfect loop, repeated "loop" times.
			if( capt->nb_index )
			{
				for( k = 0; k < loop; k++ )
				{
					for( i = 0; i < (uint32_t)capt->nb_index && nb_idx < A2R_MAX_INDEX; i++ )
					{
						index_ticks[nb_idx] = (uint32_t)( ( a2r_get_u32( &capt->index[i * 4] ) % loop_ticks ) + ( k * loop_ticks ) );
						index_main[nb_idx++] = ( hsc ? ( ( i % ( hsc + 1 ) ) == (uint32_t)hsc ) : 1 );
					}
				}
			}
			else
			{
				for( k = 0; k < loop && nb_idx < A2R_MAX_INDEX; k++ )
				{
					index_ticks[nb_idx] = (uint32_t)( k * loop_ticks );
					index_main[nb_idx++] = 1;
				}
			}
		break;
	}

	// Pulses positions of the index signals
	fxs = hxcfe_initFxStream( hxcfe );
	if( !fxs )
		goto done;

	period_ps = (double)capt->tick_ps;
	hxcfe_FxStream_setResolution( fxs, (int32_t)period_ps );

	track_dump = hxcfe_FxStream_ImportStream( fxs, pulses, 32, nb_pulses, HXCFE_STREAMCHANNEL_TYPE_RLEEVT, "data", NULL );
	if( track_dump )
	{
		j = 0;
		t = 0;
		last = 0xFFFFFFFF;
		for( i = 0; i < (uint32_t)nb_idx; i++ )
		{
			next_index = index_ticks[i];
			while( j < nb_pulses && t + pulses[j] <= next_index )
			{
				t += pulses[j];
				j++;
			}

			if( j >= nb_pulses )
				break;

			if( j != last || i == 0 )
			{
				hxcfe_FxStream_AddIndex( fxs, track_dump, j, 0, index_main[i] ? FXSTRM_INDEX_MAININDEX : FXSTRM_INDEX_SECTORINDEX );
				last = j;
			}
		}

		hxcfe_FxStream_ChangeSpeed( fxs, track_dump, timecoef );

		currentside = hxcfe_FxStream_AnalyzeAndGetTrack( fxs, track_dump );

		if( currentside )
		{
			if( rpm )
				*rpm = (short)( 60 / GetTrackPeriod( hxcfe, currentside ) );
		}
		else
		{
			hxcfe->hxc_printf(MSG_ERROR,"A2R : Track %d:%d : null track !",cyl,side_number);
		}

		if( bmpexport )
		{
			sprintf( tmp_filename, "track%.2d.%d.bmp", cyl, side_number );
			hxcfe_FxStream_ExportToBmp( fxs, track_dump, tmp_filename );
		}

		if( currentside )
			currentside->stream_dump = track_dump;
		else
			hxcfe_FxStream_FreeStream( fxs, track_dump );
	}

	hxcfe_deinitFxStream( fxs );

done:
	free( pulses );
	free( index_ticks );
	free( index_main );

	return currentside;
}

// Apple drives : remove the sticked data bits (11 -> 10 + flakey bit).
static void a2r_remove_sticked_bits(HXCFE_SIDE * curside)
{
	int k,l,tracklen;
	int previous_bit;

	if( !curside->flakybitsbuffer )
	{
		tracklen = ( curside->tracklen + 7 ) / 8;
		curside->flakybitsbuffer = calloc( 1, tracklen );
		if( !curside->flakybitsbuffer )
			return;
	}

	tracklen = curside->tracklen / 8;
	if( curside->tracklen & 7 )
		tracklen++;

	previous_bit = 0;
	for( k = 0; k < tracklen; k++ )
	{
		if( previous_bit )
		{
			if( curside->databuffer[k] & 0x80 )
			{
				curside->databuffer[k] = curside->databuffer[k] ^ 0x80;
				curside->flakybitsbuffer[k] = curside->flakybitsbuffer[k] | (0x80);
			}
		}

		for( l = 0; l < 7; l++ )
		{
			if( ( curside->databuffer[k] & (0xC0>>l) ) == (0xC0>>l) )
			{
				curside->databuffer[k] = curside->databuffer[k] ^ (0x40>>l);
				curside->flakybitsbuffer[k] = curside->flakybitsbuffer[k] | (0x40>>l);
			}
		}

		previous_bit = curside->databuffer[k] & 1;
	}
}

static void a2r_restore_env(HXCFE_IMGLDR * imgldr_ctx, envvar_entry * backup_env)
{
	envvar_entry * tmp_env;

	if( !backup_env )
		return;

	tmp_env = (envvar_entry *)imgldr_ctx->hxcfe->envvar;
	imgldr_ctx->hxcfe->envvar = backup_env;
	setget_env_script( imgldr_ctx->hxcfe->scriptctx, backup_env );
	deinitEnv( tmp_env );
}

int A2R_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	HXCFE * hxcfe;
	FILE * f;
	int ret,len;
	short rpm;
	int i,j,c,cyl,side_number,loc;
	int singleside,skip_inter_tracks,prefer_solved,double_step_mode,double_step;
	int bmp_export;
	int nb_cyl,nb_sides,max_cyl,has_side1,all_even,nb_used;
	float timecoef;
	char * folder;
	char * filepath;
	char * str;
	uint8_t * file_buffer;
	uint32_t filesize,foffset,chunk_size;
	uint8_t * chunk;
	a2r_ctx_t ctx;
	a2r_capt_t * capt;
	int * best;          // best capture index per track/side
	int * best_rank;
	int * best_revs;
	int nb_slots,slot,rank,revs;
	envvar_entry * backup_env;
	envvar_entry * tmp_env;
	HXCFE_CYLINDER * currentcylinder;
	HXCFE_SIDE * curside;

	hxcfe = imgldr_ctx->hxcfe;

	hxcfe->hxc_printf(MSG_DEBUG,"A2R_libLoad_DiskFile");

	if( !imgfile )
		return HXCFE_BADFILE;

	memset( &ctx, 0, sizeof(ctx) );
	backup_env = NULL;
	file_buffer = NULL;
	folder = NULL;
	best = NULL;
	best_rank = NULL;
	best_revs = NULL;
	ret = HXCFE_NOERROR;

	// Load the whole file
	f = hxc_fopen( imgfile, "rb" );
	if( !f )
		return HXCFE_ACCESSERROR;

	filesize = hxc_fgetsize( f );
	if( filesize < A2R_HEADER_SIZE )
	{
		hxc_fclose( f );
		return HXCFE_BADFILE;
	}

	file_buffer = malloc( filesize );
	if( !file_buffer )
	{
		hxc_fclose( f );
		return HXCFE_INTERNALERROR;
	}

	if( hxc_fread( file_buffer, filesize, f ) <= 0 )
	{
		hxc_fclose( f );
		free( file_buffer );
		return HXCFE_ACCESSERROR;
	}

	hxc_fclose( f );

	if( ( memcmp( file_buffer, "A2R2", 4 ) && memcmp( file_buffer, "A2R3", 4 ) ) || file_buffer[4] != 0xFF )
	{
		hxcfe->hxc_printf(MSG_ERROR,"A2R_libLoad_DiskFile : bad Header !");
		free( file_buffer );
		return HXCFE_BADFILE;
	}

	ctx.major_version = file_buffer[3] - '0';

	if( file_buffer[5] != 0x0A || file_buffer[6] != 0x0D || file_buffer[7] != 0x0A )
		hxcfe->hxc_printf(MSG_WARNING,"A2R : bad LF CR LF header bytes (file translated ?)");

	// Local environment : config.script in the image folder.
	tmp_env = initEnv( (envvar_entry *)hxcfe->envvar, NULL );
	if( !tmp_env )
	{
		free( file_buffer );
		return HXCFE_INTERNALERROR;
	}

	backup_env = hxcfe->envvar;
	hxcfe->envvar = tmp_env;
	setget_env_script( hxcfe->scriptctx, tmp_env );

	len = hxc_getpathfolder( imgfile, 0, SYS_PATH_TYPE );
	folder = (char*)malloc( len + 1 );
	if( !folder )
	{
		ret = HXCFE_INTERNALERROR;
		goto end;
	}

	hxc_getpathfolder( imgfile, folder, SYS_PATH_TYPE );

	filepath = malloc( strlen(imgfile) + 32 );
	if( filepath )
	{
		sprintf( filepath, "%s%s", folder, "config.script" );
		hxcfe_execScriptFile( hxcfe, filepath );
		free( filepath );
	}

	skip_inter_tracks = hxcfe_getEnvVarValue( hxcfe, "A2RLOADER_SKIP_INTER_TRACKS" );
	singleside = hxcfe_getEnvVarValue( hxcfe, "A2RLOADER_SINGLE_SIDE" ) & 1;
	bmp_export = hxcfe_getEnvVarValue( hxcfe, "A2RLOADER_BMPEXPORT" );
	prefer_solved = !( hxcfe_getEnvVarValue( hxcfe, "A2RLOADER_IGNORE_SOLVED_TRACKS" ) & 1 );
	double_step_mode = hxcfe_getEnvVarValue( hxcfe, "A2RLOADER_DOUBLE_STEP" );

	timecoef = 1;
	str = hxcfe_getEnvVar( hxcfe, "FLUXSTREAM_RPMFIX", NULL );
	if( str && !strcmp( str, "360TO300RPM" ) )
		timecoef = (float)1.2;

	if( str && !strcmp( str, "300TO360RPM" ) )
		timecoef = (float)0.833;

	hxcfe->hxc_printf(MSG_INFO_1,"Loading A2R %d.x file...",ctx.major_version);

	// Parse the chunks
	foffset = A2R_HEADER_SIZE;
	while( filesize - foffset >= A2R_CHUNK_HEADER_SIZE )
	{
		chunk_size = a2r_get_u32( &file_buffer[foffset + 4] );
		chunk = &file_buffer[foffset + A2R_CHUNK_HEADER_SIZE];

		if( chunk_size > filesize - foffset - A2R_CHUNK_HEADER_SIZE )
		{
			hxcfe->hxc_printf(MSG_WARNING,"A2R : truncated chunk %.4s (%d bytes, %d available)",&file_buffer[foffset],chunk_size,filesize - foffset - A2R_CHUNK_HEADER_SIZE);
			chunk_size = filesize - foffset - A2R_CHUNK_HEADER_SIZE;
		}

		if( !memcmp( &file_buffer[foffset], "INFO", 4 ) )
		{
			if( chunk_size >= A2R2_INFO_SIZE )
			{
				char creator[33];

				memcpy( creator, &chunk[1], 32 );
				creator[32] = 0;

				ctx.info_found = 1;
				ctx.write_protected = chunk[34];

				if( ctx.major_version >= 3 )
				{
					ctx.drive_type = chunk[33];
					if( chunk_size >= A2R3_INFO_SIZE )
						ctx.hard_sector_count = chunk[36];
				}
				else
				{
					// 2.x disk type
					ctx.drive_type = ( chunk[33] == A2R2_DISK_35 ) ? A2R_DRIVE_35_DS_80_APPLE : A2R_DRIVE_525_SS_40_QUARTER;
				}

				hxcfe->hxc_printf(MSG_INFO_1,"A2R INFO : Version %d, Creator \"%s\", %s %d, write protected %d, synchronized %d, hard sectors %d",
					chunk[0], creator, ( ctx.major_version >= 3 ) ? "drive type" : "disk type", chunk[33], chunk[34], chunk[35], ctx.hard_sector_count );
			}
		}
		else if( !memcmp( &file_buffer[foffset], "STRM", 4 ) )
		{
			ret = a2r_parse_strm( hxcfe, &ctx, chunk, chunk_size );
		}
		else if( !memcmp( &file_buffer[foffset], "RWCP", 4 ) )
		{
			ret = a2r_parse_rwcp_slvd( hxcfe, &ctx, chunk, chunk_size, 0 );
		}
		else if( !memcmp( &file_buffer[foffset], "SLVD", 4 ) )
		{
			ret = a2r_parse_rwcp_slvd( hxcfe, &ctx, chunk, chunk_size, 1 );
		}
		else if( !memcmp( &file_buffer[foffset], "META", 4 ) )
		{
			str = malloc( chunk_size + 1 );
			if( str )
			{
				memcpy( str, chunk, chunk_size );
				str[chunk_size] = 0;
				hxcfe->hxc_printf(MSG_INFO_1,"A2R META :\n%s", str);
				free( str );
			}
		}
		else
		{
			hxcfe->hxc_printf(MSG_DEBUG,"A2R : unknown chunk %.4s (%d bytes) skipped",&file_buffer[foffset],chunk_size);
		}

		if( ret != HXCFE_NOERROR )
			goto end;

		foffset += A2R_CHUNK_HEADER_SIZE + chunk_size;
	}

	if( !ctx.info_found )
	{
		hxcfe->hxc_printf(MSG_WARNING,"A2R : No INFO chunk ! Assuming a 5.25\" disk.");
		ctx.drive_type = A2R_DRIVE_525_SS_40_QUARTER;
	}

	if( ctx.drive_type != A2R_DRIVE_525_SS_40_QUARTER )
		skip_inter_tracks = 0;

	if( !ctx.nb_capts )
	{
		hxcfe->hxc_printf(MSG_ERROR,"A2R : No track data !");
		ret = HXCFE_BADFILE;
		goto end;
	}

	// Geometry
	max_cyl = -1;
	has_side1 = 0;
	all_even = 1;
	nb_used = 0;
	for( i = 0; i < ctx.nb_capts; i++ )
	{
		capt = &ctx.capts[i];

		if( a2r_capture_rank( capt ) < 0 )
			continue;

		for( loc = capt->location - capt->mirror_out; loc <= capt->location + capt->mirror_in; loc++ )
		{
			if( loc < 0 || a2r_location_to_track( &ctx, loc, skip_inter_tracks, &cyl, &side_number ) < 0 )
				continue;

			if( cyl > max_cyl )
				max_cyl = cyl;

			if( side_number )
				has_side1 = 1;

			if( cyl & 1 )
				all_even = 0;

			nb_used++;
		}
	}

	if( max_cyl < 0 )
	{
		hxcfe->hxc_printf(MSG_ERROR,"A2R : No usable track !");
		ret = HXCFE_BADFILE;
		goto end;
	}

	// 40 tracks disk imaged with a 80 tracks drive (only the even cylinders are present) ?
	double_step = 0;
	if( ctx.drive_type != A2R_DRIVE_525_SS_40_QUARTER )
	{
		switch( double_step_mode )
		{
			case A2R_DOUBLE_STEP_FORCE:
				double_step = 1;
			break;

			case A2R_DOUBLE_STEP_NEVER:
				double_step = 0;
			break;

			default:
				double_step = ( all_even && max_cyl >= 2 );
			break;
		}
	}

	if( double_step )
	{
		hxcfe->hxc_printf(MSG_INFO_1,"A2R : only even cylinders present : 40 tracks disk imaged in a 80 tracks drive.");
		max_cyl /= 2;
	}

	nb_cyl = max_cyl + 1;
	nb_sides = ( has_side1 && !singleside ) ? 2 : 1;

	// Select the best capture for each track
	nb_slots = nb_cyl * 2;
	best = malloc( nb_slots * sizeof(int) );
	best_rank = malloc( nb_slots * sizeof(int) );
	best_revs = malloc( nb_slots * sizeof(int) );
	if( !best || !best_rank || !best_revs )
	{
		ret = HXCFE_INTERNALERROR;
		goto end;
	}

	for( i = 0; i < nb_slots; i++ )
	{
		best[i] = -1;
		best_rank[i] = -1;
		best_revs[i] = -1;
	}

	for( i = 0; i < ctx.nb_capts; i++ )
	{
		capt = &ctx.capts[i];

		rank = a2r_capture_rank( capt );
		if( rank < 0 || ( capt->source == A2R_SRC_SLVD && !prefer_solved ) )
			continue;

		revs = ( capt->source == A2R_SRC_RWCP ) ? capt->nb_index : (int)( capt->data_size / 1024 );

		for( loc = capt->location - capt->mirror_out; loc <= capt->location + capt->mirror_in; loc++ )
		{
			if( loc < 0 || a2r_location_to_track( &ctx, loc, skip_inter_tracks, &cyl, &side_number ) < 0 )
				continue;

			if( double_step )
			{
				if( cyl & 1 )
					continue;
				cyl /= 2;
			}

			if( cyl >= nb_cyl || side_number >= nb_sides )
				continue;

			slot = ( cyl * 2 ) + side_number;

			// Higher rank wins, then more revolutions, then the latest capture.
			if( rank > best_rank[slot] || ( rank == best_rank[slot] && revs >= best_revs[slot] ) )
			{
				best[slot] = i;
				best_rank[slot] = rank;
				best_revs[slot] = revs;
			}
		}
	}

	floppydisk->floppyiftype = GENERIC_SHUGART_DD_FLOPPYMODE;
	floppydisk->floppyBitRate = VARIABLEBITRATE;
	floppydisk->floppyNumberOfTrack = nb_cyl;
	floppydisk->floppyNumberOfSide = nb_sides;
	floppydisk->floppySectorPerTrack = -1;

	if( ctx.write_protected )
		hxcfe_floppySetFlags( hxcfe, floppydisk, HXCFE_FLOPPY_WRPROTECTED_FLAG );

	hxcfe->hxc_printf(MSG_INFO_1,"A2R : %d captures/tracks entries, %d cylinder(s), %d side(s), drive type %d",ctx.nb_capts,nb_cyl,nb_sides,ctx.drive_type);

	floppydisk->tracks = (HXCFE_CYLINDER**)calloc( 1, sizeof(HXCFE_CYLINDER*) * nb_cyl );
	if( !floppydisk->tracks )
	{
		ret = HXCFE_INTERNALERROR;
		goto end;
	}

	for( c = 0; c < nb_cyl; c++ )
	{
		for( j = 0; j < nb_sides; j++ )
		{
			slot = ( c * 2 ) + j;

			hxcfe_imgCallProgressCallback( imgldr_ctx, slot, nb_cyl * 2 );

			if( best[slot] < 0 )
				continue;

			capt = &ctx.capts[best[slot]];

			hxcfe->hxc_printf(MSG_DEBUG,"A2R : Track %d:%d <- %s entry, location %d, capture type %d",c,j,
				( capt->source == A2R_SRC_SLVD ) ? "SLVD" : ( ( capt->source == A2R_SRC_RWCP ) ? "RWCP" : "STRM" ),
				capt->location, capt->type );

			rpm = 300;
			curside = a2r_import_capture( imgldr_ctx, &ctx, capt, c, j, timecoef, bmp_export, &rpm );
			if( !curside )
				continue;

			// Apple drives : remove the sticked data bits.
			if( ctx.drive_type == A2R_DRIVE_525_SS_40_QUARTER || ctx.drive_type == A2R_DRIVE_35_DS_80_APPLE )
				a2r_remove_sticked_bits( curside );

			if( !floppydisk->tracks[c] )
			{
				floppydisk->tracks[c] = allocCylinderEntry( rpm, nb_sides );
				if( !floppydisk->tracks[c] )
				{
					ret = HXCFE_INTERNALERROR;
					goto end;
				}
			}

			currentcylinder = floppydisk->tracks[c];
			currentcylinder->sides[j] = curside;
		}
	}

	hxcfe_sanityCheck( hxcfe, floppydisk );

	// Adjust track timings.
	if( floppydisk->tracks[0] && floppydisk->tracks[0]->sides[0] )
	{
		for( c = 0; c < floppydisk->floppyNumberOfTrack; c++ )
		{
			for( j = 0; j < floppydisk->floppyNumberOfSide; j++ )
			{
				curside = floppydisk->tracks[c]->sides[j];
				if( curside && curside != floppydisk->tracks[0]->sides[0] )
				{
					AdjustTrackPeriod( hxcfe, floppydisk->tracks[0]->sides[0], curside );
				}
			}
		}
	}

	hxcfe->hxc_printf(MSG_INFO_1,"track file successfully loaded and encoded!");

end:
	a2r_restore_env( imgldr_ctx, backup_env );

	free( folder );
	free( best );
	free( best_rank );
	free( best_revs );
	free( ctx.capts );
	free( file_buffer );

	return ret;
}

int A2R_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]="A2R_FLUX_STREAM";
	static const char plug_desc[]="A2R Stream Loader/Writer";
	static const char plug_ext[]="a2r";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   A2R_libIsValidDiskFile,
		(LOADDISKFILE)      A2R_libLoad_DiskFile,
		(WRITEDISKFILE)     A2R_libWrite_DiskFile,
		(GETPLUGININFOS)    A2R_libGetPluginInfo
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
