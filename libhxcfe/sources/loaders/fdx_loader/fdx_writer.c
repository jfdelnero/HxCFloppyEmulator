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
// File : fdx_writer.c
// Contains: FDX floppy image writer
//
// Written by: Jean-François DEL NERO
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
#include "fdx_loader.h"
#include "fdx_writer.h"
#include "fdx_format.h"
#include "floppy_utils.h"
#include "tracks/sector_extractor.h"
#include "libhxcadaptor.h"

#include "stream_analyzer/fluxStreamAnalyzer.h"

// Main writer function
int FDX_libWrite_DiskFile(HXCFE_IMGLDR* imgldr_ctx,HXCFE_FLOPPY * floppy,char * filename)
{
	fdxheader_t fdx_header;
	FILE * outfile;
	char tmp_str[512];
	int i,j,k;
	int max_track_len;
	double maxperiod,tmpperiod;
	uint8_t * track_buffer;
	fdxtrack_t * track_header;
	streamconv * strconv;
	uint32_t pulse_pos,p;

	hxcfe_imgCallProgressCallback(imgldr_ctx,0,floppy->floppyNumberOfTrack*2 );

	imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"Write FDX file %s...",filename);

	outfile = hxc_fopen(filename,"wb");
	if( !outfile )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot create %s !",filename);
		return HXCFE_ACCESSERROR;
	}

	imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"%d tracks, %d sides...",floppy->floppyNumberOfTrack,floppy->floppyNumberOfSide);

	memset(&fdx_header,0,sizeof(fdxheader_t));

	memcpy(&fdx_header.fdx_signature,"FDX",3);
	fdx_header.revision = 3;

	hxc_getfilenamebase(filename,(char*)&tmp_str, SYS_PATH_TYPE);

#if __GNUC__ && !__clang__

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-truncation"

#endif
	strncpy((char*)&fdx_header.disk_name,tmp_str,60-1);

#if __GNUC__ && !__clang__
#pragma GCC diagnostic pop
#endif

	fdx_header.disk_type = 9; // raw

	fdx_header.nb_of_cylinders = floppy->floppyNumberOfTrack;
	fdx_header.nb_of_heads = floppy->floppyNumberOfSide;
	fdx_header.default_bitrate = ((double)((double)1E12 / (double)FDX_RAW_TICK_PERIOD ) / 1000);
	fdx_header.disk_rpm = floppy->tracks[0]->floppyRPM;
	if(fdx_header.disk_rpm < 0)
		fdx_header.disk_rpm = 300;
	fdx_header.write_protect = 0;
	fdx_header.option = 0;

	fdx_header.track_block_size = 0;

	maxperiod = 0.0;
	max_track_len = 0;
	for(i=0;i<floppy->floppyNumberOfTrack;i++)
	{
		for(j=0;j<floppy->floppyNumberOfSide;j++)
		{
			tmpperiod = GetTrackPeriod(imgldr_ctx->hxcfe,floppy->tracks[i]->sides[j]);
			if(tmpperiod > maxperiod)
				maxperiod = tmpperiod;
		}
	}

	fdx_header.disk_rpm = ( 60 / maxperiod );
	if(fdx_header.disk_rpm > 250 && fdx_header.disk_rpm < 325 )
	{
		fdx_header.disk_rpm = 300;
	}

	if(fdx_header.disk_rpm >= 325 && fdx_header.disk_rpm < 370 )
	{
		fdx_header.disk_rpm = 360;
	}

	max_track_len = (int)((double)maxperiod * ((double)1/(((double)FDX_RAW_TICK_PERIOD*1E-12))));

	max_track_len = ((max_track_len + sizeof(fdxtrack_t)*8) & ~0x3FF) + 0x400;

	fdx_header.track_block_size = max_track_len / 8;

	track_buffer = malloc(fdx_header.track_block_size);
	if(!track_buffer)
		goto error;

	fwrite( &fdx_header, sizeof(fdx_header), 1, outfile );

	track_header = (fdxtrack_t *)track_buffer;
	for(i=0;i<floppy->floppyNumberOfTrack;i++)
	{
		for(j=0;j<floppy->floppyNumberOfSide;j++)
		{
			memset( track_buffer, 0, fdx_header.track_block_size );

			track_header->cylinder = i;
			track_header->head = j;
			track_header->index_bit_place = 0;                // Index hole place (bit number)

			tmpperiod = GetTrackPeriod(imgldr_ctx->hxcfe,floppy->tracks[i]->sides[j]);
			track_header->bit_track_length = (int)((double)tmpperiod * ((double)1/(((double)FDX_RAW_TICK_PERIOD*1E-12))));

			strconv = initStreamConvert(imgldr_ctx->hxcfe,floppy->tracks[i]->sides[j], FDX_RAW_TICK_PERIOD, 0x00FFFFFF,0,0,2,5000000);
			if(strconv)
			{
				pulse_pos = StreamConvert_getNextPulse(strconv);
				while(!strconv->stream_end_event)
				{
					p = StreamConvert_getNextPulse(strconv);
					pulse_pos += p;

					for(k=0;k<3;k++)
					{
						if((pulse_pos+k) < track_header->bit_track_length)
						{
							track_buffer[sizeof(fdxtrack_t) + ((pulse_pos+k)>>3)] |= (0x80>>((pulse_pos+k)&7));
						}
					}
				}

				deinitStreamConvert(strconv);
			}

			fwrite( track_buffer, fdx_header.track_block_size, 1, outfile );
		}
	}

	free(track_buffer);

	hxc_fclose(outfile);

	return HXCFE_NOERROR;

error:
	if(outfile)
		hxc_fclose(outfile);

	free(track_buffer);

	return HXCFE_INTERNALERROR;
}
