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
// File : adz_loader.c
// Contains: ADZ floppy image loader
//
// Written by:	DEL NERO Jean Francois
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "libhxcfe.h"
#include "floppy_loader.h"
#include "floppy_utils.h"

#include "adz_loader.h"

#include "thirdpartylibs/zlib/zlib.h"

#include "os_api.h"

#define UNPACKBUFFER 128*1024


int ADZ_libIsValidDiskFile(HXCFLOPPYEMULATOR* floppycontext,char * imgfile)
{
	floppycontext->hxc_printf(MSG_DEBUG,"ADZ_libIsValidDiskFile");

	if(checkfileext(imgfile,"adz") || checkfileext(imgfile,"gz"))
	{
		if(checkfileext(imgfile,"gz"))
		{
			if( !strstr(getfilenamebase(imgfile,0),".adf.gz") ) 
			{
				floppycontext->hxc_printf(MSG_DEBUG,"non ADZ file !");
				return HXCFE_BADFILE;
			}
		}

		floppycontext->hxc_printf(MSG_DEBUG,"ADZ file !");
		return HXCFE_VALIDFILE;
	}
	else
	{
		floppycontext->hxc_printf(MSG_DEBUG,"non ADZ file !");
		return HXCFE_BADFILE;
	}
	
	return HXCFE_BADPARAMETER;
}



int ADZ_libLoad_DiskFile(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	unsigned int filesize;
	unsigned int i,j;
	unsigned int file_offset;
	unsigned short sectorsize;
	unsigned char gap3len,skew,trackformat,interleave;
	char* flatimg;
	gzFile file;
	int err;

	CYLINDER* currentcylinder;
	
	floppycontext->hxc_printf(MSG_DEBUG,"ADZ_libLoad_DiskFile %s",imgfile);
	
	file = gzopen(imgfile, "rb");
	if (!file)
	{
		floppycontext->hxc_printf(MSG_ERROR,"gzopen: Error while reading the file!");
		return -1;
	}
	
	i=0;
	filesize=0;
	flatimg=(char*)malloc(UNPACKBUFFER);
	do
	{
		err=gzread(file, flatimg+filesize,UNPACKBUFFER );
		filesize=filesize+err;
		flatimg=(char *)realloc(flatimg,filesize+UNPACKBUFFER);
		i++;
	}while(err>0);
	
	gzclose(file);

	if(!flatimg)
	{
		floppycontext->hxc_printf(MSG_ERROR,"Unpack error!");
		return HXCFE_BADFILE;
	}	
	
	if(flatimg)
	{
		sectorsize=512;
		interleave=1;
		gap3len=0;
		skew=0;
		trackformat=AMIGAFORMAT_DD;

		floppydisk->floppySectorPerTrack=11;
		floppydisk->floppyNumberOfSide=2;
		floppydisk->floppyNumberOfTrack=(filesize/(512*2*floppydisk->floppySectorPerTrack));
		floppydisk->floppyBitRate=DEFAULT_AMIGA_BITRATE;
		floppydisk->floppyiftype=AMIGA_DD_FLOPPYMODE;
		floppydisk->tracks=(CYLINDER**)malloc(sizeof(CYLINDER*)*floppydisk->floppyNumberOfTrack);
		

		for(j=0;j<floppydisk->floppyNumberOfTrack;j++)
		{
			floppydisk->tracks[j]=allocCylinderEntry(DEFAULT_AMIGA_RPM,floppydisk->floppyNumberOfSide);
			currentcylinder=floppydisk->tracks[j];
			
			for(i=0;i<floppydisk->floppyNumberOfSide;i++)
			{

				file_offset=(sectorsize*(j*floppydisk->floppySectorPerTrack*floppydisk->floppyNumberOfSide))+
							(sectorsize*(floppydisk->floppySectorPerTrack)*i);
				
				currentcylinder->sides[i]=tg_generateTrack(&flatimg[file_offset],sectorsize,floppydisk->floppySectorPerTrack,(unsigned char)j,(unsigned char)i,0,interleave,(unsigned char)(((j<<1)|(i&1))*skew),floppydisk->floppyBitRate,currentcylinder->floppyRPM,trackformat,gap3len,2500,-11150);
			}
		}

		floppycontext->hxc_printf(MSG_INFO_1,"ADZ Loader : tracks file successfully loaded and encoded!");
		return 0;
	}

	return HXCFE_BADFILE;
}

int ADZ_libGetPluginInfo(HXCFLOPPYEMULATOR* floppycontext,unsigned long infotype,void * returnvalue)
{

	static const char plug_id[]="AMIGA_ADZ";
	static const char plug_desc[]="AMIGA ADZ Loader";
	static const char plug_ext[]="adz";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)	ADZ_libIsValidDiskFile,
		(LOADDISKFILE)		ADZ_libLoad_DiskFile,
		(WRITEDISKFILE)		0,
		(GETPLUGININFOS)	ADZ_libGetPluginInfo
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
