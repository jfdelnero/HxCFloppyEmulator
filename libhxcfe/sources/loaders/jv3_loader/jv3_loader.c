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
// File : jv3_loader.c
// Contains: JV3 TRS80 floppy image loader
//
// Written by:	Gustavo E A P A Batista using JV1 loader as template
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

#include "jv3_loader.h"
#include "jv3_format.h"

#include "jv3_writer.h"

#include "libhxcadaptor.h"

unsigned char* compute_interleave_tab(unsigned char interleave,unsigned short numberofsector);

unsigned int gbn(int num, int mask) {
	num = num & mask;
	while (mask % 2 == 0) {
		num >>= 1;
		mask >>= 1;
	}
	return num;
}

int JV3_compare (const void * a, const void * b) {
	return ( ((JV3SectorsOffsets*)a)->key - ((JV3SectorsOffsets*)b)->key );
}

JV3SectorsOffsets *JV3_bsearch(unsigned int key, JV3SectorsOffsets *base, size_t num) {
	JV3SectorsOffsets aux;

	aux.key = key;
	return (JV3SectorsOffsets*) bsearch (&aux, base, num, sizeof(JV3SectorsOffsets), JV3_compare);
}

int GetFirstPos(JV3SectorsOffsets *base,int size,unsigned int track_id,unsigned int side_id)
{
	int i;

	i = 0;
	while( i < size)
	{
		if( ( base[i].track_id == track_id ) && ( base[i].side_id == side_id ) )
		{
			return base[i].sector_pos;
		}
		i++;
	}

	return 0;
}

int GetNextPos(JV3SectorsOffsets *base,int size,unsigned int track_id,unsigned int side_id,unsigned int pos)
{
	int i,cur_pos;

	cur_pos = 0;
	while( (cur_pos < size) && base[cur_pos].sector_pos != pos )
	{
		cur_pos++;
	};

	if(cur_pos==size)
		return 0;

	i = cur_pos + 1;
	while( i < size)
	{
		if( ( base[i].track_id == track_id ) && ( base[i].side_id == side_id ) )
		{
			return base[i].sector_pos;
		}
		i++;
	}

	return 0;
}


JV3SectorsOffsets * GetSectorPosition(JV3SectorsOffsets *base,int size,unsigned int pos)
{
	int i;

	i = 0;
	while( i < size)
	{
		if(base[i].sector_pos == pos)
		{
			return &base[i];
		}
		i++;
	}

	return 0;
}


unsigned int JV3_disk_geometry(JV3SectorHeader JV3SH[], int32_t *NumberofSides, int32_t *SectorsperTrack, int32_t *NumberofTracks, int32_t *SectorSize, int32_t *StartIdSector, int32_t *NumberofEntries) {
	int i, total_data = 0;

	*StartIdSector = 255;
	*NumberofEntries = 0;
	*NumberofSides = 0;
	*SectorsperTrack = 0;
	*NumberofTracks = 0;
	*SectorSize = 0;
	for (i=0; i<JV3_HEADER_MAX; i++)
		if (JV3SH[i].track != JV3_FREE) {
			(*NumberofEntries)++;
			if (JV3SH[i].track > *NumberofTracks)
				*NumberofTracks = JV3SH[i].track;
			if (JV3SH[i].sector > *SectorsperTrack)
				*SectorsperTrack = JV3SH[i].sector;
			if (JV3SH[i].sector < *StartIdSector)
				*StartIdSector = JV3SH[i].sector;
			if (gbn(JV3SH[i].flags, JV3_SIDE) > (unsigned int)*NumberofSides)
				*NumberofSides = gbn(JV3SH[i].flags, JV3_SIDE);
			switch (gbn(JV3SH[i].flags, JV3_SIZE)) {
				case JV3_SIZE_USED_256:
					if (*SectorSize == 256 || *SectorSize == 0) {
						*SectorSize = 256;
						total_data += 256;
					} else
						return 0;
					break;
				case JV3_SIZE_USED_128:
					if (*SectorSize == 128 || *SectorSize == 0) {
						*SectorSize = 128;
						total_data += 128;
					} else
						return 0;
					break;
				case JV3_SIZE_USED_1024:
					if (*SectorSize == 1024 || *SectorSize == 0) {
						*SectorSize = 1024;
						total_data += 1024;
					} else
						return 0;
					break;
				case JV3_SIZE_USED_512:
					if (*SectorSize == 512 || *SectorSize == 0) {
						*SectorSize = 512;
						total_data += 512;
					} else
						return 0;
					break;
			}
		}
	if (*StartIdSector == 0)
		(*SectorsperTrack)++;         /* Sectors numered 0..Sectors-1 */
	(*NumberofTracks)++;                  /* Tracks numbered 0..Tracks-1 */
	(*NumberofSides)++;                   /* Sides numbered 0 or 1 */
	return total_data;
}

