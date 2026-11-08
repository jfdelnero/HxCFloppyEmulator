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
// File : stx_loader.c
// Contains: STX floppy image loader
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

#include "stx_loader.h"
#include "stx_writer.h"

#include "pasti_format.h"

#include "libhxcadaptor.h"
#include "tracks/crc.h"

// Enable the debug outputs by default
#define PASTI_DBG 1

int STX_libIsValidDiskFile( HXCFE_IMGLDR * imgldr_ctx, HXCFE_IMGLDR_FILEINFOS * imgfile )
{
	pasti_fileheader * fileheader;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"STX_libIsValidDiskFile");

	if(hxc_checkfileext(imgfile->path,"stx",SYS_PATH_TYPE))
	{
		fileheader = (pasti_fileheader*)imgfile->file_header;

		if(strcmp((char*)&fileheader->headertag,"RSY"))
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"STX_libIsValidDiskFile : non STX file (bad header)!");
			return HXCFE_BADFILE;
		}
		else
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"STX_libIsValidDiskFile : STX file !");
			return HXCFE_VALIDFILE;
		}
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"STX_libIsValidDiskFile : non STX file !");
		return HXCFE_BADFILE;
	}
}

#define STX_MAX_SECTOR_SIZE   1024
#define STX_DAM_SEARCH_WINDOW 64     // Max bytes between the ID field end and the data mark

// Macrodos/Speedlock default timing table (revision 0 files)
// Timing value for each quarter of the sector.
static const unsigned short macrodos_timing_table[4] = { 127, 133, 121, 127 };

typedef struct stx_sector_
{
	pasti_sector    desc;

	int             data_size;      // Sector data size (128 << (size ID & 3))
	unsigned char * data;           // Sector data (NULL if no data)
	unsigned char * fuzzy_mask;     // Fuzzy mask (NULL if no fuzzy bits)
	unsigned char * timing;         // Big endian timing values (1 per 16 bytes block), NULL if none.
	int             macrodos_timing;// Use the default Macrodos timing table (revision 0)
	int             keep_header_crc;// Use the ID field CRC from the sector descriptor

	int             id_pos;         // Position of the ID mark ($FE) in the track image
	int             data_pos;       // Position of the first data byte in the track image
}stx_sector;

static unsigned short get16le(unsigned char * buf)
{
	return (unsigned short)( buf[0] | (buf[1] << 8) );
}

static uint32_t get32le(unsigned char * buf)
{
	return (uint32_t)( buf[0] | (buf[1] << 8) | (buf[2] << 16) | ((uint32_t)buf[3] << 24) );
}

static int wrap(int idx, int size)
{
	idx %= size;
	if( idx < 0 )
		idx += size;

	return idx;
}

unsigned char getchunk(unsigned char c)
{
	int i;
	unsigned char r;

	r=0;
	for(i=0;i<4;i++)
	{
		if((c>>i)&1)
		{
			r=r | (0x3<<(i*2));
		}
	}

	return r;
}

////////////////////////////////////////////////////////////////////////////////
// Sector timing / fuzzy bits applied to a generated side.
// data_byte_index : index (in MFM bytes - 8 cells) of the first sector data cell
////////////////////////////////////////////////////////////////////////////////
static void apply_sector_fuzzy(HXCFE_SIDE * side, int data_byte_index, stx_sector * s)
{
	int k, buflen;

	if( !s->fuzzy_mask || !side->flakybitsbuffer )
		return;

	buflen = side->tracklen / 8;

	for( k = 0 ; k < s->data_size ; k++ )
	{
		side->flakybitsbuffer[wrap(data_byte_index + (k*2),     buflen)] |= getchunk( (unsigned char)( (~s->fuzzy_mask[k]) >> 4 ) );
		side->flakybitsbuffer[wrap(data_byte_index + (k*2) + 1, buflen)] |= getchunk( (unsigned char)( ~s->fuzzy_mask[k] ) );
	}
}

static void apply_sector_timing(HXCFE_SIDE * side, int data_byte_index, stx_sector * s, int default_bitrate)
{
	int k, buflen, block, nb_blocks;
	uint32_t bitrate;
	unsigned short val;
	double block_us;

	if( !side->timingbuffer || s->data_size <= 0 )
		return;

	buflen = side->tracklen / 8;
	nb_blocks = s->data_size / 16;

	for( k = 0 ; k < s->data_size ; k++ )
	{
		block = k / 16;

		if( s->timing || s->macrodos_timing )
		{
			if( s->timing )
				val = (unsigned short)( (s->timing[block*2] << 8) | s->timing[(block*2) + 1] );
			else
				val = macrodos_timing_table[ (block * 4) / nb_blocks ];

			// 1 unit = 32 FDC cycles @ 8MHz + 28 cycles per 16 bytes block.
			block_us = ( ((double)val * 32.0) + 28.0 ) / 8.0;
			if( block_us > 0 )
				bitrate = (uint32_t)( ( (16.0 * 8.0) / block_us ) * 1000000.0 );
			else
				bitrate = default_bitrate;
		}
		else
		{
			if( s->desc.sector_speed_timing )
				bitrate = (uint32_t)( ( (double)(s->data_size * 8) / (double)s->desc.sector_speed_timing ) * 1000000.0 );
			else
				bitrate = default_bitrate;
		}

		side->timingbuffer[wrap(data_byte_index + (k*2),     buflen)] = bitrate;
		side->timingbuffer[wrap(data_byte_index + (k*2) + 1, buflen)] = bitrate;
	}
}

