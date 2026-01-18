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
// File : mfi_writer.c
// Contains: Mame MFI floppy image writer
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
#include "mfi_loader.h"
#include "mfi_writer.h"
#include "mfi_format.h"
#include "floppy_utils.h"
#include "tracks/sector_extractor.h"
#include "libhxcadaptor.h"

#include "thirdpartylibs/zlib/zlib.h"

#include "stream_analyzer/fluxStreamAnalyzer.h"

#define DEFAULT_BITS_PERIOD 1000 // ps -> 1GHz

uint8_t * write_mfi_track(HXCFE_IMGLDR* imgldr_ctx,FILE *f,HXCFE_SIDE * track,mfitrack_t * mfitrk, unsigned int nb_of_revs, int * unpackedsize, int * packed_size)
{
	uint32_t totalsize;
	uint32_t * trackbuffer;
	unsigned int i,index;
	uint32_t total_time;
	streamconv * strconv;
	uLongf packedsize;
	uint8_t * pack;

	trackbuffer = NULL;
	pack = NULL;

	strconv = initStreamConvert(imgldr_ctx->hxcfe,track, DEFAULT_BITS_PERIOD, (DEFAULT_BITS_PERIOD/1000)*65536,-1,-1,nb_of_revs+1,5000000);
	if(!strconv)
	{
		return 0;
	}

	total_time = 0;
	totalsize = 0;

	// Need to start at the first index...
	while(!strconv->index_event && !strconv->stream_end_event)
	{
		StreamConvert_getNextPulse(strconv);
	}

	// No index found ...
	if(!strconv->index_event && strconv->stream_end_event)
	{
		// Restart ...
		strconv->stream_end_event = 0;
		StreamConvert_getNextPulse(strconv);
	}

	trackbuffer = calloc(1, 64*1024 * sizeof(uint32_t) );
	if(!trackbuffer)
	{
		return 0;
	}

	index = 0;
	total_time = 0;

	while( (!strconv->stream_end_event) && index < nb_of_revs && trackbuffer)
	{
		i = 0;
		do
		{
			trackbuffer[i] = StreamConvert_getNextPulse(strconv);

			total_time += trackbuffer[i];

			i++;

			if( !( i & ((64*1024)-1) ) )
			{
				trackbuffer = realloc(trackbuffer, ( i * sizeof(uint32_t) ) + ( 64*1024 * sizeof(uint32_t) ) );

				if(trackbuffer)
					memset(&trackbuffer[i],0,( 64*1024 * sizeof(uint32_t) ));
			}

		}while(!strconv->index_event && !strconv->stream_end_event && trackbuffer);

		totalsize = ( i * sizeof(uint32_t) );

		if(strconv->index_event)
		{
			index++;
			break;
		}
	}

	packedsize = totalsize;

	if(totalsize && trackbuffer)
	{
		pack = calloc( 1, totalsize + (64*1024) );
		if( pack )
		{
			compress(pack, &packedsize, (const Bytef *)trackbuffer, totalsize);
			*packed_size = packedsize;
			*unpackedsize = totalsize;
		}
	}

	free(trackbuffer);

	deinitStreamConvert(strconv);

	return pack;
}

// Main writer function
int MFI_libWrite_DiskFile(HXCFE_IMGLDR* imgldr_ctx,HXCFE_FLOPPY * floppy,char * filename)
{
	int ret;
	mfiheader_t header;
	mfitrack_t * track_header_array = NULL;
	FILE * mfidskfile;
	int trk,head;
	int idx;
	int trkoffset;
	int unpackedsize, packedsize;
	uint8_t * pack;

	ret = HXCFE_NOERROR;
	mfidskfile = NULL;

	imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"Write MFI file %s...",filename);

	memset((void*)&header,0,sizeof(mfiheader_t));
	strcpy((char*)&header.mfi_signature,"MAMEFLOPPYIMAGE");

	header.cyl_count = floppy->floppyNumberOfTrack;
	header.head_count = floppy->floppyNumberOfSide;

	track_header_array = (mfitrack_t*)calloc(1, sizeof(mfitrack_t)*floppy->floppyNumberOfTrack * floppy->floppyNumberOfSide);
	if(!track_header_array)
	{
		ret = HXCFE_INTERNALERROR;
		goto error;
	}

	mfidskfile = hxc_fopen(filename,"wb");
	if(!mfidskfile)
	{
		ret = HXCFE_ACCESSERROR;
		goto error;
	}

	fwrite(&header,sizeof(mfiheader_t),1,mfidskfile);

	fwrite(track_header_array,sizeof(mfitrack_t)*floppy->floppyNumberOfTrack * floppy->floppyNumberOfSide,1,mfidskfile);

	trkoffset = sizeof(mfiheader_t) + (sizeof(mfitrack_t)*floppy->floppyNumberOfTrack * floppy->floppyNumberOfSide);

	for(trk=0;trk<floppy->floppyNumberOfTrack;trk++)
	{
		for(head=0;head<floppy->floppyNumberOfSide;head++)
		{
			idx = ( (trk*floppy->floppyNumberOfSide) + head );

			hxcfe_imgCallProgressCallback(imgldr_ctx,idx,floppy->floppyNumberOfTrack*floppy->floppyNumberOfSide);

			track_header_array[idx].offset = trkoffset;

			pack = write_mfi_track(imgldr_ctx, mfidskfile, floppy->tracks[trk]->sides[head], &track_header_array[idx], 1, &unpackedsize, &packedsize);
			if(pack)
			{
				fwrite( pack, packedsize, 1, mfidskfile);

				trkoffset += packedsize;

				track_header_array[idx].compressed_size = packedsize;
				track_header_array[idx].uncompressed_size = unpackedsize;

				free(pack);
			}

			hxcfe_imgCallProgressCallback(imgldr_ctx,head + (trk*floppy->floppyNumberOfTrack),floppy->floppyNumberOfTrack*floppy->floppyNumberOfSide );

		}
	}

	fseek(mfidskfile, sizeof(mfiheader_t), SEEK_SET);

	fwrite(track_header_array,sizeof(mfitrack_t)*floppy->floppyNumberOfTrack * floppy->floppyNumberOfSide,1,mfidskfile);

	fclose(mfidskfile);

	return ret;

error:
	free(track_header_array);

	if(mfidskfile)
		fclose(mfidskfile);

	return ret;
}
