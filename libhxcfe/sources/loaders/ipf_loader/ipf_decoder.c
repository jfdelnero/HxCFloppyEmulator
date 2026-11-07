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
// File : ipf_decoder.c
// Contains: IPF floppy image loader - Built-in decoder (no SPS CAPS library needed)
//
// Written by: Jean-François DEL NERO
// Assisted-by: Claude Opus 5.5
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////
//
// IPF file format documentation used :
// IPF - Interchangeable Preservation Format Documentation v1.6 (Jean Louis-Guerin)
// https://www.kryoflux.com/download/ipf_documentation_v1.6.pdf
//
// The track generation behaviour (gap streams sizing, write splice, density maps)
// follows the SPS decoder output so that both loaders give the same tracks.
//

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "types.h"

#include "internal_libhxcfe.h"
#include "libhxcfe.h"

#include "floppy_loader.h"
#include "floppy_utils.h"

#include "ipf_loader.h"
#include "ipf_format.h"
#include "ipf_decoder.h"

#include "libhxcadaptor.h"

#include "../../tracks/std_crc32.h"

#define IPFD_INDEX_POSITION       0
#define IPFD_INDEX_DURATION       2000

#define IPFD_RPM                  300
#define IPFD_UNFORMATTED_SIZE     (12500*8)

#define IPFD_MAX_CYLINDERS        256
#define IPFD_MAX_SIDES            2

#define IPFD_OVERLAP_FLIP_BITS    3       // Bits inverted at the write splice position

// Record types
#define IPFD_REC_CAPS             0x43415053 // "CAPS"
#define IPFD_REC_INFO             0x494E464F // "INFO"
#define IPFD_REC_IMGE             0x494D4745 // "IMGE"
#define IPFD_REC_DATA             0x44415441 // "DATA"

// Encoders
#define IPFD_ENCODER_CAPS         1
#define IPFD_ENCODER_SPS          2

// Density types
#define IPFD_DEN_NOISE            1
#define IPFD_DEN_AUTO             2
#define IPFD_DEN_COPYLOCK_AMIGA   3
#define IPFD_DEN_COPYLOCK_AMIGA2  4
#define IPFD_DEN_COPYLOCK_ST      5
#define IPFD_DEN_SPEEDLOCK_AMIGA  6
#define IPFD_DEN_SPEEDLOCK_AMIGA2 7
#define IPFD_DEN_ABRIERLEY_AMIGA  8
#define IPFD_DEN_ABRIERLEY_AMIGA2 9
#define IPFD_DEN_LAST             10

// Data stream elements
#define IPFD_DAT_END              0
#define IPFD_DAT_MARK             1
#define IPFD_DAT_DATA             2
#define IPFD_DAT_GAP              3
#define IPFD_DAT_RAW              4
#define IPFD_DAT_FUZZY            5

// Gap stream elements
#define IPFD_GAP_END              0
#define IPFD_GAP_COUNT            1
#define IPFD_GAP_DATA             2

// First cell type written in a block
#define IPFD_FIRST_NONE           0
#define IPFD_FIRST_MFM            1
#define IPFD_FIRST_RAW            2
#define IPFD_FIRST_WEAK           3

// Gap stream loop type
#define IPFD_LOOP_NONE            0
#define IPFD_LOOP_AUTO            1    // All samples have a size : the loop sample can be repeated to grow the gap
#define IPFD_LOOP_STREAM          2    // One sample without size : repeated as needed

// Track decoding results
#define IPFD_TRK_OK               0
#define IPFD_TRK_UNFORMATTED      1
#define IPFD_TRK_ERROR            -1

typedef struct ipfd_block_
{
	uint32_t databits;
	uint32_t gapbits;
	uint32_t gapoffset;
	uint32_t celltype;
	uint32_t enctype;
	uint32_t flags;
	uint32_t gapvalue;
	uint32_t dataoffset;
}ipfd_block;

typedef struct ipfd_imge_
{
	uint32_t cyl;
	uint32_t head;
	uint32_t density;
	uint32_t signal;
	uint32_t startbit;
	uint32_t blkcnt;
	uint32_t process;
	uint32_t flags;
	uint32_t datakey;
}ipfd_imge;

typedef struct ipfd_trackentry_
{
	int        imge_valid;
	ipfd_imge  imge;
	uint32_t   data_offset;   // offset of the DATA extra block in the file
	uint32_t   data_size;     // DATA extra block size
	uint32_t   data_crc;
	int        data_valid;
}ipfd_trackentry;

typedef struct ipfd_gapsample_
{
	const uint8_t * data;
	uint32_t bits;            // sample size in bits
	uint32_t count;           // number of bits to generate (0 : undefined)
	uint64_t outbits;         // final number of bits generated
}ipfd_gapsample;

typedef struct ipfd_gapstream_
{
	int              nbsamples;
	ipfd_gapsample * samples;
	int              loopsample;
	int              looptype;
	uint64_t         fixcells;   // encoded size without loop
	uint64_t         loopcells;  // encoded size of the loop sample
	uint8_t          defaultsample;
}ipfd_gapstream;

typedef struct ipfd_ctx_
{
	HXCFE *         hxcfe;

	const uint8_t * ed;          // DATA extra block
	uint32_t        edsize;

	ipfd_block *    blocks;
	uint32_t        nbblocks;
	uint32_t        encoder;

	uint8_t *       cells;
	uint8_t *       weak;
	uint32_t        trackbits;

	uint32_t        pos;         // current write position
	int             blkfirst;    // nothing written yet in the current block
	int             firsttype;   // type of the first cell written in the current block
	int             weakfound;

	int             splitvalid;  // gap split (write splice) position of the current block
	uint32_t        splitpos;
}ipfd_ctx;

static uint32_t ipfd_be32(const uint8_t * p)
{
	return ((uint32_t)p[0]<<24) | ((uint32_t)p[1]<<16) | ((uint32_t)p[2]<<8) | (uint32_t)p[3];
}

static int ipfd_getbit(const uint8_t * buf, uint32_t bit)
{
	return (buf[bit>>3] >> (7 - (bit & 7))) & 1;
}

static void ipfd_setbit(uint8_t * buf, uint32_t bit, int val)
{
	if(val)
		buf[bit>>3] |= (0x80 >> (bit & 7));
	else
		buf[bit>>3] &= ~(0x80 >> (bit & 7));
}

///////////////////////////////////////////////////////////////////////////////
// Cells writer
///////////////////////////////////////////////////////////////////////////////

