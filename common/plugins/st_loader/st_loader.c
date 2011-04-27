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
// File : ST_DiskFile.c
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

#include "st_loader.h"

#include "../common/os_api.h"

#include "stfileformat.h"

int getfloppyconfig(char * img,unsigned int filesize,int *numberoftrack,int *numberofside,unsigned int *numberofsectorpertrack,int *gap3len,int *interleave)
{
	int i;
	int nb_of_side,nb_of_track,nb_of_sector;
	int numberofsector;
	unsigned char * uimg;
	int conffound;
	
	uimg=(unsigned char *)img;
	
	conffound=0;
	if(uimg[0x18]<24 && uimg[0x18]>8)
	{
		
		*numberofsectorpertrack=uimg[0x18];
		*numberofside=uimg[0x1A];

		*gap3len=84;
		*interleave=1;

		switch(*numberofsectorpertrack)
		{
			case 9:
				*gap3len=84;
				*interleave=1;
			break;
			case 10:
				*gap3len=30;
				*interleave=1;
			break;
			case 11:
				*interleave=2;
				*gap3len=3;
			break;
		}
	
		numberofsector=uimg[0x13]+(uimg[0x14]*256);
		if(*numberofsectorpertrack && *numberofside )
		{
			*numberoftrack=(numberofsector/(*numberofsectorpertrack*(*numberofside)));
			
			if((unsigned int)((*numberofsectorpertrack) * (*numberoftrack) * (*numberofside) *512)==filesize)
			{
				conffound=1;
			}
		}
		
	}
	
	if(!conffound)
	{
		i=0;
		do
		{
			
			if(stfileformats[i].filesize==filesize)
			{
				*numberoftrack=stfileformats[i].numberoftrack;
				*numberofsectorpertrack=stfileformats[i].sectorpertrack;
				*numberofside=stfileformats[i].numberofside;
				*gap3len=stfileformats[i].gap3len;
				*interleave=stfileformats[i].interleave;
				conffound=1;
			}
			i++;
			
		}while(stfileformats[i].filesize!=0 && conffound==0);

		if(!conffound)
		{

			for(nb_of_side=1;nb_of_side<=2;nb_of_side++)
			{
				for(nb_of_track=0;nb_of_track<85;nb_of_track++)
				{
					for(nb_of_sector=8;nb_of_sector<=11;nb_of_sector++)
					{
						if(filesize==(unsigned int)(nb_of_side*nb_of_track*nb_of_sector*512))
						{
							*numberoftrack=nb_of_track;
							*numberofsectorpertrack=nb_of_sector;
							*numberofside=nb_of_side;

							*gap3len=84;
							*interleave=1;

							switch(*numberofsectorpertrack)
							{
								case 9:
									*gap3len=84;
									*interleave=1;
								break;
								case 10:
									*gap3len=30;
									*interleave=1;
								break;
								case 11:
									*interleave=2;
									*gap3len=3;
								break;
							}
							
							conffound=1;


							return conffound;
						}
					}					
				}
			}
		}
	}
	return conffound;
}


int ST_libIsValidDiskFile(HXCFLOPPYEMULATOR* floppycontext,char * imgfile)
{
	int pathlen,filesize;
	char * filepath;
	FILE * f;
	floppycontext->hxc_printf(MSG_DEBUG,"ST_libIsValidDiskFile %s",imgfile);
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
				
				if(strstr( filepath,".st" )!=NULL)
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
					

					if(filesize&0x1FF)
					{
						floppycontext->hxc_printf(MSG_DEBUG,"non ST IMG file - bad file size !");
						return LOADER_BADFILE;
					}

					floppycontext->hxc_printf(MSG_DEBUG,"ST file !");
					free(filepath);
					return LOADER_ISVALID;
				}
				else
				{
					floppycontext->hxc_printf(MSG_DEBUG,"non ST file !");
					free(filepath);
					return LOADER_BADFILE;
				}
			}
		}
	}
	
	return LOADER_BADPARAMETER;
}



