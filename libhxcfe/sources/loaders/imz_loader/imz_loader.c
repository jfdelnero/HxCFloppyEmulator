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
// File : imz_loader.c
// Contains: IMZ floppy image loader
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

#include "imz_loader.h"

#include "thirdpartylibs/zlib/zlib.h"
#include "thirdpartylibs/zlib/contrib/minizip/unzip.h"

#include "libhxcadaptor.h"

#define UNPACKBUFFER 128*1024

extern int pc_imggetfloppyconfig(unsigned char * img,unsigned int filesize,unsigned short *numberoftrack,unsigned char *numberofside,unsigned short *numberofsectorpertrack,unsigned char *gap3len,unsigned char *interleave,unsigned short *rpm, unsigned int *bitrate,unsigned short * ifmode);

int IMZ_libIsValidDiskFile(HXCFLOPPYEMULATOR* floppycontext,char * imgfile)
{
	int err;
	unzFile uf;
	unz_file_info file_info;
	char filename_inzip[256];

	floppycontext->hxc_printf(MSG_DEBUG,"IMZ_libIsValidDiskFile");
	if( checkfileext(imgfile,"imz"))
	{
		uf=unzOpen (imgfile);
		if (!uf)
		{
			floppycontext->hxc_printf(MSG_ERROR,"IMZ_libIsValidDiskFile : unzOpen: Error while reading the file!");
			return HXCFE_BADFILE;
		}

		err = unzGetCurrentFileInfo(uf,&file_info,filename_inzip,sizeof(filename_inzip),NULL,0,NULL,0);
		if (err!=UNZ_OK)
		{
			unzClose(uf);
			return HXCFE_BADFILE;
		}

		unzClose(uf);
		floppycontext->hxc_printf(MSG_DEBUG,"IMZ_libIsValidDiskFile : IMZ file : %s (%d bytes) !",filename_inzip,file_info.uncompressed_size);
		return HXCFE_VALIDFILE;
	}
	else
	{
		floppycontext->hxc_printf(MSG_DEBUG,"IMZ_libIsValidDiskFile : non IMZ file !");
		return HXCFE_BADFILE;
	}

	return HXCFE_BADPARAMETER;
}



int IMZ_libLoad_DiskFile(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	unsigned int filesize;
	unsigned int i,j;
	unsigned int file_offset;
	unsigned short sectorsize;
	unsigned char gap3len,skew,trackformat,interleave;
	char filename_inzip[256];
	char* flatimg;
	int err=UNZ_OK;
	unzFile uf;
	unz_file_info file_info;
	CYLINDER* currentcylinder;
	unsigned short rpm;
	
	floppycontext->hxc_printf(MSG_DEBUG,"IMZ_libLoad_DiskFile %s",imgfile);
	
	uf=unzOpen (imgfile);
	if (!uf)
	{
		floppycontext->hxc_printf(MSG_ERROR,"unzOpen: Error while reading the file!");
		return HXCFE_BADFILE;
	}
	
	unzGoToFirstFile(uf);

    err = unzGetCurrentFileInfo(uf,&file_info,filename_inzip,sizeof(filename_inzip),NULL,0,NULL,0);
	if (err!=UNZ_OK)
    {
		unzClose(uf);
		return HXCFE_BADFILE;
	}

	err=unzOpenCurrentFile(uf);
	if (err!=UNZ_OK)
    {
		unzClose(uf);
		return HXCFE_BADFILE;
	}

	filesize=file_info.uncompressed_size;
	flatimg=(char*)malloc(filesize);
	if(!flatimg)
	{
		floppycontext->hxc_printf(MSG_ERROR,"Unpack error!");
		return HXCFE_BADFILE;
	}	
	
	err=unzReadCurrentFile  (uf, flatimg, filesize);
	if (err<0)
	{
		floppycontext->hxc_printf(MSG_ERROR,"error %d with zipfile in unzReadCurrentFile",err);
		unzClose(uf);
		free(flatimg);
		return HXCFE_BADFILE;
	}
	
	unzClose(uf);

	if(pc_imggetfloppyconfig(
			flatimg,
			filesize,
			&floppydisk->floppyNumberOfTrack,
			&floppydisk->floppyNumberOfSide,
			&floppydisk->floppySectorPerTrack,
			&gap3len,
			&interleave,
			&rpm,
			&floppydisk->floppyBitRate,
			&floppydisk->floppyiftype)==1
			)
	{		
		sectorsize=512;

		skew=0;
		trackformat=IBMFORMAT_DD;

		floppydisk->tracks=(CYLINDER**)malloc(sizeof(CYLINDER*)*floppydisk->floppyNumberOfTrack);

		for(j=0;j<floppydisk->floppyNumberOfTrack;j++)
		{
			floppydisk->tracks[j]=allocCylinderEntry(rpm,floppydisk->floppyNumberOfSide);
			currentcylinder=floppydisk->tracks[j];
			
			for(i=0;i<floppydisk->floppyNumberOfSide;i++)
			{
				file_offset=(sectorsize*(j*floppydisk->floppySectorPerTrack*floppydisk->floppyNumberOfSide))+
							(sectorsize*(floppydisk->floppySectorPerTrack)*i);

				currentcylinder->sides[i]=tg_generateTrack(&flatimg[file_offset],sectorsize,floppydisk->floppySectorPerTrack,(unsigned char)j,(unsigned char)i,1,interleave,(unsigned char)(((j<<1)|(i&1))*skew),floppydisk->floppyBitRate,currentcylinder->floppyRPM,trackformat,gap3len,0,2500,-2500);
			}
		}

		floppycontext->hxc_printf(MSG_INFO_1,"IMZ Loader : tracks file successfully loaded and encoded!");
		free(flatimg);
		return 0;
	}
	free(flatimg);
	return HXCFE_BADFILE;
}

int IMZ_libGetPluginInfo(HXCFLOPPYEMULATOR* floppycontext,unsigned long infotype,void * returnvalue)
{

	static const char plug_id[]="RAW_IMZ";
	static const char plug_desc[]="IBM PC IMZ Loader";
	static const char plug_ext[]="imz";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)	IMZ_libIsValidDiskFile,
		(LOADDISKFILE)		IMZ_libLoad_DiskFile,
		(WRITEDISKFILE)		0,
		(GETPLUGININFOS)	IMZ_libGetPluginInfo
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
