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
// File : msa_loader.c
// Contains: MSA floppy image loader
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
#include "libhxcfe.h"
#include "libhxcadaptor.h"
#include "floppy_loader.h"
#include "tracks/track_generator.h"

#include "msa_loader.h"
#include "msa_writer.h"

#include "loaders/common/raw_iso.h"

int MSA_libIsValidDiskFile( HXCFE_IMGLDR * imgldr_ctx, HXCFE_IMGLDR_FILEINFOS * imgfile )
{
	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"MSA_libIsValidDiskFile");

	if(hxc_checkfileext(imgfile->path,"msa",SYS_PATH_TYPE))
	{
		if(imgfile->file_header[0]==0x0E && imgfile->file_header[1]==0x0F && imgfile->file_header[2]==0x00)
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"MSA_libIsValidDiskFile : MSA file !");
			return HXCFE_VALIDFILE;
		}
	}

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"MSA_libIsValidDiskFile : non MSA file !");
	return HXCFE_BADFILE;
}

int MSA_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	FILE * f;
	raw_iso_cfg rawcfg;

	unsigned char * flatimg = NULL;
	unsigned char * tmpbuffer = NULL;

	unsigned int filesize;
	int i,j,k,l,l2;
	uint8_t c;
	int32_t len;
	int32_t extractfilesize,filetracksize;
	unsigned char fileheader[5*2];
	unsigned char trackheader[1*2];

	int ret;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"MSA_libLoad_DiskFile %s",imgfile);

	f = hxc_fopen(imgfile,"rb");
	if( f == NULL )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	raw_iso_setdefcfg(&rawcfg);

	filesize = hxc_fgetsize(f);
	if( filesize )
	{
		hxc_fread(fileheader,5*sizeof(unsigned short),f);
		if(fileheader[0]==0x0E && fileheader[1]==0x0F)
		{
			rawcfg.number_of_tracks = ((256*fileheader[8])+fileheader[9])+1;
			rawcfg.number_of_sides =  ((256*fileheader[4])+fileheader[5])+1;
			rawcfg.number_of_sectors_per_track = (fileheader[2]*256)+fileheader[3];

			extractfilesize = (rawcfg.number_of_sectors_per_track*512)*(rawcfg.number_of_tracks+1)*(rawcfg.number_of_sides);

			flatimg = (unsigned char*)malloc(extractfilesize);
			if( !flatimg )
				goto alloc_error;

			memset(flatimg,0,extractfilesize);

			// Loading and unpacking the MSA file.
			j = 0;
			i = 0;
			do
			{
				hxc_fread(trackheader,2,f);
				filetracksize = ((trackheader[0]*256)+trackheader[1]);
				if(filetracksize == (rawcfg.number_of_sectors_per_track*512))
				{
					tmpbuffer = (unsigned char*)malloc(filetracksize);
					if( !tmpbuffer )
						goto alloc_error;

					memset(tmpbuffer,0,filetracksize);
					hxc_fread(tmpbuffer,filetracksize,f);
					memcpy(flatimg+j,tmpbuffer,filetracksize);
					free(tmpbuffer);
					tmpbuffer = NULL;
					j += filetracksize;
				}
				else
				{
					k = 0;
					l = 0;

					tmpbuffer=(unsigned char*)malloc(filetracksize);
					if( !tmpbuffer )
						goto alloc_error;

					memset(tmpbuffer,0,filetracksize);

					hxc_fread(tmpbuffer,filetracksize,f);
					do
					{
						if(tmpbuffer[k]!=0xE5)
						{
							if( l+j > extractfilesize )
								goto corrupted_file;

							flatimg[l+j] = tmpbuffer[k];
							l++;
							k++;
						}
						else
						{
							k++;
							c    = tmpbuffer[k++];
							len  = tmpbuffer[k++] * 256;
							len += tmpbuffer[k++];

							if(l+j+len>extractfilesize)
							{
								goto corrupted_file;
							}
							else
							{
								for(l2=0;l2<len;l2++)
								{
									flatimg[l+j] = c;
									l++;
								}
							}

						}
					}while(k<filetracksize);

					j += l;

					free(tmpbuffer);
					tmpbuffer = NULL;
				}

				i++;
			}while(i< (rawcfg.number_of_tracks*rawcfg.number_of_sides) );

			hxc_fclose(f);

			if( rawcfg.number_of_sectors_per_track < 15 )
			{
				rawcfg.bitrate = DEFAULT_DD_BITRATE;
				rawcfg.interface_mode = ATARIST_DD_FLOPPYMODE;
				rawcfg.skew_per_track = 4;
				rawcfg.skew_per_side = 2;
			}
			else
			{
				rawcfg.bitrate = DEFAULT_HD_BITRATE;
				rawcfg.interface_mode = ATARIST_HD_FLOPPYMODE;
				rawcfg.skew_per_track = 8;
				rawcfg.skew_per_side = 4;
			}

			rawcfg.track_format = ISOFORMAT_DD;
			rawcfg.gap3 = 84;
			rawcfg.interleave = 1;
			switch( rawcfg.number_of_sectors_per_track )
			{
				case 10:
					rawcfg.gap3 = 30;
				break;
				case 11:
					rawcfg.track_format = ISOFORMAT_DD11S;
					rawcfg.gap3 = 3;
					rawcfg.interleave = 2;
				break;
				case 19:
					rawcfg.gap3 = 70;
				break;
				case 20:
					rawcfg.gap3 = 40;
				break;
				case 21:
					rawcfg.gap3 = 18;
				break;
			}

			rawcfg.rpm = 300;

			ret = raw_iso_loader(imgldr_ctx, floppydisk, 0, flatimg, extractfilesize, &rawcfg);

			free(flatimg);

			return ret;
		}
		return HXCFE_NOERROR;
	}

	imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"file size=%d !?",filesize);

	return HXCFE_BADFILE;

alloc_error:

	free( flatimg );
	free( tmpbuffer );

	hxc_fclose(f);

	hxcfe_freeFloppy(imgldr_ctx->hxcfe, floppydisk );

	return HXCFE_INTERNALERROR;

corrupted_file:

	free( flatimg );
	free( tmpbuffer );

	hxc_fclose(f);

	hxcfe_freeFloppy(imgldr_ctx->hxcfe, floppydisk );

	return HXCFE_FILECORRUPTED;
}

int MSA_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]="ATARIST_MSA";
	static const char plug_desc[]="ATARI ST MSA Loader";
	static const char plug_ext[]="msa";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   MSA_libIsValidDiskFile,
		(LOADDISKFILE)      MSA_libLoad_DiskFile,
		(WRITEDISKFILE)     MSA_libWrite_DiskFile,
		(GETPLUGININFOS)    MSA_libGetPluginInfo
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
