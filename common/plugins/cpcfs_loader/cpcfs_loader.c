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
// File : CPCFSDK_DiskFile.c
// Contains: CPCFSDK floppy image loader and plugins interfaces
//
// Written by:	DEL NERO Jean Francois
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>

#include "hxc_floppy_emulator.h"
#include "internal_floppy.h"
#include "floppy_loader.h"

#include "../common/amiga_track.h"

#include "cpcfs_loader.h"

#include "../os_api.h"


#include "./libs/adflib/Lib/adflib.h"


HXCFLOPPYEMULATOR* global_floppycontext;

int CPCFSDK_libIsValidDiskFile(HXCFLOPPYEMULATOR* floppycontext,char * imgfile)
{
	
	int pathlen;
	char * filepath;
    struct stat staterep;

	floppycontext->hxc_printf(MSG_DEBUG,"CPCFSDK_libIsValidDiskFile %s",imgfile);
	if(imgfile)
	{
		pathlen=strlen(imgfile);
		if(pathlen!=0)
		{		
			stat(imgfile,&staterep);

			if(staterep.st_mode&S_IFDIR)
			{
				filepath=malloc(pathlen+1);
				if(filepath!=0)
				{
					sprintf(filepath,"%s",imgfile);
					strlower(filepath);

					if(strstr( filepath,".cpcfs" )!=NULL)
					{
						floppycontext->hxc_printf(MSG_DEBUG,"CPCFSDK file !");
						free(filepath);
						return LOADER_ISVALID;
					}
					else
					{
						floppycontext->hxc_printf(MSG_DEBUG,"non CPCFSDK file ! (.cpcfs missing)");
						free(filepath);
						return LOADER_BADFILE;
					}
				}
			}
			else
			{
				floppycontext->hxc_printf(MSG_DEBUG,"non CPCFSDK file ! (it's not a directory)");
				return LOADER_BADFILE;
			}
		}
		floppycontext->hxc_printf(MSG_DEBUG,"0 byte string ?");
	}
	return LOADER_BADPARAMETER;


}


int ScanCpcFile(HXCFLOPPYEMULATOR* floppycontext,struct Volume * adfvolume,char * folder,char * file)
{
	long hfindfile;
	filefoundinfo FindFileData;
	int bbool;
	int byte_written;
	FILE * ftemp;
	unsigned char  tempbuffer[512];
	struct File* adffile;
	unsigned char * fullpath;//,*fileimg;
	int size,filesize;
	RETCODE  rc;
	
	hfindfile=find_first_file(folder,file, &FindFileData); 
	if(hfindfile!=-1)
	{
		bbool=TRUE;
		while(hfindfile!=-1 && bbool)
		{
			if(FindFileData.isdirectory)
			{
				if(strcmp(".",FindFileData.filename)!=0 && strcmp("..",FindFileData.filename)!=0)
				{
					if(adfCountFreeBlocks(adfvolume)>4)
					{
						floppycontext->hxc_printf(MSG_INFO_1,"Adding directory %s",FindFileData.filename);
						rc=adfCreateDir(adfvolume,adfvolume->curDirPtr,FindFileData.filename); 
						if(rc==RC_OK)
						{
							floppycontext->hxc_printf(MSG_INFO_1,"entering directory %s",FindFileData.filename);
							rc=adfChangeDir(adfvolume, FindFileData.filename);
							if(rc==RC_OK)
							{
								
								fullpath=malloc(strlen(FindFileData.filename)+strlen(folder)+2);
								sprintf(fullpath,"%s\\%s",folder,FindFileData.filename);

								if(ScanFile(floppycontext,adfvolume,fullpath,file))
								{
									adfParentDir(adfvolume);
									free(fullpath);
									return 1;
								}
								floppycontext->hxc_printf(MSG_INFO_1,"Leaving directory %s",FindFileData.filename);
								free(fullpath);
								adfParentDir( adfvolume);
							}
							else
							{
								floppycontext->hxc_printf(MSG_ERROR,"Cannot enter to the directory %s !",FindFileData.filename);
								return 1;
							}
						}
						else
						{
							floppycontext->hxc_printf(MSG_ERROR,"Cannot Add the directory %s !",FindFileData.filename);
							return 1;
						}
					}
					else
					{
						floppycontext->hxc_printf(MSG_ERROR,"Cannot Add a directory ! : no more free block!!!");
					    return 1;
					}
					
				}
			}
			else
			{
				if(adfCountFreeBlocks(adfvolume)>4)
				{
						floppycontext->hxc_printf(MSG_INFO_1,"Adding file %s, %dB",FindFileData.filename,FindFileData.size);
						adffile = adfOpenFile(adfvolume, FindFileData.filename, "w");
						if(adffile)
						{
							if(FindFileData.size)
							{
								fullpath=malloc(strlen(FindFileData.filename)+strlen(folder)+2);
								sprintf(fullpath,"%s\\%s",folder,FindFileData.filename);

								ftemp=fopen(fullpath,"rb");
								if(ftemp)
								{
									fseek(ftemp,0,SEEK_END);
									filesize=ftell(ftemp);
									fseek(ftemp,0,SEEK_SET);
									do
									{					
										if(filesize>=512)
										{
											size=512;		
										}
										else
										{
                                            size=filesize;
										}
										fread(&tempbuffer,size,1,ftemp);

										byte_written=adfWriteFile(adffile, size, tempbuffer);
										if((byte_written!=size) || (adfCountFreeBlocks(adfvolume)<2) )
										{
											floppycontext->hxc_printf(MSG_ERROR,"Error while writting the file %s. No more free block ?",FindFileData.filename);
											adfCloseFile(adffile);
											fclose(ftemp);
											free(fullpath);
											return 1;
										}
										filesize=filesize-512;

									}while( (filesize>0) && (byte_written==size));
								

									/*fileimg=(unsigned char*)malloc(filesize);
									memset(fileimg,0,filesize);
									fread(fileimg,filesize,1,ftemp);
									adfWriteFile(adffile, filesize, fileimg);
									free(fileimg);*/

									adfCloseFile(adffile);
									fclose(ftemp);
									free(fullpath);
								}
								else
								{
										floppycontext->hxc_printf(MSG_ERROR,"Error : Cannot open %s !!!",fullpath);
										free(fullpath);
										return 1;
								}
							}
						}
						else
						{
                             floppycontext->hxc_printf(MSG_ERROR,"Error : Cannot create %s, %dB!!!",FindFileData.filename,FindFileData.size);
							 return 1;
						}
				}
				else
				{
					floppycontext->hxc_printf(MSG_ERROR,"Error : Cannot add a file : no more free block");
					return 1;
				}
			}
			bbool=find_next_file(hfindfile,folder,file,&FindFileData);
		}
		
	}
	else printf("Error FindFirstFile\n");

	find_close(hfindfile);
	return 0;
}

