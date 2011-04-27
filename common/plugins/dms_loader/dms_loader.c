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
// File : DMS_DiskFile.c
// Contains: DMS floppy image loader and plugins interfaces
//
// Written by:	DEL NERO Jean Francois
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "hxc_floppy_emulator.h"
#include "internal_floppy.h"
#include "floppy_loader.h"
#include "floppy_utils.h"

#include "../common/amiga_track.h"

#include "dms_loader.h"

#include "./libs/xdms/vfile.h"

#include "./libs/xdms/xdms-1.3.2/src/cdata.h"
#include "./libs/xdms/xdms-1.3.2/src/pfile.h"
#include "./libs/xdms/xdms-1.3.2/src/crc_csum.h"

#include "../common/os_api.h"

int DMS_libIsValidDiskFile(HXCFLOPPYEMULATOR* floppycontext,char * imgfile)
{
	int pathlen;
	char * filepath;
	floppycontext->hxc_printf(MSG_DEBUG,"DMS_libIsValidDiskFile %s",imgfile);
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

				if(strstr( filepath,".dms" )!=NULL)
				{
					floppycontext->hxc_printf(MSG_DEBUG,"DMS file !");
					free(filepath);
					return LOADER_ISVALID;
				}
				else
				{
					floppycontext->hxc_printf(MSG_DEBUG,"non DMS file !");
					free(filepath);
					return LOADER_BADFILE;
				}
			}
		}
	}

	return LOADER_BADPARAMETER;
}



int DMS_libLoad_DiskFile(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	unsigned int filesize;
	unsigned int i,j,k;
	unsigned int file_offset;
	char* trackdata;
	int tracklen;
	HXCFILE * fo;
	char* flatimg;
	int retxdms;
	CYLINDER* currentcylinder;
	SIDE* currentside;
	
	
	
	floppycontext->hxc_printf(MSG_DEBUG,"DMS_libLoad_DiskFile %s",imgfile);
	
	
	// ouverture et decompression du fichier dms
	fo=HXC_fopen("","");
	retxdms=Process_File(imgfile,fo, CMD_UNPACK, 0, 0, 0);
	if(retxdms)
	{
		floppycontext->hxc_printf(MSG_ERROR,"XDMS: Error %d while reading the file!",retxdms);
		HXC_fclose(fo);
		return LOADER_ACCESSERROR;
	}
	
	
	filesize=fo->buffersize;
	flatimg=fo->buffer;	
	
	if(fo!=0)
	{		
		floppydisk->floppySectorPerTrack=11;
		floppydisk->floppyNumberOfSide=2;
		floppydisk->floppyNumberOfTrack=(filesize/(512*2*11));
		floppydisk->floppyBitRate=250000;
		floppydisk->floppyiftype=AMIGA_DD_FLOPPYMODE;
		floppydisk->tracks=(CYLINDER**)malloc(sizeof(CYLINDER*)*floppydisk->floppyNumberOfTrack);
		
		tracklen=(DEFAULT_AMIGA_BITRATE/(DEFAULT_AMIGA_RPM/60))/4;
		trackdata=(unsigned char*)malloc(512*floppydisk->floppySectorPerTrack);
		
		for(j=0;j<floppydisk->floppyNumberOfTrack;j++)
		{

			floppydisk->tracks[j]=allocCylinderEntry(DEFAULT_AMIGA_RPM,floppydisk->floppyNumberOfSide);
			currentcylinder=floppydisk->tracks[j];

			for(i=0;i<floppydisk->floppyNumberOfSide;i++)
			{	
				currentcylinder->sides[i]=malloc(sizeof(SIDE));
				currentside=currentcylinder->sides[i];
				
				memset(currentside,0,sizeof(SIDE));
				
				currentside->number_of_sector=floppydisk->floppySectorPerTrack;
				currentside->tracklen=tracklen;
				
				currentside->databuffer=malloc(currentside->tracklen);
				memset(currentside->databuffer,0,currentside->tracklen);
				
				currentside->flakybitsbuffer=0;
				
				currentside->indexbuffer=malloc(currentside->tracklen);
				memset(currentside->indexbuffer,0,currentside->tracklen);
				
				for(k=currentside->tracklen-710;k<currentside->tracklen;k++)
				{
					currentside->indexbuffer[k]=0xFF;
				}
				
				currentside->timingbuffer=0;
				currentside->bitrate=DEFAULT_AMIGA_BITRATE;
				currentside->track_encoding=AMIGA_MFM_ENCODING;	
				
				file_offset=(512*(j*currentside->number_of_sector*2))+
					(512*(currentside->number_of_sector)*i);
				
				memcpy(trackdata,&flatimg[file_offset],512*currentside->number_of_sector);
				
				BuildAmigaTrack(trackdata,currentside->databuffer,tracklen,j,i,11);
				
				currentside->tracklen=currentside->tracklen*8;
				
			}
		}

		free(trackdata);
		
		floppycontext->hxc_printf(MSG_INFO_1,"DMS Loader : tracks file successfully loaded and encoded!");
		HXC_fclose(fo);
		return LOADER_NOERROR;
	}
	
	floppycontext->hxc_printf(MSG_ERROR,"DMS file access error!");
	return LOADER_ACCESSERROR;
}
