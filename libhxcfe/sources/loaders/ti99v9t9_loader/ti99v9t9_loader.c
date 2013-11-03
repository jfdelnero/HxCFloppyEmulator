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
// File : ti99v9t9_loader.c
// Contains: TI99 v9t9 floppy image loader
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

#include "ti99v9t9_loader.h"
#include "ti99v9t9_writer.h"

#include "libhxcadaptor.h"

typedef struct ti99_vib
{
		char    name[10];			  // volume name (10 characters, pad with spaces)
		unsigned char totsecsMSB;     // disk length in sectors (big-endian) (usually 360, 720 or 1440)
		unsigned char totsecsLSB;
		unsigned char secspertrack;   // sectors per track (usually 9 (FM) or 18 (MFM))
		unsigned char id[3];          // String "DSK"
		unsigned char protection;     // 'P' if disk is protected, ' ' otherwise.
		unsigned char tracksperside;  // tracks per side (usually 40)
		unsigned char sides;          // sides (1 or 2)
		unsigned char density;        // 0,1 (FM) or 2,3,4 (MFM)
		unsigned char res[36];        // Empty for traditional disks, or up to 3 directory pointers
		unsigned char abm[200];       // allocation bitmap: a 1 for each sector in use (sector 0 is LSBit of byte 0,
									  // sector 7 is MSBit of byte 0, sector 8 is LSBit of byte 1, etc.)
} ti99_vib;

int getDiskGeometry(FILE * f,unsigned short * numberoftrack,unsigned char * numberofside,unsigned short * numberofsector,unsigned char * skewside0,unsigned char * skewside1,unsigned char * interleave,int * density,unsigned int * bitrate,unsigned short * sectorsize)
{
	int totsecs,filesize;
	ti99_vib vib;

	*sectorsize=256;
	*numberofside=1;
	*numberoftrack=40;
	*skewside0=3;
	*skewside1=6;
	*interleave=4;

	*bitrate=250000;
	*density=1;

	fseek (f , 0 , SEEK_END);
	filesize=ftell(f);
	fseek (f , 0 , SEEK_SET);
	fread(&vib,sizeof(ti99_vib),1,f);
	fseek (f , 0 , SEEK_SET);


	// If we have read the sector successfully, let us parse it
	totsecs = (vib.totsecsMSB << 8) | vib.totsecsLSB;
	*numberofsector = vib.secspertrack;
	if (vib.secspertrack == 0)
	{
		// Some images might be like this, because the original SSSD
		// TI controller always assumes 9.
		*numberofsector = 9;
	}

	switch(*numberofsector)
	{
		case 8:
			*skewside0=0;
			*skewside1=0;
			*interleave=3;
			break;
		case 9:
			*skewside0=3;
			*skewside1=6;
			*interleave=4;
			break;
		case 16:
			*skewside0=0;
			*skewside1=0;
			*interleave=9;
			break;
		case 18:
			*skewside0=0;
			*skewside1=0;
			*interleave=5;
			break;
		case 36:
			*skewside0=0;
			*skewside1=0;
			*interleave=11;
			break;
	}

	*numberoftrack = vib.tracksperside;

	if (*numberoftrack == 0)
	{
		// Some images are like this, because the original SSSD TI controller
		// always assumes 40.
		*numberoftrack = 40;
	}

	*numberofside = vib.sides;

	if (*numberofside == 0)
	{
		// Some images are like this, because the original SSSD TI controller
		// always assumes that tracks beyond 40 are on side 2. */
		*numberofside = totsecs / (*numberofsector * *numberoftrack);
	}

	*bitrate=250000;

	*density=1;
	switch(vib.density)
	{
		case 0:
		case 1:
			*bitrate=250000;
			*density=1;
			break;
		case 2:
			*bitrate=250000;
			*density=2;
			break;
		case 3:
			*bitrate=500000;
			*density=2;
			break;
	}

	if (((*numberofsector * *numberoftrack * *numberofside) == totsecs)
		&& ((vib.density <= 4) && (totsecs >= 2))
		&& (filesize == totsecs*256) && !memcmp(vib.id, "DSK", 3))
	{
		return 1;
	}
	else
	{
		*numberofsector=9;
		*numberofside=1;
		*numberoftrack=40;
		*bitrate=250000;
		*density=1;
		*skewside0=3;
		*skewside1=6;
		*interleave=4;

		switch(filesize)
		{
			case 1*40*9*256:
				*numberofside=1;
				*numberoftrack=40;
				*numberofsector=9;
				*bitrate=250000;
				*density=1;
				*skewside0=3;
				*skewside1=6;
				*interleave=4;
				break;

			case 2*40*9*256:
				// 180kbytes: either DSSD or 18-sector-per-track SSDD.
				// We assume DSSD since DSSD is more common and is supported by
				// the original TI SD disk controller.
				*numberofside=2;
				*numberoftrack=40;
				*numberofsector=9;
				*bitrate=250000;
				*density=1;
				*skewside0=3;
				*skewside1=6;
				*interleave=4;
				break;

			case 1*40*16*256:
				// 160kbytes: 16-sector-per-track SSDD (standard format for TI
				// DD disk controller prototype, and the TI hexbus disk
				// controller?) */
				*numberofside=1;
				*numberoftrack=40;
				*numberofsector=16;
				*bitrate=250000;
				*density=2;
				*skewside0=0;
				*skewside1=0;
				*interleave=9;
				break;

			case 2*40*16*256:
				// 320kbytes: 16-sector-per-track DSDD (standard format for TI
				// DD disk controller prototype, and TI hexbus disk
				// controller?)
				*numberofside=2;
				*numberoftrack=40;
				*numberofsector=16;
				*bitrate=250000;
				*density=2;
				*skewside0=0;
				*skewside1=0;
				*interleave=9;
				break;

			case 2*40*18*256:
				//  360kbytes: 18-sector-per-track DSDD (standard format for most
				// third-party DD disk controllers, but reportedly not supported by
				// the original TI DD disk controller prototype)
				*numberofside=2;
				*numberoftrack=40;
				*numberofsector=18;
				*bitrate=250000;
				*density=2;
				*skewside0=0;
				*skewside1=0;
				*interleave=5;
				break;

			case 2*80*18*256:
				// 720kbytes: 18-sector-per-track 80-track DSDD (Myarc only)
				*numberofside=2;
				*numberoftrack=80;
				*numberofsector=18;
				*bitrate=250000;
				*density=2;
				*skewside0=0;
				*skewside1=0;
				*interleave=5;
				break;

			case 2*80*36*256:
				// 1.44Mbytes: DSHD (Myarc only)
				*numberofside=2;
				*numberoftrack=80;
				*numberofsector=36;
				*bitrate=500000;
				*density=2;
				*skewside0=0;
				*skewside1=0;
				*interleave=11;
				break;

			default:
				return 0;
				break;
		}
	}

	return 2;
}


