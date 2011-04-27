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
// File : D88_loader.c
// Contains: D88 floppy image loader.
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

#include "apridisk_loader.h"
#include "apridisk_format.h"

int ApriDisk_libIsValidDiskFile(HXCFLOPPYEMULATOR* floppycontext,char * imgfile)
{
	unsigned char HeaderBuffer[128];
	FILE * f;
	int pathlen;

	floppycontext->hxc_printf(MSG_DEBUG,"ApriDisk_libIsValidDiskFile %s",imgfile);

	if(imgfile)
	{
		pathlen=strlen(imgfile);
		if(pathlen!=0)
		{

			f=fopen(imgfile,"r+b");
			if(f)
			{
				fread(HeaderBuffer,1,128,f);
				fclose(f);

				if(!strcmp(APRIDISK_HeaderString,HeaderBuffer))
				{
					floppycontext->hxc_printf(MSG_DEBUG,"ApriDisk file !");
					return LOADER_ISVALID;
				}
				else
				{
					floppycontext->hxc_printf(MSG_DEBUG,"ApriDisk file !");
					return LOADER_BADFILE;
				}
			}
		}
	}

	return LOADER_BADPARAMETER;
}



int ApriDisk_libLoad_DiskFile(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	FILE * f;
	apridisk_data_record * data_record;
	apridisk_compressed_data * compressed_dataitem;
	unsigned int i,j,rpm;
	SECTORCONFIG* sectorconfig;
	CYLINDER* currentcylinder;
	SIDE* currentside;
	
	int gap3len;
	int tracklen,interleave;
	
	unsigned char* trackdatatemp;
	int trackdatatempsize;
	
	int number_of_track,number_of_sector;
	int totalfilesize;
	unsigned char * file_buffer;
	int fileindex,newtrack;
	
	floppycontext->hxc_printf(MSG_DEBUG,"ApriDisk_libLoad_DiskFile %s",imgfile);
	
	f=fopen(imgfile,"rb");
	if(f==NULL) 
	{
		floppycontext->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return LOADER_ACCESSERROR;
	}
	
	fseek(f,0,SEEK_END);
	totalfilesize=ftell(f);
	fseek(f,0,SEEK_SET);
	file_buffer=(unsigned char *) malloc(totalfilesize);
	memset(file_buffer,0,totalfilesize);
	fread(file_buffer,1,totalfilesize,f);
	
	fileindex=0;
	//////////////////////////////////////////////////////
	// Header check
	if(strcmp(APRIDISK_HeaderString,&file_buffer[fileindex]))
	{
		floppycontext->hxc_printf(MSG_DEBUG,"ApriDisk file !");
		return LOADER_BADFILE;
	}
	
	number_of_track=0;
	i=0;
	
	floppydisk->floppyBitRate=500000;
	floppydisk->floppyiftype=GENERIC_SHUGART_DD_FLOPPYMODE;
	floppydisk->floppyNumberOfTrack=0;
	floppydisk->floppyNumberOfSide=0;
	floppydisk->floppySectorPerTrack=-1; // default value
	
	for(i=0;i<128;i++)
	{
		
		for(j=0;j<=1;j++)
		{
			number_of_sector=0;
			trackdatatemp=0;
			trackdatatempsize=0;
			sectorconfig=0;
			fileindex=128;
			gap3len=80;
			rpm=600;
			newtrack=0;
			interleave=1;

			do
			{
				
				data_record=(apridisk_data_record *)&file_buffer[fileindex];
				fileindex=fileindex+sizeof(apridisk_data_record);
				
				switch(data_record->item_type)
				{
				case DATA_RECORD_DELETED:
					if(data_record->header_size>sizeof(apridisk_data_record))
					{
						fileindex=fileindex+(data_record->header_size-sizeof(apridisk_data_record));
					}
					fileindex=fileindex+(data_record->data_size);
					break;
					
				case DATA_RECORD_SECTOR:
					if(data_record->header_size>sizeof(apridisk_data_record))
					{
						fileindex=fileindex+(data_record->header_size-sizeof(apridisk_data_record));
					}
					if((data_record->cylinder==i) && (data_record->head==j))
					{
						
						if((j+1)>(floppydisk->floppyNumberOfSide))
						{
							floppydisk->floppyNumberOfSide=j+1;
						}
						
						if((i+1)>(floppydisk->floppyNumberOfTrack))
						{
							floppydisk->floppyNumberOfTrack=i+1;
							newtrack=1;
						}
						
						data_record->item_type=DATA_RECORD_DELETED;
						
						
						sectorconfig=(SECTORCONFIG*)realloc(sectorconfig,sizeof(SECTORCONFIG)*(number_of_sector+1));
						memset(&sectorconfig[number_of_sector],0,sizeof(SECTORCONFIG));
						
						sectorconfig[number_of_sector].cylinder=i;
						sectorconfig[number_of_sector].head=j;
						sectorconfig[number_of_sector].sector=data_record->sector;
						switch(data_record->compression)
						{
						case DATA_NOT_COMPRESSED:
							sectorconfig[number_of_sector].sectorsize=data_record->data_size;
							
							trackdatatemp=realloc(trackdatatemp,trackdatatempsize+data_record->data_size);
							
							memcpy(&trackdatatemp[trackdatatempsize],&file_buffer[fileindex],data_record->data_size);
							fileindex=fileindex+data_record->data_size;
							
							trackdatatempsize=trackdatatempsize+data_record->data_size;
							break;
							
						case DATA_COMPRESSED:
							compressed_dataitem=(apridisk_compressed_data *)&file_buffer[fileindex];
							
							sectorconfig[number_of_sector].sectorsize=compressed_dataitem->count;
							
							trackdatatemp=realloc(trackdatatemp,trackdatatempsize+compressed_dataitem->count);
							
							memset(&trackdatatemp[trackdatatempsize],compressed_dataitem->byte,compressed_dataitem->count);
							fileindex=fileindex+data_record->data_size;
							trackdatatempsize=trackdatatempsize+compressed_dataitem->count;
							
							break;
							
						default:
							floppycontext->hxc_printf(MSG_ERROR,"Unknow compression id (%.4x) !",data_record->compression);
							break;
						}
					
						number_of_sector++;
						
					}
					else
					{
						fileindex=fileindex+data_record->data_size;
					}
					
					floppycontext->hxc_printf(MSG_DEBUG,"ApriDisk_libLoad_DiskFile: item DATA_RECORD_SECTOR found. Header size=%d, Data size=%d, Sector=%d Head=%d Cylinder=%d",data_record->header_size,data_record->data_size,data_record->sector,data_record->head,data_record->cylinder);
					break;
					
				case DATA_RECORD_COMMENT:
					if(data_record->header_size>sizeof(apridisk_data_record))
					{
						fileindex=fileindex+(data_record->header_size-sizeof(apridisk_data_record));
					}
					fileindex=fileindex+data_record->data_size;
					break;
					
				case DATA_RECORD_CREATOR:
					
					if(data_record->header_size>sizeof(apridisk_data_record))
					{
						fileindex=fileindex+(data_record->header_size-sizeof(apridisk_data_record));
					}
					fileindex=fileindex+data_record->data_size;					
					break;

				default:
					return LOADER_BADFILE;
					break;
					
			}
			
		}while(fileindex<totalfilesize);
		
		if(number_of_sector)
		{
			if(newtrack)
			{
				floppydisk->tracks=(CYLINDER**)realloc(floppydisk->tracks,sizeof(CYLINDER*)*floppydisk->floppyNumberOfTrack);
				floppydisk->tracks[i]=(CYLINDER*)malloc(sizeof(CYLINDER));
				memset(floppydisk->tracks[i],0,sizeof(CYLINDER));
			}

			currentcylinder=floppydisk->tracks[floppydisk->floppyNumberOfTrack-1];
			currentcylinder->number_of_side=floppydisk->floppyNumberOfSide;
			currentcylinder->sides=(SIDE**)realloc(currentcylinder->sides,sizeof(SIDE*)*currentcylinder->number_of_side);
			//memset(currentcylinder->sides,0,sizeof(SIDE*)*currentcylinder->number_of_side);
			
			currentcylinder->floppyRPM=rpm;
			currentcylinder->sides[j]=malloc(sizeof(SIDE));
			currentside=currentcylinder->sides[j];
			memset(currentcylinder->sides[j],0,sizeof(SIDE));
			
			tracklen=(floppydisk->floppyBitRate/(rpm/60))/4;
			currentside->number_of_sector=number_of_sector;
			currentside->tracklen=tracklen;
			
			currentside->databuffer=malloc(tracklen);
			memset(currentside->databuffer,0,tracklen);
			
			currentside->flakybitsbuffer=0;
			
			currentside->timingbuffer=0;
			currentside->bitrate=DEFAULT_DD_BITRATE;
			currentside->track_encoding=ISOIBM_MFM_ENCODING;
			
			currentside->indexbuffer=malloc(tracklen);
			memset(currentside->indexbuffer,0,tracklen);
			
			while((gap3len!=0) && ((int)(currentside->tracklen/2)<ISOIBMGetTrackSize(IBMFORMAT_DD,number_of_sector,512,gap3len,sectorconfig)))
			{
				gap3len--;
			};
			
			
			BuildISOTrack(floppycontext,
				IBMFORMAT_DD,
				currentside->number_of_sector,
				1,
				512,
				i,
				j,
				gap3len,
				trackdatatemp,
				currentside->databuffer,
				&(currentside->tracklen),
				interleave,
				0,
				sectorconfig);
			
			free(sectorconfig);

			currentside->tracklen=currentside->tracklen*8;

			fillindex(currentside->tracklen-1,currentside,2500,TRUE,1);

			}
		
		}
		
		free(trackdatatemp);
		trackdatatemp=0;
		trackdatatempsize=0;
		number_of_sector=0;
	}
	

	// Comment & creator extraction.
	fileindex=128;
	do
	{
				
		data_record=(apridisk_data_record *)&file_buffer[fileindex];
		fileindex=fileindex+sizeof(apridisk_data_record);
				
		switch(data_record->item_type)
		{
			case DATA_RECORD_DELETED:
				if(data_record->header_size>sizeof(apridisk_data_record))
				{
					fileindex=fileindex+(data_record->header_size-sizeof(apridisk_data_record));
				}
				fileindex=fileindex+(data_record->data_size);
				break;
					
			case DATA_RECORD_SECTOR:
				if(data_record->header_size>sizeof(apridisk_data_record))
				{
					fileindex=fileindex+(data_record->header_size-sizeof(apridisk_data_record));
				}
				fileindex=fileindex+data_record->data_size;
				break;
					
			case DATA_RECORD_COMMENT:
				if(data_record->header_size>sizeof(apridisk_data_record))
				{
					fileindex=fileindex+(data_record->header_size-sizeof(apridisk_data_record));
				}
				floppycontext->hxc_printf(MSG_DEBUG,"ApriDisk_libLoad_DiskFile: item DATA_RECORD_COMMENT found: %s",&file_buffer[fileindex]);
				fileindex=fileindex+data_record->data_size;
				break;
					
			case DATA_RECORD_CREATOR:
				if(data_record->header_size>sizeof(apridisk_data_record))
				{
					fileindex=fileindex+(data_record->header_size-sizeof(apridisk_data_record));
				}
				floppycontext->hxc_printf(MSG_DEBUG,"ApriDisk_libLoad_DiskFile: item DATA_RECORD_CREATOR found: %s",&file_buffer[fileindex]);
				fileindex=fileindex+data_record->data_size;
				break;

				default:
				return LOADER_BADFILE;
				break;
					
			}
			
		}while(fileindex<totalfilesize);



	free(file_buffer);
	
	
	
	return LOADER_NOERROR;
}

