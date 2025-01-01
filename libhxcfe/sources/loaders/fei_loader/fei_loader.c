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
// File : fei_loader.c
// Contains: FEI floppy image loader
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

#include "fei_loader.h"

#include "libhxcadaptor.h"

#include "tracks/luts.h"

int FEI_libIsValidDiskFile( HXCFE_IMGLDR * imgldr_ctx, HXCFE_IMGLDR_FILEINFOS * imgfile )
{
	return hxcfe_imgCheckFileCompatibility( imgldr_ctx, imgfile, "FEI_libIsValidDiskFile", "fei", 0 );
}

int FEI_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	FILE * f;
	int i,j,k,filesize;
	int rpm;
	HXCFE_CYLINDER* currentcylinder;
	HXCFE_SIDE* currentside;
	int tracksize;

	currentcylinder = 0;
	tracksize = 0;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"FEI_libLoad_DiskFile %s",imgfile);

	f = hxc_fopen(imgfile,"rb");
	if( f == NULL )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	filesize = hxc_fgetsize(f);

	if(!(filesize%25000))
	{
		floppydisk->floppyNumberOfTrack=(filesize/2) / 25000;
		floppydisk->floppyNumberOfSide=2;
		floppydisk->floppyBitRate=500000;
		floppydisk->floppySectorPerTrack=-1;
		floppydisk->floppyiftype=ATARIST_DD_FLOPPYMODE;
		tracksize=25000;
		rpm=300;
	}
	else
	{

		if(!(filesize%12500))
		{
			floppydisk->floppyNumberOfTrack=(filesize/2) / 12500;
			floppydisk->floppyNumberOfSide=2;
			floppydisk->floppyBitRate=250000;
			floppydisk->floppySectorPerTrack=-1;
			floppydisk->floppyiftype=ATARIST_DD_FLOPPYMODE;
			tracksize=12500;
			rpm=300;
		}
	}

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"FEI File : %d track, %d side, %d bit/s, %d sectors, mode %d",
		floppydisk->floppyNumberOfTrack,
		floppydisk->floppyNumberOfSide,
		floppydisk->floppyBitRate,
		floppydisk->floppySectorPerTrack,
		floppydisk->floppyiftype);

	floppydisk->tracks=(HXCFE_CYLINDER**)malloc(sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);
	if(tracksize && floppydisk->tracks)
	{
		memset(floppydisk->tracks,0,sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);

		for(i=0;i<floppydisk->floppyNumberOfTrack;i++)
		{
			for(j=0;j<floppydisk->floppyNumberOfSide;j++)
			{

				hxcfe_imgCallProgressCallback(imgldr_ctx, (i<<1) + (j&1),floppydisk->floppyNumberOfTrack*2);

				if(!floppydisk->tracks[i])
				{
					floppydisk->tracks[i] = allocCylinderEntry(rpm,floppydisk->floppyNumberOfSide);
					if(!floppydisk->tracks[i])
						goto error;
				}

				currentcylinder = floppydisk->tracks[i];

				currentcylinder->sides[j] = tg_alloctrack(floppydisk->floppyBitRate,UNKNOWN_ENCODING,currentcylinder->floppyRPM,tracksize*8,2500,-2500,0x00);

				currentside = currentcylinder->sides[j];
				currentside->number_of_sector=floppydisk->floppySectorPerTrack;

				fseek(f,(tracksize*i)+(tracksize*floppydisk->floppyNumberOfTrack*j),SEEK_SET);

				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"read track %d side %d at offset 0x%x (0x%x bytes)",
													i,
													j,
													ftell(f),
													tracksize);

				hxc_fread(currentside->databuffer,tracksize,f);

				for(k=0;k<tracksize;k++)
				{
					currentside->databuffer[k] = LUT_ByteBitsInverter[currentside->databuffer[k]];
				}
			}
		}

		hxc_fclose(f);
		return HXCFE_NOERROR;
	}

	hxc_fclose(f);

	return HXCFE_BADFILE;

error:

	hxc_fclose(f);

	return HXCFE_INTERNALERROR;
}

int FEI_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]="FEI";
	static const char plug_desc[]="FEI Loader";
	static const char plug_ext[]="fei";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   FEI_libIsValidDiskFile,
		(LOADDISKFILE)      FEI_libLoad_DiskFile,
		(WRITEDISKFILE)     0,
		(GETPLUGININFOS)    FEI_libGetPluginInfo
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
