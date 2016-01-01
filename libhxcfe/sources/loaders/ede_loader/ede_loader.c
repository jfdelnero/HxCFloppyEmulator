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
// File : ede_DiskFile.c
// Contains: ede floppy image loader
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

#include "ede_loader.h"

#include "libhxcadaptor.h"

int EDE_libIsValidDiskFile(HXCFE_IMGLDR * imgldr_ctx,char * imgfile)
{
	unsigned char header_buffer[512];
	FILE * f;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"EDE_libIsValidDiskFile");

	if(	hxc_checkfileext(imgfile,"ede") ||
		hxc_checkfileext(imgfile,"eda") ||
		hxc_checkfileext(imgfile,"eds") ||
		hxc_checkfileext(imgfile,"edt") ||
		hxc_checkfileext(imgfile,"edv")
		)
	{

		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"EDE_libIsValidDiskFile : EDE file !");

		f=hxc_fopen(imgfile,"rb");
		if(f==NULL)
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"EDE_libIsValidDiskFile : Cannot open %s !",imgfile);
			return HXCFE_ACCESSERROR;
		}

		fread(header_buffer,0x200,1,f);

		hxc_fclose(f);
		if((header_buffer[0]==0x0D) && (header_buffer[1]==0x0A))
		{
			switch(header_buffer[0x1FF])
			{
				case 0x01:
					imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_0,"EDE_libIsValidDiskFile : Mirage (DD) format");
				break;
				case 0x02:
					imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_0,"EDE_libIsValidDiskFile : SQ-80 (DD) format");
				break;
				case 0x03:
					imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_0,"EDE_libIsValidDiskFile : EPS (DD) format");
				break;
				case 0x04:
					imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_0,"EDE_libIsValidDiskFile : VFX-SD (DD) format");
				break;
				case 0xcb:
					imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_0,"EDE_libIsValidDiskFile : ASR-10 HD format");
				break;
				case 0xcc:
					imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_0,"EDE_libIsValidDiskFile : TS-10/12 HD format");
				break;
				case 0x07:
					imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_0,"EDE_libIsValidDiskFile : TS-10/12 DD format");
				break;
				default:
					imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"EDE_libIsValidDiskFile : Unknow format : %x !",header_buffer[0x1FF]);
					return HXCFE_BADFILE;
				break;
			}
		}

		else
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"EDE_libIsValidDiskFile : Bad header !!");
			return HXCFE_BADFILE;
		}

		return HXCFE_VALIDFILE;
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"EDE_libIsValidDiskFile : non EDE file !");
		return HXCFE_BADFILE;
	}
}



