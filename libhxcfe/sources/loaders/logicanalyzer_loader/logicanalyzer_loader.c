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
// File : logicanalyzer_loader.c
// Contains: Logic analyzer dump file loader
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

#include "logicanalyzer_loader.h"
#include "logicanalyzer_format.h"

#include "libhxcadaptor.h"

#include "misc/env.h"
#include "misc/script_exec.h"

int logicanalyzer_libIsValidDiskFile( HXCFE_IMGLDR * imgldr_ctx, HXCFE_IMGLDR_FILEINFOS * imgfile )
{
	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"logicanalyzer_libIsValidDiskFile");

	if(hxc_checkfileext(imgfile->path,"logicbin8bits",SYS_PATH_TYPE))
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"logicanalyzer_libIsValidDiskFile : logic analyzer file !");
		return HXCFE_VALIDFILE;
	}

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"logicanalyzer_libIsValidDiskFile : not a logic analyzer file !");
	return HXCFE_BADFILE;
}

#define TMP_BUFFER_SIZE (16*1024)

typedef struct la_stats_
{
	int dat_pos;
	int idx_pos;

	int dat_pulse_cnt;
	int index_cnt;
	int last_dat_pos;
	int first_idx_pos;
	int last_idx_pos;
	int sample_rate;

	int start_offset;
	int end_offset;

	uint32_t * stream;
	uint32_t * index_array;
} la_stats;

int binlogicfile2stream(HXCFE_IMGLDR * imgldr_ctx,char * file, la_stats * la)
{
	FILE * f;
	int size,offset,total_size,block_size,i;
	uint8_t data_buffer[TMP_BUFFER_SIZE];
	uint8_t old_idx_state,old_dat_state;
	int rev_dat_pulses,rev_cnt;
	uint32_t delta;

	old_idx_state = 0x00;
	old_dat_state = 0x00;
	la->dat_pulse_cnt = 0;
	la->last_dat_pos = 0;
	la->first_idx_pos = -1;
	la->last_idx_pos = -1;
	la->index_cnt = 0;

	rev_dat_pulses = -1;
	rev_cnt = 0;
	delta = 0;

	f = fopen(file,"r");
	if(f)
	{
		fseek(f,0,SEEK_END);
		size = ftell(f);
		fseek(f,0,SEEK_SET);

		if(la->end_offset>0)
		{
			if(size > la->end_offset)
			{
				size = la->end_offset + 1;
			}
			else
			{
				la->end_offset = size - 1;
			}
		}

		if(la->start_offset>0)
		{
			size = size - la->start_offset;

			fseek(f,la->start_offset,SEEK_SET);
		}

		if (size < 0)
			size = 0;

		total_size = size;
		offset = 0;
		while( offset < total_size )
		{
			if(size >= TMP_BUFFER_SIZE)
			{
				if ( fread(&data_buffer,TMP_BUFFER_SIZE,1,f) != 1 )
				{
					fclose(f);
					return -1;
				}

				block_size = TMP_BUFFER_SIZE;
			}
			else
			{
				if ( fread(&data_buffer,size,1,f) != 1 )
				{
					fclose(f);
					return -1;
				}

				block_size = size;
			}

			size -= block_size;

			for(i=0;i<block_size;i++)
			{
				if( old_idx_state && !(data_buffer[i] & (0x1<<la->idx_pos)) )
				{
					if(la->dat_pulse_cnt > 100)
					{
						if(la->index_array)
						{
							la->index_array[la->index_cnt] = la->dat_pulse_cnt;
						}

						la->index_cnt++;

						la->last_idx_pos = offset + i;
					}

					if(la->first_idx_pos < 0)
					{
						la->first_idx_pos = offset + i;
					}

					if(rev_dat_pulses != -1)
					{
						imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"binlogicfile2stream : rev %d : %d data pulses",rev_cnt,rev_dat_pulses);
					}

					rev_dat_pulses = 0;
					rev_cnt++;
				}

				if( old_dat_state && !(data_buffer[i] & (0x1<<la->dat_pos)) )
				{
					if(la->stream)
					{
						la->stream[la->dat_pulse_cnt] = delta;
						//printf("%d : %d \n",la->dat_pulse_cnt,delta);
						delta = 0;
					}
					la->dat_pulse_cnt++;
					la->last_dat_pos = offset + i;

					if(rev_dat_pulses !=-1)
						rev_dat_pulses++;
				}

				old_idx_state = (data_buffer[i] & (0x1<<la->idx_pos));
				old_dat_state = (data_buffer[i] & (0x1<<la->dat_pos));

				delta++;
			}

			offset += block_size;
		}

		fclose(f);
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"binlogicfile2stream : Can't open %s !", file);
	}

	return 0;
}

