/*
//
// Copyright (C) 2006-2014 Jean-François DEL NERO
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

#include "types.h"

#include "internal_libhxcfe.h"
#include "tracks/track_generator.h"
#include "floppy_builder.h"
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

int dummy_progress(unsigned int current,unsigned int total, void * user)
{
    return 0;
}

////////////////////////////////////////////////////////////////////////

HXCFE* hxcfe_init(void)
{
	HXCFE* hxcfe;

	hxcfe=malloc(sizeof(HXCFE));
	if(hxcfe)
	{
		memset(hxcfe,0,sizeof(HXCFE));

		hxcfe->hxc_printf=&dummy_output;
		hxcfe->hxc_settrackpos=&dummy_trackpos;

		hxcfe->hxc_printf(MSG_INFO_0,"Starting HxCFloppyEmulator...");

		sprintf(hxcfe->CONTAINERTYPE,"AUTOSELECT");
	}
	return hxcfe;
}

int32_t hxcfe_setOutputFunc( HXCFE* floppycontext, HXCFE_PRINTF_FUNC hxc_printf )
{
	floppycontext->hxc_printf=hxc_printf;

	return HXCFE_NOERROR;
}

const char * hxcfe_getVersion(HXCFE* floppycontext)
{
	return STR_FILE_VERSION2;
}

const char * hxcfe_getLicense(HXCFE* floppycontext)
{
	return (char*)licensetxt;
}

void hxcfe_deinit(HXCFE* hxcfe)
{
	if(hxcfe)
	{
		hxcfe->hxc_printf(MSG_INFO_0,"Stopping HxCFloppyEmulator...");

		free(hxcfe);
	}
}

////////////////////////////////////////////////////////////////////////

static int32_t hxcfe_checkLoaderID(HXCFE_IMGLDR * imgldr_ctx,int32_t moduleID)
{
	int i;

	if( moduleID >= 0 )
	{
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
	else
	{
		return HXCFE_BADPARAMETER;
	}
}

int32_t hxcfe_imgGetNumberOfLoader( HXCFE_IMGLDR * imgldr_ctx )
{
	int i;

	i=0;
	do
	{
		i++;
	}while((staticplugins[i]!=(GETPLUGININFOS)-1));

	return i;
}

int32_t hxcfe_imgGetLoaderID( HXCFE_IMGLDR * imgldr_ctx, char * container )
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
			ret=staticplugins[i](imgldr_ctx,GETPLUGINID,&plugin_id);
			if((ret==HXCFE_NOERROR)  && !strcmp(container,plugin_id))
			{
				return i;
			}
		}

		i++;
	}while(staticplugins[i]!=(GETPLUGININFOS)-1);

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"hxcfe_searchContainer : Plugin %s not found !",container);

	return -1;
}

int32_t hxcfe_imgGetLoaderAccess( HXCFE_IMGLDR * imgldr_ctx, int32_t moduleID )
{
	int ret;
	plugins_ptr func_ptr;

	ret = 0;

	if(hxcfe_checkLoaderID(imgldr_ctx,moduleID)==HXCFE_NOERROR)
	{
		ret=staticplugins[moduleID](imgldr_ctx,GETFUNCPTR,&func_ptr);
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
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Bad module ID : %x !",moduleID);
	}

	return ret;
}

const char* hxcfe_imgGetLoaderDesc( HXCFE_IMGLDR * imgldr_ctx, int32_t moduleID )
{
	int ret;
	plugins_ptr func_ptr;
	char * desc;

	if(hxcfe_checkLoaderID(imgldr_ctx,moduleID)==HXCFE_NOERROR)
	{
		ret=staticplugins[moduleID](imgldr_ctx,GETFUNCPTR,&func_ptr);
		if(ret==HXCFE_NOERROR)
		{
			staticplugins[moduleID](imgldr_ctx,GETDESCRIPTION,&desc);
			return desc;
		}
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Bad module ID : %x !",moduleID);
	}

	return 0;
}

const char* hxcfe_imgGetLoaderName( HXCFE_IMGLDR * imgldr_ctx, int32_t moduleID )
{
	int ret;
	plugins_ptr func_ptr;
	char * desc;

	if(hxcfe_checkLoaderID(imgldr_ctx,moduleID)==HXCFE_NOERROR)
	{
		ret=staticplugins[moduleID](imgldr_ctx,GETFUNCPTR,&func_ptr);
		if(ret==HXCFE_NOERROR)
		{
			staticplugins[moduleID](imgldr_ctx,GETPLUGINID,&desc);
			return desc;
		}
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Bad module ID : %x !",moduleID);
	}

	return 0;
}

const char* hxcfe_imgGetLoaderExt( HXCFE_IMGLDR * imgldr_ctx, int32_t moduleID )
{
	int ret;
	plugins_ptr func_ptr;
	char * desc;

	if(hxcfe_checkLoaderID(imgldr_ctx,moduleID)==HXCFE_NOERROR)
	{
		ret=staticplugins[moduleID](imgldr_ctx,GETFUNCPTR,&func_ptr);
		if(ret==HXCFE_NOERROR)
		{
			staticplugins[moduleID](imgldr_ctx,GETEXTENSION,&desc);
			return desc;
		}
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Bad module ID : %x !",moduleID);
	}

	return 0;
}

int32_t hxcfe_imgAutoSetectLoader( HXCFE_IMGLDR * imgldr_ctx, char* imgname, int32_t moduleID )
{
	int i;
	int ret;
	plugins_ptr func_ptr;
	HXCFE* hxcfe;

	ret = HXCFE_BADPARAMETER;

	if( imgldr_ctx )
	{
		hxcfe = imgldr_ctx->hxcfe;

		hxcfe->hxc_printf(MSG_INFO_0,"Checking %s",imgname);

		i = moduleID;
		do
		{
			if(staticplugins[i])
			{
				ret = staticplugins[i](imgldr_ctx,GETFUNCPTR,&func_ptr);
				if( ret == HXCFE_NOERROR )
				{
					if(func_ptr.libIsValidDiskFile)
					{
						ret = func_ptr.libIsValidDiskFile(imgldr_ctx,imgname);
						if(ret == HXCFE_VALIDFILE)
						{
							hxcfe->hxc_printf(MSG_INFO_0,"File loader found : %s (%s)",hxcfe_imgGetLoaderName(imgldr_ctx,i),hxcfe_imgGetLoaderDesc(imgldr_ctx,i));
							return i;
						}
						else
						{
							hxcfe->hxc_printf(MSG_DEBUG,"libIsValidDiskFile n%d return %d",i,ret);
						}
					}
				}
			}

			i++;
		}while( staticplugins[i] != (GETPLUGININFOS)-1 );

		hxcfe->hxc_printf(MSG_ERROR,"No loader support the file %s !",imgname);

		ret = HXCFE_UNSUPPORTEDFILE;

	}

	return ret;
}

int32_t hxcfe_imgSetProgressCallback( HXCFE_IMGLDR * imgldr_ctx, HXCFE_IMGLDRPROGRESSOUT_FUNC progress_func, void * userdata )
{
	if(imgldr_ctx)
	{
		if(progress_func)
		{
			imgldr_ctx->progress_userdata = userdata;
			imgldr_ctx->hxc_setprogress = progress_func;
		}
	}
	return 0;
}

int32_t hxcfe_imgCallProgressCallback( HXCFE_IMGLDR * imgldr_ctx, int32_t cur, int32_t max )
{
	if(imgldr_ctx)
	{
		if(imgldr_ctx->hxc_setprogress)
		{
			imgldr_ctx->hxc_setprogress(cur,max,imgldr_ctx->progress_userdata);
		}
	}
	return 0;
}


HXCFE_IMGLDR *   hxcfe_imgInitLoader(HXCFE * hxcfe)
{
	HXCFE_IMGLDR * imgldr_ctx;

	imgldr_ctx = malloc(sizeof(HXCFE_IMGLDR));
	if(imgldr_ctx)
	{
		memset(imgldr_ctx,0,sizeof(HXCFE_IMGLDR));
		imgldr_ctx->hxcfe = hxcfe;
		imgldr_ctx->hxc_setprogress=&dummy_progress;
	}

	return imgldr_ctx;
}

void hxcfe_imgDeInitLoader(HXCFE_IMGLDR * imgldr_ctx)
{
	if(imgldr_ctx)
	{
		free(imgldr_ctx);
	}

	return;
}

HXCFE_FLOPPY * hxcfe_imgLoadEx( HXCFE_IMGLDR * imgldr_ctx, char* imgname, int32_t moduleID, int32_t * err_ret, void * parameters )
{
	int ret;
	HXCFE* hxcfe;
	HXCFE_FLOPPY * newfloppy;
	plugins_ptr func_ptr;

	newfloppy = 0;

	if(imgldr_ctx)
	{
		hxcfe = imgldr_ctx->hxcfe;

		hxcfe->hxc_printf(MSG_INFO_0,"Loading %s",imgname);

		if( hxcfe_checkLoaderID(imgldr_ctx,moduleID) == HXCFE_NOERROR )
		{
			ret = staticplugins[moduleID](imgldr_ctx,GETFUNCPTR,&func_ptr);
			if( ret == HXCFE_NOERROR )
			{
				ret = func_ptr.libIsValidDiskFile(imgldr_ctx,imgname);
				if( ret == HXCFE_VALIDFILE )
				{
					newfloppy = malloc(sizeof(HXCFE_FLOPPY));
					if(newfloppy)
					{
						memset(newfloppy,0,sizeof(HXCFE_FLOPPY));
						hxcfe->hxc_printf(MSG_INFO_0,"file loader found!");
						ret = func_ptr.libLoad_DiskFile(imgldr_ctx,newfloppy,imgname,parameters);
						if(ret != HXCFE_NOERROR)
						{
							free(newfloppy);
							newfloppy = 0;
						}

						if(err_ret)
							*err_ret = ret;
					}
					return newfloppy;
				}
				else
				{
					hxcfe->hxc_printf(MSG_ERROR,"%s refuse the file %s (%d)!",hxcfe_imgGetLoaderName(imgldr_ctx,moduleID),imgname,ret);

					if(err_ret)
						*err_ret=ret;

					return newfloppy;
				}
			}
		}

		hxcfe->hxc_printf(MSG_ERROR,"Bad plugin ID : 0x%x",moduleID);

		ret = HXCFE_BADPARAMETER;
		if(err_ret)
			*err_ret = ret;

	}
	return newfloppy;

}

HXCFE_FLOPPY * hxcfe_imgLoad(HXCFE_IMGLDR * imgldr_ctx, char* imgname, int32_t moduleID, int32_t * err_ret )
{
	return hxcfe_imgLoadEx(imgldr_ctx,imgname,moduleID,err_ret,0);
}

int32_t hxcfe_imgUnload( HXCFE_IMGLDR * imgldr_ctx, HXCFE_FLOPPY * floppydisk )
{
	int i,j;

	if(floppydisk)
	{
		for(j=0;j<floppydisk->floppyNumberOfTrack;j++)
		{
			for(i=0;i<floppydisk->floppyNumberOfSide;i++)
			{
				hxcfe_freeSide(imgldr_ctx->hxcfe,floppydisk->tracks[j]->sides[i]);
			}

			free(floppydisk->tracks[j]->sides);
			free(floppydisk->tracks[j]);

		}
		free(floppydisk->tracks);

		free(floppydisk);
	}
	return 0;
}

int32_t hxcfe_imgExport( HXCFE_IMGLDR * imgldr_ctx, HXCFE_FLOPPY * newfloppy, char* imgname, int32_t moduleID )
{
	int ret;
	plugins_ptr func_ptr;

	if(hxcfe_checkLoaderID(imgldr_ctx,moduleID)==HXCFE_NOERROR)
	{
		ret=staticplugins[moduleID](imgldr_ctx->hxcfe,GETFUNCPTR,&func_ptr);
		if(ret==HXCFE_NOERROR)
		{
			if(func_ptr.libWrite_DiskFile)
			{
				ret=func_ptr.libWrite_DiskFile(imgldr_ctx,newfloppy,imgname,0);
				return ret;
			}
			else
			{
				imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"No export support in %s!",hxcfe_imgGetLoaderName(imgldr_ctx,moduleID) );
				return HXCFE_UNSUPPORTEDFILE;
			}
		}
	}

	imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Bad plugin ID : 0x%x",moduleID);
	return HXCFE_BADPARAMETER;
}

int32_t hxcfe_floppyUnload( HXCFE* floppycontext, HXCFE_FLOPPY * floppydisk )
{
	int i,j;

	if(floppydisk)
	{
		if(floppydisk->tracks)
		{
			for(j=0;j<floppydisk->floppyNumberOfTrack;j++)
			{
				if(floppydisk->tracks[j])
				{
					if(floppydisk->tracks[j]->sides)
					{

						for(i=0;i<floppydisk->floppyNumberOfSide;i++)
						{
							hxcfe_freeSide(floppycontext,floppydisk->tracks[j]->sides[i]);
						}

						free(floppydisk->tracks[j]->sides);
					}

					free(floppydisk->tracks[j]);
				}
			}
			free(floppydisk->tracks);
		}
		free(floppydisk);
	}
	return 0;
}


HXCFE_FLOPPY * hxcfe_floppyDuplicate( HXCFE* floppycontext, HXCFE_FLOPPY * floppydisk )
{
	int i,j;
	HXCFE_FLOPPY * fp;
	int bufferlen;

	fp = 0;
	if(floppydisk)
	{
		fp = malloc(sizeof(HXCFE_FLOPPY));
		if(fp)
		{
			memcpy(fp,floppydisk,sizeof(HXCFE_FLOPPY));

			fp->tracks = (HXCFE_CYLINDER**)malloc(sizeof(HXCFE_CYLINDER*)*fp->floppyNumberOfTrack);
			if(fp->tracks)
			{
				for(j=0;j<fp->floppyNumberOfTrack;j++)
				{
					fp->tracks[j] = (HXCFE_CYLINDER*)malloc(sizeof(HXCFE_CYLINDER));
					if(fp->tracks[j])
					{
						memcpy(fp->tracks[j],floppydisk->tracks[j],sizeof(HXCFE_CYLINDER));

						fp->tracks[j]->sides = (HXCFE_SIDE**)malloc(sizeof(HXCFE_SIDE*)*fp->tracks[j]->number_of_side);
						if(fp->tracks[j]->sides)
						{
							memset(fp->tracks[j]->sides,0,sizeof(HXCFE_SIDE*)*fp->tracks[j]->number_of_side);

							for(i=0;i<fp->tracks[j]->number_of_side;i++)
							{
								fp->tracks[j]->sides[i] = (HXCFE_SIDE*)malloc(sizeof(HXCFE_SIDE));
								if(fp->tracks[j]->sides[i] && floppydisk->tracks[j]->sides[i])
								{
									memcpy(fp->tracks[j]->sides[i],floppydisk->tracks[j]->sides[i],sizeof(HXCFE_SIDE));

									bufferlen = fp->tracks[j]->sides[i]->tracklen / 8;
									if(fp->tracks[j]->sides[i]->tracklen&7)
										bufferlen++;

									if(fp->tracks[j]->sides[i]->databuffer)
									{
										fp->tracks[j]->sides[i]->databuffer = malloc(bufferlen);
										if(fp->tracks[j]->sides[i]->databuffer)
											memcpy(fp->tracks[j]->sides[i]->databuffer,floppydisk->tracks[j]->sides[i]->databuffer,bufferlen);
									}

									if(fp->tracks[j]->sides[i]->flakybitsbuffer)
									{
										fp->tracks[j]->sides[i]->flakybitsbuffer = malloc(bufferlen);
										if(fp->tracks[j]->sides[i]->flakybitsbuffer)
											memcpy(fp->tracks[j]->sides[i]->flakybitsbuffer,floppydisk->tracks[j]->sides[i]->flakybitsbuffer,bufferlen);
									}

									if(fp->tracks[j]->sides[i]->indexbuffer)
									{
										fp->tracks[j]->sides[i]->indexbuffer = malloc(bufferlen);
										if(fp->tracks[j]->sides[i]->indexbuffer)
											memcpy(fp->tracks[j]->sides[i]->indexbuffer,floppydisk->tracks[j]->sides[i]->indexbuffer,bufferlen);
									}

									if(fp->tracks[j]->sides[i]->timingbuffer)
									{
										fp->tracks[j]->sides[i]->timingbuffer = malloc(bufferlen * sizeof(uint32_t));
										if(fp->tracks[j]->sides[i]->timingbuffer)
											memcpy(fp->tracks[j]->sides[i]->timingbuffer,floppydisk->tracks[j]->sides[i]->timingbuffer,bufferlen * sizeof(uint32_t));
									}

									if(fp->tracks[j]->sides[i]->track_encoding_buffer)
									{
										fp->tracks[j]->sides[i]->track_encoding_buffer = malloc(bufferlen);
										if(fp->tracks[j]->sides[i]->track_encoding_buffer)
											memcpy(fp->tracks[j]->sides[i]->track_encoding_buffer,floppydisk->tracks[j]->sides[i]->track_encoding_buffer,bufferlen);
									}
								}
							}
						}
					}
				}
			}
		}
	}

	return fp;
}

////////////////////////////////////////////////////////////////////////
int32_t hxcfe_floppyGetSetParams( HXCFE* floppycontext, HXCFE_FLOPPY * newfloppy, uint8_t dir, uint16_t param, void * value )
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

int32_t hxcfe_floppyGetInterfaceMode( HXCFE* floppycontext, HXCFE_FLOPPY * newfloppy )
{
	return newfloppy->floppyiftype;
}

int32_t hxcfe_floppySetInterfaceMode( HXCFE* floppycontext, HXCFE_FLOPPY * newfloppy, int32_t ifmode )
{

	if(hxcfe_getFloppyInterfaceModeName(floppycontext,ifmode))
	{
		newfloppy->floppyiftype=ifmode;
		return HXCFE_NOERROR;
	}

	return HXCFE_BADPARAMETER;
}

int32_t hxcfe_floppyGetDoubleStep( HXCFE* floppycontext, HXCFE_FLOPPY * newfloppy )
{
	return newfloppy->double_step;
}

int32_t hxcfe_floppySetDoubleStep( HXCFE* floppycontext, HXCFE_FLOPPY * newfloppy, int32_t doublestep )
{
	newfloppy->double_step=doublestep;
	return HXCFE_NOERROR;
}

int libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue,const char * pluginid,const char * plugindesc,plugins_ptr * pluginfunc,const char * fileext)
{
	if(imgldr_ctx)
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
	}
	return HXCFE_BADPARAMETER;
}

HXCFE_FLPGEN* hxcfe_initFloppy( HXCFE* floppycontext, int32_t nb_of_track, int32_t nb_of_side )
{
	HXCFE_FLPGEN* fb_ctx;

	fb_ctx = 0;

	floppycontext->hxc_printf(MSG_DEBUG,"hxcfe_init_floppy : floppy builder init");

	if(( nb_of_track && (nb_of_track<=256)) && ((nb_of_side>=1) && (nb_of_side<=2) ) )
	{
		fb_ctx=(HXCFE_FLPGEN*)malloc(sizeof(HXCFE_FLPGEN));
		if(fb_ctx)
		{
			memset(fb_ctx,0,sizeof(HXCFE_FLPGEN));
			fb_ctx->floppydisk=(HXCFE_FLOPPY*)malloc(sizeof(HXCFE_FLOPPY));
			memset(fb_ctx->floppydisk,0,sizeof(HXCFE_FLOPPY));
			fb_ctx->floppydisk->floppyBitRate=DEFAULT_DD_BITRATE;
			fb_ctx->floppydisk->double_step=0;
			fb_ctx->floppydisk->floppyiftype=GENERIC_SHUGART_DD_FLOPPYMODE;
			fb_ctx->floppydisk->floppyNumberOfTrack=nb_of_track;
			fb_ctx->floppydisk->floppyNumberOfSide=nb_of_side;
			fb_ctx->floppydisk->floppySectorPerTrack=-1;

			fb_ctx->floppydisk->tracks=(HXCFE_CYLINDER**)malloc(sizeof(HXCFE_CYLINDER*)*fb_ctx->floppydisk->floppyNumberOfTrack);
			memset(fb_ctx->floppydisk->tracks,0,sizeof(HXCFE_CYLINDER*)*fb_ctx->floppydisk->floppyNumberOfTrack);

			fb_ctx->fb_stack_pointer=0;
			fb_ctx->fb_stack=(fb_track_state*)malloc(sizeof(fb_track_state)*STACK_SIZE);
			memset(fb_ctx->fb_stack,0,sizeof(fb_track_state)*STACK_SIZE);

			fb_ctx->fb_stack[0].interleave=1;
			fb_ctx->fb_stack[0].rpm=300;

			fb_ctx->fb_stack[0].sectors_size = 512;

			fb_ctx->fb_stack[0].indexlen=2500;
			fb_ctx->fb_stack[0].indexpos=-2500;
			fb_ctx->fb_stack[0].sectorunderindex=0;

			fb_ctx->fb_stack[0].numberofsector = 0;
			fb_ctx->fb_stack[0].numberofsector_min = 0;
			fb_ctx->fb_stack[0].start_sector_id = 1;

			fb_ctx->fb_stack[0].bitrate=250000;
			fb_ctx->fb_stack[0].sectorconfig.bitrate=fb_ctx->fb_stack[0].bitrate;
			fb_ctx->fb_stack[0].sectorconfig.fill_byte=0xF6;
			fb_ctx->fb_stack[0].sectorconfig.gap3=255;
			fb_ctx->fb_stack[0].sectorconfig.sectorsize=512;
			fb_ctx->fb_stack[0].sectorconfig.trackencoding=IBMFORMAT_DD;
			fb_ctx->fb_stack[0].type = IBMFORMAT_DD;
			//fb_ctx->fb_stack[0].sectorconfig.;

			fb_ctx->fb_stack[0].sc_stack[0].bitrate = 250000;
			fb_ctx->fb_stack[0].sc_stack[0].fill_byte = 0xF6;
			fb_ctx->fb_stack[0].sc_stack[0].gap3=255;
			fb_ctx->fb_stack[0].sc_stack[0].sectorsize = 512;
			fb_ctx->fb_stack[0].sc_stack[0].trackencoding=IBMFORMAT_DD;
		}
		else
		{
			floppycontext->hxc_printf(MSG_ERROR,"hxcfe_init_floppy : malloc error");
		}
	}
	return fb_ctx;
}

int32_t hxcfe_getNumberOfTrack( HXCFE* floppycontext, HXCFE_FLOPPY *fp )
{
	if(fp)
	{
		return fp->floppyNumberOfTrack;
	}

	return 0;
}

int32_t hxcfe_getNumberOfSide( HXCFE* floppycontext, HXCFE_FLOPPY *fp )
{
	if(fp)
	{
		return fp->floppyNumberOfSide;
	}

	return 0;
}

int32_t hxcfe_setNumberOfTrack ( HXCFE_FLPGEN* fb_ctx, int32_t numberoftrack )
{
	HXCFE_CYLINDER	**	tmptracks;
	int	tmpfloppyNumberOfTrack;

	tmptracks = 0;
	tmpfloppyNumberOfTrack = 0;

	if( fb_ctx->floppydisk->tracks )
	{
		tmptracks = fb_ctx->floppydisk->tracks;
		tmpfloppyNumberOfTrack = fb_ctx->floppydisk->floppyNumberOfTrack;
	}

	fb_ctx->floppydisk->floppyNumberOfTrack = numberoftrack;
	fb_ctx->floppydisk->tracks = (HXCFE_CYLINDER**)malloc(sizeof(HXCFE_CYLINDER*)*fb_ctx->floppydisk->floppyNumberOfTrack);
	if( !fb_ctx->floppydisk->tracks)
	{
		return HXCFE_INTERNALERROR;
	}
	memset(fb_ctx->floppydisk->tracks,0,sizeof(HXCFE_CYLINDER*)*fb_ctx->floppydisk->floppyNumberOfTrack);

	if(tmptracks)
	{
		if ( tmpfloppyNumberOfTrack < numberoftrack )
		{
			memcpy(fb_ctx->floppydisk->tracks,tmptracks,sizeof(HXCFE_CYLINDER*)*tmpfloppyNumberOfTrack);
		}
		else
		{
			memcpy(fb_ctx->floppydisk->tracks,tmptracks,sizeof(HXCFE_CYLINDER*)*numberoftrack);
		}

		free(tmptracks);
	}

	return HXCFE_NOERROR;
}

int32_t hxcfe_setNumberOfSide ( HXCFE_FLPGEN* fb_ctx, int32_t numberofside )
{
	fb_ctx->floppydisk->floppyNumberOfSide = numberofside;
	return HXCFE_NOERROR;
}

int32_t hxcfe_setNumberOfSector ( HXCFE_FLPGEN* fb_ctx, int32_t numberofsector )
{
	fb_ctx->fb_stack[fb_ctx->fb_stack_pointer].numberofsector_min = numberofsector;
	return HXCFE_NOERROR;
}

int32_t hxcfe_setTrackPreGap ( HXCFE_FLPGEN* fb_ctx, int32_t pregap )
{
	fb_ctx->fb_stack[fb_ctx->fb_stack_pointer].pregap = pregap;
	return HXCFE_NOERROR;
}

int32_t hxcfe_pushTrack ( HXCFE_FLPGEN* fb_ctx, uint32_t rpm, int32_t number, int32_t side, int32_t type )
{
	fb_track_state * cur_track;
	unsigned char * data_to_realloc;
	int i;

	if(fb_ctx->fb_stack_pointer<(STACK_SIZE-1))
	{
		fb_ctx->fb_stack_pointer++;

		cur_track = &fb_ctx->fb_stack[fb_ctx->fb_stack_pointer];

		memcpy(&fb_ctx->fb_stack[fb_ctx->fb_stack_pointer],&fb_ctx->fb_stack[fb_ctx->fb_stack_pointer-1],sizeof(fb_track_state));

		i = 0;
		while( i < fb_ctx->fb_stack[fb_ctx->fb_stack_pointer].numberofsector )
		{
			if ( cur_track->sectortab[i].input_data )
			{
				data_to_realloc = cur_track->sectortab[i].input_data;
				cur_track->sectortab[i].input_data = (unsigned char * )malloc(cur_track->sectortab[i].sectorsize);
				memcpy(cur_track->sectortab[i].input_data,data_to_realloc,cur_track->sectortab[i].sectorsize);
			}
			i++;
		}

		fb_ctx->fb_stack[fb_ctx->fb_stack_pointer].rpm = rpm;
		fb_ctx->fb_stack[fb_ctx->fb_stack_pointer].side_number = side;
		fb_ctx->fb_stack[fb_ctx->fb_stack_pointer].track_number = number;
		fb_ctx->fb_stack[fb_ctx->fb_stack_pointer].type = type;
		fb_ctx->fb_stack[fb_ctx->fb_stack_pointer].sc_stack[fb_ctx->fb_stack[fb_ctx->fb_stack_pointer].sc_stack_pointer].trackencoding = type;
		fb_ctx->fb_stack[fb_ctx->fb_stack_pointer].sectorconfig.trackencoding = type;
		fb_ctx->fb_stack[fb_ctx->fb_stack_pointer].numberofsector = 0;

		fb_ctx->fb_stack[fb_ctx->fb_stack_pointer].sc_stack_pointer = 0;

		return HXCFE_NOERROR;
	}

	return HXCFE_BADPARAMETER;
}

int32_t hxcfe_pushSector ( HXCFE_FLPGEN* fb_ctx )
{
	fb_track_state * cur_track;

	cur_track = &fb_ctx->fb_stack[fb_ctx->fb_stack_pointer];

	if( cur_track->sc_stack_pointer < (STACK_SIZE-1) )
	{
		cur_track->sc_stack_pointer++;
		memcpy(&cur_track->sc_stack[cur_track->sc_stack_pointer],&cur_track->sc_stack[cur_track->sc_stack_pointer-1],sizeof(HXCFE_SECTCFG));

		cur_track->sc_stack[cur_track->sc_stack_pointer].sector++;

		return HXCFE_NOERROR;
	}

	return HXCFE_INTERNALERROR;
}

int32_t hxcfe_popSector ( HXCFE_FLPGEN* fb_ctx )
{
	fb_track_state * cur_track;

	cur_track = &fb_ctx->fb_stack[fb_ctx->fb_stack_pointer];

	if( cur_track->numberofsector < (NUMBEROFSECTOR_MAX-1) )
	{
		memcpy(&cur_track->sectortab[cur_track->numberofsector],&cur_track->sc_stack[cur_track->sc_stack_pointer],sizeof(HXCFE_SECTCFG) );
		cur_track->numberofsector++;
		if(cur_track->numberofsector_min < cur_track->numberofsector )
			cur_track->numberofsector_min = cur_track->numberofsector;

		memset(&cur_track->sc_stack[cur_track->sc_stack_pointer],0,sizeof(HXCFE_SECTCFG));

		if( cur_track->sc_stack_pointer )
		{
			cur_track->sc_stack_pointer--;
		}

		return HXCFE_NOERROR;
	}

	return HXCFE_INTERNALERROR;
}

int32_t hxcfe_pushTrackPFS ( HXCFE_FLPGEN* fb_ctx, int32_t number, int32_t side )
{
	return hxcfe_pushTrack (fb_ctx,fb_ctx->fb_stack[fb_ctx->fb_stack_pointer].rpm,number,side,fb_ctx->fb_stack[fb_ctx->fb_stack_pointer].type);
}

int32_t hxcfe_setTrackBitrate ( HXCFE_FLPGEN* fb_ctx, int32_t bitrate )
{
	fb_track_state * cur_track;

	cur_track = &fb_ctx->fb_stack[fb_ctx->fb_stack_pointer];

	cur_track->bitrate = bitrate;
	cur_track->sectorconfig.bitrate = bitrate;
	cur_track->sc_stack[fb_ctx->fb_stack[fb_ctx->fb_stack_pointer].sc_stack_pointer].bitrate = bitrate;
	return HXCFE_NOERROR;
}

int32_t hxcfe_setTrackType( HXCFE_FLPGEN* fb_ctx, int32_t type )
{
	fb_track_state * cur_track;

	cur_track = &fb_ctx->fb_stack[fb_ctx->fb_stack_pointer];

	cur_track->type = type;
	cur_track->sectorconfig.trackencoding = type;
	cur_track->sc_stack[fb_ctx->fb_stack[fb_ctx->fb_stack_pointer].sc_stack_pointer].trackencoding = type;

	return HXCFE_NOERROR;

}

int32_t hxcfe_setTrackInterleave ( HXCFE_FLPGEN* fb_ctx, int32_t interleave )
{
	fb_ctx->fb_stack[fb_ctx->fb_stack_pointer].interleave = interleave;
	return HXCFE_NOERROR;
}

int32_t hxcfe_setSectorSize( HXCFE_FLPGEN* fb_ctx, int32_t size )
{
	fb_track_state * cur_track;

	cur_track = &fb_ctx->fb_stack[fb_ctx->fb_stack_pointer];

	fb_ctx->fb_stack[fb_ctx->fb_stack_pointer].sectors_size = size;
	cur_track->sc_stack[cur_track->sc_stack_pointer].sectorsize = size;

	return HXCFE_NOERROR;
}

int32_t hxcfe_setTrackSkew ( HXCFE_FLPGEN* fb_ctx, int32_t skew )
{
	fb_ctx->fb_stack[fb_ctx->fb_stack_pointer].skew = skew;
	return HXCFE_NOERROR;
}

int32_t hxcfe_setIndexPosition ( HXCFE_FLPGEN* fb_ctx, int32_t position, int32_t allowsector )
{
	fb_ctx->fb_stack[fb_ctx->fb_stack_pointer].indexpos = position;
	return HXCFE_NOERROR;
}

int32_t hxcfe_setIndexLength ( HXCFE_FLPGEN* fb_ctx, int32_t Length )
{
	fb_ctx->fb_stack[fb_ctx->fb_stack_pointer].indexlen = Length;
	return HXCFE_NOERROR;
}

int32_t hxcfe_setSectorData( HXCFE_FLPGEN* fb_ctx, uint8_t * buffer, int32_t size )
{
	fb_track_state * cur_track;
	HXCFE_SECTCFG * cur_sector;

	cur_track = &fb_ctx->fb_stack[fb_ctx->fb_stack_pointer];
	cur_sector = &cur_track->sc_stack[cur_track->sc_stack_pointer];

	if(cur_sector->input_data)
	{
		free(cur_sector->input_data);
		cur_sector->input_data = 0;
	}

	if(buffer)
	{
		cur_sector->input_data = malloc(size);
		memcpy(cur_sector->input_data,buffer,size);
	}

	cur_sector->sectorsize = size;

	return HXCFE_NOERROR;
}

int32_t hxcfe_addSector ( HXCFE_FLPGEN* fb_ctx, int32_t sectornumber, int32_t side, int32_t track, uint8_t * buffer, int32_t size )
{
	hxcfe_pushSector (fb_ctx);

	hxcfe_setSectorTrackID(fb_ctx,track);
	hxcfe_setSectorHeadID(fb_ctx,side);
	hxcfe_setSectorID(fb_ctx,sectornumber);

	hxcfe_setSectorData(fb_ctx,buffer,size);

	hxcfe_popSector (fb_ctx);

	return HXCFE_NOERROR;
}

int32_t hxcfe_addSectors( HXCFE_FLPGEN* fb_ctx, int32_t side, int32_t track, uint8_t * trackdata, int32_t buffersize, int32_t numberofsectors )
{
	int i,ret;
	int startsectorid,sectorsize;

	if(fb_ctx->fb_stack[fb_ctx->fb_stack_pointer].numberofsector<0x400)
	{
		startsectorid = fb_ctx->fb_stack[fb_ctx->fb_stack_pointer].start_sector_id;
		sectorsize = fb_ctx->fb_stack[fb_ctx->fb_stack_pointer].sectors_size;

		for(i = 0; i < numberofsectors ; i++)
		{
			if(trackdata)
				ret = hxcfe_addSector(fb_ctx,startsectorid + i,side,track,&trackdata[i*sectorsize],sectorsize);
			else
				ret = hxcfe_addSector(fb_ctx,startsectorid + i,side,track,0,sectorsize);

			if(ret != HXCFE_NOERROR)
			{
				return ret;
			}
		}

		return HXCFE_NOERROR;
	}

	return HXCFE_BADPARAMETER;
}

int32_t hxcfe_setSectorSizeID ( HXCFE_FLPGEN* fb_ctx, int32_t sectorsizeid )
{
	fb_track_state * cur_track;

	cur_track = &fb_ctx->fb_stack[fb_ctx->fb_stack_pointer];

	cur_track->sc_stack[cur_track->sc_stack_pointer].use_alternate_sector_size_id=0xFF;
	cur_track->sc_stack[cur_track->sc_stack_pointer].alternate_sector_size_id=sectorsizeid;

	return HXCFE_NOERROR;
}

int32_t hxcfe_setStartSectorID( HXCFE_FLPGEN* fb_ctx, int32_t startsectorid )
{
	fb_ctx->fb_stack[fb_ctx->fb_stack_pointer].start_sector_id = startsectorid;
	return HXCFE_NOERROR;
}

int32_t hxcfe_setSectorBitrate ( HXCFE_FLPGEN* fb_ctx, int32_t bitrate )
{
	fb_track_state * cur_track;

	cur_track = &fb_ctx->fb_stack[fb_ctx->fb_stack_pointer];
	cur_track->sc_stack[cur_track->sc_stack_pointer].bitrate=bitrate;

	return HXCFE_NOERROR;
}

int32_t hxcfe_setSectorFill ( HXCFE_FLPGEN* fb_ctx, int32_t fill )
{
	fb_track_state * cur_track;

	cur_track = &fb_ctx->fb_stack[fb_ctx->fb_stack_pointer];
	cur_track->sc_stack[cur_track->sc_stack_pointer].fill_byte = (unsigned char)fill;
	return HXCFE_NOERROR;
}

int32_t hxcfe_setSectorTrackID( HXCFE_FLPGEN* fb_ctx, int32_t track )
{
	fb_track_state * cur_track;

	cur_track = &fb_ctx->fb_stack[fb_ctx->fb_stack_pointer];
	cur_track->sc_stack[cur_track->sc_stack_pointer].cylinder = track;
	return HXCFE_NOERROR;
}

int32_t hxcfe_setSectorHeadID( HXCFE_FLPGEN* fb_ctx, int32_t head )
{
	fb_track_state * cur_track;

	cur_track = &fb_ctx->fb_stack[fb_ctx->fb_stack_pointer];
	cur_track->sc_stack[cur_track->sc_stack_pointer].head = head;
	return HXCFE_NOERROR;
}

int32_t hxcfe_setSectorID( HXCFE_FLPGEN* fb_ctx, int32_t id )
{
	fb_track_state * cur_track;

	cur_track = &fb_ctx->fb_stack[fb_ctx->fb_stack_pointer];
	cur_track->sc_stack[cur_track->sc_stack_pointer].sector = id;
	return HXCFE_NOERROR;
}

int32_t hxcfe_setSectorGap3 ( HXCFE_FLPGEN* fb_ctx, int32_t Gap3 )
{
	fb_track_state * cur_track;

	cur_track = &fb_ctx->fb_stack[fb_ctx->fb_stack_pointer];
	cur_track->sc_stack[cur_track->sc_stack_pointer].gap3=Gap3;
	return HXCFE_NOERROR;
}

int32_t hxcfe_setSectorEncoding ( HXCFE_FLPGEN* fb_ctx, int32_t encoding )
{
	fb_track_state * cur_track;

	cur_track = &fb_ctx->fb_stack[fb_ctx->fb_stack_pointer];
	cur_track->sc_stack[cur_track->sc_stack_pointer].trackencoding = encoding;
	return HXCFE_NOERROR;
}

int32_t hxcfe_setRPM( HXCFE_FLPGEN* fb_ctx, int32_t rpm )
{
	fb_ctx->fb_stack[fb_ctx->fb_stack_pointer].rpm = rpm;
	return HXCFE_NOERROR;
}

int32_t hxcfe_setSectorDataCRC ( HXCFE_FLPGEN* fb_ctx, uint32_t crc )
{
	fb_track_state * cur_track;

	cur_track = &fb_ctx->fb_stack[fb_ctx->fb_stack_pointer];
	cur_track->sc_stack[cur_track->sc_stack_pointer].use_alternate_data_crc=0xFF;
	cur_track->sc_stack[cur_track->sc_stack_pointer].data_crc=crc;
	return HXCFE_NOERROR;
}

int32_t hxcfe_setSectorHeaderCRC ( HXCFE_FLPGEN* fb_ctx, uint32_t crc )
{
	fb_track_state * cur_track;

	cur_track = &fb_ctx->fb_stack[fb_ctx->fb_stack_pointer];
	cur_track->sc_stack[cur_track->sc_stack_pointer].use_alternate_header_crc=0xFF;
	cur_track->sc_stack[cur_track->sc_stack_pointer].header_crc=crc;
	return HXCFE_NOERROR;
}

int32_t hxcfe_setSectorDataMark ( HXCFE_FLPGEN* fb_ctx, uint32_t datamark )
{
	fb_track_state * cur_track;

	cur_track = &fb_ctx->fb_stack[fb_ctx->fb_stack_pointer];
	cur_track->sc_stack[cur_track->sc_stack_pointer].use_alternate_datamark = 0xFF;
	cur_track->sc_stack[cur_track->sc_stack_pointer].alternate_datamark = datamark;

	return HXCFE_NOERROR;
}

int32_t hxcfe_popTrack ( HXCFE_FLPGEN* fb_ctx )
{
	HXCFE_CYLINDER* currentcylinder;
	fb_track_state * current_fb_track_state;
	int i;
	uint32_t sui_flag;

	if(fb_ctx->fb_stack_pointer)
	{
		current_fb_track_state=&fb_ctx->fb_stack[fb_ctx->fb_stack_pointer];

		if( current_fb_track_state->track_number < fb_ctx->floppydisk->floppyNumberOfTrack)
		{
			if(!fb_ctx->floppydisk->tracks[current_fb_track_state->track_number])
				fb_ctx->floppydisk->tracks[current_fb_track_state->track_number]=allocCylinderEntry(current_fb_track_state->rpm,2);

			currentcylinder=fb_ctx->floppydisk->tracks[current_fb_track_state->track_number];
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
																							current_fb_track_state->pregap,
																							current_fb_track_state->indexlen|sui_flag,current_fb_track_state->indexpos);

			for(i=0;i<current_fb_track_state->numberofsector;i++)
			{
				if(current_fb_track_state->sectortab[i].input_data)
					free(current_fb_track_state->sectortab[i].input_data);
			}

			fb_ctx->fb_stack_pointer--;

			return HXCFE_NOERROR;
		}
		else
		{
			for(i=0;i<current_fb_track_state->numberofsector;i++)
			{
				if(current_fb_track_state->sectortab[i].input_data)
					free(current_fb_track_state->sectortab[i].input_data);
			}

			fb_ctx->fb_stack_pointer--;

			return HXCFE_BADPARAMETER;
		}

	}

	return HXCFE_NOERROR;
}

int32_t hxcfe_getCurrentNumberOfTrack ( HXCFE_FLPGEN* fb_ctx )
{
	return fb_ctx->floppydisk->floppyNumberOfTrack;
}

int32_t hxcfe_getCurrentNumberOfSide ( HXCFE_FLPGEN* fb_ctx )
{
	return fb_ctx->floppydisk->floppyNumberOfSide;
}

int32_t hxcfe_getCurrentNumberOfSector ( HXCFE_FLPGEN* fb_ctx )
{
	return fb_ctx->fb_stack[fb_ctx->fb_stack_pointer].numberofsector_min;
}

int32_t hxcfe_getCurrentSectorSize( HXCFE_FLPGEN* fb_ctx )
{
	return fb_ctx->fb_stack[fb_ctx->fb_stack_pointer].sectors_size;
}

int32_t hxcfe_getCurrentTrackType ( HXCFE_FLPGEN* fb_ctx )
{
	return fb_ctx->fb_stack[fb_ctx->fb_stack_pointer].type;
}

int32_t hxcfe_getCurrentRPM ( HXCFE_FLPGEN* fb_ctx )
{
	return fb_ctx->fb_stack[fb_ctx->fb_stack_pointer].rpm;
}

int32_t hxcfe_getCurrentSkew ( HXCFE_FLPGEN* fb_ctx )
{
	return fb_ctx->fb_stack[fb_ctx->fb_stack_pointer].skew;
}

int32_t hxcfe_generateDisk( HXCFE_FLPGEN* fb_ctx, uint8_t * diskdata, int32_t buffersize )
{
	int i,j,ret;
	int numberofsector,sectorsize;
	int bufferoffset,type;
	unsigned int rpm;

	sectorsize = fb_ctx->fb_stack[fb_ctx->fb_stack_pointer].sectors_size;
	numberofsector = fb_ctx->fb_stack[fb_ctx->fb_stack_pointer].numberofsector_min;
	rpm = fb_ctx->fb_stack[fb_ctx->fb_stack_pointer].rpm;
	type = fb_ctx->fb_stack[fb_ctx->fb_stack_pointer].type;

	for(i = 0 ; i < fb_ctx->floppydisk->floppyNumberOfTrack ; i++ )
	{
		for(j = 0 ; j < fb_ctx->floppydisk->floppyNumberOfSide ; j++ )
		{
			bufferoffset = numberofsector * sectorsize * ((i<<1) + (j&1)) ;

			if(!fb_ctx->floppydisk->tracks[i] || !fb_ctx->floppydisk->tracks[i]->sides[j])
			{
				//if(!fb_ctx->floppydisk->tracks[i]->sides[j])
				{
					hxcfe_pushTrack (fb_ctx,rpm,i,j,type);

					if( (bufferoffset + (sectorsize * numberofsector) )  > buffersize )
					{
						ret = hxcfe_addSectors(fb_ctx,j,i,0,0,numberofsector);
					}
					else
					{
						ret = hxcfe_addSectors(fb_ctx,j,i,&diskdata[bufferoffset],(sectorsize * numberofsector),numberofsector);
					}

					hxcfe_popTrack (fb_ctx);
				}
			}
		}
	}

	return HXCFE_NOERROR;
}

HXCFE_FLOPPY* hxcfe_getFloppy ( HXCFE_FLPGEN* fb_ctx )
{
	int bitrate,trackencoding;
	int i,j;
	HXCFE_FLOPPY *f;

	f=0;

	for (j = 0; j < fb_ctx->floppydisk->floppyNumberOfTrack; j++)
	{
		for (i = 0; i < fb_ctx->floppydisk->floppyNumberOfSide; i++)
		{
			if(!fb_ctx->floppydisk->tracks[j])
			{
				hxcfe_pushTrack (fb_ctx,fb_ctx->fb_stack[0].rpm,j,i,fb_ctx->fb_stack[0].type);
				hxcfe_popTrack (fb_ctx);
			}
			else
			{
				if(!fb_ctx->floppydisk->tracks[j]->sides[i])
				{
					hxcfe_pushTrack (fb_ctx,fb_ctx->fb_stack[0].rpm,j,i,fb_ctx->fb_stack[0].type);
					hxcfe_popTrack (fb_ctx);
				}
			}
		}
	}

	if(fb_ctx->floppydisk->floppyNumberOfTrack && fb_ctx->floppydisk->floppyNumberOfSide)
	{
		bitrate = fb_ctx->floppydisk->tracks[0]->sides[0]->bitrate;
		trackencoding = fb_ctx->floppydisk->tracks[0]->sides[0]->track_encoding;

		for(j=0;j<fb_ctx->floppydisk->floppyNumberOfTrack;j++)
		{
			for(i=0;i<fb_ctx->floppydisk->tracks[j]->number_of_side;i++)
			{
				if(fb_ctx->floppydisk->tracks[j]->sides[i])
				{
					if(bitrate != fb_ctx->floppydisk->tracks[j]->sides[i]->bitrate)
					{
						bitrate=-1;
					}
				}
				else
				{
					if(i==1)
					{
						fb_ctx->floppydisk->tracks[j]->number_of_side--;
					}
					else
					{
						fb_ctx->floppydisk->tracks[j]->sides[i]=tg_generateTrack(0,
																			0,
																			0,
																			0,
																			0,
																			0,
																			1,
																			1,
																			bitrate,
																			fb_ctx->floppydisk->tracks[j]->floppyRPM,
																			(unsigned char)trackencoding,
																			255,
																			0,
																			fb_ctx->fb_stack[0].indexlen,
																			fb_ctx->fb_stack[0].indexpos);
					}
				}
			}
		}
		fb_ctx->floppydisk->floppyBitRate=bitrate;

		f=fb_ctx->floppydisk;
		free(fb_ctx->fb_stack);
		free(fb_ctx);
	}

	return f;
}

HXCFE_FLOPPY* hxcfe_sanityCheck(HXCFE* floppycontext,HXCFE_FLOPPY * floppydisk)
{
	int numberoftrack,truenumberoftrack;
	int numberofside,oldnumberofside,truenumberofside;
	int needanewpass,defaultbitrate,defaultrpm;
	int i;
	HXCFE_SIDE	**		tmpsides;
	HXCFE_SIDE * currentside;

	if(floppydisk)
	{
		// Check track parameter.
		truenumberoftrack = 0;

		if(floppydisk->floppyNumberOfTrack>256)
		{
			floppydisk->floppyNumberOfTrack=256;
			floppycontext->hxc_printf(MSG_WARNING,"Sanity Checker : More than 256 tracks?!? - fixed to 256. Please Check the Loader !");
		}

		if(!floppydisk->floppyNumberOfSide)
		{
			floppycontext->hxc_printf(MSG_WARNING,"Sanity Checker : 0 Side?!? - fixed to 1. Please Check the Loader !");
			floppydisk->floppyNumberOfSide = 1;
		}

		if(floppydisk->floppyNumberOfSide>2)
		{
			floppycontext->hxc_printf(MSG_WARNING,"Sanity Checker : more than 2 Sides (%d) ?!? - fixed to 2. Please Check the Loader !",floppydisk->floppyNumberOfSide);
			floppydisk->floppyNumberOfSide = 2;
		}

		if(floppydisk->floppyBitRate!=-1)
		{
			if(floppydisk->floppyBitRate > 1000000)
			{
				floppycontext->hxc_printf(MSG_WARNING,"Sanity Checker : Over 1MBit/s ?!? - fixed to 1MB/S. Please Check the Loader !");
				floppydisk->floppyBitRate = 1000000;
			}

			if(floppydisk->floppyBitRate < 25000)
			{
				floppycontext->hxc_printf(MSG_WARNING,"Sanity Checker : Under 25kBit/s ?!? - fixed to 250KB/S. Please Check the Loader !");
				floppydisk->floppyBitRate = 250000;
			}
			defaultbitrate = floppydisk->floppyBitRate;
		}
		else
		{
			defaultbitrate = 250000;
		}

		defaultrpm = 300;
		numberofside = floppydisk->floppyNumberOfSide;

		if(!floppydisk->tracks)
		{
			floppycontext->hxc_printf(MSG_WARNING,"Sanity Checker : No track allocated ? Please Check the Loader !");
			floppydisk->tracks=(HXCFE_CYLINDER**)malloc(sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);
			memset(floppydisk->tracks,0,sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);
		}

		numberoftrack = floppydisk->floppyNumberOfTrack;

		for( i = 0; i < numberoftrack ; i++ )
		{
			if( floppydisk->tracks[truenumberoftrack] )
			{
				truenumberoftrack++;
			}
		}

		if( truenumberoftrack != numberoftrack)
		{
			truenumberoftrack=0;
			while( truenumberoftrack<numberoftrack )
			{
				if(!floppydisk->tracks[truenumberoftrack] )
				{
					floppycontext->hxc_printf(MSG_WARNING,"Sanity Checker :  Track entry %d missing !",truenumberoftrack);
					floppydisk->tracks[truenumberoftrack] = allocCylinderEntry(0,floppydisk->floppyNumberOfSide);
				}
				truenumberoftrack++;
			}
		}

		do
		{
			needanewpass = 0;
			numberofside = floppydisk->floppyNumberOfSide;
			truenumberoftrack=0;
			while( truenumberoftrack<numberoftrack )
			{
				truenumberofside=0;
				while(truenumberofside<numberofside)
				{
					if(floppydisk->tracks[truenumberoftrack]->number_of_side != numberofside)
					{
						floppycontext->hxc_printf(MSG_WARNING,"Sanity Checker :  Bad number of side (%d should be %d) at Track entry %d",floppydisk->tracks[truenumberoftrack]->number_of_side,numberofside,truenumberoftrack);
						if(floppydisk->tracks[truenumberoftrack]->number_of_side < numberofside )
						{
							tmpsides = floppydisk->tracks[truenumberoftrack]->sides;
							oldnumberofside = floppydisk->tracks[truenumberoftrack]->number_of_side;
							// Realloc buffer
							floppydisk->tracks[truenumberoftrack]->number_of_side = numberofside;
							floppydisk->tracks[truenumberoftrack]->sides=(HXCFE_SIDE**)malloc(sizeof(HXCFE_SIDE*)*numberofside);
							memset(floppydisk->tracks[truenumberoftrack]->sides,0,sizeof(HXCFE_SIDE*)*numberofside);

							if(tmpsides && oldnumberofside)
								floppydisk->tracks[truenumberoftrack]->sides[0] = tmpsides[0];
						}
						else
						{
							if(floppydisk->tracks[truenumberoftrack]->number_of_side <= 2)
								floppydisk->floppyNumberOfSide = floppydisk->tracks[truenumberoftrack]->number_of_side;
							else
							{
								floppycontext->hxc_printf(MSG_WARNING,"Sanity Checker : track %d - more than 2 Sides (%d) ?!? - fixed to 2. Please Check the Loader !",truenumberoftrack,floppydisk->floppyNumberOfSide);
								floppydisk->tracks[truenumberoftrack]->number_of_side = 2;
							}

							needanewpass = 1;
						}
					}
					truenumberofside++;
				}
				truenumberoftrack++;
			}
		}while(needanewpass);

		numberoftrack = floppydisk->floppyNumberOfTrack;
		numberofside = floppydisk->floppyNumberOfSide;
		truenumberoftrack=0;
		while( truenumberoftrack<numberoftrack )
		{
			truenumberofside=0;
			while(truenumberofside<numberofside)
			{

				currentside=floppydisk->tracks[truenumberoftrack]->sides[truenumberofside];

				if(!currentside)
				{
					currentside=(HXCFE_SIDE*)malloc(sizeof(HXCFE_SIDE));
					memset(currentside,0,sizeof(HXCFE_SIDE));
				}

				floppydisk->tracks[truenumberoftrack]->sides[truenumberofside]=currentside;

				if(!currentside->databuffer)
				{
					floppycontext->hxc_printf(MSG_WARNING,"Sanity Checker :  Track %d/Side %d not allocated ?!?",truenumberoftrack,truenumberofside);
					if(truenumberofside)
					{
						if(floppydisk->tracks[truenumberoftrack]->sides[0]->tracklen)
						{
							currentside->tracklen = floppydisk->tracks[truenumberoftrack]->sides[0]->tracklen;
						}
						else
							currentside->tracklen = 12500*8;
					}
					else
						currentside->tracklen = 12500*8;

					currentside->databuffer=malloc(currentside->tracklen/8);
					memset(currentside->databuffer,0x01,currentside->tracklen/8);
					currentside->indexbuffer=malloc(currentside->tracklen/8);
					memset(currentside->indexbuffer,0x00,currentside->tracklen/8);
					if(floppydisk->floppyBitRate!=-1)
						currentside->bitrate=floppydisk->floppyBitRate;
					else
						currentside->bitrate=250000;
					fillindex(0,currentside,2500,1,0);

				}
				truenumberofside++;
			}
			truenumberoftrack++;
		}
	}
	return floppydisk;
}


fs_config fs_config_table[]=
{
	{"fatst",		"",FS_720KB_ATARI_FAT12,0},
	{"fatst902",	"",FS_902KB_ATARI_FAT12,0},
	{"fatst360",	"",FS_360KB_ATARI_FAT12,0},
	{"amigados",	"3\"5        880KB DSDD AmigaDOS",FS_880KB_AMIGADOS,1},

	{"fat160a",		"5\"25 & 8\" 160KB SSDD 300RPM FAT12",FS_5P25_300RPM_160KB_MSDOS_FAT12,0},
	{"fat160b",		"5\"25 & 8\" 160KB SSDD 360RPM FAT12",FS_5P25_360RPM_160KB_MSDOS_FAT12,0},

	{"fat180a",		"5\"25       180KB SSDD 300RPM FAT12",FS_5P25_300RPM_180KB_MSDOS_FAT12,0},
	{"fat180b",		"5\"25       180KB SSDD 360RPM FAT12",FS_5P25_360RPM_180KB_MSDOS_FAT12,0},

	{"fat320ssa",	"5\"25       320KB SSDD 300RPM FAT12",FS_5P25_SS_300RPM_320KB_MSDOS_FAT12,0},
	{"fat320ssb",	"5\"25       320KB SSDD 360RPM FAT12",FS_5P25_SS_360RPM_320KB_MSDOS_FAT12,0},

	{"fat320dsa",	"5\"25       320KB DSDD 300RPM FAT12",FS_5P25_DS_300RPM_320KB_MSDOS_FAT12,0},
	{"fat320dsb",	"5\"25       320KB DSDD 360RPM FAT12",FS_5P25_DS_360RPM_320KB_MSDOS_FAT12,0},

	{"fat360a",		"5\"25 & 8\" 360KB DSDD 300RPM FAT12",FS_5P25_DS_300RPM_360KB_MSDOS_FAT12,0},
	{"fat360b",		"5\"25 & 8\" 360KB DSDD 360RPM FAT12",FS_5P25_DS_360RPM_360KB_MSDOS_FAT12,0},

	{"fat640",		"3\"5        640KB DSDD FAT12",FS_3P5_DS_300RPM_640KB_MSDOS_FAT12,0},

	{"fat720",		"3\"5        720KB DSDD FAT12",FS_720KB_MSDOS_FAT12,0},

	{"fat1200",		"5\"25       1.2MB DSHD FAT12",FS_5P25_300RPM_1200KB_MSDOS_FAT12,0},

	{"fat1440",		"3\"5        1.44MB DSHD FAT12",FS_1_44MB_MSDOS_FAT12,0},
	{"fat1680",		"3\"5        1.68MB DSHD FAT12",FS_1_68MB_MSDOS_FAT12,0},
	{"fat2880",		"3\"5        2.88MB DSED FAT12",FS_2_88MB_MSDOS_FAT12,0},
	{"fat3381",		"3\"5        3.38MB DSHD FAT12",FS_3_38MB_MSDOS_FAT12,0},
	{"fatbigst",	"3\"5        3.42MB DSDD Atari FAT12",FS_3_42MB_ATARI_FAT12,0},

	{"fat5355",		"3\"5        5.35MB DSHD FAT12",FS_5_35MB_MSDOS_FAT12,0},
	{"fat5355b",	"3\"5        5.35MB DSHD FAT12",FS_5_35MB_B_MSDOS_FAT12,0},

	{"fat6789",		"3\"5        6.78MB DSHD FAT12",FS_6_78MB_MSDOS_FAT12,0},
	{"fatbig",		"",FS_16MB_MSDOS_FAT12,0},
	{"fat4572",		"3\"5        4.50MB DSHD FAT12",FS_4_50MB_MSDOS_FAT12,0},
	{"fat2540",		"3\"5        2.50MB DSDD FAT12",FS_2_50MB_MSDOS_FAT12,0},
	{0,0,0,0}
};


HXCFE_FLOPPY * hxcfe_generateFloppy( HXCFE* floppycontext, char* path, int32_t fsID, int32_t * err_ret )
{
	int ret;
	HXCFE_FLOPPY * newfloppy;
	HXCFE_IMGLDR * imgldr_ctx;
	plugins_ptr func_ptr;
	int moduleID;
	int i;

	floppycontext->hxc_printf(MSG_INFO_0,"Create file system (source path : %s) ...",path);

	newfloppy=0;

	imgldr_ctx = hxcfe_imgInitLoader(floppycontext);
	if(imgldr_ctx)
	{
		if( fsID == FS_880KB_AMIGADOS )
			moduleID = hxcfe_imgGetLoaderID (imgldr_ctx,PLUGIN_AMIGA_FS);
		else
			moduleID = hxcfe_imgGetLoaderID (imgldr_ctx,PLUGIN_FAT12FLOPPY);

		i=0;
		while(fs_config_table[i].name && (fs_config_table[i].fsID != fsID))
		{
			i++;
		}

		if(hxcfe_checkLoaderID(imgldr_ctx,moduleID)==HXCFE_NOERROR && fs_config_table[i].name)
		{
			ret=staticplugins[moduleID](imgldr_ctx,GETFUNCPTR,&func_ptr);
			if(ret==HXCFE_NOERROR)
			{
				newfloppy=malloc(sizeof(HXCFE_FLOPPY));
				memset(newfloppy,0,sizeof(HXCFE_FLOPPY));
				floppycontext->hxc_printf(MSG_INFO_0,"file loader found!");
				ret=func_ptr.libLoad_DiskFile(imgldr_ctx,newfloppy,path,fs_config_table[i].name);
				if(ret!=HXCFE_NOERROR)
				{
					free(newfloppy);
					newfloppy=0;
				}
				if(err_ret) *err_ret=ret;

				hxcfe_imgDeInitLoader(imgldr_ctx);

				return newfloppy;
			}
		}

		floppycontext->hxc_printf(MSG_ERROR,"Bad plugin ID : 0x%x",moduleID);

		ret=HXCFE_BADPARAMETER;
		if(err_ret) *err_ret=ret;

		hxcfe_imgDeInitLoader(imgldr_ctx);
	}
	return newfloppy;
}

int32_t hxcfe_getTrackBitrate( HXCFE* floppycontext, HXCFE_FLOPPY * fp, int32_t track, int32_t side )
{
	if(fp)
	{
		return fp->tracks[track]->sides[side]->bitrate;
	}

	return 0;
}

int32_t hxcfe_getTrackEncoding( HXCFE* floppycontext, HXCFE_FLOPPY * fp, int32_t track, int32_t side )
{
	if(fp)
	{
		return fp->tracks[track]->sides[side]->track_encoding;
	}

	return 0;
}

int32_t hxcfe_getTrackLength( HXCFE* floppycontext, HXCFE_FLOPPY * fp, int32_t track, int32_t side )
{
	if(fp)
	{
		return fp->tracks[track]->sides[side]->tracklen;
	}

	return 0;
}

int32_t hxcfe_getTrackRPM( HXCFE* floppycontext, HXCFE_FLOPPY * fp, int32_t track )
{
	if(fp)
	{
		return fp->tracks[track]->floppyRPM;
	}

	return 0;
}

int32_t hxcfe_getTrackNumberOfSide( HXCFE* floppycontext, HXCFE_FLOPPY * fp, int32_t track )
{
	if(fp)
	{
		return fp->tracks[track]->number_of_side;
	}

	return 0;
}
