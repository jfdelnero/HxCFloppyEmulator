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
// File : raw_iso.c
// Contains: iso disk raw image loader
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
#include "libhxcfe.h"
#include "./tracks/track_generator.h"

#include "floppy_loader.h"
#include "floppy_utils.h"

#include "raw_iso.h"

#include "libhxcadaptor.h"

int raw_iso_loader(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk, FILE * f_img , unsigned char * imagebuffer, int size, raw_iso_cfg * cfg)
{
	HXCFE_FLPGEN * fb_ctx;
	int cur_offset;
	int ret;
	int32_t flags;

	if( !f_img && !imagebuffer )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"File access error or allocation error");
		return HXCFE_INTERNALERROR;
	}

	cur_offset = 0;
	if( f_img )
	{
		cur_offset = ftell(f_img);
		fseek(f_img,0, SEEK_END);
		size = ftell(f_img);
		fseek(f_img,cur_offset, SEEK_SET);
	}

	if( !size )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Null sized image !");
		return HXCFE_BADFILE;
	}

	fb_ctx = hxcfe_initFloppy( imgldr_ctx->hxcfe, 86, cfg->number_of_sides );
	if( !fb_ctx )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Alloc Error !");
		return HXCFE_INTERNALERROR;
	}

	hxcfe_setNumberOfTrack( fb_ctx, cfg->number_of_tracks );
	hxcfe_setNumberOfSide( fb_ctx, cfg->number_of_sides );
	hxcfe_setNumberOfSector( fb_ctx, cfg->number_of_sectors_per_track );
	hxcfe_setSectorSize( fb_ctx, cfg->sector_size );
	hxcfe_setStartSectorID( fb_ctx, cfg->start_sector_id );
	hxcfe_setSectorGap3( fb_ctx, cfg->gap3 );
	hxcfe_setTrackPreGap( fb_ctx, cfg->pregap );
	hxcfe_setTrackType( fb_ctx, cfg->track_format );
	hxcfe_setTrackBitrate( fb_ctx, cfg->bitrate );
	hxcfe_setRPM( fb_ctx, cfg->rpm );
	hxcfe_setInterfaceMode( fb_ctx, cfg->interface_mode );
	hxcfe_setTrackSkew ( fb_ctx, cfg->skew_per_track );
	hxcfe_setSideSkew ( fb_ctx, cfg->skew_per_side );
	hxcfe_setTrackInterleave ( fb_ctx, cfg->interleave );
	hxcfe_setSectorFill ( fb_ctx, cfg->fill_value );

	if(cfg->force_side_id >= 0)
		hxcfe_setDiskSectorsHeadID( fb_ctx, cfg->force_side_id );

	flags = 0;

	if(cfg->trk_grouped_by_sides)
		flags |= FLPGEN_SIDES_GROUPED;

	if(cfg->flip_sides)
		flags |= FLPGEN_FLIP_SIDES;

	hxcfe_setDiskFlags( fb_ctx, flags );

	ret = hxcfe_generateDisk( fb_ctx, floppydisk, f_img, imagebuffer, size );

	return ret;
}

void raw_iso_setdefcfg(raw_iso_cfg *rawcfg)
{
	if(rawcfg)
	{
		memset(rawcfg, 0, sizeof(raw_iso_cfg));
		rawcfg->number_of_tracks = 80;
		rawcfg->number_of_sides = 2;
		rawcfg->number_of_sectors_per_track = 9;
		rawcfg->sector_size = 512;
		rawcfg->start_sector_id = 1;
		rawcfg->gap3 = 84;
		rawcfg->pregap = 0;
		rawcfg->interleave = 1;
		rawcfg->skew_per_track = 0;
		rawcfg->skew_per_side = 0;
		rawcfg->bitrate = 250000;
		rawcfg->rpm = 300;
		rawcfg->track_format = IBMFORMAT_DD;
		rawcfg->interface_mode = GENERIC_SHUGART_DD_FLOPPYMODE;
		rawcfg->fill_value = 0xF6;
		rawcfg->trk_grouped_by_sides = 0;
		rawcfg->flip_sides = 0;
		rawcfg->force_side_id = -1;
	}
}
