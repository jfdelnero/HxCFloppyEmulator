/*
//
// Copyright (C) 2006-2014 Jean-François DEL NERO
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
// Written by: DEL NERO Jean Francois
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "libhxcfe.h"

#include "scp_format.h"
#include "scp_loader.h"

#include "scp_writer.h"

#include "libhxcadaptor.h"

#include "types.h"

unsigned long update_checksum(unsigned long checksum,unsigned char * buffer,unsigned int size)
{
	unsigned int i;

	for(i=0;i<size;i++)
	{
		checksum += buffer[i];
	}

	return checksum;
}

unsigned short getNextPulse(SIDE * track,unsigned int * offset,int * rollover)
{
	int i;
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

		if( track->databuffer[(*offset)>>3] & (0x80 >> ((*offset) & 7) ) )
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

unsigned long write_scp_track(FILE *f,SIDE * track,unsigned long * csum,int tracknum,unsigned int revolution)
{
	unsigned long checksum,file_checksum,size,offset,totalsize;
	unsigned short trackbuffer[256];
	unsigned int i,j;
	unsigned long total_time;
	char timestamp[64];
	scp_track_header trkh;
	int fpos;
	unsigned int trackoffset;
	int trackrollover;
	time_t curtimecnt;
	struct tm * curtime;

	checksum = 0;
	file_checksum = 0;

	memset(&trkh,0,sizeof(scp_track_header));

	strcpy((char*)&trkh.trk_sign,"TRK");

	trkh.track_number = tracknum;

	fpos = ftell(f);

	fwrite(&trkh,sizeof(scp_track_header),1,f);

	memset(trackbuffer,0,sizeof(trackbuffer));

	trackoffset = 0;
	trackrollover = 0;

	getNextPulse(track,&trackoffset,&trackrollover);

	offset = sizeof(scp_track_header);
	total_time = 0;
	totalsize = 0;

	for(j=0;j<revolution;j++)
	{
		i = 0;

		size = 0 ;

		total_time = 0;
		do
		{
			trackbuffer[i] = getNextPulse(track,&trackoffset,&trackrollover);

			total_time += trackbuffer[i];

			trackbuffer[i] = BIGENDIAN_WORD( trackbuffer[i] );

			i = (i + 1) & 0xFF;

			if(!(i&0xFF))
			{
				fwrite(trackbuffer,sizeof(trackbuffer),1,f);
				checksum = update_checksum(checksum,(unsigned char*)trackbuffer,sizeof(trackbuffer));
				size = size + sizeof(trackbuffer);
			}
		}while(!trackrollover);

		if(i)
		{
			fwrite(trackbuffer,i*sizeof(unsigned short),1,f);
			checksum = update_checksum(checksum,(unsigned char*)trackbuffer,i*sizeof(unsigned short));
			size = size + ( i * sizeof(unsigned short) );
		}

		trkh.index_position[j].index_time = LITTLEENDIAN_DWORD(total_time);
		trkh.index_position[j].track_lenght = LITTLEENDIAN_DWORD(size/2);
		trkh.index_position[j].track_offset = LITTLEENDIAN_DWORD(offset);

		totalsize += size;

		offset = offset + size;
	}

	time(&curtimecnt);
	curtime = localtime(&curtimecnt);

	memset(timestamp,0,sizeof(timestamp));
	sprintf(timestamp,"%d/%d/%d %d:%d:%d",curtime->tm_mon+1,curtime->tm_mday,(curtime->tm_year+1900),curtime->tm_hour,curtime->tm_min,curtime->tm_sec);
	fwrite(timestamp,strlen(timestamp),1,f);

	file_checksum = update_checksum(file_checksum,(unsigned char*)&timestamp,strlen(timestamp));

	trkh.track_data_checksum = checksum;

	file_checksum = update_checksum(file_checksum,(unsigned char*)&trkh,sizeof(scp_track_header));

	fseek(f,fpos,SEEK_SET);

	fwrite(&trkh,sizeof(scp_track_header),1,f);

	fseek(f,0,SEEK_END);

	file_checksum =  file_checksum + checksum;

	if(csum)
		*csum = file_checksum;

	return totalsize;
}

int SCP_libWrite_DiskFile(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * floppy,char * filename)
{
	FILE * f;
	scp_header scph;
	unsigned long tracksoffset[83*2];
	unsigned long tracklist_offset;

	unsigned long file_checksum;
	unsigned long track_checksum;
	int i,tracknumber;

	f = hxc_fopen(filename,"wb");
	if( f )
	{
		memset(&scph,0,sizeof(scp_header));

		strcpy((char*)&scph.sign,"SCP");

		tracknumber = floppy->floppyNumberOfTrack * floppy->floppyNumberOfSide;

		if(tracknumber>83*2)
		{
			floppycontext->hxc_printf(MSG_WARNING,"SCP_libWrite_DiskFile : Track number limited to 164 !");
			tracknumber = 83*2;
		}

		scph.start_track = 0;
		scph.end_track = tracknumber - 1;
		scph.number_of_revolution = 2;

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
		
		scph.flags = 0x01;
		if(floppy->floppyNumberOfTrack>42)
			scph.flags |= 0x02;

		if(floppy->tracks[0])
		{
			if( floppy->tracks[0]->floppyRPM > 350 && floppy->tracks[0]->floppyRPM < 340)
			{
				scph.flags |= 0x04;
			}
		}

		scph.version = 0x09;

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
			fseek(f,0,SEEK_END);

			tracksoffset[i] = LITTLEENDIAN_DWORD(ftell(f));

			write_scp_track(f,floppy->tracks[i>>1]->sides[i&1],&track_checksum,i,scph.number_of_revolution);

			file_checksum = file_checksum + track_checksum;
		}

		fseek(f,tracklist_offset,SEEK_SET);
		fwrite(&tracksoffset,sizeof(tracksoffset),1,f);
		file_checksum = update_checksum(file_checksum,(unsigned char*)&tracksoffset,sizeof(tracksoffset));

		fseek(f,0,SEEK_SET);
		scph.file_data_checksum = LITTLEENDIAN_DWORD(file_checksum);
		fwrite(&scph,sizeof(scp_header),1,f);

		hxc_fclose(f);
	}


	return 0;
}