int CPCFSDK_libLoad_DiskFile(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	unsigned int i,j,k;
	unsigned int file_offset;
	char* trackdata;
	int tracklen;

	unsigned char * flatimg2;

	int numberoftrack;
	int numberofsectorpertrack;

    struct stat repstate;
	struct tm * ts;

	CYLINDER* currentcylinder;
	SIDE* currentside;
  
	numberoftrack=80;
	numberofsectorpertrack=11;
	
	floppycontext->hxc_printf(MSG_DEBUG,"CPCFSDK_libLoad_DiskFile %s",imgfile);

	stat(imgfile,&repstate);
	ts=localtime(&repstate.st_ctime);
	if(repstate.st_mode&S_IFDIR)
	{

		global_floppycontext=floppycontext;

		flatimg2=0;
		if(flatimg2)
		{		
			floppydisk->floppySectorPerTrack=numberofsectorpertrack;
			floppydisk->floppyNumberOfSide=2;
			floppydisk->floppyNumberOfTrack=numberoftrack;
			floppydisk->floppyBitRate=DEFAULT_AMIGA_BITRATE;
			floppydisk->floppyiftype=AMIGA_DD_FLOPPYMODE;
			floppydisk->tracks=(CYLINDER**)malloc(sizeof(CYLINDER*)*floppydisk->floppyNumberOfTrack);

			tracklen=(DEFAULT_AMIGA_BITRATE/(DEFAULT_AMIGA_RPM/60))/4;
			trackdata=(unsigned char*)malloc(512*floppydisk->floppySectorPerTrack);

			for(j=0;j<floppydisk->floppyNumberOfTrack;j++)
			{
				
				floppydisk->tracks[j]=(CYLINDER*)malloc(sizeof(CYLINDER));
				currentcylinder=floppydisk->tracks[j];
				currentcylinder->number_of_side=floppydisk->floppyNumberOfSide;
				currentcylinder->sides=(SIDE**)malloc(sizeof(SIDE*)*currentcylinder->number_of_side);
				memset(currentcylinder->sides,0,sizeof(SIDE*)*currentcylinder->number_of_side);
						
				for(i=0;i<floppydisk->floppyNumberOfSide;i++)
				{

					currentcylinder->floppyRPM=DEFAULT_AMIGA_RPM;

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
					currentside->track_encoding=ISOIBM_MFM_ENCODING;

					file_offset=(512*(j*currentside->number_of_sector*2))+
								(512*(currentside->number_of_sector)*i);

					memcpy(trackdata,&flatimg2[file_offset],512*currentside->number_of_sector);
					
					BuildAmigaTrack(trackdata,currentside->databuffer,tracklen,j,i,11);
					
					currentside->tracklen=currentside->tracklen*8;		
							
				}
			}

			free(trackdata);
			free(flatimg2);
			floppycontext->hxc_printf(MSG_INFO_1,"AMIGADOSFSDK Loader : tracks file successfully loaded and encoded!");
			return LOADER_NOERROR;
		}
			
		floppycontext->hxc_printf(MSG_ERROR,"flatimg==0 !?");
		return LOADER_INTERNALERROR;
	}
	else
	{
		floppycontext->hxc_printf(MSG_ERROR,"not a directory !");
		return LOADER_BADFILE;
	}
}


