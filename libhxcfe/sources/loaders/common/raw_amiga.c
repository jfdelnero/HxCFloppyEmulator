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
// File : raw_loader.c
// Contains: Amiga disk raw image loader
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

#include "raw_amiga.h"

#include "libhxcadaptor.h"

int raw_amiga_loader(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk, FILE * f_img , unsigned char * imagebuffer, int size)
{
	HXCFE_FLPGEN * fb_ctx;
	int cur_offset;
	int ret;

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

	fb_ctx = hxcfe_initFloppy( imgldr_ctx->hxcfe, 86, 2 );
	if( !fb_ctx )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Alloc Error !");
		return HXCFE_INTERNALERROR;
	}

	if( size < 100*11*2*512 )
	{
		hxcfe_setNumberOfSector ( fb_ctx, 11 );
		hxcfe_setRPM( fb_ctx, DEFAULT_AMIGA_RPM ); // normal rpm
		hxcfe_setInterfaceMode( fb_ctx, AMIGA_DD_FLOPPYMODE);
	}
	else
	{
		hxcfe_setNumberOfSector ( fb_ctx, 22 );
		hxcfe_setRPM( fb_ctx, DEFAULT_AMIGA_RPM / 2); // 150 rpm
		hxcfe_setInterfaceMode( fb_ctx, AMIGA_HD_FLOPPYMODE);
	}

	if(( size / ( 512 * hxcfe_getCurrentNumberOfTrack(fb_ctx) * hxcfe_getCurrentNumberOfSide(fb_ctx)))<80)
		hxcfe_setNumberOfTrack ( fb_ctx, 80 );
	else
		hxcfe_setNumberOfTrack ( fb_ctx, size / (512* hxcfe_getCurrentNumberOfTrack(fb_ctx) * hxcfe_getCurrentNumberOfSide(fb_ctx) ) );

	hxcfe_setNumberOfSide ( fb_ctx, 2 );
	hxcfe_setSectorSize( fb_ctx, 512 );

	hxcfe_setTrackType( fb_ctx, AMIGAFORMAT_DD);
	hxcfe_setTrackBitrate( fb_ctx, DEFAULT_AMIGA_BITRATE );

	hxcfe_setStartSectorID( fb_ctx, 0 );
	hxcfe_setSectorGap3 ( fb_ctx, 0 );

	ret = hxcfe_generateDisk( fb_ctx, floppydisk, f_img, imagebuffer, size );

	return ret;
}
