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
// File : emuii_raw_loader.c
// Contains: EmuII floppy image loader
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

#include "tracks/emuii_track.h"
#include "emuii_raw_loader.h"

#include "libhxcadaptor.h"

int EMUII_RAW_libIsValidDiskFile(HXCFLOPPYEMULATOR* floppycontext,char * imgfile)
{
	int filesize;

	floppycontext->hxc_printf(MSG_DEBUG,"EMUII_RAW_libIsValidDiskFile");

	if( hxc_checkfileext(imgfile,"emuiifd") || hxc_checkfileext(imgfile,"sp1200fd") )
	{

		filesize=hxc_getfilesize(imgfile);
		if(filesize<0) 
			return HXCFE_ACCESSERROR;

		if(filesize==(0xE00*2*80))
		{
			floppycontext->hxc_printf(MSG_DEBUG,"EMUII_RAW_libIsValidDiskFile : EmuII/SP1200 raw file !");
			return HXCFE_VALIDFILE;
		}
		else
		{
			floppycontext->hxc_printf(MSG_DEBUG,"EMUII_RAW_libIsValidDiskFile : non EmuII/SP1200 raw file !");
			return HXCFE_BADFILE;
		}
	}
	else
	{
		floppycontext->hxc_printf(MSG_DEBUG,"EMUII_RAW_libIsValidDiskFile : non EmuII raw file !");
		return HXCFE_BADFILE;
	}

	return HXCFE_BADPARAMETER;
}

int EMUII_RAW_libLoad_DiskFile(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	FILE * f;
	unsigned short i;
	CYLINDER*      currentcylinder;
	SIDE*          currentside;
	unsigned char  sector_data[0xE00];
	unsigned short tracknumber,sidenumber;
	
	floppycontext->hxc_printf(MSG_DEBUG,"EMUII_RAW_libLoad_DiskFile %s",imgfile);
	
	f=hxc_fopen(imgfile,"rb");
	if(f==NULL) 
	{
		floppycontext->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}
	
	floppydisk->floppyNumberOfTrack=80;
	floppydisk->floppyNumberOfSide=2;
	floppydisk->floppyBitRate=DEFAULT_EMUII_BITRATE;
	floppydisk->floppySectorPerTrack=1;
	floppydisk->floppyiftype=EMU_SHUGART_FLOPPYMODE;

	floppycontext->hxc_printf(MSG_DEBUG,"EmuII File : %d track, %d side, %d bit/s, %d sectors, mode %d",
		floppydisk->floppyNumberOfTrack,
		floppydisk->floppyNumberOfSide,
		floppydisk->floppyBitRate,
		floppydisk->floppySectorPerTrack,
		floppydisk->floppyiftype);


	floppydisk->tracks=(CYLINDER**)malloc(sizeof(CYLINDER*)*floppydisk->floppyNumberOfTrack);
	memset(floppydisk->tracks,0,sizeof(CYLINDER*)*floppydisk->floppyNumberOfTrack);

	for(i=0;i<floppydisk->floppyNumberOfTrack*floppydisk->floppyNumberOfSide;i++)
	{			

		tracknumber=i>>1;
		sidenumber=i&1;
	
		fseek(f,i*0xE00,SEEK_SET);
		fread(&sector_data,0xE00,1,f);

		if(!floppydisk->tracks[tracknumber])
		{
			floppydisk->tracks[tracknumber]=allocCylinderEntry(300,floppydisk->floppyNumberOfSide);
			currentcylinder=floppydisk->tracks[tracknumber];
		}
			

		floppycontext->hxc_printf(MSG_DEBUG,"read track %d side %d at offset 0x%x (0x%x bytes)",
			tracknumber,
			sidenumber,
			(0xE00*tracknumber*2)+(sidenumber*0xE00),
			0xE00);

		currentcylinder->sides[sidenumber]=tg_alloctrack(floppydisk->floppyBitRate,EMU_FM_ENCODING,currentcylinder->floppyRPM,((floppydisk->floppyBitRate/5)*2),2000,-2000,0x00);
		currentside=currentcylinder->sides[sidenumber];					
		currentside->number_of_sector=floppydisk->floppySectorPerTrack;

			
		BuildEmuIITrack(floppycontext,tracknumber,sidenumber,sector_data,currentside->databuffer,&currentside->tracklen,2);
			
	}			
	
	hxc_fclose(f);
	return HXCFE_NOERROR;
}

int EMUII_RAW_libGetPluginInfo(HXCFLOPPYEMULATOR* floppycontext,unsigned long infotype,void * returnvalue)
{

	static const char plug_id[]="EMULATORII";
	static const char plug_desc[]="E-mu Emulator II / SP1200 dsk Loader";
	static const char plug_ext[]="emuiifd";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)	EMUII_RAW_libIsValidDiskFile,
		(LOADDISKFILE)		EMUII_RAW_libLoad_DiskFile,
		(WRITEDISKFILE)		0,
		(GETPLUGININFOS)	EMUII_RAW_libGetPluginInfo
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

