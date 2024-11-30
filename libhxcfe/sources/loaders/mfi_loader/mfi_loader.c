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
// File : mfi_loader.c
// Contains: mfi floppy image loader
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

#include "mfi_loader.h"
#include "mfi_writer.h"

#include "mfi_format.h"

#include "libhxcadaptor.h"
#include "tracks/crc.h"

#include "thirdpartylibs/zlib/zlib.h"

#define DEFAULT_BITS_PERIOD 1000 // ps -> 1GHz

#define MFI_DBG 1

int MFI_libIsValidDiskFile( HXCFE_IMGLDR * imgldr_ctx, HXCFE_IMGLDR_FILEINFOS * imgfile )
{
	mfiheader_t * fileheader;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"MFI_libIsValidDiskFile");

	if(hxc_checkfileext(imgfile->path,"MFI",SYS_PATH_TYPE))
	{
		fileheader = (mfiheader_t*)imgfile->file_header;

		if( memcmp((char*)&fileheader->mfi_signature,"MESSFLOPPYIMAGE",16) && memcmp((char*)&fileheader->mfi_signature,"MAMEFLOPPYIMAGE",16) && ( fileheader->head_count <= 2 ) )
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"MFI_libIsValidDiskFile : non MFI file (bad header)!");
			return HXCFE_BADFILE;
		}
		else
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"MFI_libIsValidDiskFile : MFI file !");
			return HXCFE_VALIDFILE;
		}
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"MFI_libIsValidDiskFile : non MFI file !");
		return HXCFE_BADFILE;
	}
}

