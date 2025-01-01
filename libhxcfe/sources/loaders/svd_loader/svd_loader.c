/*
//
// Copyright (C) 2006-2025 Jean-Fran�ois DEL NERO
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
// File : svd_loader.c
// Contains: SVD floppy image loader
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

#include "svd_loader.h"

#include "svd.h"

#include "libhxcadaptor.h"

int SVD_libIsValidDiskFile( HXCFE_IMGLDR * imgldr_ctx, HXCFE_IMGLDR_FILEINFOS * imgfile )
{
	int major,minor;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"SVD_libIsValidDiskFile");
	if( hxc_checkfileext(imgfile->path,"svd",SYS_PATH_TYPE) )
	{
		if (sscanf((char*)imgfile->file_header,"%d.%d",&major,&minor) != 2)
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"SVD_libIsValidDiskFile : Bad code version !");
			return(HXCFE_BADFILE);
		}

		if((major==2 && minor==0) ||(major==1 && minor==2) ||(major==1 && minor==5))
		{
			return HXCFE_VALIDFILE;
		}
		else
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"SVD_libIsValidDiskFile : Bad code version !");
			return HXCFE_BADFILE;
		}
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"SVD_libIsValidDiskFile : non SVD file !");
		return HXCFE_BADFILE;
	}
}

int SVD_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	FILE * f;
	unsigned int filesize;
	int i,j,k,skew;
	unsigned int file_offset;
	unsigned char* trackdata;
	int gap3len,interleave;
	int sectorsize,rpm;
	HXCFE_CYLINDER* currentcylinder;
	int trackformat;
	int major,minor;
	int sectorpertrack,numberoftrack,numberofside,sectsize,wprot;
	unsigned char blockbuf[256];
	char linebuffer[80];
	int indexptr,sector;
	//int blanks;
	HXCFE_SECTCFG * sectorconfig;
	int sectorindex;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"JVC_libLoad_DiskFile %s",imgfile);

	gap3len=255;
	interleave = 1;
	skew = 1;
	file_offset = 0;
	trackdata = NULL;
	sectorsize = 512;
	trackformat = 0;

	f = hxc_fopen(imgfile,"rb");
	if( f == NULL )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	filesize = hxc_fgetsize(f);

	// code version
	hxc_fgets(linebuffer,sizeof(linebuffer),f);
	if (sscanf(linebuffer,"%d.%d",&major,&minor) != 2)
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Bad code version !");
		return(HXCFE_BADFILE);
	}

	// number of sector per track
	hxc_fgets(linebuffer,sizeof(linebuffer),f);
	if (sscanf(linebuffer,"%d",&sectorpertrack) != 1)
		return(HXCFE_BADFILE);

	// number of tracks
	hxc_fgets(linebuffer,sizeof(linebuffer),f);
	if (sscanf(linebuffer,"%d",&numberoftrack) != 1)
		return(HXCFE_BADFILE);

	// number of sides
	hxc_fgets(linebuffer,sizeof(linebuffer),f);
	if (sscanf(linebuffer,"%d",&numberofside) != 1)
		return(HXCFE_BADFILE);

	// sector size
	hxc_fgets(linebuffer,sizeof(linebuffer),f);
	if (sscanf(linebuffer,"%d",&sectsize) != 1)
		return(HXCFE_BADFILE);

	// write protect
	hxc_fgets(linebuffer,sizeof(linebuffer),f);
	if (sscanf(linebuffer,"%d",&wprot) != 1)
		return(HXCFE_BADFILE);

	floppydisk->floppyNumberOfTrack=numberoftrack;
	floppydisk->floppyNumberOfSide=numberofside;
	floppydisk->floppySectorPerTrack=sectorpertrack;
	floppydisk->floppyBitRate=DEFAULT_DD_BITRATE;
	floppydisk->floppyiftype=GENERIC_SHUGART_DD_FLOPPYMODE;
	rpm=300;

	if(filesize!=0)
	{
		trackdata = malloc(sectorpertrack * 256);
		if(!trackdata)
		{
			hxc_fclose(f);
			return HXCFE_INTERNALERROR;
		}

		for(j=0;j<floppydisk->floppyNumberOfTrack;j++)
		{

			floppydisk->tracks[j]=allocCylinderEntry(rpm,floppydisk->floppyNumberOfSide);
			currentcylinder=floppydisk->tracks[j];

			for(i=0;i<floppydisk->floppyNumberOfSide;i++)
			{
				hxcfe_imgCallProgressCallback(imgldr_ctx, (j<<1) + (i&1),floppydisk->floppyNumberOfTrack*2);

				hxc_fread(blockbuf,256,f);

				//blanks=0;
				indexptr = 0;
				sectorindex=0;

				sectorconfig = malloc(sizeof(HXCFE_SECTCFG) * sectorpertrack);
				if(!sectorconfig)
				{
					free(trackdata);
					hxc_fclose(f);
					return(HXCFE_INTERNALERROR);
				}

				memset(sectorconfig,0,sizeof(HXCFE_SECTCFG) * sectorpertrack);

				for (sector = 0; sector < sectorpertrack; sector++)
				{
					int sectortype = blockbuf[indexptr++];

					//THIS_SECTOR.encoding = SVD_SECTOR_TYPE(sectortype);

					if (SVD_SECTOR_TYPE(sectortype) == BLANK_SECTOR) {
						/* blanks are always at the end...so if we see a blank, we're done  */
						/* with real data...however, we need to read in the blanks' data    */
						//blanks++;
					}
					else
					{

						/* there is a "standard" amount of data per header field in the */
						/* track header...each format needs to advance the indexpr right.*/

						switch(SVD_SECTOR_TYPE(sectortype))
						{
							case WD_FM:
							case WD_MFM:
								;
								sectorconfig[sectorindex].sector = blockbuf[indexptr++]; // id
								sectorconfig[sectorindex].head  = blockbuf[indexptr++]; // side
								//THIS_SECTOR.sector = blockbuf[indexptr++];
								sectorconfig[sectorindex].alternate_sector_size_id = blockbuf[indexptr++];
								sectorconfig[sectorindex].use_alternate_sector_size_id=0xFF;
								sectorconfig[sectorindex].header_crc=blockbuf[indexptr++];
								sectorconfig[sectorindex].header_crc +=blockbuf[indexptr++]*256;
								sectorconfig[sectorindex].use_alternate_header_crc=0xFF;
								sectorconfig[sectorindex].alternate_datamark=blockbuf[indexptr++];
								sectorconfig[sectorindex].use_alternate_datamark=0xFF;
								sectorconfig[sectorindex].data_crc = blockbuf[indexptr++];
								sectorconfig[sectorindex].data_crc += (int)(blockbuf[indexptr++]) * 256;
								sectorconfig[sectorindex].use_alternate_data_crc=0xFF;
								sectorconfig[sectorindex].sectorsize=256;
							break;

							case H17_HSFM:
								indexptr += 19;
								if (sector == sectorpertrack - 1) {
									indexptr += 5;      /* last sector has 5 extra      */
								}
								//THIS_SECTOR.sizecode = reflect(blockbuf[indexptr++]); /* sets the volume */
								//THIS_SECTOR.id = reflect(blockbuf[indexptr++]);
								//THIS_SECTOR.sector = reflect(blockbuf[indexptr++]);
								//THIS_SECTOR.headCRC = reflect(blockbuf[indexptr++]);
								//THIS_SECTOR.dataCRC = reflect(blockbuf[indexptr++]);
								//if (THIS_SECTOR.id != 0) {
								//  floppy->volume = THIS_SECTOR.sizecode;
								//}
								//THIS_SECTOR.size = 256;
							break;

							case RNIB:
								/* no information is used with the RNIB format, but it is written/read  */
								/* here just to allow the tools to work better (arguable).      */

								//THIS_SECTOR.id = blockbuf[indexptr++];
								//THIS_SECTOR.side = blockbuf[indexptr++];
								//THIS_SECTOR.sector = blockbuf[indexptr++];
								//THIS_SECTOR.sizecode = blockbuf[indexptr++];

								//if (sector != 0) {
								//  indexptr += 5;
								//}
							break;

							case AGCR6x2:
							case AGCR5x3:
							{
								//int i;
/*
								for (i=0; i < 3; i++) {
								THIS_SECTOR.hdrprolog[i] = blockbuf[indexptr++];
								}
								THIS_SECTOR.volume = blockbuf[indexptr++];
								THIS_SECTOR.id = blockbuf[indexptr++];
								THIS_SECTOR.sector = blockbuf[indexptr++];
								THIS_SECTOR.hdrchecksum = blockbuf[indexptr++];
*/
								/* ONLY 2 BYTES OF EPILOG - THIRD IS SVD STATIC */

/*                              for (i=0; i < 2; i++) {
								THIS_SECTOR.hdrepilog[i] = blockbuf[indexptr++];
								}

								for (i=0; i < 3; i++) {
								THIS_SECTOR.dataprolog[i] = blockbuf[indexptr++];
								}
								THIS_SECTOR.preload = blockbuf[indexptr++];
								THIS_SECTOR.datachecksum = blockbuf[indexptr++];
*/
								/* standard data for AGCR   */

//                              THIS_SECTOR.size = 256;
							}
							break;

							case RX02:
							case BLANK_SECTOR:
							case UNKNOWN_ENCODING:
							indexptr += 9;
							break;
						}

						sectorindex++;
					}
				}

				fseek (f , file_offset , SEEK_SET);

				for(k=0;k<11;k++)
				{
					hxc_fread(&trackdata[k*sectorsize],sectorsize,f);
				}

				currentcylinder->sides[i]=tg_generateTrack(trackdata,sectorsize,floppydisk->floppySectorPerTrack,(unsigned char)j,(unsigned char)i,1,interleave,((j<<1)|(i&1))*skew,floppydisk->floppyBitRate,currentcylinder->floppyRPM,trackformat,gap3len,0,2500 | NO_SECTOR_UNDER_INDEX,-2500);

				free(sectorconfig);
				sectorconfig = NULL;
			}
		}

		free(trackdata);

		imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"track file successfully loaded and encoded!");

		hxc_fclose(f);
		return HXCFE_NOERROR;
	}

	imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"file size=%d !?",filesize);
	hxc_fclose(f);
	return HXCFE_BADFILE;
}

int SVD_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]="SVD";
	static const char plug_desc[]="SVD Loader";
	static const char plug_ext[]="svd";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   SVD_libIsValidDiskFile,
		(LOADDISKFILE)      SVD_libLoad_DiskFile,
		(WRITEDISKFILE)     0,
		(GETPLUGININFOS)    SVD_libGetPluginInfo
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
