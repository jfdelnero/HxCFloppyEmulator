/*
//
// Copyright (C) 2006 - 2012 Jean-François DEL NERO
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
// File : floppy_loader.c
// Contains: Library interface functions
//
// Written by:	DEL NERO Jean Francois
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <string.h>
#include <stdio.h>


#include "libhxcfe.h"
#include "floppy_loader.h"
#include "floppy_utils.h"

#include "loaders_list.h"

#include "version.h"
#include "licensetxt.h"

int dummy_output(int MSGTYPE,char * chaine, ...)
{
    return 0;
}

int dummy_trackpos(unsigned int current,unsigned int total)
{
    return 0;
}

////////////////////////////////////////////////////////////////////////

HXCFLOPPYEMULATOR* hxcfe_init(void)
{
	HXCFLOPPYEMULATOR* hxcfe;

	hxcfe=malloc(sizeof(HXCFLOPPYEMULATOR));
	if(hxcfe)
	{
		memset(hxcfe,0,sizeof(HXCFLOPPYEMULATOR));

		hxcfe->hxc_printf=&dummy_output;
		hxcfe->hxc_settrackpos=&dummy_trackpos;
	
		hxcfe->hxc_printf(MSG_INFO_0,"Starting HxCFloppyEmulator...");

		sprintf(hxcfe->CONTAINERTYPE,"AUTOSELECT");	
	}
	return hxcfe;
}

int hxcfe_setOutputFunc(HXCFLOPPYEMULATOR* floppycontext,HXCPRINTF_FUNCTION hxc_printf)
{	
	floppycontext->hxc_printf=hxc_printf;

	return HXCFE_NOERROR;
}

const char * hxcfe_getVersion(HXCFLOPPYEMULATOR* floppycontext)
{
	return STR_FILE_VERSION2;
}

const char * hxcfe_getLicense(HXCFLOPPYEMULATOR* floppycontext)
{
	return licensetxt;
}

void hxcfe_deinit(HXCFLOPPYEMULATOR* hxcfe)
{
	if(hxcfe)
	{
		hxcfe->hxc_printf(MSG_INFO_0,"Stopping HxCFloppyEmulator...");

		free(hxcfe);
	}
}

////////////////////////////////////////////////////////////////////////

int hxcfe_checkLoaderID(HXCFLOPPYEMULATOR* floppycontext,int moduleID)
{
	int i;

	i=0;
	do
	{
		i++;
	}while((staticplugins[i]!=(GETPLUGININFOS)-1) && i<moduleID);

	if(staticplugins[i]==(GETPLUGININFOS)-1)
	{
		return HXCFE_BADPARAMETER;
	}
	else
	{	
		return HXCFE_NOERROR;
	}
}

int hxcfe_numberOfLoader(HXCFLOPPYEMULATOR* floppycontext)
{
	int i;

	i=0;
	do
	{
		i++;
	}while((staticplugins[i]!=(GETPLUGININFOS)-1));

	return i;
}


int hxcfe_getLoaderID(HXCFLOPPYEMULATOR* floppycontext,char * container)
{
	int i;
	int ret;
	char * plugin_id;

	ret=0;
	i=0;
	do
	{
		if(staticplugins[i])
		{
			ret=staticplugins[i](floppycontext,GETPLUGINID,&plugin_id);
			if((ret==HXCFE_NOERROR)  && !strcmp(container,plugin_id))
			{
				return i;
			}
		}

		i++;
	}while(staticplugins[i]!=(GETPLUGININFOS)-1);
		
	floppycontext->hxc_printf(MSG_DEBUG,"hxcfe_searchContainer : Plugin %s not found !",container);

	return -1;
}

int hxcfe_getLoaderAccess(HXCFLOPPYEMULATOR* floppycontext,int moduleID)
{
	int ret;
	plugins_ptr func_ptr;

	if(hxcfe_checkLoaderID(floppycontext,moduleID)==HXCFE_NOERROR)
	{
		ret=staticplugins[moduleID](floppycontext,GETFUNCPTR,&func_ptr);
		if(ret==HXCFE_NOERROR)
		{
			if(func_ptr.libLoad_DiskFile)
				ret=ret | 0x01;
			if(func_ptr.libWrite_DiskFile)
				ret=ret | 0x02;

			return ret;
		}
	}
	else
	{
		floppycontext->hxc_printf(MSG_ERROR,"Bad module ID : %x !",moduleID);
	}

	return ret;
}

const char* hxcfe_getLoaderDesc(HXCFLOPPYEMULATOR* floppycontext,int moduleID)
{
	int ret;
	plugins_ptr func_ptr;
	char * desc;

	if(hxcfe_checkLoaderID(floppycontext,moduleID)==HXCFE_NOERROR)
	{
		ret=staticplugins[moduleID](floppycontext,GETFUNCPTR,&func_ptr);
		if(ret==HXCFE_NOERROR)
		{
			staticplugins[moduleID](floppycontext,GETDESCRIPTION,&desc);
			return desc;
		}
	}
	else
	{
		floppycontext->hxc_printf(MSG_ERROR,"Bad module ID : %x !",moduleID);
	}

	return 0;
}

const char* hxcfe_getLoaderName(HXCFLOPPYEMULATOR* floppycontext,int moduleID)
{
	int ret;
	plugins_ptr func_ptr;
	char * desc;

	if(hxcfe_checkLoaderID(floppycontext,moduleID)==HXCFE_NOERROR)
	{
		ret=staticplugins[moduleID](floppycontext,GETFUNCPTR,&func_ptr);
		if(ret==HXCFE_NOERROR)
		{
			staticplugins[moduleID](floppycontext,GETPLUGINID,&desc);
			return desc;
		}
	}
	else
	{
		floppycontext->hxc_printf(MSG_ERROR,"Bad module ID : %x !",moduleID);
	}

	return 0;
}

const char* hxcfe_getLoaderExt(HXCFLOPPYEMULATOR* floppycontext,int moduleID)
{
	int ret;
	plugins_ptr func_ptr;
	char * desc;

	if(hxcfe_checkLoaderID(floppycontext,moduleID)==HXCFE_NOERROR)
	{
		ret=staticplugins[moduleID](floppycontext,GETFUNCPTR,&func_ptr);
		if(ret==HXCFE_NOERROR)
		{
			staticplugins[moduleID](floppycontext,GETEXTENSION,&desc);
			return desc;
		}
	}
	else
	{
		floppycontext->hxc_printf(MSG_ERROR,"Bad module ID : %x !",moduleID);
	}

	return 0;
}

int hxcfe_autoSelectLoader(HXCFLOPPYEMULATOR* floppycontext,char* imgname,int moduleID)
{
	int i;
	int ret;
	plugins_ptr func_ptr;

	floppycontext->hxc_printf(MSG_INFO_0,"Checking %s",imgname);

	i=moduleID;
	do
	{
		if(staticplugins[i])
		{
			ret=staticplugins[i](floppycontext,GETFUNCPTR,&func_ptr);
			if(ret==HXCFE_NOERROR)
			{
				if(func_ptr.libIsValidDiskFile)
				{
					ret=func_ptr.libIsValidDiskFile(floppycontext,imgname);
					if(ret==HXCFE_VALIDFILE)
					{						
						floppycontext->hxc_printf(MSG_INFO_0,"File loader found : %s (%s)",hxcfe_getLoaderName(floppycontext,i),hxcfe_getLoaderDesc(floppycontext,i));
						return i;
					}
					else
					{
						floppycontext->hxc_printf(MSG_DEBUG,"libIsValidDiskFile n%d return %d",i,ret);
					}
				}
			}
		}

		i++;
	}while(staticplugins[i]!=(GETPLUGININFOS)-1);
		
	floppycontext->hxc_printf(MSG_ERROR,"No loader support the file %s !",imgname);
	
	ret=HXCFE_UNSUPPORTEDFILE;

	return -1;
}


FLOPPY * hxcfe_floppyLoad(HXCFLOPPYEMULATOR* floppycontext,char* imgname,int moduleID,int * err_ret)
{
	int ret;
	FLOPPY * newfloppy;
	plugins_ptr func_ptr;

	floppycontext->hxc_printf(MSG_INFO_0,"Loading %s",imgname);

	newfloppy=0;

	if(hxcfe_checkLoaderID(floppycontext,moduleID)==HXCFE_NOERROR)
	{
		ret=staticplugins[moduleID](floppycontext,GETFUNCPTR,&func_ptr);
		if(ret==HXCFE_NOERROR)
		{
			ret=func_ptr.libIsValidDiskFile(floppycontext,imgname);
			if(ret==HXCFE_VALIDFILE)
			{
				newfloppy=malloc(sizeof(FLOPPY));
				memset(newfloppy,0,sizeof(FLOPPY));
				floppycontext->hxc_printf(MSG_INFO_0,"file loader found!");
				ret=func_ptr.libLoad_DiskFile(floppycontext,newfloppy,imgname,0);
				if(ret!=HXCFE_NOERROR)
				{
					free(newfloppy);
					newfloppy=0;
				}

				if(err_ret) *err_ret=ret;

				return newfloppy;
			}
			else
			{
				floppycontext->hxc_printf(MSG_ERROR,"%s refuse the file %s (%d)!",hxcfe_getLoaderName(floppycontext,moduleID),imgname,ret);
				if(err_ret) *err_ret=ret;
				return newfloppy;
			}
		}
	}
		
	floppycontext->hxc_printf(MSG_ERROR,"Bad plugin ID : 0x%x",moduleID);

	ret=HXCFE_BADPARAMETER;
	if(err_ret) *err_ret=ret;
	return newfloppy;

}


int hxcfe_floppyUnload(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * floppydisk)
{
	unsigned int i,j;

	if(floppydisk)
	{
		for(j=0;j<floppydisk->floppyNumberOfTrack;j++)
		{
			for(i=0;i<floppydisk->floppyNumberOfSide;i++)
			{
				if(floppydisk->tracks[j]->sides[i]->databuffer)
				{
					free(floppydisk->tracks[j]->sides[i]->databuffer);
					floppydisk->tracks[j]->sides[i]->databuffer=0;
				}
				if(floppydisk->tracks[j]->sides[i]->flakybitsbuffer)
				{
					free(floppydisk->tracks[j]->sides[i]->flakybitsbuffer);
					floppydisk->tracks[j]->sides[i]->flakybitsbuffer=0;
				}
				if(floppydisk->tracks[j]->sides[i]->indexbuffer)
				{
					free(floppydisk->tracks[j]->sides[i]->indexbuffer);
					floppydisk->tracks[j]->sides[i]->indexbuffer=0;
				}
				if(floppydisk->tracks[j]->sides[i]->timingbuffer)
				{
					free(floppydisk->tracks[j]->sides[i]->timingbuffer);
					floppydisk->tracks[j]->sides[i]->timingbuffer=0;
				}

				if(floppydisk->tracks[j]->sides[i]->track_encoding_buffer)
				{
					free(floppydisk->tracks[j]->sides[i]->track_encoding_buffer);
					floppydisk->tracks[j]->sides[i]->track_encoding_buffer=0;
				}			
				
				free(floppydisk->tracks[j]->sides[i]);
			}

			free(floppydisk->tracks[j]->sides);
			free(floppydisk->tracks[j]);

		}
		free(floppydisk->tracks);

		free(floppydisk);
	}
	return 0;
}

int hxcfe_floppyExport(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * newfloppy,char* imgname,int moduleID)
{
	int ret;
	plugins_ptr func_ptr;

	if(hxcfe_checkLoaderID(floppycontext,moduleID)==HXCFE_NOERROR)
	{
		ret=staticplugins[moduleID](floppycontext,GETFUNCPTR,&func_ptr);
		if(ret==HXCFE_NOERROR)
		{
			if(func_ptr.libWrite_DiskFile)
			{
				ret=func_ptr.libWrite_DiskFile(floppycontext,newfloppy,imgname,0);
				return ret;
			}
			else
			{
				floppycontext->hxc_printf(MSG_ERROR,"No export support in %s!",hxcfe_getLoaderName(floppycontext,moduleID) );
				return HXCFE_UNSUPPORTEDFILE;
			}
		}
	}
		
	floppycontext->hxc_printf(MSG_ERROR,"Bad plugin ID : 0x%x",moduleID);
	return HXCFE_BADPARAMETER;
}

////////////////////////////////////////////////////////////////////////

int hxcfe_floppyGetSetParams(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * newfloppy,unsigned char dir,unsigned short param,void * value)
{
	int ret;

	ret=HXCFE_BADPARAMETER;
	switch(dir)
	{
		case GET:
			switch(param)
			{
				case DOUBLESTEP:
					*((unsigned char*)value)=newfloppy->double_step;
					ret=HXCFE_NOERROR;
				break;

				case INTERFACEMODE:
					*((unsigned short*)value)=newfloppy->floppyiftype;
					ret=HXCFE_NOERROR;
				break;
			}
		break;
		case SET:
			switch(param)
			{
				case DOUBLESTEP:
					if(*(unsigned char*)value)
					{
						newfloppy->double_step=0xFF;
					}
					else
					{
						newfloppy->double_step=0x00;
					}
					ret=HXCFE_NOERROR;
				break;

				case INTERFACEMODE:
					newfloppy->floppyiftype=*(unsigned short*)value;
					ret=HXCFE_NOERROR;
				break;
			}
		break;

	}
	return ret;
}

int hxcfe_floppyGetInterfaceMode(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * newfloppy)
{
	return newfloppy->floppyiftype;
}

int hxcfe_floppySetInterfaceMode(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * newfloppy,int ifmode)
{

	if(hxcfe_getFloppyInterfaceModeName(floppycontext,ifmode))
	{
		newfloppy->floppyiftype=ifmode;
		return HXCFE_NOERROR;
	}

	return HXCFE_BADPARAMETER;
}

int hxcfe_floppyGetDoubleStep(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * newfloppy)
{
	return newfloppy->double_step;
}

int hxcfe_floppySetDoubleStep(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * newfloppy,int doublestep)
{
	newfloppy->double_step=doublestep;
	return HXCFE_NOERROR;
}

int libGetPluginInfo(HXCFLOPPYEMULATOR* floppycontext,unsigned long infotype,void * returnvalue,const char * pluginid,const char * plugindesc,plugins_ptr * pluginfunc,const char * fileext)
{
	if(returnvalue)
	{
		switch(infotype)
		{	
			case GETPLUGINID:
				*(char**)(returnvalue)=(char*)pluginid;
				break;
			
			case GETDESCRIPTION:
				*(char**)(returnvalue)=(char*)plugindesc;
				break;
			
			case GETFUNCPTR:
				memcpy(returnvalue,pluginfunc,sizeof(plugins_ptr));
				break;

			case GETEXTENSION:
				*(char**)(returnvalue)=(char*)fileext;
				break;
				
			default:
				return HXCFE_BADPARAMETER;
				break;
		}
		
		return HXCFE_NOERROR;
	}
	return HXCFE_BADPARAMETER;
}

FBuilder* hxcfe_initFloppy(HXCFLOPPYEMULATOR* floppycontext,int nb_of_track,int nb_of_side)
{
	FBuilder* fb;

	fb=0;

	floppycontext->hxc_printf(MSG_DEBUG,"hxcfe_init_floppy : floppy builder init");

	if(( nb_of_track && (nb_of_track<=256)) && ((nb_of_side>=1) && (nb_of_side<=2) ) )
	{
		fb=(FBuilder*)malloc(sizeof(FBuilder));
		if(fb)
		{
			memset(fb,0,sizeof(FBuilder));
			fb->floppydisk=(FLOPPY*)malloc(sizeof(FLOPPY));
			memset(fb->floppydisk,0,sizeof(FLOPPY));
			fb->floppydisk->floppyBitRate=DEFAULT_DD_BITRATE;
			fb->floppydisk->double_step=0;
			fb->floppydisk->floppyiftype=GENERIC_SHUGART_DD_FLOPPYMODE;
			fb->floppydisk->floppyNumberOfTrack=nb_of_track;
			fb->floppydisk->floppyNumberOfSide=nb_of_side;
			fb->floppydisk->floppySectorPerTrack=-1;
			
			
			fb->floppydisk->tracks=(CYLINDER**)malloc(sizeof(CYLINDER*)*fb->floppydisk->floppyNumberOfTrack);
			memset(fb->floppydisk->tracks,0,sizeof(CYLINDER*)*fb->floppydisk->floppyNumberOfTrack);

			fb->fb_stack_pointer=0;
			fb->fb_stack=(fb_track_state*)malloc(sizeof(fb_track_state)*STACK_SIZE);
			memset(fb->fb_stack,0,sizeof(fb_track_state)*STACK_SIZE);
			
			fb->fb_stack[0].interleave=1;
			fb->fb_stack[0].rpm=300;

			fb->fb_stack[0].indexlen=2500;
			fb->fb_stack[0].indexpos=-2500;
			fb->fb_stack[0].sectorunderindex=0;

			fb->fb_stack[0].bitrate=250000;
			fb->fb_stack[0].sectorconfig.bitrate=fb->fb_stack[0].bitrate;
			fb->fb_stack[0].sectorconfig.fill_byte=0xF6;
			fb->fb_stack[0].sectorconfig.gap3=255;
			fb->fb_stack[0].sectorconfig.sectorsize=512;
			fb->fb_stack[0].sectorconfig.trackencoding=IBMFORMAT_DD;
			//fb->fb_stack[0].sectorconfig.;

		}
		else
		{
			floppycontext->hxc_printf(MSG_ERROR,"hxcfe_init_floppy : malloc error");
		}
	}
	return fb;
}

int hxcfe_getNumberOfTrack(HXCFLOPPYEMULATOR* floppycontext,FLOPPY *fp)
{
	return fp->floppyNumberOfTrack;
}

int hxcfe_getNumberOfSide(HXCFLOPPYEMULATOR* floppycontext,FLOPPY *fp)
{
	return fp->floppyNumberOfSide;
}


int hxcfe_pushTrack (FBuilder* fb,unsigned int rpm,int number,int side,int type)
{
	if(fb->fb_stack_pointer<(STACK_SIZE-1))
	{
		fb->fb_stack_pointer++;
		memcpy(&fb->fb_stack[fb->fb_stack_pointer],&fb->fb_stack[fb->fb_stack_pointer-1],sizeof(fb_track_state));

		fb->fb_stack[fb->fb_stack_pointer].rpm=rpm;
		fb->fb_stack[fb->fb_stack_pointer].side_number=side;
		fb->fb_stack[fb->fb_stack_pointer].track_number=number;
		fb->fb_stack[fb->fb_stack_pointer].type=type;
		fb->fb_stack[fb->fb_stack_pointer].sectorconfig.trackencoding=type;
		
		fb->fb_stack[fb->fb_stack_pointer].numberofsector=0;
		
		fb->fb_stack[fb->fb_stack_pointer].sc_stack_pointer=0;
		
		return HXCFE_NOERROR;
	}
	
	return HXCFE_BADPARAMETER;
}

int hxcfe_setTrackBitrate(FBuilder* fb,int bitrate)
{
	fb->fb_stack[fb->fb_stack_pointer].bitrate=bitrate;
	fb->fb_stack[fb->fb_stack_pointer].sectorconfig.bitrate=fb->fb_stack[fb->fb_stack_pointer].bitrate;
	return HXCFE_NOERROR;
}

int hxcfe_setTrackInterleave(FBuilder* fb,int interleave)
{
	fb->fb_stack[fb->fb_stack_pointer].interleave=interleave;
	return HXCFE_NOERROR;
}

int hxcfe_setTrackSkew(FBuilder* fb,int skew)
{
	fb->fb_stack[fb->fb_stack_pointer].skew=skew;
	return HXCFE_NOERROR;
}

int hxcfe_setIndexPosition(FBuilder* fb,int position,int allowsector)
{
	fb->fb_stack[fb->fb_stack_pointer].indexpos=position;
	return HXCFE_NOERROR;
}

int hxcfe_setIndexLength(FBuilder* fb,int length)
{
	fb->fb_stack[fb->fb_stack_pointer].indexlen=length;
	return HXCFE_NOERROR;
}


int hxcfe_addSector(FBuilder* fb,int sectornumber,int side,int track,unsigned char * buffer,int size)
{
	if(fb->fb_stack[fb->fb_stack_pointer].numberofsector<0x400)
	{
		memcpy(
			&fb->fb_stack[fb->fb_stack_pointer].sectortab[fb->fb_stack[fb->fb_stack_pointer].numberofsector],
			&fb->fb_stack[fb->fb_stack_pointer].sectorconfig,
			sizeof(SECTORCONFIG));

		fb->fb_stack[fb->fb_stack_pointer].sectortab[fb->fb_stack[fb->fb_stack_pointer].numberofsector].head=side;
		fb->fb_stack[fb->fb_stack_pointer].sectortab[fb->fb_stack[fb->fb_stack_pointer].numberofsector].cylinder=track;
		fb->fb_stack[fb->fb_stack_pointer].sectortab[fb->fb_stack[fb->fb_stack_pointer].numberofsector].sector=sectornumber;
		fb->fb_stack[fb->fb_stack_pointer].sectortab[fb->fb_stack[fb->fb_stack_pointer].numberofsector].sectorsize=size;

		if(buffer)
		{
			fb->fb_stack[fb->fb_stack_pointer].sectortab[fb->fb_stack[fb->fb_stack_pointer].numberofsector].input_data=malloc(size);
			memcpy(fb->fb_stack[fb->fb_stack_pointer].sectortab[fb->fb_stack[fb->fb_stack_pointer].numberofsector].input_data,buffer,size);
		}
		else
			fb->fb_stack[fb->fb_stack_pointer].sectortab[fb->fb_stack[fb->fb_stack_pointer].numberofsector].input_data=0;

		fb->fb_stack[fb->fb_stack_pointer].numberofsector++;

		return HXCFE_NOERROR;
	}
	return HXCFE_BADPARAMETER;
}

int hxcfe_setSectorSizeID(FBuilder* fb,unsigned char sectorsizeid)
{
	fb->fb_stack[fb->fb_stack_pointer].sectorconfig.use_alternate_sector_size_id=0xFF;
	fb->fb_stack[fb->fb_stack_pointer].sectorconfig.alternate_sector_size_id=sectorsizeid;
	return HXCFE_NOERROR;
}

int hxcfe_setSectorBitrate(FBuilder* fb,int bitrate)
{
	fb->fb_stack[fb->fb_stack_pointer].sectorconfig.bitrate=bitrate;
	return HXCFE_NOERROR;
}

int hxcfe_setSectorFill(FBuilder* fb,unsigned char fill)
{
	fb->fb_stack[fb->fb_stack_pointer].sectorconfig.fill_byte=fill;	
	return HXCFE_NOERROR;
}

int hxcfe_setSectorGap3(FBuilder* fb,unsigned char Gap3)
{
	fb->fb_stack[fb->fb_stack_pointer].sectorconfig.gap3=Gap3;
	return HXCFE_NOERROR;
}

int hxcfe_setSectorEncoding(FBuilder* fb,int encoding)
{
	fb->fb_stack[fb->fb_stack_pointer].sectorconfig.trackencoding=encoding;
	return HXCFE_NOERROR;
}

int hxcfe_setSectorDataCRC(FBuilder* fb,unsigned short crc)
{
	fb->fb_stack[fb->fb_stack_pointer].sectorconfig.use_alternate_data_crc=0xFF;
	fb->fb_stack[fb->fb_stack_pointer].sectorconfig.data_crc=crc;
	return HXCFE_NOERROR;
}

int hxcfe_setSectorHeaderCRC(FBuilder* fb,unsigned short crc)
{
	fb->fb_stack[fb->fb_stack_pointer].sectorconfig.use_alternate_header_crc=0xFF;
	fb->fb_stack[fb->fb_stack_pointer].sectorconfig.header_crc=crc;
	return HXCFE_NOERROR;
}

int hxcfe_setSectorDataMark(FBuilder* fb,unsigned char datamark)
{
	fb->fb_stack[fb->fb_stack_pointer].sectorconfig.use_alternate_datamark=0xFF;
	fb->fb_stack[fb->fb_stack_pointer].sectorconfig.alternate_datamark=datamark;
	return HXCFE_NOERROR;
}

int hxcfe_popTrack (FBuilder* fb)
{
	CYLINDER* currentcylinder;
	fb_track_state * current_fb_track_state;
	int i;
	unsigned long sui_flag;

	if(fb->fb_stack_pointer)
	{
		current_fb_track_state=&fb->fb_stack[fb->fb_stack_pointer];

		if(!fb->floppydisk->tracks[current_fb_track_state->track_number])
			fb->floppydisk->tracks[current_fb_track_state->track_number]=allocCylinderEntry(current_fb_track_state->rpm,2);
		
		currentcylinder=fb->floppydisk->tracks[current_fb_track_state->track_number];
		sui_flag=0;
	
		if(!current_fb_track_state->sectorunderindex)
				sui_flag=NO_SECTOR_UNDER_INDEX;
		
		currentcylinder->sides[current_fb_track_state->side_number]=tg_generateTrackEx((unsigned short)current_fb_track_state->numberofsector,
																						current_fb_track_state->sectortab,
																						current_fb_track_state->interleave,
																						current_fb_track_state->skew,
																						current_fb_track_state->bitrate,
																						current_fb_track_state->rpm,
																						current_fb_track_state->type,
																						current_fb_track_state->indexlen|sui_flag,current_fb_track_state->indexpos);

		for(i=0;i<current_fb_track_state->numberofsector;i++)
		{
			if(current_fb_track_state->sectortab[i].input_data)
				free(current_fb_track_state->sectortab[i].input_data);
		}

		fb->fb_stack_pointer--;
		return HXCFE_NOERROR;
	}

	return HXCFE_NOERROR;

}


FLOPPY* hxcfe_getFloppy(FBuilder* fb)
{
	int bitrate,trackencoding;
	int i,j;
	FLOPPY *f;
	
	f=0;

	if(fb->floppydisk->floppyNumberOfTrack && fb->floppydisk->floppyNumberOfSide)
	{
		bitrate=fb->floppydisk->tracks[0]->sides[0]->bitrate;
		trackencoding=fb->floppydisk->tracks[0]->sides[0]->track_encoding;
		for(j=0;j<fb->floppydisk->floppyNumberOfTrack;j++)
		{
			for(i=0;i<fb->floppydisk->tracks[j]->number_of_side;i++)
			{
				if(fb->floppydisk->tracks[j]->sides[i])
				{
					if(bitrate!=fb->floppydisk->tracks[j]->sides[i]->bitrate)
					{
						bitrate=-1;
					}
				}
				else
				{
					if(i==1)
					{
						fb->floppydisk->tracks[j]->number_of_side--;
					}
					else
					{
						fb->floppydisk->tracks[j]->sides[i]=tg_generateTrack(0,
																			0,
																			0,
																			0,
																			0,
																			0,
																			1,
																			1,
																			bitrate,
																			fb->floppydisk->tracks[j]->floppyRPM,
																			(unsigned char)trackencoding,
																			255,
																			fb->fb_stack[0].indexlen,
																			fb->fb_stack[0].indexpos);
					}
				}

			}
		}
		fb->floppydisk->floppyBitRate=bitrate;

		f=fb->floppydisk;
		free(fb->fb_stack);
		free(fb);
	}

	return f;
}
