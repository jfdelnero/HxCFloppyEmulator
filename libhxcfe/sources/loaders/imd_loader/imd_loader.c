/*
//
// Copyright (C) 2006-2024 Jean-Fran�ois DEL NERO
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
// File : imd_loader.c
// Contains: IMD floppy image loader.
//
// Written by: Jean-Fran�ois DEL NERO
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

#include "imd_loader.h"
#include "imd_format.h"

#include "libhxcadaptor.h"

int IMD_libIsValidDiskFile( HXCFE_IMGLDR * imgldr_ctx, HXCFE_IMGLDR_FILEINFOS * imgfile )
{
	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"IMD_libIsValidDiskFile");

	if(hxc_checkfileext(imgfile->path,"imd",SYS_PATH_TYPE))
	{
		if( !strncmp((char*)imgfile->file_header,"IMD ",4))
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"IMD_libIsValidDiskFile : IMD file !");
			return HXCFE_VALIDFILE;
		}
		else
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"IMD_libIsValidDiskFile : non IMD file !");
			return HXCFE_BADFILE;
		}
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"IMD_libIsValidDiskFile : non IMD file !");
		return HXCFE_BADFILE;
	}

}

int IMD_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	FILE * f;
	char fileheader[5];
//  MFMTRACKIMG trackdesc;
	unsigned int i,j,trackcount,headcount;
	HXCFE_SECTCFG* sectorconfig = NULL;
	HXCFE_CYLINDER* currentcylinder = NULL;
	HXCFE_SIDE* currentside = NULL;
	int32_t bitrate;
	int32_t pregap;
	unsigned char * sectormap = NULL;
	unsigned char * sectorcylmap = NULL;
	unsigned char * sectorheadmap = NULL;
	unsigned char * track_data = NULL;
	imd_trackheader trackcfg;
	int32_t sectorsize;
	int32_t interleave,tracktype;
	int32_t rpm;
	unsigned char sectordatarecordcode,cdata;
	unsigned int filesize;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"IMD_libLoad_DiskFile %s",imgfile);

	pregap = 0;

	f = hxc_fopen(imgfile,"rb");
	if( f == NULL )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	filesize = hxc_fgetsize(f);

	memset(fileheader,0,sizeof(fileheader));

	hxc_fread(&fileheader,sizeof(fileheader),f);

	if(!strncmp(fileheader,"IMD ",4))
	{
		// recherche fin entete / comentaire(s).
		i = 0;
		do
		{
			i++;
		}while(getc(f)!=0x1A && i<filesize);

		if( i >= filesize )
		{
			hxc_fclose(f);
			return HXCFE_BADFILE;			
		}

		// recuperation de la geometries du disque
		trackcount=0;
		headcount=0;
		do
		{

			if(hxc_fread(&trackcfg,sizeof(trackcfg),f)>0)
			{
				fseek(f,trackcfg.number_of_sector,SEEK_CUR);
				if(trackcfg.physical_head & SEC_CYL_MAP)
				{
					fseek(f,trackcfg.number_of_sector,SEEK_CUR);
				}

				if(trackcfg.physical_head & SEC_HEAD_MAP)
				{
					fseek(f,trackcfg.number_of_sector,SEEK_CUR);
				}

				if(trackcount<trackcfg.physical_cylinder)
				{
					trackcount=trackcfg.physical_cylinder;
				}

				if(headcount<(unsigned int)(trackcfg.physical_head&0x0F))
				{
					headcount=trackcfg.physical_head&0x0F;
				}

				for(i=0;i<trackcfg.number_of_sector;i++)
				{
					hxc_fread(&sectordatarecordcode,1,f);

					switch(sectordatarecordcode)
					{
						case 0x00:

							break;
						case 0x01:
							fseek(f,(128<<trackcfg.sector_size_code),SEEK_CUR);
							break;
						case 0x02:
							fseek(f,1,SEEK_CUR);
							break;
						case 0x03:
							fseek(f,(128<<trackcfg.sector_size_code),SEEK_CUR);
							break;
						case 0x04:
							fseek(f,1,SEEK_CUR);
							break;
						case 0x05:
							fseek(f,(128<<trackcfg.sector_size_code),SEEK_CUR);
							break;
						case 0x06:
							fseek(f,1,SEEK_CUR);
							break;
						case 0x07:
							fseek(f,(128<<trackcfg.sector_size_code),SEEK_CUR);
							break;
						case 0x08:
							fseek(f,1,SEEK_CUR);
							break;
						default:
							break;
					}
				}
			}

		}while(!feof(f));


		floppydisk->floppyNumberOfTrack=trackcount+1;//header.number_of_track;
		floppydisk->floppyNumberOfSide=headcount+1;
		floppydisk->floppyBitRate=0;//header.floppyBitRate*1000;
		floppydisk->floppySectorPerTrack=-1;
		floppydisk->floppyiftype=GENERIC_SHUGART_DD_FLOPPYMODE;

		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"IMD File : %d track, %d side, %d bit/s, %d sectors, mode %d",
			floppydisk->floppyNumberOfTrack,
			floppydisk->floppyNumberOfSide,
			floppydisk->floppyBitRate,
			floppydisk->floppySectorPerTrack,
			floppydisk->floppyiftype);

		interleave=1;
		rpm=300;
		floppydisk->tracks = (HXCFE_CYLINDER**)malloc(sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);
		if(!floppydisk->tracks)
			goto alloc_error;

		memset(floppydisk->tracks,0,sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);

		fseek(f,0,SEEK_SET);

		// recherche fin entete / comentaire(s).
		i = 0;
		do
		{
			i++;
		}while(getc(f)!=0x1A && i<filesize);

		if( i >= filesize )
		{
			hxc_fclose(f);
			return HXCFE_BADFILE;			
		}

		for(i=0;i<(unsigned int)(floppydisk->floppyNumberOfTrack*floppydisk->floppyNumberOfSide);i++)
		{
			hxcfe_imgCallProgressCallback(imgldr_ctx,i,(floppydisk->floppyNumberOfTrack*floppydisk->floppyNumberOfSide) );

			if(hxc_fread(&trackcfg,sizeof(trackcfg),f)>0)
			{
				sectorconfig = (HXCFE_SECTCFG*)malloc(sizeof(HXCFE_SECTCFG)*trackcfg.number_of_sector);
				if(!sectorconfig)
					goto alloc_error;

				memset(sectorconfig,0,sizeof(HXCFE_SECTCFG)*trackcfg.number_of_sector);

				// lecture map sector.
				sectormap = (unsigned char*) malloc(trackcfg.number_of_sector);
				if(!sectormap)
					goto alloc_error;

				hxc_fread(sectormap,trackcfg.number_of_sector,f);

				// init map cylinder
				sectorcylmap = (unsigned char*) malloc(trackcfg.number_of_sector);
				if(!sectorcylmap)
					goto alloc_error;

				memset(sectorcylmap,trackcfg.physical_cylinder,trackcfg.number_of_sector);
				if(trackcfg.physical_head & SEC_CYL_MAP)
				{
					hxc_fread(sectorcylmap,trackcfg.number_of_sector,f);
				}

				// init map head
				sectorheadmap = (unsigned char*) malloc(trackcfg.number_of_sector);
				if(!sectorheadmap)
					goto alloc_error;

				memset(sectorheadmap,trackcfg.physical_head&0xF,trackcfg.number_of_sector);
				if(trackcfg.physical_head & SEC_HEAD_MAP)
				{
					hxc_fread(sectorheadmap,trackcfg.number_of_sector,f);
				}

				sectorsize = (int32_t)(128<<trackcfg.sector_size_code);
				track_data = malloc(sectorsize*trackcfg.number_of_sector);
				if(!track_data)
					goto alloc_error;

				memset(track_data,0,sectorsize*trackcfg.number_of_sector);

				 /*
				 00 = 500 kbps FM   \   Note:   kbps indicates transfer rate,
				 01 = 300 kbps FM    >          not the data rate, which is
				 02 = 250 kbps FM   /           1/2 for FM encoding.
				 03 = 500 kbps MFM
				 04 = 300 kbps MFM
				 05 = 250 kbps MFM
				 */
				switch(trackcfg.track_mode_code)
				{
					case 0x00:
						tracktype=IBMFORMAT_SD;
						bitrate=500000;
						if(trackcfg.number_of_sector==26)
						{
							rpm=360;
						}
						break;
					case 0x01:
						tracktype=IBMFORMAT_SD;
						bitrate=300000;
						break;
					case 0x02:
						tracktype=IBMFORMAT_SD;
						bitrate=250000;
						break;
					case 0x03:
						tracktype=IBMFORMAT_DD;
						bitrate=500000;
						break;
					case 0x04:
						tracktype=IBMFORMAT_DD;
						bitrate=300000;
						break;
					case 0x05:
						tracktype=IBMFORMAT_DD;
						bitrate=250000;
						break;

					case 0x80:
						tracktype=MEMBRAINFORMAT_DD;
						bitrate=500000;
						rpm=360;
						pregap = 130;
						break;
					case 0x81:
						tracktype=MEMBRAINFORMAT_DD;
						bitrate=300000;
						rpm=360;
						pregap = 130;
						break;
					case 0x82:
						tracktype=MEMBRAINFORMAT_DD;
						bitrate=250000;
						rpm=360;
						pregap = 130;
						break;

					default:
						tracktype=IBMFORMAT_DD;
						bitrate=250000;
				}


				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Track %d Head %d: %d kbits/s, %d %dbytes sectors, encoding :%d",
					trackcfg.physical_cylinder,
					trackcfg.physical_head&0xF,
					bitrate/1000,
					trackcfg.number_of_sector,
					128<<trackcfg.sector_size_code,
					tracktype
					);

				for(j=0;j<trackcfg.number_of_sector;j++)
				{

					hxc_fread(&sectordatarecordcode,1,f);
					switch(sectordatarecordcode)
					{
						case 0x00:
							sectorconfig[j].input_data = NULL;
							sectorconfig[j].use_alternate_datamark = 1;
							sectorconfig[j].alternate_datamark = 0x01;
							sectorconfig[j].missingdataaddressmark = 1;
						break;
						case 0x01:
							hxc_fread(&track_data[j*sectorsize],sectorsize,f);
							sectorconfig[j].input_data=&track_data[j*sectorsize];
							sectorconfig[j].use_alternate_datamark=0x00;
							sectorconfig[j].alternate_datamark=0xFB;
						break;
						case 0x02:
							hxc_fread(&cdata,1,f);
							memset(&track_data[j*sectorsize],cdata,sectorsize);
							sectorconfig[j].input_data=&track_data[j*sectorsize];
							sectorconfig[j].use_alternate_datamark=0x00;
							sectorconfig[j].alternate_datamark=0xFB;
						break;
						case 0x03:
							hxc_fread(&track_data[j*sectorsize],sectorsize,f);
							sectorconfig[j].input_data=&track_data[j*sectorsize];
							sectorconfig[j].use_alternate_datamark=1;
							sectorconfig[j].alternate_datamark=0xF8;

						break;
						case 0x04:
							hxc_fread(&cdata,1,f);
							memset(&track_data[j*sectorsize],cdata,sectorsize);
							sectorconfig[j].input_data=&track_data[j*sectorsize];
							sectorconfig[j].use_alternate_datamark=1;
							sectorconfig[j].alternate_datamark=0xF8;

						break;
						case 0x05:
							hxc_fread(&track_data[j*sectorsize],sectorsize,f);
							sectorconfig[j].input_data=&track_data[j*sectorsize];
							sectorconfig[j].use_alternate_data_crc=0x1;
							sectorconfig[j].alternate_datamark=0xFB;
						break;
						case 0x06:
							hxc_fread(&cdata,1,f);
							memset(&track_data[j*sectorsize],cdata,sectorsize);
							sectorconfig[j].input_data=&track_data[j*sectorsize];
							sectorconfig[j].use_alternate_data_crc=0x1;
							sectorconfig[j].alternate_datamark=0xFB;
							sectorconfig[j].use_alternate_datamark=0x00;
						break;
						case 0x07:
							hxc_fread(&track_data[j*sectorsize],sectorsize,f);
							sectorconfig[j].input_data=&track_data[j*sectorsize];
							sectorconfig[j].use_alternate_datamark=1;
							sectorconfig[j].alternate_datamark=0xF8;
							sectorconfig[j].use_alternate_data_crc=0x1;
						break;
						case 0x08:
							hxc_fread(&cdata,1,f);
							memset(&track_data[j*sectorsize],cdata,sectorsize);
							sectorconfig[j].input_data=&track_data[j*sectorsize];
							sectorconfig[j].use_alternate_datamark=1;
							sectorconfig[j].alternate_datamark=0xF8;
							sectorconfig[j].use_alternate_data_crc=0x1;

						break;
						default:
							break;
					}


					sectorconfig[j].cylinder=sectorcylmap[j];
					sectorconfig[j].head=sectorheadmap[j]&0xF;
					sectorconfig[j].sector=sectormap[j];
					sectorconfig[j].sectorsize=128<<trackcfg.sector_size_code;
					sectorconfig[j].bitrate=bitrate;
					sectorconfig[j].gap3=255;
					sectorconfig[j].trackencoding=tracktype;
				}

				floppydisk->floppyBitRate=bitrate;

				if(!floppydisk->tracks[trackcfg.physical_cylinder])
				{
					floppydisk->tracks[trackcfg.physical_cylinder] = allocCylinderEntry(rpm,floppydisk->floppyNumberOfSide);
				}

				currentcylinder = floppydisk->tracks[trackcfg.physical_cylinder];


				currentside = tg_generateTrackEx((unsigned short)trackcfg.number_of_sector,sectorconfig,interleave,0,floppydisk->floppyBitRate,rpm,tracktype,pregap,2500 | NO_SECTOR_UNDER_INDEX,-2500);
				currentcylinder->sides[trackcfg.physical_head&0xF] = currentside;
				currentcylinder->floppyRPM = rpm;

				for(j=0;j<trackcfg.number_of_sector;j++)
				{
					imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Sector:%d %x %x %x",sectorconfig[j].sector,sectorconfig[j].alternate_datamark,sectorconfig[j].alternate_sector_size_id,tracktype);
				}

				free(track_data);
				track_data = NULL;
				free(sectorheadmap);
				sectorheadmap = NULL;
				free(sectorcylmap);
				sectorcylmap = NULL;
				free(sectormap);
				sectormap = NULL;
				free(sectorconfig);
				sectorconfig = NULL;
			}
			else
			{
				// missing sector data...

				if(!floppydisk->tracks[i>>(floppydisk->floppyNumberOfSide-1)])
				{
					floppydisk->tracks[i>>(floppydisk->floppyNumberOfSide-1)] = allocCylinderEntry(rpm,floppydisk->floppyNumberOfSide);
				}

				currentcylinder = floppydisk->tracks[i>>(floppydisk->floppyNumberOfSide-1)];

				currentcylinder->sides[i&(floppydisk->floppyNumberOfSide-1)] = tg_alloctrack(250000,ISOFORMAT_DD,rpm, ((250000/(rpm/60))/4)*8 ,2000,0,0);

				currentside = currentcylinder->sides[i&(floppydisk->floppyNumberOfSide-1)];
				memset(currentside->databuffer,0xAA,currentside->tracklen/8);

			}

		}

		hxc_fclose(f);

		hxcfe_sanityCheck(imgldr_ctx->hxcfe,floppydisk);

		return HXCFE_NOERROR;
	}

	hxc_fclose(f);

	imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"bad header");

	return HXCFE_BADFILE;

alloc_error:
	free(track_data);
	free(sectorheadmap);
	free(sectorcylmap);
	free(sectormap);
	free(sectorconfig);

	hxcfe_freeFloppy(imgldr_ctx->hxcfe, floppydisk );

	return HXCFE_INTERNALERROR;
}

int IMD_libWrite_DiskFile(HXCFE_IMGLDR* imgldr_ctx,HXCFE_FLOPPY * floppy,char * filename);

int IMD_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]="IMD_IMG";
	static const char plug_desc[]="ImageDisk IMD file Loader";
	static const char plug_ext[]="imd";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   IMD_libIsValidDiskFile,
		(LOADDISKFILE)      IMD_libLoad_DiskFile,
		(WRITEDISKFILE)     IMD_libWrite_DiskFile,
		(GETPLUGININFOS)    IMD_libGetPluginInfo
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
