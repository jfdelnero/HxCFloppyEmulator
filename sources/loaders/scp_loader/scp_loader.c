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
// File : scp_loader.c
// Contains: SCP Stream floppy image loader
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

#include "scp_loader.h"
#include "scp_writer.h"
#include "scp_format.h"

#include "libhxcadaptor.h"

//#define SCPDEBUG 1

int SCP_libIsValidDiskFile(HXCFE_IMGLDR * imgldr_ctx,char * imgfile)
{
	scp_header scph;
	FILE *f;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"SCP_libIsValidDiskFile");

	if( hxc_checkfileext(imgfile,"scp") )
	{
		f=hxc_fopen(imgfile,"rb");
		if(f==NULL)
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"SCP_libIsValidDiskFile : Cannot open %s !",imgfile);
			return HXCFE_ACCESSERROR;
		}

		hxc_fread(&scph, sizeof(scp_header), f);

		if( strncmp((char*)scph.sign,"SCP",3) )
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"SCP_libIsValidDiskFile : Bad file header !");
			hxc_fclose(f);
			return HXCFE_BADFILE;
		}

		hxc_fclose(f);
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"SCP_libIsValidDiskFile : SCP file !");
		return HXCFE_VALIDFILE;
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"SCP_libIsValidDiskFile : non SCP file !");
		return HXCFE_BADFILE;
	}
}