int ST_libLoad_DiskFile(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	
	FILE * f;
	unsigned int filesize;
	unsigned int i,j,skew;
	unsigned int file_offset;
	char* trackdata;
	int tracklen;
	int gap3len,interleave,rpm;
	int sectorsize;
	CYLINDER* currentcylinder;
	SIDE* currentside;
	
	floppycontext->hxc_printf(MSG_DEBUG,"ST_libLoad_DiskFile %s",imgfile);
	
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
		
		sectorsize=512; // st file support only 512bytes/sector floppies.
		// read the first sector
		trackdata=(char*)malloc(sectorsize);
		fread(trackdata,sectorsize,1,f);
		if(getfloppyconfig(
			trackdata,
			filesize,
			&floppydisk->floppyNumberOfTrack,
			&floppydisk->floppyNumberOfSide,
			&floppydisk->floppySectorPerTrack,
			&gap3len,
			&interleave)==1
			)
		{
			
			free(trackdata);

			if(floppydisk->floppySectorPerTrack<15)
			{
				floppydisk->floppyBitRate=DEFAULT_DD_BITRATE;
				floppydisk->floppyiftype=ATARIST_DD_FLOPPYMODE;
				skew=2;
			}
			else
			{
				floppydisk->floppyiftype=ATARIST_HD_FLOPPYMODE;
				floppydisk->floppyBitRate=DEFAULT_HD_BITRATE;
				skew=4;
			}
			
			floppydisk->tracks=(CYLINDER**)malloc(sizeof(CYLINDER*)*floppydisk->floppyNumberOfTrack);
			
			rpm=300; // normal rpm
			
			floppycontext->hxc_printf(MSG_INFO_1,"filesize:%dkB, %d tracks, %d side(s), %d sectors/track, gap3:%d, interleave:%d,rpm:%d bitrate:%d",filesize/1024,floppydisk->floppyNumberOfTrack,floppydisk->floppyNumberOfSide,floppydisk->floppySectorPerTrack,gap3len,interleave,rpm,floppydisk->floppyBitRate);
				
			tracklen=(floppydisk->floppyBitRate/(rpm/60))/4;
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

					if(currentside->number_of_sector<15)
					{
						currentside->bitrate=DEFAULT_DD_BITRATE;
					}
					else
					{
						currentside->bitrate=DEFAULT_HD_BITRATE;
					}
					
					currentside->track_encoding=ISOIBM_MFM_ENCODING;

					tracklen=(currentside->bitrate/(rpm/60))/4;
										
					currentside->tracklen=tracklen;
					
					currentside->databuffer=malloc(currentside->tracklen);
					memset(currentside->databuffer,0,currentside->tracklen);
					
					currentside->flakybitsbuffer=0;
					
					currentside->timingbuffer=0;
					currentside->indexbuffer=malloc(currentside->tracklen);
					memset(currentside->indexbuffer,0,currentside->tracklen);											

					file_offset=(sectorsize*(j*currentside->number_of_sector*floppydisk->floppyNumberOfSide))+
						(sectorsize*(currentside->number_of_sector)*i);
					
					fseek (f , file_offset , SEEK_SET);
					
					fread(trackdata,sectorsize*currentside->number_of_sector,1,f);
					
					if(currentside->number_of_sector==11)
					{
						gap3len=3;
						BuildISOTrack(floppycontext,ISOFORMAT_DD11S,currentside->number_of_sector,1,sectorsize,j,i,gap3len,trackdata,currentside->databuffer,&currentside->tracklen,interleave,(((j<<1)|(i&1))*skew),NULL);
					}
					else
					{
						BuildISOTrack(floppycontext,ISOFORMAT_DD,currentside->number_of_sector,1,sectorsize,j,i,gap3len,trackdata,currentside->databuffer,&currentside->tracklen,interleave,( ((j<<1)|(i&1))*skew),NULL);
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
		fclose(f);
		return LOADER_FILECORRUPT;
	}
	
	floppycontext->hxc_printf(MSG_ERROR,"file size=%d !?",filesize);
	fclose(f);
	return LOADER_BADFILE;
}
