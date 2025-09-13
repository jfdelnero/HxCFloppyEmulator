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
// File : oricdsk_loader.c
// Contains: OricDSK floppy image loader
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

#include "oricdsk_loader.h"
#include "oricdsk_format.h"

#include "libhxcadaptor.h"

int OricDSK_libIsValidDiskFile( HXCFE_IMGLDR * imgldr_ctx, HXCFE_IMGLDR_FILEINFOS * imgfile )
{
	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"OricDSK_libIsValidDiskFile");

	if(hxc_checkfileext(imgfile->path,"dsk",SYS_PATH_TYPE))
	{
		if( !strncmp((char*)imgfile->file_header,"MFM_DISK",8) ||
			!strncmp((char*)imgfile->file_header,"ORICDISK",8))
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"OricDSK_libIsValidDiskFile : OricDSK file !");
			return HXCFE_VALIDFILE;
		}
		else
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"OricDSK_libIsValidDiskFile : non OricDSK file (bad header)!");
			return HXCFE_BADFILE;
		}

	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"OricDSK_libIsValidDiskFile : non OricDSK file !");
		return HXCFE_BADFILE;
	}
}

#pragma pack(1)
typedef struct mfmformatsect_
{
	uint8_t idam1[3]; // 0xA1 * 3
	uint8_t idam2;    // 0xFE
	uint8_t track;
	uint8_t head;
	uint8_t sector;
	uint8_t bytes;
	uint8_t crc[2];
	uint8_t gap2[22]; // 0x4E
	uint8_t sync[12]; // 0x00
	uint8_t dataam1[3]; // 0xA1 * 3
	uint8_t dataam2;    // 0xFB
}mfmformatsect;

#pragma pack()

int issector(mfmformatsect * sector)
{
	int i;

	i=0;
	do
	{
		if(sector->idam1[i]!=0xA1)
		{
			return 0;
		}

		i++;
	}while(i<3);

	i=0;
	do
	{
		if(sector->dataam1[i]!=0xA1)
		{
			return 0;
		}
		i++;
	}while(i<3);


	if(sector->idam2!=0xFE)
	{
		return 0;
	}

	if((sector->dataam2!=0xFB) && (sector->dataam2!=0xF8))
	{
		return 0;
	}

	return 1;

}

HXCFE_SECTCFG * extractsector(HXCFE* floppycontext,unsigned char *data,unsigned char * inputtrack,int32_t tracksize,int32_t * numberofsector)
{
	int i,j,k;
	int sectorsize;
	HXCFE_SECTCFG * tabsector;

	mfmformatsect * testsector;

	tabsector=(HXCFE_SECTCFG *)malloc(sizeof(HXCFE_SECTCFG)*32);
	if(!tabsector)
		return NULL;

	memset(tabsector,0,sizeof(HXCFE_SECTCFG)*32);

	i=0;
	j=0;
	k=0;
	do
	{
		testsector=(mfmformatsect *)(inputtrack+i);
		if(!issector(testsector))
		{
			i++;
		}
		else
		{

			floppycontext->hxc_printf(MSG_DEBUG,"Sector found ! %.4x",i);
			sectorsize=128<<(testsector->bytes&7);

			tabsector[j].head=testsector->head;
			tabsector[j].sector=testsector->sector;
			tabsector[j].sectorsize=sectorsize;
			tabsector[j].cylinder=testsector->track;
			tabsector[j].use_alternate_data_crc=0;
			tabsector[j].use_alternate_header_crc=0;
			tabsector[j].header_crc=0;
			tabsector[j].missingdataaddressmark=0;
			tabsector[j].startdataindex=0;
			tabsector[j].startsectorindex=0;

			tabsector[j].use_alternate_datamark=0xFF;
			tabsector[j].alternate_datamark=testsector->dataam2;

			tabsector[j].use_alternate_addressmark=0;
			tabsector[j].gap3=255;
			tabsector[j].bitrate=DEFAULT_DD_BITRATE;
			tabsector[j].trackencoding=IBMFORMAT_DD;

			memcpy(&data[k],&inputtrack[i+sizeof(mfmformatsect)],sectorsize);
			tabsector[j].input_data=&data[k];
			k=k+sectorsize;
			j++;

			i=i+sectorsize+sizeof(mfmformatsect)+2;

			floppycontext->hxc_printf(MSG_DEBUG,"%d bytes, track %d, sector %d, side %d",sectorsize,testsector->track,testsector->sector,testsector->head);

		}

	}while(i<tracksize);

	*numberofsector=j;

	return tabsector;
}