////////////////////////////////////////////////////////////////////////////////
// Track image based track generation
////////////////////////////////////////////////////////////////////////////////

static void put_byte(unsigned char * track, unsigned char * clock, int size, int idx, unsigned char data, unsigned char clk)
{
	idx = wrap(idx, size);
	track[idx] = data;
	clock[idx] = clk;
}

static int id_pos_used(stx_sector * sectors, int nb, int pos)
{
	int i;

	for( i = 0 ; i < nb ; i++ )
	{
		if( sectors[i].id_pos == pos )
			return 1;
	}

	return 0;
}

// Search the ID field of a sector in the track image. Return the position of the $FE mark or -1.
// mode 0 : ID + CRC + sync, 1 : ID + CRC, 2 : ID, 3 : Any free "A1 A1 FE" sequence near the expected position.
static int find_id_field(unsigned char * img, int size, stx_sector * sectors, int cur, int mode)
{
	stx_sector * s;
	int max_dist, nb_check;
	int d, k, i, pos, expected;
	unsigned char id[6];

	s = &sectors[cur];

	id[0] = s->desc.track_num;
	id[1] = s->desc.side_num;
	id[2] = s->desc.sector_num;
	id[3] = s->desc.sector_size;
	id[4] = s->desc.header_crc & 0xFF;          // CRC high byte (as stored in the file)
	id[5] = (s->desc.header_crc >> 8) & 0xFF;   // CRC low byte

	// bitPosition : Position in bits just after the $FE mark.
	expected = ( s->desc.sector_pos_timing / 8 ) - 1;

	nb_check = 6;
	if( mode == 2 )
		nb_check = 4;
	if( mode == 3 )
		nb_check = 0;

	max_dist = size / 2;
	if( mode == 3 )
		max_dist = 48;

	for( d = 0 ; d <= max_dist ; d++ )
	{
		for( k = 0 ; k < 2 ; k++ )
		{
			if( !d && k )
				continue;

			pos = wrap( expected + (k ? -d : d), size );

			if( img[pos] != 0xFE )
				continue;

			if( ( mode == 0 || mode == 3 ) && ( img[wrap(pos - 1, size)] != 0xA1 || img[wrap(pos - 2, size)] != 0xA1 ) )
				continue;

			for( i = 0 ; i < nb_check ; i++ )
			{
				if( img[wrap(pos + 1 + i, size)] != id[i] )
					break;
			}

			if( i == nb_check && !id_pos_used(sectors, cur, pos) )
				return pos;
		}
	}

	return -1;
}

// Search the data mark after an ID field. Return the position of the first data byte or -1.
static int find_data_field(unsigned char * img, int size, int id_pos)
{
	int i, pos;

	for( i = 7 + 2 ; i < 7 + STX_DAM_SEARCH_WINDOW ; i++ )
	{
		pos = wrap(id_pos + i, size);

		if( img[pos] >= 0xF8 && img[pos] <= 0xFB &&
			img[wrap(pos - 1, size)] == 0xA1 &&
			img[wrap(pos - 2, size)] == 0xA1 )
		{
			return wrap(pos + 1, size);
		}
	}

	return -1;
}

static void crc16_buf(unsigned char * crc_h, unsigned char * crc_l, unsigned char * crctable, unsigned char * buf, int size, int start, int len)
{
	int i;

	for( i = 0 ; i < len ; i++ )
		CRC16_Update(crc_h, crc_l, buf[wrap(start + i, size)], crctable);
}

