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
/*
streamconv * iniStreamConvert()
{
    streamconv * sc;

    sc = malloc(sizeof(streamconv));
    if(sc)
    {

    }

    return sc;
}
static unsigned short getNextPulse(HXCFE_SIDE * track,int * offset,int * rollover)
{
	int i;
	unsigned char tmp_byte;
	float totaltime;

	*rollover = 0x00;

	if(track->timingbuffer)
	{
		totaltime = ((float)(1000*1000*1000) / (float)(track->timingbuffer[(*offset)>>3]*2));
	}
	else
	{
		totaltime = ((float)(1000*1000*1000) / (float)(track->bitrate*2));
	}

	i=1;
	do
	{
		*offset = (*offset) +1;

		if( (*offset) >= track->tracklen )
		{
			*offset = ((*offset) % track->tracklen);
			*rollover = 0xFF;
		}

		// Overflow...
		if(totaltime >= (65536*25) )
		{
			return 0xFFFF;
		}

		if(track->flakybitsbuffer)
		{
			tmp_byte =  (track->databuffer[(*offset)>>3] & (0x80 >> ((*offset) & 7) )) ^ \
						( ( track->flakybitsbuffer[(*offset)>>3] & (rand() & 0xFF) ) & (0x80 >> ((*offset) & 7) ));
		}
		else
		{
			tmp_byte = (track->databuffer[(*offset)>>3] & (0x80 >> ((*offset) & 7) ));
		}

		if( tmp_byte )
		{
			return (int)((float)totaltime/(float)25);
		}
		else
		{
			i++;

			if(track->timingbuffer)
			{
				totaltime += ((float)(1000*1000*1000) / (float)(track->timingbuffer[(*offset)>>3]*2));
			}
			else
			{
				totaltime += ((float)(1000*1000*1000) / (float)(track->bitrate*2));
			}
		}
	}while(1);
}

// Get the next cell value.
static int32_t getNextPulse(HXCFE_SIDE * track,int * offset,int * rollover)
{
	int i,startpoint;
	float totaltime;

	*rollover = 0x00;

	if(track->timingbuffer)
	{
		totaltime = ((float)(1000*1000*1000) / (float)(track->timingbuffer[(*offset)>>3]*2));
	}
	else
	{
		totaltime = ((float)(1000*1000*1000) / (float)(track->bitrate*2));
	}

	startpoint = *offset % track->tracklen;
	i=1;
	for(;;)
	{
		*offset = (*offset) +1;

		if( (*offset) >= track->tracklen )
		{
			*offset = ((*offset) % track->tracklen);
			*rollover = 0xFF;
		}

		if( startpoint == *offset ) // starting point reached ? -> No pulse in this track !
		{
			*rollover = 0x01;
			return  (uint32_t)((float)totaltime/(float)(41.619));
		}

		if( track->databuffer[(*offset)>>3] & (0x80 >> ((*offset) & 7) ) )
		{
			return  (uint32_t)((float)totaltime/(float)(41.619));
		}
		else
		{
			i++;

			if(track->timingbuffer)
			{
				totaltime += ((float)(1000*1000*1000) / (float)(track->timingbuffer[(*offset)>>3]*2));
			}
			else
			{
				totaltime += ((float)(1000*1000*1000) / (float)(track->bitrate*2));
			}
		}
	}
}

*/