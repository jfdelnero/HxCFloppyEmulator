/*
//
// Copyright (C) 2006-2025 Jean-François DEL NERO
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
// File : h17_loader.c
// Contains: Heathkit H17 floppy image loader
//
// Written by: Jean-François DEL NERO
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

#include "h17_loader.h"
#include "h17_writer.h"

#include "h17_format.h"

#include "libhxcadaptor.h"

int H17_libIsValidDiskFile( HXCFE_IMGLDR * imgldr_ctx, HXCFE_IMGLDR_FILEINFOS * imgfile )
{
	h17_header * hdr;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"H17_libIsValidDiskFile");

	if(hxc_checkfileext(imgfile->path,"h17",SYS_PATH_TYPE))
	{
		hdr = (h17_header *)&imgfile->file_header;

		if( !strncmp((char *)hdr->file_tag, "H17D",4) && hdr->check == 0xFF )
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"H17_libIsValidDiskFile : H17 file !");
			return HXCFE_VALIDFILE;
		}
	}

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"H17_libIsValidDiskFile : non H17 file !");
	return HXCFE_BADFILE;
}

int H17_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	FILE * f;
	unsigned int filesize;
	int i,j,k,trk;
	unsigned int file_offset;
	unsigned char* trackdata;
	int gap3len,interleave;
	int rpm,sectorsize;
	int trackformat,skew;
	HXCFE_CYLINDER* currentcylinder;
	HXCFE_SECTCFG  sectorconfig[16];
	unsigned char VolumeID;
	h17_header hdr;
	h17_block  blk;
	int block_pos;
	uint32_t blkid;
	uint8_t * sector_metadata;
	int size_metadata;

	h17_DskF DskF;
	h17_Parm Parm;
	int size;
	char str_tmp[512];
	h17_sect_metadata * sect_meta;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"H17_libLoad_DiskFile %s",imgfile);

	f = hxc_fopen(imgfile,"rb");
	if( f == NULL )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	filesize = hxc_fgetsize(f);

	hxc_fread(&hdr,sizeof(h17_header),f);

	if( strncmp((char *)&hdr.file_tag, "H17D",4) || hdr.check != 0xFF )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"H17_libLoad_DiskFile : Bad H17 file ?");
		hxc_fclose(f);

		return HXCFE_BADFILE;
	}

	sector_metadata = NULL;
	size_metadata = 0;

	memset((void*)&DskF,0,sizeof(DskF));
	memset((void*)&Parm,0,sizeof(Parm));

	// Block parsing loop
	do
	{
		block_pos = ftell(f);

		memset((h17_block*)&blk,0,sizeof(h17_block));
		hxc_fread((void*)&blk,sizeof(h17_block),f);

		memcpy((void*)&blkid,(void*)&blk.id,4);

		switch(blkid)
		{
			case BLKID_DskF:
				memset((void*)&DskF,0,sizeof(DskF));

				size = BIGENDIAN_DWORD(blk.lenght);
				if(size > sizeof(DskF) )
					size = sizeof(DskF);

				hxc_fread((void*)&DskF,size,f);

				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"H17_libLoad_DiskFile : BLKID_DskF - Sides:%d Tracks:%d RO:%d",DskF.Sides,DskF.Tracks,DskF.ReadOnly);
			break;
			case BLKID_Parm:
				memset((void*)&Parm,0,sizeof(Parm));

				size = BIGENDIAN_DWORD(blk.lenght);
				if(size > sizeof(Parm) )
					size = sizeof(Parm);

				hxc_fread((void*)&Parm,size,f);

				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"H17_libLoad_DiskFile : BLKID_Parm - Distribution_Disk:%d Source_of_Header_Data:%d",Parm.Distribution_Disk,Parm.Source_of_Header_Data);
			break;
			case BLKID_Date:
				memset((void*)&str_tmp,0,sizeof(str_tmp));

				size = BIGENDIAN_DWORD(blk.lenght);
				if(size >= sizeof(str_tmp) )
					size = sizeof(str_tmp) - 1;

				hxc_fread((void*)&str_tmp,size,f);

				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"H17_libLoad_DiskFile : BLKID_Date - %s",str_tmp);
			break;
			case BLKID_Imgr:
				memset((void*)&str_tmp,0,sizeof(str_tmp));

				size = BIGENDIAN_DWORD(blk.lenght);
				if(size >= sizeof(str_tmp) )
					size = sizeof(str_tmp) - 1;

				hxc_fread((void*)&str_tmp,size,f);

				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"H17_libLoad_DiskFile : BLKID_Imgr - %s",str_tmp);
			break;
			case BLKID_Prog:
				memset((void*)&str_tmp,0,sizeof(str_tmp));

				size = BIGENDIAN_DWORD(blk.lenght);
				if(size >= sizeof(str_tmp) )
					size = sizeof(str_tmp) - 1;

				hxc_fread((void*)&str_tmp,size,f);

				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"H17_libLoad_DiskFile : BLKID_Prog - %s",str_tmp);
			break;
			case BLKID_Padd:
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"H17_libLoad_DiskFile : BLKID_Padd");
			break;
			case BLKID_H8DB:
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"H17_libLoad_DiskFile : BLKID_H8DB");
			break;
			case BLKID_Labl:
				memset((void*)&str_tmp,0,sizeof(str_tmp));

				size = BIGENDIAN_DWORD(blk.lenght);
				if(size >= sizeof(str_tmp) )
					size = sizeof(str_tmp) - 1;

				hxc_fread((void*)&str_tmp,size,f);

				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"H17_libLoad_DiskFile : BLKID_Labl - %s",str_tmp);
			break;
			case BLKID_Comm:
				memset((void*)&str_tmp,0,sizeof(str_tmp));

				size = BIGENDIAN_DWORD(blk.lenght);
				if(size >= sizeof(str_tmp) )
					size = sizeof(str_tmp) - 1;

				hxc_fread((void*)&str_tmp,size,f);

				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"H17_libLoad_DiskFile : BLKID_Comm - %s",str_tmp);
			break;
			case BLKID_SecM:
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"H17_libLoad_DiskFile : BLKID_SecM");

				size = BIGENDIAN_DWORD(blk.lenght);
				if(size)
				{
					sector_metadata = (uint8_t*)malloc(size);
					if(sector_metadata)
					{
						memset(sector_metadata,0,size);
						size_metadata = size;

						hxc_fread((void*)sector_metadata,size_metadata,f);
					}
				}

			break;
			default:
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"H17_libLoad_DiskFile : BLKID_???? (0x%.8X)",blk.id);
			break;
		}

		block_pos += (sizeof(h17_block) + BIGENDIAN_DWORD(blk.lenght) );

		fseek(f, block_pos, SEEK_SET);

	}while( block_pos < (filesize - sizeof(h17_block) ) && block_pos >= 0 );

	block_pos = sizeof(h17_header);

	fseek(f, block_pos, SEEK_SET);

	do
	{
		block_pos = ftell(f);

		memset((h17_block*)&blk,0,sizeof(h17_block));
		hxc_fread((void*)&blk,sizeof(h17_block),f);

		memcpy((void*)&blkid,(void*)&blk.id,4);

		switch(blkid)
		{
			case BLKID_H8DB:
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"H17_libLoad_DiskFile : BLKID_H8DB");

				sectorsize = 256;
				trackformat = HEATHKIT_HS_SD;

				gap3len=0;
				interleave=1;
				skew = 5;
				VolumeID = 0x00;
				floppydisk->floppySectorPerTrack = 10;
				floppydisk->floppyNumberOfTrack = DskF.Tracks;
				floppydisk->floppyNumberOfSide = DskF.Sides;

				floppydisk->floppyBitRate = 250000;
				floppydisk->floppyiftype = GENERIC_SHUGART_DD_FLOPPYMODE;

				floppydisk->tracks = (HXCFE_CYLINDER**)malloc(sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);
				if( !floppydisk->tracks )
				{
					hxc_fclose(f);
					return HXCFE_INTERNALERROR;
				}

				memset(floppydisk->tracks,0,sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);

				rpm=300; // normal rpm

				imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"filesize:%dkB, %d tracks, %d side(s), %d sectors/track, gap3:%d, interleave:%d,rpm:%d",filesize/1024,floppydisk->floppyNumberOfTrack,floppydisk->floppyNumberOfSide,floppydisk->floppySectorPerTrack,gap3len,interleave,rpm);

				trackdata = (unsigned char*)malloc(floppydisk->floppySectorPerTrack*sectorsize);
				if( !trackdata )
				{
					hxcfe_freeFloppy(imgldr_ctx->hxcfe, floppydisk );
					hxc_fclose(f);
					return HXCFE_INTERNALERROR;
				}

				// HDOS or CPM image ?
				hxc_fread(trackdata,(floppydisk->floppySectorPerTrack*sectorsize),f);

				for( i = 0x900; i < 0xA00 - 4 ; i++ )
				{
					if( !strncmp( (const char*)&trackdata[i], "HDOS", 4 ) )
					{
						// HDOS disk. Get the volume ID for track 1-39.
						VolumeID = trackdata[0x900];
						break;
					}
				}

				trk = 0;
				for(i=0;i<floppydisk->floppyNumberOfSide;i++)
				{
					for(j=0;j<floppydisk->floppyNumberOfTrack;j++)
					{
						if( !floppydisk->tracks[j] )
						{
							floppydisk->tracks[j]=allocCylinderEntry(rpm,floppydisk->floppyNumberOfSide);
						}

						currentcylinder=floppydisk->tracks[j];

						hxcfe_imgCallProgressCallback(imgldr_ctx,trk,floppydisk->floppyNumberOfTrack*2 );
						trk++;


						file_offset = ( ( floppydisk->floppySectorPerTrack * sectorsize ) * j * floppydisk->floppyNumberOfSide) + \
										( ( floppydisk->floppySectorPerTrack * sectorsize ) * j * i);

						fseek (f , (block_pos + sizeof(h17_block)) + file_offset , SEEK_SET);

						hxc_fread(trackdata,(floppydisk->floppySectorPerTrack*sectorsize),f);

						memset(sectorconfig,0,sizeof(HXCFE_SECTCFG)*floppydisk->floppySectorPerTrack);
						for(k=0;k<floppydisk->floppySectorPerTrack;k++)
						{
							sectorconfig[k].head = i;
							sectorconfig[k].cylinder = j;
							sectorconfig[k].sector = k;
							sectorconfig[k].sectorsize = sectorsize;
							sectorconfig[k].bitrate = floppydisk->floppyBitRate;
							sectorconfig[k].gap3 = gap3len;
							sectorconfig[k].trackencoding = trackformat;
							sectorconfig[k].input_data = &trackdata[k*sectorsize];

							// Use the alternate_addressmark field as Volume field...
							if( j == 0 )
							{
								sectorconfig[k].alternate_addressmark = 0x00;
							}
							else
							{
								sectorconfig[k].alternate_addressmark = ((uint16_t)(VolumeID))<<8;
							}

							sect_meta = NULL;

							if( sector_metadata )
							{
								sect_meta = (h17_sect_metadata *)( sector_metadata + ( ( ( floppydisk->floppySectorPerTrack * 16 ) * j * floppydisk->floppyNumberOfSide) + \
																 ( ( floppydisk->floppySectorPerTrack * 16 ) * j * i) ) + (k * 16));

								sectorconfig[k].use_alternate_addressmark = 0xFF;
								sectorconfig[k].alternate_addressmark = ((sect_meta->HSync) | (((uint16_t)sect_meta->Volume)<<8));
								sectorconfig[k].use_alternate_datamark = 0xFF;
								sectorconfig[k].alternate_datamark = sect_meta->DSync;
								sectorconfig[k].cylinder = sect_meta->Track;
								sectorconfig[k].sector = sect_meta->Sector;
								sectorconfig[k].input_data = &trackdata[k*sectorsize];
								sectorconfig[k].use_alternate_header_crc = 0x02;
								sectorconfig[k].header_crc = sect_meta->Header_Checksum;
								sectorconfig[k].use_alternate_data_crc = 0x02;
								sectorconfig[k].data_crc = sect_meta->Data_Checksum;

								fseek (f , BIGENDIAN_DWORD(sect_meta->Offset) , SEEK_SET);

								hxc_fread(&trackdata[k*sectorsize],sectorsize,f);
							}
						}

						currentcylinder->sides[i] = tg_generateTrackEx(floppydisk->floppySectorPerTrack,sectorconfig,1,(j*skew),floppydisk->floppyBitRate,rpm,trackformat,0,0,0);
					}
				}

				free(trackdata);

				imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"track file successfully loaded and encoded!");

				hxc_fclose(f);

				free(sector_metadata);

				return HXCFE_NOERROR;
			break;

			default:
			break;
		}

		block_pos += (sizeof(h17_block) + BIGENDIAN_DWORD(blk.lenght) );

		fseek(f, block_pos, SEEK_SET);

	}while( block_pos < (filesize - sizeof(h17_block) ) && block_pos >= 0 );

	free(sector_metadata);

	imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"file size=%d !?",filesize);

	hxc_fclose(f);

	return HXCFE_BADFILE;
}

int H17_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]="HEATHKIT";
	static const char plug_desc[]="H17 Heathkit Loader";
	static const char plug_ext[]="h17";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   H17_libIsValidDiskFile,
		(LOADDISKFILE)      H17_libLoad_DiskFile,
		(WRITEDISKFILE)     H17_libWrite_DiskFile,
		(GETPLUGININFOS)    H17_libGetPluginInfo
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