static HXCFE_SIDE * build_track_from_image(HXCFE* hxcfe, unsigned char * image, int image_size, stx_sector * sectors, int nb_sectors, int tracknum, int sidenum)
{
	HXCFE_SIDE * side;
	unsigned char * track;
	unsigned char * clock;
	int i, j, pos, mark;
	unsigned char crctable[32];
	unsigned char crc_h, crc_l;

	side = NULL;

	track = malloc(image_size);
	clock = malloc(image_size);
	if( !track || !clock )
		goto done;

	memcpy(track, image, image_size);
	memset(clock, 0xFF, image_size);

	// Locate the ID and data fields in the track image
	for( j = 0 ; j < nb_sectors ; j++ )
	{
		for( i = 0 ; i < 4 ; i++ )
		{
			sectors[j].id_pos = find_id_field(image, image_size, sectors, j, i);
			if( sectors[j].id_pos >= 0 )
				break;
		}

		if( i == 3 && sectors[j].id_pos >= 0 )
			hxcfe->hxc_printf(MSG_DEBUG,"STX : T%.2d S%d - Sector %d ID field garbled in the track image. Rebuilt at 0x%.4X",tracknum,sidenum,sectors[j].desc.sector_num,sectors[j].id_pos);

		if( sectors[j].id_pos < 0 )
		{
			// Not found (garbled by the WD1772 read track) : use the sector position.
			sectors[j].id_pos = wrap( (sectors[j].desc.sector_pos_timing / 8) - 1, image_size );
			hxcfe->hxc_printf(MSG_DEBUG,"STX : T%.2d S%d - Sector %d ID field not found in the track image ! Rebuilt at 0x%.4X",tracknum,sidenum,sectors[j].desc.sector_num,sectors[j].id_pos);
		}

		sectors[j].data_pos = -1;

		if( sectors[j].data )
		{
			sectors[j].data_pos = find_data_field(image, image_size, sectors[j].id_pos);
			if( sectors[j].data_pos < 0 )
			{
				// Standard gap 2 : 22 x 4E + 12 x 00 + A1 A1 A1 + mark
				sectors[j].data_pos = wrap( sectors[j].id_pos + 7 + 22 + 12 + 4, image_size );
				hxcfe->hxc_printf(MSG_DEBUG,"STX : T%.2d S%d - Sector %d data mark not found in the track image ! Rebuilt at 0x%.4X",tracknum,sidenum,sectors[j].desc.sector_num,sectors[j].data_pos);
			}
		}

		hxcfe->hxc_printf(MSG_DEBUG,"STX : T%.2d S%d - Sector %d (size %d) ID @ 0x%.4X, Data @ 0x%.4X",tracknum,sidenum,sectors[j].desc.sector_num,sectors[j].data_size,sectors[j].id_pos,sectors[j].data_pos);
	}

	// Index address marks (C2 C2 C2 FC) : restore the missing clocks.
	for( i = 0 ; i < image_size ; i++ )
	{
		if( image[i] == 0xC2 && image[wrap(i+1, image_size)] == 0xC2 && image[wrap(i+2, image_size)] == 0xC2 && image[wrap(i+3, image_size)] == 0xFC )
		{
			clock[i] = 0x14;
			clock[wrap(i+1, image_size)] = 0x14;
			clock[wrap(i+2, image_size)] = 0x14;
		}
	}

	if( !nb_sectors )
	{
		// No sector : Try to restore the sync marks ("xx A1 A1 FE/FB/F8" sequences).
		for( i = 0 ; i < image_size ; i++ )
		{
			if( image[wrap(i+1, image_size)] == 0xA1 && image[wrap(i+2, image_size)] == 0xA1 &&
				( image[wrap(i+3, image_size)] == 0xFE || ( image[wrap(i+3, image_size)] >= 0xF8 && image[wrap(i+3, image_size)] <= 0xFB ) ) )
			{
				for( j = 0 ; j < 3 ; j++ )
					put_byte(track, clock, image_size, i + j, 0xA1, 0x0A);
			}
		}
	}

	// First pass : Sectors data and data CRC
	for( j = 0 ; j < nb_sectors ; j++ )
	{
		if( !sectors[j].data || sectors[j].data_pos < 0 )
			continue;

		mark = track[wrap(sectors[j].data_pos - 1, image_size)];
		if( mark < 0xF8 || mark > 0xFB )
			mark = ( sectors[j].desc.FDC_status & PASTI_SECT_RECORD_TYPE ) ? 0xF8 : 0xFB;

		for( i = 0 ; i < sectors[j].data_size ; i++ )
			put_byte(track, clock, image_size, sectors[j].data_pos + i, sectors[j].data[i], 0xFF);

		CRC16_Init(&crc_h, &crc_l, (unsigned char*)crctable, 0x1021, 0xFFFF);
		CRC16_Update(&crc_h, &crc_l, 0xA1, crctable);
		CRC16_Update(&crc_h, &crc_l, 0xA1, crctable);
		CRC16_Update(&crc_h, &crc_l, 0xA1, crctable);
		CRC16_Update(&crc_h, &crc_l, (unsigned char)mark, crctable);
		crc16_buf(&crc_h, &crc_l, crctable, track, image_size, sectors[j].data_pos, sectors[j].data_size);

		pos = sectors[j].data_pos + sectors[j].data_size;

		if( !(sectors[j].desc.FDC_status & PASTI_SECT_CRC) )
		{
			put_byte(track, clock, image_size, pos,     crc_h, 0xFF);
			put_byte(track, clock, image_size, pos + 1, crc_l, 0xFF);
		}
		else
		{
			// Bad CRC expected : keep the track image value if it is wrong.
			if( track[wrap(pos, image_size)] == crc_h && track[wrap(pos + 1, image_size)] == crc_l )
				put_byte(track, clock, image_size, pos + 1, (unsigned char)(crc_l ^ 0xFF), 0xFF);
		}
	}

	// Second pass : ID fields and sync marks (after the data : sectors within sectors protections)
	for( j = 0 ; j < nb_sectors ; j++ )
	{
		pos = sectors[j].id_pos;

		for( i = 1 ; i <= 3 ; i++ )
			put_byte(track, clock, image_size, pos - i, 0xA1, 0x0A);

		put_byte(track, clock, image_size, pos,     0xFE, 0xFF);
		put_byte(track, clock, image_size, pos + 1, sectors[j].desc.track_num, 0xFF);
		put_byte(track, clock, image_size, pos + 2, sectors[j].desc.side_num, 0xFF);
		put_byte(track, clock, image_size, pos + 3, sectors[j].desc.sector_num, 0xFF);
		put_byte(track, clock, image_size, pos + 4, sectors[j].desc.sector_size, 0xFF);
		put_byte(track, clock, image_size, pos + 5, sectors[j].desc.header_crc & 0xFF, 0xFF);
		put_byte(track, clock, image_size, pos + 6, (sectors[j].desc.header_crc >> 8) & 0xFF, 0xFF);

		if( sectors[j].data && sectors[j].data_pos >= 0 )
		{
			pos = sectors[j].data_pos;

			mark = track[wrap(pos - 1, image_size)];
			if( mark < 0xF8 || mark > 0xFB )
				mark = ( sectors[j].desc.FDC_status & PASTI_SECT_RECORD_TYPE ) ? 0xF8 : 0xFB;

			for( i = 2 ; i <= 4 ; i++ )
				put_byte(track, clock, image_size, pos - i, 0xA1, 0x0A);

			put_byte(track, clock, image_size, pos - 1, (unsigned char)mark, 0xFF);
		}
	}

	side = tg_alloctrack(DEFAULT_DD_BITRATE, ISOIBM_MFM_ENCODING, 300, image_size * 2 * 8, 2500, 0, TG_ALLOCTRACK_ALLOCTIMIMGBUFFER | TG_ALLOCTRACK_ALLOCFLAKEYBUFFER);
	if( !side )
		goto done;

	BuildMFMCylinder(side->databuffer, side->tracklen / 8, clock, track, image_size);

	for( j = 0 ; j < nb_sectors ; j++ )
	{
		if( sectors[j].data && sectors[j].data_pos >= 0 )
		{
			apply_sector_fuzzy(side, sectors[j].data_pos * 2, &sectors[j]);
			apply_sector_timing(side, sectors[j].data_pos * 2, &sectors[j], DEFAULT_DD_BITRATE);
		}
	}

done:
	free(track);
	free(clock);

	return side;
}

