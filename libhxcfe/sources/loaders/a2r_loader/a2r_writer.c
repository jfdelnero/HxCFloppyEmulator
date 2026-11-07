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
// File : a2r_writer.c
// Contains: AppleSauce A2R 3.x Stream floppy image writer
//
// Written by: Jean-François DEL NERO
// Assisted-by: Claude Opus 5.5
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "types.h"

#include "internal_libhxcfe.h"
#include "libhxcfe.h"

#include "floppy_loader.h"
#include "floppy_utils.h"

#include "a2r_loader.h"
#include "a2r_writer.h"
#include "a2r_format.h"

#include "libhxcadaptor.h"

//
// A2R 3.x writer.
//
// File content :
//  - INFO chunk.
//  - RWCP chunk : one "xtiming" capture per track : index synchronized,
//    N revolutions + 1/4 revolution (A2REXPORT_NUMBER_OF_REVOLUTIONS),
//    with an index signal at the end of each revolution.
//    Weak bits are randomized at each revolution.
//  - SLVD chunk : one solved (single revolution, perfect loop) track entry
//    per track.
//  - META chunk.
//
// Flux timings resolution : 62.5 ns (Applesauce default).
//

#define A2R_WRITER_DEFAULT_REVS     2
#define A2R_WRITER_MAX_REVS         16
#define A2R_WRITER_MAX_INDEX        255

typedef struct a2r_wbuf_t_
{
	uint8_t * data;
	uint32_t  size;
	uint32_t  allocated;
	int       error;
}a2r_wbuf_t;

typedef struct a2r_wtrk_t_
{
	double   * pulse_ps;        // pulses times (ps) from the track start (main index)
	uint8_t  * pulse_weak;      // weak pulse/cell
	uint32_t   nb_pulses;
	double     period_ps;       // rotation period
	double     edges_ps[A2R_WRITER_MAX_INDEX]; // index signals rising edges (ps) from the track start
	int        nb_edges;
}a2r_wtrk_t;

static void a2r_wbuf_put(a2r_wbuf_t * b, const void * data, uint32_t size)
{
	uint8_t * tmp;
	uint32_t new_size;

	if( b->error || !size )
		return;

	if( b->size + size > b->allocated )
	{
		new_size = b->allocated ? b->allocated * 2 : 65536;
		while( new_size < b->size + size )
			new_size *= 2;

		tmp = realloc( b->data, new_size );
		if( !tmp )
		{
			b->error = 1;
			return;
		}

		b->data = tmp;
		b->allocated = new_size;
	}

	memcpy( &b->data[b->size], data, size );
	b->size += size;
}

static void a2r_wbuf_u8(a2r_wbuf_t * b, uint8_t v)
{
	a2r_wbuf_put( b, &v, 1 );
}

static void a2r_wbuf_u16(a2r_wbuf_t * b, uint16_t v)
{
	uint8_t t[2];

	t[0] = (uint8_t)v;
	t[1] = (uint8_t)(v >> 8);
	a2r_wbuf_put( b, t, 2 );
}

static void a2r_wbuf_u32(a2r_wbuf_t * b, uint32_t v)
{
	uint8_t t[4];

	t[0] = (uint8_t)v;
	t[1] = (uint8_t)(v >> 8);
	t[2] = (uint8_t)(v >> 16);
	t[3] = (uint8_t)(v >> 24);
	a2r_wbuf_put( b, t, 4 );
}

static void a2r_set_u32(uint8_t * p, uint32_t v)
{
	p[0] = (uint8_t)v;
	p[1] = (uint8_t)(v >> 8);
	p[2] = (uint8_t)(v >> 16);
	p[3] = (uint8_t)(v >> 24);
}

// Packed flux value : 255 bytes for each 255 ticks, then the remainder.
static void a2r_put_flux(a2r_wbuf_t * b, uint32_t ticks)
{
	while( ticks >= 255 )
	{
		a2r_wbuf_u8( b, 255 );
		ticks -= 255;
	}

	a2r_wbuf_u8( b, (uint8_t)ticks );
}

static uint64_t a2r_ps_to_ticks(double ps)
{
	if( ps <= 0 )
		return 0;

	return (uint64_t)( ( ps / (double)A2R3_DEFAULT_TICK_PERIOD_PS ) + 0.5 );
}

