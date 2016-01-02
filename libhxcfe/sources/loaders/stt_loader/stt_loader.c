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
// File : stt_loader.c
// Contains: STT floppy image loader
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

#include "stt_loader.h"

#include "sttfileformat.h"

#include "libhxcadaptor.h"

int STT_libIsValidDiskFile(HXCFE_IMGLDR * imgldr_ctx,char * imgfile)
{
	int filesize;
	FILE * f;
	stt_header STTHEADER;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"STT_libIsValidDiskFile");

	if( hxc_checkfileext(imgfile,"stt") )
	{
		f=hxc_fopen(imgfile,"rb");
		if(f==NULL)
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"STT_libIsValidDiskFile : Cannot open %s !",imgfile);
			return HXCFE_ACCESSERROR;
		}

		fseek (f , 0 , SEEK_END);
		filesize=ftell(f);
		fseek (f , 0 , SEEK_SET);

		STTHEADER.stt_signature=0;
		hxc_fread(&STTHEADER,sizeof(stt_header),f);

		hxc_fclose(f);

		if(STTHEADER.stt_signature!=0x4D455453) //"STEM"
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"STT_libIsValidDiskFile : non STT IMG file - bad signature !");
			return HXCFE_BADFILE;
		}

		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"STT_libIsValidDiskFile : STT file !");
		return HXCFE_VALIDFILE;
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"STT_libIsValidDiskFile : non STT file !");
		return HXCFE_BADFILE;
	}
}

