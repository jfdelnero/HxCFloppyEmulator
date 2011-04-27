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
// File : RAW_DiskFile.c
// Contains: RAW floppy image loader and plugins interfaces
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

#include "raw_loader.h"



int RAW_libIsValidFormat(HXCFLOPPYEMULATOR* floppycontext,cfgrawfile * imgformatcfg)
{
	unsigned int tracktype;
	int tracklen;
	int gap3len,interleave,rpm,bitrate;
	int sectorsize;
	int finaltracksize;
	int tracklendiv;
	

	switch(imgformatcfg->sectorsize)
	{
		case SECTORSIZE_128:
			sectorsize=128;
		break;
		case SECTORSIZE_256:
			sectorsize=256;
		break;
		case SECTORSIZE_512:
			sectorsize=512;
		break;
		case SECTORSIZE_1024:
			sectorsize=1024;
		break;
		case SECTORSIZE_2048:
			sectorsize=2048;
		break;
		case SECTORSIZE_4096:
			sectorsize=4096;
		break;
		case SECTORSIZE_8192:
			sectorsize=8192;
		break;
		case SECTORSIZE_16384:
			sectorsize=16384;
		break;
	}


	gap3len=imgformatcfg->gap3;
	rpm=imgformatcfg->rpm;
	bitrate=imgformatcfg->bitrate;
	interleave=imgformatcfg->interleave;

	switch(imgformatcfg->tracktype)
	{
		case FM_TRACK_TYPE:
			tracktype=ISOFORMAT_SD;
			tracklendiv=16;
		break;

		case FMIBM_TRACK_TYPE:
			tracktype=IBMFORMAT_SD;
			tracklendiv=16;
		break;

		case MFM_TRACK_TYPE:
			tracktype=ISOFORMAT_DD;
			tracklendiv=8;
		break;

		case MFMIBM_TRACK_TYPE:
			tracktype=IBMFORMAT_DD;
			tracklendiv=8;
		break;

		case GCR_TRACK_TYPE:
			tracktype=0;
		break;
	};

	if(rpm==0) 
		return LOADER_BADPARAMETER;


	tracklen=((bitrate*60)/rpm)/tracklendiv;

	finaltracksize=ISOIBMGetTrackSize(tracktype,imgformatcfg->sectorpertrack,sectorsize,gap3len,0);
	
	if(finaltracksize<=tracklen)
	{
		return LOADER_ISVALID;
	}
	else
	{
		return LOADER_BADPARAMETER;
	}
}



