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
// File : stx_writer.c
// Contains: STX floppy image writer
//
// Written by: Jean-François DEL NERO
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "types.h"

#include "internal_libhxcfe.h"
#include "tracks/track_generator.h"
#include "libhxcfe.h"
#include "stx_loader.h"
#include "stx_writer.h"
#include "pasti_format.h"
#include "floppy_utils.h"
#include "tracks/sector_extractor.h"
#include "tracks/trackutils.h"
#include "libhxcadaptor.h"

//
// Pasti / STX file layout written by this module (see pasti_format.h) :
//
// File descriptor (16 bytes)
// For each track/side record :
//   Track descriptor (16 bytes)
//   Sector descriptors (16 bytes each)
//   Fuzzy mask record (concatenated masks of the fuzzy sectors)
//   Track data record :
//     Track image header (2 bytes : image size)
//     Track image : What a WD1772 "Read Track" command would return (padded to an even size)
//     Sector images : Data of each sector having a data field
//   Timing record (only if at least one sector has the PASTI_SECT_VARIABLE_TIME flag)
//
// Sector descriptors data offsets are relative to the start of the track data record.
//

#define STX_MAX_TRACK_RECORDS   0xA8          // 84 tracks, 2 sides
#define STX_MAX_SECTOR_SIZE     1024          // WD1772 : Only 2 bits of the size ID are used.
#define STX_MAX_TIMING_BLOCKS   (STX_MAX_SECTOR_SIZE/PASTI_TIMING_BLOCK_SIZE)

#define MFM_SYNC_A1             0x4489        // A1 with missing clock
#define MFM_SYNC_C2             0x5224        // C2 with missing clock

typedef struct stx_wr_sector_
{
	pasti_sector    desc;

	unsigned char * data;                     // Sector data (data_size bytes), NULL if no data field.
	int             data_size;

	unsigned char * fuzzy_mask;               // Fuzzy mask (data_size bytes), NULL if no fuzzy bits.

	unsigned short  timing[STX_MAX_TIMING_BLOCKS];
	int             timing_blocks;            // > 0 if the sector has a timing record.

	int             data_offset;              // Offset of the sector image in the track data record
	int             data_cell;                // Position of the first data cell in the track.
	int             in_track_image;           // The sector data are inside the track image.
}stx_wr_sector;

static void put16le(unsigned char * buf, unsigned short val)
{
	buf[0] = val & 0xFF;
	buf[1] = (val >> 8) & 0xFF;
}

static void put16be(unsigned char * buf, unsigned short val)
{
	buf[0] = (val >> 8) & 0xFF;
	buf[1] = val & 0xFF;
}

static void put32le(unsigned char * buf, uint32_t val)
{
	buf[0] = val & 0xFF;
	buf[1] = (val >> 8) & 0xFF;
	buf[2] = (val >> 16) & 0xFF;
	buf[3] = (val >> 24) & 0xFF;
}

static int cellpos(int cell, int tracklen)
{
	cell %= tracklen;
	if(cell < 0)
		cell += tracklen;

	return cell;
}

// Duration in microseconds of nb_cells cells starting at the cell start_cell.
static double measure_us(HXCFE* hxcfe, HXCFE_SIDE * side, int start_cell, int nb_cells)
{
	if( nb_cells <= 0 || side->tracklen <= 0 )
		return 0;

	if( nb_cells >= side->tracklen )
		nb_cells = side->tracklen - 1;

	return MeasureTrackTiming(hxcfe, side, cellpos(start_cell, side->tracklen), cellpos(start_cell + nb_cells, side->tracklen)) * 1000000.0;
}

