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
// File : a2r_loader.c
// Contains: AppleSauce A2R Stream floppy image loader
//
// Written by: Jean-François DEL NERO
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

#include <math.h>

#include "types.h"

#include "internal_libhxcfe.h"
#include "libhxcfe.h"

#include "floppy_loader.h"
#include "floppy_utils.h"

#include "a2r_loader.h"

#include "a2r_format.h"

#include "libhxcadaptor.h"

#include "misc/env.h"
#include "misc/script_exec.h"

//#define A2RDEBUG 1

int A2R_libIsValidDiskFile( HXCFE_IMGLDR * imgldr_ctx, HXCFE_IMGLDR_FILEINFOS * imgfile )
{
	a2r_header * a2rh;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"A2R_libIsValidDiskFile");

	if( hxc_checkfileext(imgfile->path,"a2r",SYS_PATH_TYPE) )
	{
		a2rh = (a2r_header *)&imgfile->file_header;

		if( strncmp((char*)&a2rh->sign,"A2R2",4) || (a2rh->ff_byte != 0xFF) )
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"A2R_libIsValidDiskFile : Bad file header !");
			return HXCFE_BADFILE;
		}

		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"A2R_libIsValidDiskFile : A2R file !");
		return HXCFE_VALIDFILE;
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"A2R_libIsValidDiskFile : non A2R file !");
		return HXCFE_BADFILE;
	}
}


