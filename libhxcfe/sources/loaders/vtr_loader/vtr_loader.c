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
// File : vtr_loader.c
// Contains: VTR (VTrucco file format) floppy image loader
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
#include "libhxcfe.h"

#include "floppy_loader.h"
#include "floppy_utils.h"

#include "vtr_loader.h"
#include "vtr_format.h"

#include "libhxcadaptor.h"

#include "tracks/luts.h"

int VTR_libIsValidDiskFile( HXCFE_IMGLDR * imgldr_ctx, HXCFE_IMGLDR_FILEINFOS * imgfile )
{
	vtrucco_picfileformatheader *header;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"VTR_libIsValidDiskFile");

	if(imgfile)
	{
		header = (vtrucco_picfileformatheader *)&imgfile->file_header;

		if( !strncmp((char*)header->HEADERSIGNATURE,"VTrucco",7))
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"VTR_libIsValidDiskFile : VTrucco file !");
			return HXCFE_VALIDFILE;
		}
		else
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"VTR_libIsValidDiskFile : non VTrucco file !");
			return HXCFE_BADFILE;
		}
	}

	return HXCFE_BADPARAMETER;
}

int VTR_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	FILE * f;
	vtrucco_picfileformatheader header;
	int i,j,k,l,offset,offset2;
	HXCFE_CYLINDER* currentcylinder;
	HXCFE_SIDE* currentside;
	vtrucco_pictrack* trackoffsetlist;
	unsigned int tracks_base;
	unsigned char * hfetrack;
	int nbofblock,tracklen;

	hfetrack = NULL;
	trackoffsetlist = NULL;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"VTR_libLoad_DiskFile %s",imgfile);

	f = hxc_fopen(imgfile,"rb");
	if( f == NULL )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	hxc_fread(&header,sizeof(header),f);

	if(!strncmp((char*)header.HEADERSIGNATURE,"VTrucco",7))
	{
		floppydisk->floppyNumberOfTrack=header.number_of_track;
		floppydisk->floppyNumberOfSide=header.number_of_side;
		floppydisk->floppyBitRate=header.bitRate*1000;
		floppydisk->floppySectorPerTrack=-1;
		floppydisk->floppyiftype=header.floppyinterfacemode;

		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"VTrucco File : %d track, %d side, %d bit/s, %d sectors, mode %d",
			floppydisk->floppyNumberOfTrack,
			floppydisk->floppyNumberOfSide,
			floppydisk->floppyBitRate,
			floppydisk->floppySectorPerTrack,
			floppydisk->floppyiftype);

		trackoffsetlist=(vtrucco_pictrack*)malloc(sizeof(vtrucco_pictrack)* header.number_of_track);
		if(!trackoffsetlist)
			goto alloc_error;

		memset(trackoffsetlist,0,sizeof(vtrucco_pictrack)* header.number_of_track);

		fseek( f,512,SEEK_SET);
		hxc_fread( trackoffsetlist,sizeof(vtrucco_pictrack)* header.number_of_track,f);

		tracks_base= 512+( (((sizeof(vtrucco_pictrack)* header.number_of_track)/512)+1)*512);
		fseek( f,tracks_base,SEEK_SET);

		floppydisk->tracks=(HXCFE_CYLINDER**)malloc(sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);
		if(!floppydisk->tracks)
			goto alloc_error;

		memset(floppydisk->tracks,0,sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);

		for(i=0;i<floppydisk->floppyNumberOfTrack;i++)
		{
			fseek(f,(trackoffsetlist[i].offset*512),SEEK_SET);
			if(trackoffsetlist[i].track_len&0x1FF)
			{
				tracklen=(trackoffsetlist[i].track_len&(~0x1FF))+0x200;
			}
			else
			{
				tracklen=trackoffsetlist[i].track_len;
			}

			hfetrack = (unsigned char*)malloc( tracklen );
			if(!hfetrack)
				goto alloc_error;

			hxc_fread( hfetrack,tracklen,f);

			floppydisk->tracks[i] = allocCylinderEntry(300,floppydisk->floppyNumberOfSide);
			if(!floppydisk->tracks[i])
				goto alloc_error;

			currentcylinder=floppydisk->tracks[i];

		/*  imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"read track %d side %d at offset 0x%x (0x%x bytes)",
			trackdesc.track_number,
			trackdesc.side_number,
			trackdesc.mfmtrackoffset,
			trackdesc.mfmtracksize);
*/
			for(j=0;j<currentcylinder->number_of_side;j++)
			{
				hxcfe_imgCallProgressCallback(imgldr_ctx,(i<<1) + (j&1),floppydisk->floppyNumberOfTrack*2 );

				currentcylinder->sides[j]=malloc(sizeof(HXCFE_SIDE));
				if(!currentcylinder->sides[j])
					goto alloc_error;

				memset(currentcylinder->sides[j],0,sizeof(HXCFE_SIDE));
				currentside=currentcylinder->sides[j];

				currentside->number_of_sector=floppydisk->floppySectorPerTrack;
				currentside->tracklen=tracklen/2;

				currentside->databuffer=malloc(currentside->tracklen);
				if(!currentside->databuffer)
					goto alloc_error;

				memset(currentside->databuffer,0,currentside->tracklen);

				currentside->flakybitsbuffer=0;

				currentside->indexbuffer = malloc(currentside->tracklen);
				if(!currentside->indexbuffer)
					goto alloc_error;

				memset(currentside->indexbuffer,0,currentside->tracklen);

				for(k=0;k<256;k++)
				{
					currentside->indexbuffer[k]=0xFF;
				}

				currentside->timingbuffer=0;
				currentside->bitrate=floppydisk->floppyBitRate;
				currentside->track_encoding=UNKNOWN_ENCODING;

				nbofblock=(currentside->tracklen/256);
				for(k=0;k<nbofblock;k++)
				{
					for(l=0;l<256;l++)
					{
						offset=(k*256)+l;
						offset2=(k*512)+l+(256*j);
						currentside->databuffer[offset]=LUT_ByteBitsInverter[hfetrack[offset2]];
					}
				}

				currentside->tracklen=currentside->tracklen*8;
			}

			free(hfetrack);
			hfetrack = NULL;
		}

		free(trackoffsetlist);

		hxc_fclose(f);
		return HXCFE_NOERROR;
	}

	hxc_fclose(f);
	imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"bad header");
	return HXCFE_BADFILE;

alloc_error:
	free(hfetrack);
	free(trackoffsetlist);

	if( f )
		hxc_fclose(f);

	hxcfe_freeFloppy(imgldr_ctx->hxcfe, floppydisk );

	return HXCFE_INTERNALERROR;
}

int VTR_libWrite_DiskFile(HXCFE_IMGLDR* imgldr_ctx,HXCFE_FLOPPY * floppy,char * filename);

int VTR_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]="VTR_IMG";
	static const char plug_desc[]="VTR IMG Loader";
	static const char plug_ext[]="vtr";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   VTR_libIsValidDiskFile,
		(LOADDISKFILE)      VTR_libLoad_DiskFile,
		(WRITEDISKFILE)     VTR_libWrite_DiskFile,
		(GETPLUGININFOS)    VTR_libGetPluginInfo
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

