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
// File : sad_loader.c
// Contains: SAD floppy image loader
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

#include "sad_loader.h"
#include "sad_fileformat.h"

int SAD_libIsValidDiskFile(HXCFLOPPYEMULATOR* floppycontext,char * imgfile)
{
	int pathlen;
	FILE * f;
	SAD_HEADER sadh;

	floppycontext->hxc_printf(MSG_DEBUG,"SAD_libIsValidDiskFile");
	if(imgfile)
	{
		pathlen=strlen(imgfile);
		if(pathlen!=0)
		{
				
			f=fopen(imgfile,"rb");
			if(f==NULL) 
			{
				floppycontext->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
				return HXCFE_ACCESSERROR;
			}

			fread(&sadh,sizeof(SAD_HEADER),1,f);
			
			fclose(f);
					
			if(!strncmp(sadh.abSignature,SAD_SIGNATURE,sizeof SAD_SIGNATURE - 1))
			{
				floppycontext->hxc_printf(MSG_DEBUG,"SAD file !");
				return HXCFE_VALIDFILE;
			}
			else
			{
				floppycontext->hxc_printf(MSG_DEBUG,"non SAD file !");
				return HXCFE_BADFILE;
			}
			
		}
	}
	
	return HXCFE_BADPARAMETER;
}



int SAD_libLoad_DiskFile(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	FILE * f;
	unsigned int i,j;
	unsigned int file_offset;
	char* trackdata;
	unsigned char gap3len,interleave;
	unsigned short rpm;
	unsigned short sectorsize;
	unsigned char trackformat,skew;
	CYLINDER* currentcylinder;
	SAD_HEADER sadh;
	
	floppycontext->hxc_printf(MSG_DEBUG,"SAD_libLoad_DiskFile %s",imgfile);
	
	f=fopen(imgfile,"rb");
	if(f==NULL) 
	{
		floppycontext->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}
	
	fread(&sadh,sizeof(SAD_HEADER),1,f);
	if(strncmp(sadh.abSignature,SAD_SIGNATURE,sizeof SAD_SIGNATURE - 1))
	{
		fclose(f);
		floppycontext->hxc_printf(MSG_DEBUG,"non SAD file !");
		return HXCFE_BADFILE;
	}

	floppydisk->floppyNumberOfTrack=sadh.bTracks;
	floppydisk->floppyNumberOfSide=sadh.bSides;
	floppydisk->floppySectorPerTrack=sadh.bSectors;
	sectorsize=sadh.bSectorSizeDiv64*64;
	gap3len=35;
	interleave=1;
	skew=0;
	trackformat=ISOFORMAT_DD;

	floppydisk->floppyBitRate=250000;
	floppydisk->floppyiftype=GENERIC_SHUGART_DD_FLOPPYMODE;
	floppydisk->tracks=(CYLINDER**)malloc(sizeof(CYLINDER*)*floppydisk->floppyNumberOfTrack);

	rpm=300; // normal rpm
	
	floppycontext->hxc_printf(MSG_INFO_1,"%d tracks, %d side(s), %d sectors/track, gap3:%d, interleave:%d,rpm:%d",floppydisk->floppyNumberOfTrack,floppydisk->floppyNumberOfSide,floppydisk->floppySectorPerTrack,gap3len,interleave,rpm);
			
	trackdata=(unsigned char*)malloc(sectorsize*floppydisk->floppySectorPerTrack);
			
	for(j=0;j<floppydisk->floppyNumberOfTrack;j++)
	{		
		floppydisk->tracks[j]=allocCylinderEntry(rpm,floppydisk->floppyNumberOfSide);
		currentcylinder=floppydisk->tracks[j];

		for(i=0;i<floppydisk->floppyNumberOfSide;i++)
		{
			file_offset=(sectorsize*(j*floppydisk->floppySectorPerTrack))+
				(sectorsize*(floppydisk->floppyNumberOfTrack*floppydisk->floppySectorPerTrack)*i) + sizeof(SAD_HEADER) ;
			fseek (f , file_offset , SEEK_SET);
			fread(trackdata,sectorsize*floppydisk->floppySectorPerTrack,1,f);

			currentcylinder->sides[i]=tg_generateTrack(trackdata,sectorsize,floppydisk->floppySectorPerTrack,(unsigned char)j,(unsigned char)i,1,interleave,(unsigned char)(((j<<1)|(i&1))*skew),floppydisk->floppyBitRate,currentcylinder->floppyRPM,trackformat,gap3len,2500|NO_SECTOR_UNDER_INDEX,-2500);
		}
	}

	free(trackdata);
			
	floppycontext->hxc_printf(MSG_INFO_1,"track file successfully loaded and encoded!");
		
	fclose(f);
	return HXCFE_NOERROR;
}

int SAD_libGetPluginInfo(HXCFLOPPYEMULATOR* floppycontext,unsigned long infotype,void * returnvalue)
{

	static const char plug_id[]="SAMCOUPE_SAD";
	static const char plug_desc[]="SAM COUPE SAD Loader";
	static const char plug_ext[]="sad";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)	SAD_libIsValidDiskFile,
		(LOADDISKFILE)		SAD_libLoad_DiskFile,
		(WRITEDISKFILE)		0,
		(GETPLUGININFOS)	SAD_libGetPluginInfo
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