static uint32_t a2r_rand_state = 0x1234567;

static int a2r_rand_bit(void)
{
	a2r_rand_state = ( a2r_rand_state * 1103515245 ) + 12345;
	return ( a2r_rand_state >> 16 ) & 1;
}

static void a2r_free_wtrk(a2r_wtrk_t * trk)
{
	free( trk->pulse_ps );
	free( trk->pulse_weak );
	memset( trk, 0, sizeof(a2r_wtrk_t) );
}

// HxC track -> flux pulses times from the main index.
static int a2r_track_to_pulses(HXCFE_SIDE * side, a2r_wtrk_t * trk)
{
	uint32_t i,cell,n,start;
	int32_t bitrate;
	double t,d;
	double * cell_time;
	int index_prev,index_cur,first_edge;

	memset( trk, 0, sizeof(a2r_wtrk_t) );

	if( !side || !side->databuffer || side->tracklen <= 0 )
		return HXCFE_NOERROR;

	n = side->tracklen;

	cell_time = malloc( ( n + 1 ) * sizeof(double) );
	trk->pulse_ps = malloc( n * sizeof(double) );
	trk->pulse_weak = malloc( n * sizeof(uint8_t) );
	if( !cell_time || !trk->pulse_ps || !trk->pulse_weak )
	{
		free( cell_time );
		a2r_free_wtrk( trk );
		return HXCFE_INTERNALERROR;
	}

	// Cells start times
	t = 0;
	for( i = 0; i < n; i++ )
	{
		cell_time[i] = t;

		if( side->bitrate == VARIABLEBITRATE && side->timingbuffer )
			bitrate = side->timingbuffer[i >> 3];
		else
			bitrate = side->bitrate;

		if( bitrate <= 0 )
			bitrate = 250000;

		d = 1E12 / ( 2.0 * (double)bitrate );
		t += d;
	}
	cell_time[n] = t;
	trk->period_ps = t;

	// Index signals : rising edges. The first one is the track start (main index).
	start = 0;
	first_edge = -1;
	if( side->indexbuffer )
	{
		index_prev = side->indexbuffer[ (n - 1) >> 3 ] ? 1 : 0;
		for( i = 0; i < n; i += 8 )
		{
			index_cur = side->indexbuffer[ i >> 3 ] ? 1 : 0;
			if( index_cur && !index_prev )
			{
				if( first_edge < 0 )
				{
					first_edge = i;
					start = i;
				}

				if( trk->nb_edges < A2R_WRITER_MAX_INDEX )
				{
					trk->edges_ps[trk->nb_edges++] = cell_time[i];
				}
			}
			index_prev = index_cur;
		}
	}

	// Edges relative to the track start
	for( i = 0; i < (uint32_t)trk->nb_edges; i++ )
	{
		trk->edges_ps[i] -= cell_time[start];
		if( trk->edges_ps[i] < 0 )
			trk->edges_ps[i] += trk->period_ps;
	}

	// Pulses, starting at the main index. A pulse is at the middle of its cell.
	for( i = 0; i < n; i++ )
	{
		cell = ( start + i ) % n;

		if( ( side->databuffer[cell >> 3] & ( 0x80 >> ( cell & 7 ) ) ) ||
		    ( side->flakybitsbuffer && ( side->flakybitsbuffer[cell >> 3] & ( 0x80 >> ( cell & 7 ) ) ) ) )
		{
			t = ( cell_time[cell] + cell_time[cell + 1] ) / 2.0 - cell_time[start];
			if( t < 0 )
				t += trk->period_ps;

			trk->pulse_ps[trk->nb_pulses] = t;
			trk->pulse_weak[trk->nb_pulses] = ( side->flakybitsbuffer && ( side->flakybitsbuffer[cell >> 3] & ( 0x80 >> ( cell & 7 ) ) ) ) ? 1 : 0;
			trk->nb_pulses++;
		}
	}

	free( cell_time );

	return HXCFE_NOERROR;
}

