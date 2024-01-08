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
// File : fdx_loader.c
// Contains: FDX floppy image loader
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

#include "tracks/encoding/mfm_encoding.h"

#include "floppy_loader.h"
#include "floppy_utils.h"

#include "fdx_loader.h"
#include "fdx_writer.h"

#include "fdx_format.h"

#include "libhxcadaptor.h"
#include "tracks/crc.h"

#define FDX_NB_FAKE_REV 5

#define FDX_DBG 1

int FDX_libIsValidDiskFile( HXCFE_IMGLDR * imgldr_ctx, HXCFE_IMGLDR_FILEINFOS * imgfile )
{
	fdxheader_t * fileheader;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"FDX_libIsValidDiskFile");

	if(hxc_checkfileext(imgfile->path,"fdx",SYS_PATH_TYPE))
	{
		fileheader = (fdxheader_t*)imgfile->file_header;

		if(memcmp((char*)&fileheader->fdx_signature,"FDX",3) || !(fileheader->nb_of_cylinders && fileheader->nb_of_cylinders <= 86 && fileheader->nb_of_heads >= 1 && fileheader->nb_of_heads <= 2 ) )
		{

			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"FDX_libIsValidDiskFile : non FDX file (bad header)!");
			return HXCFE_BADFILE;
		}
		else
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"FDX_libIsValidDiskFile : FDX file !");
			return HXCFE_VALIDFILE;
		}
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"FDX_libIsValidDiskFile : non FDX file !");
		return HXCFE_BADFILE;
	}
}

int raw2stream(uint8_t * buf,uint32_t * stream_buffer,int size,int rev, int * indexpos)
{
	int i,s,cnt,delta;
	int off;

	delta = 0;
	cnt = 0;
	s = 0;
	for(i=0;i<(size*rev);i++)
	{
		off = i % size;

		if(!(i%size))
		{
			if(indexpos)
			{
				*indexpos = cnt;
				indexpos++;
			}
		}

		if((buf[off>>3] & (0x80>>(off&7))))
		{
			if(!s)
			{
				stream_buffer[cnt] = delta+1;
				cnt++;
				delta = 0;
			}
			else
			{
				delta++;
			}

			s = 1;
		}
		else
		{
			delta++;
			s = 0;
		}
	}

	return cnt;
}

int count_pulses(uint8_t * buf, int size)
{
	int i,s,cnt;

	cnt = 0;
	s = 0;
	for(i=0;i<size;i++)
	{
		if((buf[i>>3] & (0x80>>(i&7))))
		{
			if(!s)
			{
				cnt++;
			}

			s = 1;
		}
		else
		{
			s = 0;
		}
	}

	return cnt;
}

static HXCFE_SIDE* decodestream(HXCFE* floppycontext,uint8_t * track_buffer,int bit_track_length,int track,short * rpm,float timecoef,int phasecorrection,int revolution, int bitrate,int filter,int filterpasses, int bmpexport, int fdxfreq)
{
	HXCFE_SIDE* currentside;
	int pulses_cnt;
	HXCFE_TRKSTREAM *track_dump;
	HXCFE_FXSA * fxs;
	char tmp_filename[512];
	int tick_period,freq;
	int indexarray[FDX_NB_FAKE_REV+1];
	uint32_t * trackbuf_dword;
	int i;

	currentside = NULL;

	floppycontext->hxc_printf(MSG_DEBUG,"------------------------------------------------");

	freq = hxcfe_getEnvVarValue( floppycontext, "FDXLOADER_SAMPLE_FREQUENCY_MHZ" );

	tick_period = ((float)1E9/(float)fdxfreq);

	if(freq>0 && freq <= 1000)
	{
		tick_period = (int)((float)tick_period * (float)((float)100  / (float)freq));
	}

	floppycontext->hxc_printf(MSG_INFO_1,"Sample tick period : %d ps", tick_period);

	fxs = hxcfe_initFxStream(floppycontext);
	if(fxs)
	{
		hxcfe_FxStream_setBitrate(fxs,bitrate);

		hxcfe_FxStream_setPhaseCorrectionFactor(fxs,phasecorrection);

		hxcfe_FxStream_setFilterParameters(fxs,filterpasses,filter);

		pulses_cnt = count_pulses(track_buffer, bit_track_length);

		#ifdef FDX_DBG
		floppycontext->hxc_printf(MSG_DEBUG,"FDX RAW Track pulses count : %d",pulses_cnt);
		#endif

		trackbuf_dword = malloc((pulses_cnt+1)*sizeof(uint32_t)*FDX_NB_FAKE_REV);
		if(!trackbuf_dword)
		{
			hxcfe_deinitFxStream(fxs);
			return NULL;
		}

		memset(trackbuf_dword,0x00,(pulses_cnt+1)*sizeof(uint32_t) * FDX_NB_FAKE_REV);
		pulses_cnt = raw2stream(track_buffer,trackbuf_dword,bit_track_length,FDX_NB_FAKE_REV,(int*)&indexarray);

		hxcfe_FxStream_setResolution(fxs, tick_period);

		track_dump = hxcfe_FxStream_ImportStream(fxs,trackbuf_dword,32,(pulses_cnt), HXCFE_STREAMCHANNEL_TYPE_RLEEVT, "data", NULL);
		if(track_dump)
		{
			for(i=0;i<FDX_NB_FAKE_REV;i++)
			{
				hxcfe_FxStream_AddIndex(fxs,track_dump,indexarray[i],0,FXSTRM_INDEX_MAININDEX);
			}

			hxcfe_FxStream_ChangeSpeed(fxs,track_dump,timecoef);

			fxs->pll.track = track>>1;
			fxs->pll.side = track&1;
			currentside = hxcfe_FxStream_AnalyzeAndGetTrack(fxs,track_dump);

			if(currentside)
			{
				if(rpm)
					*rpm = (short)( 60 / GetTrackPeriod(floppycontext,currentside) );
			}

			if( bmpexport )
			{
				sprintf(tmp_filename,"track%.2d.%d.bmp",track>>1,track&1);
				hxcfe_FxStream_ExportToBmp(fxs,track_dump, tmp_filename);
			}

			if(currentside)
			{
				currentside->stream_dump = track_dump;
			}
			else
			{
				hxcfe_FxStream_FreeStream(fxs,track_dump);
			}
		}
		else
		{
			floppycontext->hxc_printf(MSG_DEBUG,"hxcfe_FxStream_ImportStream failed !");
		}

		free(trackbuf_dword);

		hxcfe_deinitFxStream(fxs);
	}

	floppycontext->hxc_printf(MSG_DEBUG,"------------------------------------------------");

	return currentside;
}

