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
// File : emax_loader.c
// Contains: Emax floppy image loader
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

#include "loaders/common/raw_iso.h"

#include "emax_loader.h"

#define SCTR_SIZE 512
#define HEADS 2
#define SCTR_TRK 10
#define TRK_CYL 80
#define START_SCTR 1
#define END_SCTR 10

#define RWDISK_VERSION "1.1"
#define RWDISK_DATE "Fri Mar 19 13:31:05 1993"
#define EMAXUTIL_HDR "emaxutil v%3s %24s\n"
#define EMAXUTIL_HDRLEN 39

#define BANK_LOW 368
#define BANK_HIGH 423
#define SAMPLE_LOW 440
#define SAMPLE_HIGH 1463
#define OS1_LOW 0
#define OS1_HIGH 367
#define OS2_LOW 424
#define OS2_HIGH 439
#define OS3_LOW 1464
#define OS3_HIGH 1599
#define TOTAL_BLKS ((BANK_HIGH-BANK_LOW)+(SAMPLE_HIGH-SAMPLE_LOW))
#define TOTAL_OS ((OS1_HIGH-OS1_LOW)+(OS2_HIGH-OS2_LOW)+(OS3_HIGH-OS3_LOW))

int EMAX_libIsValidDiskFile( HXCFE_IMGLDR * imgldr_ctx, HXCFE_IMGLDR_FILEINFOS * imgfile )
{
	return hxcfe_imgCheckFileCompatibility( imgldr_ctx, imgfile, "EMAX_libIsValidDiskFile", "em1,em2,emx", 0 );
}

int EMAX_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	raw_iso_cfg rawcfg;
	FILE * f_img;
	int ret;

	FILE *f_os;
	unsigned int filesize;
	int i;
	unsigned char* floppy_data;
	char os_filename[512];

	char hdr[EMAXUTIL_HDRLEN+1];
	char fhdr[EMAXUTIL_HDRLEN+1];

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"EMAX_libLoad_DiskFile %s",imgfile);

	f_img = hxc_fopen(imgfile,"rb");
	if( f_img == NULL )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	filesize = hxc_fgetsize(f_img);

	raw_iso_setdefcfg(&rawcfg);

	strcpy(os_filename,imgfile);

	sprintf( hxc_getfilenamebase(os_filename,NULL,SYS_PATH_TYPE),"emaxos.emx");

	f_os = hxc_fopen(os_filename,"rb");
	if(f_os == NULL)
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open os file %s !",os_filename);
	}

	if(filesize!=0)
	{
		rawcfg.gap3 = 30;
		rawcfg.interleave = 1;
		rawcfg.skew_per_track = 4;
		rawcfg.skew_per_side = 2;
		rawcfg.track_format = IBMFORMAT_DD;
		rawcfg.number_of_sides = 2;
		rawcfg.number_of_sectors_per_track = 10;
		rawcfg.number_of_tracks = 80;
		rawcfg.bitrate = 250000;
		rawcfg.interface_mode = GENERIC_SHUGART_DD_FLOPPYMODE;
		rawcfg.rpm = 300;

		if(1)
		{
			floppy_data = malloc((SCTR_SIZE * SCTR_TRK) * TRK_CYL * HEADS);
			if(!floppy_data)
			{
				hxc_fclose(f_img);
				if(f_os)
					hxc_fclose(f_os);

				return HXCFE_INTERNALERROR;
			}

			memset(floppy_data,0xF6,(SCTR_SIZE * SCTR_TRK) * TRK_CYL * HEADS);

			sprintf (hdr, EMAXUTIL_HDR, RWDISK_VERSION, RWDISK_DATE);
			hdr[EMAXUTIL_HDRLEN]=0;

			hxc_fread (fhdr, (unsigned int) EMAXUTIL_HDRLEN,f_img);
			fhdr[EMAXUTIL_HDRLEN]=0;

			if (strncmp(fhdr, hdr, EMAXUTIL_HDRLEN)!=0)
			{

				imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Wrong version: disk says %s", fhdr);
				hxc_fclose(f_img);
				if(f_os)
					hxc_fclose(f_os);
				free(floppy_data);

				return HXCFE_BADFILE;
			}

			if(f_os)
			{
				for(i=OS1_LOW;i<=OS1_HIGH;i++)
				{
					hxc_fread(&floppy_data[i*512],512,f_os);
				}

				for(i=OS2_LOW;i<=OS2_HIGH;i++)
				{
					hxc_fread(&floppy_data[i*512],512,f_os);
				}

				for(i=OS3_LOW;i<=OS3_HIGH;i++)
				{
					hxc_fread(&floppy_data[i*512],512,f_os);
				}
			}

			for(i=BANK_LOW;i<=BANK_HIGH;i++)
			{
				hxc_fread(&floppy_data[i*512],512,f_img);
			}

			for(i=SAMPLE_LOW;i<=SAMPLE_HIGH;i++)
			{
				hxc_fread(&floppy_data[i*512],512,f_img);
			}

			hxc_fclose(f_img);

			if(f_os)
				hxc_fclose(f_os);

			ret = raw_iso_loader(imgldr_ctx, floppydisk, 0, floppy_data, (SCTR_SIZE * SCTR_TRK) * TRK_CYL * HEADS, &rawcfg);

			free(floppy_data);

			return ret;
		}

		hxc_fclose(f_img);
		if(f_os)
			hxc_fclose(f_os);

		return HXCFE_FILECORRUPTED;
	}

	imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"file size=%d !?",filesize);
	hxc_fclose(f_img);
	if(f_os)
		hxc_fclose(f_os);

	return HXCFE_BADFILE;
}

int EMAX_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]="EMAX_EM";
	static const char plug_desc[]="EMAX EM1 & EM2 Loader";
	static const char plug_ext[]="em1";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   EMAX_libIsValidDiskFile,
		(LOADDISKFILE)      EMAX_libLoad_DiskFile,
		(WRITEDISKFILE)     0,
		(GETPLUGININFOS)    EMAX_libGetPluginInfo
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
