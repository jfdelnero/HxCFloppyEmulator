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
// File : sdd_speccydos_loader.c
// Contains: SpeccyDos SDD floppy image loader
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

#include "sdd_speccydos_loader.h"
#include "sdd_speccydos_writer.h"

#include "sddfileformat.h"

#include "libhxcadaptor.h"

int sdd_getfloppyconfig(unsigned char * img,unsigned int filesize,unsigned char * density,unsigned short *numberoftrack,unsigned char *numberofside,unsigned short *numberofsectorpertrack,unsigned char *gap3len,unsigned char *interleave)
{
	sddfileformats_t  * uimg;

	uimg=(sddfileformats_t *)img;

	if(!strncmp((char*)uimg->SIGN,"TRKY2",5))
	{
		*gap3len = 64;
		*interleave = 1;
		*numberofsectorpertrack = uimg->nb_sector_per_track;
		*numberofside = ((uimg->nb_track_side >> 7) & 1) + 1;
		*numberoftrack = uimg->nb_track_side & 0x7F;

		if(!(uimg->density & 1))
			*density = 0xFF;
		else
			*density = 0x00;

		return 1;
	}
	else
	{
		return 0;
	}
}


int SDDSpeccyDos_libIsValidDiskFile(HXCFLOPPYEMULATOR* floppycontext,char * imgfile)
{
	int filesize;
	unsigned char buffer[256];
	FILE * f;
	unsigned char density;
	unsigned short NumberOfTrack;
	unsigned char NumberOfSide;
	unsigned short SectorPerTrack;
	unsigned char gap3len;
	unsigned char interleave;

	floppycontext->hxc_printf(MSG_DEBUG,"SDDSpeccyDos_libIsValidDiskFile");

	if(hxc_checkfileext(imgfile,"sdd"))
	{
		filesize=hxc_getfilesize(imgfile);
		if(filesize<0)
		{
			floppycontext->hxc_printf(MSG_ERROR,"SDDSpeccyDos_libIsValidDiskFile : Cannot open %s !",imgfile);
			return HXCFE_ACCESSERROR;
		}

		if(filesize&0xFF)
		{
			floppycontext->hxc_printf(MSG_DEBUG,"SDDSpeccyDos_libIsValidDiskFile : non SDD IMG file - bad file size !");
			return HXCFE_BADFILE;
		}

		f = fopen(imgfile,"rb");
		if(f)
		{
			fread(buffer,256,1,f);

			fclose(f);

			if(sdd_getfloppyconfig(
				buffer,
				filesize,
				&density,
				&NumberOfTrack,
				&NumberOfSide,
				&SectorPerTrack,
				&gap3len,
				&interleave)
				)
			{
				floppycontext->hxc_printf(MSG_INFO_1,"SDD file : filesize:%dkB, %d tracks, %d side(s), %d sectors/track",filesize/1024,NumberOfTrack,NumberOfSide,SectorPerTrack);
			}
			else
			{
				floppycontext->hxc_printf(MSG_DEBUG,"SDDSpeccyDos_libIsValidDiskFile : non SDD file : Wrong boot sector signature!");
			}
		}
		else
		{
			floppycontext->hxc_printf(MSG_ERROR,"SDDSpeccyDos_libIsValidDiskFile : Cannot open %s !",imgfile);
			return HXCFE_ACCESSERROR;
		}

		floppycontext->hxc_printf(MSG_DEBUG,"SDDSpeccyDos_libIsValidDiskFile : SDD file !");
		return HXCFE_VALIDFILE;
	}
	else
	{
		floppycontext->hxc_printf(MSG_DEBUG,"SDDSpeccyDos_libIsValidDiskFile : non SDD file !");
		return HXCFE_BADFILE;
	}

	return HXCFE_BADPARAMETER;
}

