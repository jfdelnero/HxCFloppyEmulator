/*
//
// Copyright (C) 2006-2014 Jean-François DEL NERO
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
// File : adf_loader.c
// Contains: ADF floppy image loader
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

#include "adf_loader.h"

#include "adf_writer.h"

#include "libhxcadaptor.h"

int ADF_libIsValidDiskFile(HXCFLOPPYEMULATOR* floppycontext,char * imgfile)
{
	int filesize;

	floppycontext->hxc_printf(MSG_DEBUG,"ADF_libIsValidDiskFile");

	if(hxc_checkfileext(imgfile,"adf"))
	{
		floppycontext->hxc_printf(MSG_DEBUG,"ADF_libIsValidDiskFile : %s is an ADF file !",imgfile);

		filesize=hxc_getfilesize(imgfile);
		if(filesize<0)
		{
			floppycontext->hxc_printf(MSG_ERROR,"ADF_libIsValidDiskFile : Cannot open %s !",imgfile);
			return HXCFE_ACCESSERROR;
		}

		if(filesize%(512*11))
		{
			floppycontext->hxc_printf(MSG_DEBUG,"ADF_libIsValidDiskFile : non ADF file ! Bad file size!");
			return HXCFE_BADFILE;
		}

		return HXCFE_VALIDFILE;
	}
	else
	{
		floppycontext->hxc_printf(MSG_DEBUG,"ADF_libIsValidDiskFile : non ADF file !");
		return HXCFE_BADFILE;
	}

	return HXCFE_BADPARAMETER;
}

int ADF_libLoad_DiskFile(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	FILE * f;
	unsigned int filesize;
	unsigned int i,j;
	unsigned short rpm;
	unsigned int file_offset;
	unsigned char* trackdata;
	unsigned char gap3len,skew,trackformat,interleave;
	unsigned short sectorsize;
	CYLINDER* currentcylinder;

	floppycontext->hxc_printf(MSG_DEBUG,"ADF_libLoad_DiskFile %s",imgfile);

	f=hxc_fopen(imgfile,"rb");
	if(f==NULL)
	{
		floppycontext->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	fseek (f , 0 , SEEK_END);
	filesize=ftell(f);
	fseek (f , 0 , SEEK_SET);

	if(!filesize)
	{
		floppycontext->hxc_printf(MSG_ERROR,"Bad file size : %d !",filesize);
		hxc_fclose(f);
		return HXCFE_BADFILE;
	}

	if(filesize<100*11*2*512)
	{
		floppydisk->floppySectorPerTrack=11;
		rpm=DEFAULT_AMIGA_RPM;
	}
	else
	{
		floppydisk->floppySectorPerTrack=22;
		rpm=DEFAULT_AMIGA_RPM/2;
	}

	floppydisk->floppyNumberOfSide=2;

	if((filesize/(512*floppydisk->floppySectorPerTrack*floppydisk->floppyNumberOfSide))<80)
		floppydisk->floppyNumberOfTrack=80;
	else
		floppydisk->floppyNumberOfTrack=filesize/(512*floppydisk->floppySectorPerTrack*floppydisk->floppyNumberOfSide);

	floppydisk->floppyBitRate=DEFAULT_AMIGA_BITRATE;
	floppydisk->floppyiftype=AMIGA_DD_FLOPPYMODE;
	floppydisk->tracks=(CYLINDER**)malloc(sizeof(CYLINDER*)*(floppydisk->floppyNumberOfTrack+4));

	sectorsize=512;
	interleave=1;
	gap3len=0;
	skew=0;
	trackformat=AMIGAFORMAT_DD;

	trackdata = (unsigned char*)malloc(sectorsize*floppydisk->floppySectorPerTrack);

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

			currentcylinder->sides[i]=tg_generateTrack(trackdata,sectorsize,floppydisk->floppySectorPerTrack,(unsigned char)j,(unsigned char)i,0,interleave,(unsigned char)(((j<<1)|(i&1))*skew),floppydisk->floppyBitRate,currentcylinder->floppyRPM,trackformat,gap3len,0,2500,-11150);
		}
	}

	// Add 4 empty tracks
	for(j=floppydisk->floppyNumberOfTrack;j<(unsigned int)(floppydisk->floppyNumberOfTrack + 4);j++)
	{
		floppydisk->tracks[j]=allocCylinderEntry(rpm,floppydisk->floppyNumberOfSide);
		currentcylinder=floppydisk->tracks[j];

		for(i=0;i<floppydisk->floppyNumberOfSide;i++)
		{
			currentcylinder->sides[i]=tg_generateTrack(trackdata,sectorsize,0,(unsigned char)j,(unsigned char)i,0,interleave,(unsigned char)(((j<<1)|(i&1))*skew),floppydisk->floppyBitRate,currentcylinder->floppyRPM,trackformat,gap3len,0,2500,-11150);
		}
	}

	floppydisk->floppyNumberOfTrack += 4;


	free(trackdata);

	floppycontext->hxc_printf(MSG_INFO_1,"track file successfully loaded and encoded!");
	hxc_fclose(f);
	return HXCFE_NOERROR;
}

int ADF_libGetPluginInfo(HXCFLOPPYEMULATOR* floppycontext,unsigned long infotype,void * returnvalue)
{

	static const char plug_id[]="AMIGA_ADF";
	static const char plug_desc[]="AMIGA ADF Loader";
	static const char plug_ext[]="adf";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)	ADF_libIsValidDiskFile,
		(LOADDISKFILE)		ADF_libLoad_DiskFile,
		(WRITEDISKFILE)		ADF_libWrite_DiskFile,
		(GETPLUGININFOS)	ADF_libGetPluginInfo
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
