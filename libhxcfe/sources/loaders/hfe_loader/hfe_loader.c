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
// File : hfe_loader.c
// Contains: HFE floppy image loader
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

#include "floppy_loader.h"
#include "floppy_utils.h"

#include "hfe_loader.h"
#include "hfe_format.h"

#include "libhxcadaptor.h"

#include "tracks/luts.h"

const char * trackencodingcode[]=
{
	"ISOIBM_MFM_ENCODING",
	"AMIGA_MFM_ENCODING",
	"ISOIBM_FM_ENCODING",
	"EMU_FM_ENCODING",
	"UNKNOWN_ENCODING"
};

const char * interfacemodecode[]=
{
	"IBMPC_DD_FLOPPYMODE",
	"IBMPC_HD_FLOPPYMODE",
	"ATARIST_DD_FLOPPYMODE",
	"ATARIST_HD_FLOPPYMODE",
	"AMIGA_DD_FLOPPYMODE",
	"AMIGA_HD_FLOPPYMODE",
	"CPC_DD_FLOPPYMODE",
	"GENERIC_SHUGART_DD_FLOPPYMODE",
	"IBMPC_ED_FLOPPYMODE",
	"MSX2_DD_FLOPPYMODE",
	"C64_DD_FLOPPYMODE",
	"EMU_SHUGART_FLOPPYMODE"
};

int HFE_libIsValidDiskFile( HXCFE_IMGLDR * imgldr_ctx, HXCFE_IMGLDR_FILEINFOS * imgfile )
{
	picfileformatheader * header;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"HFE_libIsValidDiskFile");

	if(imgfile)
	{
		header = (picfileformatheader *)&imgfile->file_header;

		if( !strncmp((char*)header->HEADERSIGNATURE,"HXCPICFE",8))
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"HFE_libIsValidDiskFile : HFE file !");
			return HXCFE_VALIDFILE;
		}
		else
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"HFE_libIsValidDiskFile : non HFE file !");
			return HXCFE_BADFILE;
		}
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"HFE_libIsValidDiskFile : non HFE file !");
		return HXCFE_BADFILE;
	}
}

int HFE_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	FILE * f;
	picfileformatheader header;
	int i,j,k,l,offset,offset2;
	HXCFE_CYLINDER* currentcylinder;
	HXCFE_SIDE* currentside;
	pictrack* trackoffsetlist;
	unsigned int tracks_base;
	unsigned char * hfetrack;
	int nbofblock,tracklen;

	trackoffsetlist = NULL;
	currentcylinder = NULL;
	currentside = NULL;
	hfetrack = NULL;
	f = NULL;
	floppydisk->tracks = NULL;

	memset(&header,0,sizeof(picfileformatheader));

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"HFE_libLoad_DiskFile %s",imgfile);

	f = hxc_fopen(imgfile,"rb");
	if( f == NULL )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	// Reading the header
	hxc_fread(&header,sizeof(header),f);

	if(!strncmp((char*)header.HEADERSIGNATURE,"HXCPICFE",8))
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"HFE File : Format v%d, %d track, %d side, %d bit/s, interface mode %s, track encoding:%s",
			header.formatrevision+1,
			header.number_of_track,
			header.number_of_side,
			header.bitRate*1000,
			floppydisk->floppyiftype<0xC?interfacemodecode[header.floppyinterfacemode]:"Unknow!",
			(header.track_encoding&(~3))?trackencodingcode[4]:trackencodingcode[header.track_encoding&0x3]);

		if( header.formatrevision != 0 )
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"HFE File : Format version %d currently not supported !",header.formatrevision+1);

			hxc_fclose(f);
			return HXCFE_BADFILE;
		}

		if( !header.number_of_track || !header.number_of_side )
		{
			// Nothing to Load ?!
			imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"HFE File : 0 track or side ! Nothing to load !");

			hxc_fclose(f);
			return HXCFE_BADFILE;
		}

		if( !header.bitRate )
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"HFE File : Null bitrate !");

			hxc_fclose(f);
			return HXCFE_BADFILE;
		}

		floppydisk->floppyNumberOfTrack = header.number_of_track;
		floppydisk->floppyNumberOfSide = header.number_of_side;
		floppydisk->floppyBitRate = header.bitRate*1000;
		floppydisk->floppySectorPerTrack = -1;
		floppydisk->floppyiftype=header.floppyinterfacemode;

		trackoffsetlist = (pictrack*)malloc(sizeof(pictrack)* header.number_of_track);
		if(!trackoffsetlist)
			goto alloc_error;

		memset(trackoffsetlist,0,sizeof(pictrack)* header.number_of_track);
		fseek( f,512,SEEK_SET);
		hxc_fread( trackoffsetlist,sizeof(pictrack)* header.number_of_track,f);

		tracks_base= 512 + ( (((sizeof(pictrack)* header.number_of_track)/512)+1)*512);
		fseek( f,tracks_base,SEEK_SET);

		floppydisk->tracks = (HXCFE_CYLINDER**)malloc(sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);
		if(!floppydisk->tracks)
			goto alloc_error;

		memset(floppydisk->tracks,0,sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);

		for(i=0;i<floppydisk->floppyNumberOfTrack;i++)
		{
			fseek(f,(trackoffsetlist[i].offset*512),SEEK_SET);
			if(trackoffsetlist[i].track_len&0x1FF)
			{
				tracklen = (trackoffsetlist[i].track_len&(~0x1FF))+0x200;
			}
			else
			{
				tracklen = trackoffsetlist[i].track_len;
			}

			if(!tracklen)
			{
				imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"HFE File : Invalid track %d lentght ! Stopping here !",i);
				break;
			}

			hfetrack = (unsigned char*)malloc( tracklen );
			if(!hfetrack)
				goto alloc_error;

			memset(hfetrack,0,tracklen);

			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"HFE File : reading track %d, track size:%d - file offset:%.8X", i, tracklen, (trackoffsetlist[i].offset*512));

			hxc_fread( hfetrack,tracklen,f);

			floppydisk->tracks[i] = (HXCFE_CYLINDER*)malloc(sizeof(HXCFE_CYLINDER));
			if( !floppydisk->tracks[i] )
				goto alloc_error;

			currentcylinder = floppydisk->tracks[i];
			currentcylinder->number_of_side = floppydisk->floppyNumberOfSide;
			currentcylinder->sides = (HXCFE_SIDE**)malloc(sizeof(HXCFE_SIDE*)*currentcylinder->number_of_side);
			if( !currentcylinder->sides )
				goto alloc_error;

			memset(currentcylinder->sides,0,sizeof(HXCFE_SIDE*)*currentcylinder->number_of_side);
			currentcylinder->floppyRPM = header.floppyRPM;

			for(j=0;j<currentcylinder->number_of_side;j++)
			{
				hxcfe_imgCallProgressCallback(imgldr_ctx,(i<<1) + (j&1),floppydisk->floppyNumberOfTrack*2 );

				currentcylinder->sides[j] = calloc( 1, sizeof(HXCFE_SIDE));
				if(!currentcylinder->sides[j])
					goto alloc_error;

				currentside = currentcylinder->sides[j];

				currentside->number_of_sector = floppydisk->floppySectorPerTrack;
				currentside->tracklen=tracklen/2;

				currentside->databuffer = calloc( 1, currentside->tracklen );
				if(!currentside->databuffer)
					goto alloc_error;

				currentside->flakybitsbuffer=0;

				currentside->indexbuffer = calloc( 1, currentside->tracklen);
				if(!currentside->indexbuffer)
					goto alloc_error;

				for(k=0;k<256;k++)
				{
					currentside->indexbuffer[k] = 0xFF;
				}

				currentside->timingbuffer = 0;
				currentside->bitrate = floppydisk->floppyBitRate;

				currentside->track_encoding = header.track_encoding;

				if( i == 0 )
				{
					if ( j == 0 )
					{
						if(!header.track0s0_altencoding)
						{
							currentside->track_encoding = header.track0s0_encoding;
						}
					}
					else
					{
						if(!header.track0s1_altencoding)
						{
							currentside->track_encoding = header.track0s1_encoding;
						}
					}
				}

				nbofblock = (currentside->tracklen/256);
				for(k=0;k<nbofblock;k++)
				{
					for(l=0;l<256;l++)
					{
						offset = (k*256)+l;
						offset2 = (k*512)+l+(256*j);
						currentside->databuffer[offset] = LUT_ByteBitsInverter[hfetrack[offset2]];
					}
				}

				currentside->tracklen = currentside->tracklen*8;

				if(!currentcylinder->floppyRPM)
					currentcylinder->floppyRPM = (short)( 60 / GetTrackPeriod(imgldr_ctx->hxcfe,currentside) );

			}

			free(hfetrack);
			hfetrack = NULL;
		}

		free(trackoffsetlist);

		hxc_fclose(f);

		hxcfe_sanityCheck(imgldr_ctx->hxcfe,floppydisk);

		return HXCFE_NOERROR;
	}

	hxc_fclose(f);

	imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"HFE File : Bad file header !");

	return HXCFE_BADFILE;