// RWCP "xtiming" capture : starts at the index, nb_revs revolutions + 1/4.
static void a2r_write_capture(a2r_wbuf_t * b, a2r_wtrk_t * trk, uint16_t location, int nb_revs)
{
	a2r_wbuf_t flux;
	uint32_t i,nb_index;
	int r,e;
	uint64_t prev_ticks,ticks;
	double t,end_ps;
	uint8_t idx_count;
	uint32_t idx_values[A2R_WRITER_MAX_INDEX];

	memset( &flux, 0, sizeof(flux) );

	// Index signals within the capture, except the capture start one.
	end_ps = ( nb_revs + 0.25 ) * trk->period_ps;
	nb_index = 0;
	for( r = 0; r <= nb_revs; r++ )
	{
		if( trk->nb_edges )
		{
			for( e = 0; e < trk->nb_edges; e++ )
			{
				t = ( r * trk->period_ps ) + trk->edges_ps[e];
				if( t > 0 && t < end_ps && nb_index < A2R_WRITER_MAX_INDEX )
					idx_values[nb_index++] = (uint32_t)a2r_ps_to_ticks( t );
			}
		}
		else
		{
			if( r > 0 && nb_index < A2R_WRITER_MAX_INDEX )
				idx_values[nb_index++] = (uint32_t)a2r_ps_to_ticks( r * trk->period_ps );
		}
	}

	// Flux data
	prev_ticks = 0;
	for( r = 0; r <= nb_revs; r++ )
	{
		for( i = 0; i < trk->nb_pulses; i++ )
		{
			t = ( r * trk->period_ps ) + trk->pulse_ps[i];
			if( t >= end_ps )
				break;

			if( trk->pulse_weak[i] && !a2r_rand_bit() )
				continue;

			ticks = a2r_ps_to_ticks( t );
			if( ticks <= prev_ticks )
				ticks = prev_ticks + 1;

			a2r_put_flux( &flux, (uint32_t)( ticks - prev_ticks ) );
			prev_ticks = ticks;
		}
	}

	idx_count = (uint8_t)nb_index;

	a2r_wbuf_u8( b, A2R3_MARK_CAPTURE );
	a2r_wbuf_u8( b, A2R_CAPTURE_XTIMING );
	a2r_wbuf_u16( b, location );
	a2r_wbuf_u8( b, idx_count );
	for( i = 0; i < nb_index; i++ )
		a2r_wbuf_u32( b, idx_values[i] );
	a2r_wbuf_u32( b, flux.size );
	a2r_wbuf_put( b, flux.data, flux.size );

	if( flux.error )
		b->error = 1;

	free( flux.data );
}

// SLVD track entry : one perfect loop starting at the last pulse of the previous revolution.
static void a2r_write_solved(a2r_wbuf_t * b, a2r_wtrk_t * trk, uint16_t location)
{
	a2r_wbuf_t flux;
	uint32_t i,nb_index;
	int e;
	uint64_t prev_ticks,ticks,loop_ticks;
	double start_ps,t;
	uint32_t idx_values[A2R_WRITER_MAX_INDEX];

	memset( &flux, 0, sizeof(flux) );

	// Loop start : last pulse of the track, one revolution before.
	start_ps = trk->pulse_ps[trk->nb_pulses - 1] - trk->period_ps;
	loop_ticks = a2r_ps_to_ticks( trk->period_ps );

	prev_ticks = 0;
	for( i = 0; i < trk->nb_pulses; i++ )
	{
		t = trk->pulse_ps[i] - start_ps;

		// Weak cells : random, once.
		if( trk->pulse_weak[i] && i != trk->nb_pulses - 1 && !a2r_rand_bit() )
			continue;

		if( i == trk->nb_pulses - 1 )
			ticks = loop_ticks;   // exact loop period
		else
			ticks = a2r_ps_to_ticks( t );

		if( ticks <= prev_ticks )
			ticks = prev_ticks + 1;

		a2r_put_flux( &flux, (uint32_t)( ticks - prev_ticks ) );
		prev_ticks = ticks;
	}

	// Index signals relative to the loop start
	nb_index = 0;
	if( trk->nb_edges )
	{
		for( e = 0; e < trk->nb_edges && nb_index < A2R_WRITER_MAX_INDEX; e++ )
		{
			ticks = a2r_ps_to_ticks( trk->edges_ps[e] - start_ps ) % loop_ticks;
			idx_values[nb_index++] = (uint32_t)ticks;
		}
	}
	else
	{
		idx_values[nb_index++] = (uint32_t)( a2r_ps_to_ticks( -start_ps ) % loop_ticks );
	}

	a2r_wbuf_u8( b, A2R3_MARK_TRACK );
	a2r_wbuf_u16( b, location );
	a2r_wbuf_u8( b, 0 );          // Mirror distance outward
	a2r_wbuf_u8( b, 0 );          // Mirror distance inward
	for( i = 0; i < 6; i++ )
		a2r_wbuf_u8( b, 0 );      // Reserved
	a2r_wbuf_u8( b, (uint8_t)nb_index );
	for( i = 0; i < nb_index; i++ )
		a2r_wbuf_u32( b, idx_values[i] );
	a2r_wbuf_u32( b, flux.size );
	a2r_wbuf_put( b, flux.data, flux.size );

	if( flux.error )
		b->error = 1;

	free( flux.data );
}