////////////////////////////////////////////////////////////////////////////////
// Resample the track cells to the WD1772 nominal MFM cell period (2us @ 250kbps).
//
// The HxC track cells may not be at the nominal bitrate (variable bitrate
// tracks, non formatted tracks decoded at another bitrate, ...). The flux
// transitions positions are computed with the track timing buffer and are
// re-quantized with a simple digital PLL tracking the cell period, like the
// WD1772 data separator.
//
// cells_map (optional) receives for each source cell the index of the
// corresponding resampled cell.
////////////////////////////////////////////////////////////////////////////////
static unsigned char * resample_track(HXCFE_SIDE * side, int * out_nb_cells, int * cells_map)
{
	int i, n, k, nb_cells, buf_cells;
	double t, last_t, cell_time, period, nominal, elapsed;
	unsigned char * buf;
	uint32_t bitrate;

	*out_nb_cells = 0;

	nominal = 1.0 / (2.0 * (double)DEFAULT_DD_BITRATE);
	period = nominal;

	// Worst case : one resampled cell per source cell (+margin)
	buf_cells = side->tracklen + 64;
	buf = malloc( (buf_cells / 8) + 1 );
	if( !buf )
		return NULL;

	memset(buf, 0, (buf_cells / 8) + 1);

	t = 0;
	last_t = 0;
	nb_cells = 0;

	for( i = 0 ; i < side->tracklen ; i++ )
	{
		if( side->timingbuffer )
			bitrate = side->timingbuffer[i >> 3];
		else
			bitrate = side->bitrate;

		if( bitrate <= 0 || bitrate > 10000000 )
			bitrate = DEFAULT_DD_BITRATE;

		cell_time = 1.0 / (2.0 * (double)bitrate);
		t += cell_time;

		if( getbit(side->databuffer, i) )
		{
			elapsed = t - last_t;

			n = (int)floor( (elapsed / period) + 0.5 );
			if( n < 1 )
				n = 1;

			// PLL : Follow slowly the cell period, within +/- 8% of the nominal period.
			if( n <= 4 )
			{
				period += ( (elapsed / (double)n) - period ) * 0.05;
				if( period < nominal * 0.92 )
					period = nominal * 0.92;
				if( period > nominal * 1.08 )
					period = nominal * 1.08;
			}

			for( k = 0 ; k < n - 1 && nb_cells < buf_cells ; k++ )
				nb_cells++;               // '0' cells (buffer already cleared)

			if( nb_cells < buf_cells )
			{
				setbit(buf, nb_cells, 1);
				nb_cells++;
			}

			last_t = t;
		}

		if( cells_map )
			cells_map[i] = nb_cells;
	}

	// Remaining time without transition up to the index.
	n = (int)floor( ((t - last_t) / period) + 0.5 );
	for( k = 0 ; k < n && nb_cells < buf_cells ; k++ )
		nb_cells++;

	if( nb_cells < 16 )
	{
		free(buf);
		return NULL;
	}

	*out_nb_cells = nb_cells;

	return buf;
}

////////////////////////////////////////////////////////////////////////////////
// WD1772 "Read Track" command emulation.
//
// The track is read from the index, the bytes are assembled from the MFM cells
// with the same synchronization behavior than the WD1772 :
//
// - The controller resynchronizes on the A1 (0x4489) and C2 (0x5224) MFM
//   sync patterns during the whole command (even inside the sectors data).
// - Before the first sync the bytes are assembled with an arbitrary alignment.
// - On a 0x5224 pattern, the byte being assembled is immediately transferred
//   (if at least 4 cells have been shifted in since the last byte).
//   Since a "00 A1" sequence contains this pattern, the first A1 of a sync
//   sequence is generally read as 0x14 or 0xC2 ("... 00 14 A1 A1 FE ...").
// - On a 0x4489 pattern the byte boundary is realigned on the A1.
//
// This model reproduces the track images produced by the Pasti imaging tool.
//
// bytes_pos (optional) receives the cell index of the end of each byte.
////////////////////////////////////////////////////////////////////////////////
static int wd1772_read_track(unsigned char * cells, int nb_cells, unsigned char * outbuf, int * bytes_pos, int outbuf_size)
{
	int i, nb_bytes, cnt, data_phase, cell;
	uint16_t shift_reg;
	uint8_t data_reg;

	nb_bytes = 0;
	shift_reg = 0;
	data_reg = 0;
	data_phase = 0;
	cnt = 0;

	for( i = 0 ; i < nb_cells && nb_bytes < outbuf_size ; i++ )
	{
		cell = getbit(cells, i);

		shift_reg = (uint16_t)( (shift_reg << 1) | cell );

		if( data_phase )
			data_reg = (uint8_t)( (data_reg << 1) | cell );

		data_phase ^= 1;
		cnt++;

		if( shift_reg == MFM_SYNC_C2 || shift_reg == MFM_SYNC_A1 )
		{
			if( cnt >= 16 || ( cnt >= 4 && shift_reg == MFM_SYNC_C2 ) )
			{
				if( bytes_pos )
					bytes_pos[nb_bytes] = i + 1;

				outbuf[nb_bytes++] = data_reg;
			}

			// Resync : The next cell is a clock cell.
			cnt = 0;
			data_phase = 0;
			continue;
		}

		if( cnt >= 16 )
		{
			if( bytes_pos )
				bytes_pos[nb_bytes] = i + 1;

			outbuf[nb_bytes++] = data_reg;
			cnt = 0;
		}
	}

	return nb_bytes;
}

