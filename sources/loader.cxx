/*
//
// Copyright (C) 2006-2012 Jean-François DEL NERO
//
// This file is part of HxCFloppyEmulator.
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
// File : loader.cxx
// Contains: image loader functions
//
// Written by:	DEL NERO Jean Francois
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

extern "C"
{
	#include "libhxcfe.h"
	#include "usb_hxcfloppyemulator.h"
	#include "libhxcadaptor.h"
}
#include "main.h"
#include "loader.h"


extern s_gui_context * guicontext;

extern track_type track_type_list[];
extern void sync_if_config();

int load_floppy(FLOPPY * floppydisk)
{
	int ret;

	hxcfe_floppyUnload(guicontext->hxcfe,guicontext->loadedfloppy);
	guicontext->loadedfloppy=0;
	guicontext->loadedfloppy=floppydisk;

	sync_if_config();

	ret=HXCFE_NOERROR;
	guicontext->loadstatus=ret;

	if(ret!=HXCFE_NOERROR || !guicontext->loadedfloppy)
	{
		guicontext->loadedfloppy=0;
		guicontext->bufferfilename[0]=0;
	}
	else
	{
		libusbhxcfe_loadFloppy(guicontext->hxcfe,guicontext->usbhxcfe,guicontext->loadedfloppy);

		sprintf(guicontext->bufferfilename,"Floppy Dump");
	}

	guicontext->updatefloppyinfos++;
	return ret;
}

int load_floppy_image(char *filename)
{
	int ret;
	int i;
	int loaderid;

	hxcfe_floppyUnload(guicontext->hxcfe,guicontext->loadedfloppy);
	guicontext->loadedfloppy=0;

	ret=-1;

	loaderid=hxcfe_autoSelectLoader(guicontext->hxcfe,filename,0);
		
	if(loaderid>=0)
		guicontext->loadedfloppy=hxcfe_floppyLoad(guicontext->hxcfe,filename,loaderid,&ret);
		
	guicontext->loadstatus=ret;

	if(ret!=HXCFE_NOERROR || !guicontext->loadedfloppy)
	{
		guicontext->loadedfloppy=0;
		guicontext->bufferfilename[0]=0;
	}
	else
	{	
		sync_if_config();

		libusbhxcfe_loadFloppy(guicontext->hxcfe,guicontext->usbhxcfe,guicontext->loadedfloppy);
		
		if(filename)
		{
			i=strlen(filename);
			while(i!=0 && filename[i]!='\\')
			{
				i--;
			}
			if(filename[i]=='\\') i++;
			sprintf(guicontext->bufferfilename,"%s",&filename[i]);
		}
		else
		{
			sprintf(guicontext->bufferfilename,"Empty Floppy");
		}
	}
	
	guicontext->updatefloppyinfos++;

	return ret;
}

int loadrawfile(HXCFLOPPYEMULATOR* floppycontext,cfgrawfile * rfc,char * file)
{
	FBuilder* fb;
	FILE * f;
	unsigned int i,j,k,nbside;
	int ret;
	unsigned char * trackbuffer;
	int sectornumber;
	int offset;

	f=0;
	if(file)
		f=hxc_fopen(file,"r+b");

	if(f || file==NULL)
	{
		if(guicontext->loadedfloppy)
			hxcfe_floppyUnload(guicontext->hxcfe,guicontext->loadedfloppy);

		nbside=(rfc->sidecfg)&2?2:1;

		if(f)
		{
			trackbuffer=(unsigned char *)malloc((128<<rfc->sectorsize)*rfc->sectorpertrack);
			memset(trackbuffer,rfc->fillvalue,(128<<rfc->sectorsize)*rfc->sectorpertrack);
		}

		sprintf(guicontext->bufferfilename,"");
		guicontext->loadedfloppy=0;
		fb=hxcfe_initFloppy(floppycontext,rfc->numberoftrack,nbside);
		if(fb)
		{
			hxcfe_setTrackInterleave(fb,rfc->interleave);	
			hxcfe_setSectorFill(fb,rfc->fillvalue);

			hxcfe_setTrackPreGap(fb, (unsigned short)rfc->pregap);

			if(rfc->autogap3)
				hxcfe_setSectorGap3(fb,255);
			else
				hxcfe_setSectorGap3(fb,(unsigned char)rfc->gap3);

			hxcfe_setTrackBitrate(fb,rfc->bitrate);

			for(i=0;i<rfc->numberoftrack;i++)
			{
				sectornumber=rfc->firstidsector;

				for(j=0;j<nbside;j++)
				{

					if(!rfc->intersidesectornumbering)
						sectornumber=rfc->firstidsector;

					// prepare a new track.
					if( ( rfc->sidecfg & SIDE_INVERTED ) && nbside==2)
						hxcfe_pushTrack(fb,rfc->rpm,i,1-j,track_type_list[rfc->tracktype].tracktype);
					else
						hxcfe_pushTrack(fb,rfc->rpm,i,j,track_type_list[rfc->tracktype].tracktype);
					
					// Set the skew
					if(rfc->sideskew)
					{
						hxcfe_setTrackSkew(fb,(unsigned char)(((i<<1)|(j&1))*rfc->skew) );
					}
					else
					{
						if(rfc->sectorpertrack)
							hxcfe_setTrackSkew(fb,((unsigned char)i*rfc->skew)%rfc->sectorpertrack);
					}

					if(f)
					{
						memset(trackbuffer,rfc->fillvalue,(128<<rfc->sectorsize)*rfc->sectorpertrack);

						if(rfc->sidecfg & SIDE0_FIRST )
						{
							offset = 0;
							if(j)
							{
								offset = (128<<rfc->sectorsize) * rfc->sectorpertrack * rfc->numberoftrack;
							}
							
							offset += (128<<rfc->sectorsize) * rfc->sectorpertrack * i;
						}
						else
						{
							offset  = (128<<rfc->sectorsize) * rfc->sectorpertrack * i * nbside;
							offset += (128<<rfc->sectorsize) * rfc->sectorpertrack * j;
						}
						
						fseek(f,offset,SEEK_SET);
						fread(trackbuffer,(128<<rfc->sectorsize)*rfc->sectorpertrack,1,f);
					}

					// add all sectors
					for(k=0;k<rfc->sectorpertrack;k++)
					{
						if(f)
							hxcfe_addSector(fb,sectornumber,j,i,&trackbuffer[k*(128<<rfc->sectorsize)],128<<rfc->sectorsize);
						else
							hxcfe_addSector(fb,sectornumber,j,i,0,128<<rfc->sectorsize);

						sectornumber++;
					}

					// generate the track
					hxcfe_popTrack(fb);
				}
			}

			guicontext->loadedfloppy=hxcfe_getFloppy(fb);
		}

		/*t=hxcfe_getfloppysize(floppycontext,thefloppydisk,0);

		test=malloc(512*18);

		ss=hxcfe_init_sectorsearch(floppycontext,thefloppydisk);
		t=hxcfe_readsectordata(ss,0,0,1,5,512,test);

		hxcfe_deinit_sectorsearch(ss);*/

		sync_if_config();

		libusbhxcfe_loadFloppy(guicontext->hxcfe,guicontext->usbhxcfe,guicontext->loadedfloppy);

		if(guicontext->loadedfloppy)
		{
			ret=HXCFE_NOERROR;

			guicontext->loadstatus=ret;

			if(f)
				strcpy(guicontext->bufferfilename,hxc_getfilenamebase(file,0));
			else
				strcpy(guicontext->bufferfilename,"Empty Floppy");
		}

		if(f)
		{
			free(trackbuffer);
			hxc_fclose(f);
		}
	}	

	guicontext->updatefloppyinfos++;
	guicontext->updatefloppyfs++;

	return ret;
}