////////////////////////////////////////////////////////////////////////////////
// Sector only based track generation
////////////////////////////////////////////////////////////////////////////////
static HXCFE_SIDE * build_track_from_sectors(HXCFE* hxcfe, stx_sector * sectors, int nb_sectors, int bitrate)
{
	HXCFE_SIDE * side;
	HXCFE_SECTCFG * sectorconfig;
	int j, trackformat;

	side = NULL;

	if( nb_sectors <= 0 )
		return NULL;

	sectorconfig = (HXCFE_SECTCFG *) malloc(sizeof(HXCFE_SECTCFG) * nb_sectors);
	if( !sectorconfig )
		return NULL;

	memset(sectorconfig, 0, sizeof(HXCFE_SECTCFG) * nb_sectors);

	trackformat = ISOFORMAT_DD;
	if( nb_sectors == 11 )
		trackformat = ISOFORMAT_DD11S;

	for( j = 0 ; j < nb_sectors ; j++ )
	{
		sectorconfig[j].head = sectors[j].desc.side_num;
		sectorconfig[j].cylinder = sectors[j].desc.track_num;
		sectorconfig[j].sector = sectors[j].desc.sector_num;
		sectorconfig[j].sectorsize = sectors[j].data_size;
		sectorconfig[j].use_alternate_sector_size_id = 1;
		sectorconfig[j].alternate_sector_size_id = sectors[j].desc.sector_size;
		if( sectors[j].keep_header_crc )
		{
			// The track generator expects the CRC high byte in bits 15-8.
			sectorconfig[j].use_alternate_header_crc = 0x2;
			sectorconfig[j].header_crc = ( (sectors[j].desc.header_crc & 0xFF) << 8 ) | ( (sectors[j].desc.header_crc >> 8) & 0xFF );
		}
		sectorconfig[j].gap3 = 255;
		sectorconfig[j].bitrate = bitrate;
		sectorconfig[j].trackencoding = trackformat;

		if( sectors[j].desc.FDC_status & PASTI_SECT_RECORD_TYPE ) // Deleted sector ?
		{
			sectorconfig[j].use_alternate_datamark = 0xFF;
			sectorconfig[j].alternate_datamark = 0xF8;
		}

		if( !sectors[j].data ) // Data block not found ?
		{
			sectorconfig[j].use_alternate_datamark = 0xFF;
			sectorconfig[j].alternate_datamark = 0x00;
			sectorconfig[j].flags |= TRACKGEN_NO_DATA;
		}
		else
		{
			if( sectors[j].desc.FDC_status & PASTI_SECT_CRC ) // Bad Data CRC ?
				sectorconfig[j].use_alternate_data_crc = 0x01;

			sectorconfig[j].input_data = malloc(sectors[j].data_size);
			if( sectorconfig[j].input_data )
				memcpy(sectorconfig[j].input_data, sectors[j].data, sectors[j].data_size);
		}
	}

	side = tg_generateTrackEx(nb_sectors, sectorconfig, 1, 0, bitrate, 300, trackformat, 0, 2500, 0);
	if( side )
	{
		if( !side->flakybitsbuffer )
			side->flakybitsbuffer = tg_allocsubtrack_char(side->tracklen, 0x00);

		if( !side->timingbuffer )
			side->timingbuffer = tg_allocsubtrack_long(side->tracklen, bitrate);

		side->bitrate = VARIABLEBITRATE;

		for( j = 0 ; j < nb_sectors ; j++ )
		{
			if( sectors[j].data )
			{
				apply_sector_fuzzy(side, sectorconfig[j].startdataindex, &sectors[j]);
				apply_sector_timing(side, sectorconfig[j].startdataindex, &sectors[j], bitrate);
			}
		}
	}

	for( j = 0 ; j < nb_sectors ; j++ )
		hxcfe_freeSectorConfigData( 0, &sectorconfig[j] );

	free(sectorconfig);

	return side;
}