////////////////////////////////////////////////////////////////////////////////
// Fill a sector descriptor and its associated data from an extracted sector.
// Return 0 if the sector must be ignored.
////////////////////////////////////////////////////////////////////////////////
static int build_sector(HXCFE* hxcfe, HXCFE_SIDE * side, int * cells_map, HXCFE_SECTCFG * sc, stx_wr_sector * ws, int variable_timing_threshold)
{
	int i, k, data_start, cell, fuzzy, size_id;
	int datamark;
	double total_us, block_us, avg_us, min_us, max_us;

	memset(ws, 0, sizeof(stx_wr_sector));

	// Data field without ID field : Not visible by the WD1772 Read sector command.
	if( !sc->use_alternate_addressmark )
		return 0;

	size_id = sc->alternate_sector_size_id & 0xFF;

	ws->desc.track_num   = (uint8_t)sc->cylinder;
	ws->desc.side_num    = (uint8_t)sc->head;
	ws->desc.sector_num  = (uint8_t)sc->sector;
	ws->desc.sector_size = (uint8_t)size_id;
	// Stored as in the address field (big endian)
	ws->desc.header_crc  = (uint16_t)( ((sc->header_crc >> 8) & 0xFF) | ((sc->header_crc & 0xFF) << 8) );
	ws->desc.sector_flags = 0x00;

	// Position (in bits at the nominal bitrate) of the sector, just after the ID address mark ($FE).
	if( cells_map )
		ws->desc.sector_pos_timing = (uint16_t)( (cells_map[cellpos(sc->startsectorindex + ((4 * 8 * 2) - 1), side->tracklen)] + 1) / 2 );
	else
		ws->desc.sector_pos_timing = (uint16_t)( (sc->startsectorindex / 2) + (4 * 8) );

	if( sc->use_alternate_header_crc )
	{
		// Bad ID field CRC : The WD1772 doesn't read the data field.
		ws->desc.FDC_status = PASTI_SECT_RNF | PASTI_SECT_CRC;
		return 1;
	}

	if( !sc->input_data || sc->sectorsize <= 0 )
	{
		// No data field found
		ws->desc.FDC_status = PASTI_SECT_RNF;
		return 1;
	}

	// The WD1772 only uses 2 bits of the size ID field.
	ws->data_size = 128 << (size_id & 3);

	ws->data = malloc(ws->data_size);
	if( !ws->data )
		return -1;

	memset(ws->data, 0x00, ws->data_size);
	memcpy(ws->data, sc->input_data, ws->data_size < sc->sectorsize ? ws->data_size : sc->sectorsize);

	ws->desc.FDC_status = 0x00;

	if( sc->use_alternate_data_crc )
		ws->desc.FDC_status |= PASTI_SECT_CRC;

	datamark = sc->alternate_datamark & 0xFF;
	if( datamark == 0xF8 || datamark == 0xF9 )
		ws->desc.FDC_status |= PASTI_SECT_RECORD_TYPE;  // Deleted data

	// startdataindex : first A1 of the data mark -> Skip the A1 A1 A1 Fx sequence.
	data_start = sc->startdataindex + ( (3 + 1) * 8 * 2 );
	ws->data_cell = cellpos(data_start, side->tracklen);

	// Fuzzy bits. Only for sectors with a bad data CRC (A stable sector doesn't have fuzzy bits).
	if( (ws->desc.FDC_status & PASTI_SECT_CRC) && side->flakybitsbuffer )
	{
		fuzzy = 0;

		ws->fuzzy_mask = malloc(ws->data_size);
		if( !ws->fuzzy_mask )
			return -1;

		memset(ws->fuzzy_mask, 0xFF, ws->data_size);

		for( i = 0 ; i < ws->data_size * 8 ; i++ )
		{
			// Clock cell and data cell of the bit i
			for( k = 0 ; k < 2 ; k++ )
			{
				cell = cellpos(data_start + (i * 2) + k, side->tracklen);
				if( getbit(side->flakybitsbuffer, cell) )
				{
					ws->fuzzy_mask[i >> 3] &= ~(0x80 >> (i & 7));
					fuzzy = 1;
				}
			}
		}

		if( fuzzy )
		{
			ws->desc.FDC_status |= PASTI_SECT_FUZZY;
		}
		else
		{
			free(ws->fuzzy_mask);
			ws->fuzzy_mask = NULL;
		}
	}

	// Data field read time (us)
	total_us = measure_us(hxcfe, side, data_start, ws->data_size * 8 * 2);
	if( total_us > 0xFFFF )
		total_us = 0xFFFF;

	ws->desc.sector_speed_timing = (uint16_t)(total_us + 0.5);

	// Intra sector bit width variation detection (Macrodos/Speedlock like protections)
	if( variable_timing_threshold > 0 && side->timingbuffer && ws->data_size >= PASTI_TIMING_BLOCK_SIZE )
	{
		ws->timing_blocks = ws->data_size / PASTI_TIMING_BLOCK_SIZE;

		avg_us = total_us / (double)ws->timing_blocks;
		min_us = 1E9;
		max_us = 0;

		for( i = 0 ; i < ws->timing_blocks ; i++ )
		{
			block_us = measure_us(hxcfe, side, data_start + (i * PASTI_TIMING_BLOCK_SIZE * 8 * 2), PASTI_TIMING_BLOCK_SIZE * 8 * 2);

			if( block_us < min_us )
				min_us = block_us;

			if( block_us > max_us )
				max_us = block_us;

			// 1 unit = 32 FDC cycles @ 8MHz, + 28 cycles per block.
			k = (int)floor( ( ( (block_us * 8.0) - 28.0 ) / 32.0 ) + 0.5 );
			if( k < 0 )
				k = 0;
			if( k > 0xFFFF )
				k = 0xFFFF;

			ws->timing[i] = (unsigned short)k;
		}

		// Some blocks faster and some blocks slower than the sector average ?
		if( avg_us > 0 &&
			( ( max_us - avg_us ) * 100.0 > avg_us * (double)variable_timing_threshold ) &&
			( ( avg_us - min_us ) * 100.0 > avg_us * (double)variable_timing_threshold ) )
		{
			ws->desc.FDC_status |= PASTI_SECT_VARIABLE_TIME;
		}
		else
		{
			ws->timing_blocks = 0;
		}
	}

	return 1;
}