static HXCFE_SIDE* decodestream(HXCFE* floppycontext,uint32_t * trackbuf_dword,int pulses_cnt,uint32_t * indexarray, int nbindex,int track,short * rpm,float timecoef,int phasecorrection, int bitrate,int filter,int filterpasses, int bmpexport, int lafreq)
{
	HXCFE_SIDE* currentside;
	HXCFE_TRKSTREAM *track_dump;
	HXCFE_FXSA * fxs;
	char tmp_filename[512];
	int tick_period;
	int i;

	currentside=0;

	floppycontext->hxc_printf(MSG_DEBUG,"------------------------------------------------");

	tick_period = ((double)(1*1E12)/(double)lafreq);

	floppycontext->hxc_printf(MSG_INFO_1,"Sample tick period : %d ps", tick_period);

	fxs = hxcfe_initFxStream(floppycontext);
	if(fxs)
	{
		hxcfe_FxStream_setBitrate(fxs,bitrate);

		hxcfe_FxStream_setPhaseCorrectionFactor(fxs,phasecorrection);

		hxcfe_FxStream_setFilterParameters(fxs,filterpasses,filter);

		#ifdef LA_DBG
		floppycontext->hxc_printf(MSG_DEBUG,"LA RAW Track pulses count : %d",pulses_cnt);
		#endif

		hxcfe_FxStream_setResolution(fxs, tick_period);

		track_dump = hxcfe_FxStream_ImportStream(fxs,trackbuf_dword,32,(pulses_cnt), HXCFE_STREAMCHANNEL_TYPE_RLEEVT, "data", NULL);
		if(track_dump)
		{
			for(i=0;i<nbindex;i++)
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

		hxcfe_deinitFxStream(fxs);

	}

	floppycontext->hxc_printf(MSG_DEBUG,"------------------------------------------------");

	return currentside;
}

int logicanalyzer_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	HXCFE_CYLINDER* currentcylinder;
	struct stat staterep;
	HXCFE_SIDE * curside;
	short rpm;
	int mintrack,maxtrack;
	int minside,maxside,singleside;
	int nbtrack,nbside;
	int found,track,side;
	char fname[512];
	char * folder;
	int len,trackstep,singlefile;
	char * filepath;
	FILE * f;
	int i,j;

	envvar_entry * backup_env;
	envvar_entry * tmp_env;
	la_stats la;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"logicanalyzer_libLoad_DiskFile");

	folder = NULL;
	backup_env = NULL;
	filepath = NULL;
	singleside = 0;
	singlefile = 1;

	if(imgfile)
	{
		if(!hxc_stat(imgfile,&staterep))
		{
			tmp_env = initEnv( (envvar_entry *)imgldr_ctx->hxcfe->envvar, NULL );
			if(!tmp_env)
				goto alloc_error;

			backup_env = imgldr_ctx->hxcfe->envvar;
			imgldr_ctx->hxcfe->envvar = tmp_env;

			len = hxc_getpathfolder(imgfile,0,SYS_PATH_TYPE);

			folder = (char*)malloc(len+1);
			if(!folder)
				goto alloc_error;

			hxc_getpathfolder(imgfile,folder,SYS_PATH_TYPE);

			if(staterep.st_mode&S_IFDIR)
			{
				sprintf(fname,"track");
			}
			else
			{
				hxc_getfilenamebase(imgfile,(char*)&fname,SYS_PATH_TYPE);
				if(!strstr(fname,".0.logicbin8bits") && !strstr(fname,".1.logicbin8bits") )
				{
					if(!strstr(fname,".logicbin8bits"))
					{
						tmp_env = (envvar_entry *)imgldr_ctx->hxcfe->envvar;
						imgldr_ctx->hxcfe->envvar = backup_env;
						deinitEnv( tmp_env );

						free(folder);
						return HXCFE_BADFILE;
					}
					else
					{
						singlefile = 1;
					}
				}
				else
				{
					singlefile = 0;
					fname[strlen(fname)-18]=0;
				}

			}

			filepath = malloc( strlen(imgfile) + 32 );
			if(!filepath)
				goto alloc_error;

			sprintf(filepath,"%s%s",folder,"config.script");
			hxcfe_execScriptFile(imgldr_ctx->hxcfe, filepath);

			memset(&la,0,sizeof(la_stats));

			la.idx_pos = -1;
			if( hxcfe_getEnvVar( imgldr_ctx->hxcfe, "LOGICANALYZER_INDEX_BIT", NULL) )
				la.idx_pos = hxcfe_getEnvVarValue( imgldr_ctx->hxcfe, "LOGICANALYZER_INDEX_BIT" );

			la.dat_pos = -1;
			if( hxcfe_getEnvVar( imgldr_ctx->hxcfe, "LOGICANALYZER_DATA_BIT", NULL) )
				la.dat_pos = hxcfe_getEnvVarValue( imgldr_ctx->hxcfe, "LOGICANALYZER_DATA_BIT" );

			la.sample_rate = -1;
			if( hxcfe_getEnvVar( imgldr_ctx->hxcfe, "LOGICANALYZER_SAMPLERATE", NULL) )
				la.sample_rate = hxcfe_getEnvVarValue( imgldr_ctx->hxcfe, "LOGICANALYZER_SAMPLERATE" );

			la.start_offset = -1;
			if( hxcfe_getEnvVar( imgldr_ctx->hxcfe, "LOGICANALYZER_IMPORT_START_OFFSET", NULL) )
				la.start_offset = hxcfe_getEnvVarValue( imgldr_ctx->hxcfe, "LOGICANALYZER_IMPORT_START_OFFSET" );

			la.end_offset = -1;
			if( hxcfe_getEnvVar( imgldr_ctx->hxcfe, "LOGICANALYZER_IMPORT_END_OFFSET", NULL) )
				la.end_offset = hxcfe_getEnvVarValue( imgldr_ctx->hxcfe, "LOGICANALYZER_IMPORT_END_OFFSET" );

			if( hxcfe_getEnvVarValue( imgldr_ctx->hxcfe, "LOGICANALYZER_DOUBLE_STEP" ) & 1 )
				trackstep = 2;
			else
				trackstep = 1;

			imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"logicanalyzer_libLoad_DiskFile : Index bit = %d",la.idx_pos);
			imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"logicanalyzer_libLoad_DiskFile : Data bit = %d",la.dat_pos);
			imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"logicanalyzer_libLoad_DiskFile : Sample rate = %d Hz",la.sample_rate);
			imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"logicanalyzer_libLoad_DiskFile : Start offset = %d",la.start_offset);
			imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"logicanalyzer_libLoad_DiskFile : End offset = %d",la.end_offset);

			track = 0;
			side = 0;
			mintrack=0;
			maxtrack=0;
			minside=0;
			maxside=0;
			found = 0;

			if(!singlefile)
			{
				do
				{
					sprintf(filepath,"%s%s%.2d.%d.logicbin8bits",folder,fname,track,side);
					f = hxc_fopen(filepath,"rb");
					if(f)
					{
						if(mintrack>track)
							mintrack = track;

						if(maxtrack<track)
							maxtrack = track;

						if(minside>side)
							minside = side;

						if(maxside<side)
							maxside = side;

						found=1;

						hxc_fclose(f);
					}
					side++;
					if(side>1)
					{
						side = 0;
						track=track+trackstep;
					}
				}while(track<84);
			}
			else
			{
				found = 1;
				minside = 0;
				maxside = 0;
				mintrack = 0;
				maxtrack = 0;
				singleside = 1;
				trackstep = 1;
			}

			if( found )
			{
				nbside=(maxside-minside)+1;
				if(singleside)
					nbside = 1;
				nbtrack=(maxtrack-mintrack)+1;
				if(trackstep==2)
					nbtrack=(nbtrack/trackstep) + 1;

				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"%d track (%d - %d), %d sides (%d - %d)",nbtrack,mintrack,maxtrack,nbside,minside,maxside);

				floppydisk->floppyiftype=GENERIC_SHUGART_DD_FLOPPYMODE;
				floppydisk->floppyBitRate=VARIABLEBITRATE;
				floppydisk->floppyNumberOfTrack=nbtrack;
				floppydisk->floppyNumberOfSide=nbside;
				floppydisk->floppySectorPerTrack=-1;

				floppydisk->tracks = (HXCFE_CYLINDER**)malloc(sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);
				if(!floppydisk->tracks)
					goto alloc_error;

				memset(floppydisk->tracks,0,sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);

				for(j=0;j<floppydisk->floppyNumberOfTrack*trackstep;j=j+trackstep)
				{
					for(i=0;i<floppydisk->floppyNumberOfSide;i++)
					{
						hxcfe_imgCallProgressCallback(imgldr_ctx,(j<<1) | (i&1),(floppydisk->floppyNumberOfTrack*trackstep)*2 );

						if(singlefile)
							sprintf(filepath,"%s",imgfile);
						else
							sprintf(filepath,"%s%s%.2d.%d.logicbin8bits",folder,fname,j,i);

						rpm = 300;

						la.dat_pulse_cnt = 0;
						la.index_cnt = 0;

						binlogicfile2stream(imgldr_ctx,filepath, &la);

						if(la.dat_pulse_cnt>0)
						{
							la.stream = malloc(la.dat_pulse_cnt * sizeof(uint32_t));
							if(la.stream)
								memset(la.stream,0,la.dat_pulse_cnt * sizeof(uint32_t));
						}

						if(la.index_cnt>0)
						{
							la.index_array = malloc(la.index_cnt * sizeof(uint32_t));
							if(la.index_array)
								memset(la.index_array,0,la.index_cnt * sizeof(uint32_t));
						}

						binlogicfile2stream(imgldr_ctx,filepath, &la);

						imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"logicanalyzer_libLoad_DiskFile : Data pulses count = %d",la.dat_pulse_cnt);
						imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"logicanalyzer_libLoad_DiskFile : Last Data position = %d",la.last_dat_pos);
						imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"logicanalyzer_libLoad_DiskFile : Index pulses count = %d",la.index_cnt);
						imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"logicanalyzer_libLoad_DiskFile : First index position = %d",la.first_idx_pos);
						imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"logicanalyzer_libLoad_DiskFile : Last Index position = %d",la.last_idx_pos);

						curside = decodestream(imgldr_ctx->hxcfe,la.stream,la.dat_pulse_cnt,la.index_array, la.index_cnt,0,&rpm,1.0,
									hxcfe_getEnvVarValue( imgldr_ctx->hxcfe, "FLUXSTREAM_PLL_PHASE_CORRECTION_DIVISOR" ), \
									hxcfe_getEnvVarValue( imgldr_ctx->hxcfe, "LOGICANALYZER_BITRATE" ), \
									hxcfe_getEnvVarValue( imgldr_ctx->hxcfe, "FLUXSTREAM_BITRATE_FILTER_WINDOW" ), \
									hxcfe_getEnvVarValue( imgldr_ctx->hxcfe, "FLUXSTREAM_BITRATE_FILTER_PASSES" ), \
									hxcfe_getEnvVarValue( imgldr_ctx->hxcfe, "LOGICANALYZER_BMPEXPORT" ), \
									hxcfe_getEnvVarValue( imgldr_ctx->hxcfe, "LOGICANALYZER_SAMPLERATE" ));

						free(la.index_array);

						free(la.stream);

						la.stream = NULL;
						la.index_array = NULL;

						if(!floppydisk->tracks[j/trackstep])
						{
							floppydisk->tracks[j/trackstep] = allocCylinderEntry(rpm,floppydisk->floppyNumberOfSide);
						}

						currentcylinder = floppydisk->tracks[j/trackstep];
						currentcylinder->sides[i] = curside;
					}
				}

				imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"tracks files successfully loaded and encoded!");

				hxcfe_sanityCheck(imgldr_ctx->hxcfe,floppydisk);

				tmp_env = (envvar_entry *)imgldr_ctx->hxcfe->envvar;
				imgldr_ctx->hxcfe->envvar = backup_env;
				deinitEnv( tmp_env );

				free(la.stream);
				free(la.index_array);

				return HXCFE_NOERROR;
			}
			else
			{
				tmp_env = (envvar_entry *)imgldr_ctx->hxcfe->envvar;
				imgldr_ctx->hxcfe->envvar = backup_env;
				deinitEnv( tmp_env );

				if( la.stream )
					free(la.stream);

				if( la.index_array )
					free(la.index_array);

				return HXCFE_BADFILE;
			}
		}
	}

	return HXCFE_BADFILE;

alloc_error:
	if(folder)
		free(folder);

	if(filepath)
		free(filepath);

	if( backup_env )
	{
		tmp_env = (envvar_entry *)imgldr_ctx->hxcfe->envvar;
		imgldr_ctx->hxcfe->envvar = backup_env;
		deinitEnv( tmp_env );
	}

	return HXCFE_INTERNALERROR;

}

int logicanalyzer_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]="LOGICANALYZER";
	static const char plug_desc[]="Logic Analyzer Stream Loader";
	static const char plug_ext[]="logicbin8bits";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   logicanalyzer_libIsValidDiskFile,
		(LOADDISKFILE)      logicanalyzer_libLoad_DiskFile,
		(WRITEDISKFILE)     NULL,
		(GETPLUGININFOS)    logicanalyzer_libGetPluginInfo
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
