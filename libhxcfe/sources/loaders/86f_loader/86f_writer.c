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
// File : 86f_writer.c
// Contains: 86F floppy image writer
//
// Written by: Jean-François DEL NERO
// Assisted-by: Claude Opus 5.5
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "types.h"

#include "internal_libhxcfe.h"
#include "tracks/track_generator.h"
#include "libhxcfe.h"
#include "86f_loader.h"
#include "86f_writer.h"
#include "86f_format.h"
#include "floppy_utils.h"
#include "tracks/sector_extractor.h"
#include "libhxcadaptor.h"

//
// 86F file layout produced by this writer (86Box format v2.12) :
//
//  - Header : "86BF", minor 0x0C, major 0x02, 16-bit disk flags.
//  - Track offsets table : 512 entries (2048 bytes) for double sided disks,
//    256 entries (1024 bytes) for single sided disks (86Box reads
//    only 1024 bytes in this case and indexes the tracks without the side bit).
//  - Tracks are stored in thin track (96/135 tpi) geometry : 40 cylinders
//    (48 tpi) disks have each cylinder written twice.
//  - "Total bitcells count" mode (disk flags bit 7 and bit 12 set, bits 6,5
//    cleared) : each track header gives the exact number of bit cells of the
//    track. This is the mode recommended by the format specification for
//    images converted from other formats.
//  - Bit cells are stored MSB first, 16-bit word aligned (no byte reversal).
//
// The HxC internal track bit cells are resampled to the 86Box fixed cell
// period selected by the track rate code. FM tracks are stored at the half
// cell rate (86Box FM cells are twice longer than the HxC internal FM cells).
//

#define F86_WRITER_DBG 1

#define F86_THICK_TRACKS_AUTO_MAX_CYL 43

typedef struct f86_rate_t_
{
	int32_t  bitrate;   // MFM bitrate (FM data rate is the half)
	uint16_t code;      // Track flags bits 2,1,0
	int      hole;      // Minimum disk hole type for this rate
}f86_rate_t;

static const f86_rate_t f86_rates[]=
{
	{  250000, 0x2, 0 },
	{  300000, 0x1, 0 },
	{  500000, 0x0, 1 },
	{ 1000000, 0x3, 2 },
	{ 2000000, 0x5, 3 }
};

typedef struct f86_track_t_
{
	uint8_t  * data;
	uint8_t  * surface;
	uint32_t   nb_of_cells;
	uint32_t   index_position;
	uint32_t   size;        // data (and surface) size in bytes
	uint16_t   flags;
	int        hole;
	int        has_weak_bits;
}f86_track_t;

static void f86_free_track(f86_track_t * trk)
{
	if(trk)
	{
		free(trk->data);
		free(trk->surface);
		memset(trk,0,sizeof(f86_track_t));
	}
}

static int f86_get_track_encoding(HXCFE_SIDE * side)
{
	int32_t i,nb_bytes,best;
	int32_t count[256];

	if( side->track_encoding_buffer )
	{
		// Mixed encoding track : take the dominant one.
		memset(count,0,sizeof(count));

		nb_bytes = side->tracklen / 8;
		for(i=0;i<nb_bytes;i++)
		{
			count[side->track_encoding_buffer[i]]++;
		}

		best = 0;
		for(i=0;i<256;i++)
		{
			if(count[i] > count[best])
				best = i;
		}

		return best;
	}

	return side->track_encoding;
}

static int f86_hxc_to_86f_encoding(int hxc_encoding)
{
	switch(hxc_encoding)
	{
		case ISOIBM_FM_ENCODING:
		case EMU_FM_ENCODING:
		case TYCOM_FM_ENCODING:
		case HEATHKIT_HS_FM_ENCODING:
		case MICRALN_HS_FM_ENCODING:
			return F86_TRK_ENC_FM;

		case DEC_RX02_M2FM_ENCODING:
			return F86_TRK_ENC_M2FM;

		case APPLEII_GCR1_ENCODING:
		case APPLEII_GCR2_ENCODING:
		case APPLEII_HDDD_A2_GCR1_ENCODING:
		case APPLEII_HDDD_A2_GCR2_ENCODING:
		case APPLEMAC_GCR_ENCODING:
		case C64_GCR_ENCODING:
		case VICTOR9K_GCR_ENCODING:
			return F86_TRK_ENC_GCR;

		default:
			return F86_TRK_ENC_MFM;
	}
}