// Search the Apple GCR address field prologue (D5 AA 96) in the track bit cells,
// with "step" HxC cells per GCR bit (2 : Apple II 4us bits, 1 : Mac 2us bits).
static int a2r_find_apple_prologue(HXCFE_SIDE * side, int step)
{
	uint32_t i,phase,cell,nb_bits;
	uint32_t shift;
	int found;

	if( !side || !side->databuffer || side->tracklen <= 0 )
		return 0;

	found = 0;
	for( phase = 0; phase < (uint32_t)step; phase++ )
	{
		shift = 0;
		nb_bits = ( side->tracklen - phase ) / step;
		for( i = 0; i < nb_bits; i++ )
		{
			cell = phase + ( i * step );
			shift = ( shift << 1 ) | ( ( side->databuffer[cell >> 3] >> ( 7 - ( cell & 7 ) ) ) & 1 );

			if( ( shift & 0xFFFFFF ) == 0xD5AA96 )
			{
				found++;
				if( found >= 4 )
					return 1;
			}
		}
	}

	return 0;
}

static int a2r_get_drive_type(HXCFE_IMGLDR * imgldr_ctx, HXCFE_FLOPPY * floppy)
{
	int drive_type,encoding;
	int32_t rpm;
	HXCFE_SIDE * side;

	drive_type = hxcfe_getEnvVarValue( imgldr_ctx->hxcfe, "A2REXPORT_DRIVE_TYPE" );
	if( drive_type >= A2R_DRIVE_525_SS_40_QUARTER && drive_type <= A2R_DRIVE_3_DS_40 )
		return drive_type;

	encoding = UNKNOWN_ENCODING;
	rpm = 300;
	side = NULL;
	if( floppy->tracks[0] && floppy->tracks[0]->sides )
		side = floppy->tracks[0]->sides[0];

	if( side )
		encoding = side->track_encoding;

	if( floppy->tracks[0] )
		rpm = floppy->tracks[0]->floppyRPM;

	switch( encoding )
	{
		case APPLEII_GCR1_ENCODING:
		case APPLEII_GCR2_ENCODING:
		case APPLEII_HDDD_A2_GCR1_ENCODING:
		case APPLEII_HDDD_A2_GCR2_ENCODING:
			return A2R_DRIVE_525_SS_40_QUARTER;

		case APPLEMAC_GCR_ENCODING:
			return A2R_DRIVE_35_DS_80_APPLE;
	}

	// Apple GCR content ? (Source loader without GCR encoding information)
	if( a2r_find_apple_prologue( side, 2 ) )
		return A2R_DRIVE_525_SS_40_QUARTER;

	if( a2r_find_apple_prologue( side, 1 ) )
		return A2R_DRIVE_35_DS_80_APPLE;

	if( floppy->floppyNumberOfTrack <= 43 )
		return A2R_DRIVE_525_DS_40;

	if( rpm > 330 )
		return A2R_DRIVE_525_DS_80;

	return A2R_DRIVE_35_DS_80;
}

static int a2r_write_chunk(FILE * f, const char * id, uint8_t * data, uint32_t size)
{
	uint8_t header[A2R_CHUNK_HEADER_SIZE];

	memcpy( header, id, 4 );
	a2r_set_u32( &header[4], size );

	if( fwrite( header, A2R_CHUNK_HEADER_SIZE, 1, f ) != 1 )
		return -1;

	if( size && fwrite( data, size, 1, f ) != 1 )
		return -1;

	return 0;
}