static void free_sectors(stx_wr_sector * ws, int nb)
{
	int i;

	if( !ws )
		return;

	for( i = 0 ; i < nb ; i++ )
	{
		free(ws[i].data);
		free(ws[i].fuzzy_mask);
	}

	free(ws);
}

////////////////////////////////////////////////////////////////////////////////
// Build and write a track record
////////////////////////////////////////////////////////////////////////////////
static int write_track_record(HXCFE_IMGLDR* imgldr_ctx, HXCFE_FLOPPY * floppy, HXCFE_SECTORACCESS* ss, FILE * f, int track, int side, int track_image, int variable_timing_threshold)
{
	HXCFE* hxcfe;
	HXCFE_SIDE * currentside;
	HXCFE_SECTCFG ** sca;
	stx_wr_sector * ws;
	int nb_sca, nb_sectors, nb_ext_sectors, i, ret;
	int fuzzy_size, image_size, image_record_size, sectors_data_size, timing_size;
	int track_length;
	unsigned short flags;
	uint32_t record_size;
	unsigned char * image;
	unsigned char * record;
	unsigned char * ptr;
	unsigned char * cells;
	int * cells_map;
	int * bytes_pos;
	int nb_cells;

	hxcfe = imgldr_ctx->hxcfe;

	ws = NULL;
	sca = NULL;
	image = NULL;
	record = NULL;
	cells = NULL;
	cells_map = NULL;
	bytes_pos = NULL;
	nb_cells = 0;
	nb_sca = 0;
	nb_sectors = 0;
	image_size = 0;
	track_length = 0;
	ret = HXCFE_INTERNALERROR;

	currentside = NULL;
	if( track < floppy->floppyNumberOfTrack && floppy->tracks[track] && floppy->tracks[track]->sides && side < floppy->tracks[track]->number_of_side )
		currentside = floppy->tracks[track]->sides[side];

	if( currentside && currentside->databuffer && currentside->tracklen > 0 )
	{
		track_length = currentside->tracklen / 16;

		// Track cells at the WD1772 nominal bitrate
		cells_map = malloc(sizeof(int) * currentside->tracklen);
		if( !cells_map )
			goto error;

		cells = resample_track(currentside, &nb_cells, cells_map);
		if( !cells )
		{
			free(cells_map);
			cells_map = NULL;
		}
		else
		{
			track_length = nb_cells / 16;
		}

		// Sectors
		sca = hxcfe_getAllTrackSectors(ss, track, side, ISOIBM_MFM_ENCODING, &nb_sca);
		if( sca && nb_sca > 0 )
		{
			ws = malloc(sizeof(stx_wr_sector) * nb_sca);
			if( !ws )
				goto error;

			memset(ws, 0, sizeof(stx_wr_sector) * nb_sca);

			for( i = 0 ; i < nb_sca ; i++ )
			{
				if( sca[i] )
				{
					switch( build_sector(hxcfe, currentside, cells_map, sca[i], &ws[nb_sectors], variable_timing_threshold) )
					{
						case 1:
							nb_sectors++;
						break;
						case 0:
						break;
						default:
							nb_sectors++;
							goto error;
						break;
					}
				}
			}
		}

		// Track image
		if( track_image && cells )
		{
			image = malloc( (nb_cells / 4) + 16 );
			bytes_pos = malloc( sizeof(int) * ((nb_cells / 4) + 16) );
			if( !image || !bytes_pos )
				goto error;

			image_size = wd1772_read_track(cells, nb_cells, image, bytes_pos, (nb_cells / 4) + 16);
			if( image_size > 0xFFFE )
				image_size = 0xFFFE;

			track_length = image_size;

			// Sector data matching the track image data : point inside the track image.
			for( i = 0 ; i < nb_sectors ; i++ )
			{
				if( ws[i].data )
				{
					int start, j, k;

					// Image byte containing the first sector data bit.
					start = cells_map[ws[i].data_cell];
					j = 0;
					while( j < image_size && bytes_pos[j] <= start )
						j++;

					for( k = j - 2 ; k <= j + 2 ; k++ )
					{
						if( k >= 0 && k + ws[i].data_size <= image_size )
						{
							if( !memcmp(&image[k], ws[i].data, ws[i].data_size) )
							{
								ws[i].in_track_image = 1;
								ws[i].data_offset = 2 + k;
								break;
							}
						}
					}
				}
			}
		}
	}

	if( track_length > 0xFFFF )
		track_length = 0xFFFF;

	// Compute the records sizes and offsets
	fuzzy_size = 0;
	sectors_data_size = 0;
	timing_size = 0;

	nb_ext_sectors = 0;
	for( i = 0 ; i < nb_sectors ; i++ )
	{
		if( ws[i].fuzzy_mask )
			fuzzy_size += ws[i].data_size;

		if( ws[i].data && !ws[i].in_track_image )
			nb_ext_sectors++;

		if( ws[i].timing_blocks )
			timing_size += ws[i].timing_blocks * 2;
	}

	image_record_size = 0;
	if( image )
	{
		image_record_size = 2 + image_size; // Header + image

		// Sectors images are aligned on an even offset. Without sector image, the
		// timing record (if any) is expected just after the track image (no padding).
		if( (image_size & 1) && ( nb_ext_sectors || !timing_size ) )
			image_record_size++;
	}

	for( i = 0 ; i < nb_sectors ; i++ )
	{
		if( ws[i].data && !ws[i].in_track_image )
		{
			ws[i].data_offset = image_record_size + sectors_data_size;
			sectors_data_size += ws[i].data_size;
		}
	}

	if( timing_size )
		timing_size += sizeof(pasti_timingheader);

	record_size = sizeof(pasti_trackheader) + (sizeof(pasti_sector) * nb_sectors) + fuzzy_size + image_record_size + sectors_data_size + timing_size;
	record_size += (record_size & 1);

	record = malloc(record_size);
	if( !record )
		goto error;

	memset(record, 0, record_size);

	flags = PASTI_TRK_SECT | PASTI_TRK_PROT;
	if( image )
		flags |= PASTI_TRK_IMAGE;

	// Track descriptor
	ptr = record;
	put32le(&ptr[0], record_size);
	put32le(&ptr[4], fuzzy_size);
	put16le(&ptr[8], (unsigned short)nb_sectors);
	put16le(&ptr[10], flags);
	put16le(&ptr[12], (unsigned short)track_length);
	ptr[14] = (unsigned char)( ((side & 1) << 7) | (track & 0x7F) );
	ptr[15] = 0x00;
	ptr += sizeof(pasti_trackheader);

	// Sector descriptors
	for( i = 0 ; i < nb_sectors ; i++ )
	{
		put32le(&ptr[0], ws[i].data ? ws[i].data_offset : 0);
		put16le(&ptr[4], ws[i].desc.sector_pos_timing);
		put16le(&ptr[6], ws[i].desc.sector_speed_timing);
		ptr[8]  = ws[i].desc.track_num;
		ptr[9]  = ws[i].desc.side_num;
		ptr[10] = ws[i].desc.sector_num;
		ptr[11] = ws[i].desc.sector_size;
		ptr[12] = ws[i].desc.header_crc & 0xFF;        // CRC high byte
		ptr[13] = (ws[i].desc.header_crc >> 8) & 0xFF; // CRC low byte
		ptr[14] = ws[i].desc.FDC_status;
		ptr[15] = ws[i].desc.sector_flags;
		ptr += sizeof(pasti_sector);
	}

	// Fuzzy mask record
	for( i = 0 ; i < nb_sectors ; i++ )
	{
		if( ws[i].fuzzy_mask )
		{
			memcpy(ptr, ws[i].fuzzy_mask, ws[i].data_size);
			ptr += ws[i].data_size;
		}
	}

	// Track data record : Track image
	if( image )
	{
		put16le(&ptr[0], (unsigned short)image_size);
		memcpy(&ptr[2], image, image_size);
		ptr += image_record_size;
	}

	// Track data record : Sectors images
	for( i = 0 ; i < nb_sectors ; i++ )
	{
		if( ws[i].data && !ws[i].in_track_image )
		{
			memcpy(ptr, ws[i].data, ws[i].data_size);
			ptr += ws[i].data_size;
		}
	}

	// Timing record
	if( timing_size )
	{
		put16le(&ptr[0], PASTI_TIMING_FLAGS);
		put16le(&ptr[2], (unsigned short)timing_size);
		ptr += sizeof(pasti_timingheader);

		for( i = 0 ; i < nb_sectors ; i++ )
		{
			if( ws[i].timing_blocks )
			{
				int b;
				for( b = 0 ; b < ws[i].timing_blocks ; b++ )
				{
					put16be(ptr, ws[i].timing[b]);
					ptr += 2;
				}
			}
		}
	}

	if( fwrite(record, record_size, 1, f) != 1 )
		ret = HXCFE_ACCESSERROR;
	else
		ret = HXCFE_NOERROR;

	hxcfe->hxc_printf(MSG_DEBUG,"STX writer : Track %d Side %d - %d sector(s), track image : %d bytes, fuzzy : %d bytes, timing : %d bytes",track,side,nb_sectors,image_size,fuzzy_size,timing_size);

error:
	free(record);
	free(image);
	free(bytes_pos);
	free(cells);
	free(cells_map);

	free_sectors(ws, nb_sectors);

	if( sca )
	{
		for( i = 0 ; i < nb_sca ; i++ )
		{
			if( sca[i] )
				hxcfe_freeSectorConfig(ss, sca[i]);
		}
		free(sca);
	}

	return ret;
}