HXCFE_SIDE* ORICDSKpatchtrack(HXCFE* floppycontext,unsigned char * trackdata,unsigned int tracklen)
{
	int i,c2_cnt,a1_cnt;
	unsigned int lastdensity,tracksize,k;
	unsigned char trackformat;
	unsigned int  final_tracklen;

	HXCFE_SIDE* currentside;
	unsigned char * trackclk;

	track_generator tg;

	trackformat=ISOFORMAT_DD;

	trackclk = malloc(tracklen);
	if(!trackclk)
		return NULL;

	memset(trackclk,0xFF,tracklen);

	k=0;

	a1_cnt = 0;
	c2_cnt = 0;

	i = 0;
	while( i < tracklen)
	{
		if(trackdata[i] == 0xC2)
		{
			a1_cnt = 0;
			c2_cnt++;
		}else
		{
			if(trackdata[i] == 0xA1)
			{
				c2_cnt = 0;
				a1_cnt++;
			}
			else
			{
				if(a1_cnt == 3)
				{
					switch(trackdata[i])
					{
						case 0xF8:
						case 0xF9:
						case 0xFA:
						case 0xFB:
						case 0xFE:
							trackclk[i-1] = 0x0A;
							trackclk[i-2] = 0x0A;
							trackclk[i-3] = 0x0A;
						break;
					}
				}

				if(c2_cnt == 3)
				{
					switch(trackdata[i])
					{
						case 0xFC:
							trackclk[i-1] = 0x0A;
							trackclk[i-2] = 0x0A;
							trackclk[i-3] = 0x0A;
						break;
					}
				}

				a1_cnt = 0;
				c2_cnt = 0;
			}
		}

		i++;
	}

	final_tracklen = tracklen * 2;
	lastdensity=ISOFORMAT_DD;

	// alloc the track...
	tg_initTrackEncoder(&tg);

	tracksize=final_tracklen*8;
	if(tracksize&0x1F)
		tracksize=(tracksize&(~0x1F))+0x20;

	currentside=tg_initTrack(&tg,tracksize,0,trackformat,DEFAULT_DD_BITRATE,0,0);

	k=0;
	do
	{
		pushTrackCode(&tg,trackdata[k],trackclk[k],currentside,lastdensity);
		k++;
	}while(k<tracklen);

	tg_completeTrack(&tg,currentside,trackformat);

	free(trackclk);

	return currentside;
}

