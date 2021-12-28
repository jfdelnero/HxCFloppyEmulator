/*
//
// Copyright (C) 2006-2021 Jean-François DEL NERO
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
// File : streamConvert.c
// Contains: Flux (pulses) Stream conversion utility
//
// Written by: Jean-François DEL NERO
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
#include "tracks/track_generator.h"
#include "libhxcfe.h"

#include "floppy_loader.h"
#include "floppy_utils.h"
#include "tracks/trackutils.h"

#include "fluxStreamAnalyzer.h"

#include "libhxcadaptor.h"

#include "misc/env.h"


streamconv * initStreamConvert(HXCFE_SIDE * track, float stream_period_ps, float overflowvalue)
{
	streamconv * sc;

	sc = malloc(sizeof(streamconv));
	if(sc)
	{
		memset(sc,0,sizeof(streamconv));

		sc->stream_period_ps = stream_period_ps;
		sc->track = track;
		sc->overflow_value = overflowvalue;
		sc->old_index_state = track->indexbuffer[sc->bitstream_pos>>3];
	}

	return sc;
}

uint32_t StreamConvert_getNextPulse(streamconv * sc)
{
	int i,startpoint;
	unsigned char tmp_byte;
	float totaltime;

	sc->rollover = 0x00;
	sc->index_event = 0;

	if(sc->track->timingbuffer)
	{
		totaltime = ((float)(1000*1000*1000) / (float)(sc->track->timingbuffer[sc->bitstream_pos>>3]*2));
	}
	else
	{
		totaltime = ((float)(1000*1000*1000) / (float)(sc->track->bitrate*2));
	}

	startpoint = sc->bitstream_pos % sc->track->tracklen;
	i=1;
	do
	{
		sc->bitstream_pos++;

		if( sc->bitstream_pos >= sc->track->tracklen )
		{
			sc->bitstream_pos = (sc->bitstream_pos % sc->track->tracklen);
			sc->rollover = 0xFF;
		}

		if( startpoint == sc->bitstream_pos ) // starting point reached ? -> No pulse in this track !
		{
			sc->rollover = 0x01;
			return  (uint32_t)((float)totaltime/(float)(sc->stream_period_ps/1000.0));
		}

		// Overflow...
		if(totaltime >= sc->overflow_value)
		{
			return sc->overflow_value-1;
		}

		sc->index_state = sc->track->indexbuffer[sc->bitstream_pos>>3];
		if(sc->index_state && !sc->old_index_state)
		{
			sc->index_event = 1;
		}
		sc->old_index_state = sc->index_state;

		if(sc->track->flakybitsbuffer)
		{
			tmp_byte =  (sc->track->databuffer[sc->bitstream_pos>>3] & (0x80 >> (sc->bitstream_pos & 7) )) ^ \
						( ( sc->track->flakybitsbuffer[sc->bitstream_pos>>3] & (rand() & 0xFF) ) & (0x80 >> (sc->bitstream_pos & 7) ));
		}
		else
		{
			tmp_byte = (sc->track->databuffer[sc->bitstream_pos>>3] & (0x80 >> (sc->bitstream_pos & 7) ));
		}

		if( tmp_byte )
		{
			return (int)((float)totaltime/(float)(sc->stream_period_ps/1000.0));
		}
		else
		{
			i++;

			if(sc->track->timingbuffer)
			{
				totaltime += ((float)(1000*1000*1000) / (float)(sc->track->timingbuffer[sc->bitstream_pos>>3]*2));
			}
			else
			{
				totaltime += ((float)(1000*1000*1000) / (float)(sc->track->bitrate*2));
			}
		}
	}while(1);
}

void deinitStreamConvert(streamconv * sc)
{
	if(sc)
		free(sc);
}

