/*
//
// Copyright (C) 2006 - 2013 Jean-François DEL NERO
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
// File : msa_loader.c
// Contains: MSA floppy image loader
//
// Written by:	DEL NERO Jean Francois
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "types.h"
#include "libhxcfe.h"

#include "floppy_loader.h"
#include "floppy_utils.h"

#include "msa_loader.h"

#include "libhxcadaptor.h"

int MSA_libIsValidDiskFile(HXCFLOPPYEMULATOR* floppycontext,char * imgfile)
{
	FILE * f;
	unsigned char signature[3];

	floppycontext->hxc_printf(MSG_DEBUG,"MSA_libIsValidDiskFile");

	if(hxc_checkfileext(imgfile,"msa"))
	{
		f=hxc_fopen(imgfile,"rb");
		if(f)
		{
			fread(signature,3,1,f);
			hxc_fclose(f);
			if(signature[0]==0x0E && signature[1]==0x0F && signature[2]==0x00)
			{
				floppycontext->hxc_printf(MSG_DEBUG,"MSA_libIsValidDiskFile : MSA file !");
				return HXCFE_VALIDFILE;
			}
		}
	}

	floppycontext->hxc_printf(MSG_DEBUG,"MSA_libIsValidDiskFile : non MSA file !");
	return HXCFE_BADFILE;
}

int MSA_libLoad_DiskFile(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	FILE * f;
	unsigned int filesize;
	unsigned int i,j,k,l,l2;
	unsigned int file_offset;
	unsigned char * flatimg,c,skew;
	unsigned char   gap3len,interleave,numberofside,numberofsectorpertrack;
	unsigned short  rpm;
	unsigned short  sectorsize;
	unsigned short  numberoftrack;
	unsigned int    extractfilesize,filetracksize;
	unsigned char   fileheader[5*2];
	unsigned char   trackheader[1*2];
	unsigned char*  tmpbuffer;
	unsigned long   len;
	unsigned char   trackformat;

	CYLINDER* currentcylinder;
	
	floppycontext->hxc_printf(MSG_DEBUG,"MSA_libLoad_DiskFile %s",imgfile);
	
	f=hxc_fopen(imgfile,"rb");
	if(f==NULL) 
	{
		floppycontext->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
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
				if(filetracksize==((unsigned int)numberofsectorpertrack*512))
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
								return HXCFE_FILECORRUPTED;
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
								return HXCFE_FILECORRUPTED;
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
			}while(i<(unsigned int)(numberoftrack*(numberofside)));
			
			hxc_fclose(f);
			
			floppydisk->floppyNumberOfTrack=numberoftrack;
			floppydisk->floppyNumberOfSide=numberofside;
			floppydisk->floppySectorPerTrack=numberofsectorpertrack;

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

			trackformat=ISOFORMAT_DD;
			gap3len=84;
			interleave=1;
			switch(floppydisk->floppySectorPerTrack)
			{
				case 10:
					gap3len=30;
				break;
				case 11:
					trackformat=ISOFORMAT_DD11S;
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

			floppydisk->tracks=(CYLINDER**)malloc(sizeof(CYLINDER*)*floppydisk->floppyNumberOfTrack);
			
			rpm=300;
								
			for(j=0;j<floppydisk->floppyNumberOfTrack;j++)
			{
				
				floppydisk->tracks[j]=allocCylinderEntry(rpm,floppydisk->floppyNumberOfSide);
				currentcylinder=floppydisk->tracks[j];
				
				for(i=0;i<floppydisk->floppyNumberOfSide;i++)
				{
					file_offset=(sectorsize*(j*floppydisk->floppySectorPerTrack*floppydisk->floppyNumberOfSide))+
								(sectorsize*(floppydisk->floppySectorPerTrack)*i);
					
					currentcylinder->sides[i]=tg_generateTrack(&flatimg[file_offset],sectorsize,floppydisk->floppySectorPerTrack,(unsigned char)j,(unsigned char)i,1,interleave,(unsigned char)(((j<<1)|(i&1))*skew),floppydisk->floppyBitRate,rpm,trackformat,gap3len,0,2500,-2500);
				}
			}
			free(flatimg);
			
			floppycontext->hxc_printf(MSG_INFO_1,"track file successfully loaded and encoded!");
		}
		return HXCFE_NOERROR;
	}
	
	floppycontext->hxc_printf(MSG_ERROR,"file size=%d !?",filesize);
	return HXCFE_BADFILE;
}

int MSA_libGetPluginInfo(HXCFLOPPYEMULATOR* floppycontext,unsigned long infotype,void * returnvalue)
{

	static const char plug_id[]="ATARIST_MSA";
	static const char plug_desc[]="ATARI ST MSA Loader";
	static const char plug_ext[]="msa";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)	MSA_libIsValidDiskFile,
		(LOADDISKFILE)		MSA_libLoad_DiskFile,
		(WRITEDISKFILE)		0,
		(GETPLUGININFOS)	MSA_libGetPluginInfo
	};

	return libGetPluginInfo(
			floppycontext,
			infotype,
			returnvalue,
			plug_id,
			plug_desc,
			&plug_funcs,
			plug_ext
			);
}
