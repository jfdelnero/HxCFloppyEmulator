/*
//
// Copyright (C) 2006, 2007, 2008, 2009 Jean-François DEL NERO
//
// This file is part of HxCFloppyEmulator.
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
// File : ti99pc99_loader.c
// Contains: TI99 PC99 floppy image loader and plugins interfaces
//
// Written by:	DEL NERO Jean Francois
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "types.h"
#include "hxc_floppy_emulator.h"
#include "internal_floppy.h"
#include "floppy_loader.h"
#include "floppy_utils.h"

#include "../common/crc.h"
#include "../common/iso_ibm_track.h"

#include "vdk_loader.h"
#include "vdk_format.h"
#include "../common/os_api.h"


int VDK_libIsValidDiskFile(HXCFLOPPYEMULATOR* floppycontext,char * imgfile)
{
	int filesize;
	vdk_header vdk_h;
	FILE * f;

	floppycontext->hxc_printf(MSG_DEBUG,"VDK_libIsValidDiskFile %s",imgfile);
	if(imgfile)
	{

		f=fopen(imgfile,"rb");
		if(f==NULL) 
		{
			floppycontext->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
			return -1;
		}
		
		fseek (f , 0 , SEEK_END); 
		filesize=ftell(f);
		fseek (f , 0 , SEEK_SET); 
		
		fread(&vdk_h,sizeof(vdk_header),1,f);
		
		fclose(f);
		
		if(vdk_h.signature==0x6B64)
		{
			filesize=filesize-vdk_h.header_size;

			if(filesize%256)
			{
				floppycontext->hxc_printf(MSG_DEBUG,"non VDK file !");
				return LOADER_BADFILE;
			}

			floppycontext->hxc_printf(MSG_DEBUG,"VDK file !");
			return LOADER_ISVALID;
		}

		floppycontext->hxc_printf(MSG_DEBUG,"non VDK file !");
		return LOADER_BADFILE;	
	}
	
	return LOADER_BADPARAMETER;
}


int VDK_libLoad_DiskFile(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	
	FILE * f;
	unsigned int filesize,sectorsize,filetracklen;
	unsigned int i,j;
	unsigned int file_offset,gap3len,interleave;
	char* trackdata;
	vdk_header vdk_h;
	int rpm;
	int tracklen;
	CYLINDER* currentcylinder;
	SIDE* currentside;
	
	
	floppycontext->hxc_printf(MSG_DEBUG,"VDK_libLoad_DiskFile %s",imgfile);
	
	f=fopen(imgfile,"rb");
	if(f==NULL) 
	{
		floppycontext->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return LOADER_ACCESSERROR;
	}

	
	fseek (f , 0 , SEEK_END); 
	filesize=ftell(f);
	fseek (f , 0 , SEEK_SET); 

	memset(&vdk_h,0,sizeof(vdk_header));
	fread(&vdk_h,sizeof(vdk_header),1,f);
	
	fseek (f , vdk_h.header_size , SEEK_SET); 

	if((vdk_h.signature!=0x6B64) || ((filesize-vdk_h.header_size)%256))
	{
		floppycontext->hxc_printf(MSG_DEBUG,"non VDK file !");
		fclose(f);
		return LOADER_BADFILE;
	}
		
			
	floppydisk->floppyNumberOfTrack=vdk_h.number_of_track;
	floppydisk->floppyNumberOfSide=vdk_h.number_of_sides;
	floppydisk->floppySectorPerTrack=(filesize-vdk_h.header_size)/(floppydisk->floppyNumberOfTrack*floppydisk->floppyNumberOfSide*256);
	floppydisk->floppyBitRate=250000;
	floppydisk->floppyiftype=GENERIC_SHUGART_DD_FLOPPYMODE;
	floppydisk->tracks=(CYLINDER**)malloc(sizeof(CYLINDER*)*floppydisk->floppyNumberOfTrack);
			
	rpm=300; // normal rpm
	
	floppycontext->hxc_printf(MSG_INFO_1,"VDK File : VDK version: 0x%.2x,Source ID: 0x%.2x, Source Version: 0x%.2x, Flags: 0x%.2x",vdk_h.version,vdk_h.file_source_id,vdk_h.file_source_ver,vdk_h.flags);			
	floppycontext->hxc_printf(MSG_INFO_1,"%d tracks, %d side(s), %d sectors/track, rpm:%d",floppydisk->floppyNumberOfTrack,floppydisk->floppyNumberOfSide,floppydisk->floppySectorPerTrack,rpm);
							
	sectorsize=256;
	gap3len=24;
	interleave=2;

	tracklen=(DEFAULT_DD_BITRATE/(rpm/60))/4;
	filetracklen=sectorsize*floppydisk->floppySectorPerTrack;
	trackdata=(unsigned char*)malloc(filetracklen);


	for(j=0;j<floppydisk->floppyNumberOfTrack;j++)
	{
				
		floppydisk->tracks[j]=(CYLINDER*)malloc(sizeof(CYLINDER));
		currentcylinder=floppydisk->tracks[j];
		currentcylinder->number_of_side=floppydisk->floppyNumberOfSide;
		currentcylinder->sides=(SIDE**)malloc(sizeof(SIDE*)*currentcylinder->number_of_side);
		memset(currentcylinder->sides,0,sizeof(SIDE*)*currentcylinder->number_of_side);
				
		for(i=0;i<floppydisk->floppyNumberOfSide;i++)
		{
					
			currentcylinder->floppyRPM=rpm;
					
			currentcylinder->sides[i]=malloc(sizeof(SIDE));
			memset(currentcylinder->sides[i],0,sizeof(SIDE));
			currentside=currentcylinder->sides[i];
					
			currentside->number_of_sector=floppydisk->floppySectorPerTrack;
			currentside->tracklen=tracklen;

			currentside->databuffer=malloc(currentside->tracklen);
			memset(currentside->databuffer,0,currentside->tracklen);
					
			currentside->flakybitsbuffer=0;
				
			currentside->timingbuffer=0;
			currentside->bitrate=DEFAULT_DD_BITRATE;
			currentside->track_encoding=ISOIBM_MFM_ENCODING;

			currentside->indexbuffer=malloc(currentside->tracklen);
			memset(currentside->indexbuffer,0,currentside->tracklen);						

			file_offset=((filetracklen*j*floppydisk->floppyNumberOfSide)+(filetracklen*(i&1)))+vdk_h.header_size;					
			fseek (f , file_offset , SEEK_SET);
					
			fread(trackdata,filetracklen,1,f);

			BuildISOTrack(floppycontext,ISOFORMAT_DD,currentside->number_of_sector,1,sectorsize,j,i,gap3len,trackdata,currentside->databuffer,&currentside->tracklen,interleave,0,NULL);			

			currentside->tracklen=currentside->tracklen*8;

			fillindex(currentside->tracklen-1,currentside,2500,TRUE,1);

		}

	}
		
	free(trackdata);
		
	floppycontext->hxc_printf(MSG_INFO_1,"track file successfully loaded and encoded!");
		
	fclose(f);
	return LOADER_NOERROR;
}
