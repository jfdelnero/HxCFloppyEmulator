/*
//
// Copyright (C) 2006-2024 Jean-François DEL NERO
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
#include <stdint.h>

#include "fl_includes.h"

#include "libhxcfe.h"
#include "usb_hxcfloppyemulator.h"
#include "libhxcadaptor.h"

#include "soft_cfg_file.h"
#include "fl_dnd_box.h"

#include "main.h"
#include "loader.h"

extern s_gui_context * guicontext;

extern const track_type track_type_list[];
extern void sync_if_config();

int load_floppy(HXCFE_FLOPPY * floppydisk,const char * defaultfilename)
{
	HXCFE_IMGLDR * imgldr_ctx;
	int ret;

	imgldr_ctx = hxcfe_imgInitLoader(guicontext->hxcfe);
	if(imgldr_ctx)
	{
		hxcfe_imgUnload(imgldr_ctx,guicontext->loadedfloppy);
		hxcfe_imgDeInitLoader(imgldr_ctx);
	}

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
#if !defined(STANDALONEFSBROWSER) && !defined(HXC_STREAMER_MODE)
		libusbhxcfe_loadFloppy(guicontext->hxcfe,guicontext->usbhxcfe,guicontext->loadedfloppy);
#endif

		strcpy(guicontext->bufferfilename,defaultfilename);
	}

	guicontext->updatefloppyinfos++;
	guicontext->updatefloppyfs++;

	return ret;
}

int progress_callback(unsigned int current,unsigned int total, void * user)
{
	s_gui_context * guicontext;

	guicontext = (s_gui_context *)user;

	if(total)
		guicontext->loadingprogess = ((float)current / (float)total) * (float)100;
	else
		guicontext->loadingprogess = (float)100;

	return 0;
}

int load_floppy_image(char *filename)
{
	int ret;
	int loaderid;
	HXCFE_IMGLDR * imgldr_ctx;

	ret=-1;

	imgldr_ctx = hxcfe_imgInitLoader(guicontext->hxcfe);
	if(imgldr_ctx)
	{

		guicontext->loadingprogess = 0;
		hxcfe_imgSetProgressCallback(imgldr_ctx,progress_callback,(void*)guicontext);

		hxcfe_imgUnload(imgldr_ctx,guicontext->loadedfloppy);

		guicontext->loadedfloppy=0;

		hxc_getfilenamebase(filename,guicontext->bufferfilename, SYS_PATH_TYPE);

		loaderid = hxcfe_imgAutoSetectLoader(imgldr_ctx,filename,0);

		if(loaderid>=0)
			guicontext->loadedfloppy = hxcfe_imgLoad(imgldr_ctx,filename,loaderid,&ret);

		guicontext->loadstatus=ret;

		if(ret!=HXCFE_NOERROR || !guicontext->loadedfloppy)
		{
			guicontext->loadedfloppy=0;
			guicontext->bufferfilename[0]=0;
		}
		else
		{
			sync_if_config();
	#if !defined(STANDALONEFSBROWSER) && !defined(HXC_STREAMER_MODE)
			libusbhxcfe_loadFloppy(guicontext->hxcfe,guicontext->usbhxcfe,guicontext->loadedfloppy);
	#endif
			if(filename)
			{
				hxc_getfilenamebase(filename,guicontext->bufferfilename, SYS_PATH_TYPE);
			}
			else
			{
				snprintf(guicontext->bufferfilename,sizeof(guicontext->bufferfilename),"Empty Floppy");
			}
		}

		guicontext->updatefloppyinfos++;

		hxcfe_imgDeInitLoader(imgldr_ctx);
	}

	return ret;
}


HXCFE_FLOPPY * loadrawimage(HXCFE* floppycontext,cfgrawfile * rfc,char * file,int * ret)
{
	HXCFE_FLPGEN* fb;
	FILE * f;
	unsigned int i,j,k,nbside;
	unsigned char * trackbuffer;
	int sectornumber;
	int offset;
	HXCFE_FLOPPY * fp;

	*ret = HXCFE_ACCESSERROR;

	trackbuffer = NULL;
	f = NULL;
	fp = NULL;
	if(file)
	{
		f=hxc_fopen(file,"r+b");
		if(!f)
		{
			return NULL;
		}
	}

	if(f || file==NULL)
	{

		nbside=(rfc->sidecfg)&2?2:1;

		if(f)
		{
			trackbuffer=(unsigned char *)malloc((128<<rfc->sectorsize)*rfc->sectorpertrack);
			if(!trackbuffer)
			{
				hxc_fclose(f);
				return NULL;
			}
		}

		fb = hxcfe_initFloppy(floppycontext,rfc->numberoftrack,nbside);
		if(fb)
		{
			*ret = HXCFE_NOERROR;

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
						if(!fread(trackbuffer,(128<<rfc->sectorsize)*rfc->sectorpertrack,1,f))
						{
							*ret = HXCFE_ACCESSERROR;
						}

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

			fp = hxcfe_getFloppy(fb);
		}

		if(f)
		{
			free(trackbuffer);
			hxc_fclose(f);
		}
	}

	return fp;
}

int loadrawfile(HXCFE* floppycontext,cfgrawfile * rfc,char * file)
{
	int ret;
	HXCFE_FLOPPY * fp;
	HXCFE_IMGLDR * imgldr_ctx;

	fp = loadrawimage(floppycontext,rfc,file,&ret);
	if(fp)
	{
		if(guicontext->loadedfloppy)
		{
			imgldr_ctx = hxcfe_imgInitLoader(guicontext->hxcfe);
			if(imgldr_ctx)
			{
				hxcfe_imgUnload(imgldr_ctx,guicontext->loadedfloppy);
				hxcfe_imgDeInitLoader(imgldr_ctx);
			}
		}

		strcpy(guicontext->bufferfilename,"");

		guicontext->loadedfloppy = fp;

		sync_if_config();

#if !defined(STANDALONEFSBROWSER) && !defined(HXC_STREAMER_MODE)
		libusbhxcfe_loadFloppy(guicontext->hxcfe,guicontext->usbhxcfe,guicontext->loadedfloppy);
#endif

		if(guicontext->loadedfloppy)
		{
			guicontext->loadstatus = ret;

			if(file)
				strcpy(guicontext->bufferfilename,hxc_getfilenamebase(file,0,SYS_PATH_TYPE));
			else
				strcpy(guicontext->bufferfilename,"Empty Floppy");
		}

	}

	guicontext->updatefloppyinfos++;
	guicontext->updatefloppyfs++;

	return ret;
}