////////////////////////////////////////////////////////////////////////////////
// Track record parser
////////////////////////////////////////////////////////////////////////////////
static HXCFE_SIDE * load_track_record(HXCFE* hxcfe, unsigned char * rec, uint32_t rec_size, int revision, int bitrate)
{
	HXCFE_SIDE * side;
	stx_sector * sectors;
	unsigned short flags, nb_sectors, image_size, sync_offset;
	uint32_t fuzzy_size, fuzzy_offset, trackdata_offset, sectors_image_offset, max_sector_end, timing_offset, offset;
	unsigned char * image;
	int j, tracknum, sidenum, variable_timing;
	unsigned char * timing_ptr;
	unsigned char * timing_end;
	unsigned char * ptr;
#ifdef PASTI_DBG
	char tempstring[512];
	int debug_i;
#endif

	side = NULL;
	sectors = NULL;
	image = NULL;
	image_size = 0;
	sync_offset = 0;

	fuzzy_size = get32le(&rec[4]);
	nb_sectors = get16le(&rec[8]);
	flags      = get16le(&rec[10]);
	tracknum   = rec[14] & 0x7F;
	sidenum    = rec[14] >> 7;

	hxcfe->hxc_printf(MSG_DEBUG,"STX : Track %.2d Side %d - Record size : %d, Sectors : %d, Flags : 0x%.4X, Track length : %d, Fuzzy size : %d, Type : 0x%.2X",
							tracknum, sidenum, rec_size, nb_sectors, flags, get16le(&rec[12]), fuzzy_size, rec[15]);

	if( nb_sectors )
	{
		sectors = malloc(sizeof(stx_sector) * nb_sectors);
		if( !sectors )
			return NULL;

		memset(sectors, 0, sizeof(stx_sector) * nb_sectors);
	}

	if( !(flags & PASTI_TRK_SECT) )
	{
		// Standard 512 bytes sectors, numbered from 1 to n, without sector descriptor.
		for( j = 0 ; j < nb_sectors ; j++ )
		{
			sectors[j].desc.track_num = tracknum;
			sectors[j].desc.side_num = sidenum;
			sectors[j].desc.sector_num = j + 1;
			sectors[j].desc.sector_size = 2;
			sectors[j].data_size = 512;

			offset = sizeof(pasti_trackheader) + (512 * j);
			if( offset + 512 <= rec_size )
				sectors[j].data = &rec[offset];
		}

		// Header CRC computed by the generator (keep_header_crc = 0)
		side = build_track_from_sectors(hxcfe, sectors, nb_sectors, bitrate);

		free(sectors);

		return side;
	}

	fuzzy_offset = sizeof(pasti_trackheader) + (sizeof(pasti_sector) * nb_sectors);
	trackdata_offset = fuzzy_offset + fuzzy_size;

	if( trackdata_offset > rec_size )
	{
		hxcfe->hxc_printf(MSG_ERROR,"STX : Track %.2d Side %d - Bad track record !",tracknum,sidenum);
		free(sectors);
		return NULL;
	}

	// Track image
	sectors_image_offset = trackdata_offset;
	if( flags & PASTI_TRK_IMAGE )
	{
		offset = trackdata_offset;
		if( flags & PASTI_TRK_SYNC )
		{
			if( offset + 4 <= rec_size )
			{
				sync_offset = get16le(&rec[offset]);
				image_size = get16le(&rec[offset + 2]);
			}
			offset += 4;
		}
		else
		{
			if( offset + 2 <= rec_size )
				image_size = get16le(&rec[offset]);
			offset += 2;
		}

		if( offset + image_size > rec_size )
		{
			hxcfe->hxc_printf(MSG_WARNING,"STX : Track %.2d Side %d - Truncated track image (%d bytes) !",tracknum,sidenum,image_size);
			image_size = ( offset < rec_size ) ? (unsigned short)(rec_size - offset) : 0;
		}

		image = &rec[offset];
		sectors_image_offset = offset + image_size;

		hxcfe->hxc_printf(MSG_DEBUG,"STX : Track image : %d bytes, first sync offset : 0x%.4X",image_size,sync_offset);
	}

	// Sector descriptors
	ptr = &rec[sizeof(pasti_trackheader)];
	fuzzy_offset = sizeof(pasti_trackheader) + (sizeof(pasti_sector) * nb_sectors);
	max_sector_end = 0;
	variable_timing = 0;

	for( j = 0 ; j < nb_sectors ; j++ )
	{
		sectors[j].desc.sector_pos          = get32le(&ptr[0]);
		sectors[j].desc.sector_pos_timing   = get16le(&ptr[4]);
		sectors[j].desc.sector_speed_timing = get16le(&ptr[6]);
		sectors[j].desc.track_num   = ptr[8];
		sectors[j].desc.side_num    = ptr[9];
		sectors[j].desc.sector_num  = ptr[10];
		sectors[j].desc.sector_size = ptr[11];
		sectors[j].desc.header_crc  = (uint16_t)( ptr[12] | (ptr[13] << 8) ); // Stored high byte first.
		sectors[j].desc.FDC_status  = ptr[14];
		sectors[j].desc.sector_flags= ptr[15];
		sectors[j].keep_header_crc = 1;
		ptr += sizeof(pasti_sector);

		// The WD1772 only uses 2 bits of the size ID field.
		sectors[j].data_size = 128 << (sectors[j].desc.sector_size & 3);

		if( !(sectors[j].desc.FDC_status & PASTI_SECT_RNF) )
		{
			offset = trackdata_offset + sectors[j].desc.sector_pos;
			if( offset + sectors[j].data_size <= rec_size )
			{
				sectors[j].data = &rec[offset];

				if( max_sector_end < sectors[j].desc.sector_pos + sectors[j].data_size )
					max_sector_end = sectors[j].desc.sector_pos + sectors[j].data_size;
			}
			else
			{
				hxcfe->hxc_printf(MSG_WARNING,"STX : Track %.2d Side %d - Sector %d data outside the track record !",tracknum,sidenum,sectors[j].desc.sector_num);
			}

			if( sectors[j].desc.FDC_status & PASTI_SECT_FUZZY )
			{
				if( fuzzy_offset + sectors[j].data_size <= trackdata_offset )
				{
					sectors[j].fuzzy_mask = &rec[fuzzy_offset];
					fuzzy_offset += sectors[j].data_size;
				}
			}

			if( sectors[j].desc.FDC_status & PASTI_SECT_VARIABLE_TIME )
				variable_timing = 1;
		}

#ifdef PASTI_DBG
		tempstring[0] = 0;
		for( debug_i = 0 ; debug_i < (int)sizeof(pasti_sector) ; debug_i++ )
			sprintf(&tempstring[strlen(tempstring)],"%.2X ",ptr[debug_i - (int)sizeof(pasti_sector)]);

		hxcfe->hxc_printf(MSG_DEBUG,"STX : Sector %.2d - ID: T%.3d H%d S%.3d N%d CRC:0x%.4X FDC:0x%.2X Data offset:0x%.5X Bit position:%d Read time:%d (%s)",
							j, sectors[j].desc.track_num, sectors[j].desc.side_num, sectors[j].desc.sector_num, sectors[j].desc.sector_size,
							(sectors[j].desc.header_crc>>8) | ((sectors[j].desc.header_crc&0xFF)<<8),
							sectors[j].desc.FDC_status, sectors[j].desc.sector_pos, sectors[j].desc.sector_pos_timing, sectors[j].desc.sector_speed_timing,
							tempstring);
#endif
	}

	// Timing record : Just after the sectors images (or after the track image if all sectors are inside the track image).
	if( variable_timing )
	{
		timing_offset = trackdata_offset + max_sector_end;
		if( timing_offset < sectors_image_offset )
			timing_offset = sectors_image_offset;

		timing_ptr = NULL;
		timing_end = NULL;

		if( revision == PASTI_FILE_REVISION_NEW )
		{
			if( timing_offset + sizeof(pasti_timingheader) <= rec_size )
			{
				hxcfe->hxc_printf(MSG_DEBUG,"STX : Timing record - flags : 0x%.4X, size : %d",get16le(&rec[timing_offset]),get16le(&rec[timing_offset+2]));

				timing_ptr = &rec[timing_offset + sizeof(pasti_timingheader)];
				timing_end = &rec[rec_size];
				if( timing_offset + get16le(&rec[timing_offset + 2]) <= rec_size )
					timing_end = &rec[timing_offset + get16le(&rec[timing_offset + 2])];
			}
		}

		for( j = 0 ; j < nb_sectors ; j++ )
		{
			if( sectors[j].data && (sectors[j].desc.FDC_status & PASTI_SECT_VARIABLE_TIME) )
			{
				if( revision == PASTI_FILE_REVISION_NEW )
				{
					if( timing_ptr && timing_ptr + ((sectors[j].data_size / 16) * 2) <= timing_end )
					{
						sectors[j].timing = timing_ptr;
						timing_ptr += (sectors[j].data_size / 16) * 2;
					}
				}
				else
				{
					sectors[j].macrodos_timing = 1;
				}
			}
		}
	}

	if( image && image_size >= 64 )
	{
		side = build_track_from_image(hxcfe, image, image_size, sectors, nb_sectors, tracknum, sidenum);
	}
	else
	{
		side = build_track_from_sectors(hxcfe, sectors, nb_sectors, bitrate);
	}

	free(sectors);

	return side;
}

