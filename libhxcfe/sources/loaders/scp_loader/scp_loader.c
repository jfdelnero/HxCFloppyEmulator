/*
//
// Copyright (C) 2006 - 2013 Jean-François DEL NERO
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
#include "libhxcfe.h"

#include "floppy_loader.h"
#include "floppy_utils.h"

#include "scp_loader.h"
#include "scp_writer.h"
#include "scp_format.h"

#include "libhxcadaptor.h"

int SCP_libIsValidDiskFile(HXCFLOPPYEMULATOR* floppycontext,char * imgfile)
{
	scp_header scph;
	FILE *f;

	floppycontext->hxc_printf(MSG_DEBUG,"SCP_libIsValidDiskFile");

	if( hxc_checkfileext(imgfile,"scp") )
	{
		f=hxc_fopen(imgfile,"rb");
		if(f==NULL)
		{
			floppycontext->hxc_printf(MSG_ERROR,"SCP_libIsValidDiskFile : Cannot open %s !",imgfile);
			return HXCFE_ACCESSERROR;
		}

		fread(&scph, sizeof(scp_header), 1, f);

		if( strncmp((char*)scph.sign,"SCP",3) )
		{
			floppycontext->hxc_printf(MSG_ERROR,"SCP_libIsValidDiskFile : Bad file header !");
			fclose(f);
			return HXCFE_BADFILE;
		}

		floppycontext->hxc_printf(MSG_DEBUG,"SCP_libIsValidDiskFile : SCP file !");
		return HXCFE_VALIDFILE;
	}
	else
	{
		floppycontext->hxc_printf(MSG_DEBUG,"SCP_libIsValidDiskFile : non SCP file !");
		return HXCFE_BADFILE;
	}

	return HXCFE_BADPARAMETER;
}

static SIDE* decodestream(HXCFLOPPYEMULATOR* floppycontext,FILE * f,unsigned long foffset,short * rpm,float timecoef,int phasecorrection,int revolution)
{
	SIDE* currentside;
	int totallenght,i,j,offset;

	s_track_dump *track_dump;
	FXS * fxs;
	scp_track_header trkh;

	unsigned short * trackbuf;

	currentside=0;

	floppycontext->hxc_printf(MSG_DEBUG,"------------------------------------------------");

	floppycontext->hxc_printf(MSG_DEBUG,"Loading...");

	fxs = hxcfe_initFxStream(floppycontext);
	if(fxs)
	{
		fseek(f,foffset,SEEK_SET);

		fread(&trkh,sizeof(scp_track_header),1,f);

		if(!strncmp((char*)&trkh.trk_sign,"TRK",3))
		{
			totallenght = 0;
			for(i=0;i<revolution;i++)
			{
				totallenght += trkh.index_position[i].track_lenght;
			}

			totallenght = totallenght * 2;

			if(totallenght)
			{
				trackbuf = malloc(totallenght);
				if(trackbuf)
				{
					memset(trackbuf,0,totallenght);

					offset = 0;
					for(j=0;j<2;j++)
					{
						for(i=0;i<revolution;i++)
						{
							fseek(f,foffset + trkh.index_position[i].track_offset,SEEK_SET);
		
							fread(&trackbuf[offset], trkh.index_position[i].track_lenght , 1, f);

							offset += (trkh.index_position[i].track_lenght/2);
						}
					}

					for(i=0;i<totallenght/2;i++)
					{
						trackbuf[i] = BIGENDIAN_WORD( trackbuf[i] );
					}

					hxcfe_FxStream_setResolution(fxs,25000); // 25 ns per tick

					track_dump = hxcfe_FxStream_ImportStream(fxs,trackbuf,16,totallenght/2);
					if(track_dump)
					{
						offset = 0;
//						hxcfe_FxStream_AddIndex(fxs,track_dump,0);
						for(j=0;j<2;j++)
						{
							for(i=0;i<revolution;i++)
							{
								offset += (trkh.index_position[i].track_lenght/2);
								hxcfe_FxStream_AddIndex(fxs,track_dump,offset-1);
							}
						}

						currentside = hxcfe_FxStream_AnalyzeAndGetTrack(fxs,track_dump);
						
						hxcfe_FxStream_FreeStream(fxs,track_dump);
					}

					free(trackbuf);
				}
			}
		}

		hxcfe_deinitFxStream(fxs);
	}

	floppycontext->hxc_printf(MSG_DEBUG,"------------------------------------------------");

	return currentside;
}


int SCP_libLoad_DiskFile(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	FILE * f;
	int mintrack,maxtrack;
	int minside,maxside;
	short rpm;
	unsigned short i,j;
	int k,l;
	int doublestep;
	CYLINDER* currentcylinder;
	int found,track,side;
	SIDE * curside;
	int nbtrack,nbside;
	float timecoef;
	int tracklen;

	int previous_bit;
	int phasecorrection;

	scp_header scph;
	unsigned long tracksoffset[83*2];

	floppycontext->hxc_printf(MSG_DEBUG,"SCP_libLoad_DiskFile");

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
			fread(&scph,sizeof(scp_header),1,f);
			if(strncmp((char*)&scph.sign,"SCP",3))
			{
				fclose(f);
				floppycontext->hxc_printf(MSG_DEBUG,"SCP_libLoad_DiskFile : bad Header !");
				return HXCFE_BADFILE;
			}

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

			floppycontext->hxc_printf(MSG_DEBUG,"%d track (%d - %d), %d sides (%d - %d)",nbtrack,mintrack,maxtrack,nbside,minside,maxside);

			floppydisk->floppyiftype=GENERIC_SHUGART_DD_FLOPPYMODE;
			floppydisk->floppyBitRate=VARIABLEBITRATE;
			floppydisk->floppyNumberOfTrack=nbtrack;
			floppydisk->floppyNumberOfSide=nbside;
			floppydisk->floppySectorPerTrack=-1;

			fread(tracksoffset,sizeof(tracksoffset),1,f);

			floppydisk->tracks=(CYLINDER**)malloc(sizeof(CYLINDER*)*floppydisk->floppyNumberOfTrack);
			memset(floppydisk->tracks,0,sizeof(CYLINDER*)*floppydisk->floppyNumberOfTrack);

			for(j=0;j<floppydisk->floppyNumberOfTrack*doublestep;j=j+doublestep)
			{
				for(i=0;i<floppydisk->floppyNumberOfSide;i++)
				{
					curside=decodestream(floppycontext,f,tracksoffset[(j<<1)|(i&1)],&rpm,timecoef,phasecorrection,scph.number_of_revolution);

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

			fclose(f);

			// Adjust track timings.
			for(j=0;j<floppydisk->floppyNumberOfTrack;j++)
			{
				for(i=0;i<floppydisk->floppyNumberOfSide;i++)
				{
					curside = floppydisk->tracks[j]->sides[i];
					if(curside && floppydisk->tracks[0]->sides[0])
					{
						AdjustTrackPeriod(floppycontext,floppydisk->tracks[0]->sides[0],curside);
					}
				}
			}

			floppycontext->hxc_printf(MSG_INFO_1,"track file successfully loaded and encoded!");

			hxcfe_sanityCheck(floppycontext,floppydisk);

			return HXCFE_NOERROR;
		}
	}

	return HXCFE_BADFILE;
}

int SCP_libGetPluginInfo(HXCFLOPPYEMULATOR* floppycontext,unsigned long infotype,void * returnvalue)
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
			floppycontext,
			infotype,
			returnvalue,
			plug_id,
			plug_desc,
			&plug_funcs,
			plug_ext
			);
}