alloc_error:
	imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"HFE File : Internal memory allocation error ! Please report !");

	hxcfe_freeFloppy(imgldr_ctx->hxcfe, floppydisk );

	if(f)
		hxc_fclose(f);

	free(hfetrack);

	free(trackoffsetlist);

	return HXCFE_INTERNALERROR;
}

int HFE_libWrite_DiskFile(HXCFE_IMGLDR* imgldr_ctx,HXCFE_FLOPPY * floppy,char * filename);

int HFE_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]="HXC_HFE";
	static const char plug_desc[]="SD Card HxCFE HFE file Loader";
	static const char plug_ext[]="hfe";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   HFE_libIsValidDiskFile,
		(LOADDISKFILE)      HFE_libLoad_DiskFile,
		(WRITEDISKFILE)     HFE_libWrite_DiskFile,
		(GETPLUGININFOS)    HFE_libGetPluginInfo
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

int EXTHFE_libWrite_DiskFile(HXCFE_IMGLDR* imgldr_ctx,HXCFE_FLOPPY * floppy,char * filename);

int EXTHFE_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]="HXC_EXTHFE";
	static const char plug_desc[]="SD Card HxCFE EXTENDED HFE file Loader";
	static const char plug_ext[]="hfe";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   0,
		(LOADDISKFILE)      0,
		(WRITEDISKFILE)     EXTHFE_libWrite_DiskFile,
		(GETPLUGININFOS)    EXTHFE_libGetPluginInfo
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

int HFE_HDDD_A2_libWrite_DiskFile(HXCFE_IMGLDR* imgldr_ctx,HXCFE_FLOPPY * floppy,char * filename);

int HFE_HDDD_A2_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]="HXC_HDDD_A2_HFE";
	static const char plug_desc[]="SD Card HxCFE HFE file Loader (HDDD A2 Support)";
	static const char plug_ext[]="hfe";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   0,
		(LOADDISKFILE)      0,
		(WRITEDISKFILE)     HFE_HDDD_A2_libWrite_DiskFile,
		(GETPLUGININFOS)    HFE_HDDD_A2_libGetPluginInfo
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
