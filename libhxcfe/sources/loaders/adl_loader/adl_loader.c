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
// File : adl_ssd_dsd_loader.c
// Contains: BBC floppy image loader
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

#include "adl_loader.h"

#include "libhxcadaptor.h"

int ADL_libIsValidDiskFile(HXCFLOPPYEMULATOR* floppycontext,char * imgfile)
{
	int filesize;

	floppycontext->hxc_printf(MSG_DEBUG,"ADL_libIsValidDiskFile");

	if( checkfileext(imgfile,"adl"))
	{
		filesize=getfilesize(imgfile);
		if(filesize<0) 
		{
			floppycontext->hxc_printf(MSG_ERROR,"ADL_libIsValidDiskFile : Cannot open %s !",imgfile);
			return HXCFE_ACCESSERROR;
		}

		if(filesize&0x1FF)
		{
			floppycontext->hxc_printf(MSG_DEBUG,"ADL_libIsValidDiskFile : non Acorn BBC ADL IMG file - bad file size !");
			return HXCFE_BADFILE;
		}

		floppycontext->hxc_printf(MSG_DEBUG,"ADL_libIsValidDiskFile : Acorn BBC ADL file !");
		return HXCFE_VALIDFILE;
	}
	else
	{
		floppycontext->hxc_printf(MSG_DEBUG,"ADL_libIsValidDiskFile : non Acorn BBC ADL file !");
		return HXCFE_BADFILE;
	}
	
	return HXCFE_BADPARAMETER;
}



int ADL_libLoad_DiskFile(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	
	FILE * f;
	unsigned int filesize;
	unsigned int file_offset;

	unsigned short i,j;
	unsigned char* trackdata;
	unsigned char  gap3len,interleave,skew,trackformat;
	unsigned short rpm;
	unsigned short sectorsize;
	CYLINDER* currentcylinder;
	
	floppycontext->hxc_printf(MSG_DEBUG,"ADL_libLoad_DiskFile %s",imgfile);
	
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
		sectorsize=256; 
		floppydisk->floppyNumberOfTrack=80;
		floppydisk->floppyNumberOfSide=2;
		floppydisk->floppySectorPerTrack=16;
		gap3len=255;
		interleave=2;
		skew=2;
		rpm=300; // normal rpm	
		floppydisk->floppyiftype=GENERIC_SHUGART_DD_FLOPPYMODE;
		floppydisk->floppyBitRate=DEFAULT_DD_BITRATE;
		trackformat=IBMFORMAT_DD;

		floppydisk->floppyNumberOfSide=2;

		floppydisk->tracks=(CYLINDER**)malloc(sizeof(CYLINDER*)*floppydisk->floppyNumberOfTrack);

		floppycontext->hxc_printf(MSG_INFO_1,"filesize:%dkB, %d tracks, %d side(s), %d sectors/track, gap3:%d, interleave:%d,rpm:%d bitrate:%d",filesize/1024,floppydisk->floppyNumberOfTrack,floppydisk->floppyNumberOfSide,floppydisk->floppySectorPerTrack,gap3len,interleave,rpm,floppydisk->floppyBitRate);	
		trackdata=(unsigned char*)malloc(sectorsize*floppydisk->floppySectorPerTrack);
			
		for(j=0;j<floppydisk->floppyNumberOfTrack;j++)
		{
			
			floppydisk->tracks[j]=allocCylinderEntry(rpm,floppydisk->floppyNumberOfSide);
			currentcylinder=floppydisk->tracks[j];
								
			for(i=0;i<floppydisk->floppyNumberOfSide;i++)
			{					
				file_offset=(sectorsize*(j*floppydisk->floppySectorPerTrack*floppydisk->floppyNumberOfSide))+
					        (sectorsize*(floppydisk->floppySectorPerTrack)*i);
				fseek (f , file_offset , SEEK_SET);
				fread(trackdata,sectorsize*floppydisk->floppySectorPerTrack,1,f);

				currentcylinder->sides[i]=tg_generateTrack(trackdata,sectorsize,floppydisk->floppySectorPerTrack,(unsigned char)j,(unsigned char)i,1,interleave,(unsigned char)(((j<<1)|(i&1))*skew),floppydisk->floppyBitRate,currentcylinder->floppyRPM,trackformat,gap3len,2500| NO_SECTOR_UNDER_INDEX,-2500);
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

int ADL_libGetPluginInfo(HXCFLOPPYEMULATOR* floppycontext,unsigned long infotype,void * returnvalue)
{

	static const char plug_id[]="BBC_ADL";
	static const char plug_desc[]="BBC ADL floppy image loader";
	static const char plug_ext[]="adl";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)	ADL_libIsValidDiskFile,
		(LOADDISKFILE)		ADL_libLoad_DiskFile,
		(WRITEDISKFILE)		0,
		(GETPLUGININFOS)	ADL_libGetPluginInfo
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