int EDE_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{

	FILE * f;
	unsigned int filesize;
	int i,j,l,k;
	int gap3len,interleave;
	int rpm,sectorsize;
	HXCFE_CYLINDER* currentcylinder;
	unsigned char header_buffer[512];
	int header_offset;
	int blocknum;
	int number_of_block;
	unsigned char bitmask;
	int floppy_buffer_index;
	unsigned char trackformat;
	int skew;
	HXCFE_SECTCFG  * sectorconfig;
	unsigned int sectorsizelayout[32];
	unsigned int sectoridlayout[32];

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"EDE_libLoad_DiskFile %s",imgfile);

	f=hxc_fopen(imgfile,"rb");
	if(f==NULL)
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	fseek (f , 0 , SEEK_END);
	filesize=ftell(f);
	fseek (f , 0 , SEEK_SET);


	fread(header_buffer,0x200,1,f);

	if((filesize!=0) && (header_buffer[0]==0x0D) && (header_buffer[1]==0x0A))
	{

		floppydisk->floppyiftype=GENERIC_SHUGART_DD_FLOPPYMODE;
		sectorsize=512;
		rpm=300;
		trackformat=ISOFORMAT_DD;
		skew=0;
		switch(header_buffer[0x1FF])
		{

			case 0x01:
				imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_0,"Mirage (DD) format");
				header_offset=0xA0;
				sectorsize=1024;
				floppydisk->floppyBitRate=250000;
				floppydisk->floppyNumberOfTrack=80;
				floppydisk->floppyNumberOfSide=1;
				floppydisk->floppySectorPerTrack=6;
				gap3len=255;
				interleave=1;
				for(k=0;k<floppydisk->floppySectorPerTrack;k++)sectorsizelayout[k]=sectorsize;
				for(k=1;k<floppydisk->floppySectorPerTrack;k++)sectoridlayout[k]=k-1;
				sectoridlayout[0]=5;
				sectorsizelayout[0]=512;
				break;

			case 0x02:
				imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_0,"SQ-80 (DD) format");
				header_offset=0xA0;
				sectorsize=1024;
				rpm=290;
				floppydisk->floppyBitRate=250000;
				floppydisk->floppyNumberOfTrack=80;
				floppydisk->floppyNumberOfSide=2;
				floppydisk->floppySectorPerTrack=6;
				gap3len=255;
				interleave=1;
				for(k=0;k<floppydisk->floppySectorPerTrack;k++)sectorsizelayout[k]=sectorsize;
				for(k=1;k<floppydisk->floppySectorPerTrack;k++)sectoridlayout[k]=k-1;
				sectoridlayout[0]=5;
				sectorsizelayout[0]=512;
				break;

			case 0x03:
				imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_0,"EPS (DD) format");
				header_offset=0xA0;
				floppydisk->floppyBitRate=250000;
				floppydisk->floppyNumberOfTrack=80;
				floppydisk->floppyNumberOfSide=2;
				floppydisk->floppySectorPerTrack=10;
				gap3len=36;
				interleave=1;
				skew=2;
				for(k=0;k<floppydisk->floppySectorPerTrack;k++)sectorsizelayout[k]=sectorsize;
				for(k=0;k<floppydisk->floppySectorPerTrack;k++)sectoridlayout[k]=k;
				break;

			case 0x04:
				imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_0,"VFX-SD (DD) format");
				header_offset=0xA0;
				floppydisk->floppyBitRate=250000;
				floppydisk->floppyNumberOfTrack=80;
				floppydisk->floppyNumberOfSide=2;
				floppydisk->floppySectorPerTrack=10;
				gap3len=36;
				interleave=1;
				for(k=0;k<floppydisk->floppySectorPerTrack;k++)sectorsizelayout[k]=sectorsize;
				for(k=0;k<floppydisk->floppySectorPerTrack;k++)sectoridlayout[k]=k;
				break;

			case 0xcb:
				imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_0,"ASR-10 HD format");
				floppydisk->floppyiftype=IBMPC_HD_FLOPPYMODE;
				header_offset=0x60;
				floppydisk->floppyBitRate=500000;
				floppydisk->floppyNumberOfTrack=80;
				floppydisk->floppyNumberOfSide=2;
				floppydisk->floppySectorPerTrack=20;
				// ASR 10 GAP3 verified with an HFE image formatted from one ASR-10
				gap3len = 36;
				interleave=1;
				skew=2;
				for(k=0;k<floppydisk->floppySectorPerTrack;k++)sectorsizelayout[k]=sectorsize;
				for(k=0;k<floppydisk->floppySectorPerTrack;k++)sectoridlayout[k]=k;
				break;

			case 0xcc:
				imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_0,"TS-10/12 HD format");
				floppydisk->floppyiftype=IBMPC_HD_FLOPPYMODE;
				header_offset=0x60;
				floppydisk->floppyBitRate=500000;
				floppydisk->floppyNumberOfTrack=80;
				floppydisk->floppyNumberOfSide=2;
				floppydisk->floppySectorPerTrack=20;
				gap3len=40;
				interleave=1;
				skew=2;
				for(k=0;k<floppydisk->floppySectorPerTrack;k++)sectorsizelayout[k]=sectorsize;
				for(k=0;k<floppydisk->floppySectorPerTrack;k++)sectoridlayout[k]=k;
				break;

			case 0x07:
				floppydisk->floppyiftype=IBMPC_DD_FLOPPYMODE;
				imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_0,"TS-10/12 DD format");
				header_offset=0xA0;
				floppydisk->floppyBitRate=250000;
				floppydisk->floppyNumberOfTrack=80;
				floppydisk->floppyNumberOfSide=2;
				floppydisk->floppySectorPerTrack=10;
				gap3len=40;
				interleave=1;
				skew=2;
				for(k=0;k<floppydisk->floppySectorPerTrack;k++)sectorsizelayout[k]=sectorsize;
				for(k=0;k<floppydisk->floppySectorPerTrack;k++)sectoridlayout[k]=k;
				break;

			default:
				imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Unknow format : %x !",header_buffer[0x1FF]);
				hxc_fclose(f);
				return HXCFE_BADFILE;
			break;
		}

		number_of_block=floppydisk->floppyNumberOfTrack*floppydisk->floppyNumberOfSide*floppydisk->floppySectorPerTrack;

		sectorconfig=malloc(sizeof(HXCFE_SECTCFG) * floppydisk->floppySectorPerTrack);
		memset(sectorconfig,0,sizeof(HXCFE_SECTCFG) * floppydisk->floppySectorPerTrack);

		floppy_buffer_index=0;
		blocknum=0;
		bitmask=0x80;

		floppydisk->tracks=(HXCFE_CYLINDER**)malloc(sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);

		imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"%d tracks, %d side(s), %d sectors/track, gap3:%d, interleave:%d,rpm:%d",floppydisk->floppyNumberOfTrack,floppydisk->floppyNumberOfSide,floppydisk->floppySectorPerTrack,gap3len,interleave,rpm);

		for(j=0;j<floppydisk->floppyNumberOfTrack;j++)
		{

			floppydisk->tracks[j]=allocCylinderEntry(rpm,floppydisk->floppyNumberOfSide);
			currentcylinder=floppydisk->tracks[j];

			for(i=0;i<floppydisk->floppyNumberOfSide;i++)
			{
				hxcfe_imgCallProgressCallback(imgldr_ctx,(j<<1) + (i&1),floppydisk->floppyNumberOfTrack*2 );

				memset(sectorconfig,0,sizeof(HXCFE_SECTCFG)*floppydisk->floppySectorPerTrack);
				for(k=0;k<floppydisk->floppySectorPerTrack;k++)
				{
					sectorconfig[k].head=i;
					sectorconfig[k].cylinder=j;
					sectorconfig[k].sector=sectoridlayout[k];
					sectorconfig[k].sectorsize=sectorsizelayout[k];
					sectorconfig[k].bitrate=floppydisk->floppyBitRate;
					sectorconfig[k].gap3=gap3len;
					sectorconfig[k].trackencoding=trackformat;
					sectorconfig[k].input_data=malloc(sectorsize);
					memset(sectorconfig[k].input_data,0,sectorsize);

					if(blocknum<number_of_block)
					{
						if(!(header_buffer[header_offset]&bitmask))
						{
							imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"T:%.3d S:%d Sector:%.2d Size:%.4d File offset: 0x%.8x",j,i,sectorconfig[k].sector,sectorconfig[k].sectorsize,ftell(f));
							fread(sectorconfig[k].input_data,sectorsize,1,f);
						}
						else
						{
							imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"T:%.3d S:%d Sector:%.2d Size:%.4d File offset: ----------",j,i,sectorconfig[k].sector,sectorconfig[k].sectorsize);
							for(l=0;l<(sectorconfig[k].sectorsize/2);l++)
							{
								sectorconfig[k].input_data[(l*2)]=0x6D;
								sectorconfig[k].input_data[(l*2)+1]=0xB6;
							}
						}
						bitmask=bitmask>>1;
						if(!bitmask)
						{
							header_offset++;
							bitmask=0x80;
						}
						blocknum++;
					}
					else
					{
						imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"T:%.3d S:%d Sector:%.2d Size:%.4d",j,i,sectorconfig[k].sector,sectorconfig[k].sectorsize);
						for(l=0;l<(sectorconfig[k].sectorsize/2);l++)
						{
							sectorconfig[k].input_data[(l*2)]=0x6D;
							sectorconfig[k].input_data[(l*2)+1]=0xB6;
						}
					}
				}

				currentcylinder->sides[i]=tg_generateTrackEx(floppydisk->floppySectorPerTrack,sectorconfig,interleave,(unsigned char)(((j<<1)|(i&1))*skew),floppydisk->floppyBitRate,rpm,trackformat,128,2500,-2500);

				for(k=0;k<floppydisk->floppySectorPerTrack;k++)
				{
					free(sectorconfig[k].input_data);
				}
			}
		}

		free(sectorconfig);

		imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"track file successfully loaded and encoded!");
		hxc_fclose(f);

		return HXCFE_NOERROR;
	}

	imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"BAD EDE file!");
	hxc_fclose(f);
	return HXCFE_BADFILE;
}

int EDE_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{

	static const char plug_id[]="ENSONIQ_EDE";
	static const char plug_desc[]="ENSONIQ EDE Loader";
	static const char plug_ext[]="ede";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)	EDE_libIsValidDiskFile,
		(LOADDISKFILE)		EDE_libLoad_DiskFile,
		(WRITEDISKFILE)		0,
		(GETPLUGININFOS)	EDE_libGetPluginInfo
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