JV3SectorsOffsets *JV3_offset(JV3SectorHeader JV3SH[], unsigned int NumberofSides, unsigned int SectorsperTrack, unsigned int NumberofTracks, unsigned int NumberofEntries, FILE *jv3_file) {
	JV3SectorsOffsets *SO;
	unsigned int i, offset;
	int pos,sector_pos;

	sector_pos = 1;
	pos = 0;
	offset = ftell(jv3_file);
	SO = (JV3SectorsOffsets *) malloc(sizeof(JV3SectorsOffsets)*NumberofEntries);
	for (i=0; i<JV3_HEADER_MAX; i++) {
		if (JV3SH[i].track != JV3_FREE)
		{
			SO[pos].key = JV3SH[i].track << 16 | JV3SH[i].sector << 8 | gbn(JV3SH[i].flags, JV3_SIDE);
			SO[pos].offset = offset;
			SO[pos].sector_id = JV3SH[i].sector;
			SO[pos].track_id = JV3SH[i].track;
			SO[pos].sector_pos = sector_pos;

			if(JV3SH[i].flags&JV3_ERROR)
			{
				SO[pos].bad_sector = 0xFF;
			}
			else
			{
				SO[pos].bad_sector = 0;
			}

			if(JV3SH[i].flags&JV3_SIDE)
			{
				SO[pos].side_id = 1;
			}
			else
			{
				SO[pos].side_id = 0;
			}
			sector_pos++;

			switch (gbn(JV3SH[i].flags, JV3_SIZE))
			{
				case JV3_SIZE_USED_256:
					SO[pos].size = 256;
					offset += 256;
					break;
				case JV3_SIZE_USED_128:
					SO[pos].size = 128;
					offset += 128;
					break;
				case JV3_SIZE_USED_1024:
					SO[pos].size = 1024;
					offset += 1024;
					break;
				case JV3_SIZE_USED_512:
					SO[pos].size = 512;
					offset += 512;
					break;
			}

			if (gbn(JV3SH[i].flags, JV3_DENSITY))
			{         		/* Double density */

				SO[pos].density=0xFF;

				switch (gbn(JV3SH[i].flags, JV3_DAM))
				{
					case JV3_DAM_FB_DD:
						SO[pos].DAM = 0xFB;
						break;
					case JV3_DAM_F8_DD:
						SO[pos].DAM = 0xF8;
						break;
				}
			}
			else
			{
				SO[pos].density=0x00;

				switch (gbn(JV3SH[i].flags, JV3_DAM)) /* Single density */
				{
					case JV3_DAM_FB_SD:
						SO[pos].DAM = 0xFB;
						break;
					case JV3_DAM_FA_SD:
						SO[pos].DAM = 0xFA;
						break;
					case JV3_DAM_F8_SD:
						SO[pos].DAM = 0xF8;
						break;
					case JV3_DAM_F9_SD:
						SO[pos].DAM = 0xF9;
						break;
				}
  			}
        }
		else
		{
			//SO[pos].density=0x00;

			switch (gbn(JV3SH[i].flags, JV3_SIZE)) {
				case JV3_SIZE_FREE_256:
					offset += 256;
					break;
				case JV3_SIZE_FREE_128:
					offset += 128;
					break;
				case JV3_SIZE_FREE_1024:
					offset += 1024;
					break;
				case JV3_SIZE_FREE_512:
					offset += 512;
					break;
			}
		}
		pos++;
	}
	return SO;
}

