/*
//
// Copyright (C) 2006-2019 Jean-François DEL NERO
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
// File : streamhfe_loader.c
// Contains: HFE floppy image loader
//
// Written by:	Jean-François DEL NERO
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

#include "streamhfe_loader.h"
#include "streamhfe_format.h"

#include "libhxcadaptor.h"

#include "tracks/trackutils.h"

#include "tracks/luts.h"

static char * interfacemodecode[]=
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

int STREAMHFE_libIsValidDiskFile( HXCFE_IMGLDR * imgldr_ctx, HXCFE_IMGLDR_FILEINFOS * imgfile )
{
	streamhfe_fileheader * header;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"STREAMHFE_libIsValidDiskFile");

	if(imgfile)
	{
		header = (streamhfe_fileheader *)&imgfile->file_header;

		if( !strncmp((char*)header->signature,"HxC_Stream_Image",16))
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"STREAMHFE_libIsValidDiskFile : Stream HFE file !");
			return HXCFE_VALIDFILE;
		}
		else
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"STREAMHFE_libIsValidDiskFile : non Stream HFE file !");
			return HXCFE_BADFILE;
		}
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"STREAMHFE_libIsValidDiskFile : non Stream HFE file !");
		return HXCFE_BADFILE;
	}
}

static HXCFE_SIDE* decodestream(HXCFE* floppycontext, streamhfe_track_def * trackdef,unsigned short * unpacked_data,int side,short * rpm,float timecoef,int phasecorrection,int revolution)
{
	HXCFE_SIDE* currentside;
	int i,k,l,offset;

	HXCFE_TRKSTREAM *track_dump;
	HXCFE_FXSA * fxs;

	unsigned short * tmp_trackbuf;

	uint32_t * trackbuf_dword;
	uint32_t realnumberofpulses;
	uint32_t curpulselength;
	uint32_t j,index_position[16];

	uint32_t pulses_count;
	currentside = NULL;

	floppycontext->hxc_printf(MSG_DEBUG,"------------------------------------------------");

	floppycontext->hxc_printf(MSG_DEBUG,"Loading Stream HFE track...");

	fxs = hxcfe_initFxStream(floppycontext);
	if(fxs)
	{
		//Unpack data...
		tmp_trackbuf = malloc(trackdef->unpacked_data_size/2);
		if(!tmp_trackbuf)
			return NULL;

		for(i = 0;i<(trackdef->track_len)/32;i++)
		{
			tmp_trackbuf[i] = unpacked_data[(i*2) + (side&1)];
		}

		pulses_count = 0;
		for(i=0;i<(trackdef->track_len)/2;i++)
		{
			if(getbit((unsigned char *)tmp_trackbuf,i))
				pulses_count++;
		}

		if(!pulses_count)
			pulses_count++;

		if(pulses_count)
		{
			offset = 0;

			realnumberofpulses = 0;

			trackbuf_dword = malloc(((pulses_count*revolution)+1)*sizeof(uint32_t));
			if(trackbuf_dword)
			{
				memset(trackbuf_dword,0x00,((pulses_count*revolution)+1)*sizeof(uint32_t));

				realnumberofpulses = 0;
				j = 0;
				l = 0;
				k = 0;
				curpulselength = 1;
				while(j < ((trackdef->track_len)/2) * revolution )
				{
					if(!getbit((unsigned char *)tmp_trackbuf,j%((trackdef->track_len)/2)))
					{
						curpulselength++;
					}
					else
					{
						trackbuf_dword[k] = curpulselength;
						curpulselength = 1;
						k++;
						realnumberofpulses++;
					}

					if( j%((trackdef->track_len)/2) > (j+1)%((trackdef->track_len)/2) )
					{
						index_position[l++] = k;
					}

					j++;
				}

				// dummy pulse
				trackbuf_dword[realnumberofpulses] = 300;
				realnumberofpulses++;
			}

			free(tmp_trackbuf);

			hxcfe_FxStream_setResolution(fxs,DEFAULT_BITS_PERIOD);

			track_dump = hxcfe_FxStream_ImportStream(fxs,trackbuf_dword,32,(realnumberofpulses));
			if(track_dump)
			{
				if(revolution)
				{
					offset = 0;
					hxcfe_FxStream_AddIndex(fxs,track_dump,offset,0,FXSTRM_INDEX_MAININDEX);

					for(i=0;i<revolution;i++)
					{
						hxcfe_FxStream_AddIndex(fxs,track_dump,index_position[i],0,FXSTRM_INDEX_MAININDEX);
					}
				}

				currentside = hxcfe_FxStream_AnalyzeAndGetTrack(fxs,track_dump);

				if(currentside)
				{
					if(rpm)
						*rpm = (short)( 60 / GetTrackPeriod(floppycontext,currentside) );
				}

				hxcfe_FxStream_FreeStream(fxs,track_dump);
			}

			free(trackbuf_dword);

		}

		hxcfe_deinitFxStream(fxs);
	}

	floppycontext->hxc_printf(MSG_DEBUG,"------------------------------------------------");

	return currentside;
}

void fix_track(	HXCFE_SIDE * curside )
{
	int k,l,tracklen;
	int previous_bit;

	if(!curside)
		return;

	tracklen = curside->tracklen /8;
	if(curside->tracklen & 7)
		tracklen++;

	previous_bit = 0;
	for(k=0;k<tracklen;k++)
	{
		if(previous_bit)
		{
			if(curside->databuffer[k] & 0x80)
			{
				curside->databuffer[k] = curside->databuffer[k] ^ 0x80;
				curside->flakybitsbuffer[k] =  curside->flakybitsbuffer[k] | (0x80);
			}
		}

		for(l=0;l<7;l++)
		{
			if((curside->databuffer[k] & (0xC0>>l)) == (0xC0>>l))
			{
				curside->databuffer[k] = curside->databuffer[k] ^ (0x40>>l);
				curside->flakybitsbuffer[k] =  curside->flakybitsbuffer[k] | (0x40>>l);
			}
		}

		previous_bit = curside->databuffer[k] & 1;
	}

	return;
}

int STREAMHFE_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	FILE * f;
	short rpm;
	streamhfe_fileheader header;
	int i,j;
	HXCFE_CYLINDER* currentcylinder;
	HXCFE_SIDE * curside;
	int phasecorrection;
	float timecoef;
	streamhfe_track_def * trackoffsetlist;
	unsigned char * packed_track_data;
	unsigned short * unpacked_track;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"STREAMHFE_libLoad_DiskFile %s",imgfile);

	f = hxc_fopen(imgfile,"rb");
	if( f == NULL )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	hxc_fread(&header,sizeof(header),f);

	if(!strncmp((char*)header.signature,"HxC_Stream_Image",16))
	{
		floppydisk->floppyNumberOfTrack = header.number_of_track;
		floppydisk->floppyNumberOfSide = header.number_of_side;

		floppydisk->floppyBitRate = VARIABLEBITRATE;
		floppydisk->floppySectorPerTrack = -1;
		floppydisk->floppyiftype = header.floppyinterfacemode;

		phasecorrection = 8;
		timecoef = 1;

		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Stream HFE File : %d track, %d side, %d bit/s, %d sectors, interface mode %s",
			floppydisk->floppyNumberOfTrack,
			floppydisk->floppyNumberOfSide,
			floppydisk->floppyBitRate,
			floppydisk->floppySectorPerTrack,
			floppydisk->floppyiftype<0xC?interfacemodecode[floppydisk->floppyiftype]:"Unknow!");

        trackoffsetlist = (streamhfe_track_def*)malloc(sizeof(streamhfe_track_def) * header.number_of_track);
		if(!trackoffsetlist)
			goto error;

        memset( trackoffsetlist, 0, sizeof(streamhfe_track_def) * header.number_of_track);
        fseek( f,header.track_list_offset,SEEK_SET);
        hxc_fread( trackoffsetlist, sizeof(streamhfe_track_def) * header.number_of_track, f);

		floppydisk->tracks = (HXCFE_CYLINDER**)malloc(sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);
		if(!floppydisk->tracks)
			goto error;

		memset(floppydisk->tracks,0,sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);

		for(j=0;j<floppydisk->floppyNumberOfTrack;j++)
		{
			hxcfe_imgCallProgressCallback(imgldr_ctx, j,(floppydisk->floppyNumberOfTrack) );
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Load Track %.3d",j);

			fseek(f, trackoffsetlist[j].packed_data_offset,SEEK_SET);
			if(trackoffsetlist[j].packed_data_size)
			{
				packed_track_data = malloc(trackoffsetlist[j].packed_data_size);
				if(!packed_track_data)
					goto error;

				hxc_fread(packed_track_data,trackoffsetlist[j].packed_data_size,f);

				//Unpack data...
				unpacked_track = malloc(trackoffsetlist[j].unpacked_data_size);
				if(!unpacked_track)
					goto error;

				memcpy(unpacked_track,packed_track_data,trackoffsetlist[j].unpacked_data_size);

				curside = decodestream(imgldr_ctx->hxcfe, &trackoffsetlist[j],unpacked_track,0,&rpm,timecoef,phasecorrection,4);
				fix_track(curside);

				if(!floppydisk->tracks[j])
				{
					floppydisk->tracks[j] = allocCylinderEntry(rpm,floppydisk->floppyNumberOfSide);
				}

				currentcylinder=floppydisk->tracks[j];

				currentcylinder->sides[0] = curside;

				if(floppydisk->floppyNumberOfSide == 2)
				{
					curside = decodestream(imgldr_ctx->hxcfe, &trackoffsetlist[j],unpacked_track,1,&rpm,timecoef,phasecorrection,4);
					fix_track(curside);
					currentcylinder->sides[1] = curside;
				}

				free(unpacked_track);
				free(packed_track_data);
			}
		}

		hxc_fclose(f);

		// Adjust track timings.
		for(j=0;j<floppydisk->floppyNumberOfTrack;j++)
		{
			for(i=0;i<floppydisk->floppyNumberOfSide;i++)
			{
				curside = floppydisk->tracks[j]->sides[i];
				if(curside && floppydisk->tracks[0]->sides[0])
				{
					AdjustTrackPeriod(imgldr_ctx->hxcfe,floppydisk->tracks[0]->sides[0],curside);
				}
			}
		}

		imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"track file successfully loaded and encoded!");

		hxcfe_sanityCheck(imgldr_ctx->hxcfe,floppydisk);

		return HXCFE_NOERROR;
	}

	hxc_fclose(f);
	imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"bad header");
	return HXCFE_BADFILE;

error:
	hxc_fclose(f);

	return HXCFE_INTERNALERROR;
}

int STREAMHFE_libWrite_DiskFile(HXCFE_IMGLDR* imgldr_ctx,HXCFE_FLOPPY * floppy,char * filename);

int STREAMHFE_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{

	static const char plug_id[]="HXC_STREAMHFE";
	static const char plug_desc[]="Stream HFE file Loader";
	static const char plug_ext[]="hfe";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)	STREAMHFE_libIsValidDiskFile,
		(LOADDISKFILE)		STREAMHFE_libLoad_DiskFile,
		(WRITEDISKFILE)		STREAMHFE_libWrite_DiskFile,
		(GETPLUGININFOS)	STREAMHFE_libGetPluginInfo
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