int STT_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{

	FILE * f;
	unsigned int filesize;
	int i,j,k;
	unsigned int file_offset;
	int gap3len,interleave;
	int sectorsize,rpm;
	int trackformat;
	uint32_t file_track_list_offset;
	HXCFE_CYLINDER* currentcylinder;
	HXCFE_SIDE* currentside;
	HXCFE_SECTCFG* sectorconfig;
	stt_header STTHEADER;
	stt_track_offset STTTRACKOFFSET;
	stt_track_header STTTRACKHEADER;
	stt_sector       STTSECTOR;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"STT_libLoad_DiskFile %s",imgfile);

	f=hxc_fopen(imgfile,"rb");
	if(f==NULL)
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	fseek (f , 0 , SEEK_END);
	filesize=ftell(f);
	fseek (f , 0 , SEEK_SET);

	STTHEADER.stt_signature=0;
	hxc_fread(&STTHEADER,sizeof(stt_header),f);

	if(STTHEADER.stt_signature!=0x4D455453) //"STEM"
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"non STT IMG file - bad signature !");
		hxc_fclose(f);

		return HXCFE_BADFILE;
	}

	file_track_list_offset=ftell(f);

	floppydisk->floppyNumberOfTrack=STTHEADER.number_of_tracks;
	floppydisk->floppyNumberOfSide=(unsigned char)STTHEADER.number_of_sides;

	floppydisk->floppyBitRate=250000;
	floppydisk->floppyiftype=ATARIST_DD_FLOPPYMODE;
	floppydisk->tracks=(HXCFE_CYLINDER**)malloc(sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);
	memset(floppydisk->tracks,0,sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);

	rpm=300; // normal rpm

	interleave=1;
	gap3len=80;
	sectorsize=512;

	imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"filesize:%dkB, %d tracks, %d side(s), %d sectors/track, gap3:%d, interleave:%d,rpm:%d",filesize/1024,floppydisk->floppyNumberOfTrack,floppydisk->floppyNumberOfSide,floppydisk->floppySectorPerTrack,gap3len,interleave,rpm);

	for(i=0;i<floppydisk->floppyNumberOfSide;i++)
	{
		for(j=0;j<floppydisk->floppyNumberOfTrack;j++)
		{
			hxcfe_imgCallProgressCallback(imgldr_ctx,(j<<1) + (i&1),floppydisk->floppyNumberOfTrack*2 );

			if(!floppydisk->tracks[j])
			{
				floppydisk->tracks[j]=allocCylinderEntry(rpm,floppydisk->floppyNumberOfSide);
				currentcylinder=floppydisk->tracks[j];
			}

			currentcylinder=floppydisk->tracks[j];

			currentcylinder->floppyRPM=rpm;

			fseek (f, file_track_list_offset, SEEK_SET);
			hxc_fread((void*)&STTTRACKOFFSET,sizeof(stt_track_offset),f);
			file_track_list_offset=file_track_list_offset+sizeof(stt_track_offset);

			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Current Track Offset : 0x%.8X, Size: 0x%.8X, Next File Track List Offset : 0x%.8X",STTTRACKOFFSET.track_offset,STTTRACKOFFSET.track_size,file_track_list_offset);

			fseek (f, STTTRACKOFFSET.track_offset, SEEK_SET);

			hxc_fread((void*)&STTTRACKHEADER,sizeof(stt_track_header),f);

			if(!memcmp(&STTTRACKHEADER.stt_track_signature,"TRCK",4))
			{

				imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"Track: %d, Side: %d, Number of sector: %d, Tracks Flags:0x%.8x, Sector Flags:0x%.8x",j,i,STTTRACKHEADER.number_of_sectors,STTTRACKHEADER.tracks_flags,STTTRACKHEADER.sectors_flags);

				trackformat=ISOFORMAT_DD;
				if(STTTRACKHEADER.number_of_sectors==11 || STTTRACKHEADER.number_of_sectors==12)
				{
					trackformat=ISOFORMAT_DD11S;
				}

				sectorconfig=malloc(sizeof(HXCFE_SECTCFG)*STTTRACKHEADER.number_of_sectors);
				memset(sectorconfig,0,sizeof(HXCFE_SECTCFG)*STTTRACKHEADER.number_of_sectors);
				for(k=0;k<STTTRACKHEADER.number_of_sectors;k++)
				{
					hxc_fread((void*)&STTSECTOR,sizeof(stt_sector),f);

					imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"Sector id: %d, Side id: %d, Track id: %d, Sector size:%d",STTSECTOR.sector_nb_id,STTSECTOR.side_nb_id,STTSECTOR.track_nb_id,STTSECTOR.data_len);

					sectorconfig[k].sector=STTSECTOR.sector_nb_id;
					sectorconfig[k].head=STTSECTOR.side_nb_id;
					sectorconfig[k].sectorsize=STTSECTOR.data_len;
					sectorconfig[k].use_alternate_sector_size_id=1;
					sectorconfig[k].alternate_sector_size_id=STTSECTOR.sector_len_code;
					sectorconfig[k].cylinder=STTSECTOR.track_nb_id;
					//sectorconfig[k].use_alternate_header_crc=0x2;
					sectorconfig[k].header_crc=(STTSECTOR.crc_byte_1<<8) | STTSECTOR.crc_byte_2;
					sectorconfig[k].trackencoding=trackformat;
					sectorconfig[k].gap3=255;
					sectorconfig[k].bitrate=floppydisk->floppyBitRate;

					file_offset=ftell(f);

					sectorconfig[k].input_data=malloc(STTSECTOR.data_len);
					fseek (f, STTSECTOR.data_offset + STTTRACKOFFSET.track_offset, SEEK_SET);
					hxc_fread(sectorconfig[k].input_data,STTSECTOR.data_len,f);

					fseek (f, file_offset, SEEK_SET);

				}

				currentside=tg_generateTrackEx((unsigned short)STTTRACKHEADER.number_of_sectors,sectorconfig,interleave,0,floppydisk->floppyBitRate,rpm,trackformat,0,2500 | NO_SECTOR_UNDER_INDEX,-2500);
				currentcylinder->sides[i]=currentside;

				currentside->bitrate=(int32_t)(250000*(float)((float)(currentside->tracklen/2)/(float)50000));


				for(k=0;k<STTTRACKHEADER.number_of_sectors;k++)
				{
					if(sectorconfig[k].input_data)
						free(sectorconfig[k].input_data);
				}

				free(sectorconfig);
			}
			else
			{
				imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"Bad track Header !? : Track: %d, Side: %d",j,i);
				trackformat=ISOFORMAT_DD;

				STTTRACKHEADER.number_of_sectors=0;

				sectorconfig=malloc(sizeof(HXCFE_SECTCFG));
				memset(sectorconfig,0,sizeof(HXCFE_SECTCFG));
				currentside=tg_generateTrackEx((unsigned short)STTTRACKHEADER.number_of_sectors,sectorconfig,interleave,0,floppydisk->floppyBitRate,rpm,trackformat,0,2500 | NO_SECTOR_UNDER_INDEX,-2500);

				currentcylinder->sides[i]=currentside;

				free(sectorconfig);
			}
		}
	}

	imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"track file successfully loaded and encoded!");

	hxc_fclose(f);
	return HXCFE_NOERROR;

}

int STT_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{

	static const char plug_id[]="ATARIST_STT";
	static const char plug_desc[]="ATARI ST STT Loader";
	static const char plug_ext[]="stt";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)	STT_libIsValidDiskFile,
		(LOADDISKFILE)		STT_libLoad_DiskFile,
		(WRITEDISKFILE)		0,
		(GETPLUGININFOS)	STT_libGetPluginInfo
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
