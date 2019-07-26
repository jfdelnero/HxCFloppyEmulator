/*
//
// Copyright (C) 2006-2019 Jean-François DEL NERO
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
// File : streamhfe_writer.c
// Contains: Stream HFE floppy image writer
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
#include "libhxcfe.h"

#include "streamhfe_loader.h"
#include "streamhfe_format.h"

#include "tracks/trackutils.h"

#include "libhxcadaptor.h"

#include "tracks/luts.h"

unsigned short * convert_track(HXCFE_IMGLDR * imgldr_ctx, HXCFE_CYLINDER * cyl,unsigned int * tracksize)
{
	int i,j;
	unsigned int tracklen,bitrate;
	unsigned int track_time[2];

	unsigned int final_time_track_len,final_track_len;
	unsigned int timepos,bitpos;

	unsigned short * data_track_s[2];
	unsigned short * final_data_track;

	final_data_track = NULL;
	track_time[0] = 0;
	track_time[1] = 0;

	if(cyl->number_of_side)
	{
		for(i=0;i<cyl->number_of_side;i++)
		{
			track_time[i] = 0;

			tracklen = cyl->sides[i]->tracklen;
			bitrate = cyl->sides[i]->bitrate;

			for(j=0;j<tracklen;j++)
			{
				getbit(cyl->sides[i]->databuffer,j);

				if(cyl->sides[i]->timingbuffer)
				{
					bitrate = cyl->sides[i]->timingbuffer[j/8];
				}

				track_time[i] += ( (1*1000*1000*1000) / (bitrate*2));
			}
		}

		if( track_time[1] > track_time[0] )
			final_time_track_len = track_time[1];
		else
			final_time_track_len = track_time[0];

		final_track_len = ( final_time_track_len / (DEFAULT_BITS_PERIOD / 1000) ) * 2;

		if(final_track_len & 0x1F)
			final_track_len = (final_track_len & ~0x1F) + 0x20;

		data_track_s[0] = malloc((final_track_len/2) / 8);
		data_track_s[1] = malloc((final_track_len/2) / 8);
		final_data_track = malloc(final_track_len / 8);
		if(final_data_track && data_track_s[0] && data_track_s[1])
		{
			memset(final_data_track,0,final_track_len / 8);
			memset(data_track_s[0],0,(final_track_len/2) / 8);
			memset(data_track_s[1],0,(final_track_len/2) / 8);

			for(i=0;i<cyl->number_of_side;i++)
			{
				track_time[i] = 0;

				tracklen = cyl->sides[i]->tracklen;
				bitrate = cyl->sides[i]->bitrate;

				for(j=0;j<tracklen;j++)
				{
					if(getbit(cyl->sides[i]->databuffer,j))
					{
						timepos = track_time[i];
						bitpos = timepos / (DEFAULT_BITS_PERIOD / 1000);
						data_track_s[i][bitpos/16] |= (0x8000>>(bitpos&0xF));
					}

					if(cyl->sides[i]->timingbuffer)
					{
						bitrate = cyl->sides[i]->timingbuffer[j/8];
					}

					track_time[i] += ( (1*1000*1000*1000) / (bitrate*2));
				}
			}

			for(i=0;i<final_track_len/(16*2);i++)
			{
				final_data_track[i*2] = data_track_s[0][i];
				final_data_track[(i*2)+1] = data_track_s[1][i];
			}

			*tracksize = final_track_len;

			free(data_track_s[0]);
			free(data_track_s[1]);
		}
		else
		{
			if(data_track_s[0])
				free(data_track_s[0]);
			if(data_track_s[1])
				free(data_track_s[1]);
			if(final_data_track)
				free(final_data_track);
		}
	}

	return final_data_track;
}


int STREAMHFE_libWrite_DiskFile(HXCFE_IMGLDR* imgldr_ctx,HXCFE_FLOPPY * floppy,char * filename)
{
	streamhfe_fileheader * FILEHEADER;
	streamhfe_track_def * tracks_def;

	FILE * hxcstreamhfefile;
	unsigned int i;
	unsigned int tracklistlen;
	unsigned short * stream_track;
	unsigned int stream_track_size;
	unsigned char tempbuf[512];

	imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"Write Stream HFE file %s.",filename);

	if(!floppy->floppyNumberOfTrack)
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot create zero track HFE file");
		return -1;
	}

	hxcstreamhfefile = hxc_fopen(filename,"wb");

	if(hxcstreamhfefile)
	{
		FILEHEADER=(streamhfe_fileheader *) malloc(512);
		memset(FILEHEADER,0x00,512);
		memcpy(&FILEHEADER->signature,"HxC_Stream_Image",16);
		FILEHEADER->formatrevision = 0;

		FILEHEADER->flags = 0x00000000;
		FILEHEADER->number_of_track=(unsigned char)floppy->floppyNumberOfTrack;
		FILEHEADER->number_of_side=floppy->floppyNumberOfSide;
		FILEHEADER->bits_period = DEFAULT_BITS_PERIOD;

		FILEHEADER->floppyinterfacemode=(unsigned char)floppy->floppyiftype;

		imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"Floppy interface mode %s (%s)",	hxcfe_getFloppyInterfaceModeName(imgldr_ctx->hxcfe,FILEHEADER->floppyinterfacemode),
																			hxcfe_getFloppyInterfaceModeDesc(imgldr_ctx->hxcfe,FILEHEADER->floppyinterfacemode) );

		if(!floppy->double_step)
			FILEHEADER->flags |= STREAMHFE_HDRFLAG_SINGLE_SIDE;

		fwrite(FILEHEADER,512,1,hxcstreamhfefile);

		FILEHEADER->track_list_offset = ftell(hxcstreamhfefile);

		tracklistlen=((((((FILEHEADER->number_of_track)+1)*sizeof(streamhfe_track_def))/512)+1));

		tracks_def = (streamhfe_track_def *) malloc(tracklistlen*512);
		memset(tracks_def,0x00,tracklistlen*512);
		fwrite(tracks_def,tracklistlen*512,1,hxcstreamhfefile);

		//-----------------------------------------------------

		FILEHEADER->index_array_offset = ftell(hxcstreamhfefile);

		memset(&tempbuf,0,512);
		fwrite(&tempbuf,512,1,hxcstreamhfefile);

		//-----------------------------------------------------
		FILEHEADER->track_data_offset = ftell(hxcstreamhfefile);

		i=0;
		while(i<(FILEHEADER->number_of_track))
		{
			tracks_def[i].flags = STREAMHFE_TRKFLAG_PACKED;
			tracks_def[i].packed_data_offset = ftell(hxcstreamhfefile);
			stream_track_size = 0;

			stream_track = convert_track(imgldr_ctx, floppy->tracks[i],&stream_track_size);
			if(stream_track)
			{
				fwrite(stream_track,stream_track_size/8,1,hxcstreamhfefile);
				free(stream_track);
			}

			tracks_def[i].packed_data_size = stream_track_size / 8;
			tracks_def[i].unpacked_data_size = stream_track_size / 8;
			tracks_def[i].track_len = stream_track_size;

			i++;
		}

		fseek( hxcstreamhfefile, FILEHEADER->track_list_offset, SEEK_SET );
		fwrite(tracks_def,tracklistlen*512,1,hxcstreamhfefile);

		fseek( hxcstreamhfefile, 0, SEEK_SET );
		fwrite(FILEHEADER,512,1,hxcstreamhfefile);

		fclose(hxcstreamhfefile);

		imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"%d tracks written to the file",FILEHEADER->number_of_track);

		free(FILEHEADER);

		return 0;
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot create %s!",filename);

		return -1;
	}
}
