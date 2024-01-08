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
// File : kryofluxstream_writer.c
// Contains: Kryoflux Stream floppy image writer
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

#include "kryofluxstream_format.h"
#include "kryofluxstream_loader.h"

#include "kryofluxstream_writer.h"

#include "libhxcadaptor.h"

#include "misc/env.h"

#include "stream_analyzer/fluxStreamAnalyzer.h"

int nextbyte(FILE *f,int i,unsigned char * trackbuffer)
{
	i = (i + 1) & 0xFF;

	if(!(i&0xFF))
	{
		fwrite(trackbuffer,256,1,f);
	}

	return i;
}

int alignOOB(FILE * f)
{
	int i,j;
	unsigned char buffer[8];

	j = 0;
	i = ftell(f);

	while(i&0xF)
	{
		if((0x10-(i&0xF))>=3)
		{
			memset(buffer,0x00,8);
			buffer[0] = 0xA;
			buffer[1] = 0x9;
			buffer[2] = 0x8;
			fwrite(&buffer,3,1,f);
			i=i+3;
			j += 3;
		}
		else
		{
			if((0x10-(i&0xF))==2)
			{
				memset(buffer,0x00,8);
				buffer[0] = 0x9;
				buffer[1] = 0x8;
				fwrite(&buffer,2,1,f);
				i=i+2;

				j += 2;
			}
			else
			{
				memset(buffer,0x00,8);
				buffer[0] = 0x8;
				fwrite(&buffer,1,1,f);
				i++;

				j += 1;
			}
		}
	};

	return j;
}