int JV3_libIsValidDiskFile(HXCFE_IMGLDR * imgldr_ctx,char * imgfile)
{
	int offset1, offset2;
	int32_t SectorPerTrack, NumberOfTrack, SectorSize, NumberOfEntries;
	unsigned int   total_data;
	int32_t  StartIdSector,NumberOfSide;
	FILE *f;
	JV3SectorHeader sh[JV3_HEADER_MAX];

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"JV3_libIsValidDiskFile");

	if( hxc_checkfileext(imgfile,"jv3") ||
		hxc_checkfileext(imgfile,"dsk")
		)
	{

		f=hxc_fopen(imgfile,"rb");
		if(f==NULL)
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"JV3_libIsValidDiskFile : Cannot open %s !",imgfile);
			return HXCFE_ACCESSERROR;
		}

		hxc_fread(sh, sizeof(JV3SectorHeader)*JV3_HEADER_MAX, f);
       	if ((total_data = JV3_disk_geometry(sh, &NumberOfSide, &SectorPerTrack, &NumberOfTrack, &SectorSize, &StartIdSector, &NumberOfEntries)) != 0)
		{
			offset1 = ftell(f);
			fseek (f , 0 , SEEK_END);
			offset2 = ftell(f);
			hxc_fclose(f);

			if (total_data == (unsigned int)(offset2 - offset1 -1)) {
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"JV3_libIsValidDiskFile : JV3 file !");
				return HXCFE_VALIDFILE;
			} else {
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"JV3_libIsValidDiskFile : non JV3 file !");
				return HXCFE_BADFILE;
			}
		}
		else
		{
			hxc_fclose(f);
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"JV3_libIsValidDiskFile : non JV3 file !");
			return HXCFE_BADFILE;
		}
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"JV3_libIsValidDiskFile : non JV3 file !");
		return HXCFE_BADFILE;
	}
}