static void ipfd_put(ipfd_ctx * c, int val, int weak, int type)
{
	if(c->blkfirst)
	{
		c->firsttype = type;
		c->blkfirst = 0;
	}

	ipfd_setbit(c->cells, c->pos, val);
	ipfd_setbit(c->weak, c->pos, weak);

	if(weak)
		c->weakfound = 1;

	c->pos++;
	if(c->pos >= c->trackbits)
		c->pos = 0;
}

// Previous cell value. At the block start, the cell before the block is not
// known yet (track wrap) : 0 is used and the block start is fixed afterward.
static int ipfd_prevcell(ipfd_ctx * c)
{
	if(c->blkfirst)
		return 0;

	return ipfd_getbit(c->cells, c->pos ? c->pos - 1 : c->trackbits - 1);
}

static void ipfd_put_mfm(ipfd_ctx * c, int databit)
{
	ipfd_put(c, !ipfd_prevcell(c) && !databit, 0, IPFD_FIRST_MFM);
	ipfd_put(c, databit, 0, IPFD_FIRST_MFM);
}

///////////////////////////////////////////////////////////////////////////////
// Stream element header : size width (bits 7-5) | type (bits 4-0), big endian size
///////////////////////////////////////////////////////////////////////////////

static int ipfd_read_element(ipfd_ctx * c, uint32_t * off, int * type, uint32_t * count)
{
	int width, i;
	uint8_t head;

	if(*off >= c->edsize)
		return -1;

	head = c->ed[(*off)++];
	width = head >> IPF_STREAM_SIZE_WIDTH_SHIFT;
	*type = head & IPF_STREAM_TYPE_MASK;

	if(width > 4 || *off + width > c->edsize)
		return -1;

	*count = 0;
	for(i = 0; i < width; i++)
		*count = (*count << 8) | c->ed[(*off)++];

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Block data stream
///////////////////////////////////////////////////////////////////////////////

static int ipfd_data_stream(ipfd_ctx * c, ipfd_block * b, uint32_t size)
{
	uint32_t off, written, count, bits, bytes, i;
	int type, dmb;

	if(!size)
		return 0;

	off = b->dataoffset;
	if(off >= c->edsize)
		return -1;

	dmb = (c->encoder == IPFD_ENCODER_SPS) && (b->flags & IPF_BLOCK_FLAG_DATA_IN_BITS);

	written = 0;
	while(written < size)
	{
		if(ipfd_read_element(c, &off, &type, &count))
			return -1;

		if(b->enctype == IPF_BLOCK_ENC_RAW)
		{
			// Raw block : raw elements only, size always in bytes.
			if(type != IPFD_DAT_RAW || !count || count > c->edsize - off)
				return -1;

			bits = count * 8;
			for(i = 0; i < bits && written < size; i++, written++)
				ipfd_put(c, ipfd_getbit(&c->ed[off], i), 0, IPFD_FIRST_RAW);

			off += count;
			continue;
		}

		if(dmb)
		{
			bits = count;
			bytes = (bits + 7) / 8;
		}
		else
		{
			if(count > 0x1FFFFFFF)
				return -1;

			bits = count * 8;
			bytes = count;
		}

		switch(type)
		{
			case IPFD_DAT_MARK:   // Raw cells
				if(!bits || bytes > c->edsize - off)
					return -1;

				for(i = 0; i < bits && written < size; i++, written++)
					ipfd_put(c, ipfd_getbit(&c->ed[off], i), 0, IPFD_FIRST_RAW);

				off += bytes;
			break;

			case IPFD_DAT_DATA:   // MFM encoded data bits
			case IPFD_DAT_GAP:
				if(!bits || bytes > c->edsize - off)
					return -1;

				for(i = 0; i < bits && written < size; i++)
				{
					if(size - written >= 2)
					{
						ipfd_put_mfm(c, ipfd_getbit(&c->ed[off], i));
						written += 2;
					}
					else
					{
						// Only the clock cell fits
						ipfd_put(c, !ipfd_prevcell(c) && !ipfd_getbit(&c->ed[off], i), 0, IPFD_FIRST_MFM);
						written++;
					}
				}

				off += bytes;
			break;

			case IPFD_DAT_FUZZY:  // Weak bits : no sample stored, 2 cells per data bit
				for(i = 0; i < bits * 2 && written < size; i++, written++)
					ipfd_put(c, 0, 1, IPFD_FIRST_WEAK);
			break;

			default:              // End of stream before the end of the block or invalid element
				return -1;
		}
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Gap streams
///////////////////////////////////////////////////////////////////////////////

static void ipfd_free_gapstream(ipfd_gapstream * g)
{
	free(g->samples);
	g->samples = NULL;
	g->nbsamples = 0;
}

static int ipfd_add_gapsample(ipfd_gapstream * g, const uint8_t * data, uint32_t bits, uint32_t count)
{
	ipfd_gapsample * tmp;

	if(!(g->nbsamples & 15))
	{
		tmp = realloc(g->samples, sizeof(ipfd_gapsample) * (g->nbsamples + 16));
		if(!tmp)
			return -1;

		g->samples = tmp;
	}

	g->samples[g->nbsamples].data = data;
	g->samples[g->nbsamples].bits = bits;
	g->samples[g->nbsamples].count = count;
	g->samples[g->nbsamples].outbits = count ? count : bits;
	g->nbsamples++;

	return 0;
}

// Parse a gap elements list. *off is moved after the list end element.
static int ipfd_parse_gaplist(ipfd_ctx * c, uint32_t * off, ipfd_gapstream * g)
{
	int type;
	uint32_t count, pendingcount;

	pendingcount = 0;
	for(;;)
	{
		if(ipfd_read_element(c, off, &type, &count))
			return -1;

		switch(type)
		{
			case IPFD_GAP_END:
				if(count)
					return -1;
				return 0;

			case IPFD_GAP_COUNT:  // Number of bits to generate with the next sample
				pendingcount = count;
			break;

			case IPFD_GAP_DATA:   // Sample, size always in bits
				if(!count || (count + 7) / 8 > c->edsize - *off)
					return -1;

				if(g && ipfd_add_gapsample(g, &c->ed[*off], count, pendingcount))
					return -1;

				*off += (count + 7) / 8;
				pendingcount = 0;
			break;

			default:
				return -1;
		}
	}
}

// Find the loop sample and compute the stream size
static int ipfd_gapstream_sizes(ipfd_gapstream * g, int backward, int cellsperbit)
{
	int i, loopfound;

	g->loopsample = -1;
	g->looptype = IPFD_LOOP_NONE;
	g->fixcells = 0;
	g->loopcells = 0;

	if(!g->nbsamples)
		return 0;

	loopfound = 0;
	for(i = 0; i < g->nbsamples; i++)
	{
		if(!backward)
		{
			// Forward stream : the loop sample is the last one, nothing allowed after a sample without size.
			if(loopfound)
				return -1;

			g->loopsample = i;
		}
		else
		{
			// Backward stream : the loop sample is the first one, all other samples must have a size.
			if(!i)
				g->loopsample = i;
			else
				if(!g->samples[i].count)
					return -1;
		}

		if(!g->samples[i].count)
		{
			if(loopfound)
				return -1;

			loopfound = 1;
		}

		g->fixcells += (uint64_t)g->samples[i].outbits * cellsperbit;
	}

	g->loopcells = (uint64_t)g->samples[g->loopsample].bits * cellsperbit;
	g->looptype = loopfound ? IPFD_LOOP_STREAM : IPFD_LOOP_AUTO;

	return 0;
}

// Data bits iterator over the gap stream samples
typedef struct ipfd_gapiter_
{
	ipfd_gapstream * g;
	int      sample;
	uint64_t rem;      // remaining bits in the current sample output
	uint32_t phase;    // bit position in the sample data
}ipfd_gapiter;

static void ipfd_gapiter_seek(ipfd_gapiter * it, ipfd_gapstream * g, uint64_t bit)
{
	it->g = g;
	it->sample = 0;

	while(it->sample < g->nbsamples && bit >= g->samples[it->sample].outbits)
	{
		bit -= g->samples[it->sample].outbits;
		it->sample++;
	}

	if(it->sample < g->nbsamples)
	{
		it->rem = g->samples[it->sample].outbits - bit;
		it->phase = (uint32_t)(bit % g->samples[it->sample].bits);
	}
	else
	{
		it->rem = 0;
		it->phase = 0;
	}
}

static int ipfd_gapiter_next(ipfd_gapiter * it)
{
	int val;
	ipfd_gapsample * s;

	while(it->sample < it->g->nbsamples && !it->rem)
	{
		it->sample++;
		it->phase = 0;
		if(it->sample < it->g->nbsamples)
			it->rem = it->g->samples[it->sample].outbits;
	}

	if(it->sample >= it->g->nbsamples)
		return 0;

	s = &it->g->samples[it->sample];
	val = ipfd_getbit(s->data, it->phase);

	it->phase++;
	if(it->phase >= s->bits)
		it->phase = 0;

	it->rem--;

	return val;
}

// Write one gap stream (size cells). The forward stream is written from its start,
// the backward stream is aligned on the next block (its first cells are skipped).
static int ipfd_gap_stream(ipfd_ctx * c, ipfd_gapstream * g, uint32_t size, int backward, int mfm)
{
	uint64_t miss, scnt, smod, skip, total;
	uint64_t j0;
	ipfd_gapiter it;
	int lv, prevd, d;
	uint32_t remaining;
	int i;

	if(!size)
		return 0;

	skip = 0;

	for(i = 0; i < g->nbsamples; i++)
		g->samples[i].outbits = g->samples[i].count ? g->samples[i].count : g->samples[i].bits;

	if(g->fixcells >= size)
	{
		if(backward)
			skip = g->fixcells - size;
	}
	else
	{
		// The loop sample is repeated to fill the gap.
		if(!g->loopcells)
			return -1;

		miss = size - g->fixcells;
		scnt = miss / g->loopcells;
		smod = miss % g->loopcells;
		if(smod)
		{
			scnt++;
			if(backward)
				skip = g->loopcells - smod;
		}

		g->samples[g->loopsample].outbits += scnt * g->samples[g->loopsample].bits;
	}

	total = 0;
	for(i = 0; i < g->nbsamples; i++)
		total += g->samples[i].outbits * (mfm ? 2 : 1);

	if(total < skip + size)
		return -1;

	remaining = size;

	if(!mfm)
	{
		ipfd_gapiter_seek(&it, g, skip);
		while(remaining--)
			ipfd_put(c, ipfd_gapiter_next(&it), 0, IPFD_FIRST_RAW);

		return 0;
	}

	lv = ipfd_prevcell(c);
	j0 = skip >> 1;

	if(skip & 1)
	{
		// Start on a data cell
		ipfd_gapiter_seek(&it, g, j0);
		d = ipfd_gapiter_next(&it);
		ipfd_put(c, d, 0, IPFD_FIRST_MFM);
		remaining--;
		prevd = d;
	}
	else
	{
		// Start on a clock cell : the skipped data bits are dropped before the
		// encoding, the first clock uses the cell before the gap.
		ipfd_gapiter_seek(&it, g, j0);
		prevd = lv;
	}

	while(remaining)
	{
		d = ipfd_gapiter_next(&it);

		ipfd_put(c, !prevd && !d, 0, IPFD_FIRST_MFM);
		remaining--;

		if(remaining)
		{
			ipfd_put(c, d, 0, IPFD_FIRST_MFM);
			remaining--;
		}

		prevd = d;
	}

	return 0;
}

static int ipfd_block_gap(ipfd_ctx * c, uint32_t blkidx, uint32_t gapsize)
{
	ipfd_block * b;
	ipfd_gapstream gs[2];
	uint32_t off;
	uint64_t gs0, gs1, fs, rem, rem0, rem1, miss, miss0, miss1, sbp;
	int se0, se1, tl0, tl1, mfm, cellsperbit, ret, i;

	if(!gapsize)
		return 0;

	b = &c->blocks[blkidx];

	memset(gs, 0, sizeof(gs));
	ret = -1;

	mfm = (b->enctype == IPF_BLOCK_ENC_MFM);
	cellsperbit = mfm ? 2 : 1;

	if(c->encoder != IPFD_ENCODER_SPS || !(b->flags & (IPF_BLOCK_FLAG_FORWARD_GAP | IPF_BLOCK_FLAG_BACKWARD_GAP)))
	{
		// No gap stream : the gap value byte is repeated in both directions.
		for(i = 0; i < 2; i++)
		{
			gs[i].defaultsample = (uint8_t)b->gapvalue;
			if(ipfd_add_gapsample(&gs[i], &gs[i].defaultsample, 8, 0))
				goto end;
		}
	}
	else
	{
		off = b->gapoffset;
		if(off >= c->edsize)
			goto end;

		if(b->flags & IPF_BLOCK_FLAG_FORWARD_GAP)
		{
			if(ipfd_parse_gaplist(c, &off, &gs[0]))
				goto end;
		}

		if(b->flags & IPF_BLOCK_FLAG_BACKWARD_GAP)
		{
			if(ipfd_parse_gaplist(c, &off, &gs[1]))
				goto end;
		}
	}

	if(ipfd_gapstream_sizes(&gs[0], 0, cellsperbit) || ipfd_gapstream_sizes(&gs[1], 1, cellsperbit))
		goto end;

	se0 = gs[0].fixcells || gs[0].loopcells;
	se1 = gs[1].fixcells || gs[1].loopcells;

	tl0 = gs[0].loopcells && (gs[0].looptype == IPFD_LOOP_STREAM);
	tl1 = gs[1].loopcells && (gs[1].looptype == IPFD_LOOP_STREAM);

	if(!se0 && !se1)
		goto end;

	if(se0 != se1)
	{
		// Only one stream
		if(se0)
			ret = ipfd_gap_stream(c, &gs[0], gapsize, 0, mfm);
		else
			ret = ipfd_gap_stream(c, &gs[1], gapsize, 1, mfm);

		goto end;
	}

	// Two streams : compute the size of each one
	gs0 = gs[0].fixcells;
	gs1 = gs[1].fixcells;

	if(tl0 + tl1 == 1)
	{
		// The stream with the loop fills the remaining gap
		if(tl1)
		{
			if(gs0 > gapsize)
				gs0 = gapsize;
			gs1 = gapsize - gs0;
		}
		else
		{
			if(gs1 > gapsize)
				gs1 = gapsize;
			gs0 = gapsize - gs1;
		}
	}
	else
	{
		fs = gs0 + gs1;
		if(fs >= gapsize)
		{
			// Remove the extra cells evenly
			rem = fs - gapsize;
			rem0 = rem >> 1;
			rem1 = rem - rem0;

			while(rem0 || rem1)
			{
				if(gs0 >= rem0)
				{
					gs0 -= rem0;
					rem0 = 0;
				}
				else
				{
					rem1 += rem0 - gs0;
					rem0 = 0;
					gs0 = 0;
				}

				if(gs1 >= rem1)
				{
					gs1 -= rem1;
					rem1 = 0;
				}
				else
				{
					rem0 += rem1 - gs1;
					rem1 = 0;
					gs1 = 0;
				}
			}
		}
		else
		{
			if(!gs[0].loopcells && !gs[1].loopcells)
				goto end;

			miss = gapsize - fs;
			miss0 = miss >> 1;

			// Track gap of the last block crossing the index : the forward stream ends at the index.
			if(blkidx == c->nbblocks - 1)
			{
				sbp = c->pos % c->trackbits;
				if(sbp + gs0 <= c->trackbits && sbp + gs0 + miss >= c->trackbits)
					miss0 = c->trackbits - (sbp + gs0);
			}

			miss1 = miss - miss0;

			while(miss0 || miss1)
			{
				if(gs[0].loopcells)
				{
					gs0 += miss0;
					miss0 = 0;
				}
				else
				{
					miss1 += miss0;
					miss0 = 0;
				}

				if(gs[1].loopcells)
				{
					gs1 += miss1;
					miss1 = 0;
				}
				else
				{
					miss0 += miss1;
					miss1 = 0;
				}
			}
		}
	}

	if(gs0 + gs1 != gapsize)
		goto end;

	if(ipfd_gap_stream(c, &gs[0], (uint32_t)gs0, 0, mfm))
		goto end;

	// Write splice between the two streams
	c->splitvalid = 1;
	c->splitpos = c->pos;

	ret = ipfd_gap_stream(c, &gs[1], (uint32_t)gs1, 1, mfm);

end:
	ipfd_free_gapstream(&gs[0]);
	ipfd_free_gapstream(&gs[1]);

	return ret;
}

///////////////////////////////////////////////////////////////////////////////
// Cell density maps (one value per track byte, 1000 = nominal cell size)
///////////////////////////////////////////////////////////////////////////////

static uint32_t ipfd_bytes(uint32_t bits)
{
	return (bits + 7) / 8;
}

static void ipfd_density_add(uint32_t * tb, int32_t timecnt, int32_t pos, int32_t from, int32_t to, int32_t val)
{
	int32_t cp, p;

	for(cp = from; cp < to; cp++)
	{
		p = (pos + cp) % timecnt;
		if(p < 0)
			p += timecnt;

		tb[p] += val;
	}
}

static uint32_t * ipfd_density_map(ipfd_ctx * c, uint32_t density, uint32_t startbit, uint32_t * timecnt_out)
{
	uint32_t * tb;
	int32_t timecnt, pos, bs, gs, lg;
	uint32_t blk, key, mask;
	int32_t delta;

	timecnt = ipfd_bytes(c->trackbits);
	if(!timecnt)
		return NULL;

	tb = malloc(timecnt * sizeof(uint32_t));
	if(!tb)
		return NULL;

	for(pos = 0; pos < timecnt; pos++)
		tb[pos] = 1000;

	pos = startbit >> 3;

	lg = 0;
	if(density == IPFD_DEN_COPYLOCK_AMIGA && c->nbblocks > 3)
		lg = ipfd_bytes(c->blocks[3].gapbits);

	if(density == IPFD_DEN_COPYLOCK_AMIGA2 && c->nbblocks)
		lg = ipfd_bytes(c->blocks[c->nbblocks - 1].gapbits);

	key = 0;
	mask = 1;

	for(blk = 0; blk < c->nbblocks; blk++)
	{
		bs = ipfd_bytes(c->blocks[blk].databits);
		gs = ipfd_bytes(c->blocks[blk].gapbits);
		delta = 0;

		switch(density)
		{
			case IPFD_DEN_COPYLOCK_AMIGA:   // Blocks 4,5,6 and the previous gaps
			case IPFD_DEN_COPYLOCK_AMIGA2:  // Blocks 0,1,2 and the previous gaps
				switch(blk - (density == IPFD_DEN_COPYLOCK_AMIGA ? 4 : 0))
				{
					case 0: delta = -55; break;
					case 1: delta = -5;  break;
					case 2: delta = 45;  break;
				}

				if(delta)
					ipfd_density_add(tb, timecnt, pos, -lg, bs, delta);
			break;

			case IPFD_DEN_COPYLOCK_ST:
				if(blk == 5)
					delta = 50;
			break;

			case IPFD_DEN_SPEEDLOCK_AMIGA:
				if(blk == 1)
					delta = 100;
				if(blk == 2)
					delta = -100;
			break;

			case IPFD_DEN_SPEEDLOCK_AMIGA2:
				if(blk == 1)
					delta = 50;
			break;

			case IPFD_DEN_ABRIERLEY_AMIGA:
				switch(blk)
				{
					case 1: delta = 100;  break;
					case 2: delta = 50;   break;
					case 4: delta = -50;  break;
					case 5: delta = -100; break;
					case 6: delta = -150; break;
				}
			break;

			case IPFD_DEN_ABRIERLEY_AMIGA2:
				// The density key is stored in the first block gap value.
				if(!blk)
				{
					key = c->blocks[0].gapvalue;
				}
				else
				{
					delta = (key & mask) ? -50 : 50;
					mask <<= 1;
				}
			break;
		}

		if(delta && density != IPFD_DEN_COPYLOCK_AMIGA && density != IPFD_DEN_COPYLOCK_AMIGA2)
			ipfd_density_add(tb, timecnt, pos, 0, bs, delta);

		pos += bs + gs;
		if(pos >= timecnt)
			pos -= timecnt;

		lg = gs;
	}

	*timecnt_out = timecnt;

	return tb;
}

///////////////////////////////////////////////////////////////////////////////
// Track decoder
///////////////////////////////////////////////////////////////////////////////

typedef struct ipfd_trackout_
{
	uint8_t *  cells;
	uint8_t *  weak;
	uint32_t   trackbits;
	int32_t    overlap;      // write splice position (-1 : unknown)
	uint32_t * timebuf;      // density map (variable density tracks only)
	uint32_t   timecnt;
	int        weakfound;
}ipfd_trackout;

static void ipfd_free_trackout(ipfd_trackout * t)
{
	free(t->cells);
	free(t->weak);
	free(t->timebuf);
	memset(t, 0, sizeof(ipfd_trackout));
}

static int ipfd_decode_track(HXCFE * hxcfe, ipfd_trackentry * te, const uint8_t * file, uint32_t encoder, int lockindex, ipfd_trackout * out)
{
	ipfd_ctx c;
	uint32_t i, trackbits, databits, gapbits, startbit, prev;
	uint32_t * fdbitpos;
	int * fdtype;
	int ret;

	memset(out, 0, sizeof(ipfd_trackout));
	out->overlap = -1;

	if(te->imge.density == IPFD_DEN_NOISE)
		return IPFD_TRK_UNFORMATTED;

	if(te->imge.density == 0 || te->imge.density >= IPFD_DEN_LAST)
	{
		hxcfe->hxc_printf(MSG_ERROR,"IPF Loader : Track %d:%d : unsupported density type %d", te->imge.cyl, te->imge.head, te->imge.density);
		return IPFD_TRK_ERROR;
	}

	if(te->imge.signal != IPF_SIGNAL_2US || te->imge.process)
	{
		hxcfe->hxc_printf(MSG_ERROR,"IPF Loader : Track %d:%d : unsupported signal type (%d) / process (%d)", te->imge.cyl, te->imge.head, te->imge.signal, te->imge.process);
		return IPFD_TRK_ERROR;
	}

	if(!te->imge.blkcnt)
		return IPFD_TRK_UNFORMATTED;

	if(!te->data_size)
		return IPFD_TRK_ERROR;

	memset(&c, 0, sizeof(c));
	c.hxcfe = hxcfe;
	c.ed = file + te->data_offset;
	c.edsize = te->data_size;
	c.encoder = encoder;

	if(te->data_crc && std_crc32(0x00000000, (unsigned char*)c.ed, c.edsize) != te->data_crc)
	{
		hxcfe->hxc_printf(MSG_ERROR,"IPF Loader : Track %d:%d : bad data CRC", te->imge.cyl, te->imge.head);
		return IPFD_TRK_ERROR;
	}

	if(te->imge.blkcnt > c.edsize / IPF_BLOCK_DESCRIPTOR_SIZE)
	{
		hxcfe->hxc_printf(MSG_ERROR,"IPF Loader : Track %d:%d : blocks descriptors outside the data record", te->imge.cyl, te->imge.head);
		return IPFD_TRK_ERROR;
	}

	c.nbblocks = te->imge.blkcnt;
	c.blocks = calloc(c.nbblocks, sizeof(ipfd_block));
	fdbitpos = calloc(c.nbblocks, sizeof(uint32_t));
	fdtype = calloc(c.nbblocks, sizeof(int));
	if(!c.blocks || !fdbitpos || !fdtype)
	{
		ret = IPFD_TRK_ERROR;
		goto end;
	}

	databits = 0;
	gapbits = 0;
	for(i = 0; i < c.nbblocks; i++)
	{
		const uint8_t * d = &c.ed[i * IPF_BLOCK_DESCRIPTOR_SIZE];

		c.blocks[i].databits   = ipfd_be32(d + 0);
		c.blocks[i].gapbits    = ipfd_be32(d + 4);
		c.blocks[i].gapoffset  = ipfd_be32(d + 8);
		c.blocks[i].celltype   = ipfd_be32(d + 12);
		c.blocks[i].enctype    = ipfd_be32(d + 16);
		c.blocks[i].flags      = ipfd_be32(d + 20);
		c.blocks[i].gapvalue   = ipfd_be32(d + 24);
		c.blocks[i].dataoffset = ipfd_be32(d + 28);

		if(encoder == IPFD_ENCODER_CAPS)
		{
			// CAPS encoder : no gap stream, no flags, 2us cells
			c.blocks[i].gapoffset = 0;
			c.blocks[i].celltype = IPF_CELL_TYPE_2US;
			c.blocks[i].flags = 0;
		}

		// Gaps smaller than one byte are ignored.
		if(c.blocks[i].gapbits < 8)
			c.blocks[i].gapbits = 0;

		if(c.blocks[i].enctype != IPF_BLOCK_ENC_MFM && c.blocks[i].enctype != IPF_BLOCK_ENC_RAW)
		{
			hxcfe->hxc_printf(MSG_ERROR,"IPF Loader : Track %d:%d : unsupported block encoding %d", te->imge.cyl, te->imge.head, c.blocks[i].enctype);
			ret = IPFD_TRK_ERROR;
			goto end;
		}

		databits += c.blocks[i].databits;
		gapbits += c.blocks[i].gapbits;

		if(databits > 0x1000000 || gapbits > 0x1000000)
		{
			ret = IPFD_TRK_ERROR;
			goto end;
		}
	}

	trackbits = databits + gapbits;
	if(!trackbits)
	{
		ret = IPFD_TRK_UNFORMATTED;
		goto end;
	}

	c.trackbits = trackbits;
	c.cells = calloc(1, ipfd_bytes(trackbits) + 1);
	c.weak = calloc(1, ipfd_bytes(trackbits) + 1);
	if(!c.cells || !c.weak)
	{
		ret = IPFD_TRK_ERROR;
		goto end;
	}

	// First block position from the index
	startbit = te->imge.startbit % trackbits;

	if(lockindex && (te->imge.density == IPFD_DEN_AUTO))
		startbit = 0;

	c.pos = startbit;

	ret = IPFD_TRK_OK;

	for(i = 0; i < c.nbblocks; i++)
	{
		c.blkfirst = 1;
		c.firsttype = IPFD_FIRST_NONE;
		c.splitvalid = 0;
		fdbitpos[i] = c.pos;

		if(ipfd_data_stream(&c, &c.blocks[i], c.blocks[i].databits))
		{
			hxcfe->hxc_printf(MSG_ERROR,"IPF Loader : Track %d:%d : block %d data stream error", te->imge.cyl, te->imge.head, i);
			ret = IPFD_TRK_ERROR;
			goto end;
		}

		if(ipfd_block_gap(&c, i, c.blocks[i].gapbits))
		{
			hxcfe->hxc_printf(MSG_ERROR,"IPF Loader : Track %d:%d : block %d gap stream error", te->imge.cyl, te->imge.head, i);
			ret = IPFD_TRK_ERROR;
			goto end;
		}

		fdtype[i] = c.firsttype;
	}

	// The MFM clock of each block first cell depends on the cell before the block.
	for(i = 0; i < c.nbblocks; i++)
	{
		if(fdtype[i] != IPFD_FIRST_MFM)
			continue;

		prev = fdbitpos[i] ? fdbitpos[i] - 1 : trackbits - 1;
		if(ipfd_getbit(c.cells, prev))
			ipfd_setbit(c.cells, fdbitpos[i], 0);
	}

	// Track write splice : between the last block gap streams.
	if(c.splitvalid)
	{
		out->overlap = c.splitpos % trackbits;

		for(i = 0; i < IPFD_OVERLAP_FLIP_BITS; i++)
		{
			prev = (out->overlap + i) % trackbits;
			ipfd_setbit(c.cells, prev, !ipfd_getbit(c.cells, prev));
		}
	}

	if(te->imge.density >= IPFD_DEN_COPYLOCK_AMIGA)
	{
		out->timebuf = ipfd_density_map(&c, te->imge.density, startbit, &out->timecnt);
		if(!out->timebuf)
		{
			ret = IPFD_TRK_ERROR;
			goto end;
		}
	}

	out->cells = c.cells;
	out->weak = c.weak;
	out->trackbits = trackbits;
	out->weakfound = c.weakfound;
	c.cells = NULL;
	c.weak = NULL;

end:
	free(c.cells);
	free(c.weak);
	free(c.blocks);
	free(fdbitpos);
	free(fdtype);

	return ret;
}

///////////////////////////////////////////////////////////////////////////////
// HxC track generation
///////////////////////////////////////////////////////////////////////////////

// Copy the track and insert one 0 cell at the write splice position (same as the SPS loader).
static uint32_t ipfd_copy_splice(uint8_t * dst, const uint8_t * src, uint32_t len, int32_t overlap)
{
	uint32_t i, j;

	j = 0;
	for(i = 0; i < len; i++)
	{
		if(overlap >= 0 && i == (uint32_t)overlap)
			ipfd_setbit(dst, j++, 0);

		ipfd_setbit(dst, j++, ipfd_getbit(src, i));
	}

	return j;
}

static HXCFE_SIDE * ipfd_alloc_side(uint32_t tracklen)
{
	HXCFE_SIDE * side;
	uint32_t len;

	len = ipfd_bytes(tracklen);

	side = calloc(1, sizeof(HXCFE_SIDE));
	if(!side)
		return NULL;

	side->tracklen = tracklen;
	side->databuffer = calloc(1, len);
	side->indexbuffer = calloc(1, len);
	if(!side->databuffer || !side->indexbuffer)
	{
		free(side->databuffer);
		free(side->indexbuffer);
		free(side);
		return NULL;
	}

	side->track_encoding = AMIGA_MFM_ENCODING;
	side->bitrate = 250000;

	return side;
}

static HXCFE_SIDE * ipfd_unformatted_side(uint8_t fill)
{
	HXCFE_SIDE * side;
	uint32_t len;

	side = ipfd_alloc_side(IPFD_UNFORMATTED_SIZE);
	if(!side)
		return NULL;

	len = ipfd_bytes(IPFD_UNFORMATTED_SIZE);

	side->flakybitsbuffer = malloc(len);
	if(!side->flakybitsbuffer)
	{
		hxcfe_freeSide(NULL, side);
		return NULL;
	}

	memset(side->databuffer, fill, len);
	memset(side->flakybitsbuffer, 0xFF, len);

	// Track not available / decoding error : same track as the SPS CAPS library loader.
	if(fill)
		side->track_encoding = ISOIBM_MFM_ENCODING;

	return side;
}

static HXCFE_SIDE * ipfd_build_side(ipfd_trackout * t)
{
	HXCFE_SIDE * side;
	uint32_t i, tracklen, bitrate, nbbytes;

	tracklen = t->trackbits + (t->overlap >= 0 ? 1 : 0);

	side = ipfd_alloc_side(tracklen);
	if(!side)
		return NULL;

	ipfd_copy_splice(side->databuffer, t->cells, t->trackbits, t->overlap);

	if(t->weakfound)
	{
		side->flakybitsbuffer = calloc(1, ipfd_bytes(tracklen));
		if(!side->flakybitsbuffer)
			goto error;

		ipfd_copy_splice(side->flakybitsbuffer, t->weak, t->trackbits, t->overlap);
	}

	bitrate = ((IPFD_RPM / 60) * tracklen) >> 1;
	side->bitrate = bitrate;

	if(t->timebuf)
	{
		nbbytes = ipfd_bytes(tracklen);

		side->timingbuffer = malloc(nbbytes * sizeof(uint32_t));
		if(!side->timingbuffer)
			goto error;

		for(i = 0; i < nbbytes; i++)
		{
			uint32_t tv = t->timebuf[i < t->timecnt ? i : t->timecnt - 1];
			side->timingbuffer[i] = ((1000 - tv) * (bitrate / 1000)) + bitrate;
		}

		side->bitrate = VARIABLEBITRATE;
	}

	return side;

error:
	hxcfe_freeSide(NULL, side);
	return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// File parsing
///////////////////////////////////////////////////////////////////////////////

int IPF_decoder_IsValidFile(HXCFE_IMGLDR * imgldr_ctx, char * path)
{
	FILE * f;
	uint8_t hdr[IPF_RECORD_HEADER_SIZE];
	int ret;

	ret = HXCFE_BADFILE;

	f = hxc_fopen(path, "rb");
	if(!f)
		return HXCFE_ACCESSERROR;

	if(hxc_fread(hdr, sizeof(hdr), f) == 1)
	{
		if(ipfd_be32(hdr) == IPFD_REC_CAPS && ipfd_be32(hdr + 4) == IPF_RECORD_HEADER_SIZE)
			ret = HXCFE_VALIDFILE;
	}

	hxc_fclose(f);

	return ret;
}

static int ipfd_check_header_crc(const uint8_t * rec, uint32_t size)
{
	uint8_t tmp[IPF_RECORD_HEADER_SIZE];
	uint32_t crc;

	memcpy(tmp, rec, IPF_RECORD_HEADER_SIZE);
	memset(&tmp[8], 0, 4);

	crc = std_crc32(0x00000000, tmp, IPF_RECORD_HEADER_SIZE);
	if(size > IPF_RECORD_HEADER_SIZE)
		crc = std_crc32(crc, (unsigned char*)rec + IPF_RECORD_HEADER_SIZE, size - IPF_RECORD_HEADER_SIZE);

	return crc == ipfd_be32(rec + 8);
}

int IPF_decoder_Load_DiskFile(HXCFE_IMGLDR * imgldr_ctx, HXCFE_FLOPPY * floppydisk, char * imgfile)
{
	HXCFE * hxcfe;
	FILE * f;
	uint8_t * file;
	uint32_t filesize, pos, recsize, rectype, i, j, k;
	uint32_t encoder, enc_rev, mincyl, maxcyl, minhead, maxhead, nbcyl, nbsides;
	int info_found, lockindex, ret, trkret, progress;
	ipfd_trackentry * entries;
	uint32_t nbentries;
	ipfd_trackout tout;
	HXCFE_CYLINDER * cyl;
	HXCFE_SIDE * side;

	hxcfe = imgldr_ctx->hxcfe;

	hxcfe->hxc_printf(MSG_DEBUG,"IPF_decoder_Load_DiskFile %s",imgfile);

	file = NULL;
	entries = NULL;
	nbentries = 0;

	f = hxc_fopen(imgfile, "rb");
	if(!f)
	{
		hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	filesize = hxc_fgetsize(f);
	if(filesize < IPF_RECORD_HEADER_SIZE)
	{
		hxc_fclose(f);
		return HXCFE_BADFILE;
	}

	file = malloc(filesize);
	if(!file)
	{
		hxc_fclose(f);
		return HXCFE_INTERNALERROR;
	}

	if(hxc_fread(file, filesize, f) != 1)
	{
		hxcfe->hxc_printf(MSG_ERROR,"Error while loading %s !", imgfile);
		hxc_fclose(f);
		free(file);
		return HXCFE_ACCESSERROR;
	}

	hxc_fclose(f);

	hxcfe->hxc_printf(MSG_INFO_1,"IPF File loaded (%d bytes, CRC32: 0x%.8X)", filesize, std_crc32(0x00000000, file, filesize) );

	ret = HXCFE_BADFILE;

	if(ipfd_be32(file) != IPFD_REC_CAPS)
	{
		hxcfe->hxc_printf(MSG_ERROR,"IPF Loader : Not an IPF file !");
		goto done;
	}

	// Records scan
	info_found = 0;
	encoder = 0;
	enc_rev = 0;
	mincyl = maxcyl = minhead = maxhead = 0;

	pos = 0;
	while(pos < filesize)
	{
		if(filesize - pos < IPF_RECORD_HEADER_SIZE)
		{
			hxcfe->hxc_printf(MSG_ERROR,"IPF Loader : Truncated record at offset 0x%X",pos);
			goto done;
		}

		rectype = ipfd_be32(&file[pos]);
		recsize = ipfd_be32(&file[pos + 4]);

		if(recsize < IPF_RECORD_HEADER_SIZE || recsize > filesize - pos)
		{
			hxcfe->hxc_printf(MSG_ERROR,"IPF Loader : Bad record size at offset 0x%X",pos);
			goto done;
		}

		switch(rectype)
		{
			case IPFD_REC_CAPS:
			case IPFD_REC_INFO:
			case IPFD_REC_IMGE:
			case IPFD_REC_DATA:
				if(!ipfd_check_header_crc(&file[pos], recsize))
				{
					hxcfe->hxc_printf(MSG_ERROR,"IPF Loader : Bad record CRC at offset 0x%X",pos);
					goto done;
				}
			break;
		}

		switch(rectype)
		{
			case IPFD_REC_INFO:
				if(recsize < IPF_RECORD_HEADER_SIZE + IPF_INFO_BLOCK_SIZE)
					goto done;

				encoder = ipfd_be32(&file[pos + 12 + 4]);
				enc_rev = ipfd_be32(&file[pos + 12 + 8]);
				mincyl  = ipfd_be32(&file[pos + 12 + 24]);
				maxcyl  = ipfd_be32(&file[pos + 12 + 28]);
				minhead = ipfd_be32(&file[pos + 12 + 32]);
				maxhead = ipfd_be32(&file[pos + 12 + 36]);
				info_found = 1;

				hxcfe->hxc_printf(MSG_DEBUG,"IPF INFO : Media %d, Encoder %d (rev %d), File key 0x%.8X rev %d, Cylinders %d-%d, Heads %d-%d, Platforms %d %d %d %d",
										ipfd_be32(&file[pos + 12]), encoder, enc_rev,
										ipfd_be32(&file[pos + 12 + 12]), ipfd_be32(&file[pos + 12 + 16]),
										mincyl, maxcyl, minhead, maxhead,
										ipfd_be32(&file[pos + 12 + 48]), ipfd_be32(&file[pos + 12 + 52]),
										ipfd_be32(&file[pos + 12 + 56]), ipfd_be32(&file[pos + 12 + 60]) );
			break;

			case IPFD_REC_IMGE:
			case IPFD_REC_DATA:
			{
				uint32_t key;
				ipfd_trackentry * te;

				if(rectype == IPFD_REC_IMGE)
				{
					if(recsize < IPF_RECORD_HEADER_SIZE + IPF_IMGE_BLOCK_SIZE)
						goto done;

					key = ipfd_be32(&file[pos + 12 + 52]);
				}
				else
				{
					if(recsize < IPF_RECORD_HEADER_SIZE + IPF_DATA_BLOCK_SIZE)
						goto done;

					key = ipfd_be32(&file[pos + 12 + 12]);
				}

				// Data key 0 is not used.
				if(key && key < 0x100000)
				{
					if(key >= nbentries)
					{
						ipfd_trackentry * tmp;

						tmp = realloc(entries, sizeof(ipfd_trackentry) * (key + 1));
						if(!tmp)
						{
							ret = HXCFE_INTERNALERROR;
							goto done;
						}

						entries = tmp;
						memset(&entries[nbentries], 0, sizeof(ipfd_trackentry) * ((key + 1) - nbentries));
						nbentries = key + 1;
					}

					te = &entries[key];

					if(rectype == IPFD_REC_IMGE)
					{
						const uint8_t * b = &file[pos + 12];

						te->imge.cyl      = ipfd_be32(b + 0);
						te->imge.head     = ipfd_be32(b + 4);
						te->imge.density  = ipfd_be32(b + 8);
						te->imge.signal   = ipfd_be32(b + 12);
						te->imge.startbit = ipfd_be32(b + 24);
						te->imge.blkcnt   = ipfd_be32(b + 40);
						te->imge.process  = ipfd_be32(b + 44);
						te->imge.flags    = ipfd_be32(b + 48);
						te->imge.datakey  = key;
						te->imge_valid = 1;
					}
					else
					{
						uint32_t extsize = ipfd_be32(&file[pos + 12]);

						if(extsize > filesize - (pos + recsize))
						{
							hxcfe->hxc_printf(MSG_ERROR,"IPF Loader : Truncated data record (key %d)",key);
							extsize = 0;
						}

						te->data_offset = pos + recsize;
						te->data_size = extsize;
						te->data_crc = ipfd_be32(&file[pos + 12 + 8]);
						te->data_valid = 1;
					}
				}

				if(rectype == IPFD_REC_DATA)
				{
					uint32_t extsize = ipfd_be32(&file[pos + 12]);

					if(extsize > filesize - (pos + recsize))
						extsize = filesize - (pos + recsize);

					pos += extsize;
				}
			}
			break;

			default:  // CAPS, CTEI, CTEX and unknown records : ignored
			break;
		}

		pos += recsize;
	}

	if(!info_found)
	{
		hxcfe->hxc_printf(MSG_ERROR,"IPF Loader : No INFO record !");
		goto done;
	}

	for(k = 0; k < nbentries; k++)
	{
		if(entries[k].imge_valid && entries[k].data_valid)
			break;
	}

	if(k == nbentries)
	{
		hxcfe->hxc_printf(MSG_ERROR,"IPF Loader : No track in this file !");
		goto done;
	}

	if((encoder != IPFD_ENCODER_CAPS && encoder != IPFD_ENCODER_SPS) || enc_rev != 1)
	{
		hxcfe->hxc_printf(MSG_ERROR,"IPF Loader : Unsupported encoder %d (rev %d) !",encoder,enc_rev);
		goto done;
	}

	if(mincyl > maxcyl || minhead > maxhead || maxcyl >= IPFD_MAX_CYLINDERS || maxhead >= IPFD_MAX_SIDES)
	{
		hxcfe->hxc_printf(MSG_ERROR,"IPF Loader : Unsupported geometry : cylinders %d-%d, heads %d-%d !",mincyl,maxcyl,minhead,maxhead);
		goto done;
	}

	lockindex = hxcfe_getEnvVarValue( hxcfe, "LOADER_IPF_CAPS_DI_LOCK_INDEX" ) ? 1 : 0;

	nbcyl = maxcyl + 1;
	nbsides = maxhead + 1;

	floppydisk->floppySectorPerTrack = 0;
	floppydisk->floppyNumberOfSide = nbsides;
	floppydisk->floppyNumberOfTrack = nbcyl;
	floppydisk->floppyBitRate = 250000;
	floppydisk->floppyiftype = AMIGA_DD_FLOPPYMODE;

	floppydisk->tracks = (HXCFE_CYLINDER**)calloc(nbcyl, sizeof(HXCFE_CYLINDER*));
	if(!floppydisk->tracks)
		goto alloc_error;

	for(i = 0; i < nbcyl; i++)
	{
		floppydisk->tracks[i] = allocCylinderEntry(IPFD_RPM, nbsides);
		if(!floppydisk->tracks[i])
			goto alloc_error;
	}

	progress = 0;
	for(i = 0; i < nbcyl; i++)
	{
		cyl = floppydisk->tracks[i];

		for(j = 0; j < nbsides; j++)
		{
			ipfd_trackentry * te;

			hxcfe_imgCallProgressCallback(imgldr_ctx, progress++, nbcyl * nbsides);

			// Last IMGE record of this track with its DATA record
			te = NULL;
			for(k = 0; k < nbentries; k++)
			{
				if(entries[k].imge_valid && entries[k].data_valid && entries[k].imge.cyl == i && entries[k].imge.head == j)
					te = &entries[k];
			}

			side = NULL;
			trkret = IPFD_TRK_UNFORMATTED;

			if(!te || i < mincyl || j < minhead)
			{
				hxcfe->hxc_printf(MSG_DEBUG,"IPF Loader : Track %d:%d not present",i,j);
				side = ipfd_unformatted_side(0x11);
			}
			else
			{
				trkret = ipfd_decode_track(hxcfe, te, file, encoder, lockindex, &tout);

				switch(trkret)
				{
					case IPFD_TRK_OK:
						hxcfe->hxc_printf(MSG_DEBUG,"IPF Loader : Track %d:%d : %d blocks, %d cells, density %d, start %d, splice %d%s",
												i, j, te->imge.blkcnt, tout.trackbits, te->imge.density,
												te->imge.startbit, tout.overlap, tout.weakfound ? ", weak bits" : "");

						side = ipfd_build_side(&tout);
					break;

					case IPFD_TRK_UNFORMATTED:
						hxcfe->hxc_printf(MSG_DEBUG,"IPF Loader : Track %d:%d : unformatted track",i,j);
						side = ipfd_unformatted_side(0x00);
					break;

					default:
						hxcfe->hxc_printf(MSG_ERROR,"IPF Loader : Track %d:%d decoding error : random track",i,j);
						side = ipfd_unformatted_side(0x11);
					break;
				}

				ipfd_free_trackout(&tout);

				if(trkret != IPFD_TRK_ERROR && floppydisk->floppySectorPerTrack < (int32_t)te->imge.blkcnt)
					floppydisk->floppySectorPerTrack = te->imge.blkcnt;
			}

			if(!side)
				goto alloc_error;

			if(te && trkret != IPFD_TRK_ERROR)
				side->number_of_sector = te->imge.blkcnt;

			cyl->sides[j] = side;

			fillindex(IPFD_INDEX_POSITION, side, IPFD_INDEX_DURATION, TRUE, 0);
		}
	}

	hxcfe->hxc_printf(MSG_INFO_1,"IPF Loader : tracks file successfully loaded and encoded!");

	ret = HXCFE_NOERROR;

done:
	free(entries);
	free(file);

	return ret;

alloc_error:
	hxcfe->hxc_printf(MSG_ERROR,"IPF Loader : Memory allocation error !");

	free(entries);
	free(file);

	hxcfe_freeFloppy(hxcfe, floppydisk);

	return HXCFE_INTERNALERROR;
}
