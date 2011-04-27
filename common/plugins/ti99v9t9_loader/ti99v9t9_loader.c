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
// File : ti99v9t9_loader.c
// Contains: TI99 v9t9 floppy image loader and plugins interfaces
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

#include "ti99v9t9_loader.h"

#include "../common/os_api.h"


int TI99V9T9_libIsValidDiskFile(HXCFLOPPYEMULATOR* floppycontext,char * imgfile)
{
	int filesize;
	FILE * f;
	char * filepath;

	floppycontext->hxc_printf(MSG_DEBUG,"TI99V9T9_libIsValidDiskFile %s",imgfile);
	if(imgfile)
	{

		filepath=malloc(strlen(imgfile)+1);
		if(filepath!=0)
		{
			sprintf(filepath,"%s",imgfile);
			strlower(filepath);
				
			if(strstr( filepath,".v9t9" )!=NULL || strstr( filepath,".pc99" )!=NULL)
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

				if(filesize!=92160 && filesize!=92160*2 )//&& filesize!=92160*4)
				{
					floppycontext->hxc_printf(MSG_DEBUG,"non TI99 V9T9 file - bad file size !");
					return LOADER_BADFILE;
				}

				floppycontext->hxc_printf(MSG_DEBUG,"V9T9 file !");
				free(filepath);
				return LOADER_ISVALID;
			}
			else
			{
				floppycontext->hxc_printf(MSG_DEBUG,"non V9T9 file !");
				free(filepath);
				return LOADER_BADFILE;
			}
		}
		
		floppycontext->hxc_printf(MSG_DEBUG,"Internal error!");
		return LOADER_INTERNALERROR;

	}
	return LOADER_BADPARAMETER;
}


int TI99V9T9_libLoad_DiskFile(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * floppydisk,char * imgfile,void * parameters)
{

	FILE * f;
	unsigned int filesize,skew,interleave;
	unsigned int i,j,sectorsize;
	unsigned int file_offset,gap3len;
	char* trackdata;

	int rpm;
	int fmmode,tracklen,numberoftrack,numberofside;
	CYLINDER* currentcylinder;
	SIDE* currentside;


	floppycontext->hxc_printf(MSG_DEBUG,"TI99V9T9_libLoad_DiskFile %s",imgfile);

	f=fopen(imgfile,"rb");
	if(f==NULL)
	{
		floppycontext->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return LOADER_ACCESSERROR;
	}

	fseek (f , 0 , SEEK_END);
	filesize=ftell(f);
	fseek (f , 0 , SEEK_SET);

	if(filesize!=0)
	{		
			
		fmmode=0;

		if(filesize!=92160 && filesize!=92160*2 && filesize!=92160*4)
		{
			floppycontext->hxc_printf(MSG_DEBUG,"non TI99 V9T9 file !");
			return LOADER_BADFILE;
		}

		switch(filesize)
		{
		case 92160:
			numberofside=1;
			numberoftrack=40;
			floppydisk->floppyBitRate=250000;
			break;

		case 92160*2:
			numberofside=2;
			numberoftrack=40;
			floppydisk->floppyBitRate=250000;
			break;

		case 92160*4:
			tracklen=6872;
			fmmode=0;
			numberofside=2;
			numberoftrack=40;
			floppydisk->floppyBitRate=250000;
			break;
		}

		
		sectorsize=256;
		floppydisk->floppyNumberOfTrack=numberoftrack;
		floppydisk->floppyNumberOfSide=numberofside;
		floppydisk->floppySectorPerTrack=9;
		floppydisk->floppyiftype=GENERIC_SHUGART_DD_FLOPPYMODE;
		rpm=300; // normal rpm
		fmmode=1;
		tracklen=3253;
		skew=6;
		gap3len=45;
		interleave=4;

			
		floppydisk->tracks=(CYLINDER**)malloc(sizeof(CYLINDER*)*floppydisk->floppyNumberOfTrack);			
			
		floppycontext->hxc_printf(MSG_INFO_1,"filesize:%dkB, %d tracks, %d side(s), %d sectors/track, rpm:%d",filesize/1024,floppydisk->floppyNumberOfTrack,floppydisk->floppyNumberOfSide,floppydisk->floppySectorPerTrack,rpm);

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
				if(fmmode)
				{	
					currentside->track_encoding=ISOIBM_FM_ENCODING;
					currentside->tracklen=tracklen*4;
				}
				else
				{
					currentside->track_encoding=ISOIBM_MFM_ENCODING;
					currentside->tracklen=tracklen*2;
				}

				currentside->databuffer=malloc(currentside->tracklen);
				memset(currentside->databuffer,0,currentside->tracklen);
					
				currentside->flakybitsbuffer=0;
				
				currentside->timingbuffer=0;
				currentside->bitrate=floppydisk->floppyBitRate;

				currentside->indexbuffer=malloc(currentside->tracklen);
				memset(currentside->indexbuffer,0,currentside->tracklen);						

				file_offset=(sectorsize*(j*currentside->number_of_sector*floppydisk->floppyNumberOfSide))+
					(sectorsize*(currentside->number_of_sector)*i);
					
				fseek (f , file_offset , SEEK_SET);
					
				fread(trackdata,sectorsize*currentside->number_of_sector,1,f);
					
				if(fmmode)
				{
					BuildISOTrack(floppycontext,ISOFORMAT_SD,currentside->number_of_sector,0,sectorsize,j,i,gap3len,trackdata,currentside->databuffer,&currentside->tracklen,interleave,j*skew,NULL);
				}
				else
				{
					BuildISOTrack(floppycontext,ISOFORMAT_DD,currentside->number_of_sector,0,sectorsize,j,i,gap3len,trackdata,currentside->databuffer,&currentside->tracklen,interleave,j*skew,NULL);
				}

				currentside->tracklen=currentside->tracklen*8;

				fillindex(currentside->tracklen-1,currentside,2500,TRUE,1);

			}

		}
		
		free(trackdata);
		
		floppycontext->hxc_printf(MSG_INFO_1,"track file successfully loaded and encoded!");
		
		fclose(f);
		return LOADER_NOERROR;
	}

	floppycontext->hxc_printf(MSG_ERROR,"file size=%d !?",filesize);
	fclose(f);
	return LOADER_BADFILE;
}
