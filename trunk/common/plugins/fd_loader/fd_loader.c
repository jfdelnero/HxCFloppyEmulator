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
// File : fd_DiskFile.c
// Contains: ST floppy image loader and plugins interfaces
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

#include "fd_loader.h"

#include "../common/os_api.h"




int FD_libIsValidDiskFile(HXCFLOPPYEMULATOR* floppycontext,char * imgfile)
{
	int pathlen;
	char * filepath;
	FILE * f;
	unsigned int filesize;

	floppycontext->hxc_printf(MSG_DEBUG,"fd_libIsValidDiskFile %s",imgfile);
	if(imgfile)
	{
		pathlen=strlen(imgfile);
		if(pathlen!=0)
		{
			filepath=malloc(pathlen+1);
			if(filepath!=0)
			{
				sprintf(filepath,"%s",imgfile);
				strlower(filepath);
				
				if(strstr( filepath,".fd" )!=NULL)
				{

					f=fopen(imgfile,"rb");
					if(f==NULL) 
					{
						floppycontext->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
						return LOADER_ACCESSERROR;
					}
					
					fseek (f , 0 , SEEK_END); 
					filesize=ftell(f);
					fseek (f , 0 , SEEK_SET); 
					fclose(f);

					if((filesize==327680) || (filesize==655360))
					{
						floppycontext->hxc_printf(MSG_DEBUG,"TO8D FD file !");
						free(filepath);
						return LOADER_ISVALID;
					}
					else
					{
						floppycontext->hxc_printf(MSG_DEBUG,"non TO8D FD file ! - bad file size! ");
						free(filepath);
						return LOADER_BADFILE;
					}
				}
				else
				{
					floppycontext->hxc_printf(MSG_DEBUG,"non TO8D FD file !");
					free(filepath);
					return LOADER_BADFILE;
				}
			}
		}
	}
	
	return LOADER_BADPARAMETER;
}



int FD_libLoad_DiskFile(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	FILE * f;
	unsigned int filesize;
	unsigned int i,j;
	unsigned int file_offset;
	char* trackdata;
	int tracklen;
	int gap3len,interleave,rpm;
	int sectorsize;
	CYLINDER* currentcylinder;
	SIDE* currentside;
	
	floppycontext->hxc_printf(MSG_DEBUG,"fd_libLoad_DiskFile %s",imgfile);
	
	f=fopen(imgfile,"rb");
	if(f==NULL) 
	{
		floppycontext->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return LOADER_ACCESSERROR;
	}
	
	fseek (f , 0 , SEEK_END); 
	filesize=ftell(f);
	fseek (f , 0 , SEEK_SET); 
	
	sectorsize=256; // st file support only 512bytes/sector floppies.
	// read the first sector
	switch(filesize)
	{
		case 327680:
			floppydisk->floppyNumberOfSide=1;
		break;
		case 655360:
			floppydisk->floppyNumberOfSide=2;
		break;
		default:
			floppycontext->hxc_printf(MSG_DEBUG,"non TO8D FD file ! - bad file size! ");
			fclose(f);
			return LOADER_BADFILE;
		break;
	}
	
	gap3len=50;
	interleave=1;
	floppydisk->floppyNumberOfTrack=80;
	floppydisk->floppySectorPerTrack=16;
	floppydisk->floppyBitRate=250000;
	floppydisk->floppyiftype=GENERIC_SHUGART_DD_FLOPPYMODE;
	floppydisk->tracks=(CYLINDER**)malloc(sizeof(CYLINDER*)*floppydisk->floppyNumberOfTrack);
	rpm=300; // normal rpm
			
	floppycontext->hxc_printf(MSG_INFO_1,"filesize:%dkB, %d tracks, %d side(s), %d sectors/track, gap3:%d, interleave:%d,rpm:%d",filesize/1024,floppydisk->floppyNumberOfTrack,floppydisk->floppyNumberOfSide,floppydisk->floppySectorPerTrack,gap3len,interleave,rpm);
				
	tracklen=(DEFAULT_DD_BITRATE/(rpm/60))/4;
	trackdata=(unsigned char*)malloc(sectorsize*floppydisk->floppySectorPerTrack);
			
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

			file_offset=(sectorsize*(j*currentside->number_of_sector))+
				(sectorsize*currentside->number_of_sector*floppydisk->floppyNumberOfTrack*i);
					
			fseek (f , file_offset , SEEK_SET);
					
			fread(trackdata,sectorsize*currentside->number_of_sector,1,f);
					
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
