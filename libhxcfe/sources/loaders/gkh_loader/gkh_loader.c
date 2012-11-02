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
// File : gkh_loader.c
// Contains: Ensoniq GKH floppy image loader
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

#include "gkh_loader.h"
#include "gkh_format.h"

#include "libhxcadaptor.h"

int GKH_libIsValidDiskFile(HXCFLOPPYEMULATOR* floppycontext,char * imgfile)
{
	gkh_header header;
	FILE * f;

	floppycontext->hxc_printf(MSG_DEBUG,"GKH_libIsValidDiskFile");

	if( hxc_checkfileext(imgfile,"gkh"))
	{

		f=hxc_fopen(imgfile,"rb");
		if(f==NULL) 
		{
			floppycontext->hxc_printf(MSG_ERROR,"GKH_libIsValidDiskFile : Cannot open %s !",imgfile);
			return HXCFE_ACCESSERROR;
		}

		fread(&header,sizeof(header),1,f);
		hxc_fclose(f);

		if(!memcmp(&header.header_tag,"TDDFI",5))
		{
			floppycontext->hxc_printf(MSG_DEBUG,"GKH_libIsValidDiskFile : GKH file !");
		}
		else
		{
			floppycontext->hxc_printf(MSG_ERROR,"GKH_libIsValidDiskFile : Bad header !!");
			return HXCFE_BADFILE;
		}

		return HXCFE_VALIDFILE;
	}
	else
	{
		floppycontext->hxc_printf(MSG_DEBUG,"GKH_libIsValidDiskFile : non GKH file !");
		return HXCFE_BADFILE;
	}

	return HXCFE_BADPARAMETER;
}

int GKH_libLoad_DiskFile(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * floppydisk,char * imgfile,void * parameters)
{

	FILE * f;
	unsigned int i,j;
	unsigned char gap3len,interleave,startid;
	unsigned short rpm,sectorsize;
	CYLINDER* currentcylinder;
	int data_offset;
	unsigned char trackformat;
	unsigned char skew;	
	gkh_header header;
	unsigned char tagbuffer[10];
	image_type_tag *img_type_tag;
	image_location_tag *img_location_tag;
	unsigned char * trackdata;
	int file_offset;

	floppycontext->hxc_printf(MSG_DEBUG,"GKH_libLoad_DiskFile %s",imgfile);
	
	f=hxc_fopen(imgfile,"rb");
	if(f==NULL) 
	{
		floppycontext->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}
	
	data_offset=58;
	fread(&header,sizeof(header),1,f);

	if(!memcmp(&header.header_tag,"TDDFI",5) && header.version==1)
	{

		i=0;
		do
		{
			fread(&tagbuffer,10,1,f);
			switch(tagbuffer[0])
			{
				case 0x0A:
					img_type_tag=(image_type_tag *)&tagbuffer;
					sectorsize=img_type_tag->sectorsize;
					floppydisk->floppyNumberOfTrack=img_type_tag->nboftrack;
					floppydisk->floppyNumberOfSide=(unsigned char)img_type_tag->nbofheads;
					floppydisk->floppySectorPerTrack=img_type_tag->nbofsectors;
				break;

				case 0x0B:
					img_location_tag=(image_location_tag *)&tagbuffer;
					data_offset=img_location_tag->fileoffset;
				break;
			}
			i++;
		}while(i<header.numberoftags);

		floppydisk->floppyiftype=GENERIC_SHUGART_DD_FLOPPYMODE;
		floppydisk->floppyBitRate=250000;
		rpm=300;
		
		trackformat=IBMFORMAT_DD;
		
		skew=2;
		startid=0;
		gap3len=255;
		interleave=1;

		floppydisk->tracks=(CYLINDER**)malloc(sizeof(CYLINDER*)*floppydisk->floppyNumberOfTrack);	
		floppycontext->hxc_printf(MSG_INFO_1,"%d tracks, %d side(s), %d sectors/track, gap3:%d, interleave:%d,rpm:%d",floppydisk->floppyNumberOfTrack,floppydisk->floppyNumberOfSide,floppydisk->floppySectorPerTrack,gap3len,interleave,rpm);

		trackdata=(unsigned char*)malloc(sectorsize*floppydisk->floppySectorPerTrack);
			
		for(j=0;j<floppydisk->floppyNumberOfTrack;j++)
		{
				
			floppydisk->tracks[j]=allocCylinderEntry(rpm,floppydisk->floppyNumberOfSide);
			currentcylinder=floppydisk->tracks[j];
							
			for(i=0;i<floppydisk->floppyNumberOfSide;i++)
			{
					
				file_offset=(sectorsize*(j*floppydisk->floppySectorPerTrack*floppydisk->floppyNumberOfSide))+
					        (sectorsize*(floppydisk->floppySectorPerTrack)*i)+
							data_offset;

				fseek (f , file_offset , SEEK_SET);
				fread(trackdata,sectorsize*floppydisk->floppySectorPerTrack,1,f);

				currentcylinder->sides[i]=tg_generateTrack(trackdata,sectorsize,floppydisk->floppySectorPerTrack,(unsigned char)j,(unsigned char)i,startid,interleave,(unsigned char)(((j<<1)|(i&1))*skew),floppydisk->floppyBitRate,currentcylinder->floppyRPM,trackformat,gap3len,0,2500|NO_SECTOR_UNDER_INDEX,-2500);
			}
		}

		free(trackdata);

		floppycontext->hxc_printf(MSG_INFO_1,"track file successfully loaded and encoded!");
		hxc_fclose(f);
		
		return HXCFE_NOERROR;	
	}

	floppycontext->hxc_printf(MSG_ERROR,"BAD GKH file!");
	hxc_fclose(f);
	return HXCFE_BADFILE;
}

int GKH_libGetPluginInfo(HXCFLOPPYEMULATOR* floppycontext,unsigned long infotype,void * returnvalue)
{

	static const char plug_id[]="ENSONIQ_GKH";
	static const char plug_desc[]="ENSONIQ GKH Loader";
	static const char plug_ext[]="gkh";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)	GKH_libIsValidDiskFile,
		(LOADDISKFILE)		GKH_libLoad_DiskFile,
		(WRITEDISKFILE)		0,
		(GETPLUGININFOS)	GKH_libGetPluginInfo
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