static HXCFE_SIDE* decodestream(HXCFE* floppycontext, int track_size,uint32_t * unpacked_data,int track,int side,short * rpm,float timecoef,int phasecorrection,unsigned int revolution ,int bitrate,int filter,int filterpasses, int bmpexport)
{
	HXCFE_SIDE* currentside;
	unsigned int i,p,offset;

	HXCFE_TRKSTREAM *track_dump;
	HXCFE_FXSA * fxs;

	uint32_t * trackbuf_dword;
	uint32_t index_position[16];
	uint32_t pulses_count;
	uint32_t r,cumul;
	char tmp_filename[512];
	int in_rand_zone;

	currentside = NULL;

	floppycontext->hxc_printf(MSG_DEBUG,"------------------------------------------------");

	floppycontext->hxc_printf(MSG_DEBUG,"Loading MFI track...");

	index_position[0] = 0;

	fxs = hxcfe_initFxStream(floppycontext);
	if(fxs)
	{
		hxcfe_FxStream_setBitrate(fxs,bitrate);

		hxcfe_FxStream_setPhaseCorrectionFactor(fxs,phasecorrection);

		hxcfe_FxStream_setFilterParameters(fxs,filterpasses,filter);

		pulses_count = track_size;

		if( pulses_count )
		{
			offset = 0;

			p = 0;

			trackbuf_dword = calloc(1, ((pulses_count*revolution)+1)*sizeof(uint32_t));
			if(trackbuf_dword)
			{
				r = 0;
				while( r < revolution )
				{
					cumul = 0;
					in_rand_zone = 0;

					for(i=0;i<pulses_count;i++)
					{
						uint32_t dat;

						dat = unpacked_data[i];

						switch( dat & 0xF0000000 )
						{
							case MG_F:
								if( !in_rand_zone )
									trackbuf_dword[p++] = (dat & 0xFFFFFFF);
								else
									cumul += (dat & 0xFFFFFFF);
							break;

							case MG_N:
							case MG_D:
								in_rand_zone = 1;
							break;

							case MG_E:
								in_rand_zone = 0;
								// TODO : randomize starting at p and ending p +  cumul
								trackbuf_dword[p++] = cumul + (dat & 0xFFFFFFF);
								cumul = 0;
							break;
						}
					}

					index_position[r] = p;

					r++;
				}

				hxcfe_FxStream_setResolution(fxs,DEFAULT_BITS_PERIOD);

				track_dump = hxcfe_FxStream_ImportStream(fxs,trackbuf_dword,32, p, HXCFE_STREAMCHANNEL_TYPE_RLEEVT, "data", NULL);
				if(track_dump)
				{
					offset = 0;
					hxcfe_FxStream_AddIndex(fxs,track_dump,offset,0,FXSTRM_INDEX_MAININDEX);

					for(i=0;i<revolution;i++)
					{
						hxcfe_FxStream_AddIndex(fxs,track_dump,index_position[i],0,FXSTRM_INDEX_MAININDEX);
					}

					hxcfe_FxStream_ChangeSpeed(fxs,track_dump,timecoef);

					fxs->pll.track = track;
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

				free(trackbuf_dword);
			}
		}

		hxcfe_deinitFxStream(fxs);
	}

	floppycontext->hxc_printf(MSG_DEBUG,"------------------------------------------------");

	return currentside;
}

int MFI_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	FILE * f = NULL;
	int ret;
	short rpm;
	int i;
	int tracknumber,sidenumber;
	HXCFE_CYLINDER* currentcylinder;
	HXCFE_SIDE * curside;
	mfiheader_t fileheader;
	mfitrack_t * track_header_array = NULL;
	int resolution;
	unsigned char * packed_data_buf = NULL;
	uint32_t * unpacked_data_buf = NULL;

	float timecoef;
	int phasecorrection;
	int bitrate;
	int filterpasses,filter;
	int bmp_export;

	currentcylinder = 0;
	bmp_export = 0;

	f = NULL;

	ret = HXCFE_NOERROR;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"MFI_libLoad_DiskFile %s",imgfile);

	phasecorrection = hxcfe_getEnvVarValue( imgldr_ctx->hxcfe, "FLUXSTREAM_PLL_PHASE_CORRECTION_DIVISOR" );
	filterpasses = hxcfe_getEnvVarValue( imgldr_ctx->hxcfe, "FLUXSTREAM_BITRATE_FILTER_PASSES" );
	filter = hxcfe_getEnvVarValue( imgldr_ctx->hxcfe, "FLUXSTREAM_BITRATE_FILTER_WINDOW" );
	bitrate = hxcfe_getEnvVarValue( imgldr_ctx->hxcfe, "STREAMHFELOADER_BITRATE" );

	timecoef=1;
	if( !strcmp(hxcfe_getEnvVar( imgldr_ctx->hxcfe, "FLUXSTREAM_RPMFIX", NULL),"360TO300RPM") )
	{
		timecoef=(float)1.2;
	}

	if( !strcmp(hxcfe_getEnvVar( imgldr_ctx->hxcfe, "FLUXSTREAM_RPMFIX", NULL),"300TO360RPM") )
	{
		timecoef=(float)0.833;
	}

	f = hxc_fopen(imgfile,"rb");
	if( f == NULL )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	memset( &fileheader,0,sizeof(mfiheader_t));
	if( hxc_fread( &fileheader, sizeof(mfiheader_t), f ) <= 0 )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Can't read the file header %s !",imgfile);
		fclose(f);
		return HXCFE_ACCESSERROR;
	}

	if( !memcmp((char*)&fileheader.mfi_signature,"MESSFLOPPYIMAGE",16) || !memcmp((char*)&fileheader.mfi_signature,"MAMEFLOPPYIMAGE",16) )
	{

#ifdef MFI_DBG
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"MFI revision : %s",(char*)&fileheader.mfi_signature);
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"MFI form factor : 0x%.8X",(char*)&fileheader.form_factor);
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"MFI variants : 0x%.8X",(char*)&fileheader.variant);
#endif

		floppydisk->floppySectorPerTrack = -1;
		floppydisk->floppyiftype = GENERIC_SHUGART_DD_FLOPPYMODE;
		floppydisk->floppyBitRate = 250 * 1000;

		resolution = fileheader.cyl_count >> RESOLUTION_SHIFT;
		floppydisk->floppyNumberOfSide = fileheader.head_count;
		floppydisk->floppyNumberOfTrack = (fileheader.cyl_count & CYLINDER_MASK);
		rpm = 300;

