/*
//
// Copyright (C) 2006, 2007, 2008, 2009, 2010, 2011 Jean-François DEL NERO
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
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


#include "hxc_floppy_emulator.h"
#include "floppy_loader.h"
#include "floppy_utils.h"

#include "loaders_list.h"

#include "version.h"
#include "licensetxt.h"

int dummy_output(int MSGTYPE,char * chaine, ...)
{
	if(MSGTYPE!=MSG_DEBUG)
	{
	
	}
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

int hxcfe_set_outputfunc(HXCFLOPPYEMULATOR* floppycontext,HXCPRINTF_FUNCTION hxc_printf)
{	
	floppycontext->hxc_printf=hxc_printf;

	return HXCFE_NOERROR;
}

int hxcfe_getversion(HXCFLOPPYEMULATOR* floppycontext,char * version,unsigned int * size1,char *copyright,unsigned int * size2)
{	
	if(version && size1)
	{
		if(*size1>= strlen(STR_FILE_VERSION2))
		{
			sprintf(version,STR_FILE_VERSION2);
		}
		else
		{
			*size1=strlen(STR_FILE_VERSION2);
			return HXCFE_BADPARAMETER;
		}
	}

	if(copyright && size2)
	{
		if(*size2>= strlen(licensetxt))
		{
			sprintf(copyright,licensetxt);
		}
		else
		{
			*size2=strlen(licensetxt);
			return HXCFE_BADPARAMETER;
		}
	}

	return HXCFE_NOERROR;
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

int hxcfe_select_container(HXCFLOPPYEMULATOR* floppycontext,char * container)
{
	sprintf(floppycontext->CONTAINERTYPE,container);

	return HXCFE_NOERROR;
}

int hxcfe_getcontainerid(HXCFLOPPYEMULATOR* floppycontext,int index,char * id,char * desc)
{
	int i;

	i=0;
	while( (staticplugins[i]!=(GETPLUGININFOS)-1) && (i<index))
	{
		i++;
	}

	if((staticplugins[i]!=(GETPLUGININFOS)-1) && i==index)
	{
		if(id)
			staticplugins[i](floppycontext,GETPLUGINID,id);

		if(desc)
			staticplugins[i](floppycontext,GETDESCRIPTION,desc);

		return HXCFE_NOERROR;
	}

	return HXCFE_BADPARAMETER;
}

FLOPPY * hxcfe_floppy_load(HXCFLOPPYEMULATOR* floppycontext,char* imgname,int * err_ret)
{
	int i;
	int ret;
	char plugin_id[16];
	FLOPPY * newfloppy;
	plugins_ptr func_ptr;

	floppycontext->hxc_printf(MSG_INFO_0,"Loading %s",imgname);

	newfloppy=0;

	if(!strncmp(floppycontext->CONTAINERTYPE,"AUTOSELECT",16))
	{
		i=0;
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
						floppycontext->hxc_printf(MSG_DEBUG,"libIsValidDiskFile n%d return %d",i,ret);
					}
				}
			}

			i++;
		}while(staticplugins[i]!=(GETPLUGININFOS)-1);
		
		floppycontext->hxc_printf(MSG_ERROR,"no loader support the file %s !",imgname);
		
		ret=HXCFE_UNSUPPORTEDFILE;

		if(err_ret) *err_ret=ret;
		return newfloppy;
	}
	else
	{
		i=0;
		do
		{
			if(staticplugins[i])
			{
				ret=staticplugins[i](floppycontext,GETPLUGINID,&plugin_id);
				if(!strcmp(floppycontext->CONTAINERTYPE,plugin_id))
				{
					ret=staticplugins[i](floppycontext,GETFUNCPTR,&func_ptr);
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
							floppycontext->hxc_printf(MSG_ERROR,"%s refuse the file %s (%d)!",plugin_id,imgname,ret);
							if(err_ret) *err_ret=ret;
							return newfloppy;
						}
					}
				}
			}

			i++;
		}while(staticplugins[i]!=(GETPLUGININFOS)-1);
		
		floppycontext->hxc_printf(MSG_ERROR,"Plugin %s not found !",floppycontext->CONTAINERTYPE);

		ret=HXCFE_BADPARAMETER;
		if(err_ret) *err_ret=ret;
		return newfloppy;
	}
}


int hxcfe_floppy_unload(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * floppydisk)
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

int hxcfe_floppy_export(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * newfloppy,char* imgname)
{
	int i,ret;
	char plugin_id[16];
	plugins_ptr func_ptr;

	i=0;
	do
	{
		if(staticplugins[i])
		{
			ret=staticplugins[i](floppycontext,GETPLUGINID,&plugin_id);
			if(!strcmp(floppycontext->CONTAINERTYPE,plugin_id))
			{
				ret=staticplugins[i](floppycontext,GETFUNCPTR,&func_ptr);
				if(ret==HXCFE_NOERROR)
				{
					if(func_ptr.libWrite_DiskFile)
					{
						ret=func_ptr.libWrite_DiskFile(floppycontext,newfloppy,imgname,0);
						return ret;
					}
					else
					{
						floppycontext->hxc_printf(MSG_ERROR,"No export support in %s!",plugin_id);
						return HXCFE_UNSUPPORTEDFILE;
					}
				}
				else
				{

				}
			}
		}

		i++;
	}while(staticplugins[i]!=(GETPLUGININFOS)-1);
		
	floppycontext->hxc_printf(MSG_ERROR,"Plugin %s not found !",floppycontext->CONTAINERTYPE);
	return HXCFE_BADPARAMETER;
}

////////////////////////////////////////////////////////////////////////

int hxcfe_floppy_getset_params(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * newfloppy,unsigned char dir,unsigned short param,void * value)
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

int libGetPluginInfo(HXCFLOPPYEMULATOR* floppycontext,unsigned long infotype,void * returnvalue,const char * pluginid,const char * plugindesc,plugins_ptr * pluginfunc,const char * fileext)
{
	if(returnvalue)
	{
		switch(infotype)
		{	
			case GETPLUGINID:
				memcpy(returnvalue,pluginid,strlen(pluginid)+1);
				break;
			
			case GETDESCRIPTION:
				memcpy(returnvalue,plugindesc,strlen(plugindesc)+1);
				break;
			
			case GETFUNCPTR:
				memcpy(returnvalue,pluginfunc,sizeof(plugins_ptr));
				break;

			case GETEXTENSION:
				memcpy(returnvalue,fileext,strlen(fileext)+1);
				break;
				
			default:
				return HXCFE_BADPARAMETER;
				break;
		}
		
		return HXCFE_NOERROR;
	}
	return HXCFE_BADPARAMETER;
}

FBuilder* hxcfe_init_floppy(HXCFLOPPYEMULATOR* floppycontext,int nb_of_track,int nb_of_side)
{
	FBuilder* fb;

	fb=0;

	floppycontext->hxc_printf(MSG_DEBUG,"hxcfe_init_floppy : floppy builder init");

	if(nb_of_track<=256 && ((nb_of_side>=1) && (nb_of_side<=2) ) )
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
			fb->fb_stack[fb->fb_stack_pointer].sectortab[fb->fb_stack[fb->fb_stack_pointer].numberofsector].input_data=buffer;
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

	if(fb->fb_stack_pointer)
	{
		current_fb_track_state=&fb->fb_stack[fb->fb_stack_pointer];

		if(!fb->floppydisk->tracks[current_fb_track_state->track_number])
			fb->floppydisk->tracks[current_fb_track_state->track_number]=allocCylinderEntry(current_fb_track_state->rpm,2);
		
		currentcylinder=fb->floppydisk->tracks[current_fb_track_state->track_number];
						
		//currentcylinder->sides[i]=tg_generatetrack(trackdata,sectorsize,floppydisk->floppySectorPerTrack,(unsigned char)j,(unsigned char)i,1,interleave,(unsigned char)(((j<<1)|(i&1))*skew),floppydisk->floppyBitRate,currentcylinder->floppyRPM,trackformat,gap3len,2500,-2500);
		currentcylinder->sides[current_fb_track_state->side_number]=tg_generatetrackEx((unsigned short)current_fb_track_state->numberofsector,
																						current_fb_track_state->sectortab,
																						current_fb_track_state->interleave,
																						current_fb_track_state->skew,
																						current_fb_track_state->bitrate,
																						current_fb_track_state->rpm,
																						current_fb_track_state->type,
																						2500/* | NO_SECTOR_UNDER_INDEX*/,-2500);
		fb->fb_stack_pointer--;
		return HXCFE_NOERROR;
	}

	return HXCFE_NOERROR;

}


FLOPPY* hxcfe_get_floppy(FBuilder* fb)
{
	FLOPPY *f;
	f=fb->floppydisk;
	free(fb->fb_stack);
	free(fb);
	return f;
}
