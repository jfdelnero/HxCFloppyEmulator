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
// File : ipf_writer.c
// Contains: IPF floppy image writer
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

#include "ipf_loader.h"
#include "ipf_writer.h"

#include "libhxcadaptor.h"

#include "ipf_format.h"
#include "tracks/std_crc32.h"

//
// IPF writer (SPS encoder, revision 1).
//
// Each track is stored as a single MFM block, without gap, starting at the
// index (startBitPos = 0). The block data stream reproduces the track bit
// cells exactly :
//
//  - Valid MFM areas (clock bits matching the MFM rule) are stored as
//    "Data" stream elements (decoded data bits, MFM encoded by the decoder).
//  - Anything else (sync marks, missing clocks, FM tracks, half-cell shifted
//    areas, ...) is stored as "Mark/Sync" stream elements (raw cells copied
//    as-is by the decoder).
//  - Weak/flakey cells are stored as "Fuzzy" stream elements.
//
// The data stream elements sizes are given in bits (block flag DataInBit).
//
// Unformatted tracks (almost fully flakey, or not present) are stored as
// "Noise" density tracks without data.
//

//#define IPF_WRITER_DBG 1

#define IPF_DATA_CHUNK_CELLS          16    // 1 MFM byte
#define IPF_DATA_MIN_RESYNC_CHUNKS    2     // valid MFM chunks needed to leave a raw area
#define IPF_NOISE_TRACK_PERCENT       90    // flakey cells percentage of an unformatted track

typedef struct ipf_buf_t_
{
	uint8_t * data;
	uint32_t  size;
	uint32_t  allocated;
	int       error;
}ipf_buf_t;

typedef struct ipf_trk_t_
{
	uint8_t * cells;      // track bit cells, starting at the index
	uint8_t * weak;       // weak cells (NULL if none)
	uint32_t  nb_cells;   // track size in cells (multiple of 8)
	int       noise;      // unformatted track
	int       has_weak;
}ipf_trk_t;

