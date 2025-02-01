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
// File : ti99pc99_loader.c
// Contains: TI99 PC99 floppy image loader
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

#include "tracks/encoding/fm_encoding.h"
#include "tracks/encoding/mfm_encoding.h"

#include "libhxcfe.h"

#include "floppy_loader.h"
#include "floppy_utils.h"

#include "ti99pc99_loader.h"

#include "tracks/crc.h"

#include "libhxcadaptor.h"

int TI99PC99_libIsValidDiskFile( HXCFE_IMGLDR * imgldr_ctx, HXCFE_IMGLDR_FILEINFOS * imgfile )
{
	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"TI99PC99_libIsValidDiskFile : TI99PC99_libIsValidDiskFile");
	if(imgfile)
	{
		if(
			!hxc_checkfileext(imgfile->path,"dsk",SYS_PATH_TYPE) &&
			!hxc_checkfileext(imgfile->path,"pc99",SYS_PATH_TYPE) &&
			!hxc_checkfileext(imgfile->path,"pc9",SYS_PATH_TYPE) &&
			!hxc_checkfileext(imgfile->path,"tdf",SYS_PATH_TYPE)
		)
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"TI99PC99_libIsValidDiskFile : non TI99 PC99 file !");
			return HXCFE_BADFILE;
		}

		if( (imgfile->file_size%3253 && imgfile->file_size%6872) || !imgfile->file_size )
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"TI99PC99_libIsValidDiskFile : non TI99 PC99 file !");
			return HXCFE_BADFILE;
		}

		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"TI99PC99_libIsValidDiskFile : TI99 PC99 file !");
		return HXCFE_VALIDFILE;
	}

	return HXCFE_BADPARAMETER;
}

int patchtrackFM(unsigned char * trackdata, unsigned char * trackclk,int tracklen)
{
	int i,j,k,l;
	int sectorsize;
	int nbofsector;
	unsigned char crctable[32];
	unsigned char CRC16_High,CRC16_Low;

	nbofsector=0;
	i=0;
	do
	{
		if(trackdata[i]==0xFE)
		{
			trackclk[i]=0xC7;
			if( (trackdata[i+5]==0xF7) && (trackdata[i+6]==0xF7) )
			{
				// calculate header crc
				sectorsize=128<<trackdata[i+4];
				l=i;
				CRC16_Init(&CRC16_High,&CRC16_Low,(unsigned char*)crctable,0x1021,0xFFFF);
				for(k=0;k<5;k++)
				{
					CRC16_Update(&CRC16_High,&CRC16_Low, trackdata[l],(unsigned char*)crctable );
					l++;
				}

				trackdata[i+5]=CRC16_High;
				trackdata[i+6]=CRC16_Low;

				j=12;
				do
				{
					if((trackdata[i+j]==0xFB) || (trackdata[i+j]==0xF8))
					{
						trackclk[i+j]=0xC7;

						if((trackdata[i+j+sectorsize+1]==0xF7) && (trackdata[i+j+sectorsize+2]==0xF7))
						{
							// calculate data crc
							//02 CRC The sector Header CRC
							CRC16_Init(&CRC16_High,&CRC16_Low,(unsigned char*)crctable,0x1021,0xFFFF);
							for(k=0;k<sectorsize+1;k++)
							{
								CRC16_Update(&CRC16_High,&CRC16_Low, trackdata[i+j+k],(unsigned char*)crctable );
							}

							trackdata[i+j+sectorsize+1]=CRC16_High;
							trackdata[i+j+sectorsize+2]=CRC16_Low;
							i=i+j+sectorsize+3;
							j=32;
							nbofsector++;
						}
					}

					j++;
				}while(j<32);
			}
		}

		i++;
	}while(i<tracklen);

	return nbofsector;
}