int TI99V9T9_libIsValidDiskFile(HXCFLOPPYEMULATOR* floppycontext,char * imgfile)
{
	FILE * f;
	short numberoftrack,numberofsector;
	char skew0,skew1,interleave,numberofside;
	int density;
	short sectorsize;
	unsigned int bitrate;
	int ret;

	floppycontext->hxc_printf(MSG_DEBUG,"TI99V9T9_libIsValidDiskFile");
	if(imgfile)
	{
		f=hxc_fopen(imgfile,"rb");
		if(f==NULL)
		{
			floppycontext->hxc_printf(MSG_ERROR,"TI99V9T9_libIsValidDiskFile : Cannot open %s !",imgfile);
			return HXCFE_ACCESSERROR;
		}

		ret=getDiskGeometry(f,&numberoftrack,&numberofside,&numberofsector,&skew0,&skew1,&interleave,&density,&bitrate,&sectorsize);

		hxc_fclose(f);

		switch(ret)
		{
			case 1:
				floppycontext->hxc_printf(MSG_DEBUG,"TI99V9T9_libIsValidDiskFile : V9T9 file !");
				return HXCFE_VALIDFILE;
				break;

			case 2:

				if( hxc_checkfileext(imgfile,"v9t9") || hxc_checkfileext(imgfile,"pc99") )
				{
					floppycontext->hxc_printf(MSG_DEBUG,"TI99V9T9_libIsValidDiskFile : V9T9 file !");
					return HXCFE_VALIDFILE;
				}
				else
				{
					floppycontext->hxc_printf(MSG_DEBUG,"TI99V9T9_libIsValidDiskFile : non TI99 V9T9 file!");
					return HXCFE_BADFILE;
				}
				break;

			default:
				floppycontext->hxc_printf(MSG_DEBUG,"TI99V9T9_libIsValidDiskFile : non TI99 V9T9 file!");
				return HXCFE_BADFILE;
				break;

		}

	}

	return HXCFE_BADPARAMETER;
}