uint32_t write_kf_stream_track(HXCFE_IMGLDR * imgldr_ctx,char * filepath,HXCFE_SIDE * track,int tracknum,int sidenum,unsigned int revolution)
{
	char fullp[512];
	char fullp2[512];
	char fileext[16];

	unsigned char trackbuffer[256];
	uint32_t value;

	int streampos,streamsize;
	unsigned int i;

	uint32_t totalcelllen;
	uint32_t iclk;

	int SRcntdown;

	s_oob_header oobh;
	s_oob_StreamRead oobsr;
	s_oob_StreamEnd  oobse;
	s_oob_DiskIndex  oobdi;

	FILE *f;
	streamconv * strconv;

	strconv = initStreamConvert(imgldr_ctx->hxcfe,track, 41619, 0x00FFFFFF,-1,-1,revolution+1,5000000);
	if(!strconv)
		return 0;

	hxc_getpathfolder(filepath,fullp,SYS_PATH_TYPE);
	hxc_getfilenamewext(filepath,fullp2,SYS_PATH_TYPE);
	strcat(fullp,fullp2);
	sprintf(fileext,"%.2d.%d.raw",tracknum,sidenum);
	strcat(fullp,fileext);

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"write_kf_stream_track : Creating %s (trk %d, side %d, rev %d)",fullp,tracknum,sidenum,revolution);

	f = hxc_fopen(fullp,"wb");
	if(f)
	{
		streamsize = 0;

		StreamConvert_getNextPulse(strconv);

		streampos = 0;

		memset(&oobh,0,sizeof(s_oob_header));
		oobh.Sign = 0xD;
		oobh.Size = sizeof(s_oob_StreamRead);
		oobh.Type = OOBTYPE_Stream_Read;
		fwrite(&oobh,sizeof(s_oob_header),1,f);

		memset(&oobsr,0,sizeof(s_oob_StreamRead));
		oobsr.StreamPosition = streampos;
		oobsr.TrTime = 0x0;
		fwrite(&oobsr,sizeof(s_oob_StreamRead),1,f);

		SRcntdown = 0x7FF4;

		iclk = 0;
		if(!strconv->stream_end_event) // If no pulse, don't process the track.
		{
			StreamConvert_getNextPulse(strconv);

			totalcelllen = 0;

			i = 0;

			do
			{
				streamsize = 0;

				value = StreamConvert_getNextPulse(strconv);

				totalcelllen += value;

				// Encode cell value...
				if( ( (value >= 0xE) && (value < 0x100) ) )
				{
					trackbuffer[i] = (unsigned char)value;
					i = nextbyte(f,i,trackbuffer);
					streamsize = 1;
				}
				else
				{
					if( (value <= 0xD) || (value >= 0x100 && value < 0x800) )
					{
						trackbuffer[i] = (unsigned char)( value >> 8 );
						i = nextbyte(f,i,trackbuffer);
						trackbuffer[i] = (unsigned char)( value & 0xFF );
						i = nextbyte(f,i,trackbuffer);
						streamsize = 2;
					}
					else
					{
						if(value < 0x10000)
						{
							trackbuffer[i] = 0x0C; // Value16
							i = nextbyte(f,i,trackbuffer);
							trackbuffer[i] = (unsigned char)( value >> 8 );
							i = nextbyte(f,i,trackbuffer);
							trackbuffer[i] = (unsigned char)( value & 0xFF );
							i = nextbyte(f,i,trackbuffer);
							streamsize = 3;
						}
						else
						{
							streamsize = 0;
							do
							{
								trackbuffer[i] = 0x0B; // Overflow16
								i = nextbyte(f,i,trackbuffer);
								streamsize++;
								value -= 0x10000;
							}while(value>= 0x10000);

							trackbuffer[i] = 0x0C; // Value16
							i = nextbyte(f,i,trackbuffer);
							trackbuffer[i] = (unsigned char)( value >> 8 );
							i = nextbyte(f,i,trackbuffer);
							trackbuffer[i] = (unsigned char)( value & 0xFF );
							i = nextbyte(f,i,trackbuffer);

							streamsize+=3;
						}
					}
				}

				// Index pulse ?
				if( strconv->index_event )
				{
					if(i)
					{
						fwrite(trackbuffer,i,1,f);
					}

					streamsize += alignOOB(f);

					i=0;
					memset(&oobh,0,sizeof(s_oob_header));
					oobh.Sign = 0xD;
					oobh.Size = sizeof(s_oob_DiskIndex);
					oobh.Type = OOBTYPE_Index;
					fwrite(&oobh,sizeof(s_oob_header),1,f);

					memset(&oobdi,0,sizeof(s_oob_DiskIndex));
					oobdi.StreamPosition = streampos + streamsize;
					iclk = iclk + ((totalcelllen/16) * 2);
					oobdi.SysClk = iclk;
					totalcelllen = 0;
					fwrite(&oobdi,sizeof(s_oob_DiskIndex),1,f);

					imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"write_kf_stream_track : Index added (Stream pos : %d, Sysclk : %d)",oobdi.StreamPosition,oobdi.SysClk);
				}

				SRcntdown--;

				// Time to add a Stream Read OOB ?
				if(!SRcntdown)
				{
					if(i)
					{
						fwrite(trackbuffer,i,1,f);
					}
					i=0;

					streamsize += alignOOB(f);

					i=0;
					memset(&oobh,0,sizeof(s_oob_header));
					oobh.Sign = 0xD;
					oobh.Size = sizeof(s_oob_StreamRead);
					oobh.Type = OOBTYPE_Stream_Read;
					fwrite(&oobh,sizeof(s_oob_header),1,f);

					memset(&oobsr,0,sizeof(s_oob_StreamRead));
					oobsr.StreamPosition = streampos + streamsize;
					oobsr.TrTime = 0x0;
					fwrite(&oobsr,sizeof(s_oob_StreamRead),1,f);

					SRcntdown = 0x7FF4;

				}

				streampos += streamsize;

			}while(!strconv->stream_end_event);

			if(i)
			{
				fwrite(trackbuffer,i,1,f);
				i = 0;
			}

		}
		else
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_WARNING,"write_kf_stream_track : No pulse in this track !");
		}

		// Finish the stream
		memset(&oobh,0,sizeof(s_oob_header));
		oobh.Sign = 0xD;
		oobh.Size = sizeof(s_oob_StreamEnd);
		oobh.Type = OOBTYPE_Stream_End;
		fwrite(&oobh,sizeof(s_oob_header),1,f);

		memset(&oobse,0,sizeof(s_oob_StreamEnd));
		oobse.StreamPosition = streampos;
		oobse.Result = 0x0;
		fwrite(&oobse,sizeof(s_oob_StreamEnd),1,f);

		memset(trackbuffer,0x0D,7);
		fwrite(&trackbuffer,7,1,f);

		imgldr_ctx->hxcfe->hxc_printf(MSG_WARNING,"write_kf_stream_track : End of the track ! (StreamPosition : %d)",oobse.StreamPosition);

		hxc_fclose(f);
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"write_kf_stream_track : Can't create %s !",fullp);
	}

	return 0;
}

int KryoFluxStream_libWrite_DiskFile(HXCFE_IMGLDR* imgldr_ctx,HXCFE_FLOPPY * floppy,char * filename)
{
	int i,j, nbrevolutions;
	int track_step;

	nbrevolutions = hxcfe_getEnvVarValue( imgldr_ctx->hxcfe, "KFRAWEXPORT_NUMBER_OF_REVOLUTIONS" );

	track_step = 1;

	if( hxcfe_getEnvVarValue( imgldr_ctx->hxcfe, "KFRAWEXPORT_DOUBLE_STEP" ) == 1 )
		track_step = 2;

	for(j=0;j<floppy->floppyNumberOfSide;j++)
	{
		for(i=0;i<floppy->floppyNumberOfTrack;i++)
		{
			hxcfe_imgCallProgressCallback(imgldr_ctx,i + (j*floppy->floppyNumberOfTrack),floppy->floppyNumberOfTrack*floppy->floppyNumberOfSide );

			write_kf_stream_track(imgldr_ctx, filename,floppy->tracks[i]->sides[j],i*track_step,j,nbrevolutions);
		}
	}

	return HXCFE_NOERROR;
}