// Main writer function
int A2R_libWrite_DiskFile(HXCFE_IMGLDR* imgldr_ctx,HXCFE_FLOPPY * floppy,char * filename)
{
	FILE * f;
	int ret;
	int cyl,head,nb_sides,nb_revs,drive_type,hard_sectors,idx,nb_entries;
	int write_raw,write_solved,nb_written;
	uint16_t location;
	uint8_t info[A2R3_INFO_SIZE];
	char tmp_str[256];
	a2r_wbuf_t rwcp,slvd,meta;
	a2r_wtrk_t trk;
	HXCFE_SIDE * side;
	time_t now;
	struct tm * tm_now;

	f = NULL;
	ret = HXCFE_NOERROR;
	memset( &rwcp, 0, sizeof(rwcp) );
	memset( &slvd, 0, sizeof(slvd) );
	memset( &meta, 0, sizeof(meta) );

	imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"Write A2R file %s...",filename);

	if( !floppy || !floppy->tracks || floppy->floppyNumberOfTrack <= 0 || floppy->floppyNumberOfSide <= 0 )
		return HXCFE_BADPARAMETER;

	nb_revs = hxcfe_getEnvVarValue( imgldr_ctx->hxcfe, "A2REXPORT_NUMBER_OF_REVOLUTIONS" );
	if( nb_revs <= 0 )
		nb_revs = A2R_WRITER_DEFAULT_REVS;
	if( nb_revs > A2R_WRITER_MAX_REVS )
		nb_revs = A2R_WRITER_MAX_REVS;

	write_raw = !( hxcfe_getEnvVarValue( imgldr_ctx->hxcfe, "A2REXPORT_NO_RAW_CAPTURES" ) & 1 );
	write_solved = !( hxcfe_getEnvVarValue( imgldr_ctx->hxcfe, "A2REXPORT_NO_SOLVED_TRACKS" ) & 1 );
	if( !write_raw && !write_solved )
		write_solved = 1;

	drive_type = a2r_get_drive_type( imgldr_ctx, floppy );

	nb_sides = ( floppy->floppyNumberOfSide >= 2 ) ? 2 : 1;
	if( drive_type == A2R_DRIVE_525_SS_40_QUARTER )
		nb_sides = 1;

	nb_entries = floppy->floppyNumberOfTrack * nb_sides;

	// Hard sectors count : index signals on track 0
	hard_sectors = 0;
	side = NULL;
	if( floppy->tracks[0] && floppy->tracks[0]->sides )
		side = floppy->tracks[0]->sides[0];

	if( a2r_track_to_pulses( side, &trk ) != HXCFE_NOERROR )
		return HXCFE_INTERNALERROR;

	if( trk.nb_edges > 1 )
		hard_sectors = trk.nb_edges - 1;

	a2r_free_wtrk( &trk );

	imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"A2R writer : drive type %d, %d track(s), %d side(s), %d hard sector(s), %d revolution(s) per capture",
		drive_type,floppy->floppyNumberOfTrack,nb_sides,hard_sectors,nb_revs);

	// RWCP / SLVD chunks headers : version, resolution, 11 reserved bytes
	a2r_wbuf_u8( &rwcp, A2R3_RWCP_VERSION );
	a2r_wbuf_u32( &rwcp, A2R3_DEFAULT_TICK_PERIOD_PS );
	for( idx = 0; idx < 11; idx++ )
		a2r_wbuf_u8( &rwcp, 0 );

	a2r_wbuf_u8( &slvd, A2R3_SLVD_VERSION );
	a2r_wbuf_u32( &slvd, A2R3_DEFAULT_TICK_PERIOD_PS );
	for( idx = 0; idx < 11; idx++ )
		a2r_wbuf_u8( &slvd, 0 );

	a2r_rand_state = 0x1234567;
	nb_written = 0;

	for( cyl = 0; cyl < floppy->floppyNumberOfTrack; cyl++ )
	{
		for( head = 0; head < nb_sides; head++ )
		{
			idx = ( cyl * nb_sides ) + head;
			hxcfe_imgCallProgressCallback( imgldr_ctx, idx, nb_entries );

			side = NULL;
			if( floppy->tracks[cyl] && floppy->tracks[cyl]->sides && head < floppy->tracks[cyl]->number_of_side )
				side = floppy->tracks[cyl]->sides[head];

			if( a2r_track_to_pulses( side, &trk ) != HXCFE_NOERROR )
			{
				ret = HXCFE_INTERNALERROR;
				goto error;
			}

			if( trk.nb_pulses < 2 )
			{
				// Unformatted / empty track : no entry.
				a2r_free_wtrk( &trk );
				continue;
			}

			if( drive_type == A2R_DRIVE_525_SS_40_QUARTER )
				location = (uint16_t)( cyl * 4 );          // quarter tracks
			else
				location = (uint16_t)( ( cyl << 1 ) + head );

			if( write_raw )
				a2r_write_capture( &rwcp, &trk, location, nb_revs );

			if( write_solved )
				a2r_write_solved( &slvd, &trk, location );

			nb_written++;

			a2r_free_wtrk( &trk );

			if( rwcp.error || slvd.error )
			{
				ret = HXCFE_INTERNALERROR;
				goto error;
			}
		}
	}

	a2r_wbuf_u8( &rwcp, A2R3_MARK_END );
	a2r_wbuf_u8( &slvd, A2R3_MARK_END );

	// META chunk
	now = time( NULL );
	tm_now = gmtime( &now );
	if( tm_now )
	{
		snprintf( tmp_str, sizeof(tmp_str), "image_date\t%.4d-%.2d-%.2dT%.2d:%.2d:%.2d.000Z\n",
			tm_now->tm_year + 1900, tm_now->tm_mon + 1, tm_now->tm_mday, tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec );
		a2r_wbuf_put( &meta, tmp_str, strlen(tmp_str) );
	}
	snprintf( tmp_str, sizeof(tmp_str), "notes\tConverted with HxC Floppy Emulator v%s\n", hxcfe_getVersion( imgldr_ctx->hxcfe ) );
	a2r_wbuf_put( &meta, tmp_str, strlen(tmp_str) );

	if( rwcp.error || slvd.error || meta.error )
	{
		ret = HXCFE_INTERNALERROR;
		goto error;
	}

	// INFO chunk
	memset( info, 0, sizeof(info) );
	info[0] = A2R3_INFO_VERSION;
	memset( &info[1], ' ', 32 );
	snprintf( tmp_str, sizeof(tmp_str), "HxC Floppy Emulator v%s", hxcfe_getVersion( imgldr_ctx->hxcfe ) );
	memcpy( &info[1], tmp_str, strlen(tmp_str) > 32 ? 32 : strlen(tmp_str) );
	info[33] = (uint8_t)drive_type;
	info[34] = ( hxcfe_floppyGetFlags( imgldr_ctx->hxcfe, floppy ) & HXCFE_FLOPPY_WRPROTECTED_FLAG ) ? 1 : 0;
	info[35] = 1;   // Index synchronized captures
	info[36] = (uint8_t)hard_sectors;

	f = hxc_fopen( filename, "wb" );
	if( !f )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"A2R writer : Cannot create %s !",filename);
		ret = HXCFE_ACCESSERROR;
		goto error;
	}

	if( fwrite( "A2R3\xFF\x0A\x0D\x0A", A2R_HEADER_SIZE, 1, f ) != 1 )
		goto write_error;

	if( a2r_write_chunk( f, "INFO", info, A2R3_INFO_SIZE ) )
		goto write_error;

	if( write_raw && a2r_write_chunk( f, "RWCP", rwcp.data, rwcp.size ) )
		goto write_error;

	if( write_solved && a2r_write_chunk( f, "SLVD", slvd.data, slvd.size ) )
		goto write_error;

	if( a2r_write_chunk( f, "META", meta.data, meta.size ) )
		goto write_error;

	hxc_fclose( f );

	imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"A2R writer : %d track(s) written",nb_written);

	free( rwcp.data );
	free( slvd.data );
	free( meta.data );

	return HXCFE_NOERROR;

write_error:
	imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"A2R writer : Write error (%s) !",filename);
	ret = HXCFE_ACCESSERROR;

error:
	if( f )
		hxc_fclose( f );

	free( rwcp.data );
	free( slvd.data );
	free( meta.data );

	return ret;
}
