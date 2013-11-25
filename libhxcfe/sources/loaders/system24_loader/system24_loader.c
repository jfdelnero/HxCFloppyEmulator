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
// File : system24_loader.c
// Contains: System24 floppy image loader
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

#include "system24_loader.h"


#include "libhxcadaptor.h"


int System24_libIsValidDiskFile(HXCFLOPPYEMULATOR* floppycontext,char * imgfile)
{
	int filesize;

	floppycontext->hxc_printf(MSG_DEBUG,"System24_libIsValidDiskFile");

	if(hxc_checkfileext(imgfile,"s24"))
	{
		filesize=hxc_getfilesize(imgfile);
		if(filesize<0)
		{
			floppycontext->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
			return HXCFE_ACCESSERROR;
		}

		if(  ( filesize % ( 2 * ( ( 2048 * 5 ) + 1024 + 256 ) ) ) && ( filesize % ( 2 * ( 8192 + ( 1024 * 3 ) + 512 + 256 ) ) ) )
		{
			floppycontext->hxc_printf(MSG_DEBUG,"System24_libIsValidDiskFile : non System 24 file - bad file size !");
			return HXCFE_BADFILE;
		}

		floppycontext->hxc_printf(MSG_DEBUG,"System24_libIsValidDiskFile : S24 file !");
		return HXCFE_VALIDFILE;
	}
	else
	{
		floppycontext->hxc_printf(MSG_DEBUG,"System24_libIsValidDiskFile : non S24 file !");
		return HXCFE_BADFILE;
	}

	return HXCFE_BADPARAMETER;
}



