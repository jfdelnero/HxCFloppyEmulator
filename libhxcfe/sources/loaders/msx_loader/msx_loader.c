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
// File : msx_loader.c
// Contains: MSX IMG floppy image loader
//
// Written by:	DEL NERO Jean Francois
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "types.h"

#include "internal_libhxcfe.h"
#include "tracks/track_generator.h"
#include "libhxcfe.h"

#include "floppy_loader.h"
#include "floppy_utils.h"

#include "msx_loader.h"
#include "msxfileformat.h"

#include "libhxcadaptor.h"

int msx_imggetfloppyconfig(char * filename,unsigned char * img,unsigned int filesize,int  *numberoftrack,int *numberofside,int *numberofsectorpertrack,int *sectorsize,int *gap3len,int *interleave,int *rpm, int *bitrate)
{
	int i;
	int imgmode,nbofside_img;
	//unsigned int numberofsector;
	unsigned char * uimg;
	int conffound,numberofsector;

	uimg=(unsigned char *)img;
	imgmode=0;
	conffound=0;
	uimg=(unsigned char *)img;


	if(!strstr( filename,".img" ))
	{
		nbofside_img=img[0];
		imgmode=1;
		img++;
	}

	conffound=0;
	if(uimg[0x18]<24 && uimg[0x18]>7)
	{

		*rpm=300;
		*numberofsectorpertrack=uimg[0x18];
		*numberofside=uimg[0x1A];
		*gap3len=60;
		*interleave=1;
		*bitrate=250000;
		*sectorsize=512;

		numberofsector=uimg[0x13]+(uimg[0x14]*256);
		*numberoftrack=(numberofsector/(*numberofsectorpertrack*(*numberofside)));

		//	if((unsigned int)((*numberofsectorpertrack) * (*numberoftrack) * (*numberofside) *512)==filesize)
		{
			conffound=1;
		}

	}

	if(conffound==0)
	{
		i=0;
		do
		{

			if(msxfileformats[i].filesize==filesize)
			{
				*numberoftrack=msxfileformats[i].numberoftrack;
				*numberofsectorpertrack=msxfileformats[i].sectorpertrack;
				*numberofside=msxfileformats[i].numberofside;
				*gap3len=msxfileformats[i].gap3len;
				*interleave=msxfileformats[i].interleave;
				*rpm=msxfileformats[i].RPM;
				*bitrate=msxfileformats[i].bitrate;
				*sectorsize=msxfileformats[i].sectorsize;
				conffound=1;
			}
			i++;

		}while(msxfileformats[i].filesize!=0 && conffound==0);

	}
	return conffound;
}


int MSX_libIsValidDiskFile(HXCFE_IMGLDR * imgldr_ctx,char * imgfile)
{
	int conffound,i;
	int filesize;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"MSX_libIsValidDiskFile");

	if( hxc_checkfileext(imgfile,"img") || hxc_checkfileext(imgfile,"dsk") )
	{

		filesize=hxc_getfilesize(imgfile);
		if(filesize<0)
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"MSX_libIsValidDiskFile : Cannot open %s !",imgfile);
			return HXCFE_ACCESSERROR;
		}

		if(filesize&0x1FF)
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"MSX_libIsValidDiskFile : non MSX IMG file - bad file size !");
			return HXCFE_BADFILE;
		}

		i=0;
		conffound=0;
		do
		{
			if((int)msxfileformats[i].filesize==filesize)
			{
				conffound=1;
			}
			i++;
		}while(msxfileformats[i].filesize!=0 && conffound==0);

		if(conffound)
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"MSX_libIsValidDiskFile : MSX IMG file !");
		}
		else
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"MSX_libIsValidDiskFile : non MSX IMG file - bad file size !");
			return HXCFE_BADFILE;
		}
		return HXCFE_VALIDFILE;
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"MSX_libIsValidDiskFile : non MSX IMG file !");
		return HXCFE_BADFILE;
	}
}



