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
// File : scp_writer.c
// Contains: SCP Stream floppy image writer
//
// Written by: Jean-François DEL NERO
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "types.h"

#include "internal_libhxcfe.h"
#include "libhxcfe.h"

#include "scp_format.h"
#include "scp_loader.h"

#include "scp_writer.h"

#include "libhxcadaptor.h"

#include "misc/env.h"

#include "stream_analyzer/fluxStreamAnalyzer.h"

static uint32_t update_checksum(uint32_t checksum,unsigned char * buffer,unsigned int size)
{
	unsigned int i;

	for(i=0;i<size;i++)
	{
		checksum += buffer[i];
	}

	return checksum;
}

uint32_t write_scp_track(HXCFE_IMGLDR* imgldr_ctx,FILE *f,HXCFE_SIDE * track,uint32_t * csum,int tracknum,unsigned int nb_of_revs)
{
	uint32_t checksum,file_checksum,size,offset,totalsize;
	unsigned short trackbuffer[256];
	unsigned int i,index;
	uint32_t total_time;
	char timestamp[64];
	scp_track_header * trkh;
	int scp_trkh_size;
	int fpos;
	time_t curtimecnt;
	struct tm * curtime;
	streamconv * strconv;
	checksum = 0;
	file_checksum = 0;

	scp_trkh_size = sizeof(scp_track_header) + ( nb_of_revs * sizeof(scp_index_pos) ) + sizeof(uint32_t);

	trkh = malloc( scp_trkh_size );
	if(!trkh)
		return 0;

	strconv = initStreamConvert(imgldr_ctx->hxcfe,track, DEFAULT_SCP_PERIOD, (DEFAULT_SCP_PERIOD/1000)*65536,-1,-1,nb_of_revs+1,5000000);
	if(!strconv)
	{
		free(trkh);
		return 0;
	}

	memset(trkh,0,scp_trkh_size);

	memcpy((char*)trkh->trk_sign,"TRK",3);

	trkh->track_number = tracknum;

	fpos = ftell(f);

	fwrite(trkh,scp_trkh_size,1,f);

	memset(trackbuffer,0,sizeof(trackbuffer));

	StreamConvert_getNextPulse(strconv);

	offset = scp_trkh_size;
	total_time = 0;
	totalsize = 0;

	index = 0;
	while( (!strconv->stream_end_event) && index < nb_of_revs)
	{
		i = 0;

		size = 0 ;

		total_time = 0;
		do
		{
			trackbuffer[i] = StreamConvert_getNextPulse(strconv);

			total_time += trackbuffer[i];

			trackbuffer[i] = BIGENDIAN_WORD( trackbuffer[i] );

			i = (i + 1) & 0xFF;

			if(!(i&0xFF))
			{
				fwrite(trackbuffer,sizeof(trackbuffer),1,f);
				checksum = update_checksum(checksum,(unsigned char*)trackbuffer,sizeof(trackbuffer));
				size = size + sizeof(trackbuffer);
			}
		}while(!strconv->index_event && !strconv->stream_end_event);

		if(i)
		{
			fwrite(trackbuffer,i*sizeof(unsigned short),1,f);
			checksum = update_checksum(checksum,(unsigned char*)trackbuffer,i*sizeof(unsigned short));
			size = size + ( i * sizeof(unsigned short) );
		}

		if(strconv->index_event)
		{
			trkh->index_position[index].index_time = LITTLEENDIAN_DWORD(total_time);
			trkh->index_position[index].track_length = LITTLEENDIAN_DWORD(size/2);
			trkh->index_position[index].track_offset = LITTLEENDIAN_DWORD(offset);

			index++;
		}

		totalsize += size;

		offset = offset + size;
	}

	time(&curtimecnt);
	curtime = localtime(&curtimecnt);

	memset(timestamp,0,sizeof(timestamp));
	sprintf(timestamp,"%d/%d/%d %d:%d:%d",curtime->tm_mon+1,curtime->tm_mday,(curtime->tm_year+1900),curtime->tm_hour,curtime->tm_min,curtime->tm_sec);
	fwrite(timestamp,strlen(timestamp),1,f);

	file_checksum = update_checksum(file_checksum,(unsigned char*)&timestamp,strlen(timestamp));

	// track_data_checksum
	*((uint32_t *)&trkh->index_position[nb_of_revs]) = checksum;

	file_checksum = update_checksum(file_checksum,(unsigned char*)trkh, scp_trkh_size);

	fseek(f,fpos,SEEK_SET);

	fwrite(trkh,scp_trkh_size,1,f);

	fseek(f,0,SEEK_END);

	file_checksum =  file_checksum + checksum;

	if(csum)
		*csum = file_checksum;

	deinitStreamConvert(strconv);

	free(trkh);

	return totalsize;
}

