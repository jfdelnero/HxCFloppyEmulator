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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "types.h"

#include "version.h"

#include "h17_format.h"

#include "types.h"

#include "internal_libhxcfe.h"
#include "tracks/track_generator.h"
#include "libhxcfe.h"
#include "h17_loader.h"
#include "h17_writer.h"
#include "tracks/sector_extractor.h"
#include "libhxcadaptor.h"

int write_meta_data_track(FILE * f,HXCFE_SECTORACCESS* ss,int32_t startidsector,int32_t sectorpertrack,int32_t trk,int32_t side,int32_t sectorsize,int32_t tracktype, int * badsect, int * missingsect )
{
	int sect;
	HXCFE_SECTCFG * scfg;

	h17_sect_metadata sect_meta;

	for( sect = 0 ; sect < sectorpertrack ; sect++ )
	{
		memset(&sect_meta,0,sizeof(sect_meta));

		scfg = hxcfe_searchSector ( ss, trk, side, startidsector + sect, tracktype );
		if( scfg )
		{
			if( scfg->use_alternate_data_crc || !scfg->input_data )
			{
				*badsect = *badsect + 1;

				if(!scfg->input_data)
					sect_meta.Sector_Status |= 0x10;

				if(scfg->use_alternate_data_crc)
					sect_meta.Sector_Status |= 0x20;
			}

			if( scfg->cylinder != trk )
			{
				sect_meta.Sector_Status |= 0x02;
			}

			sect_meta.Track = scfg->cylinder;
			sect_meta.Sector = scfg->sector;
			sect_meta.Data_Checksum = scfg->data_crc;
			sect_meta.Header_Checksum = scfg->header_crc;
			sect_meta.DSync = scfg->alternate_datamark;
			sect_meta.Volume = (scfg->alternate_addressmark>>8) & 0xFF;
			sect_meta.HSync  =  scfg->alternate_addressmark & 0xFF;
			sect_meta.Valid_Bytes = BIGENDIAN_WORD(256);
			sect_meta.Offset = BIGENDIAN_DWORD( 256 + (((trk * sectorpertrack) + sect) * sectorsize) ) ;

			hxcfe_freeSectorConfig( ss , scfg );
		}
		else
		{
			*missingsect = *missingsect + 1;

			sect_meta.Track = trk;
			sect_meta.Sector = sect;
			sect_meta.Data_Checksum = 0x0000;
			sect_meta.Header_Checksum = 0x0000;
			sect_meta.DSync = 0x00;
			sect_meta.Volume = 0x00;
			sect_meta.HSync  =  0x00;
			sect_meta.Valid_Bytes = 0;
			sect_meta.Offset = 0;
			sect_meta.Sector_Status = 0x59;
		}

		fwrite(&sect_meta,sizeof(sect_meta),1,f);
	}

	return 0;
}

int write_meta_data(HXCFE_IMGLDR * imgldr_ctx,FILE * f,HXCFE_FLOPPY * fp,int32_t startidsector,int32_t sectorpertrack,int32_t nboftrack,int32_t nbofside,int32_t sectorsize,int32_t tracktype,int32_t sidefilelayout)
{
	int trk,side;
	HXCFE_SECTORACCESS* ss;
	int badsect,missingsect;

	badsect = 0;
	missingsect = 0;

	if(f && fp)
	{
		ss = hxcfe_initSectorAccess( imgldr_ctx->hxcfe, fp );
		if(ss)
		{
			for( trk = 0 ; trk < nboftrack ; trk++ )
			{
				for( side = 0 ; side < nbofside; side++ )
				{
					write_meta_data_track(f,ss,startidsector,sectorpertrack,trk,side,sectorsize,tracktype,&badsect,&missingsect );
				}
			}

			hxcfe_deinitSectorAccess(ss);
		}
	}

	if(badsect || missingsect)
		return HXCFE_FILECORRUPTED;
	else
		return HXCFE_NOERROR;
}