int patchtrackMFM(unsigned char * trackdata, unsigned char * trackclk,int tracklen)
{
	int i,j,k,l;
	int sectorsize;
	int nbofsector;
	unsigned char crctable[32];
	unsigned char CRC16_High,CRC16_Low;

	nbofsector=0;
	i=0;
	do
	{
		if(trackdata[i]==0xA1 && trackdata[i+1]==0xA1 && trackdata[i+2]==0xA1 && trackdata[i+3]==0xFE)
		{
			l=i;
			trackclk[i]=0x0A;
			trackclk[i+1]=0x0A;
			trackclk[i+2]=0x0A;
			trackclk[i+3]=0xFF;
			i=i+3;

			if( (trackdata[i+5]==0xF7) && (trackdata[i+6]==0xF7) )
			{
				// calculate header crc
				sectorsize=128<<trackdata[i+4];

				CRC16_Init(&CRC16_High,&CRC16_Low,(unsigned char*)crctable,0x1021,0xFFFF);
				for(k=0;k<8;k++)
				{
					CRC16_Update(&CRC16_High,&CRC16_Low, trackdata[l],(unsigned char*)crctable );
					l++;
				}
				trackdata[i+5]=CRC16_High;
				trackdata[i+6]=CRC16_Low;

				j=0;
				do
				{
					if(trackdata[i+j]==0xA1 && trackdata[i+j+1]==0xA1 && trackdata[i+j+2]==0xA1 && trackdata[i+j+3]==0xFB)
					{
						trackclk[i+j]=0x0A;
						trackclk[i+j+1]=0x0A;
						trackclk[i+j+2]=0x0A;
						trackclk[i+j+3]=0xFF;
						i=i+j+3+1;
						if((trackdata[i+sectorsize]==0xF7) && (trackdata[i+sectorsize+1]==0xF7))
						{
							// calculate data crc
							//02 CRC The sector Header CRC
							CRC16_Init(&CRC16_High,&CRC16_Low,(unsigned char*)crctable,0x1021,0xFFFF);
							for(k=0;k<sectorsize+4;k++)
							{
								CRC16_Update(&CRC16_High,&CRC16_Low, trackdata[i+k-4],(unsigned char*)crctable );
							}

							trackdata[i+sectorsize]=CRC16_High;
							trackclk[i+sectorsize]=0xFF;
							trackdata[i+sectorsize+1]=CRC16_Low;
							trackclk[i+sectorsize+1]=0xFF;
							i=i+sectorsize+2;
							j=64;
							nbofsector++;
						}

					}

					j++;
				}while(j<64);

			}
		}

		i++;
	}while(i<tracklen);

	return nbofsector;
}

