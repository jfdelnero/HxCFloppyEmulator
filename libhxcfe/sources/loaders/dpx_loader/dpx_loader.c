/*
//
// Copyright (C) 2006 - 2012 Jean-François DEL NERO
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
// File : dpx_loader.c
// Contains: DPX floppy image loader
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

#include "dpx_loader.h"

#include "os_api.h"

int DPX_libIsValidDiskFile(HXCFLOPPYEMULATOR* floppycontext,char * imgfile)
{
	int filesize;
	
	floppycontext->hxc_printf(MSG_DEBUG,"DPX_libIsValidDiskFile");
	
	if( checkfileext(imgfile,"dpx") )
	{

		filesize=getfilesize(imgfile);
		if(filesize<0) 
		{
			floppycontext->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
			return HXCFE_ACCESSERROR;
		}

		if(filesize%((5*1024) + (1*512)) )
		{
			floppycontext->hxc_printf(MSG_DEBUG,"non DPX file - bad file size !");
			return HXCFE_BADFILE;
		}

		floppycontext->hxc_printf(MSG_DEBUG,"DPX file !");
		return HXCFE_VALIDFILE;
	}
	else
	{
		floppycontext->hxc_printf(MSG_DEBUG,"non DPX file !");
		return HXCFE_BADFILE;
	}
	
	return HXCFE_BADPARAMETER;
}



int DPX_libLoad_DiskFile(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	
	FILE * f;
	unsigned int filesize;
	unsigned int i,j,k;
	unsigned int file_offset;
	char* trackdata;
	int tracklen;
	unsigned char gap3len,interleave;
	unsigned short sectorsize,rpm;
	int numberofsector;
	unsigned char trackformat;
	unsigned char skew;
	CYLINDER* currentcylinder;
	SECTORCONFIG  sectorconfig[6];
	
	floppycontext->hxc_printf(MSG_DEBUG,"DPX_libLoad_DiskFile %s",imgfile);
	
	f=hxc_fopen(imgfile,"rb");
	if(f==NULL) 
	{
		floppycontext->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}
	
	fseek (f , 0 , SEEK_END); 
	filesize=ftell(f);
	fseek (f , 0 , SEEK_SET); 
	
	numberofsector=6;
	trackformat=ISOFORMAT_DD;

	if(filesize!=0)
	{		
		
		sectorsize=1024;
		gap3len=30;
		interleave=1;
		skew=0;
		floppydisk->floppyNumberOfSide=2;
		floppydisk->floppySectorPerTrack=6;
		floppydisk->floppyNumberOfTrack=(filesize/(((numberofsector-1)*1024)+512)) / floppydisk->floppyNumberOfSide;

		if(1)
		{
			
			floppydisk->floppyBitRate=250000;
			floppydisk->floppyiftype=GENERIC_SHUGART_DD_FLOPPYMODE;
			floppydisk->tracks=(CYLINDER**)malloc(sizeof(CYLINDER*)*floppydisk->floppyNumberOfTrack);
			
			rpm=300; // normal rpm
			
			floppycontext->hxc_printf(MSG_INFO_1,"filesize:%dkB, %d tracks, %d side(s), %d sectors/track, gap3:%d, interleave:%d,rpm:%d",filesize/1024,floppydisk->floppyNumberOfTrack,floppydisk->floppyNumberOfSide,floppydisk->floppySectorPerTrack,gap3len,interleave,rpm);
				
			tracklen=(DEFAULT_DD_BITRATE/(rpm/60))/4;
			trackdata=(unsigned char*)malloc(((numberofsector-1)*1024)+512);

			for(j=0;j<floppydisk->floppyNumberOfTrack;j++)
			{
				
				floppydisk->tracks[j]=allocCylinderEntry(rpm,floppydisk->floppyNumberOfSide);
				currentcylinder=floppydisk->tracks[j];
				
				for(i=0;i<floppydisk->floppyNumberOfSide;i++)
				{
					
					memset(sectorconfig,0,sizeof(SECTORCONFIG)*6);
					for(k=0;k<6;k++)
					{
						sectorconfig[k].head=i;
						sectorconfig[k].cylinder=j;
						sectorconfig[k].sector=k;
						sectorconfig[k].sectorsize=1024;
						sectorconfig[k].bitrate=floppydisk->floppyBitRate;
						sectorconfig[k].gap3=gap3len;
						sectorconfig[k].trackencoding=trackformat;
						sectorconfig[k].input_data=&trackdata[k*1024];
					}
					sectorconfig[numberofsector-1].sectorsize=512;
					
					file_offset=( (((numberofsector-1)*1024)+512) * floppydisk->floppyNumberOfSide * j ) +
						        ( (((numberofsector-1)*1024)+512) * i );
					
					fseek (f , file_offset , SEEK_SET);
					
					fread(trackdata,(((numberofsector-1)*1024)+512),1,f);
					
					currentcylinder->sides[i]=tg_generateTrackEx(floppydisk->floppySectorPerTrack,sectorconfig,1,0,floppydisk->floppyBitRate,rpm,trackformat,2500,-2500);
				}
			}

			free(trackdata);
			
			floppycontext->hxc_printf(MSG_INFO_1,"track file successfully loaded and encoded!");
		
			hxc_fclose(f);
			return HXCFE_NOERROR;

		}
		hxc_fclose(f);
		return HXCFE_FILECORRUPTED;
	}
	
	floppycontext->hxc_printf(MSG_ERROR,"file size=%d !?",filesize);
	hxc_fclose(f);
	return HXCFE_BADFILE;
}

int DPX_libGetPluginInfo(HXCFLOPPYEMULATOR* floppycontext,unsigned long infotype,void * returnvalue)
{

	static const char plug_id[]="OBERHEIM_DPX";
	static const char plug_desc[]="Oberheim DPX Loader";
	static const char plug_ext[]="dpx";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)	DPX_libIsValidDiskFile,
		(LOADDISKFILE)		DPX_libLoad_DiskFile,
		(WRITEDISKFILE)		0,
		(GETPLUGININFOS)	DPX_libGetPluginInfo
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