static const f86_rate_t * f86_get_nearest_rate(double bitrate)
{
	unsigned int i;
	const f86_rate_t * best;
	double ratio,best_ratio;

	best = &f86_rates[0];
	best_ratio = -1;

	for(i=0;i<sizeof(f86_rates)/sizeof(f86_rate_t);i++)
	{
		if( bitrate > f86_rates[i].bitrate )
			ratio = bitrate / (double)f86_rates[i].bitrate;
		else
			ratio = (double)f86_rates[i].bitrate / bitrate;

		if( best_ratio < 0 || ratio < best_ratio )
		{
			best_ratio = ratio;
			best = &f86_rates[i];
		}
	}

	return best;
}

static double f86_get_cell_duration(HXCFE_SIDE * side, int32_t cell)
{
	int32_t bitrate;

	if( side->bitrate == VARIABLEBITRATE && side->timingbuffer )
		bitrate = side->timingbuffer[cell>>3];
	else
		bitrate = side->bitrate;

	if( bitrate <= 0 )
		bitrate = 250000;

	// HxC internal cell period : 1 / (2 * bitrate)
	return 1.0 / ( 2.0 * (double)bitrate );
}

static double f86_get_track_duration(HXCFE_SIDE * side)
{
	int32_t i;
	double total_time;

	total_time = 0;
	for(i=0;i<side->tracklen;i++)
	{
		total_time += f86_get_cell_duration(side, i);
	}

	return total_time;
}

// Nearest 86F rate of a side (average bitrate)
static const f86_rate_t * f86_get_side_rate(HXCFE_SIDE * side)
{
	double total_time;

	total_time = f86_get_track_duration(side);
	if( side->tracklen <= 0 || total_time <= 0 )
		return &f86_rates[0];

	return f86_get_nearest_rate( (double)side->tracklen / ( 2.0 * total_time ) );
}

// Convert an HxC side to a 86F track (data/surface arrays + header fields).
//
// Encoded tracks (known encoding) : the bit cells are copied 1:1 (FM : 2 HxC
// cells -> 1 86F cell). The 86F track bit cells count is the HxC track one.
// The cells must not be resampled to the 86Box fixed cell period : the flux
// stream based tracks have variable bitrates (drive speed variations) and any
// resampling to a fixed period drops / inserts cells in the bitstream, which
// breaks the MFM/FM data.
//
// Unknown encoding tracks (unformatted / noise) : the cells are resampled by
// time at the disk rate (default_rate) to keep the track duration.
static int f86_convert_side(HXCFE_IMGLDR * imgldr_ctx, HXCFE_SIDE * side, int cyl, int head, const f86_rate_t * default_rate, f86_track_t * trk)
{
	int32_t i,tracklen;
	uint32_t o;
	int encoding,hxc_encoding,resample,step;
	const f86_rate_t * rate;
	double total_time,t,d,out_period,rpm;
	int index_prev,index_cur,index_found;

	memset(trk,0,sizeof(f86_track_t));

	tracklen = side->tracklen;
	if( tracklen <= 0 || !side->databuffer )
		return HXCFE_BADPARAMETER;

	hxc_encoding = f86_get_track_encoding(side);
	encoding = f86_hxc_to_86f_encoding(hxc_encoding);

	// Track duration and average bitrate
	total_time = f86_get_track_duration(side);

	resample = 0;
	if( hxc_encoding == UNKNOWN_ENCODING && default_rate )
	{
		rate = default_rate;
		resample = 1;
	}
	else
	{
		rate = f86_get_nearest_rate( (double)tracklen / ( 2.0 * total_time ) );
	}

	// 86Box cell period. The FM cells are twice longer than the MFM ones
	// for the same rate code. (HxC internal FM cells are MFM sized : 0C0D)
	out_period = 1.0 / ( 2.0 * (double)rate->bitrate );
	step = 1;
	if( encoding == F86_TRK_ENC_FM )
	{
		out_period *= 2.0;
		step = 2;
	}

	if( resample )
		trk->nb_of_cells = (uint32_t)( ( total_time / out_period ) + 0.5 );
	else
		trk->nb_of_cells = (uint32_t)( ( tracklen + (step - 1) ) / step );

	if( !trk->nb_of_cells )
		trk->nb_of_cells = 1;

	if( trk->nb_of_cells > F86_MAX_BITCELLS )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_WARNING,"86F writer : Track %d:%d too long (%d cells) ! Truncated to %d cells.",cyl,head,trk->nb_of_cells,F86_MAX_BITCELLS);
		trk->nb_of_cells = F86_MAX_BITCELLS;
	}

	// Track data size : padded to the next 16-bit word.
	trk->size = ( ( trk->nb_of_cells + 15 ) / 16 ) * 2;

	trk->data = calloc( 1, trk->size );
	trk->surface = calloc( 1, trk->size );
	if( !trk->data || !trk->surface )
	{
		f86_free_track(trk);
		return HXCFE_INTERNALERROR;
	}

	// Copy (or resample) the cells. Resampling : each input cell goes to the
	// output cell containing its center time.
	index_found = 0;
	index_prev = 0;
	if( side->indexbuffer )
		index_prev = side->indexbuffer[ (tracklen-1) >> 3 ] ? 1 : 0;

	t = 0;
	for(i=0;i<tracklen;i++)
	{
		d = f86_get_cell_duration(side, i);

		if( resample )
			o = (uint32_t)( ( t + (d / 2.0) ) / out_period );
		else
			o = (uint32_t)( i / step );

		if( o >= trk->nb_of_cells )
			o = trk->nb_of_cells - 1;

		if( side->databuffer[i>>3] & (0x80 >> (i&7)) )
		{
			trk->data[o>>3] |= (0x80 >> (o&7));
		}

		if( side->flakybitsbuffer && ( side->flakybitsbuffer[i>>3] & (0x80 >> (i&7)) ) )
		{
			// Weak bit : surface bit set + data bit set.
			// (surface bit set + data bit cleared is a hole)
			trk->surface[o>>3] |= (0x80 >> (o&7));
			trk->data[o>>3] |= (0x80 >> (o&7));
			trk->has_weak_bits = 1;
		}

		// Index position : index signal rising edge.
		if( side->indexbuffer && !index_found )
		{
			index_cur = side->indexbuffer[i>>3] ? 1 : 0;
			if( index_cur && !index_prev )
			{
				trk->index_position = o;
				index_found = 1;
			}
			index_prev = index_cur;
		}

		t += d;
	}

	// RPM flag
	rpm = 60.0 / ( (double)trk->nb_of_cells * out_period );

	trk->flags = (uint16_t)( ( encoding << F86_TRKFLAG_ENC_SHIFT ) | rate->code );
	if( rpm > 330.0 )
		trk->flags |= F86_TRKFLAG_RPM_360;

	// Unformatted tracks don't select the disk hole type.
	trk->hole = resample ? 0 : rate->hole;

