/*
//
// Copyright (C) 2006-2016 Jean-François DEL NERO
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
// File : vfddat_loader.c
// Contains: Densei Sirius FDE VFD DAT loader.
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
#include "libhxcfe.h"
#include "./tracks/track_generator.h"

#include "floppy_loader.h"
#include "floppy_utils.h"

#include "vfddat_loader.h"

#include "libhxcadaptor.h"

int VFDDAT_libIsValidDiskFile(HXCFE_IMGLDR * imgldr_ctx,char * imgfile)
{
	int filesize;
	unsigned char header[512];
	FILE *f;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"VFDDAT_libIsValidDiskFile");
	if(imgfile)
	{
		f=hxc_fopen(imgfile,"r+b");
		if(f)
		{
			fseek (f , 0 , SEEK_END);
			filesize=ftell(f);
			fseek (f , 0 , SEEK_SET);
			hxc_fread(&header,sizeof(header),f);

			hxc_fclose(f);

			if(!filesize ||(filesize%512))
			{
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"VFDDAT_libIsValidDiskFile : non DAT file ! bad file size !");
				return HXCFE_BADFILE;
			}

			if(strncmp((char*)&header[0x150],"VFD",3))
			{
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"VFDDAT_libIsValidDiskFile : non DAT file ! bad header !");
				return HXCFE_BADFILE;
			}

			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"VFDDAT_libIsValidDiskFile : DAT file !");
			return HXCFE_VALIDFILE;
		}

		if(hxc_checkfileext( imgfile,"dat"))
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"VFDDAT_libIsValidDiskFile : DAT file !");
			return HXCFE_VALIDFILE;
		}

		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"VFDDAT_libIsValidDiskFile : non DAT file !");
		return HXCFE_BADFILE;

	}

	return HXCFE_BADPARAMETER;
}


static HXCFE_SIDE* VFDpatchtrack(HXCFE* floppycontext,unsigned char * trackdata, unsigned char * trackclk,unsigned int tracklen,uint32_t * tracktotalsize, unsigned char *header,int s)
{
	int i,j;
	unsigned int nbofsector,lastptr,lastdensity,tracksize,bitrate,k;
	unsigned char * track_density;
	unsigned char trackformat;
	unsigned long * idamoffset;

	HXCFE_SIDE* currentside;
	track_generator tg;

	lastptr=0;
	bitrate=500000;

	memset(trackclk,0xFF,tracklen);

	idamoffset = (unsigned long*)header;

	nbofsector=0;
	k=0;
	track_density=malloc(tracklen+4);
	
	memset(track_density,ISOFORMAT_DD,tracklen);
	lastdensity=ISOFORMAT_DD;
	trackformat=ISOFORMAT_DD;

	i = 0;
	while(idamoffset[i])
	{
		floppycontext->hxc_printf(MSG_DEBUG,"IDAM Code : 0x%.8X",BIGENDIAN_DWORD(idamoffset[i]));

		j = BIGENDIAN_DWORD(idamoffset[i]);
		if(j<0x3200-4)
		{
			trackclk[j]=0x0A;
			trackclk[j+1]=0x0A;
			trackclk[j+2]=0x0A;
			trackclk[j+3]=0xFF;
		}
		
		i++;
	}

	// alloc the track...
	tracklen = 10680;
	tg_initTrackEncoder(&tg);
	tracksize = tracklen*8 * 2;

	currentside=tg_initTrack(&tg,tracksize,0,trackformat,500000,0,0);
	currentside->number_of_sector=-1;
	k=0;
	do
	{
		pushTrackCode(&tg,trackdata[k],trackclk[k],currentside,track_density[k]);
		k++;
	}while(k<tracklen);

	free(track_density);

	tg_completeTrack(&tg,currentside,trackformat);

	return currentside;
}

int VFDDAT_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{

	FILE * f;
	unsigned int filesize;
	int i,j;
	unsigned int file_offset;
	uint32_t tracktotalsize;
	unsigned char* trackdata;
	unsigned char* trackclk;
	int rpm,track_len;
	int numberoftrack,numberofside;
	HXCFE_CYLINDER* currentcylinder;
	HXCFE_SIDE* currentside;
	unsigned char header[512];

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"VFDDAT_libLoad_DiskFile %s",imgfile);

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

		hxc_fread(&header,sizeof(header),f);
		floppydisk->floppyBitRate=500000;

		numberofside=2;
		track_len = 0x3200;
		numberoftrack= ((filesize - 512 - 1024) / 0x3200) / 2;

		floppydisk->floppyNumberOfTrack=numberoftrack;
		floppydisk->floppyNumberOfSide=numberofside;
		floppydisk->floppySectorPerTrack=-1;
		floppydisk->floppyiftype=GENERIC_SHUGART_DD_FLOPPYMODE;
		floppydisk->tracks=(HXCFE_CYLINDER**)malloc(sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);

		rpm=360;

		imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"filesize:%dkB, %d tracks, %d side(s), %d sectors/track, rpm:%d",filesize/1024,floppydisk->floppyNumberOfTrack,floppydisk->floppyNumberOfSide,floppydisk->floppySectorPerTrack,rpm);

		trackdata=(unsigned char*)malloc(track_len+16);
		trackclk=(unsigned char*)malloc(track_len+16);

		if( trackdata && trackclk )
		{

			for(j=0;j<floppydisk->floppyNumberOfTrack;j++)
			{

				floppydisk->tracks[j]=allocCylinderEntry(rpm,floppydisk->floppyNumberOfSide);
				currentcylinder=floppydisk->tracks[j];

				for(i=0;i<floppydisk->floppyNumberOfSide;i++)
				{
					hxcfe_imgCallProgressCallback(imgldr_ctx, (j<<1) + (i&1),floppydisk->floppyNumberOfTrack*2);

					file_offset=sizeof(header)+((track_len)*(j*floppydisk->floppyNumberOfSide))+((track_len)*(i&1));
					fseek (f , file_offset , SEEK_SET);
					hxc_fread(trackdata,track_len,f);
					memset(trackclk,0xFF,track_len);

					imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Track %d Side %d Tracklen %d Track Offset:0x%.8x",j,i,track_len,file_offset);

					currentside = VFDpatchtrack(imgldr_ctx->hxcfe,trackdata, trackclk,track_len,&tracktotalsize, (unsigned char*)&header,j);
					currentcylinder->sides[i] = currentside;
					fillindex(-2500,currentside,2500,TRUE,1);

				}
			}
		}

		if(trackclk)
			free(trackclk);

		if(trackdata)
			free(trackdata);

		imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"track file successfully loaded and encoded!");

		hxc_fclose(f);
		return HXCFE_NOERROR;
	}

	imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"file size=%d !?",filesize);
	hxc_fclose(f);
	return HXCFE_BADFILE;
}

int VFDDAT_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{

	static const char plug_id[]="VFD_DAT";
	static const char plug_desc[]="PC98 VFD DAT";
	static const char plug_ext[]="dat";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)	VFDDAT_libIsValidDiskFile,
		(LOADDISKFILE)		VFDDAT_libLoad_DiskFile,
		(WRITEDISKFILE)		0,
		(GETPLUGININFOS)	VFDDAT_libGetPluginInfo
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

