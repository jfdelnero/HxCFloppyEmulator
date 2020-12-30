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

#include "thirdpartylibs/lz4/lib/lz4lib.h"

// 0XXXXXXX  < 128
// 10XXXXXX XXXXXXXX < 16K
// 110XXXXX XXXXXXXX XXXXXXXX < 6M
// 1110XXXX XXXXXXXX XXXXXXXX XXXXXXXX < 6M

unsigned char * convert_track(HXCFE_IMGLDR * imgldr_ctx, HXCFE_SIDE * side,unsigned int * tracksize,unsigned int * outbuffersize, unsigned int * number_of_pulses)
{
	unsigned int j,k;
	unsigned int tracklen,bitrate;
	unsigned int track_time;

	unsigned int final_track_len;
	unsigned int timepos,oldbitpos,bitpos;

	unsigned char  * data_track;
	unsigned int bits_delta,nb_pulses;

	track_time = 0;

	tracklen = side->tracklen;
	bitrate = side->bitrate;

	for(j=0;j<tracklen;j++)
	{
		getbit(side->databuffer,j);

		if(side->timingbuffer)
		{
			bitrate = side->timingbuffer[j/8];
		}

		track_time += ( (1*1000*1000*1000) / (bitrate*2));
	}

	final_track_len = ( track_time / (DEFAULT_BITS_PERIOD / 1000) );

	if(final_track_len & 0x1F)
		final_track_len = (final_track_len & ~0x1F) + 0x20;

	data_track = malloc(tracklen);

	if( data_track )
	{
		memset(data_track,0,tracklen);

		track_time = 0;

		k = 0;
		nb_pulses = 0;

		oldbitpos = 0;
		for(j=0;j<tracklen;j++)
		{
			if(getbit(side->databuffer,j))
			{
				timepos = track_time;
				bitpos = timepos / (DEFAULT_BITS_PERIOD / 1000);

				bits_delta = bitpos - oldbitpos;
				if( bits_delta < 0x00000080)
				{
					data_track[k++] = bits_delta;
				}
				else
				{
					if( bits_delta < 0x00004000)
					{
						data_track[k++] = 0x80 | (bits_delta>>8);
						data_track[k++] = (bits_delta&0xFF);
					}
					else
					{
						if( bits_delta < 0x00200000)
						{
								data_track[k++] = 0xC0 | (bits_delta>>16);
								data_track[k++] = ((bits_delta>>8)&0xFF);
								data_track[k++] = (bits_delta&0xFF);
						}
						else
						{
							if( bits_delta < 0x10000000)
							{
								data_track[k++] = 0xE0 | (bits_delta>>24);
								data_track[k++] = ((bits_delta>>16)&0xFF);
								data_track[k++] = ((bits_delta>>8)&0xFF);
								data_track[k++] = (bits_delta&0xFF);
							}
							else
							{
								data_track[k++] = 0xE0 | 0x0F;
								data_track[k++] = 0xFF;
								data_track[k++] = 0xFF;
								data_track[k++] = 0xFF;
							}
						}
					}
				}

				nb_pulses++;

				oldbitpos = bitpos;
			}

			if(side->timingbuffer)
			{
				bitrate = side->timingbuffer[j/8];
			}

			track_time += ( (1*1000*1000*1000) / (bitrate*2));
		}

		*tracksize = final_track_len;
		*outbuffersize = k;
		*number_of_pulses = nb_pulses;
	}

	return data_track;
}

int STREAMHFE_libWrite_DiskFile(HXCFE_IMGLDR* imgldr_ctx,HXCFE_FLOPPY * floppy,char * filename)
{
	streamhfe_fileheader * FILEHEADER;
	streamhfe_track_def * tracks_def;

	FILE * hxcstreamhfefile;
	unsigned int i,j,packedsize;
	unsigned int tracklistlen;
	unsigned char * stream_track;
	unsigned char * packed_track;
	unsigned int stream_track_size;
	unsigned int track_size,number_of_pulses;
	unsigned char tempbuf[512];
	unsigned int max_packed_size;

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

		if(floppy->double_step)
			FILEHEADER->flags |= STREAMHFE_HDRFLAG_DOUBLE_STEP;

		fwrite(FILEHEADER,512,1,hxcstreamhfefile);

		FILEHEADER->track_list_offset = ftell(hxcstreamhfefile);

		tracklistlen=((((((FILEHEADER->number_of_track * FILEHEADER->number_of_side)+1)*sizeof(streamhfe_track_def))/512)+1));

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
			j=0;
			while(j<(FILEHEADER->number_of_side))
			{
				tracks_def[(i<<1) | (j&1)].flags = STREAMHFE_TRKFLAG_PACKED;
				tracks_def[(i<<1) | (j&1)].packed_data_offset = ftell(hxcstreamhfefile);
				stream_track_size = 0;

				packedsize = 0;
				number_of_pulses = 0;

				stream_track = convert_track(imgldr_ctx, floppy->tracks[i]->sides[j],&track_size,&stream_track_size, &number_of_pulses);
				if(stream_track)
				{
					max_packed_size = LZ4_compressBound(stream_track_size);
					packed_track = malloc(max_packed_size);
					packedsize = LZ4_compress_default((const char*)stream_track, (char*)packed_track, stream_track_size, max_packed_size);

					//packedsize = stream_track_size;
					//memcpy( (char*)packed_track , (const char*)stream_track, packedsize);

					fwrite(packed_track,packedsize,1,hxcstreamhfefile);

					free(stream_track);
					free(packed_track);
				}

				tracks_def[(i<<1) | (j&1)].packed_data_size = packedsize;
				tracks_def[(i<<1) | (j&1)].unpacked_data_size = stream_track_size;
				tracks_def[(i<<1) | (j&1)].track_len = track_size;
				tracks_def[(i<<1) | (j&1)].nb_pulses = number_of_pulses;

				j++;
			}

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