#ifdef F86_WRITER_DBG
	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"86F writer : Track %d:%d - HxC encoding: %d, HxC cells: %d -> 86F flags: 0x%.4X (%s %d kbps, %d RPM), cells: %d, index: %d",
		cyl,head,
		hxc_encoding,
		tracklen,
		trk->flags,
		(encoding == F86_TRK_ENC_FM)?"FM":( (encoding == F86_TRK_ENC_MFM)?"MFM":( (encoding == F86_TRK_ENC_M2FM)?"M2FM":"GCR") ),
		(encoding == F86_TRK_ENC_FM)?(rate->bitrate/2000):(rate->bitrate/1000),
		(int)(rpm+0.5),
		trk->nb_of_cells,
		trk->index_position);
#endif

	return HXCFE_NOERROR;
}

static int f86_write_u16(FILE * f, uint16_t value)
{
	uint8_t buf[2];

	buf[0] = (uint8_t)( value & 0xFF );
	buf[1] = (uint8_t)( (value >> 8) & 0xFF );

	return ( fwrite(buf, 2, 1, f) == 1 ) ? 0 : -1;
}

static int f86_write_u32(FILE * f, uint32_t value)
{
	uint8_t buf[4];

	buf[0] = (uint8_t)( value & 0xFF );
	buf[1] = (uint8_t)( (value >> 8) & 0xFF );
	buf[2] = (uint8_t)( (value >> 16) & 0xFF );
	buf[3] = (uint8_t)( (value >> 24) & 0xFF );

	return ( fwrite(buf, 4, 1, f) == 1 ) ? 0 : -1;
}