// Return 1 if at least one MFM sector is present on this side.
static int side_has_sectors(HXCFE_FLOPPY * floppy, HXCFE_SECTORACCESS* ss, int side)
{
	int i, found;
	HXCFE_SECTCFG * sc;

	found = 0;

	for( i = 0 ; i < floppy->floppyNumberOfTrack && !found ; i++ )
	{
		if( !floppy->tracks[i] || side >= floppy->tracks[i]->number_of_side || !floppy->tracks[i]->sides[side] )
			continue;

		hxcfe_resetSearchTrackPosition(ss);

		sc = hxcfe_getNextSector(ss, i, side, ISOIBM_MFM_ENCODING);
		if( sc )
		{
			found = 1;
			hxcfe_freeSectorConfig(ss, sc);
		}
	}

	hxcfe_resetSearchTrackPosition(ss);

	return found;
}

// Main writer function
int STX_libWrite_DiskFile(HXCFE_IMGLDR* imgldr_ctx,HXCFE_FLOPPY * floppy,char * filename)
{
	int i,j,ret;
	FILE * stxdskfile;
	HXCFE_SECTORACCESS* ss;
	int number_of_side;
	int number_of_track;
	int track_image;
	int variable_timing_threshold;
	unsigned char header[sizeof(pasti_fileheader)];

	imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"Write STX file %s...",filename);

	track_image = hxcfe_getEnvVarValue( imgldr_ctx->hxcfe, "STXEXPORT_TRACK_IMAGE" );
	variable_timing_threshold = hxcfe_getEnvVarValue( imgldr_ctx->hxcfe, "STXEXPORT_VARIABLE_TIMING_THRESHOLD" );

	ss = hxcfe_initSectorAccess(imgldr_ctx->hxcfe,floppy);
	if( !ss )
		return HXCFE_INTERNALERROR;

	number_of_track = floppy->floppyNumberOfTrack;
	number_of_side = 1;
	if( floppy->floppyNumberOfSide >= 2 )
	{
		number_of_side = 2;

		// Single sided disk dumped with a double sided drive ? (No sector on the side 1)
		if( !side_has_sectors(floppy, ss, 1) )
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"STX writer : No sector found on the side 1 - single sided disk.");
			number_of_side = 1;
		}
	}

	if( number_of_track * number_of_side > STX_MAX_TRACK_RECORDS )
		number_of_track = STX_MAX_TRACK_RECORDS / number_of_side;

	// File descriptor
	memset(header, 0, sizeof(header));
	header[0] = 'R';
	header[1] = 'S';
	header[2] = 'Y';
	header[3] = 0;
	put16le(&header[4], PASTI_FILE_VERSION);           // Version
	put16le(&header[6], 0x0001);                       // Imaging tool
	put16le(&header[8], 0x0000);                       // Reserved
	header[10] = (unsigned char)(number_of_track * number_of_side); // Number of track records
	header[11] = PASTI_FILE_REVISION_NEW;              // Revision
	put32le(&header[12], 0x00000000);                  // Reserved

	stxdskfile = hxc_fopen(filename,"wb");
	if( !stxdskfile )
	{
		hxcfe_deinitSectorAccess(ss);
		return HXCFE_ACCESSERROR;
	}

	ret = HXCFE_NOERROR;

	if( fwrite(header, sizeof(header), 1, stxdskfile) != 1 )
		ret = HXCFE_ACCESSERROR;

	for( i = 0 ; i < number_of_track && ret == HXCFE_NOERROR ; i++ )
	{
		for( j = 0 ; j < number_of_side && ret == HXCFE_NOERROR ; j++ )
		{
			hxcfe_imgCallProgressCallback(imgldr_ctx, (i * number_of_side) + j, number_of_track * number_of_side );

			ret = write_track_record(imgldr_ctx, floppy, ss, stxdskfile, i, j, track_image, variable_timing_threshold);
		}
	}

	hxc_fclose(stxdskfile);

	hxcfe_deinitSectorAccess(ss);

	return ret;
}