int SCP_libWrite_DiskFile(HXCFE_IMGLDR* imgldr_ctx,HXCFE_FLOPPY * floppy,char * filename)
{
	FILE * f;
	scp_header scph;
	uint32_t tracksoffset[83*2];
	uint32_t tracklist_offset;

	uint32_t file_checksum;
	uint32_t track_checksum;
	int i,tracknumber;

	f = hxc_fopen(filename,"wb");
	if( f )
	{
		memset(&scph,0,sizeof(scp_header));

		memcpy((char*)&scph.sign,"SCP",3);

		tracknumber = floppy->floppyNumberOfTrack * floppy->floppyNumberOfSide;

		if(tracknumber>83*2)
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_WARNING,"SCP_libWrite_DiskFile : Track number limited to 164 !");
			tracknumber = 83*2;
		}

		scph.number_of_revolution = hxcfe_getEnvVarValue( imgldr_ctx->hxcfe, "SCPEXPORT_NUMBER_OF_REVOLUTIONS" );

		scph.disk_type = 0x15;

		switch(floppy->floppyiftype)
		{
			case IBMPC_DD_FLOPPYMODE:
				scph.disk_type = 0x41;
				break;
			case IBMPC_ED_FLOPPYMODE:
			case IBMPC_HD_FLOPPYMODE:
				scph.disk_type = 0x43;
				break;

			case ATARIST_DD_FLOPPYMODE:
			case ATARIST_HD_FLOPPYMODE:
				if(floppy->floppyNumberOfSide==2)
					scph.disk_type = 0x15;
				else
					scph.disk_type = 0x14;
				break;
			case AMIGA_HD_FLOPPYMODE:
			case AMIGA_DD_FLOPPYMODE:
				scph.disk_type = 0x04;
				break;

			case C64_DD_FLOPPYMODE:
				scph.disk_type = 0x00;
				break;

			case S950_HD_FLOPPYMODE:
			case S950_DD_FLOPPYMODE:
			case EMU_SHUGART_FLOPPYMODE:
			case MSX2_DD_FLOPPYMODE:
			case CPC_DD_FLOPPYMODE:
			case GENERIC_SHUGART_DD_FLOPPYMODE:
				if(floppy->floppyNumberOfSide==2)
					scph.disk_type = 0x15;
				else
					scph.disk_type = 0x14;
				break;
		}

		if( hxcfe_getEnvVar( imgldr_ctx->hxcfe, "SCPEXPORT_DISK_TYPE", NULL ) )
		{
			scph.disk_type = hxcfe_getEnvVarValue( imgldr_ctx->hxcfe, "SCPEXPORT_DISK_TYPE" );
		}

		scph.flags = INDEXMARK;
		if(floppy->floppyNumberOfTrack>42)
			scph.flags |= DISK_96TPI;

		if( hxcfe_getEnvVar( imgldr_ctx->hxcfe, "SCPEXPORT_DISK_96TPI", NULL ) )
		{
			scph.flags |= DISK_96TPI;
		}

		if( hxcfe_getEnvVar( imgldr_ctx->hxcfe, "SCPEXPORT_DISK_48TPI", NULL ) )
		{
			scph.flags &= ~DISK_96TPI;
		}

		if(floppy->tracks[0])
		{
			if( floppy->tracks[0]->floppyRPM > 345 && floppy->tracks[0]->floppyRPM < 400)
			{
				scph.flags |= DISK_360RPM;
			}
		}

		// v2.3 - 06/03/21
		//*  Added additional FLAG bit (bit 7) to identify a 3rd party flux creator.  PLEASE
		// SET THIS BIT IF YOU ARE A 3RD PARTY DEVELOPER USING THE SCP FORMAT!
		// Bit 7 - FLUX CREATOR, cleared if the image was generated by SuperCard Pro
		//                       set if the image was created with any other devices
		scph.flags |= FLUX_CREATOR;

		switch (floppy->floppyNumberOfSide)
		{
			case 1:
				scph.number_of_heads = 1;
				scph.start_track = 0;
				scph.end_track = (tracknumber * 2) - 1;
				break;

			case 2:
				scph.number_of_heads = 0;
				scph.start_track = 0;
				scph.end_track = tracknumber - 1;
				break;

			default:
				scph.number_of_heads = 0;
				scph.start_track = 0;
				scph.end_track = tracknumber - 1;
				break;
		}

		scph.version = 0x09;

		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"SCP_libWrite_DiskFile : Flags=0x%.2X Disktype=0x%.2X NumberOfRevolution=%d Version=%d NbTrack=%d NbSide:%d",scph.flags,scph.disk_type,scph.number_of_revolution,scph.version,floppy->floppyNumberOfTrack,floppy->floppyNumberOfSide);

		// Header
		fwrite(&scph,sizeof(scp_header),1,f);

		file_checksum = 0;

		// Track list
		tracklist_offset = sizeof(scp_header);
		memset(tracksoffset,0,sizeof(tracksoffset));
		fwrite(&tracksoffset,sizeof(tracksoffset),1,f);

		file_checksum = update_checksum(file_checksum,(unsigned char*)&tracksoffset,sizeof(tracksoffset));

		for(i=0;i<tracknumber;i++)
		{
			hxcfe_imgCallProgressCallback(imgldr_ctx,i,tracknumber);

			fseek(f,0,SEEK_END);

			if(floppy->floppyNumberOfSide == 2)
			{
				tracksoffset[i] = LITTLEENDIAN_DWORD(ftell(f));
				write_scp_track(imgldr_ctx,f,floppy->tracks[i>>1]->sides[i&1],&track_checksum,i,scph.number_of_revolution);
			}
			else
			{
				tracksoffset[i * 2] = LITTLEENDIAN_DWORD(ftell(f));
				write_scp_track(imgldr_ctx,f,floppy->tracks[i]->sides[0],&track_checksum,i * 2,scph.number_of_revolution);
			}

			file_checksum = file_checksum + track_checksum;
		}

		fseek(f,tracklist_offset,SEEK_SET);
		fwrite(&tracksoffset,sizeof(tracksoffset),1,f);
		file_checksum = update_checksum(file_checksum,(unsigned char*)&tracksoffset,sizeof(tracksoffset));

		fseek(f,0,SEEK_SET);
		scph.file_data_checksum = LITTLEENDIAN_DWORD(file_checksum);
		fwrite(&scph,sizeof(scp_header),1,f);

		hxc_fclose(f);
		
		return HXCFE_NOERROR;
	}

	return HXCFE_ACCESSERROR;
}
