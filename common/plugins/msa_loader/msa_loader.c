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
// File : MSA_DiskFile.c
// Contains: MSA floppy image loader and plugins interfaces
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

#include "msa_loader.h"

#include "../common/os_api.h"

int MSA_libIsValidDiskFile(HXCFLOPPYEMULATOR* floppycontext,char * imgfile)
{
	int pathlen;
	char * filepath;
	floppycontext->hxc_printf(MSG_DEBUG,"MSA_libIsValidDiskFile %s",imgfile);
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

				if(strstr( filepath,".msa" )!=NULL)
				{
					floppycontext->hxc_printf(MSG_DEBUG,"MSA file !");
					free(filepath);
					return LOADER_ISVALID;
				}
				else
				{
					floppycontext->hxc_printf(MSG_DEBUG,"non MSA file !");
					free(filepath);
					return LOADER_BADFILE;
				}
			}
		}
	}

	return LOADER_BADPARAMETER;
}



int MSA_libLoad_DiskFile(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	FILE * f;
	unsigned int filesize;
	unsigned int i,j,k,l,l2;
	unsigned int file_offset;
	char* trackdata;
	unsigned char * flatimg,c;
	int tracklen;
	int gap3len,interleave,rpm;
	int sectorsize;
	unsigned int numberoftrack,numberofside,numberofsectorpertrack;
	unsigned int extractfilesize,filetracksize,skew;
	unsigned char   fileheader[5*2];
	unsigned char   trackheader[1*2];
	unsigned char* tmpbuffer;
	unsigned long len;
	CYLINDER* currentcylinder;
	SIDE* currentside;
	
	floppycontext->hxc_printf(MSG_DEBUG,"MSA_libLoad_DiskFile %s",imgfile);
	
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
		sectorsize=512; // msa file support only 512bytes/sector floppies.
		
		fread(fileheader,5*sizeof(unsigned short),1,f);
		if(fileheader[0]==0x0E && fileheader[1]==0x0F)
		{
			numberoftrack=((256*fileheader[8])+fileheader[9])+1;
			numberofside=((256*fileheader[4])+fileheader[5])+1;
			numberofsectorpertrack=fileheader[2]*256+fileheader[3];
			
			extractfilesize=(numberofsectorpertrack*512)*(numberoftrack+1)*(numberofside);
			
			flatimg=(unsigned char*)malloc(extractfilesize);
			memset(flatimg,0,extractfilesize);
			
			// chargement et decompression msa.
			j=0;
			i=0;
			do
			{
				fread(trackheader,2,1,f);
				filetracksize=((trackheader[0]*256)+trackheader[1]);
				if(filetracksize==(numberofsectorpertrack*512))
				{
					tmpbuffer=(unsigned char*)malloc(filetracksize);
					memset(tmpbuffer,0,filetracksize);
					fread(tmpbuffer,filetracksize,1,f);
					memcpy(flatimg+j,tmpbuffer,filetracksize);
					free(tmpbuffer);
					j=j+filetracksize;
				}
				else
				{
					k=0;
					l=0;
					tmpbuffer=(unsigned char*)malloc(filetracksize);
					memset(tmpbuffer,0,filetracksize);

					fread(tmpbuffer,filetracksize,1,f);
					do
					{
						if(tmpbuffer[k]!=0xE5)
						{
							if(l+j>extractfilesize)
							{
								return LOADER_FILECORRUPT;
							}
							
							flatimg[l+j]=tmpbuffer[k];
							l++;
							k++;
						}
						else
						{
							k++;
							c=tmpbuffer[k];
							k++;
							len=256*tmpbuffer[k];
							k++;
							len=len+tmpbuffer[k];
							k++;
							
							if(l+j+len>extractfilesize)
							{
								return LOADER_FILECORRUPT;
							}
							else
							{
								for(l2=0;l2<len;l2++)
								{
									flatimg[l+j]=c;
									l++;
								}
							}

						}
					}while(k<filetracksize);
					
					j=j+l;
					
					free(tmpbuffer);
				}
				
				i++;
			}while(i<(numberoftrack*(numberofside)));
			
			fclose(f);
			
			floppydisk->floppyNumberOfTrack=numberoftrack;
			floppydisk->floppyNumberOfSide=numberofside;
			floppydisk->floppySectorPerTrack=numberofsectorpertrack;
			gap3len=84;
			interleave=1;
			

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
			
			rpm=300;
					
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
					currentside=currentcylinder->sides[i];
					memset(currentcylinder->sides[i],0,sizeof(SIDE));
					
					currentside->number_of_sector=floppydisk->floppySectorPerTrack;
					currentside->tracklen=tracklen;
					
					currentside->databuffer=malloc(currentside->tracklen);
					memset(currentside->databuffer,0,currentside->tracklen);
					
					currentside->flakybitsbuffer=0;
					
					currentside->timingbuffer=0;
					currentside->bitrate=floppydisk->floppyBitRate;

					currentside->track_encoding=ISOIBM_MFM_ENCODING;

					currentside->indexbuffer=malloc(currentside->tracklen);
					memset(currentside->indexbuffer,0,currentside->tracklen);


					file_offset=(sectorsize*(j*currentside->number_of_sector*floppydisk->floppyNumberOfSide))+
						(sectorsize*(currentside->number_of_sector)*i);
					
					memcpy(trackdata,&flatimg[file_offset],sectorsize*currentside->number_of_sector);
					

					gap3len=84;
					interleave=1;

					switch(currentside->number_of_sector)
					{
						case 10:
							gap3len=30;
							break;
						case 11:
							gap3len=3;
							interleave=2;
							break;
						case 19:
							gap3len=70;
							break;
						case 20:
							gap3len=40;
							break;
						case 21:
							gap3len=18;
							break;
					}

					if(currentside->number_of_sector==11)
					{
						BuildISOTrack(floppycontext,ISOFORMAT_DD11S,currentside->number_of_sector,1,sectorsize,j,i,gap3len,trackdata,currentside->databuffer,&currentside->tracklen,interleave,(((j<<1)|(i&1))*skew),NULL);
					}
					else
					{
						BuildISOTrack(floppycontext,ISOFORMAT_DD,currentside->number_of_sector,1,sectorsize,j,i,gap3len,trackdata,currentside->databuffer,&currentside->tracklen,interleave,(((j<<1)|(i&1))*skew),NULL);
					}

					currentside->tracklen=currentside->tracklen*8;

					fillindex(currentside->tracklen-1,currentside,2500,TRUE,1);

				}
			}
	        free(trackdata);		
			free(flatimg);
			
			floppycontext->hxc_printf(MSG_INFO_1,"track file successfully loaded and encoded!");
		}
		return LOADER_NOERROR;
	}
	
	floppycontext->hxc_printf(MSG_ERROR,"file size=%d !?",filesize);
	return LOADER_BADFILE;
}

