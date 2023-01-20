/*
//
// Copyright (C) 2006-2023 Jean-François DEL NERO
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
} la_stats;

uint32_t * binlogicfile2stream(HXCFE_IMGLDR * imgldr_ctx,char * file, la_stats * la)
{
	FILE * f;
	int size,offset,total_size,block_size,i;
	uint8_t data_buffer[TMP_BUFFER_SIZE];
	uint8_t old_idx_state,old_dat_state;
	int rev_dat_pulses,rev_cnt;

	old_idx_state = 0x00;
	old_dat_state = 0x00;
	la->dat_pulse_cnt = 0;
	la->last_dat_pos = 0;
	la->first_idx_pos = -1;
	la->last_idx_pos = -1;
	la->index_cnt = 0;
	rev_dat_pulses = -1;
	rev_cnt = 0;

	f = fopen(file,"r");
	if(f)
	{
		fseek(f,0,SEEK_END);
		size = ftell(f);
		fseek(f,0,SEEK_SET);

		total_size = size;
		offset = 0;
		while( offset < total_size )
		{
			if(size >= TMP_BUFFER_SIZE)
			{
				fread(&data_buffer,TMP_BUFFER_SIZE,1,f);

				block_size = TMP_BUFFER_SIZE;
			}
			else
			{
				fread(&data_buffer,size,1,f);

				block_size = size;
			}

			size -= block_size;

			for(i=0;i<block_size;i++)
			{
				if( old_idx_state && !(data_buffer[i] & (0x1<<la->idx_pos)) )
				{
					if(la->dat_pulse_cnt > 4000)
					{
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
					la->dat_pulse_cnt++;
					la->last_dat_pos = offset + i;

					if(rev_dat_pulses !=-1)
						rev_dat_pulses++;
				}

				old_idx_state = (data_buffer[i] & (0x1<<la->idx_pos));
				old_dat_state = (data_buffer[i] & (0x1<<la->dat_pos));
			}

			offset += block_size;
		}
		fclose(f);
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"binlogicfile2stream : Can't open %s !", file);
	}
}

int logicanalyzer_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	FILE * f;
	char * filepath;
	char * folder;
	char fname[512];
	int mintrack,maxtrack;
	int minside,maxside,singleside;
	unsigned short i,j;
	int trackstep;
	HXCFE_CYLINDER* currentcylinder;
	int len;
	int found,track,side;
	struct stat staterep;
	HXCFE_SIDE * curside;
	int nbtrack,nbside;

	envvar_entry * backup_env;
	la_stats la;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"logicanalyzer_libLoad_DiskFile");

	backup_env = NULL;

	if(imgfile)
	{
		if(!hxc_stat(imgfile,&staterep))
		{
			backup_env = duplicate_env_vars((envvar_entry *)imgldr_ctx->hxcfe->envvar);

			track=0;
			side=0;
			found=0;

			mintrack=0;
			maxtrack=0;
			minside=0;
			maxside=0;

			la.idx_pos = -1;
			if( hxcfe_getEnvVar( imgldr_ctx->hxcfe, "LOGICANALYZER_INDEX_BIT", NULL) )
				la.idx_pos = hxcfe_getEnvVarValue( imgldr_ctx->hxcfe, "LOGICANALYZER_INDEX_BIT" );

			la.dat_pos = -1;
			if( hxcfe_getEnvVar( imgldr_ctx->hxcfe, "LOGICANALYZER_DATA_BIT", NULL) )
				la.dat_pos = hxcfe_getEnvVarValue( imgldr_ctx->hxcfe, "LOGICANALYZER_DATA_BIT" );

			la.sample_rate = -1;
			if( hxcfe_getEnvVar( imgldr_ctx->hxcfe, "LOGICANALYZER_SAMPLERATE", NULL) )
				la.sample_rate = hxcfe_getEnvVarValue( imgldr_ctx->hxcfe, "LOGICANALYZER_SAMPLERATE" );

			imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"logicanalyzer_libLoad_DiskFile : Index bit = %d",la.idx_pos);
			imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"logicanalyzer_libLoad_DiskFile : Data bit = %d",la.dat_pos);
			imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"logicanalyzer_libLoad_DiskFile : Sample rate = %d Hz",la.sample_rate);

			binlogicfile2stream(imgldr_ctx,imgfile, &la);

			imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"logicanalyzer_libLoad_DiskFile : Data pulses count = %d",la.dat_pulse_cnt);
			imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"logicanalyzer_libLoad_DiskFile : Last Data position = %d",la.last_dat_pos);
			imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"logicanalyzer_libLoad_DiskFile : Index pulses count = %d",la.index_cnt);
			imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"logicanalyzer_libLoad_DiskFile : First index position = %d",la.first_idx_pos);
			imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"logicanalyzer_libLoad_DiskFile : Last Index position = %d",la.last_idx_pos);

			imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"track file successfully loaded and encoded!");

			hxcfe_sanityCheck(imgldr_ctx->hxcfe,floppydisk);

			free_env_vars((envvar_entry *)imgldr_ctx->hxcfe->envvar);
			imgldr_ctx->hxcfe->envvar = backup_env;

			return HXCFE_NOERROR;
		}
	}

	return HXCFE_BADFILE;
}

int logicanalyzer_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{

	static const char plug_id[]="LOGICANALYZER";
	static const char plug_desc[]="Logic Analyzer Stream Loader";
	static const char plug_ext[]="bin";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)	logicanalyzer_libIsValidDiskFile,
		(LOADDISKFILE)		logicanalyzer_libLoad_DiskFile,
		(WRITEDISKFILE)		NULL,
		(GETPLUGININFOS)	logicanalyzer_libGetPluginInfo
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