#ifdef MFI_DBG
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"--- Header ---");
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Resolution : %d",resolution);
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Number Of Side  : %d",floppydisk->floppyNumberOfSide);
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Number Of Track : %d",floppydisk->floppyNumberOfTrack);
#endif

		track_header_array = (mfitrack_t*)calloc(1, sizeof(mfitrack_t)*floppydisk->floppyNumberOfTrack * floppydisk->floppyNumberOfSide);
		if(!track_header_array)
		{
			ret = HXCFE_INTERNALERROR;
			goto error;
		}

		if( hxc_fread( track_header_array, sizeof(mfitrack_t)*floppydisk->floppyNumberOfTrack * floppydisk->floppyNumberOfSide, f ) <= 0 )
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Can't read the file header %s !",imgfile);
			ret = HXCFE_ACCESSERROR;
			goto error;
		}

		floppydisk->tracks = (HXCFE_CYLINDER**)malloc(sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);
		if(!floppydisk->tracks)
		{
			ret = HXCFE_INTERNALERROR;
			goto error;
		}

		packed_data_buf = NULL;
		unpacked_data_buf = NULL;

		memset(floppydisk->tracks,0,sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);

		for(i=0;i<floppydisk->floppyNumberOfTrack*floppydisk->floppyNumberOfSide;i++)
		{

			hxcfe_imgCallProgressCallback(imgldr_ctx,i,floppydisk->floppyNumberOfTrack*floppydisk->floppyNumberOfSide );

#ifdef MFI_DBG
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"--- Entry %d ---",i);
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Offset : 0x%X",track_header_array[i].offset);
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Compressed_size : 0x%X",track_header_array[i].compressed_size);
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Uncompressed_size : 0x%X",track_header_array[i].uncompressed_size);
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Write_splice : 0x%X",track_header_array[i].write_splice);
#endif

			if ( floppydisk->floppyNumberOfSide == 1 )
			{
				tracknumber = i;
				sidenumber = 0;
			}
			else
			{
				tracknumber = i>>1;
				sidenumber = i&1;
			}

			if(track_header_array[i].compressed_size && track_header_array[i].compressed_size)
			{
				long unsigned int destLen;

				packed_data_buf = calloc(1,track_header_array[i].compressed_size + 16);
				unpacked_data_buf = calloc(1,track_header_array[i].uncompressed_size + 16);
				if( packed_data_buf && unpacked_data_buf )
				{
					destLen = track_header_array[i].uncompressed_size;

					fseek(f,track_header_array[i].offset,SEEK_SET);
					hxc_fread(packed_data_buf,track_header_array[i].compressed_size,f);

					uncompress((unsigned char *)unpacked_data_buf, &destLen, packed_data_buf, track_header_array[i].compressed_size);

					if(destLen)
					{
						curside = decodestream(imgldr_ctx->hxcfe, destLen / sizeof(uint32_t),unpacked_data_buf,tracknumber,sidenumber,&rpm,timecoef,phasecorrection,3,bitrate,filter,filterpasses,bmp_export);

						if(!floppydisk->tracks[tracknumber])
						{
							floppydisk->tracks[tracknumber] = allocCylinderEntry(rpm,floppydisk->floppyNumberOfSide);
						}

						currentcylinder = floppydisk->tracks[tracknumber];

						currentcylinder->sides[sidenumber] = curside;
					}
					else
					{
						imgldr_ctx->hxcfe->hxc_printf(MSG_WARNING,"MFI : Track Entry %d : Unpack failure",i);
					}
				}
				else
				{
					imgldr_ctx->hxcfe->hxc_printf(MSG_WARNING,"MFI : Track allocation failure",i);
				}

				free(packed_data_buf);
				packed_data_buf = NULL;

				free(unpacked_data_buf);
				unpacked_data_buf = NULL;
			}
			else
			{
				imgldr_ctx->hxcfe->hxc_printf(MSG_WARNING,"MFI : Track Entry %d : Unformatted",i);
				curside = tg_alloctrack(floppydisk->floppyBitRate,ISOIBM_MFM_ENCODING,rpm,((250000/(rpm/60))/4)*8,2500,-2500,TG_ALLOCTRACK_ALLOCFLAKEYBUFFER|TG_ALLOCTRACK_RANDOMIZEDATABUFFER|TG_ALLOCTRACK_UNFORMATEDBUFFER);

				if(!floppydisk->tracks[tracknumber])
				{
					floppydisk->tracks[tracknumber] = allocCylinderEntry(rpm,floppydisk->floppyNumberOfSide);
				}

				currentcylinder = floppydisk->tracks[tracknumber];

				currentcylinder->sides[sidenumber] = curside;
			}
		}

		hxcfe_sanityCheck(imgldr_ctx->hxcfe,floppydisk);
	}

	hxc_fclose(f);

	free(track_header_array);

	return ret;

error:
	if(f)
		hxc_fclose(f);

	free(track_header_array);
	free(packed_data_buf);
	free(unpacked_data_buf);

	return ret;
}

int MFI_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]="MAME_MFI";
	static const char plug_desc[]="Mame MFI Loader";
	static const char plug_ext[]="mfi";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   MFI_libIsValidDiskFile,
		(LOADDISKFILE)      MFI_libLoad_DiskFile,
		(WRITEDISKFILE)     0,//MFI_libWrite_DiskFile,
		(GETPLUGININFOS)    MFI_libGetPluginInfo
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
