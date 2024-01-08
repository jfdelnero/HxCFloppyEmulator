/*
//
// Copyright (C) 2006-2024 Jean-François DEL NERO
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
// File : fat12floppy_loader.c
// Contains: FAT12FLOPPY floppy image loader
//
// Written by: Jean-François DEL NERO
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

#include "types.h"

#include "internal_libhxcfe.h"
#include "libhxcfe.h"
#include "libhxcadaptor.h"
#include "floppy_loader.h"
#include "tracks/track_generator.h"

#include "fat12floppy_loader.h"

#include "loaders/common/raw_iso.h"

#include "fat12.h"
#include "fat12formats.h"

#include "fatlib.h"


int FAT12FLOPPY_libIsValidDiskFile( HXCFE_IMGLDR * imgldr_ctx, HXCFE_IMGLDR_FILEINFOS * imgfile )
{
	int i,found;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"FAT12FLOPPY_libIsValidDiskFile");
	if(imgfile)
	{
		if( imgfile->is_dir )
		{
			found=0;
			i=0;
			do
			{
				if( configlist[i].dir && hxc_checkfileext(imgfile->path,configlist[i].dirext,SYS_PATH_TYPE))
				{
					found=1;
				}
				i++;
			}while( strlen(configlist[i].dirext) && !found );

			if(found)
			{
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"FAT12FLOPPY_libIsValidDiskFile : FAT12FLOPPY file ! (Dir , %s)",configlist[i].dirext);
				return HXCFE_VALIDFILE;
			}
			else
			{
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"FAT12FLOPPY_libIsValidDiskFile : non FAT12FLOPPY file !");
				return HXCFE_BADFILE;
			}
		}
		else
		{

			found=0;
			i=0;
			do
			{
				if( !configlist[i].dir && hxc_checkfileext(imgfile->path,configlist[i].dirext,SYS_PATH_TYPE))
				{
					found=1;
				}
				i++;
			}while( strlen(configlist[i].dirext) && !found );

			if(found)
			{
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"FAT12FLOPPY_libIsValidDiskFile : FAT12FLOPPY file ! (File , %s)",configlist[i].dirext);
				return HXCFE_VALIDFILE;
			}
			else
			{
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"FAT12FLOPPY_libIsValidDiskFile : non FAT12FLOPPY file !");
				return HXCFE_BADFILE;
			}
		}
	}

	return HXCFE_BADPARAMETER;
}

int FAT12FLOPPY_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	int32_t  i,j;
	raw_iso_cfg rawcfg;

	unsigned char * flatimg;
	int32_t    dirmode;
	int32_t    numberofcluster;
	uint32_t   fatposition;
	uint32_t   rootposition;
	uint32_t   dataposition;
	int32_t    dksize;
	unsigned char media_type;
	FATCONFIG fatconfig;
	struct stat staterep;
	int ret;
	char dummyext[512];

	fat_boot_sector * fatbs;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"FAT12FLOPPY_libLoad_DiskFile %s",imgfile);

	if(!parameters)
	{
		i=0;
		while(!hxc_checkfileext(imgfile,configlist[i].dirext,SYS_PATH_TYPE) && strlen(configlist[i].dirext))
		{
			i++;
		}
	}
	else
	{
		sprintf(dummyext,"dummy.%s",(char*)parameters);
		i=0;
		while(!hxc_checkfileext(dummyext,configlist[i].dirext,SYS_PATH_TYPE) && strlen(configlist[i].dirext))
		{
			i++;
		}
	}

	dirmode = configlist[i].dir;

	raw_iso_setdefcfg(&rawcfg);

	rawcfg.number_of_tracks = configlist[i].number_of_track;
	rawcfg.number_of_sides = configlist[i].number_of_side;
	rawcfg.number_of_sectors_per_track = configlist[i].number_of_sectorpertrack;
	rawcfg.bitrate = configlist[i].bitrate;
	rawcfg.interface_mode = configlist[i].interface_mode;
	rawcfg.track_format = configlist[i].tracktype;
	rawcfg.interleave = configlist[i].interleave;
	rawcfg.gap3 = configlist[i].gap3;
	rawcfg.rpm = configlist[i].rpm;
	rawcfg.sector_size = configlist[i].sectorsize;
	rawcfg.pregap = configlist[i].pregap;
	media_type = configlist[i].BPB_media;

	dksize = rawcfg.number_of_tracks *
			(rawcfg.number_of_sectors_per_track * rawcfg.number_of_sides * configlist[i].sectorsize);

	flatimg = (unsigned char*)malloc(dksize);
	if(flatimg != NULL)
	{
		memset(flatimg,0xF6,dksize);
		if(configlist[i].bootsector)
			memcpy(flatimg,configlist[i].bootsector,512);
		else
			memset(flatimg,0x00,512);

		for(j=0;j<4;j++)
		{
			flatimg[j+0x27] = rand();
		}

		fatconfig.sectorsize = configlist[i].sectorsize;
		fatconfig.clustersize = configlist[i].clustersize;
		fatconfig.reservedsector = configlist[i].reserved_sector;
		fatconfig.numberoffat = 2;

		fatconfig.numberofrootentries = configlist[i].root_dir_entries;
		fatconfig.nbofsector = (rawcfg.number_of_tracks*
								(rawcfg.number_of_sectors_per_track*rawcfg.number_of_sides));
		fatconfig.nbofsectorperfat = ( ((fatconfig.nbofsector-(fatconfig.reservedsector+(fatconfig.numberofrootentries/32)))/fatconfig.clustersize)/((fatconfig.sectorsize*8)/12))+1;
		//sprintf(&flatimg[CHSTOADR(0,0,0)+3],"HXC.EMU");

		fatbs = (fat_boot_sector*)flatimg;

		fatbs->BPB_BytsPerSec = fatconfig.sectorsize;            // Nombre d'octets par secteur;
		fatbs->BPB_SecPerClus = fatconfig.clustersize;           // Nombre de secteurs par cluster (1, 2, 4, 8, 16, 32, 64 ou 128).
		fatbs->BPB_RsvdSecCnt = fatconfig.reservedsector;        // Nombre de secteur réservé en comptant le secteur de boot (32 par défaut pour FAT32, 1 par défaut pour FAT12/16).
		fatbs->BPB_NumFATs = fatconfig.numberoffat;              // Nombre de FATs sur le disque (2 par défaut).
		fatbs->BPB_RootEntCnt = fatconfig.numberofrootentries;   // Taille du répertoire racine (0 par défaut pour FAT32).
		fatbs->BPB_TotSec16 = fatconfig.nbofsector;              // Nombre total de secteur 16-bit (0 par défaut pour FAT32).
		fatbs->BPB_Media = media_type;                           // Type de disque

		fatbs->BPB_FATSz16 = fatconfig.nbofsectorperfat;         // Taille d'une FAT en secteurs (0 par défaut pour FAT32).
		fatbs->BPB_SecPerTrk = rawcfg.number_of_sectors_per_track; // Sectors per track
		fatbs->BPB_NumHeads = rawcfg.number_of_sides;            // Number of heads.

		*( (unsigned short*) &flatimg[0x1FE])=0xAA55;            // End of sector marker (0x55 0xAA)

		fatposition = fatconfig.sectorsize * fatconfig.reservedsector;
		memset(&flatimg[fatposition],0x00,fatconfig.numberoffat*fatconfig.nbofsectorperfat*512);

		rootposition = ((fatconfig.reservedsector)+(fatconfig.numberoffat*fatconfig.nbofsectorperfat))*fatconfig.sectorsize;
		memset(&flatimg[rootposition],0x00,fatconfig.numberofrootentries*32);

		dataposition = (((fatconfig.reservedsector)+(fatconfig.numberoffat*fatconfig.nbofsectorperfat))*fatconfig.sectorsize)+(32*fatconfig.numberofrootentries);

		numberofcluster = (fatconfig.nbofsector-(dataposition/fatconfig.sectorsize))/fatconfig.clustersize;

		memset(&staterep,0,sizeof(struct stat));
		if(strlen(imgfile))
		{
			hxc_stat(imgfile,&staterep);
			if(!(staterep.st_mode&S_IFDIR))
				dirmode=0x00;
		}
		else
		{
			dirmode=0x00;
		}

		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"FAT12FLOPPY_libLoad_DiskFile : dirmode=0x%x",dirmode);

		if(dirmode)
		{
			if(ScanFileAndAddToFAT(imgldr_ctx->hxcfe,imgfile,"*.*",&flatimg[fatposition],&flatimg[rootposition],&flatimg[dataposition],0,&fatconfig,numberofcluster))
			{
				return HXCFE_BADFILE;
			}
		}
		else
		{
			if(ScanFileAndAddToFAT(imgldr_ctx->hxcfe,imgfile,0    ,&flatimg[fatposition],&flatimg[rootposition],&flatimg[dataposition],0,&fatconfig,numberofcluster))
			{
				return HXCFE_BADFILE;
			}
		}

		flatimg[fatposition+0] = fatbs->BPB_Media;
		flatimg[fatposition+1] = 0xFF;
		flatimg[fatposition+2] = 0xFF;

		memcpy(&flatimg[((fatconfig.reservedsector)+(fatconfig.nbofsectorperfat))*fatconfig.sectorsize],&flatimg[fatposition],fatconfig.nbofsectorperfat*fatconfig.sectorsize);

		ret = raw_iso_loader(imgldr_ctx, floppydisk, 0, flatimg, dksize, &rawcfg);

		free(flatimg);

		return ret;
	
	}
	else
	{
		return HXCFE_INTERNALERROR;
	}

	return HXCFE_BADFILE;
}

int FAT12FLOPPY_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]="FAT12FLOPPY";
	static const char plug_desc[]="FAT12/MS DOS Loader";
	static const char plug_ext[]="fat";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   FAT12FLOPPY_libIsValidDiskFile,
		(LOADDISKFILE)      FAT12FLOPPY_libLoad_DiskFile,
		(WRITEDISKFILE)     0,
		(GETPLUGININFOS)    FAT12FLOPPY_libGetPluginInfo
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