int TI99PC99_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{

	FILE * f;
	unsigned int filesize;
	int i,j;
	int file_offset;
	unsigned char* trackdata;
	unsigned char* trackclk;
	int rpm;
	int fmmode,tracklen,numberoftrack,numberofside;
	HXCFE_CYLINDER* currentcylinder;
	HXCFE_SIDE* currentside;

	trackclk = NULL;
	trackdata = NULL;
	f = NULL;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"TI99PC99_libLoad_DiskFile %s",imgfile);

	f = hxc_fopen(imgfile,"rb");
	if( f == NULL )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	filesize = hxc_fgetsize(f);

	if( filesize )
	{
		fmmode=0;

		if(filesize%3253 && filesize%6872)
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"non TI99 PC99 file !");
			hxc_fclose(f);

			return HXCFE_BADFILE;
		}

		if(!(filesize%3253))
		{
			tracklen=3253;
			fmmode=1;
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"FM TI99 PC99 file !");
			floppydisk->floppyBitRate=250000;
		}
		else
		{
			tracklen=6872;
			fmmode=0;
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"MFM TI99 PC99 file !");
			floppydisk->floppyBitRate=250000;
		}

		switch(filesize/tracklen)
		{
		case 40:
			numberofside=1;
			numberoftrack=40;
			break;
		case 80:
			numberofside=2;
			numberoftrack=40;
			break;
		default:
			imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Unsupported geometry!");
			hxc_fclose(f);
			return HXCFE_BADFILE;
			break;
		}

		floppydisk->floppyNumberOfTrack=numberoftrack;
		floppydisk->floppyNumberOfSide=numberofside;
		floppydisk->floppySectorPerTrack=-1;
		floppydisk->floppyiftype=GENERIC_SHUGART_DD_FLOPPYMODE;
		floppydisk->tracks=(HXCFE_CYLINDER**)malloc(sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);
		if(!floppydisk->tracks)
			goto alloc_error;

		rpm=300; // normal rpm

		imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"filesize:%dkB, %d tracks, %d side(s), %d sectors/track, rpm:%d",filesize/1024,floppydisk->floppyNumberOfTrack,floppydisk->floppyNumberOfSide,floppydisk->floppySectorPerTrack,rpm);

		trackdata = (unsigned char*)malloc(tracklen);
		if(!trackdata)
			goto alloc_error;

		trackclk=(unsigned char*)malloc(tracklen);
		if(!trackclk)
			goto alloc_error;

		for(j=0;j<floppydisk->floppyNumberOfTrack;j++)
		{

			floppydisk->tracks[j]=allocCylinderEntry(rpm,floppydisk->floppyNumberOfSide);
			currentcylinder=floppydisk->tracks[j];

			for(i=0;i<floppydisk->floppyNumberOfSide;i++)
			{
				hxcfe_imgCallProgressCallback(imgldr_ctx,(j<<1) + (i&1),floppydisk->floppyNumberOfTrack*2 );

				if(fmmode)
				{
					floppydisk->tracks[j]->sides[i]=tg_alloctrack(floppydisk->floppyBitRate,ISOIBM_FM_ENCODING ,currentcylinder->floppyRPM,tracklen*4*8,2500,-2500,0x00);
				}
				else
				{
					floppydisk->tracks[j]->sides[i]=tg_alloctrack(floppydisk->floppyBitRate,ISOIBM_MFM_ENCODING,currentcylinder->floppyRPM,tracklen*2*8,2500,-2500,0x00);
				}

				currentside=currentcylinder->sides[i];

				currentside->number_of_sector=floppydisk->floppySectorPerTrack;

				file_offset=(tracklen*j)+(tracklen*numberoftrack*(i&1));
				fseek (f , file_offset , SEEK_SET);

				hxc_fread(trackdata,tracklen,f);
				memset(trackclk,0xFF,tracklen);

				if(fmmode)
				{
					patchtrackFM(trackdata,trackclk,tracklen);
					BuildFMCylinder(currentside->databuffer,currentside->tracklen/8,trackclk,trackdata,tracklen);
				}
				else
				{
					patchtrackMFM(trackdata,trackclk,tracklen);
					BuildMFMCylinder(currentside->databuffer,currentside->tracklen/8,trackclk,trackdata,tracklen);
				}
			}
		}

		free(trackdata);
		free(trackclk);

		imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"track file successfully loaded and encoded!");

		hxc_fclose(f);

		return HXCFE_NOERROR;
	}

	imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"file size=%d !?",filesize);
	hxc_fclose(f);

	return HXCFE_BADFILE;

alloc_error:

	free( trackclk );
	free( trackdata );

	if( f )
		hxc_fclose(f);

	hxcfe_freeFloppy(imgldr_ctx->hxcfe, floppydisk );

	return HXCFE_INTERNALERROR;
}

int TI99PC99_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]="TI994A_PC99";
	static const char plug_desc[]="TI99 4A PC99 Loader";
	static const char plug_ext[]="pc99";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   TI99PC99_libIsValidDiskFile,
		(LOADDISKFILE)      TI99PC99_libLoad_DiskFile,
		(WRITEDISKFILE)     0,
		(GETPLUGININFOS)    TI99PC99_libGetPluginInfo
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