// Main writer function
int H17_libWrite_DiskFile(HXCFE_IMGLDR* imgldr_ctx,HXCFE_FLOPPY * floppy,char * filename)
{
	int nbsector;
	int nbtrack;
	int nbside;
	int sectorsize;

	int writeret;
	int offset;
	int offset2;

	FILE * h8dfile;

	unsigned int sectorcnt_s0;
	unsigned int sectorcnt_s1;

	h17_block blk;
	h17_DskF DskF;
	h17_Parm Parm;
	h17_header hdr;

	struct tm * ts;
	time_t currenttime;

	char str_tmp[512];

	sectorsize = 256;

	hxcfe_imgCallProgressCallback(imgldr_ctx,0,floppy->floppyNumberOfTrack*2 );

	imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"Write H17 Heathkit file %s...",filename);

	sectorcnt_s0 = count_sector(imgldr_ctx->hxcfe,floppy,0,0,0,sectorsize,HEATHKIT_HS_FM_ENCODING,0x0000);
	sectorcnt_s1 = count_sector(imgldr_ctx->hxcfe,floppy,0,0,1,sectorsize,HEATHKIT_HS_FM_ENCODING,0x0000);

	writeret = HXCFE_ACCESSERROR;

	h8dfile = hxc_fopen(filename,"wb");
	if(h8dfile)
	{
		memcpy((void*)&hdr.file_tag,"H17D",4);
		memcpy((void*)&hdr.version,"200",3);
		hdr.check = 0xFF;
		fwrite(&hdr,sizeof(hdr),1,h8dfile);

		if(sectorcnt_s0 != 10)
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"Error : Disk format doesn't match...",filename);
			return HXCFE_FILECORRUPTED;
		}

		nbtrack = 40;
		while(nbtrack && !count_sector(imgldr_ctx->hxcfe,floppy,0,nbtrack-1,0,sectorsize,HEATHKIT_HS_FM_ENCODING,0x0000))
		{
			nbtrack--;
		}

		nbside = 1;
		if(sectorcnt_s1)
			nbside = 2;

		blk.id = BLKID_DskF;
		blk.lenght = BIGENDIAN_DWORD(sizeof(DskF));
		fwrite(&blk,sizeof(blk),1,h8dfile);
		DskF.Tracks = nbtrack;
		DskF.Sides = nbside;
		DskF.ReadOnly = 0;
		fwrite(&DskF,sizeof(DskF),1,h8dfile);

		blk.id = BLKID_Parm;
		blk.lenght = BIGENDIAN_DWORD(sizeof(Parm));
		fwrite(&blk,sizeof(blk),1,h8dfile);
		Parm.Distribution_Disk = 0;
		Parm.Source_of_Header_Data = 0;
		fwrite(&Parm,sizeof(Parm),1,h8dfile);

		sprintf(str_tmp,"HxC Floppy Emulator software v%s\nhttps://hxc2001.com",STR_FILE_VERSION2);
		blk.id = BLKID_Prog;
		blk.lenght = BIGENDIAN_DWORD(strlen(str_tmp));
		fwrite(&blk,sizeof(blk),1,h8dfile);
		fwrite(&str_tmp,strlen(str_tmp),1,h8dfile);

		strcpy(str_tmp,"");
		blk.id = BLKID_Imgr;
		blk.lenght = BIGENDIAN_DWORD(strlen(str_tmp));
		fwrite(&blk,sizeof(blk),1,h8dfile);
		fwrite(&str_tmp,strlen(str_tmp),1,h8dfile);

		currenttime=time (NULL);
		ts=localtime(&currenttime);

		sprintf(str_tmp,"%.4d-%.2d-%.2dT%.2d:%.2d:%.2d,000Z",ts->tm_year+1900,ts->tm_mon,ts->tm_mday,ts->tm_hour,ts->tm_min,ts->tm_sec);
		blk.id = BLKID_Date;
		blk.lenght = BIGENDIAN_DWORD(strlen(str_tmp));
		fwrite(&blk,sizeof(blk),1,h8dfile);
		fwrite(&str_tmp,strlen(str_tmp),1,h8dfile);

		offset = ftell(h8dfile);

		if( (offset + sizeof(blk)) & 0xFF )
		{
			blk.id = BLKID_Padd;
			offset += sizeof(blk);
			blk.lenght = BIGENDIAN_DWORD( (0x100 - ((offset + sizeof(blk)) & 0xFF)) & 0xFF );
			fwrite(&blk,sizeof(blk),1,h8dfile);
			memset(str_tmp,0,sizeof(str_tmp));
			fwrite(&str_tmp,BIGENDIAN_DWORD(blk.lenght),1,h8dfile);
		}

		blk.id = BLKID_H8DB;
		blk.lenght = 0;
		fwrite(&blk,sizeof(blk),1,h8dfile);

		offset = ftell(h8dfile);

		nbsector = sectorcnt_s0;

		imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"%d sectors (%d bytes), %d tracks, %d sides...",nbsector,sectorsize,nbtrack,nbside);

		writeret = write_raw_file(imgldr_ctx,h8dfile,floppy,0,nbsector,nbtrack,nbside,sectorsize,HEATHKIT_HS_FM_ENCODING,1);

		offset2 = ftell(h8dfile);
		// update size
		fseek(h8dfile,offset - sizeof(blk),SEEK_SET);
		blk.lenght = BIGENDIAN_DWORD( offset2 - offset );
		fwrite(&blk,sizeof(blk),1,h8dfile);
		fseek(h8dfile,0,SEEK_END);

		blk.id = BLKID_SecM;
		blk.lenght = 0;
		fwrite(&blk,sizeof(blk),1,h8dfile);

		offset = ftell(h8dfile);

		writeret = write_meta_data(imgldr_ctx,h8dfile,floppy,0,nbsector,nbtrack,nbside,sectorsize,HEATHKIT_HS_FM_ENCODING,1);

		offset2 = ftell(h8dfile);

		// update size
		fseek(h8dfile,offset - sizeof(blk),SEEK_SET);
		blk.lenght = BIGENDIAN_DWORD( offset2 - offset );
		fwrite(&blk,sizeof(blk),1,h8dfile);
		fseek(h8dfile,0,SEEK_END);


		hxc_fclose(h8dfile);
	}

	return writeret;
}
