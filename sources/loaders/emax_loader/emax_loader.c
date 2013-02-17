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
// File : emax_loader.c
// Contains: Emax floppy image loader
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

#include "emax_loader.h"

#include "libhxcadaptor.h"

#define SCTR_SIZE 512
#define HEADS 2
#define SCTR_TRK 10
#define TRK_CYL 80
#define START_SCTR 1
#define END_SCTR 10

#define RWDISK_VERSION "1.1"
#define RWDISK_DATE "Fri Mar 19 13:31:05 1993" 
#define EMAXUTIL_HDR "emaxutil v%3s %24s\n"
#define EMAXUTIL_HDRLEN 39 	



#define BANK_LOW 368
#define BANK_HIGH 423
#define SAMPLE_LOW 440
#define SAMPLE_HIGH 1463
#define OS1_LOW 0
#define OS1_HIGH 367
#define OS2_LOW 424
#define OS2_HIGH 439
#define OS3_LOW 1464
#define OS3_HIGH 1599
#define TOTAL_BLKS ((BANK_HIGH-BANK_LOW)+(SAMPLE_HIGH-SAMPLE_LOW))
#define TOTAL_OS ((OS1_HIGH-OS1_LOW)+(OS2_HIGH-OS2_LOW)+(OS3_HIGH-OS3_LOW))


int EMAX_libIsValidDiskFile(HXCFLOPPYEMULATOR* floppycontext,char * imgfile)
{
	int filesize;
	
	floppycontext->hxc_printf(MSG_DEBUG,"EMAX_libIsValidDiskFile");

	if( hxc_checkfileext(imgfile,"em1") ||
		hxc_checkfileext(imgfile,"em2") ||
		hxc_checkfileext(imgfile,"emx")
		)
	{

		filesize=hxc_getfilesize(imgfile);
		if(filesize<0) 
		{
			floppycontext->hxc_printf(MSG_ERROR,"EMAX_libIsValidDiskFile : Cannot open %s !",imgfile);
			return HXCFE_ACCESSERROR;
		}

		floppycontext->hxc_printf(MSG_DEBUG,"EMAX_libIsValidDiskFile : Emax file !");
		return HXCFE_VALIDFILE;
	}
	else
	{
		floppycontext->hxc_printf(MSG_DEBUG,"EMAX_libIsValidDiskFile : non Emax file !");
		return HXCFE_BADFILE;
	}
	
	return HXCFE_BADPARAMETER;
}

