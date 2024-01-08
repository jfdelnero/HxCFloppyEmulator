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
// File : dfi_loader.c
// Contains: DiscFerret DFI Stream floppy image loader
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

#include "dfi_loader.h"
#include "dfi_format.h"

#include "libhxcadaptor.h"

#include "misc/env.h"
#include "misc/script_exec.h"

//#define DFIDEBUG 1

int DFI_libIsValidDiskFile( HXCFE_IMGLDR * imgldr_ctx, HXCFE_IMGLDR_FILEINFOS * imgfile )
{
	dfi_header * dfih;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"DFI_libIsValidDiskFile");

	if( hxc_checkfileext(imgfile->path,"dfi",SYS_PATH_TYPE) )
	{
		dfih = (dfi_header *)&imgfile->file_header;

		if( strncmp((char*)&dfih->sign,"DFER",4) && strncmp((char*)&dfih->sign,"DFE2",4) )
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"DFI_libIsValidDiskFile : Bad file header !");
			return HXCFE_BADFILE;
		}

		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"DFI_libIsValidDiskFile : DFI file !");
		return HXCFE_VALIDFILE;
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"DFI_libIsValidDiskFile : non DFI file !");
		return HXCFE_BADFILE;
	}
}

int dfi_rev2_decode(HXCFE_FXSA * fxs, HXCFE_TRKSTREAM *track_dump,int size, unsigned char * src, uint32_t * dst)
{
	uint8_t byte;
	int carry,abspos;
	int i,pulses_cnt;

	pulses_cnt = 0;
	carry = 0;
	abspos = 0;
	for(i=0;i<size;i++)
	{
		byte = src[i];
		if((byte & 0x7F) == 0x7f)
		{
			carry = carry + 127;
			abspos = abspos + 127;
		}
		else
		{
			if( (byte & 0x80) )
			{
				carry = carry + (byte & 0x7F);  // add lower 7 bit value to carry and absolute-position
				abspos = abspos + (byte & 0x7F);

				if(track_dump)
					hxcfe_FxStream_AddIndex(fxs,track_dump,pulses_cnt,carry,FXSTRM_INDEX_MAININDEX);

				//add_index_position(abspos)     // this store was caused by an index pulse: save its absolute position
			}
			else
			{
				if(dst)
				{
					dst[pulses_cnt] = (byte & 0x7f) + carry;
				}

				pulses_cnt++;
				abspos = abspos + (byte & 0x7F);
				carry = 0;
			}
		}
	}

	hxcfe_FxStream_AddIndex(fxs,track_dump,pulses_cnt-1,0,FXSTRM_INDEX_MAININDEX);

	return pulses_cnt;
}

static HXCFE_SIDE* decodestream(HXCFE* floppycontext,FILE * f,int track,short * rpm,float timecoef,int phasecorrection,int revolution, int resolution,int bitrate,int filter,int filterpasses, int bmpexport, int formatrev)
{
	HXCFE_SIDE* currentside;
	int pulses_cnt;
	HXCFE_TRKSTREAM *track_dump;
	HXCFE_FXSA * fxs;
	char tmp_filename[512];
	int tick_period,freq;

	uint32_t * trackbuf_dword;
	uint8_t * block_buf;

	dfi_block_header dfibh;

	currentside=0;

	floppycontext->hxc_printf(MSG_DEBUG,"------------------------------------------------");

	floppycontext->hxc_printf(MSG_DEBUG,"Loading DFI block...");

	hxc_fread(&dfibh, sizeof(dfi_block_header), f);

	floppycontext->hxc_printf(MSG_INFO_1,"cylinder : 0x%.2X",BIGENDIAN_WORD( dfibh.cylinder));
	floppycontext->hxc_printf(MSG_INFO_1,"head : %d",BIGENDIAN_WORD( dfibh.head ));
	floppycontext->hxc_printf(MSG_INFO_1,"sector : %d",BIGENDIAN_WORD( dfibh.sector ));
	floppycontext->hxc_printf(MSG_INFO_1,"data_length : 0x%.4X",BIGENDIAN_DWORD( dfibh.data_length ));

	freq = hxcfe_getEnvVarValue( floppycontext, "DFILOADER_SAMPLE_FREQUENCY_MHZ" );
	tick_period = 10000;  // 10 ns per tick

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

		block_buf = malloc(BIGENDIAN_DWORD( dfibh.data_length ));
		if(block_buf)
		{
			if( fread(block_buf,BIGENDIAN_DWORD( dfibh.data_length ),1,f) == 1 )
			{
				pulses_cnt = dfi_rev2_decode( NULL, NULL, BIGENDIAN_DWORD( dfibh.data_length ), block_buf, NULL);

				trackbuf_dword = malloc((pulses_cnt+1)*sizeof(uint32_t));
				if(trackbuf_dword)
				{
					memset(trackbuf_dword,0x00,(pulses_cnt+1)*sizeof(uint32_t));
					dfi_rev2_decode( NULL, NULL, BIGENDIAN_DWORD( dfibh.data_length ), block_buf, trackbuf_dword);

					hxcfe_FxStream_setResolution(fxs, tick_period);

					track_dump = hxcfe_FxStream_ImportStream(fxs,trackbuf_dword,32,(pulses_cnt), HXCFE_STREAMCHANNEL_TYPE_RLEEVT, "data", NULL);
					if(track_dump)
					{
						hxcfe_FxStream_AddIndex(fxs,track_dump,0,0,FXSTRM_INDEX_MAININDEX);

						dfi_rev2_decode( fxs, track_dump, BIGENDIAN_DWORD( dfibh.data_length ), block_buf, NULL);

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

					free(trackbuf_dword);
					trackbuf_dword = NULL;
				}

				hxcfe_deinitFxStream(fxs);

			}

			free(block_buf);
			block_buf = NULL;
		}
	}

	floppycontext->hxc_printf(MSG_DEBUG,"------------------------------------------------");

	return currentside;
}