int JV3_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{

	FILE * f;
	unsigned int filesize,cur_pos;
	int i,j,k,bitrate;
	int32_t SectorSize, NumberofEntries;
	int32_t gap3len,interleave,StartIdSector;
	int32_t rpm;
	int32_t trackformat;
	int sector_found;

	HXCFE_SECTCFG*	sectorconfig;
	HXCFE_CYLINDER*		currentcylinder;

	JV3SectorHeader sh[JV3_HEADER_MAX];
	JV3SectorsOffsets *pOffset, *SectorsOffsets;
	unsigned char write_protected;
	unsigned int inc;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"JV3_libLoad_DiskFile %s",imgfile);

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
		hxc_fread(sh, sizeof(JV3SectorHeader) * JV3_HEADER_MAX, f);

		JV3_disk_geometry(sh, &floppydisk->floppyNumberOfSide, &floppydisk->floppySectorPerTrack, &floppydisk->floppyNumberOfTrack, &SectorSize, &StartIdSector, &NumberofEntries);

		hxc_fread(&write_protected, sizeof(write_protected), f); // just to jump this infomation


		SectorsOffsets = JV3_offset(sh, floppydisk->floppyNumberOfSide, floppydisk->floppySectorPerTrack, floppydisk->floppyNumberOfTrack, NumberofEntries, f);

		bitrate=250000;
		rpm=300;
		interleave=1;
		gap3len=255;
		trackformat=IBMFORMAT_SD;

		floppydisk->floppyBitRate=bitrate;
		floppydisk->floppyiftype=GENERIC_SHUGART_DD_FLOPPYMODE;
		floppydisk->tracks=(HXCFE_CYLINDER**)malloc(sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);

		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"rpm %d bitrate:%d track:%d side:%d sector:%d",rpm,bitrate,floppydisk->floppyNumberOfTrack,floppydisk->floppyNumberOfSide,floppydisk->floppySectorPerTrack);

		sectorconfig=(HXCFE_SECTCFG*)malloc(sizeof(HXCFE_SECTCFG)*floppydisk->floppySectorPerTrack);
		memset(sectorconfig,0,sizeof(HXCFE_SECTCFG)*floppydisk->floppySectorPerTrack);


		for(j=0;j<floppydisk->floppyNumberOfTrack;j++)
		{

			floppydisk->tracks[j]=allocCylinderEntry(rpm,floppydisk->floppyNumberOfSide);
			currentcylinder=floppydisk->tracks[j];

			for(i=0;i<floppydisk->floppyNumberOfSide;i++)
			{
				hxcfe_imgCallProgressCallback(imgldr_ctx,(j<<1) + (i&1),floppydisk->floppyNumberOfTrack*2 );

				inc = 0;                                    // used to build track data
				memset(sectorconfig,0,sizeof(HXCFE_SECTCFG)*floppydisk->floppySectorPerTrack);
				sector_found=0;

				cur_pos = GetFirstPos(SectorsOffsets,NumberofEntries,j,i);

				for(k=0;k<floppydisk->floppySectorPerTrack;k++)
				{
					pOffset = GetSectorPosition(SectorsOffsets,NumberofEntries,cur_pos);

		    		if (pOffset == NULL)
					{
						inc += SectorSize;
					}
					else
					{

						sectorconfig[sector_found].sectorsize=pOffset->size;
						sectorconfig[sector_found].input_data=malloc(sectorconfig[sector_found].sectorsize);
						memset(sectorconfig[sector_found].input_data,0,sectorconfig[sector_found].sectorsize);

						fseek(f, pOffset->offset, SEEK_SET);
						fread(sectorconfig[sector_found].input_data,pOffset->size,1,f);

						inc += pOffset->size;

						if (pOffset->DAM != 0xFB)
						{
							sectorconfig[sector_found].use_alternate_datamark=1;
							sectorconfig[sector_found].alternate_datamark=pOffset->DAM;
						}

						if(pOffset->density)
						{
							sectorconfig[sector_found].trackencoding=IBMFORMAT_DD;
							if(!sector_found) trackformat=IBMFORMAT_DD;
						}
						else
						{
							sectorconfig[sector_found].trackencoding=IBMFORMAT_SD;
							if(!sector_found) trackformat=IBMFORMAT_SD;
						}

						if(pOffset->bad_sector)
						{
							sectorconfig[sector_found].use_alternate_data_crc = 0x01;
							sectorconfig[sector_found].data_crc = 0xAA55;
						}

						sectorconfig[sector_found].cylinder = pOffset->track_id;
						sectorconfig[sector_found].head = i;
						sectorconfig[sector_found].sector = pOffset->sector_id;
						sectorconfig[sector_found].bitrate = floppydisk->floppyBitRate;
						sectorconfig[sector_found].gap3 = gap3len;

						sector_found++;

					}

					cur_pos = GetNextPos(SectorsOffsets,NumberofEntries,j,i,cur_pos);

				}
				currentcylinder->sides[i]=tg_generateTrackEx(sector_found,sectorconfig,interleave,0,floppydisk->floppyBitRate,rpm,trackformat,0,2500|NO_SECTOR_UNDER_INDEX,-2500);

				for(k=0;k<floppydisk->floppySectorPerTrack;k++)
				{
					free(sectorconfig[k].input_data);
				}

			}
		}

		free(sectorconfig);
		free(SectorsOffsets);
		imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"track file successfully loaded and encoded!");

		hxc_fclose(f);

		hxcfe_sanityCheck(imgldr_ctx->hxcfe,floppydisk);

		return HXCFE_NOERROR;
	}

	imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"file size=%d !?",filesize);
	hxc_fclose(f);
	return HXCFE_BADFILE;
}

int JV3_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{

	static const char plug_id[]="TRS80_JV3";
	static const char plug_desc[]="TRS80 JV3 Loader";
	static const char plug_ext[]="jv3";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)	JV3_libIsValidDiskFile,
		(LOADDISKFILE)		JV3_libLoad_DiskFile,
		(WRITEDISKFILE)		JV3_libWrite_DiskFile,
		(GETPLUGININFOS)	JV3_libGetPluginInfo
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