static HXCFE_SIDE* import_a2r_stream(HXCFE* floppycontext, a2r_capture * captureinfo, unsigned char * stream, int size, short * rpm, float timecoef, int bitrate, int phasecorrection, int filter, int filterpasses,int bmpexport )
{
	uint32_t * tmp_stream;
	uint32_t cumul,offset_in,offset_out;
	HXCFE_FXSA * fxs;
	HXCFE_SIDE* currentside;
	HXCFE_TRKSTREAM *track_dump;
	int index_stream_pos;
	int total;
	uint8_t cur_value;
	char tmp_filename[512];

	currentside = NULL;

	tmp_stream = malloc(size * sizeof(uint32_t));
	if(tmp_stream)
	{
		memset(tmp_stream,0,size * sizeof(uint32_t));

		total = 0;
		cumul = 0;
		offset_in = 0;
		offset_out = 0;
		index_stream_pos = 0;

		for(offset_in = 0; offset_in < size ; offset_in++)
		{
			cur_value = stream[offset_in];

			cumul += cur_value;
			total += cur_value;

			if( cur_value != 0xFF )
			{
				tmp_stream[offset_out++] = cumul;
				cumul = 0;

				if( total < captureinfo->estimated_loop_point )
				{
					index_stream_pos = offset_out;
				}
			}
		}

		fxs = hxcfe_initFxStream(floppycontext);
		if(fxs)
		{
		//	hxcfe_FxStream_setBitrate(fxs,bitrate);

		//	hxcfe_FxStream_setPhaseCorrectionFactor(fxs,phasecorrection);

		//	hxcfe_FxStream_setFilterParameters(fxs,filterpasses,filter);

			hxcfe_FxStream_setResolution(fxs,125000); // 125 ns per tick

			track_dump = hxcfe_FxStream_ImportStream(fxs,tmp_stream,32,(offset_out),HXCFE_STREAMCHANNEL_TYPE_RLEEVT, "data", NULL);
			if(track_dump)
			{
				hxcfe_FxStream_ChangeSpeed(fxs,track_dump,timecoef);

				hxcfe_FxStream_AddIndex(fxs,track_dump,0,0,FXSTRM_INDEX_MAININDEX);
				hxcfe_FxStream_AddIndex(fxs,track_dump,index_stream_pos,0,FXSTRM_INDEX_MAININDEX);

				currentside = hxcfe_FxStream_AnalyzeAndGetTrack(fxs,track_dump);

				if(currentside)
				{
					if(rpm)
						*rpm = (short)( 60 / GetTrackPeriod(floppycontext,currentside) );
				}
				else
				{
					floppycontext->hxc_printf(MSG_ERROR,"import_a2r_stream : null track !");
				}

				if( bmpexport )
				{
					sprintf(tmp_filename,"track%.2d.%d.bmp",captureinfo->location>>1,captureinfo->location&1);
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

			hxcfe_deinitFxStream(fxs);
		}

		free(tmp_stream);
		tmp_stream = NULL;
	}

	return currentside;
}

int A2R_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	FILE * f;
	int mintrack,maxtrack;
	int minside,maxside;
	short rpm;
	unsigned short i,j;
	int k,l,len;
	int singleside;
	HXCFE_CYLINDER* currentcylinder;
	HXCFE_SIDE * curside;
	int nbside;
	float timecoef;
	int tracklen;
	char * folder;
	char * filepath;
	int previous_bit;
	int phasecorrection;
	int filterpasses,filter;
	int bitrate,bmp_export;
	//int mac_clv;
	int foffset,filesize;
	int stream_start_pos;
	int max_location;
	int skip_inter_tracks;
	int track_pos;

	a2r_header  a2rh;
	a2r_info    info;
	a2r_capture capture;
	int str_offset;
	envvar_entry * backup_env;
	envvar_entry * tmp_env;
	int last_location;

	a2r_chunk_header a2r_chunkh;
	unsigned char *tmp_buffer;
	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"A2R_libLoad_DiskFile");

	backup_env = NULL;

	rpm = 300;

	if(imgfile)
	{
		mintrack=0;
		maxtrack=0;
		minside=0;
		maxside=0;

		filesize = hxc_getfilesize(imgfile);

		if(filesize < 8)
			return HXCFE_BADFILE;

		f = hxc_fopen(imgfile,"rb");
		if(f)
		{
			nbside = 2;
/*
			if( hxcfe_getEnvVarValue( imgldr_ctx->hxcfe, "FLUXSTREAM_IMPORT_PCCAV_TO_MACCLV" ) & 1 )
				mac_clv = 1;
			else
				mac_clv = 0;
*/
			skip_inter_tracks = hxcfe_getEnvVarValue( imgldr_ctx->hxcfe, "A2RLOADER_SKIP_INTER_TRACKS" );

			singleside = hxcfe_getEnvVarValue( imgldr_ctx->hxcfe, "A2RLOADER_SINGLE_SIDE" )&1;
			phasecorrection = hxcfe_getEnvVarValue( imgldr_ctx->hxcfe, "FLUXSTREAM_PLL_PHASE_CORRECTION_DIVISOR" );
			filterpasses = hxcfe_getEnvVarValue( imgldr_ctx->hxcfe, "FLUXSTREAM_BITRATE_FILTER_PASSES" );
			filter = hxcfe_getEnvVarValue( imgldr_ctx->hxcfe, "FLUXSTREAM_BITRATE_FILTER_WINDOW" );
			bitrate = hxcfe_getEnvVarValue( imgldr_ctx->hxcfe, "A2RLOADER_BITRATE" );
			bmp_export = hxcfe_getEnvVarValue( imgldr_ctx->hxcfe, "A2RLOADER_BMPEXPORT" );

			timecoef = 1;
			if( !strcmp(hxcfe_getEnvVar( imgldr_ctx->hxcfe, "FLUXSTREAM_RPMFIX", NULL),"360TO300RPM") )
			{
				timecoef=(float)1.2;
			}

			if( !strcmp(hxcfe_getEnvVar( imgldr_ctx->hxcfe, "FLUXSTREAM_RPMFIX", NULL),"300TO360RPM") )
			{
				timecoef=(float)0.833;
			}

			hxc_fread(&a2rh, sizeof(a2r_header), f);

			if( strncmp((char*)&a2rh.sign,"A2R2",4) || (a2rh.ff_byte != 0xFF) )
			{
				hxc_fclose(f);
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"A2R_libLoad_DiskFile : bad Header !");
				return HXCFE_BADFILE;
			}

			tmp_env = initEnv( (envvar_entry *)imgldr_ctx->hxcfe->envvar, NULL );
			if(!tmp_env)
			{
				hxc_fclose(f);
				return HXCFE_INTERNALERROR;
			}

			backup_env = imgldr_ctx->hxcfe->envvar;
			imgldr_ctx->hxcfe->envvar = tmp_env;

			len=hxc_getpathfolder(imgfile,0,SYS_PATH_TYPE);
			folder = (char*)malloc(len+1);
			if( !folder )
			{
				hxc_fclose(f);

				tmp_env = (envvar_entry *)imgldr_ctx->hxcfe->envvar;
				imgldr_ctx->hxcfe->envvar = backup_env;
				deinitEnv( tmp_env );

				return HXCFE_INTERNALERROR;
			}

			hxc_getpathfolder(imgfile,folder,SYS_PATH_TYPE);

			filepath = malloc( strlen(imgfile) + 32 );
			if(filepath)
			{
				sprintf(filepath,"%s%s",folder,"config.script");
				hxcfe_execScriptFile(imgldr_ctx->hxcfe, filepath);
				free(filepath);
				filepath = NULL;
			}

			imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"Loading A2R file...");

			foffset = ftell(f);

			while( foffset < filesize )
			{
				memset(&a2r_chunkh,0,sizeof(a2r_chunk_header));
				hxc_fread(&a2r_chunkh, sizeof(a2r_chunk_header), f);

				if(!strncmp((char*)&a2r_chunkh.sign,"STRM",4))
				{
					imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"A2R STRM CHUNK - %d bytes",a2r_chunkh.chunk_size);
					last_location = -1;

					stream_start_pos = ftell(f);

					mintrack=0;
					maxtrack=0;
					minside=0;
					maxside=0;

					max_location = 0;
					str_offset = 0;
					while( str_offset < a2r_chunkh.chunk_size - 1)
					{
						hxc_fread(&capture, sizeof(a2r_capture), f);
						fseek(f,capture.data_length,SEEK_CUR);

						if(max_location < capture.location)
							max_location = capture.location;

						str_offset += sizeof(a2r_capture);
						str_offset += capture.data_length;
					}

					fseek(f,stream_start_pos,SEEK_SET);

					maxside = nbside;
					maxtrack = max_location / nbside;

					imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"%d track (%d - %d), %d sides (%d - %d)",( maxtrack - mintrack ) + 1,mintrack,maxtrack,maxside - minside,minside,maxside);

					floppydisk->floppyiftype = GENERIC_SHUGART_DD_FLOPPYMODE;
					floppydisk->floppyBitRate = VARIABLEBITRATE;
					floppydisk->floppyNumberOfTrack = maxtrack + 1;
					if(skip_inter_tracks)
						floppydisk->floppyNumberOfTrack /= 4;

					if(singleside)
						floppydisk->floppyNumberOfSide = 1;
					else
						floppydisk->floppyNumberOfSide = nbside;

					floppydisk->floppySectorPerTrack = -1;

					floppydisk->tracks=(HXCFE_CYLINDER**)calloc( 1, sizeof(HXCFE_CYLINDER*)*(floppydisk->floppyNumberOfTrack+1) );
					if(!floppydisk->tracks)
					{
						hxc_fclose(f);

						tmp_env = (envvar_entry *)imgldr_ctx->hxcfe->envvar;
						imgldr_ctx->hxcfe->envvar = backup_env;
						deinitEnv( tmp_env );

						return HXCFE_INTERNALERROR;
					}

					str_offset = 0;
					while( str_offset < a2r_chunkh.chunk_size - 1)
					{
						hxc_fread(&capture, sizeof(a2r_capture), f);

						rpm = 300;

						imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"A2R STRM CAPTURE : New Capture block");
						imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"A2R STRM CAPTURE : location             : %d", capture.location);
						imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"A2R STRM CAPTURE : capture_type         : %d", capture.capture_type);
						imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"A2R STRM CAPTURE : data_length          : %d", capture.data_length);
						imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"A2R STRM CAPTURE : estimated_loop_point : %d", capture.estimated_loop_point);

						tmp_buffer = malloc(capture.data_length + 1);
						if(tmp_buffer)
						{
							curside = NULL;

							memset(tmp_buffer,0,capture.data_length + 1);
							hxc_fread(tmp_buffer, capture.data_length, f);

							if( (last_location != capture.location) && (!skip_inter_tracks || !(capture.location&3) ) )
							{
								curside = import_a2r_stream(imgldr_ctx->hxcfe, &capture, tmp_buffer, capture.data_length, &rpm, timecoef, bitrate, phasecorrection, filter, filterpasses, bmp_export );
								last_location = capture.location;

								if(curside)
								{
									tracklen = curside->tracklen /8;
									if(curside->tracklen & 7)
										tracklen++;

									// Remove sticked data bits ...
									previous_bit = 0;
									for(k=0;k<tracklen;k++)
									{
										if(previous_bit)
										{
											if(curside->databuffer[k] & 0x80)
											{
												//curside->databuffer[k-1] = curside->databuffer[k-1] ^ 0x01;
												//curside->flakybitsbuffer[k-1] =  curside->flakybitsbuffer[k-1] | (0x01);
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
								}

								if(capture.location <= max_location)
								{
									track_pos = capture.location >> (nbside-1);
									if(skip_inter_tracks)
										track_pos /= 4;

									if(!floppydisk->tracks[track_pos])
									{
										floppydisk->tracks[track_pos] = allocCylinderEntry(rpm,floppydisk->floppyNumberOfSide);
									}

									currentcylinder=floppydisk->tracks[track_pos];
									if(nbside == 2)
										currentcylinder->sides[ capture.location & 1 ] = curside;
									else
										currentcylinder->sides[ 0 ] = curside;

								}

								hxcfe_imgCallProgressCallback(imgldr_ctx,capture.location,max_location );
							}

							free(tmp_buffer);
							tmp_buffer = NULL;
						}

						str_offset += sizeof(a2r_capture);
						str_offset += capture.data_length;
					}
				}
				else
				{
					if(!strncmp((char*)&a2r_chunkh.sign,"META",4))
					{
						imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"A2R META CHUNK - %d bytes",a2r_chunkh.chunk_size);
						tmp_buffer = malloc(a2r_chunkh.chunk_size + 1);
						if(tmp_buffer)
						{
							memset(tmp_buffer,0,a2r_chunkh.chunk_size + 1);
							hxc_fread(tmp_buffer, a2r_chunkh.chunk_size, f);
							imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"A2R META :\n%s", tmp_buffer);
							free(tmp_buffer);
							tmp_buffer = NULL;
						}
					}
					else
					{
						if(!strncmp((char*)&a2r_chunkh.sign,"INFO",4))
						{
							imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"A2R INFO CHUNK - %d bytes",a2r_chunkh.chunk_size);
							hxc_fread(&info, sizeof(a2r_info), f);

							if( info.disk_type == 2 )
							{
								skip_inter_tracks = 0;
								nbside = 2;
							}
							else
								nbside = 1;

							switch(info.version)
							{
								case 1:
									imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"A2R INFO : Version         : %d", info.version);
									imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"A2R INFO : Creator         : %s", info.creator);
									imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"A2R INFO : disk_type       : %d", info.disk_type);
									imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"A2R INFO : write_protected : %d", info.write_protected);
									imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"A2R INFO : synchronized    : %d", info.synchronized);
								break;

								default:
									imgldr_ctx->hxcfe->hxc_printf(MSG_WARNING,"UNKNOWN INFO Version : %d !", info.version);
								break;
							}
						}
						else
						{
							imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"A2R UNKNOWN CHUNK !");
							break;
						}
					}
				}

				fseek(f,foffset + sizeof(a2r_chunk_header) + a2r_chunkh.chunk_size, SEEK_SET);

				foffset = ftell(f);

				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Scan offset : %d",foffset);
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

			tmp_env = (envvar_entry *)imgldr_ctx->hxcfe->envvar;
			imgldr_ctx->hxcfe->envvar = backup_env;
			deinitEnv( tmp_env );

			free(folder);
			folder = NULL;

			return HXCFE_NOERROR;
		}
	}

	return HXCFE_BADFILE;
}

int A2R_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]="A2R_FLUX_STREAM";
	static const char plug_desc[]="A2R Stream Loader";
	static const char plug_ext[]="a2r";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   A2R_libIsValidDiskFile,
		(LOADDISKFILE)      A2R_libLoad_DiskFile,
		(WRITEDISKFILE)     NULL,
		(GETPLUGININFOS)    A2R_libGetPluginInfo
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