extern float clv_track2rpm(int track, int type);

int DFI_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	FILE * f;
	int mintrack,maxtrack;
	int minside,maxside;
	short rpm;
	unsigned short i,j;
	int k,l,len;
	int trackstep,singleside;
	HXCFE_CYLINDER* currentcylinder;
	HXCFE_SIDE * curside;
	int nbtrack,nbside;
	float timecoef;
	int tracklen;
	char * folder;
	char * filepath;
	int previous_bit;
	int phasecorrection;
	int filterpasses,filter;
	int bitrate,bmp_export;
	int mac_clv,c64_clv;
	int filesize;
	int format_rev;

	dfi_header dfih;

	dfi_block_header dfibh;

	envvar_entry * backup_env;
	envvar_entry * tmp_env;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"DFI_libLoad_DiskFile");

	backup_env = NULL;

	if(imgfile)
	{
		mintrack=0;
		maxtrack=0;
		minside=0;
		maxside=0;

		format_rev = 1;

		mac_clv = 0;

		f = hxc_fopen(imgfile,"rb");
		if(f)
		{
			hxc_fread(&dfih, sizeof(dfi_header), f);
			if( strncmp((char*)&dfih.sign,"DFER",4) && strncmp((char*)&dfih.sign,"DFE2",4) )
			{
				hxc_fclose(f);
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"DFI_libLoad_DiskFile : bad Header !");
				return HXCFE_BADFILE;
			}

			if(!strncmp((char*)&dfih.sign,"DFER",4))
				format_rev = 1;

			if(!strncmp((char*)&dfih.sign,"DFE2",4))
				format_rev = 2;

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

			fseek(f,0,SEEK_END);

			filesize = ftell(f);

			fseek(f,sizeof(dfi_header),SEEK_SET);

			memset(&dfibh, 0, sizeof(dfi_block_header) );

			while(!feof(f) && (int)(ftell(f) + BIGENDIAN_DWORD( dfibh.data_length )) < filesize )
			{
				hxc_fread(&dfibh, sizeof(dfi_block_header), f);

				#ifdef DFIDEBUG
				imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"Checking DFI block...");
				imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"cylinder : 0x%.2X",BIGENDIAN_WORD( dfibh.cylinder));
				imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"head : %d",BIGENDIAN_WORD( dfibh.head ));
				imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"sector : %d",BIGENDIAN_WORD( dfibh.sector ));
				imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"data_length : 0x%.4X",BIGENDIAN_DWORD( dfibh.data_length ));
				#endif

				if(maxtrack < BIGENDIAN_WORD( dfibh.cylinder))
				{
					maxtrack = BIGENDIAN_WORD( dfibh.cylinder);
				}

				if(maxtrack < BIGENDIAN_WORD( dfibh.head ))
				{
					maxside = BIGENDIAN_WORD( dfibh.head );
				}

				fseek(f,ftell(f) + BIGENDIAN_DWORD( dfibh.data_length ),SEEK_SET);
			}

			nbtrack=(maxtrack-mintrack)+1;
			if(maxside)
				nbside = 2;
			else
				nbside = 1;

			fseek(f,sizeof(dfi_header),SEEK_SET);

			if( hxcfe_getEnvVarValue( imgldr_ctx->hxcfe, "DFILOADER_DOUBLE_STEP" ) & 1 )
				trackstep = 2;
			else
				trackstep = 1;

			mac_clv = hxcfe_getEnvVarValue( imgldr_ctx->hxcfe, "FLUXSTREAM_IMPORT_PCCAV_TO_MACCLV" );
			c64_clv = hxcfe_getEnvVarValue( imgldr_ctx->hxcfe, "FLUXSTREAM_IMPORT_PCCAV_TO_C64CLV" );

			singleside = hxcfe_getEnvVarValue( imgldr_ctx->hxcfe, "DFILOADER_SINGLE_SIDE" )&1;
			phasecorrection = hxcfe_getEnvVarValue( imgldr_ctx->hxcfe, "FLUXSTREAM_PLL_PHASE_CORRECTION_DIVISOR" );
			filterpasses = hxcfe_getEnvVarValue( imgldr_ctx->hxcfe, "FLUXSTREAM_BITRATE_FILTER_PASSES" );
			filter = hxcfe_getEnvVarValue( imgldr_ctx->hxcfe, "FLUXSTREAM_BITRATE_FILTER_WINDOW" );
			bitrate = hxcfe_getEnvVarValue( imgldr_ctx->hxcfe, "DFILOADER_BITRATE" );
			bmp_export = hxcfe_getEnvVarValue( imgldr_ctx->hxcfe, "DFILOADER_BMPEXPORT" );

			timecoef = 1;
			if( !strcmp(hxcfe_getEnvVar( imgldr_ctx->hxcfe, "FLUXSTREAM_RPMFIX", NULL),"360TO300RPM") )
			{
				timecoef=(float)1.2;
			}

			if( !strcmp(hxcfe_getEnvVar( imgldr_ctx->hxcfe, "FLUXSTREAM_RPMFIX", NULL),"300TO360RPM") )
			{
				timecoef=(float)0.833;
			}

			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"%d track (%d - %d), %d sides (%d - %d)",nbtrack,mintrack,maxtrack,nbside,minside,maxside);

			floppydisk->floppyiftype = GENERIC_SHUGART_DD_FLOPPYMODE;
			floppydisk->floppyBitRate = VARIABLEBITRATE;
			floppydisk->floppyNumberOfTrack = nbtrack;
			if(singleside)
				floppydisk->floppyNumberOfSide = 1;
			else
				floppydisk->floppyNumberOfSide = nbside;

			floppydisk->floppySectorPerTrack = -1;

			floppydisk->tracks=(HXCFE_CYLINDER**)malloc(sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);
			if(!floppydisk->tracks)
			{
				hxc_fclose(f);

				tmp_env = (envvar_entry *)imgldr_ctx->hxcfe->envvar;
				imgldr_ctx->hxcfe->envvar = backup_env;
				deinitEnv( tmp_env );

				return HXCFE_INTERNALERROR;
			}

			memset(floppydisk->tracks,0,sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);

			for(j=0;j<floppydisk->floppyNumberOfTrack*trackstep;j=j+trackstep)
			{
				for(i=0;i<floppydisk->floppyNumberOfSide;i++)
				{
					hxcfe_imgCallProgressCallback(imgldr_ctx,(j<<1) | (i&1),(floppydisk->floppyNumberOfTrack*trackstep)*2 );

					imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Load Track %.3d Side %d",j,i);

					if(mac_clv)
						timecoef = (float)mac_clv / clv_track2rpm(j,1);

					if(c64_clv)
						timecoef = (float)c64_clv / clv_track2rpm(j,2);

					rpm = 300;

					curside = decodestream(imgldr_ctx->hxcfe,f,(j<<1)|(i&1),&rpm,timecoef,phasecorrection,1,1 + 24,bitrate,filter,filterpasses,bmp_export,format_rev);

					if(!floppydisk->tracks[j/trackstep])
					{
						floppydisk->tracks[j/trackstep] = allocCylinderEntry(rpm,floppydisk->floppyNumberOfSide);
					}

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

					currentcylinder=floppydisk->tracks[j/trackstep];
					currentcylinder->sides[i]=curside;
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

int DFI_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]="DFI_FLUX_STREAM";
	static const char plug_desc[]="DiscFerret DFI Stream Loader";
	static const char plug_ext[]="dfi";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   DFI_libIsValidDiskFile,
		(LOADDISKFILE)      DFI_libLoad_DiskFile,
		(WRITEDISKFILE)     NULL,
		(GETPLUGININFOS)    DFI_libGetPluginInfo
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
