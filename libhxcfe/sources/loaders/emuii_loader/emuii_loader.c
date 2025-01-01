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
// File : emuii_loader.c
// Contains: Emax floppy image loader
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

#include "tracks/track_formats/emu_emulator_fm_track.h"

#include "emuii_loader.h"

#include "libhxcadaptor.h"

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


int EMUII_libIsValidDiskFile( HXCFE_IMGLDR * imgldr_ctx, HXCFE_IMGLDR_FILEINFOS * imgfile )
{
	return hxcfe_imgCheckFileCompatibility( imgldr_ctx, imgfile, "EMUII_libIsValidDiskFile", "eii", 0);
}

int EMUII_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{

	FILE * f_eii,*f_os;
	unsigned int i;
	char os_filename[512];

	HXCFE_CYLINDER* currentcylinder;
	HXCFE_SIDE* currentside;
	unsigned char sector_data[0xE00];
	int tracknumber,sidenumber;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"EMUII_libLoad_DiskFile %s",imgfile);

	strcpy(os_filename,imgfile);

	sprintf( hxc_getfilenamebase(os_filename,NULL,SYS_PATH_TYPE),"emuiios.emuiifd");

	f_os=hxc_fopen(os_filename,"rb");
	if(f_os==NULL)
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open os file %s !",os_filename);
		return HXCFE_ACCESSERROR;
	}

	f_eii=hxc_fopen(imgfile,"rb");
	if(f_eii==NULL)
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	floppydisk->floppyNumberOfTrack=80;
	floppydisk->floppyNumberOfSide=2;
	floppydisk->floppyBitRate=DEFAULT_EMUII_BITRATE;
	floppydisk->floppySectorPerTrack=1;
	floppydisk->floppyiftype=EMU_SHUGART_FLOPPYMODE;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"EmuII File : %d track, %d side, %d bit/s, %d sectors, mode %d",
		floppydisk->floppyNumberOfTrack,
		floppydisk->floppyNumberOfSide,
		floppydisk->floppyBitRate,
		floppydisk->floppySectorPerTrack,
		floppydisk->floppyiftype);

	floppydisk->tracks=(HXCFE_CYLINDER**)malloc(sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);
	if( !floppydisk->tracks )
		goto alloc_error;

	memset(floppydisk->tracks,0,sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);

	for(i=0;i<(unsigned int)(floppydisk->floppyNumberOfTrack*floppydisk->floppyNumberOfSide);i++)
	{

		hxcfe_imgCallProgressCallback(imgldr_ctx,i,floppydisk->floppyNumberOfTrack*floppydisk->floppyNumberOfSide);

		tracknumber=i>>1;
		sidenumber=i&1;

		if((i<22) || (i>=158))
		{
			fseek(f_os,i*0xE00,SEEK_SET);
			memset(&sector_data,0x00,0xE00);
			hxc_fread(&sector_data,0xE00,f_os);
		}
		else
		{
			memset(&sector_data,0xFF,0xE00);

			fseek(f_os,i*0xE00,SEEK_SET);
			hxc_fread(&sector_data,0xE00,f_os);

			fseek(f_eii,(i-22)*0xE00,SEEK_SET);
			hxc_fread(&sector_data,0xE00,f_eii);
		}

		if(!floppydisk->tracks[tracknumber])
		{
			floppydisk->tracks[tracknumber] = allocCylinderEntry(300,floppydisk->floppyNumberOfSide);
		}

		currentcylinder = floppydisk->tracks[tracknumber];

		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"track %d side %d at offset 0x%x (0x%x bytes)",
			tracknumber,
			sidenumber,
			(0xE00*tracknumber*2)+(sidenumber*0xE00),
			0xE00);

		currentcylinder->sides[sidenumber]=tg_alloctrack(floppydisk->floppyBitRate,EMU_FM_ENCODING,currentcylinder->floppyRPM,((floppydisk->floppyBitRate/5)*2),2000,-2000,0x00);
		currentside=currentcylinder->sides[sidenumber];
		currentside->number_of_sector=floppydisk->floppySectorPerTrack;

		BuildEmuIITrack(imgldr_ctx->hxcfe,tracknumber,sidenumber,sector_data,currentside->databuffer,&currentside->tracklen,2);

	}

	hxc_fclose(f_eii);
	hxc_fclose(f_os);

	return HXCFE_NOERROR;

alloc_error:

	if ( f_eii )
		hxc_fclose( f_eii );

	if ( f_os )
		hxc_fclose( f_os );

	free( floppydisk->tracks );

	hxcfe_freeFloppy(imgldr_ctx->hxcfe, floppydisk );

	return HXCFE_INTERNALERROR;

}

int EMUII_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]="EMULATORII_EMUII";
	static const char plug_desc[]="E-mu Emulator II *.eii Loader";
	static const char plug_ext[]="eii";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   EMUII_libIsValidDiskFile,
		(LOADDISKFILE)      EMUII_libLoad_DiskFile,
		(WRITEDISKFILE)     0,
		(GETPLUGININFOS)    EMUII_libGetPluginInfo
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