int MSX_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{

	FILE * f;
	unsigned int   filesize;
	int i,j;
	unsigned int   file_offset;
	unsigned char* trackdata;
	int gap3len,interleave;
	int rpm;
	int sectorsize;
	int bitrate;
	int trackformat,skew;

	HXCFE_CYLINDER* currentcylinder;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"MSX_libLoad_DiskFile %s",imgfile);

	f=hxc_fopen(imgfile,"rb");
	if(f==NULL)
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	fseek (f , 0 , SEEK_END);
	filesize=ftell(f);
	fseek (f , 0 , SEEK_SET);

	if(filesize!=0)
	{
		sectorsize=512; // IMG file support only 512bytes/sector floppies.

		// read the first sector
		trackdata = (unsigned char*)malloc( sectorsize + 1);
		if( trackdata )
		{
			fread(trackdata,sectorsize+1,1,f);

			if(msx_imggetfloppyconfig(
				imgfile,
				trackdata,
				filesize,
				&floppydisk->floppyNumberOfTrack,
				&floppydisk->floppyNumberOfSide,
				&floppydisk->floppySectorPerTrack,
				&sectorsize,
				&gap3len,
				&interleave,
				&rpm,
				&bitrate)==1
				)
			{

				free(trackdata);
				trackformat=IBMFORMAT_DD;
				floppydisk->floppyBitRate=bitrate;
				floppydisk->floppyiftype=MSX2_DD_FLOPPYMODE;
				floppydisk->tracks=(HXCFE_CYLINDER**)malloc(sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);

				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"rpm %d bitrate:%d track:%d side:%d sector:%d",rpm,bitrate,floppydisk->floppyNumberOfTrack,floppydisk->floppyNumberOfSide,floppydisk->floppySectorPerTrack);

				skew=0;
				trackdata=(unsigned char*)malloc(sectorsize*floppydisk->floppySectorPerTrack);

				for(j=0;j<floppydisk->floppyNumberOfTrack;j++)
				{

					floppydisk->tracks[j]=allocCylinderEntry(rpm,floppydisk->floppyNumberOfSide);
					currentcylinder=floppydisk->tracks[j];

					for(i=0;i<floppydisk->floppyNumberOfSide;i++)
					{
						hxcfe_imgCallProgressCallback(imgldr_ctx,(j<<1) + (i&1),floppydisk->floppyNumberOfTrack*2 );

						file_offset=(sectorsize*(j*floppydisk->floppySectorPerTrack*floppydisk->floppyNumberOfSide))+
									(sectorsize*(floppydisk->floppySectorPerTrack)*i);

						fseek (f , file_offset , SEEK_SET);

						fread(trackdata,sectorsize*floppydisk->floppySectorPerTrack,1,f);

						currentcylinder->sides[i]=tg_generateTrack(trackdata,sectorsize,floppydisk->floppySectorPerTrack,(unsigned char)j,(unsigned char)i,1,interleave,(unsigned char)(((j<<1)|(i&1))*skew),floppydisk->floppyBitRate,currentcylinder->floppyRPM,trackformat,gap3len,0,2500|NO_SECTOR_UNDER_INDEX,-2500);
					}
				}


				imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"track file successfully loaded and encoded!");
			}

			hxc_fclose(f);
			return HXCFE_NOERROR;
		}
		else
		{
			hxc_fclose(f);
			return HXCFE_INTERNALERROR;
		}
	}

	imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"file size=%d !?",filesize);
	hxc_fclose(f);
	return HXCFE_BADFILE;
}

int MSX_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{

	static const char plug_id[]="MSX_DSK";
	static const char plug_desc[]="MSX DSK Loader";
	static const char plug_ext[]="dsk";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)	MSX_libIsValidDiskFile,
		(LOADDISKFILE)		MSX_libLoad_DiskFile,
		(WRITEDISKFILE)		0,
		(GETPLUGININFOS)	MSX_libGetPluginInfo
	};

	return libGetPluginInfo(
			imgldr_ctx,
			infotype,
			returnvalue,
			plug_id,
			plug_desc,
			&plug_funcs,
			plug_ext
			);
}