int EMAX_libLoad_DiskFile(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	
	FILE * f,*f2;
	unsigned int filesize;
	unsigned int i,j,k;
	unsigned int file_offset;
	unsigned char* floppy_data;
	char os_filename[512];
	unsigned char  gap3len,interleave;
	unsigned short sectorsize,rpm;
	unsigned short numberofsector;
	unsigned char  trackformat,skew;

	CYLINDER* currentcylinder;
	SECTORCONFIG  sectorconfig[10];
    
	char hdr[EMAXUTIL_HDRLEN+1];
    char fhdr[EMAXUTIL_HDRLEN+1]; 

	floppycontext->hxc_printf(MSG_DEBUG,"EMAX_libLoad_DiskFile %s",imgfile);
	
	f=hxc_fopen(imgfile,"rb");
	if(f==NULL) 
	{
		floppycontext->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}
	
	fseek (f , 0 , SEEK_END); 
	filesize=ftell(f);
	fseek (f , 0 , SEEK_SET); 
	
	numberofsector=10;
	
    strcpy(os_filename,imgfile);
	i=strlen(os_filename)-1;
	while(i && (os_filename[i]!='\\') && (os_filename[i]!='/') )
	{
		
		i--;
	}
	if(i)
	{
		i++;
	}
	sprintf(&os_filename[i],"emaxos.emx");

	f2=hxc_fopen(os_filename,"rb");
	if(f2==NULL) 
	{	
		hxc_fclose(f);
		floppycontext->hxc_printf(MSG_ERROR,"Cannot open os file %s !",os_filename);
		return HXCFE_ACCESSERROR;
	}

	if(filesize!=0)
	{		
		
		sectorsize=512;
		gap3len=255;
		interleave=1;
		skew=2;
		trackformat=ISOFORMAT_DD;
		floppydisk->floppyNumberOfSide=2;
		floppydisk->floppySectorPerTrack=10;
		floppydisk->floppyNumberOfTrack=80;

		if(1)
		{
			
			floppydisk->floppyBitRate=250000;
			floppydisk->floppyiftype=GENERIC_SHUGART_DD_FLOPPYMODE;
			floppydisk->tracks=(CYLINDER**)malloc(sizeof(CYLINDER*)*floppydisk->floppyNumberOfTrack);
			
			rpm=300; // normal rpm
			
			floppycontext->hxc_printf(MSG_INFO_1,"filesize:%dkB, %d tracks, %d side(s), %d sectors/track, gap3:%d, interleave:%d,rpm:%d",filesize/1024,floppydisk->floppyNumberOfTrack,floppydisk->floppyNumberOfSide,floppydisk->floppySectorPerTrack,gap3len,interleave,rpm);
				
			floppy_data= malloc((SCTR_SIZE * SCTR_TRK) * TRK_CYL * HEADS);
			memset(floppy_data,0xE6,(SCTR_SIZE * SCTR_TRK) * TRK_CYL * HEADS);

			sprintf (hdr, EMAXUTIL_HDR, RWDISK_VERSION, RWDISK_DATE); 
			hdr[EMAXUTIL_HDRLEN]=0;
			
			fread (fhdr, (unsigned int) EMAXUTIL_HDRLEN,1,f);
			fhdr[EMAXUTIL_HDRLEN]=0;


			if (strncmp(fhdr, hdr, EMAXUTIL_HDRLEN)!=0)
			{
				floppycontext->hxc_printf(MSG_ERROR,"Wrong version: disk says %s", fhdr);
				hxc_fclose(f);
				hxc_fclose(f2);
				return HXCFE_BADFILE;
			} 


			for(i=OS1_LOW;i<=OS1_HIGH;i++)
			{
				fread(&floppy_data[i*512],512,1,f2);
			}

			for(i=OS2_LOW;i<=OS2_HIGH;i++)
			{
				fread(&floppy_data[i*512],512,1,f2);
			}

			for(i=OS3_LOW;i<=OS3_HIGH;i++)
			{
				fread(&floppy_data[i*512],512,1,f2);
			}

			for(i=BANK_LOW;i<=BANK_HIGH;i++)
			{
				fread(&floppy_data[i*512],512,1,f);
			}

			for(i=SAMPLE_LOW;i<=SAMPLE_HIGH;i++)
			{
				fread(&floppy_data[i*512],512,1,f);
			}

			for(j=0;j<floppydisk->floppyNumberOfTrack;j++)
			{	
				floppydisk->tracks[j]=allocCylinderEntry(rpm,floppydisk->floppyNumberOfSide);
				currentcylinder=floppydisk->tracks[j];
				
				for(i=0;i<floppydisk->floppyNumberOfSide;i++)
				{
					
					file_offset=( (((numberofsector)*512)) * floppydisk->floppyNumberOfSide * j ) +
						        ( (((numberofsector)*512)) * i );

					memset(sectorconfig,0,sizeof(SECTORCONFIG)*10);
					for(k=0;k<10;k++)
					{
						sectorconfig[k].head=i;
						sectorconfig[k].cylinder=j;
						sectorconfig[k].sector=k+1;
						sectorconfig[k].sectorsize=512;
						sectorconfig[k].bitrate=floppydisk->floppyBitRate;
						sectorconfig[k].gap3=gap3len;
						sectorconfig[k].trackencoding=trackformat;
						sectorconfig[k].input_data=&floppy_data[file_offset+(k*512)];

					}

					currentcylinder->sides[i]=tg_generateTrackEx(floppydisk->floppySectorPerTrack,(SECTORCONFIG *)&sectorconfig,interleave,(unsigned char)(((j<<1)|(i&1))*skew),floppydisk->floppyBitRate,rpm,trackformat,0,2500|NO_SECTOR_UNDER_INDEX,-2500);
				}
			}

			free(floppy_data);
			
			floppycontext->hxc_printf(MSG_INFO_1,"track file successfully loaded and encoded!");
		
			hxc_fclose(f);
			hxc_fclose(f2);
			return HXCFE_NOERROR;

		}
		hxc_fclose(f);
		hxc_fclose(f2);
		return HXCFE_FILECORRUPTED;
	}
	
	floppycontext->hxc_printf(MSG_ERROR,"file size=%d !?",filesize);
	hxc_fclose(f);
	hxc_fclose(f2);
	return HXCFE_BADFILE;
}

int EMAX_libGetPluginInfo(HXCFLOPPYEMULATOR* floppycontext,unsigned long infotype,void * returnvalue)
{

	static const char plug_id[]="EMAX_EM";
	static const char plug_desc[]="EMAX EM1 & EM2 Loader";
	static const char plug_ext[]="em1";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)	EMAX_libIsValidDiskFile,
		(LOADDISKFILE)		EMAX_libLoad_DiskFile,
		(WRITEDISKFILE)		0,
		(GETPLUGININFOS)	EMAX_libGetPluginInfo
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