static HXCFE_SIDE* decodestream(HXCFE* floppycontext,FILE * f,uint32_t foffset,short * rpm,float timecoef,int phasecorrection,int revolution)
{
	HXCFE_SIDE* currentside;
	int totallength,i,k,offset;

	HXCFE_TRKSTREAM *track_dump;
	HXCFE_FXSA * fxs;
	scp_track_header trkh;

	unsigned short * trackbuf;
	uint32_t * trackbuf_dword;
	uint32_t realnumberofpulses;
	uint32_t revonumberofpulses;
	uint32_t curpulselength;
	uint32_t j;

	currentside=0;

	floppycontext->hxc_printf(MSG_DEBUG,"------------------------------------------------");

	floppycontext->hxc_printf(MSG_DEBUG,"Loading SCP track...");

	fxs = hxcfe_initFxStream(floppycontext);
	if(fxs)
	{
		fseek(f,foffset,SEEK_SET);

		hxc_fread(&trkh,sizeof(scp_track_header),f);

		if(!strncmp((char*)&trkh.trk_sign,"TRK",3))
		{
			totallength = 0;
			for(i=0;i<revolution;i++)
			{
				totallength += trkh.index_position[i].track_length;

#ifdef SCPDEBUG
				floppycontext->hxc_printf(MSG_DEBUG,"Revolution %d : %d words - offset 0x%x - index time : %d",i,trkh.index_position[i].track_length,trkh.index_position[i].track_offset,(trkh.index_position[i].index_time));
#endif
			}


#ifdef SCPDEBUG
			floppycontext->hxc_printf(MSG_DEBUG,"Total track length : [0x%X - 0x%X] - %d bytes",foffset + trkh.index_position[0].track_offset,foffset + trkh.index_position[0].track_offset + ((totallength*sizeof(unsigned short))-1),totallength*sizeof(unsigned short));
#endif

			totallength++;

			if(totallength)
			{
				trackbuf = malloc(totallength*sizeof(unsigned short));
				if(trackbuf)
				{
					memset(trackbuf,0x00,totallength*sizeof(unsigned short));

					offset = 0;

					for(i=0;i<revolution;i++)
					{
						fseek(f,foffset + trkh.index_position[i].track_offset,SEEK_SET);

						hxc_fread(&trackbuf[offset], (trkh.index_position[i].track_length*sizeof(unsigned short)), f);

#ifdef SCPDEBUG
						floppycontext->hxc_printf(MSG_DEBUG,"SCP read stream - offset [0x%X - 0x%X] : %d bytes",
							 foffset + trkh.index_position[i].track_offset,
							 (foffset + trkh.index_position[i].track_offset) + ((trkh.index_position[i].track_length*sizeof(unsigned short)) - 1 ),
							(trkh.index_position[i].track_length*sizeof(unsigned short)));
#endif
						offset += (trkh.index_position[i].track_length);
					}

					for(i=0;i<totallength;i++)
					{
						trackbuf[i] = BIGENDIAN_WORD( trackbuf[i] );
					}

					realnumberofpulses = 0;

					trackbuf_dword = malloc((totallength+1)*sizeof(uint32_t));
					if(trackbuf_dword)
					{
						memset(trackbuf_dword,0x00,(totallength+1)*sizeof(uint32_t));
						curpulselength = 0;
						k = 0;

						for(i=0;i<revolution;i++)
						{
							revonumberofpulses = 0;

							for(j=0;j<trkh.index_position[i].track_length;j++)
							{
								curpulselength += trackbuf[k];

								if(trackbuf[k])
								{
									trackbuf_dword[realnumberofpulses] = curpulselength;
									curpulselength = 0;
									realnumberofpulses++;
									revonumberofpulses++;
								}
								else
								{
									curpulselength += 65536;
								}

								k++;
							}

							trkh.index_position[i].track_length = revonumberofpulses;

						}

						// dummy pulse
						trackbuf_dword[realnumberofpulses] = 300;
						realnumberofpulses++;

					}
					free(trackbuf);

					hxcfe_FxStream_setResolution(fxs,25000); // 25 ns per tick

					track_dump = hxcfe_FxStream_ImportStream(fxs,trackbuf_dword,32,(realnumberofpulses));
					if(track_dump)
					{
						if(revolution)
						{
							offset = 0;
							hxcfe_FxStream_AddIndex(fxs,track_dump,offset,0);

							for(i=0;i<revolution;i++)
							{
								offset += (trkh.index_position[i].track_length);
								hxcfe_FxStream_AddIndex(fxs,track_dump,offset,0);
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
			}
		}

		hxcfe_deinitFxStream(fxs);
	}

	floppycontext->hxc_printf(MSG_DEBUG,"------------------------------------------------");

	return currentside;
}


int SCP_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	FILE * f;
	int mintrack,maxtrack;
	int minside,maxside;
	short rpm;
	unsigned short i,j;
	int k,l;
	int doublestep;
	HXCFE_CYLINDER* currentcylinder;
	int found,track,side;
	HXCFE_SIDE * curside;
	int nbtrack,nbside;
	float timecoef;
	int tracklen;

	int previous_bit;
	int phasecorrection;

	scp_header scph;
	uint32_t tracksoffset[83*2];

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"SCP_libLoad_DiskFile");

	if(imgfile)
	{
		track=0;
		side=0;
		found=0;

		mintrack=0;
		maxtrack=0;
		minside=0;
		maxside=0;

		f=hxc_fopen(imgfile,"rb");
		if(f)
		{
			hxc_fread(&scph, sizeof(scp_header), f);
			if(strncmp((char*)&scph.sign,"SCP",3))
			{
				hxc_fclose(f);
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"SCP_libLoad_DiskFile : bad Header !");
				return HXCFE_BADFILE;
			}

			imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"Loading SCP file...");
			imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"Version : 0x%.2X",scph.version);
			imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"Disk Type : 0x%.2X",scph.disk_type);
			imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"Start track : %d",scph.start_track);
			imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"End track : %d",scph.end_track);
			imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"Number of revolution(s) : %d",scph.number_of_revolution);
			imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"Flags : 0x%.2X",scph.flags);
			imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"File Checksum : 0x%.4X",scph.file_data_checksum);
			imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"RFU 1,2,3 : 0x%.2X,0x%.2X,0x%.2X",scph.RFU_0,scph.RFU_1,scph.RFU_2);

			nbside = 1;

			doublestep = 1;

			phasecorrection = 8;

			timecoef = 1;

			mintrack = scph.start_track;
			maxtrack = scph.end_track;

			nbtrack=(maxtrack-mintrack)+1;

			if(nbtrack > 84)
			{
				nbtrack = nbtrack / 2;
				nbside = 2;
			}

			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"%d track (%d - %d), %d sides (%d - %d)",nbtrack,mintrack,maxtrack,nbside,minside,maxside);

			floppydisk->floppyiftype = GENERIC_SHUGART_DD_FLOPPYMODE;
			floppydisk->floppyBitRate = VARIABLEBITRATE;
			floppydisk->floppyNumberOfTrack = nbtrack;
			floppydisk->floppyNumberOfSide = nbside;
			floppydisk->floppySectorPerTrack = -1;

			hxc_fread(tracksoffset,sizeof(tracksoffset),f);

			floppydisk->tracks=(HXCFE_CYLINDER**)malloc(sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);
			memset(floppydisk->tracks,0,sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);

			for(j=0;j<floppydisk->floppyNumberOfTrack*doublestep;j=j+doublestep)
			{
				for(i=0;i<floppydisk->floppyNumberOfSide;i++)
				{
					hxcfe_imgCallProgressCallback(imgldr_ctx,(j<<1) | (i&1),(floppydisk->floppyNumberOfTrack*doublestep)*2 );

					imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Load Track %.3d Side %d",j,i);

					if(floppydisk->floppyNumberOfSide == 2)
						curside=decodestream(imgldr_ctx->hxcfe,f,tracksoffset[(j<<1)|(i&1)],&rpm,timecoef,phasecorrection,scph.number_of_revolution);
					else
						curside=decodestream(imgldr_ctx->hxcfe,f,tracksoffset[j],&rpm,timecoef,phasecorrection,scph.number_of_revolution);

					if(!floppydisk->tracks[j/doublestep])
					{
						floppydisk->tracks[j/doublestep]=allocCylinderEntry(rpm,floppydisk->floppyNumberOfSide);
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

					currentcylinder=floppydisk->tracks[j/doublestep];
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

			return HXCFE_NOERROR;
		}
	}

	return HXCFE_BADFILE;
}

int SCP_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]="SCP_FLUX_STREAM";
	static const char plug_desc[]="SCP Stream Loader";
	static const char plug_ext[]="scp";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)	SCP_libIsValidDiskFile,
		(LOADDISKFILE)		SCP_libLoad_DiskFile,
		(WRITEDISKFILE)		SCP_libWrite_DiskFile,
		(GETPLUGININFOS)	SCP_libGetPluginInfo
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
