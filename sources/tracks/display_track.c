/*
//
// Copyright (C) 2006 - 2012 Jean-François DEL NERO
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
// File : display_track.c
// Contains: 
//
// Written by:	DEL NERO Jean Francois
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "libhxcfe.h"

#include "crc.h"
#include "floppy_utils.h"
#include "math.h"

#include "display_track.h"


s_trackdisplay * td_init(unsigned long xsize,unsigned long ysize,unsigned long x_us,unsigned long y_us,unsigned long x_start_us)
{
	s_trackdisplay * td;
	
	td=malloc(sizeof(s_trackdisplay));
	memset(td,0,sizeof(s_trackdisplay));
	
	td->xsize=xsize;
	td->ysize=ysize;
	
	td->framebuffer=malloc(td->xsize*td->ysize*sizeof(unsigned long));
	memset(td->framebuffer,0,td->xsize*td->ysize*sizeof(unsigned long));

	return td;
}

void td_draw_track(s_trackdisplay *td,SIDE * currentside)
{
	int tracksize;
	int i;
	
	int bitrate;

	tracksize=currentside->tracklen;
	

	i=0;
	do
	{
		i++;

	}while(i<tracksize);
}

void td_deinit(s_trackdisplay *td)
{
	free(td->framebuffer);
	free(td);
}
