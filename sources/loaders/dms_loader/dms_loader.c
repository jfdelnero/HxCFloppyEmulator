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
// File : dms_loader.c
// Contains: DMS floppy image loader
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

#include "dms_loader.h"

#include "thirdpartylibs/xdms/vfile.h"

#include "thirdpartylibs/xdms/xdms-1.3.2/src/cdata.h"
#include "thirdpartylibs/xdms/xdms-1.3.2/src/pfile.h"
#include "thirdpartylibs/xdms/xdms-1.3.2/src/crc_csum.h"

#include "libhxcadaptor.h"

int DMS_libIsValidDiskFile(HXCFLOPPYEMULATOR* floppycontext,char * imgfile)
{
	floppycontext->hxc_printf(MSG_DEBUG,"DMS_libIsValidDiskFile");

	if(hxc_checkfileext(imgfile,"dms"))
	{
		floppycontext->hxc_printf(MSG_DEBUG,"DMS_libIsValidDiskFile : DMS file !");
		return HXCFE_VALIDFILE;
	}
	else
	{
		floppycontext->hxc_printf(MSG_DEBUG,"DMS_libIsValidDiskFile : non DMS file !");
		return HXCFE_BADFILE;
	}

	return HXCFE_BADPARAMETER;
}



int DMS_libLoad_DiskFile(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	unsigned int filesize;
	unsigned int i,j;
	unsigned int file_offset;
	unsigned short sectorsize;
	unsigned char gap3len,skew,trackformat,interleave;
	HXCFILE * fo;
	unsigned char* flatimg;
	int retxdms;
	CYLINDER* currentcylinder;

	floppycontext->hxc_printf(MSG_DEBUG,"DMS_libLoad_DiskFile %s",imgfile);

	// ouverture et decompression du fichier dms
	fo=HXC_fopen("","");
	retxdms=Process_File(imgfile,fo, CMD_UNPACK, 0, 0, 0);
	if(retxdms)
	{
		floppycontext->hxc_printf(MSG_ERROR,"XDMS: Error %d while reading the file!",retxdms);
		HXC_fclose(fo);
		return HXCFE_ACCESSERROR;
	}


	filesize=fo->buffersize;
	flatimg=fo->buffer;

	if(fo!=0)
	{
		sectorsize=512;
		interleave=1;
		gap3len=0;
		skew=0;
		trackformat=AMIGAFORMAT_DD;
		floppydisk->floppySectorPerTrack=11;
		floppydisk->floppyNumberOfSide=2;
		floppydisk->floppyNumberOfTrack=(filesize/(512*2*11));
		floppydisk->floppyBitRate=DEFAULT_AMIGA_BITRATE;
		floppydisk->floppyiftype=AMIGA_DD_FLOPPYMODE;
		floppydisk->tracks=(CYLINDER**)malloc(sizeof(CYLINDER*)*(floppydisk->floppyNumberOfTrack+4));

		for(j=0;j<floppydisk->floppyNumberOfTrack;j++)
		{

			floppydisk->tracks[j]=allocCylinderEntry(DEFAULT_AMIGA_RPM,floppydisk->floppyNumberOfSide);
			currentcylinder=floppydisk->tracks[j];

			for(i=0;i<floppydisk->floppyNumberOfSide;i++)
			{

				file_offset=(sectorsize*(j*floppydisk->floppySectorPerTrack*floppydisk->floppyNumberOfSide))+
							(sectorsize*(floppydisk->floppySectorPerTrack)*i);

				currentcylinder->sides[i]=tg_generateTrack(&flatimg[file_offset],sectorsize,floppydisk->floppySectorPerTrack,(unsigned char)j,(unsigned char)i,0,interleave,(unsigned char)(((j<<1)|(i&1))*skew),floppydisk->floppyBitRate,currentcylinder->floppyRPM,trackformat,gap3len,0,2500,-11150);

			}
		}

		// Add 4 empty tracks
		for(j=floppydisk->floppyNumberOfTrack;j<(unsigned int)(floppydisk->floppyNumberOfTrack + 4);j++)
		{
			floppydisk->tracks[j]=allocCylinderEntry(DEFAULT_AMIGA_RPM,floppydisk->floppyNumberOfSide);
			currentcylinder=floppydisk->tracks[j];

			for(i=0;i<floppydisk->floppyNumberOfSide;i++)
			{
				currentcylinder->sides[i]=tg_generateTrack(&flatimg[file_offset],sectorsize,0,(unsigned char)j,(unsigned char)i,0,interleave,(unsigned char)(((j<<1)|(i&1))*skew),floppydisk->floppyBitRate,currentcylinder->floppyRPM,trackformat,gap3len,0,2500,-11150);
			}
		}

		floppydisk->floppyNumberOfTrack += 4;

		floppycontext->hxc_printf(MSG_INFO_1,"DMS Loader : tracks file successfully loaded and encoded!");
		HXC_fclose(fo);
		return HXCFE_NOERROR;
	}

	floppycontext->hxc_printf(MSG_ERROR,"DMS file access error!");
	return HXCFE_ACCESSERROR;
}

int DMS_libGetPluginInfo(HXCFLOPPYEMULATOR* floppycontext,unsigned long infotype,void * returnvalue)
{

	static const char plug_id[]="AMIGA_DMS";
	static const char plug_desc[]="AMIGA DMS Loader";
	static const char plug_ext[]="dms";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)	DMS_libIsValidDiskFile,
		(LOADDISKFILE)		DMS_libLoad_DiskFile,
		(WRITEDISKFILE)		0,
		(GETPLUGININFOS)	DMS_libGetPluginInfo
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
