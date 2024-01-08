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
// File : qd_loader.c
// Contains: HxC Quickdisk floppy image loader
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
#include "tracks/track_generator.h"
#include "libhxcfe.h"

#include "floppy_loader.h"
#include "floppy_utils.h"

#include "qd_loader.h"
#include "qd_format.h"

#include "tracks/luts.h"

#include "libhxcadaptor.h"

int QD_libIsValidDiskFile( HXCFE_IMGLDR * imgldr_ctx, HXCFE_IMGLDR_FILEINFOS * imgfile )
{
	qdhfefileformatheader * header;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"QD_libIsValidDiskFile");

	if(hxc_checkfileext(imgfile->path,"qd",SYS_PATH_TYPE))
	{
		header = (qdhfefileformatheader *)&imgfile->file_header;

		if( !strncmp((char*)header->HEADERSIGNATURE,"HXCQDDRV",8) || (header->HEADERSIGNATURE[3] == 'Q' && header->HEADERSIGNATURE[4] == 'D') )
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"QD_libIsValidDiskFile : QD file !");
			return HXCFE_VALIDFILE;
		}
		else
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"QD_libIsValidDiskFile : non QD file !");
			return HXCFE_BADFILE;
		}
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"QD_libIsValidDiskFile : non QD file !");
		return HXCFE_BADFILE;
	}
}

int QD_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	FILE * f;
	qdhfefileformatheader header;
	qdtrack trackdesc;
	unsigned int i,j;
	int startswpos,stopswpos;
	HXCFE_CYLINDER* currentcylinder;
	HXCFE_SIDE* currentside;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"QD_libLoad_DiskFile %s",imgfile);

	f = hxc_fopen(imgfile,"rb");
	if( f == NULL )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	memset(&header,0,sizeof(qdhfefileformatheader));
	memset(&trackdesc,0,sizeof(qdtrack));

	hxc_fread(&header,sizeof(header),f);

	if(!strncmp((char*)header.HEADERSIGNATURE,"HXCQDDRV",8) || (header.HEADERSIGNATURE[3] == 'Q' && header.HEADERSIGNATURE[4] == 'D'))
	{
		if( !header.number_of_track && !header.number_of_side && !header.bitRate && !header.track_list_offset)
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_WARNING,"Malformed QD File - Guessing some parameters...");
			header.number_of_track = 1;
			header.number_of_side = 1;
			header.track_list_offset = 512;
			header.bitRate = 203128;
		}

		floppydisk->floppyNumberOfTrack = header.number_of_track;
		floppydisk->floppyNumberOfSide = header.number_of_side;
		floppydisk->floppyBitRate = header.bitRate / 2;
		floppydisk->floppySectorPerTrack=-1;
		floppydisk->floppyiftype = QUICKDISK_FLOPPYMODE;

		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"QD File : %d track, %d side, %d bit/s, %d sectors, mode %d",
			floppydisk->floppyNumberOfTrack,
			floppydisk->floppyNumberOfSide,
			floppydisk->floppyBitRate,
			floppydisk->floppySectorPerTrack,
			floppydisk->floppyiftype);

		floppydisk->tracks = (HXCFE_CYLINDER**)malloc(sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);
		if(!floppydisk->tracks)
			goto error;

		memset(floppydisk->tracks,0,sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);

		for(i=0;i<(unsigned int)(floppydisk->floppyNumberOfTrack*floppydisk->floppyNumberOfSide);i++)
		{
			hxcfe_imgCallProgressCallback(imgldr_ctx,i,(floppydisk->floppyNumberOfTrack*floppydisk->floppyNumberOfSide) );

			fseek(f,(header.track_list_offset)+(i*sizeof(qdtrack)),SEEK_SET);
			hxc_fread(&trackdesc,sizeof(qdtrack),f);
			fseek(f,trackdesc.offset,SEEK_SET);

			if(!floppydisk->tracks[i>>1])
			{
				floppydisk->tracks[i>>1] = allocCylinderEntry(0,floppydisk->floppyNumberOfSide);
			}

			currentcylinder = floppydisk->tracks[i>>1];

			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Read track %d side %d at offset 0x%x (0x%x bytes)",
												i>>1,
												i&1,
												trackdesc.offset,
												trackdesc.track_len);

			startswpos = (int)((float)(trackdesc.start_sw_position * 4 * 1000) / (float)floppydisk->floppyBitRate) * 1000;
			stopswpos = (int)((float)(trackdesc.stop_sw_position * 4 * 1000) / (float)floppydisk->floppyBitRate) * 1000;

			currentcylinder->sides[i&1] = tg_alloctrack(floppydisk->floppyBitRate, UNKNOWN_ENCODING, 0, trackdesc.track_len*8,20*1000,startswpos,0x00);
			currentside = currentcylinder->sides[i&1];

			fillindex(stopswpos,currentside,20*1000,1,0);

			currentside->number_of_sector = floppydisk->floppySectorPerTrack;

			currentcylinder->floppyRPM = (short)( 60 / GetTrackPeriod(imgldr_ctx->hxcfe,currentside) );

			hxc_fread(currentside->databuffer,currentside->tracklen/8,f);

			for(j=0;j<trackdesc.track_len;j++)
			{
				currentside->databuffer[j] = LUT_ByteBitsInverter[currentside->databuffer[j]];
			}
		}

		hxc_fclose(f);
		return HXCFE_NOERROR;
	}

	hxc_fclose(f);
	imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"bad header");
	return HXCFE_BADFILE;

error:
	hxc_fclose(f);
	imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Error while loading QD file...");
	return HXCFE_BADFILE;
}

int QD_libWrite_DiskFile(HXCFE_IMGLDR* imgldr_ctx,HXCFE_FLOPPY * floppy,char * filename);

int QD_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]="HXC_QD";
	static const char plug_desc[]="HXC Quickdisk image Loader";
	static const char plug_ext[]="qd";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   QD_libIsValidDiskFile,
		(LOADDISKFILE)      QD_libLoad_DiskFile,
		(WRITEDISKFILE)     QD_libWrite_DiskFile,
		(GETPLUGININFOS)    QD_libGetPluginInfo
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