int TI99V9T9_libLoad_DiskFile(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * floppydisk,char * imgfile,void * parameters)
{

	FILE * f;
	unsigned int filesize;
	unsigned short i,j,sectorsize;
	unsigned int file_offset;
	unsigned char skew0,skew1,skew,interleave,gap3len;
	unsigned char* trackdata;
	unsigned char trackformat;
	unsigned short rpm;
	int density;
	CYLINDER* currentcylinder;

	floppycontext->hxc_printf(MSG_DEBUG,"TI99V9T9_libLoad_DiskFile %s",imgfile);

	f=hxc_fopen(imgfile,"rb");
	if(f==NULL)
	{
		floppycontext->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	fseek (f , 0 , SEEK_END);
	filesize=ftell(f);
	fseek (f , 0 , SEEK_SET);

	if(getDiskGeometry(f,&floppydisk->floppyNumberOfTrack,&floppydisk->floppyNumberOfSide,&floppydisk->floppySectorPerTrack,&skew0,&skew1,&interleave,&density,&floppydisk->floppyBitRate,&sectorsize))
	{
		floppydisk->floppyiftype=GENERIC_SHUGART_DD_FLOPPYMODE;
		rpm=300; // normal rpm

		if(density==1)
		{
			trackformat=ISOFORMAT_SD;
			gap3len=45;
		}
		else
		{
			trackformat=ISOFORMAT_DD;
			gap3len=24;
		}

		floppydisk->tracks=(CYLINDER**)malloc(sizeof(CYLINDER*)*floppydisk->floppyNumberOfTrack);
		floppycontext->hxc_printf(MSG_INFO_1,"filesize:%dkB, %d tracks, %d side(s), %d sectors/track, rpm:%d",filesize/1024,floppydisk->floppyNumberOfTrack,floppydisk->floppyNumberOfSide,floppydisk->floppySectorPerTrack,rpm);

		trackdata=(unsigned char*)malloc(sectorsize*floppydisk->floppySectorPerTrack);

		for(j=0;j<floppydisk->floppyNumberOfTrack;j++)
		{

			floppydisk->tracks[j]=allocCylinderEntry(rpm,floppydisk->floppyNumberOfSide);
			currentcylinder=floppydisk->tracks[j];

			for(i=0;i<floppydisk->floppyNumberOfSide;i++)
			{
				if(i==0)
				{
					file_offset=(j*floppydisk->floppySectorPerTrack)*sectorsize;
					skew=skew0;
				}
				else
				{
					file_offset=(  floppydisk->floppyNumberOfTrack      *floppydisk->floppySectorPerTrack*sectorsize)+
								(((floppydisk->floppyNumberOfTrack-1)-j)*floppydisk->floppySectorPerTrack*sectorsize);
					skew=skew1;
				}

				floppycontext->hxc_printf(MSG_DEBUG,"Read track [%.3d:%.1X] : Offset:%.8X Size:%d*%d",j,i,file_offset,floppydisk->floppySectorPerTrack,sectorsize);

				fseek (f , file_offset , SEEK_SET);
				fread(trackdata,sectorsize*floppydisk->floppySectorPerTrack,1,f);

				currentcylinder->sides[i]=tg_generateTrack(trackdata,sectorsize,floppydisk->floppySectorPerTrack,(unsigned char)j,(unsigned char)i,0,interleave,(unsigned char)(j*skew),floppydisk->floppyBitRate,currentcylinder->floppyRPM,trackformat,gap3len,0,2500 | NO_SECTOR_UNDER_INDEX,-2500);
			}

		}

		free(trackdata);

		floppycontext->hxc_printf(MSG_INFO_1,"track file successfully loaded and encoded!");

		hxc_fclose(f);
		return HXCFE_NOERROR;
	}

	floppycontext->hxc_printf(MSG_ERROR,"file size=%d !?",filesize);
	hxc_fclose(f);
	return HXCFE_BADFILE;
}

int TI99V9T9_libGetPluginInfo(HXCFLOPPYEMULATOR* floppycontext,unsigned long infotype,void * returnvalue)
{

	static const char plug_id[]="TI994A_V9T9";
	static const char plug_desc[]="TI99 4A V9T9 Loader";
	static const char plug_ext[]="v9t9";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)	TI99V9T9_libIsValidDiskFile,
		(LOADDISKFILE)		TI99V9T9_libLoad_DiskFile,
		(WRITEDISKFILE)		TI99V9T9_libWrite_DiskFile,
		(GETPLUGININFOS)	TI99V9T9_libGetPluginInfo
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