int SDDSpeccyDos_libLoad_DiskFile(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * floppydisk,char * imgfile,void * parameters)
{

	FILE * f;
	unsigned int filesize;
	unsigned int file_offset;

	unsigned short i,j;
	unsigned char* trackdata;
	unsigned char  gap3len,interleave,skew,trackformat,density;
	unsigned short rpm;
	unsigned short sectorsize;
	CYLINDER* currentcylinder;

	floppycontext->hxc_printf(MSG_DEBUG,"SDDSpeccyDos_libLoad_DiskFile %s",imgfile);

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

		sectorsize = 256;

		// read the first sector
		trackdata=(unsigned char*)malloc(sectorsize);
		fread(trackdata,sectorsize,1,f);
		if(sdd_getfloppyconfig(
			trackdata,
			filesize,
			&density,
			&floppydisk->floppyNumberOfTrack,
			&floppydisk->floppyNumberOfSide,
			&floppydisk->floppySectorPerTrack,
			&gap3len,
			&interleave)==1
			)
		{

			free(trackdata);

			floppydisk->floppyiftype = GENERIC_SHUGART_DD_FLOPPYMODE;
			floppydisk->floppyBitRate = DEFAULT_DD_BITRATE;

			skew=0;
			if(density)
			{
				trackformat = IBMFORMAT_DD;
			}
			else
			{
				trackformat = IBMFORMAT_SD;
			}

			floppydisk->tracks=(CYLINDER**)malloc(sizeof(CYLINDER*)*floppydisk->floppyNumberOfTrack);

			rpm=300; // normal rpm

			floppycontext->hxc_printf(MSG_INFO_1,"filesize:%dkB, %d tracks, %d side(s), %d sectors/track, gap3:%d, interleave:%d,rpm:%d bitrate:%d",filesize/1024,floppydisk->floppyNumberOfTrack,floppydisk->floppyNumberOfSide,floppydisk->floppySectorPerTrack,gap3len,interleave,rpm,floppydisk->floppyBitRate);
			trackdata=(unsigned char*)malloc(sectorsize*floppydisk->floppySectorPerTrack);

			for(j=0;j<floppydisk->floppyNumberOfTrack;j++)
			{

				floppydisk->tracks[j]=allocCylinderEntry(rpm,floppydisk->floppyNumberOfSide);
				currentcylinder=floppydisk->tracks[j];

				for(i=0;i<floppydisk->floppyNumberOfSide;i++)
				{

					file_offset=(sectorsize*(j*floppydisk->floppySectorPerTrack))+
						        (sectorsize*(floppydisk->floppySectorPerTrack*floppydisk->floppyNumberOfTrack)*i);
					fseek (f , file_offset , SEEK_SET);
					fread(trackdata,sectorsize*floppydisk->floppySectorPerTrack,1,f);

					currentcylinder->sides[i]=tg_generateTrack(trackdata,sectorsize,floppydisk->floppySectorPerTrack,(unsigned char)j,(unsigned char)i,1,interleave,(unsigned char)(((j<<1)|(i&1))*skew),floppydisk->floppyBitRate,currentcylinder->floppyRPM,trackformat,gap3len,0,2500,-2500);
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

	floppycontext->hxc_printf(MSG_ERROR,"file size=%d !?",filesize);
	hxc_fclose(f);
	return HXCFE_BADFILE;
}

int SDDSpeccyDos_libGetPluginInfo(HXCFLOPPYEMULATOR* floppycontext,unsigned long infotype,void * returnvalue)
{

	static const char plug_id[]="SPECCYDOS_SDD";
	static const char plug_desc[]="Speccy DOS SDD File Loader";
	static const char plug_ext[]="sdd";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)	SDDSpeccyDos_libIsValidDiskFile,
		(LOADDISKFILE)		SDDSpeccyDos_libLoad_DiskFile,
		(WRITEDISKFILE)		SDDSpeccyDos_libWrite_DiskFile,
		(GETPLUGININFOS)	SDDSpeccyDos_libGetPluginInfo
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
