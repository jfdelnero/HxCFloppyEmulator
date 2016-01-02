/*
//
// Copyright (C) 2006-2016 Jean-François DEL NERO
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
// File : KryoFluxStream_loader.c
// Contains: KryoFlux Stream floppy image loader
//
// Written by: DEL NERO Jean Francois
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

#include "kryofluxstream_loader.h"
#include "kryofluxstream_format.h"
#include "kryofluxstream.h"

#include "kryofluxstream_writer.h"

#include "libhxcadaptor.h"

int KryoFluxStream_libIsValidDiskFile(HXCFE_IMGLDR * imgldr_ctx,char * imgfile)
{
	int found,track,side;
	struct stat staterep;
	char * filepath;
	FILE * f;
	s_oob_header oob;
	char filename[512];

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"KryoFluxStream_libIsValidDiskFile");

	if(imgfile)
	{
		memset(&staterep,0,sizeof(struct stat));
		if(!hxc_stat(imgfile,&staterep))
		{
			if(staterep.st_mode&S_IFDIR)
			{

				filepath = malloc( strlen(imgfile) + 32 );

				track=0;
				side=0;
				found=0;
				do
				{
					sprintf(filepath,"%s\\track%.2d.%d.raw",imgfile,track,side);
					f=hxc_fopen(filepath,"rb");
					if(f)
					{
						hxc_fread(&oob,sizeof(s_oob_header),f);
						if(oob.Sign==OOB_SIGN)
						{
							found=1;
						}
						hxc_fclose(f);
					}
					side++;
					if(side>1)
					{
						side = 0;
						track++;
					}

				}while(track<84);

				free( filepath );

				if(found)
				{
					return HXCFE_VALIDFILE;
				}
				else
				{
					return HXCFE_BADFILE;
				}

			}
			else
			{
				hxc_getfilenamebase(imgfile,(char*)&filename);
				hxc_strlower((char*)&filename);
				found=0;

				if(!strstr(filename,".0.raw") && !strstr(filename,".1.raw") )
				{
					return HXCFE_BADFILE;
				}

				f=hxc_fopen(imgfile,"rb");
				if(f)
				{
					hxc_fread(&oob,sizeof(s_oob_header),f);
					if( ( oob.Sign == OOB_SIGN ) && ( oob.Type>=1 && oob.Type<=4 ) )
					{
						found=1;
					}
					hxc_fclose(f);

					if(found)
					{
						return HXCFE_VALIDFILE;
					}
					else
					{
						return HXCFE_BADFILE;
					}
				}
			}
		}
	}

	return HXCFE_BADPARAMETER;
}

static HXCFE_SIDE* decodestream(HXCFE* floppycontext,char * file,short * rpm,float timecoef,int phasecorrection,int bitrate)
{
	HXCFE_SIDE* currentside;

	HXCFE_TRKSTREAM *track_dump;
	HXCFE_FXSA * fxs;

	currentside=0;

	floppycontext->hxc_printf(MSG_DEBUG,"------------------------------------------------");

	floppycontext->hxc_printf(MSG_DEBUG,"Loading %s...",hxc_getfilenamebase(file,0));

	fxs = hxcfe_initFxStream(floppycontext);
	if(fxs)
	{
		track_dump=DecodeKFStreamFile(floppycontext,fxs,file,timecoef);
		if(track_dump)
		{
			hxcfe_FxStream_setBitrate(fxs,bitrate);

			hxcfe_FxStream_setPhaseCorrectionFactor(fxs,phasecorrection);

			currentside = hxcfe_FxStream_AnalyzeAndGetTrack(fxs,track_dump);

			if(rpm && currentside)
				*rpm = (short)( 60 / GetTrackPeriod(floppycontext,currentside) );

			hxcfe_FxStream_FreeStream(fxs,track_dump);
		}

		hxcfe_deinitFxStream(fxs);
	}

	floppycontext->hxc_printf(MSG_DEBUG,"------------------------------------------------");

	return currentside;
}

int KryoFluxStream_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	FILE * f;
	char * filepath;
	char * folder;
	char fname[512];
	int mintrack,maxtrack;
	int minside,maxside,singleside;
	short rpm;
	unsigned short i,j;
	int doublestep;
	HXCFE_CYLINDER* currentcylinder;
	int len;
	int found,track,side;
	struct stat staterep;
	s_oob_header oob;
	HXCFE_SIDE * curside;
	int nbtrack,nbside;
	float timecoef;

	int phasecorrection;
	int bitrate;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"KryoFluxStream_libLoad_DiskFile");

	if(imgfile)
	{
		if(!hxc_stat(imgfile,&staterep))
		{

			len=hxc_getpathfolder(imgfile,0);
			folder=(char*)malloc(len+1);
			hxc_getpathfolder(imgfile,folder);

			if(staterep.st_mode&S_IFDIR)
			{
				sprintf(fname,"track");
			}
			else
			{
				hxc_getfilenamebase(imgfile,(char*)&fname);
				if(!strstr(fname,".0.raw") && !strstr(fname,".1.raw") )
				{
					free(folder);
					return HXCFE_BADFILE;
				}

				fname[strlen(fname)-8]=0;

			}

			filepath = malloc( strlen(imgfile) + 32 );

			doublestep=1;///
			sprintf(filepath,"%s%s",folder,"doublestep");
			f=hxc_fopen(filepath,"rb");
			if(f)
			{
				doublestep=2;
				hxc_fclose(f);
			}

			singleside=0;
			sprintf(filepath,"%s%s",folder,"singleside");
			f=hxc_fopen(filepath,"rb");
			if(f)
			{
				singleside=1;
				hxc_fclose(f);
			}

			timecoef=1;//2;////
			sprintf(filepath,"%s%s",folder,"rpm360rpm300");
			f=hxc_fopen(filepath,"rb");
			if(f)
			{
				timecoef=(float)1.2;
				hxc_fclose(f);
			}

			sprintf(filepath,"%s%s",folder,"rpm300rpm360");
			f=hxc_fopen(filepath,"rb");
			if(f)
			{
				timecoef=(float)0.833;
				hxc_fclose(f);
			}

			phasecorrection = 8;
			sprintf(filepath,"%s%s",folder,"pllsettings.txt");
			f=hxc_fopen(filepath,"rb");
			if(f)
			{
				fscanf(f,"%d",&phasecorrection);
				hxc_fclose(f);
			}

			bitrate = 0;
			sprintf(filepath,"%s%s",folder,"bitrate.txt");
			f=hxc_fopen(filepath,"rb");
			if(f)
			{
				fscanf(f,"%d",&bitrate);
				hxc_fclose(f);
			}

			track=0;
			side=0;
			found=0;

			mintrack=0;
			maxtrack=0;
			minside=0;
			maxside=0;

			do
			{
				sprintf(filepath,"%s%s%.2d.%d.raw",folder,fname,track,side);
				f=hxc_fopen(filepath,"rb");
				if(f)
				{
					hxc_fread(&oob,sizeof(s_oob_header),f);
					if(oob.Sign==OOB_SIGN)
					{
						if(mintrack>track) mintrack = track;
						if(maxtrack<track) maxtrack = track;
						if(minside>side) minside = side;
						if(maxside<side) maxside = side;
						found=1;
					}
					hxc_fclose(f);
				}
				side++;
				if(side>1)
				{
					side = 0;
					track=track+doublestep;
				}
			}while(track<84);

			if(!found)
			{
				free( folder );
				free( filepath );
				return HXCFE_BADFILE;
			}

			nbside=(maxside-minside)+1;
			if(singleside)
				nbside = 1;
			nbtrack=(maxtrack-mintrack)+1;
			if(doublestep==2)
				nbtrack=(nbtrack/doublestep) + 1;

			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"%d track (%d - %d), %d sides (%d - %d)",nbtrack,mintrack,maxtrack,nbside,minside,maxside);

			floppydisk->floppyiftype=GENERIC_SHUGART_DD_FLOPPYMODE;
			floppydisk->floppyBitRate=VARIABLEBITRATE;
			floppydisk->floppyNumberOfTrack=nbtrack;
			floppydisk->floppyNumberOfSide=nbside;
			floppydisk->floppySectorPerTrack=-1;

			floppydisk->tracks=(HXCFE_CYLINDER**)malloc(sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);
			memset(floppydisk->tracks,0,sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);

			for(j=0;j<floppydisk->floppyNumberOfTrack*doublestep;j=j+doublestep)
			{
				for(i=0;i<floppydisk->floppyNumberOfSide;i++)
				{
					hxcfe_imgCallProgressCallback(imgldr_ctx,(j<<1) | (i&1),(floppydisk->floppyNumberOfTrack*doublestep)*2 );

					sprintf(filepath,"%s%s%.2d.%d.raw",folder,fname,j,i);

					curside=decodestream(imgldr_ctx->hxcfe,filepath,&rpm,timecoef,phasecorrection,bitrate);

					if(!floppydisk->tracks[j/doublestep])
					{
						floppydisk->tracks[j/doublestep]=allocCylinderEntry(rpm,floppydisk->floppyNumberOfSide);
					}

					currentcylinder=floppydisk->tracks[j/doublestep];
					currentcylinder->sides[i]=curside;
				}
			}

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

			free( folder );
			free( filepath );

			hxcfe_sanityCheck(imgldr_ctx->hxcfe,floppydisk);

			return HXCFE_NOERROR;
		}
	}

	return HXCFE_BADFILE;
}



int KryoFluxStream_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{

	static const char plug_id[]="KRYOFLUXSTREAM";
	static const char plug_desc[]="KryoFlux Stream Loader";
	static const char plug_ext[]="raw";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)	KryoFluxStream_libIsValidDiskFile,
		(LOADDISKFILE)		KryoFluxStream_libLoad_DiskFile,
		(WRITEDISKFILE)		KryoFluxStream_libWrite_DiskFile,
		(GETPLUGININFOS)	KryoFluxStream_libGetPluginInfo
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
