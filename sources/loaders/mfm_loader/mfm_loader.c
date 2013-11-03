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
// File : mfm_loader.c
// Contains: MFM floppy image loader
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

#include "mfm_loader.h"
#include "mfm_format.h"

#include "libhxcadaptor.h"

int MFM_libIsValidDiskFile(HXCFLOPPYEMULATOR* floppycontext,char * imgfile)
{
	FILE *f;
	MFMIMG header;

	floppycontext->hxc_printf(MSG_DEBUG,"MFM_libIsValidDiskFile");

	if(hxc_checkfileext(imgfile,"mfm"))
	{
		f=hxc_fopen(imgfile,"rb");
		if(f==NULL)
			return HXCFE_ACCESSERROR;

		memset(&header,0,sizeof(MFMIMG));

		fread(&header,sizeof(header),1,f);
		hxc_fclose(f);

		if( !strncmp((char*)header.headername,"HXCMFM",6))
		{
			floppycontext->hxc_printf(MSG_DEBUG,"MFM_libIsValidDiskFile : MFM file !");
			return HXCFE_VALIDFILE;
		}
		else
		{
			floppycontext->hxc_printf(MSG_DEBUG,"MFM_libIsValidDiskFile : non MFM file !");
			return HXCFE_BADFILE;
		}
	}
	else
	{
		floppycontext->hxc_printf(MSG_DEBUG,"MFM_libIsValidDiskFile : non MFM file !");
		return HXCFE_BADFILE;
	}

	return HXCFE_BADPARAMETER;
}

int MFM_libLoad_DiskFile(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	FILE * f;
	MFMIMG header;
	MFMTRACKIMG trackdesc;
	unsigned int i;
	CYLINDER* currentcylinder;
	SIDE* currentside;


	floppycontext->hxc_printf(MSG_DEBUG,"MFM_libLoad_DiskFile %s",imgfile);

	f=hxc_fopen(imgfile,"rb");
	if(f==NULL)
	{
		floppycontext->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	memset(&header,0,sizeof(MFMIMG));
	memset(&trackdesc,0,sizeof(MFMTRACKIMG));

	fread(&header,sizeof(header),1,f);

	if(!strncmp((char*)header.headername,"HXCMFM",6))
	{

		floppydisk->floppyNumberOfTrack=header.number_of_track;
		floppydisk->floppyNumberOfSide=header.number_of_side;
		floppydisk->floppyBitRate=header.floppyBitRate*1000;
		floppydisk->floppySectorPerTrack=-1;
		floppydisk->floppyiftype=ATARIST_DD_FLOPPYMODE;

		floppycontext->hxc_printf(MSG_DEBUG,"MFM File : %d track, %d side, %d bit/s, %d sectors, mode %d",
			floppydisk->floppyNumberOfTrack,
			floppydisk->floppyNumberOfSide,
			floppydisk->floppyBitRate,
			floppydisk->floppySectorPerTrack,
			floppydisk->floppyiftype);

		floppydisk->tracks=(CYLINDER**)malloc(sizeof(CYLINDER*)*floppydisk->floppyNumberOfTrack);
		memset(floppydisk->tracks,0,sizeof(CYLINDER*)*floppydisk->floppyNumberOfTrack);

		for(i=0;i<(unsigned int)(floppydisk->floppyNumberOfTrack*floppydisk->floppyNumberOfSide);i++)
		{

			fseek(f,(header.mfmtracklistoffset)+(i*sizeof(trackdesc)),SEEK_SET);
			fread(&trackdesc,sizeof(trackdesc),1,f);
			fseek(f,trackdesc.mfmtrackoffset,SEEK_SET);

			if(!floppydisk->tracks[trackdesc.track_number])
			{
				floppydisk->tracks[trackdesc.track_number]=allocCylinderEntry(header.floppyRPM,floppydisk->floppyNumberOfSide);
				currentcylinder=floppydisk->tracks[trackdesc.track_number];
			}


			floppycontext->hxc_printf(MSG_DEBUG,"read track %d side %d at offset 0x%x (0x%x bytes)",
												trackdesc.track_number,
												trackdesc.side_number,
												trackdesc.mfmtrackoffset,
												trackdesc.mfmtracksize);

			currentcylinder->sides[trackdesc.side_number]=tg_alloctrack(floppydisk->floppyBitRate,UNKNOWN_ENCODING,currentcylinder->floppyRPM,trackdesc.mfmtracksize*8,2500,-2500,0x00);
			currentside=currentcylinder->sides[trackdesc.side_number];
			currentside->number_of_sector=floppydisk->floppySectorPerTrack;

			fread(currentside->databuffer,currentside->tracklen/8,1,f);
		}

		hxc_fclose(f);
		return HXCFE_NOERROR;
	}

	hxc_fclose(f);
	floppycontext->hxc_printf(MSG_ERROR,"bad header");
	return HXCFE_BADFILE;
}

int MFM_libWrite_DiskFile(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * floppy,char * filename);

int MFM_libGetPluginInfo(HXCFLOPPYEMULATOR* floppycontext,unsigned long infotype,void * returnvalue)
{
	static const char plug_id[]="HXCMFM_IMG";
	static const char plug_desc[]="HXC MFM IMG Loader";
	static const char plug_ext[]="mfm";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)	MFM_libIsValidDiskFile,
		(LOADDISKFILE)		MFM_libLoad_DiskFile,
		(WRITEDISKFILE)		MFM_libWrite_DiskFile,
		(GETPLUGININFOS)	MFM_libGetPluginInfo
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