int STX_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	FILE * f;
	int i, filesize;
	int tracknumber, sidenumber;
	int numberoftrack, numberoftrackperside, numberofside;
	int revision;
	unsigned char fileheader[sizeof(pasti_fileheader)];
	unsigned char trackheader[sizeof(pasti_trackheader)];
	unsigned char * record;
	uint32_t record_size;
	int presenceside[2];
	long trackheaderpos;
	HXCFE_SIDE * side;
	HXCFE_CYLINDER* currentcylinder;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"STX_libLoad_DiskFile %s",imgfile);

	f = hxc_fopen(imgfile,"rb");
	if( f == NULL )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	filesize = hxc_fgetsize(f);

	memset(fileheader, 0, sizeof(fileheader));
	hxc_fread( fileheader, sizeof(fileheader), f );

	if( memcmp(fileheader, "RSY\0", 4) )
	{
		hxc_fclose(f);
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"non STX/pasti image (bad header)");
		return HXCFE_BADFILE;
	}

	if( get16le(&fileheader[4]) != PASTI_FILE_VERSION )
		imgldr_ctx->hxcfe->hxc_printf(MSG_WARNING,"STX : Unsupported file version (%d) !",get16le(&fileheader[4]));

	numberoftrack = fileheader[10];
	revision = fileheader[11];

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"STX : Version : %d, Tool : 0x%.4X, Track records : %d, Revision : 0x%.2X",
						get16le(&fileheader[4]), get16le(&fileheader[6]), numberoftrack, revision);

	// First pass : Count the tracks and sides
	presenceside[0] = 0;
	presenceside[1] = 0;
	numberoftrackperside = 0;

	for( i = 0 ; i < numberoftrack ; i++ )
	{
		trackheaderpos = ftell(f);
		if( trackheaderpos + (long)sizeof(trackheader) > filesize )
			break;

		hxc_fread( trackheader, sizeof(trackheader), f );

		record_size = get32le(&trackheader[0]);

		presenceside[trackheader[14] >> 7] = 1;
		if( (trackheader[14] & 0x7F) + 1 > numberoftrackperside )
			numberoftrackperside = (trackheader[14] & 0x7F) + 1;

		if( record_size < sizeof(trackheader) )
			break;

		fseek(f, trackheaderpos + record_size, SEEK_SET);
	}

	numberofside = ( presenceside[0] && presenceside[1] ) ? 2 : 1;

	if( !numberoftrackperside )
	{
		hxc_fclose(f);
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"STX : No track found !");
		return HXCFE_BADFILE;
	}

	floppydisk->floppySectorPerTrack = -1;
	floppydisk->floppyNumberOfSide = numberofside;
	floppydisk->floppyNumberOfTrack = numberoftrackperside;
	floppydisk->floppyiftype = ATARIST_DD_FLOPPYMODE;
	floppydisk->floppyBitRate = DEFAULT_DD_BITRATE;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"STX : %d track(s) per side, %d side(s)",numberoftrackperside,numberofside);

	floppydisk->tracks = (HXCFE_CYLINDER**)malloc(sizeof(HXCFE_CYLINDER*) * floppydisk->floppyNumberOfTrack);
	if( !floppydisk->tracks )
	{
		hxc_fclose(f);
		return HXCFE_INTERNALERROR;
	}

	memset(floppydisk->tracks, 0, sizeof(HXCFE_CYLINDER*) * floppydisk->floppyNumberOfTrack);

	// Second pass : Load the tracks
	fseek(f, sizeof(fileheader), SEEK_SET);

	for( i = 0 ; i < numberoftrack ; i++ )
	{
		hxcfe_imgCallProgressCallback(imgldr_ctx, i, numberoftrack);

		trackheaderpos = ftell(f);
		if( trackheaderpos + (long)sizeof(trackheader) > filesize )
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_WARNING,"STX : Truncated file (%d/%d tracks records) !",i,numberoftrack);
			break;
		}

		hxc_fread( trackheader, sizeof(trackheader), f );

		record_size = get32le(&trackheader[0]);
		if( record_size < sizeof(trackheader) )
			break;

		if( trackheaderpos + (long)record_size > filesize )
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_WARNING,"STX : Truncated track record !");
			record_size = filesize - trackheaderpos;
		}

		tracknumber = trackheader[14] & 0x7F;
		sidenumber = trackheader[14] >> 7;

		record = malloc(record_size);
		if( !record )
			goto alloc_error;

		fseek(f, trackheaderpos, SEEK_SET);
		hxc_fread( record, record_size, f );

		side = load_track_record(imgldr_ctx->hxcfe, record, record_size, revision, floppydisk->floppyBitRate);

		free(record);

		if( side )
		{
			if( !floppydisk->tracks[tracknumber] )
			{
				floppydisk->tracks[tracknumber] = allocCylinderEntry(300, numberofside);
				if( !floppydisk->tracks[tracknumber] )
				{
					hxcfe_freeSide(imgldr_ctx->hxcfe, side);
					goto alloc_error;
				}
			}

			currentcylinder = floppydisk->tracks[tracknumber];

			if( currentcylinder->sides[sidenumber] )
				hxcfe_freeSide(imgldr_ctx->hxcfe, currentcylinder->sides[sidenumber]);

			currentcylinder->sides[sidenumber] = side;
		}

		fseek(f, trackheaderpos + record_size, SEEK_SET);
	}

	hxc_fclose(f);

	// Unformatted / missing tracks
	for( i = 0 ; i < floppydisk->floppyNumberOfTrack ; i++ )
	{
		if( !floppydisk->tracks[i] )
		{
			floppydisk->tracks[i] = allocCylinderEntry(300, numberofside);
			if( !floppydisk->tracks[i] )
				goto alloc_error_noclose;
		}

		for( sidenumber = 0 ; sidenumber < numberofside ; sidenumber++ )
		{
			if( !floppydisk->tracks[i]->sides[sidenumber] )
			{
				floppydisk->tracks[i]->sides[sidenumber] = tg_generateTrack(0,512,0 ,(unsigned char)i,(unsigned char)sidenumber,1,1,(unsigned char)(0),DEFAULT_DD_BITRATE,300,ISOFORMAT_DD,255,0,2500| NO_SECTOR_UNDER_INDEX,0);
			}
		}
	}

	hxcfe_sanityCheck(imgldr_ctx->hxcfe,floppydisk);

	imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_0,"Pasti image loading done!");

	return HXCFE_NOERROR;

alloc_error:
	hxc_fclose(f);

alloc_error_noclose:
	hxcfe_freeFloppy(imgldr_ctx->hxcfe, floppydisk );

	return HXCFE_INTERNALERROR;
}

int STX_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]="ATARIST_STX";
	static const char plug_desc[]="Atari ST STX/Pasti Loader";
	static const char plug_ext[]="stx";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   STX_libIsValidDiskFile,
		(LOADDISKFILE)      STX_libLoad_DiskFile,
		(WRITEDISKFILE)     STX_libWrite_DiskFile,
		(GETPLUGININFOS)    STX_libGetPluginInfo
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