int FDX_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	FILE * f;
	int ret;
	short rpm;
	int i,j,k,len;
	int tracknumber,sidenumber;
	HXCFE_CYLINDER* currentcylinder;
	HXCFE_SIDE* currentside;
	fdxheader_t fileheader;
	fdxtrack_t * fdxtrackheader;
	uint8_t * track_buffer;
	int phasecorrection;
	int bitrate;
	int filterpasses,filter;

	currentcylinder = 0;

	f = NULL;
	track_buffer = NULL;

	ret = HXCFE_NOERROR;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"FDX_libLoad_DiskFile %s",imgfile);

	phasecorrection = hxcfe_getEnvVarValue( imgldr_ctx->hxcfe, "FLUXSTREAM_PLL_PHASE_CORRECTION_DIVISOR" );
	filterpasses = hxcfe_getEnvVarValue( imgldr_ctx->hxcfe, "FLUXSTREAM_BITRATE_FILTER_PASSES" );
	filter = hxcfe_getEnvVarValue( imgldr_ctx->hxcfe, "FLUXSTREAM_BITRATE_FILTER_WINDOW" );
	bitrate = hxcfe_getEnvVarValue( imgldr_ctx->hxcfe, "FDXLOADER_BITRATE" );

	f = hxc_fopen(imgfile,"rb");
	if( f == NULL )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	memset( &fileheader,0,sizeof(fdxheader_t));
	if( hxc_fread( &fileheader, sizeof(fdxheader_t), f ) <= 0 )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Can't read the file header %s !",imgfile);
		fclose(f);
		return HXCFE_ACCESSERROR;
	}

	if(!memcmp((char*)&fileheader.fdx_signature,"FDX",3) && (fileheader.nb_of_cylinders && fileheader.nb_of_cylinders <= 86 && fileheader.nb_of_heads >= 1 && fileheader.nb_of_heads <= 2 && fileheader.track_block_size) )
	{
#ifdef FDX_DBG
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"FDX revision : %d",fileheader.revision);
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"FDX Disk name : %s",fileheader.disk_name);
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"FDX Disk type : 0x%X",fileheader.disk_type);
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"FDX Number of cylinders : %d",fileheader.nb_of_cylinders);
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"FDX Number of heads : %d",fileheader.nb_of_heads);
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"FDX Default bitrate : %d",fileheader.default_bitrate);
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"FDX Disk RPM : %d",fileheader.disk_rpm);
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"FDX Write Protect : %d",fileheader.write_protect);
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"FDX Options flags : 0x%.8X",fileheader.option);
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"FDX Track block size : 0x%.8X",fileheader.track_block_size);
#endif

		floppydisk->floppySectorPerTrack = -1;
		floppydisk->floppyNumberOfSide = fileheader.nb_of_heads;
		floppydisk->floppyNumberOfTrack = fileheader.nb_of_cylinders;
		floppydisk->floppyiftype = ATARIST_DD_FLOPPYMODE;
		floppydisk->floppyBitRate = (fileheader.default_bitrate / 2) * 1000;

		track_buffer = malloc(fileheader.track_block_size);
		if(!track_buffer)
		{
			ret = HXCFE_INTERNALERROR;
			goto alloc_error;
		}

		fdxtrackheader = (fdxtrack_t*)track_buffer;

		floppydisk->tracks = (HXCFE_CYLINDER**)malloc(sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);
		if(!floppydisk->tracks)
		{
			ret = HXCFE_INTERNALERROR;
			goto alloc_error;
		}

		memset(floppydisk->tracks,0,sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);

		for(i=0;i<floppydisk->floppyNumberOfTrack*floppydisk->floppyNumberOfSide;i++)
		{
			hxcfe_imgCallProgressCallback(imgldr_ctx,i,floppydisk->floppyNumberOfTrack*floppydisk->floppyNumberOfSide );

			if( hxc_fread( track_buffer, fileheader.track_block_size, f ) > 0 )
			{
				#ifdef FDX_DBG
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"FDX Cylinder: %d, Head: %d, Index position: %d, bit track length : %d",fdxtrackheader->cylinder,fdxtrackheader->head,fdxtrackheader->index_bit_place,fdxtrackheader->bit_track_length);
				#endif

				tracknumber = fdxtrackheader->cylinder;
				sidenumber = fdxtrackheader->head;

				if(!floppydisk->tracks[tracknumber])
				{
					floppydisk->tracks[tracknumber] = (HXCFE_CYLINDER*)malloc(sizeof(HXCFE_CYLINDER));
					if(!floppydisk->tracks[tracknumber])
					{
						ret = HXCFE_INTERNALERROR;
						goto alloc_error;
					}

					memset(floppydisk->tracks[tracknumber],0,sizeof(HXCFE_CYLINDER));

					floppydisk->tracks[tracknumber]->floppyRPM = fileheader.disk_rpm;
					currentcylinder = floppydisk->tracks[tracknumber];
					currentcylinder->number_of_side = 0;

					currentcylinder->sides = (HXCFE_SIDE**)malloc(sizeof(HXCFE_SIDE*)*2);
					if(!currentcylinder->sides)
					{
						ret = HXCFE_INTERNALERROR;
						goto alloc_error;
					}

					memset(currentcylinder->sides,0,sizeof(HXCFE_SIDE*)*2);
				}

				currentcylinder=floppydisk->tracks[tracknumber];

				switch(fileheader.disk_type)
				{
					case 9:
						len = 0;
						currentcylinder->sides[sidenumber] = decodestream(imgldr_ctx->hxcfe,track_buffer+sizeof(fdxtrack_t),fdxtrackheader->bit_track_length,i>>0,&rpm,1,phasecorrection,FDX_NB_FAKE_REV, bitrate, filter, filterpasses, 0, fileheader.default_bitrate);
						currentcylinder->number_of_side++;

						if(currentcylinder->sides[sidenumber])
							len = currentcylinder->sides[sidenumber]->tracklen;
					break;

					default:
						currentcylinder->sides[sidenumber] = tg_alloctrack(floppydisk->floppyBitRate,ISOIBM_MFM_ENCODING,fileheader.disk_rpm,fdxtrackheader->bit_track_length,3800,0,TG_ALLOCTRACK_ALLOCTIMIMGBUFFER|TG_ALLOCTRACK_ALLOCFLAKEYBUFFER);
						if(!currentcylinder->sides[sidenumber])
						{
							ret = HXCFE_INTERNALERROR;
							goto alloc_error;
						}

						currentside=currentcylinder->sides[sidenumber];
						currentcylinder->number_of_side++;

						len = fdxtrackheader->bit_track_length/8;
						if( fdxtrackheader->bit_track_length & 7 )
							len++;

						memset(currentside->databuffer,0x00,len);

						k = (fdxtrackheader->index_bit_place) % fdxtrackheader->bit_track_length;
						for(j=0;j<fdxtrackheader->bit_track_length;j++)
						{
							if( track_buffer[sizeof(fdxtrack_t) + (k>>3)] & (0x80>>(k&7)) )
							{
								currentside->databuffer[j>>3] |= (0x80>>(j&7));
							}

							k++;
							k %= fdxtrackheader->bit_track_length;
						}

						for(j=0;j<len;j++)
						{
							if(!currentside->databuffer[j])
							{
								currentside->databuffer[j] = 0x24;
								currentside->flakybitsbuffer[j] = 0xFF;
							}
						}

					break;
				}

			}
		}

		hxcfe_sanityCheck(imgldr_ctx->hxcfe,floppydisk);

	}

	hxc_fclose(f);

	free(track_buffer);

	return ret;

alloc_error:
	if(f)
		hxc_fclose(f);

	free(track_buffer);

	hxcfe_freeFloppy(imgldr_ctx->hxcfe, floppydisk );

	return ret;
}

int FDX_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]="FDX68_FDX";
	static const char plug_desc[]="FDX Loader";
	static const char plug_ext[]="fdx";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   FDX_libIsValidDiskFile,
		(LOADDISKFILE)      FDX_libLoad_DiskFile,
		(WRITEDISKFILE)     FDX_libWrite_DiskFile,
		(GETPLUGININFOS)    FDX_libGetPluginInfo
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