int System24_libLoad_DiskFile(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * floppydisk,char * imgfile,void * parameters)
{

	FILE * f;
	unsigned int filesize;
	unsigned int file_offset;

	unsigned short i,j,k,l,ptr;
	unsigned char* trackdata;
	unsigned char  gap3len,trackformat;
	unsigned short rpm;
	int tracksize;
	CYLINDER* currentcylinder;
	SECTORCONFIG  * sectorconfig;

	floppycontext->hxc_printf(MSG_DEBUG,"System24_libLoad_DiskFile %s",imgfile);

	f=hxc_fopen(imgfile,"rb");
	if(f==NULL)
	{
		floppycontext->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	fseek (f , 0 , SEEK_END);
	filesize=ftell(f);
	fseek (f , 0 , SEEK_SET);

	if(  !( filesize % ( 2 * ( ( 2048 * 5 ) + 1024 + 256 ) ) ) )
	{
		tracksize = ( ( 2048 * 5 ) + 1024 + 256 );
		floppydisk->floppyNumberOfTrack= filesize / ( 2 * tracksize );
		floppydisk->floppyNumberOfSide=2;
		floppydisk->floppySectorPerTrack= 5 + 1 + 1;
	}
	else
	{
		if ( !( filesize % ( 2 * ( 8192 + ( 1024 * 3 ) + 512 + 256 ) ) ) )
		{
			tracksize = ( 8192 + ( 1024 * 3 ) + 512 + 256 );
			floppydisk->floppyNumberOfTrack= filesize / ( 2 * tracksize );
			floppydisk->floppyNumberOfSide=2;
			floppydisk->floppySectorPerTrack= 1 + 3 + 1 + 1;
		}
		else
		{
			hxc_fclose(f);

			floppycontext->hxc_printf(MSG_ERROR,"file size=%d !?",filesize);

			return HXCFE_BADFILE;
		}
	}


	gap3len = 50;

	floppydisk->floppyiftype = IBMPC_HD_FLOPPYMODE;
	floppydisk->floppyBitRate = DEFAULT_HD_BITRATE;
	trackformat=ISOFORMAT_DD;

	floppydisk->tracks=(CYLINDER**)malloc(sizeof(CYLINDER*)*floppydisk->floppyNumberOfTrack);

	rpm=300; // normal rpm

	sectorconfig=malloc(sizeof(SECTORCONFIG) * floppydisk->floppySectorPerTrack);
	memset(sectorconfig,0,sizeof(SECTORCONFIG) * floppydisk->floppySectorPerTrack);

	trackdata=(unsigned char*)malloc(tracksize);

	for(j=0;j<floppydisk->floppyNumberOfTrack;j++)
	{

		floppydisk->tracks[j]=allocCylinderEntry(rpm,floppydisk->floppyNumberOfSide);
		currentcylinder=floppydisk->tracks[j];

		for(i=0;i<floppydisk->floppyNumberOfSide;i++)
		{

			file_offset=( j *  ( tracksize * 2 ) ) +
				        ( i *  ( tracksize ) );

			fseek (f , file_offset , SEEK_SET);
			fread(trackdata,tracksize,1,f);

			if(tracksize == ( 8192 + ( 1024 * 3 ) + 512 + 256 ) )
			{
				ptr=0;
				k=0;
				sectorconfig[k].sector=(unsigned char)k+1;
				sectorconfig[k].cylinder = (unsigned char)j;
				sectorconfig[k].head = (unsigned char)i;
				sectorconfig[k].sectorsize=8192;
				sectorconfig[k].bitrate=floppydisk->floppyBitRate;
				sectorconfig[k].input_data=&trackdata[ptr];
				sectorconfig[k].trackencoding=trackformat;
				sectorconfig[k].gap3=gap3len;
				ptr = ptr + sectorconfig[k].sectorsize;
				k++;

				for(l=0;l<3;l++)
				{
					sectorconfig[k].sector=(unsigned char)k+1;
					sectorconfig[k].cylinder = (unsigned char)j;
					sectorconfig[k].head = (unsigned char)i;
					sectorconfig[k].sectorsize=1024;
					sectorconfig[k].bitrate=floppydisk->floppyBitRate;
					sectorconfig[k].input_data=&trackdata[ptr];
					sectorconfig[k].trackencoding=trackformat;
					sectorconfig[k].gap3=gap3len;
					ptr = ptr + sectorconfig[k].sectorsize;
					k++;
				}

				sectorconfig[k].sector=(unsigned char)k+1;
				sectorconfig[k].cylinder = (unsigned char)j;
				sectorconfig[k].head = (unsigned char)i;
				sectorconfig[k].sectorsize=512;
				sectorconfig[k].bitrate=floppydisk->floppyBitRate;
				sectorconfig[k].input_data=&trackdata[ptr];
				sectorconfig[k].trackencoding=trackformat;
				sectorconfig[k].gap3=gap3len;
				ptr = ptr + sectorconfig[k].sectorsize;
				k++;

				sectorconfig[k].sector=(unsigned char)k+1;
				sectorconfig[k].cylinder = (unsigned char)j;
				sectorconfig[k].head = (unsigned char)i;
				sectorconfig[k].sectorsize=256;
				sectorconfig[k].bitrate=floppydisk->floppyBitRate;
				sectorconfig[k].input_data=&trackdata[ptr];
				sectorconfig[k].trackencoding=trackformat;
				sectorconfig[k].gap3=gap3len;
				ptr = ptr + sectorconfig[k].sectorsize;
				k++;
			}
			else
			{
				ptr=0;
				for(k=0;k<5;k++)
				{
					sectorconfig[k].sector=(unsigned char)k+1;
					sectorconfig[k].cylinder = (unsigned char)j;
					sectorconfig[k].head = (unsigned char)i;
					sectorconfig[k].sectorsize=2048;
					sectorconfig[k].bitrate=floppydisk->floppyBitRate;
					sectorconfig[k].input_data=&trackdata[ptr];
					sectorconfig[k].trackencoding=trackformat;
					sectorconfig[k].gap3=gap3len;
					ptr = ptr + sectorconfig[k].sectorsize;
				}
				sectorconfig[k].sector=(unsigned char)k+1;
				sectorconfig[k].cylinder = (unsigned char)j;
				sectorconfig[k].head = (unsigned char)i;
				sectorconfig[k].sectorsize=1024;
				sectorconfig[k].bitrate=floppydisk->floppyBitRate;
				sectorconfig[k].input_data=&trackdata[ptr];
				sectorconfig[k].trackencoding=trackformat;
				sectorconfig[k].gap3=gap3len;
				ptr = ptr + sectorconfig[k].sectorsize;
				k++;
				sectorconfig[k].sector=(unsigned char)k+1;
				sectorconfig[k].cylinder = (unsigned char)j;
				sectorconfig[k].head = (unsigned char)i;
				sectorconfig[k].sectorsize=256;
				sectorconfig[k].bitrate=floppydisk->floppyBitRate;
				sectorconfig[k].input_data=&trackdata[ptr];
				sectorconfig[k].trackencoding=trackformat;
				sectorconfig[k].gap3=gap3len;
				ptr = ptr + sectorconfig[k].sectorsize;
				k++;
			}

			currentcylinder->sides[i]=tg_generateTrackEx(floppydisk->floppySectorPerTrack,sectorconfig,1,0,500000,300,trackformat,0,2500|NO_SECTOR_UNDER_INDEX,-2500);

		}
	}

	free(trackdata);

	floppycontext->hxc_printf(MSG_INFO_1,"track file successfully loaded and encoded!");
	hxc_fclose(f);
	return HXCFE_NOERROR;
}

int System24_libGetPluginInfo(HXCFLOPPYEMULATOR* floppycontext,unsigned long infotype,void * returnvalue)
{

	static const char plug_id[]="SYSTEM_24";
	static const char plug_desc[]="System 24 loader";
	static const char plug_ext[]="s24";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)	System24_libIsValidDiskFile,
		(LOADDISKFILE)		System24_libLoad_DiskFile,
		(WRITEDISKFILE)		0,
		(GETPLUGININFOS)	System24_libGetPluginInfo
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