// Main writer function
int F86_libWrite_DiskFile(HXCFE_IMGLDR* imgldr_ctx,HXCFE_FLOPPY * floppy,char * filename)
{
	FILE * f;
	int ret;
	int cyl,head,dup,nb_dup;
	int nb_sides,nb_cyl,max_cyl;
	int thick_mode;
	int hole,has_weak_bits;
	int table_entries,entry;
	uint16_t disk_flags;
	uint32_t offset;
	uint32_t * offsets_table;
	f86_track_t * tracks;
	HXCFE_SIDE * side;
	long file_size;
	uint8_t zero;
	int rate_count[sizeof(f86_rates)/sizeof(f86_rate_t)];
	const f86_rate_t * default_rate;

	f = NULL;
	tracks = NULL;
	offsets_table = NULL;
	ret = HXCFE_NOERROR;

	imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"Write 86F file %s...",filename);

	if( !floppy || !floppy->tracks || floppy->floppyNumberOfTrack <= 0 || floppy->floppyNumberOfSide <= 0 )
		return HXCFE_BADPARAMETER;

	nb_sides = (floppy->floppyNumberOfSide >= 2) ? 2 : 1;
	nb_cyl = floppy->floppyNumberOfTrack;

	// Thin/Thick tracks : 86F images are in thin tracks geometry.
	// 40 cylinders disks must have each track written twice.
	thick_mode = hxcfe_getEnvVarValue( imgldr_ctx->hxcfe, "F86EXPORT_THICK_TRACKS" );
	switch( thick_mode )
	{
		case F86_THICK_TRACKS_FORCE:
			nb_dup = 2;
		break;
		case F86_THICK_TRACKS_NEVER:
			nb_dup = 1;
		break;
		default:
			nb_dup = ( nb_cyl <= F86_THICK_TRACKS_AUTO_MAX_CYL ) ? 2 : 1;
		break;
	}

	table_entries = (nb_sides == 2) ? F86_TRACK_TABLE_ENTRIES : (F86_TRACK_TABLE_ENTRIES / 2);

	max_cyl = table_entries / ( (nb_sides == 2) ? 2 : 1 ) / nb_dup;
	if( nb_cyl > max_cyl )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_WARNING,"86F writer : Too many tracks (%d) ! Only the first %d tracks will be written.",nb_cyl,max_cyl);
		nb_cyl = max_cyl;
	}

	if( nb_dup == 2 )
		imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"86F writer : %d cylinders disk, thick tracks mode (each track written twice).",nb_cyl);

	tracks = calloc( nb_cyl * nb_sides, sizeof(f86_track_t) );
	offsets_table = calloc( table_entries, sizeof(uint32_t) );
	if( !tracks || !offsets_table )
	{
		ret = HXCFE_INTERNALERROR;
		goto error;
	}

	// Disk rate : the most common rate of the encoded tracks.
	// (Used for the unformatted tracks)
	memset(rate_count,0,sizeof(rate_count));
	for(cyl=0;cyl<nb_cyl;cyl++)
	{
		for(head=0;head<nb_sides;head++)
		{
			if( floppy->tracks[cyl] && floppy->tracks[cyl]->sides && head < floppy->tracks[cyl]->number_of_side )
			{
				side = floppy->tracks[cyl]->sides[head];
				if( side && side->tracklen > 0 && f86_get_track_encoding(side) != UNKNOWN_ENCODING )
				{
					rate_count[ f86_get_side_rate(side) - f86_rates ]++;
				}
			}
		}
	}

	default_rate = &f86_rates[0];
	for(entry=0;entry<(int)(sizeof(f86_rates)/sizeof(f86_rate_t));entry++)
	{
		if( rate_count[entry] > rate_count[default_rate - f86_rates] )
			default_rate = &f86_rates[entry];
	}

	// Convert all the tracks first : the disk flags depend on their content.
	hole = 0;
	has_weak_bits = 0;

	for(cyl=0;cyl<nb_cyl;cyl++)
	{
		for(head=0;head<nb_sides;head++)
		{
			hxcfe_imgCallProgressCallback(imgldr_ctx, (cyl*nb_sides)+head, nb_cyl*nb_sides*2 );

			side = NULL;
			if( floppy->tracks[cyl] && floppy->tracks[cyl]->sides && head < floppy->tracks[cyl]->number_of_side )
				side = floppy->tracks[cyl]->sides[head];

			if( !side )
			{
				imgldr_ctx->hxcfe->hxc_printf(MSG_WARNING,"86F writer : Track %d:%d not present !",cyl,head);
				continue;
			}

			ret = f86_convert_side(imgldr_ctx, side, cyl, head, default_rate, &tracks[(cyl*nb_sides)+head]);
			if( ret == HXCFE_INTERNALERROR )
				goto error;

			ret = HXCFE_NOERROR;

			if( tracks[(cyl*nb_sides)+head].data )
			{
				if( tracks[(cyl*nb_sides)+head].hole > hole )
					hole = tracks[(cyl*nb_sides)+head].hole;

				if( tracks[(cyl*nb_sides)+head].has_weak_bits )
					has_weak_bits = 1;
			}
		}
	}

	if( !tracks[0].data || ( nb_sides == 2 && !tracks[1].data ) )
	{
		// 86Box refuses images without track 0.
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"86F writer : Track 0 missing !");
		ret = HXCFE_BADPARAMETER;
		goto error;
	}

	disk_flags = F86_DISKFLAG_BITCELL_COUNT | F86_DISKFLAG_TOTAL_BITCELLS;
	disk_flags |= (uint16_t)( (hole & 3) << F86_DISKFLAG_HOLE_SHIFT );

	if( nb_sides == 2 )
		disk_flags |= F86_DISKFLAG_DOUBLE_SIDED;

	if( hxcfe_floppyGetFlags( imgldr_ctx->hxcfe, floppy ) & HXCFE_FLOPPY_WRPROTECTED_FLAG )
		disk_flags |= F86_DISKFLAG_WRITE_PROTECT;

	if( has_weak_bits )
		disk_flags |= F86_DISKFLAG_SURFACE_DATA;

	imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"86F writer : %d cylinder(s), %d side(s), disk flags : 0x%.4X",nb_cyl,nb_sides,disk_flags);

	f = hxc_fopen(filename,"wb");
	if( !f )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"86F writer : Cannot create %s !",filename);
		ret = HXCFE_ACCESSERROR;
		goto error;
	}

	// File header
	if( fwrite("86BF", 4, 1, f) != 1 )
		goto write_error;

	zero = F86_FORMAT_MINOR_VERSION;
	if( fwrite(&zero, 1, 1, f) != 1 )
		goto write_error;

	zero = F86_FORMAT_MAJOR_VERSION;
	if( fwrite(&zero, 1, 1, f) != 1 )
		goto write_error;

	if( f86_write_u16(f, disk_flags) )
		goto write_error;

	// Tracks offsets table (filled later)
	for(entry=0;entry<table_entries;entry++)
	{
		if( f86_write_u32(f, 0) )
			goto write_error;
	}

	offset = 8 + ( table_entries * 4 );

	// Tracks
	for(cyl=0;cyl<nb_cyl;cyl++)
	{
		for(dup=0;dup<nb_dup;dup++)
		{
			for(head=0;head<nb_sides;head++)
			{
				f86_track_t * trk = &tracks[(cyl*nb_sides)+head];

				hxcfe_imgCallProgressCallback(imgldr_ctx, (nb_cyl*nb_sides) + (cyl*nb_sides) + head, nb_cyl*nb_sides*2 );

				if( !trk->data )
					continue;

				if( nb_sides == 2 )
					entry = ( ( (cyl*nb_dup) + dup ) << 1 ) + head;
				else
					entry = (cyl*nb_dup) + dup;

				offsets_table[entry] = offset;

				if( f86_write_u16(f, trk->flags) )
					goto write_error;

				if( f86_write_u32(f, trk->nb_of_cells) )   // Total bit cells count
					goto write_error;

				if( f86_write_u32(f, trk->index_position) )
					goto write_error;

				if( fwrite(trk->data, trk->size, 1, f) != 1 )
					goto write_error;

				offset += 2 + 4 + 4 + trk->size;

				if( disk_flags & F86_DISKFLAG_SURFACE_DATA )
				{
					if( fwrite(trk->surface, trk->size, 1, f) != 1 )
						goto write_error;

					offset += trk->size;
				}
			}
		}
	}

	// 86Box refuses files smaller than 51052 bytes : pad.
	file_size = (long)offset;
	zero = 0;
	while( file_size < F86_MIN_FILE_SIZE )
	{
		if( fwrite(&zero, 1, 1, f) != 1 )
			goto write_error;

		file_size++;
	}

	// Write the final tracks offsets table
	if( fseek(f, 8, SEEK_SET) )
		goto write_error;

	for(entry=0;entry<table_entries;entry++)
	{
		if( f86_write_u32(f, offsets_table[entry]) )
			goto write_error;
	}

	hxc_fclose(f);
	f = NULL;

	for(cyl=0;cyl<nb_cyl*nb_sides;cyl++)
		f86_free_track(&tracks[cyl]);

	free(tracks);
	free(offsets_table);

	return HXCFE_NOERROR;

write_error:
	imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"86F writer : Write error (%s) !",filename);
	ret = HXCFE_ACCESSERROR;

error:
	if( f )
		hxc_fclose(f);

	if( tracks )
	{
		for(cyl=0;cyl<nb_cyl*nb_sides;cyl++)
			f86_free_track(&tracks[cyl]);

		free(tracks);
	}

	free(offsets_table);

	return ret;
}