static void ipf_buf_put(ipf_buf_t * b, const void * data, uint32_t size)
{
	uint8_t * tmp;
	uint32_t new_size;

	if( b->error || !size )
		return;

	if( b->size + size > b->allocated )
	{
		new_size = ( b->allocated ? b->allocated * 2 : 4096 );
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

static void ipf_buf_put_u8(ipf_buf_t * b, uint8_t value)
{
	ipf_buf_put( b, &value, 1 );
}

static void ipf_buf_put_u32(ipf_buf_t * b, uint32_t value)
{
	uint8_t tmp[4];

	tmp[0] = (uint8_t)(value >> 24);
	tmp[1] = (uint8_t)(value >> 16);
	tmp[2] = (uint8_t)(value >> 8);
	tmp[3] = (uint8_t)(value);

	ipf_buf_put( b, tmp, 4 );
}

static void ipf_set_u32(uint8_t * p, uint32_t value)
{
	p[0] = (uint8_t)(value >> 24);
	p[1] = (uint8_t)(value >> 16);
	p[2] = (uint8_t)(value >> 8);
	p[3] = (uint8_t)(value);
}

static int ipf_get_cell(uint8_t * buf, uint32_t cell)
{
	return ( buf[cell >> 3] >> ( 7 - (cell & 7) ) ) & 1;
}

static void ipf_set_cell(uint8_t * buf, uint32_t cell)
{
	buf[cell >> 3] |= (uint8_t)( 0x80 >> (cell & 7) );
}

// Write a record : header (type, size, crc) + record specific block.
static int ipf_write_record(FILE * f, const char * type, uint8_t * block, uint32_t block_size)
{
	uint8_t header[IPF_RECORD_HEADER_SIZE];
	uint32_t crc;

	memcpy( header, type, 4 );
	ipf_set_u32( &header[4], IPF_RECORD_HEADER_SIZE + block_size );
	ipf_set_u32( &header[8], 0 );

	// CRC32 of the complete record with the CRC field set to 0.
	crc = std_crc32( 0, header, IPF_RECORD_HEADER_SIZE );
	if( block_size )
		crc = std_crc32( crc, block, block_size );

	ipf_set_u32( &header[8], crc );

	if( fwrite( header, IPF_RECORD_HEADER_SIZE, 1, f ) != 1 )
		return -1;

	if( block_size && fwrite( block, block_size, 1, f ) != 1 )
		return -1;

	return 0;
}

// Data stream element : header byte (size width | type) + big endian size.
static void ipf_put_stream_element(ipf_buf_t * b, int type, uint32_t size)
{
	int width;

	if( size > 0xFFFFFF )
		width = 4;
	else if( size > 0xFFFF )
		width = 3;
	else if( size > 0xFF )
		width = 2;
	else
		width = 1;

	ipf_buf_put_u8( b, (uint8_t)( ( width << IPF_STREAM_SIZE_WIDTH_SHIFT ) | type ) );

	while( width-- )
		ipf_buf_put_u8( b, (uint8_t)( size >> ( width * 8 ) ) );
}

// Raw cells -> Mark element (cells copied as-is by the decoder)
static void ipf_put_mark(ipf_buf_t * b, ipf_trk_t * trk, uint32_t pos, uint32_t nb_cells)
{
	uint8_t sample[256];
	uint32_t i,j;

	ipf_put_stream_element( b, chunk_Sync, nb_cells );

	j = 0;
	memset( sample, 0, sizeof(sample) );
	for( i = 0; i < nb_cells; i++ )
	{
		if( ipf_get_cell( trk->cells, pos + i ) )
			sample[ j >> 3 ] |= (uint8_t)( 0x80 >> ( j & 7 ) );

		j++;
		if( j == sizeof(sample) * 8 )
		{
			ipf_buf_put( b, sample, sizeof(sample) );
			memset( sample, 0, sizeof(sample) );
			j = 0;
		}
	}

	if( j )
		ipf_buf_put( b, sample, ( j + 7 ) / 8 );
}

// Valid MFM cells -> Data element (data bits only, MFM encoded by the decoder)
static void ipf_put_data(ipf_buf_t * b, ipf_trk_t * trk, uint32_t pos, uint32_t nb_cells)
{
	uint8_t sample[256];
	uint32_t i,j;

	ipf_put_stream_element( b, chunk_Data, nb_cells / 2 );

	j = 0;
	memset( sample, 0, sizeof(sample) );
	for( i = 0; i < nb_cells; i += 2 )
	{
		if( ipf_get_cell( trk->cells, pos + i + 1 ) )
			sample[ j >> 3 ] |= (uint8_t)( 0x80 >> ( j & 7 ) );

		j++;
		if( j == sizeof(sample) * 8 )
		{
			ipf_buf_put( b, sample, sizeof(sample) );
			memset( sample, 0, sizeof(sample) );
			j = 0;
		}
	}

	if( j )
		ipf_buf_put( b, sample, ( j + 7 ) / 8 );
}

static int ipf_is_weak(ipf_trk_t * trk, uint32_t pos)
{
	if( !trk->weak || pos >= trk->nb_cells )
		return 0;

	return ipf_get_cell( trk->weak, pos );
}

// Check that 16 cells are a valid MFM byte, the decoder generating the
// clock bits from the previous cell : clock = !(previous cell | data bit)
static int ipf_valid_mfm_chunk(ipf_trk_t * trk, uint32_t pos, int prev)
{
	uint32_t i;
	int clock,data;

	if( pos + IPF_DATA_CHUNK_CELLS > trk->nb_cells )
		return 0;

	for( i = 0; i < IPF_DATA_CHUNK_CELLS; i += 2 )
	{
		if( ipf_is_weak( trk, pos + i ) || ipf_is_weak( trk, pos + i + 1 ) )
			return 0;

		clock = ipf_get_cell( trk->cells, pos + i );
		data  = ipf_get_cell( trk->cells, pos + i + 1 );

		if( clock != ( !( prev | data ) ) )
			return 0;

		prev = data;
	}

	return 1;
}

// Cell value as seen by the decoder MFM encoder (weak cells are cleared at encoding time)
static int ipf_decoder_cell(ipf_trk_t * trk, uint32_t pos)
{
	if( ipf_is_weak( trk, pos ) )
		return 0;

	return ipf_get_cell( trk->cells, pos );
}

// Build the block data stream of a track.
static void ipf_build_data_stream(ipf_buf_t * b, ipf_trk_t * trk, uint32_t * nb_data_elements, uint32_t * nb_mark_elements, uint32_t * nb_weak_elements)
{
	uint32_t pos,end,len,k;
	int prev;

	pos = 0;

	// Cell before the first one : last track cell. (The decoder fixes the
	// first MFM clock bit of the block according to it).
	prev = ipf_decoder_cell( trk, trk->nb_cells - 1 );

	while( pos < trk->nb_cells )
	{
		if( ipf_is_weak( trk, pos ) )
		{
			// Weak area : Fuzzy element. (size in data bits : 2 cells per bit)
			end = pos;
			while( end < trk->nb_cells && ipf_is_weak( trk, end ) )
				end++;

			len = end - pos;
			if( len & 1 )
				len++;

			if( pos + len > trk->nb_cells )
				len = ( trk->nb_cells - pos ) & ~1;

			if( !len )
			{
				// Single cell left at the end of the track : raw.
				ipf_put_mark( b, trk, pos, 1 );
				(*nb_mark_elements)++;
				prev = ipf_get_cell( trk->cells, pos );
				pos++;
				continue;
			}

			ipf_put_stream_element( b, chunk_Flaky, len / 2 );
			(*nb_weak_elements)++;

			pos += len;
			prev = 0;
			continue;
		}

		if( ipf_valid_mfm_chunk( trk, pos, prev ) )
		{
			// Valid MFM area : Data element.
			end = pos;
			k = prev;
			while( ipf_valid_mfm_chunk( trk, end, (int)k ) )
			{
				end += IPF_DATA_CHUNK_CELLS;
				k = ipf_get_cell( trk->cells, end - 1 );
			}

			ipf_put_data( b, trk, pos, end - pos );
			(*nb_data_elements)++;

			prev = (int)k;
			pos = end;
			continue;
		}

		// Raw area : Mark element, up to the next valid MFM area (any cell phase) or weak area.
		end = pos + 1;
		while( end < trk->nb_cells && !ipf_is_weak( trk, end ) )
		{
			prev = ipf_get_cell( trk->cells, end - 1 );

			if( ipf_valid_mfm_chunk( trk, end, prev ) )
			{
				// Require several consecutive valid chunks to resync.
				k = 1;
				while( k < IPF_DATA_MIN_RESYNC_CHUNKS &&
				       ipf_valid_mfm_chunk( trk, end + ( k * IPF_DATA_CHUNK_CELLS ), ipf_get_cell( trk->cells, end + ( k * IPF_DATA_CHUNK_CELLS ) - 1 ) ) )
				{
					k++;
				}

				if( k == IPF_DATA_MIN_RESYNC_CHUNKS || end + ( k * IPF_DATA_CHUNK_CELLS ) >= trk->nb_cells )
					break;
			}

			end++;
		}

		ipf_put_mark( b, trk, pos, end - pos );
		(*nb_mark_elements)++;

		prev = ipf_get_cell( trk->cells, end - 1 );
		pos = end;
	}

	// End of the data stream
	ipf_buf_put_u8( b, chunk_End );
}

// Get the HxC track, rotated to start at the index, size padded to a multiple of 8.
static int ipf_prepare_track(HXCFE_IMGLDR * imgldr_ctx, HXCFE_SIDE * side, ipf_trk_t * trk)
{
	uint32_t i,j,src,nb_cells,index_pos,nb_weak;
	int index_prev,index_cur;

	memset( trk, 0, sizeof(ipf_trk_t) );

	if( !side || !side->databuffer || side->tracklen <= 0 )
	{
		trk->noise = 1;
		return HXCFE_NOERROR;
	}

	nb_cells = side->tracklen;

	// Index position : index signal rising edge.
	index_pos = 0;
	if( side->indexbuffer )
	{
		index_prev = side->indexbuffer[ (nb_cells - 1) >> 3 ] ? 1 : 0;
		for( i = 0; i < nb_cells; i += 8 )
		{
			index_cur = side->indexbuffer[ i >> 3 ] ? 1 : 0;
			if( index_cur && !index_prev )
			{
				index_pos = i;
				break;
			}
			index_prev = index_cur;
		}
	}

	// Unformatted track ?
	nb_weak = 0;
	if( side->flakybitsbuffer )
	{
		for( i = 0; i < nb_cells; i++ )
		{
			if( ipf_get_cell( side->flakybitsbuffer, i ) )
				nb_weak++;
		}

		if( (uint64_t)nb_weak * 100 >= (uint64_t)nb_cells * IPF_NOISE_TRACK_PERCENT )
		{
			trk->noise = 1;
			return HXCFE_NOERROR;
		}
	}

	// Track size : multiple of 8 cells (needed by the decoder when the last block has no gap)
	trk->nb_cells = ( nb_cells + 7 ) & ~7;

	trk->cells = calloc( 1, trk->nb_cells / 8 );
	if( !trk->cells )
		return HXCFE_INTERNALERROR;

	if( nb_weak )
	{
		trk->weak = calloc( 1, trk->nb_cells / 8 );
		if( !trk->weak )
		{
			free( trk->cells );
			trk->cells = NULL;
			return HXCFE_INTERNALERROR;
		}
		trk->has_weak = 1;
	}

	for( i = 0; i < trk->nb_cells; i++ )
	{
		if( i < nb_cells )
		{
			src = ( index_pos + i ) % nb_cells;

			if( ipf_get_cell( side->databuffer, src ) )
				ipf_set_cell( trk->cells, i );

			if( trk->weak && ipf_get_cell( side->flakybitsbuffer, src ) )
				ipf_set_cell( trk->weak, i );
		}
		else
		{
			// Padding : continue the (gap) pattern of the previous 16 cells.
			j = ( i >= 16 ) ? ( i - 16 ) : 0;
			if( ipf_get_cell( trk->cells, j ) )
				ipf_set_cell( trk->cells, i );
		}
	}

	return HXCFE_NOERROR;
}

static uint32_t ipf_get_platform(HXCFE_FLOPPY * floppy)
{
	switch( floppy->floppyiftype )
	{
		case AMIGA_DD_FLOPPYMODE:
		case AMIGA_HD_FLOPPYMODE:
			return IPF_PLATFORM_AMIGA;

		case ATARIST_DD_FLOPPYMODE:
		case ATARIST_HD_FLOPPYMODE:
			return IPF_PLATFORM_ATARI_ST;

		case IBMPC_DD_FLOPPYMODE:
		case IBMPC_HD_FLOPPYMODE:
		case IBMPC_ED_FLOPPYMODE:
		case IBMPC_DD_HD_FLOPPYMODE:
			return IPF_PLATFORM_PC;

		case CPC_DD_FLOPPYMODE:
			return IPF_PLATFORM_AMSTRAD_CPC;

		case C64_DD_FLOPPYMODE:
			return IPF_PLATFORM_C64;

		default:
			return IPF_PLATFORM_NONE;
	}
}

// Main writer function
int IPF_libWrite_DiskFile(HXCFE_IMGLDR* imgldr_ctx,HXCFE_FLOPPY * floppy,char * filename)
{
	FILE * f;
	int ret;
	int cyl,head,nb_sides,nb_tracks,idx,nb_entries;
	uint8_t block[IPF_IMGE_BLOCK_SIZE > IPF_INFO_BLOCK_SIZE ? IPF_IMGE_BLOCK_SIZE : IPF_INFO_BLOCK_SIZE];
	ipf_buf_t * streams;
	ipf_trk_t trk;
	uint32_t * track_bits;
	uint8_t * track_flags;
	uint32_t file_key;
	uint32_t nb_data,nb_mark,nb_weak;
	uint32_t total_data,total_mark,total_weak,nb_noise;
	HXCFE_SIDE * side;
	time_t now;
	struct tm * tm_now;

	f = NULL;
	streams = NULL;
	track_bits = NULL;
	track_flags = NULL;
	ret = HXCFE_NOERROR;

	imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"Write IPF file %s...",filename);

	if( !floppy || !floppy->tracks || floppy->floppyNumberOfTrack <= 0 || floppy->floppyNumberOfSide <= 0 )
		return HXCFE_BADPARAMETER;

	nb_tracks = floppy->floppyNumberOfTrack;
	nb_sides = ( floppy->floppyNumberOfSide >= 2 ) ? 2 : 1;
	nb_entries = nb_tracks * nb_sides;

	streams = calloc( nb_entries, sizeof(ipf_buf_t) );
	track_bits = calloc( nb_entries, sizeof(uint32_t) );
	track_flags = calloc( nb_entries, sizeof(uint8_t) );
	if( !streams || !track_bits || !track_flags )
	{
		ret = HXCFE_INTERNALERROR;
		goto error;
	}

	// Encode all the tracks data areas first (IMGE records need their sizes).
	total_data = total_mark = total_weak = nb_noise = 0;
	file_key = 0;

	for( cyl = 0; cyl < nb_tracks; cyl++ )
	{
		for( head = 0; head < nb_sides; head++ )
		{
			idx = ( cyl * nb_sides ) + head;

			hxcfe_imgCallProgressCallback( imgldr_ctx, idx, nb_entries );

			side = NULL;
			if( floppy->tracks[cyl] && floppy->tracks[cyl]->sides && head < floppy->tracks[cyl]->number_of_side )
				side = floppy->tracks[cyl]->sides[head];

			ret = ipf_prepare_track( imgldr_ctx, side, &trk );
			if( ret != HXCFE_NOERROR )
				goto error;

			if( trk.noise )
			{
				nb_noise++;
				track_flags[idx] = IPF_TRACK_NOISE;
				continue;
			}

			if( trk.has_weak )
				track_flags[idx] |= IPF_TRACK_WEAK;

			track_bits[idx] = trk.nb_cells;

			// Block descriptor (one block per track)
			ipf_buf_put_u32( &streams[idx], trk.nb_cells );                // dataBits
			ipf_buf_put_u32( &streams[idx], 0 );                           // gapBits
			ipf_buf_put_u32( &streams[idx], 0 );                           // gapOffset (no gap stream)
			ipf_buf_put_u32( &streams[idx], IPF_CELL_TYPE_2US );           // cellType
			ipf_buf_put_u32( &streams[idx], IPF_BLOCK_ENC_MFM );           // encoderType
			ipf_buf_put_u32( &streams[idx], IPF_BLOCK_FLAG_DATA_IN_BITS ); // blockFlags
			ipf_buf_put_u32( &streams[idx], 0 );                           // gapDefault
			ipf_buf_put_u32( &streams[idx], IPF_BLOCK_DESCRIPTOR_SIZE );   // dataOffset

			nb_data = nb_mark = nb_weak = 0;
			ipf_build_data_stream( &streams[idx], &trk, &nb_data, &nb_mark, &nb_weak );

			total_data += nb_data;
			total_mark += nb_mark;
			total_weak += nb_weak;

#ifdef IPF_WRITER_DBG
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"IPF writer : Track %d:%d - %d cells, %d data, %d raw, %d fuzzy elements, %d bytes",
				cyl,head,trk.nb_cells,nb_data,nb_mark,nb_weak,streams[idx].size);
#endif

			file_key = std_crc32( file_key, trk.cells, trk.nb_cells / 8 );

			free( trk.cells );
			free( trk.weak );

			if( streams[idx].error )
			{
				ret = HXCFE_INTERNALERROR;
				goto error;
			}
		}
	}

	imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"IPF writer : %d track(s), %d side(s), %d unformatted track(s), %d data / %d raw / %d fuzzy stream elements",
		nb_tracks,nb_sides,nb_noise,total_data,total_mark,total_weak);

	f = hxc_fopen( filename, "wb" );
	if( !f )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"IPF writer : Cannot create %s !",filename);
		ret = HXCFE_ACCESSERROR;
		goto error;
	}

	// CAPS record
	if( ipf_write_record( f, "CAPS", NULL, 0 ) )
		goto write_error;

	// INFO record
	now = time( NULL );
	tm_now = localtime( &now );

	memset( block, 0, sizeof(block) );
	ipf_set_u32( &block[0],  IPF_MEDIA_FLOPPY );              // mediaType
	ipf_set_u32( &block[4],  ENCOD_SPS );                     // encoderType
	ipf_set_u32( &block[8],  1 );                             // encoderRev
	ipf_set_u32( &block[12], file_key );                      // fileKey
	ipf_set_u32( &block[16], 1 );                             // fileRev
	ipf_set_u32( &block[20], 0 );                             // origin
	ipf_set_u32( &block[24], 0 );                             // minTrack
	ipf_set_u32( &block[28], nb_tracks - 1 );                 // maxTrack
	ipf_set_u32( &block[32], 0 );                             // minSide
	ipf_set_u32( &block[36], nb_sides - 1 );                  // maxSide
	if( tm_now )
	{
		ipf_set_u32( &block[40], ( (tm_now->tm_year + 1900) * 10000 ) + ( (tm_now->tm_mon + 1) * 100 ) + tm_now->tm_mday ); // creationDate
		ipf_set_u32( &block[44], ( tm_now->tm_hour * 10000000 ) + ( tm_now->tm_min * 100000 ) + ( tm_now->tm_sec * 1000 ) ); // creationTime
	}
	ipf_set_u32( &block[48], ipf_get_platform( floppy ) );    // platforms[0]
	ipf_set_u32( &block[64], 0 );                             // diskNumber
	ipf_set_u32( &block[68], 0 );                             // creatorId

	if( ipf_write_record( f, "INFO", block, IPF_INFO_BLOCK_SIZE ) )
		goto write_error;

	// IMGE records
	for( cyl = 0; cyl < nb_tracks; cyl++ )
	{
		for( head = 0; head < nb_sides; head++ )
		{
			idx = ( cyl * nb_sides ) + head;

			memset( block, 0, sizeof(block) );
			ipf_set_u32( &block[0],  cyl );                    // track
			ipf_set_u32( &block[4],  head );                   // side
			ipf_set_u32( &block[12], IPF_SIGNAL_2US );         // signalType

			if( track_flags[idx] & IPF_TRACK_NOISE )
			{
				ipf_set_u32( &block[8], denNoise );            // density
			}
			else
			{
				ipf_set_u32( &block[8],  denUniform );         // density : auto
				ipf_set_u32( &block[16], track_bits[idx] / 8 );// trackBytes
				ipf_set_u32( &block[20], 0 );                  // startBytePos
				ipf_set_u32( &block[24], 0 );                  // startBitPos
				ipf_set_u32( &block[28], track_bits[idx] );    // dataBits
				ipf_set_u32( &block[32], 0 );                  // gapBits
				ipf_set_u32( &block[36], track_bits[idx] );    // trackBits
				ipf_set_u32( &block[40], 1 );                  // blockCount
				ipf_set_u32( &block[44], 0 );                  // encoderProcess
				ipf_set_u32( &block[48], (track_flags[idx] & IPF_TRACK_WEAK) ? IPF_IMGE_FLAG_FUZZY : 0 ); // trackFlags
			}

			ipf_set_u32( &block[52], idx + 1 );                // dataKey

			if( ipf_write_record( f, "IMGE", block, IPF_IMGE_BLOCK_SIZE ) )
				goto write_error;
		}
	}

	// DATA records
	for( idx = 0; idx < nb_entries; idx++ )
	{
		memset( block, 0, sizeof(block) );
		ipf_set_u32( &block[0],  streams[idx].size );          // extra data block size
		ipf_set_u32( &block[4],  streams[idx].size * 8 );      // size in bits
		ipf_set_u32( &block[8],  streams[idx].size ? std_crc32( 0, streams[idx].data, streams[idx].size ) : 0 ); // extra data block CRC
		ipf_set_u32( &block[12], idx + 1 );                    // dataKey

		if( ipf_write_record( f, "DATA", block, IPF_DATA_BLOCK_SIZE ) )
			goto write_error;

		if( streams[idx].size && fwrite( streams[idx].data, streams[idx].size, 1, f ) != 1 )
			goto write_error;
	}

	hxc_fclose( f );
	f = NULL;

	for( idx = 0; idx < nb_entries; idx++ )
		free( streams[idx].data );

	free( streams );
	free( track_bits );
	free( track_flags );

	return HXCFE_NOERROR;

write_error:
	imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"IPF writer : Write error (%s) !",filename);
	ret = HXCFE_ACCESSERROR;

error:
	if( f )
		hxc_fclose( f );

	if( streams )
	{
		for( idx = 0; idx < nb_entries; idx++ )
			free( streams[idx].data );
	}

	free( streams );
	free( track_bits );
	free( track_flags );

	return ret;
}