int OricDSK_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	FILE * f;
	unsigned int filesize;
	int i,j,mfmformat,geometrie;
	unsigned int file_offset;
	unsigned char* trackdata;
	unsigned char* trackdatatab;
	int32_t tracksize;
	int32_t numberofsector;
	int32_t sectorsize,rpm;
	int32_t interleave,gap3len;
	HXCFE_CYLINDER* currentcylinder;
	HXCFE_SECTCFG * sectlist;
	unsigned char header_buffer[256];
	oricdsk_fileheader * fileheader;
	int regenerate_mode;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"OricDSK_libLoad_DiskFile %s",imgfile);

	trackdata = NULL;
	trackdatatab = NULL;
	sectlist = NULL;

	regenerate_mode = 1;

	if(hxcfe_getEnvVarValue( imgldr_ctx->hxcfe, "ORICDSK_LOADER_REGENERATE_TRACK" ) == 0)
	{
		regenerate_mode = 0;
	}

	f = hxc_fopen(imgfile,"rb");
	if( f == NULL )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	filesize = hxc_fgetsize(f);

	if( filesize )
	{
		mfmformat=0;
		fileheader = (oricdsk_fileheader *) header_buffer;
		memset(fileheader,0,sizeof(header_buffer));

		hxc_fread(fileheader,256,f);
		sectorsize=256; // OricDSK file support only 256bytes/sector floppies.
		gap3len=255;
		interleave=1;
		rpm=300; // normal rpm

		floppydisk->floppyNumberOfTrack=(unsigned short)fileheader->number_of_tracks;
		floppydisk->floppyNumberOfSide=(unsigned char)fileheader->number_of_side;
		floppydisk->floppyBitRate=250000;
		floppydisk->floppyiftype=GENERIC_SHUGART_DD_FLOPPYMODE;

		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"%d tracks, %d side",floppydisk->floppyNumberOfTrack,floppydisk->floppyNumberOfSide);

		if( !strncmp((char*)&fileheader->headertag,"MFM_DISK",8))
		{
			mfmformat=1;
			geometrie=fileheader->number_of_sectors_geometrie;
			floppydisk->floppySectorPerTrack=-1;
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"OricDSK_libLoad_DiskFile MFM_DISK %d tracks %d sides geometrie %d (regenerate mode : %d)",floppydisk->floppyNumberOfTrack,floppydisk->floppyNumberOfSide,geometrie,regenerate_mode);
		}
		else
		{
			if( !strncmp((char*)&fileheader->headertag,"ORICDISK",8))
			{
				mfmformat=0;
				geometrie=0;
				floppydisk->floppySectorPerTrack=(unsigned short)fileheader->number_of_sectors_geometrie;
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"OricDSK_libLoad_DiskFile ORICDISK %d tracks %d sides %d",floppydisk->floppyNumberOfTrack,floppydisk->floppyNumberOfSide,floppydisk->floppySectorPerTrack);
			}
			else
			{
				return HXCFE_BADFILE;
			}
		}

		switch(mfmformat)
		{
			case 0:// "OLD" format
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"ORICDISK format !");

				trackdata = (unsigned char*)malloc(sectorsize*floppydisk->floppySectorPerTrack);
				if( !trackdata )
					goto error;

				floppydisk->tracks = (HXCFE_CYLINDER**)malloc(sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);
				if( !floppydisk->tracks )
					goto error;

				memset(floppydisk->tracks,0,sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);
				for(i=0;i<floppydisk->floppyNumberOfSide;i++)
				{
					for(j=0;j<floppydisk->floppyNumberOfTrack;j++)
					{

						hxcfe_imgCallProgressCallback(imgldr_ctx,j + (i*floppydisk->floppyNumberOfTrack),(floppydisk->floppyNumberOfTrack*floppydisk->floppyNumberOfSide) );

						if(!floppydisk->tracks[j])
							floppydisk->tracks[j]=allocCylinderEntry(rpm,floppydisk->floppyNumberOfSide);

						currentcylinder = floppydisk->tracks[j];

						file_offset=((j*(sectorsize*floppydisk->floppySectorPerTrack))+
							(sectorsize*(floppydisk->floppySectorPerTrack)*floppydisk->floppyNumberOfTrack*i))+256;

						fseek (f , file_offset , SEEK_SET);
						hxc_fread(trackdata,sectorsize*floppydisk->floppySectorPerTrack,f);

						currentcylinder->sides[i]=tg_generateTrack(trackdata,sectorsize,floppydisk->floppySectorPerTrack,(unsigned char)j,(unsigned char)i,1,1,0,floppydisk->floppyBitRate,currentcylinder->floppyRPM,IBMFORMAT_DD,gap3len,0,2500|NO_SECTOR_UNDER_INDEX,-2500);
					}

				}
				free(trackdata);
				trackdata = NULL;
			break;

			case 1:// "New" format
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"MFM_DISK format !");

				floppydisk->tracks=(HXCFE_CYLINDER**)malloc(sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);
				if( !floppydisk->tracks )
					goto error;

				memset(floppydisk->tracks,0,sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);

				for(j=0;j<floppydisk->floppyNumberOfTrack;j++)
				{
					floppydisk->tracks[j]=allocCylinderEntry(rpm,floppydisk->floppyNumberOfSide);
				}

				switch(geometrie)
				{
					case 1:
						imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Geometrie 1:  11111..000000!");

						tracksize=(((filesize-256)/fileheader->number_of_side)/fileheader->number_of_tracks);

						if( tracksize > 9000 || (tracksize & 0xFF) )
						{
							imgldr_ctx->hxcfe->hxc_printf(MSG_WARNING,"Wrongly sized images ? Fall back to 6400 bytes tracks !");
							tracksize = 6400;
						}

						trackdata = (unsigned char *) malloc(tracksize);
						if( !trackdata )
							goto error;

						trackdatatab = (unsigned char*) malloc(256*1024);
						if( !trackdatatab )
							goto error;

						for(i=0;i<floppydisk->floppyNumberOfSide;i++)
						{
							for(j=0;j<floppydisk->floppyNumberOfTrack;j++)
							{
								hxcfe_imgCallProgressCallback(imgldr_ctx,j + (i*floppydisk->floppyNumberOfTrack),(floppydisk->floppyNumberOfTrack*floppydisk->floppyNumberOfSide) );

								imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"------------------ Track %d, side %d -------------------",j,i);
								currentcylinder=floppydisk->tracks[j];

								file_offset=(i*floppydisk->floppyNumberOfTrack*tracksize)+(j*tracksize)+256;

								fseek (f , file_offset , SEEK_SET);

								memset(trackdata,0x4E,tracksize);
								hxc_fread(trackdata,tracksize,f);

								if(regenerate_mode)
								{
									sectlist = extractsector(imgldr_ctx->hxcfe,trackdatatab,trackdata,tracksize,&numberofsector);
									if(sectlist)
									{
										currentcylinder->sides[i] = tg_generateTrackEx((unsigned short)numberofsector,sectlist,interleave,(unsigned char)(j*2),DEFAULT_DD_BITRATE,rpm,IBMFORMAT_DD,0,2500|NO_SECTOR_UNDER_INDEX,-2500);
										free(sectlist);
										sectlist = NULL;
									}
								}
								else
								{
									currentcylinder->sides[i] = ORICDSKpatchtrack(imgldr_ctx->hxcfe,trackdata,tracksize);
								}
							}
						}

						free(trackdatatab);
						trackdatatab = NULL;

						free(trackdata);
						trackdata = NULL;
					break;

					case 0:
						imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Geometrie 0 ?: using Geometrie 2");
					case 2:
						imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Geometrie 2:  01010101010...!");

						tracksize=(((filesize-256)/fileheader->number_of_side)/fileheader->number_of_tracks);

						trackdata=(unsigned char *) malloc(tracksize);
						if( !trackdata )
							goto error;

						trackdatatab=(unsigned char*) malloc(256*1024);
						if( !trackdatatab )
							goto error;

						for(j=0;j<floppydisk->floppyNumberOfTrack;j++)
						{
							currentcylinder=floppydisk->tracks[j];

							for(i=0;i<floppydisk->floppyNumberOfSide;i++)
							{
								hxcfe_imgCallProgressCallback(imgldr_ctx,(j<<1) + (i&1),(floppydisk->floppyNumberOfTrack*floppydisk->floppyNumberOfSide) );

								imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"------------------ Track %d, side %d -------------------",j,i);

								file_offset=(j*tracksize*floppydisk->floppyNumberOfSide) + (i*tracksize) + 256;

								fseek (f , file_offset , SEEK_SET);

								memset(trackdata,0x4E,tracksize);
								hxc_fread(trackdata,tracksize,f);

								if(regenerate_mode)
								{
									sectlist = extractsector(imgldr_ctx->hxcfe,trackdatatab,trackdata,tracksize,&numberofsector);
									if( sectlist )
									{
										currentcylinder->sides[i] = tg_generateTrackEx((unsigned short)numberofsector,sectlist,interleave,(unsigned char)(j*2),DEFAULT_DD_BITRATE,rpm,IBMFORMAT_DD,0,2500|NO_SECTOR_UNDER_INDEX,-2500);
										free(sectlist);
										sectlist = NULL;
									}
								}
								else
								{
									currentcylinder->sides[i] = ORICDSKpatchtrack(imgldr_ctx->hxcfe,trackdata,tracksize);
								}
							}
						}

						free(trackdatatab);
						trackdatatab = NULL;

						free(trackdata);
						trackdata = NULL;
					break;

					default:
					break;
				}
			break;
		}

		imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"track file successfully loaded and encoded!");
		hxc_fclose(f);

		hxcfe_sanityCheck(imgldr_ctx->hxcfe,floppydisk);

		return HXCFE_NOERROR;

	}

	hxc_fclose(f);

	imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"file size=%d !?",filesize);

	return HXCFE_BADFILE;

error:
	if(f)
		hxc_fclose(f);

	free(trackdatatab);
	free(trackdata);

	return HXCFE_INTERNALERROR;

}

int OricDSK_libWrite_DiskFile(HXCFE_IMGLDR* imgldr_ctx,HXCFE_FLOPPY * floppy,char * filename);

int OricDSK_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]="ORIC_DSK";
	static const char plug_desc[]="ORIC DSK Loader";
	static const char plug_ext[]="dsk";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   OricDSK_libIsValidDiskFile,
		(LOADDISKFILE)      OricDSK_libLoad_DiskFile,
		(WRITEDISKFILE)     OricDSK_libWrite_DiskFile,
		(GETPLUGININFOS)    OricDSK_libGetPluginInfo
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