int RAW_libLoad_DiskFile(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * floppydisk,char * imgfile,cfgrawfile * imgformatcfg)
{
	
	FILE * f;
	unsigned int filesize;
	unsigned int i,j,fileside;
	unsigned int file_offset,tracktype;
	char* trackdata;
	int tracklen;
	int gap3len,interleave,rpm,bitrate,skew;
	int sectorsize;
	CYLINDER* currentcylinder;
	SIDE* currentside;
	
	f=0;
	
	if(imgfile)
	{
		floppycontext->hxc_printf(MSG_DEBUG,"RAW_libLoad_DiskFile %s",imgfile);
		
		f=fopen(imgfile,"rb");
		if(f==NULL) 
		{
			floppycontext->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
			return -1;
		}
		
		fseek (f , 0 , SEEK_END); 
		filesize=ftell(f);
		fseek (f , 0 , SEEK_SET); 
	}
	else
	{
		floppycontext->hxc_printf(MSG_DEBUG,"RAW_libLoad_DiskFile empty floppy");
	}	

	
	switch(imgformatcfg->sectorsize)
	{
		case SECTORSIZE_128:
			sectorsize=128;
		break;
		case SECTORSIZE_256:
			sectorsize=256;
		break;
		case SECTORSIZE_512:
			sectorsize=512;
		break;
		case SECTORSIZE_1024:
			sectorsize=1024;
		break;
		case SECTORSIZE_2048:
			sectorsize=2048;
		break;
		case SECTORSIZE_4096:
			sectorsize=4096;
		break;
		case SECTORSIZE_8192:
			sectorsize=8192;
		break;
		case SECTORSIZE_16384:
			sectorsize=16384;
		break;
	}


	gap3len=imgformatcfg->gap3;
	rpm=imgformatcfg->rpm;
	bitrate=imgformatcfg->bitrate;
	interleave=imgformatcfg->interleave;
	skew=imgformatcfg->skew;

	floppydisk->floppyNumberOfTrack=imgformatcfg->numberoftrack;
	if((imgformatcfg->sidecfg&TWOSIDESFLOPPY) || (imgformatcfg->sidecfg&SIDE_INVERTED))
	{
		floppydisk->floppyNumberOfSide=2;
	}
	else
	{
		floppydisk->floppyNumberOfSide=1;
	}
	floppydisk->floppySectorPerTrack=imgformatcfg->sectorpertrack;

	floppydisk->floppyBitRate=bitrate;
	floppydisk->floppyiftype=GENERIC_SHUGART_DD_FLOPPYMODE;
	floppydisk->tracks=(CYLINDER**)malloc(sizeof(CYLINDER*)*floppydisk->floppyNumberOfTrack);

	floppycontext->hxc_printf(MSG_DEBUG,"%d bytes sectors, %d sectors/tracks,interleaving %d, skew %d, %d tracks, %d side(s), gap3 %d, %d rpm, %d bits/s",
		sectorsize,
		floppydisk->floppySectorPerTrack,
		interleave,
		skew,
		floppydisk->floppyNumberOfTrack,
		floppydisk->floppyNumberOfSide,
		gap3len,
		rpm,
		bitrate
		);
	switch(imgformatcfg->tracktype)
	{
		case FM_TRACK_TYPE:
			tracktype=ISOFORMAT_SD;
			floppycontext->hxc_printf(MSG_DEBUG,"FM ISO tracks format");
		break;

		case FMIBM_TRACK_TYPE:
			tracktype=IBMFORMAT_SD;
			floppycontext->hxc_printf(MSG_DEBUG,"FM IBM tracks format");
		break;

		case MFM_TRACK_TYPE:
			tracktype=ISOFORMAT_DD;
			floppycontext->hxc_printf(MSG_DEBUG,"MFM ISO tracks format");
		break;

		case MFMIBM_TRACK_TYPE:
			tracktype=IBMFORMAT_DD;
			floppycontext->hxc_printf(MSG_DEBUG,"MFM IBM tracks format");
		break;

		case GCR_TRACK_TYPE:
			tracktype=0;
			floppycontext->hxc_printf(MSG_DEBUG,"GCR tracks format");
		break;
	};

	tracklen=((bitrate*60)/rpm)/4;
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
			currentside->bitrate=imgformatcfg->bitrate;
					
			currentside->indexbuffer=malloc(currentside->tracklen);
			memset(currentside->indexbuffer,0,currentside->tracklen);			


			if(imgformatcfg->sidecfg&TWOSIDESFLOPPY)
			{
				if(imgformatcfg->sidecfg&SIDE_INVERTED)
				{
					fileside=i^0x1;
				}
				else
				{
					fileside=i;
				}
			}
			else
			{
				fileside=0;
			}


			if(tracktype==IBMFORMAT_DD || tracktype==ISOFORMAT_DD)
				currentside->track_encoding=ISOIBM_MFM_ENCODING;
			else
				currentside->track_encoding=ISOIBM_FM_ENCODING;


			if(imgformatcfg->sidecfg&SIDE0_FIRST)
			{
				file_offset=(sectorsize*(j*currentside->number_of_sector))+
							(sectorsize*currentside->number_of_sector*floppydisk->floppyNumberOfTrack*fileside);
			}
			else
			{
				file_offset=(sectorsize*(j*currentside->number_of_sector*floppydisk->floppyNumberOfSide))+
							(sectorsize*(currentside->number_of_sector)*fileside);
			}
			
			if(f)
			{
				fseek (f , file_offset , SEEK_SET);
					
				floppycontext->hxc_printf(MSG_DEBUG,"Track %d Head %d : Reading %d bytes at %.8X",j,i,sectorsize*currentside->number_of_sector,file_offset);
				fread(trackdata,sectorsize*currentside->number_of_sector,1,f);
				
			}
			else
			{
				memset(trackdata,0,sectorsize*currentside->number_of_sector);
			}
					
			if(tracktype)
			{
				BuildISOTrack(floppycontext,tracktype,currentside->number_of_sector,imgformatcfg->firstidsector,sectorsize,j,i,gap3len,trackdata,currentside->databuffer,&currentside->tracklen,interleave,(((j<<1)|(i&1))*skew),NULL);
			}
			else
			{

			}

			currentside->tracklen=currentside->tracklen*8;

			fillindex(currentside->tracklen-1,currentside,2500,TRUE,1);
		}
	}

	free(trackdata);
	if(f) fclose(f);

	floppycontext->hxc_printf(MSG_INFO_1,"track file successfully loaded and encoded!");



	return LOADER_NOERROR;
}
