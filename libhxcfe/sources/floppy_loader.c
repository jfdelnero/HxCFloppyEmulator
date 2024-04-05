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
// File : floppy_loader.c
// Contains: Library interface functions
//
// Written by: Jean-François DEL NERO
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

#include "plugins_id.h"

#include "libhxcadaptor.h"

#include "misc/env.h"
#include "misc/script_exec.h"

#include "init_script.h"

#include "xml_disk/packer/pack.h"

#include "tracks/track_types_defs.h"


int dummy_output(int MSGTYPE,const char * chaine, ...)
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
	HXCFE_IMGLDR *imgldr_ctx;
	image_plugin* plugin_ptr;
	char * savefilepath;
	int nb_loader;
	int nb_xml_loader;
	int i,j;
	int ret;
	unsigned char * init_script_buf;

	init_script_buf = NULL;

	hxcfe = calloc( 1 ,sizeof(HXCFE) );
	if( hxcfe )
	{
		hxcfe->hxc_printf = &dummy_output;
		hxcfe->hxc_settrackpos = &dummy_trackpos;

		hxcfe->envvar = initEnv( NULL, NULL );
		hxcfe_setEnvVar(hxcfe, "LIBVERSION", "v"STR_FILE_VERSION2);

		if ( hxcfe_initScript(hxcfe) != HXCFE_NOERROR )
			goto init_error;

		hxcfe->hxc_printf(MSG_INFO_0,"Starting HxCFloppyEmulator...");

		init_script_buf = data_unpack(data_init_script->data,data_init_script->csize,0,data_init_script->size);
		if(init_script_buf)
		{
			if( hxcfe_execScriptRam(hxcfe, init_script_buf, data_init_script->size)  != HXCFE_NOERROR )
				goto init_error;

			free( init_script_buf );
		}

#if defined(WIN32)
		hxcfe_execScriptLine( hxcfe, "set SPSCAPS_LIB_NAME CAPSImg.dll" );
#elif __APPLE__
		hxcfe_execScriptLine( hxcfe, "set SPSCAPS_LIB_NAME CAPSImage.framework/CAPSImage,CAPSImg.framework/CAPSImg" );
#else
		hxcfe_execScriptLine( hxcfe, "set SPSCAPS_LIB_NAME libcapsimage.so.5,libcapsimage.so.5.1,libcapsimage.so" );
#endif

		hxcfe_execScriptFile(hxcfe, "config.script");

		savefilepath = hxcfe_getEnvVar( hxcfe, "UISTATE_SAVE_FILE", 0 );
		if(savefilepath)
		{
			if(strlen(savefilepath))
			{
				hxcfe_execScriptFile(hxcfe, savefilepath);
			}
			else
			{
				hxcfe_execScriptLine( hxcfe, "set UISTATE_SAVE_FILE laststate.script" );
				savefilepath = hxcfe_getEnvVar( hxcfe, "UISTATE_SAVE_FILE", 0 );
				hxcfe_execScriptFile(hxcfe, savefilepath);
			}
		}

		nb_loader = 0;
		// Count how many static loaders we have.
		while( staticplugins[nb_loader] )
		{
			nb_loader++;
		}

		nb_xml_loader = 0;
		imgldr_ctx = hxcfe_imgInitLoader(hxcfe);
		if( imgldr_ctx )
		{
			ret = staticplugins[nb_loader - 1](imgldr_ctx,GETNBSUBLOADER,&nb_xml_loader);
			if( ret != HXCFE_NOERROR )
			{
				nb_xml_loader = 0;
				hxcfe->hxc_printf(MSG_ERROR,"XML Loader unavailable...");
			}
		}

		if( nb_loader )
		{
			hxcfe->image_handlers = (void*)calloc( 1, (nb_loader+nb_xml_loader+1) * sizeof(image_plugin) );
			if( hxcfe->image_handlers )
			{
				plugin_ptr = (image_plugin*)hxcfe->image_handlers;

				for( i = 0 ; i < nb_loader - 1; i++ )
				{
					plugin_ptr[i].infos_handler = staticplugins[i];
					plugin_ptr[i].sub_id = 0;
					plugin_ptr[i].flags = 0x00000000;
				}

				for( j = 0 ; j < nb_xml_loader; j++ )
				{
					plugin_ptr[i + j].infos_handler = staticplugins[nb_loader - 1];
					plugin_ptr[i + j].sub_id = j + 1;
					plugin_ptr[i + j].flags = IMAGE_LDR_DISABLED;
				}
			}
			else
			{
				if( imgldr_ctx )
					hxcfe_imgDeInitLoader(imgldr_ctx);

				free( hxcfe );

				return NULL;
			}
		}

		if( imgldr_ctx )
			hxcfe_imgDeInitLoader(imgldr_ctx);
	}

	return hxcfe;

init_error:

	free( hxcfe );

	free( init_script_buf );

	return NULL;
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
	if(floppycontext)
	{
		if(!floppycontext->license)
		{
			floppycontext->license = (char*)data_unpack(data_COPYING_FULL->data,data_COPYING_FULL->csize,0,data_COPYING_FULL->size);
		}

		return (char*)floppycontext->license;
	}

	return NULL;
}

void hxcfe_deinit(HXCFE* hxcfe)
{
	if( hxcfe )
	{
		hxcfe->hxc_printf(MSG_INFO_0,"Stopping HxCFloppyEmulator...");

		hxcfe_deinitScript(hxcfe);

		free( hxcfe->image_handlers );

		free( hxcfe->license );

		free( hxcfe );
	}
}

////////////////////////////////////////////////////////////////////////

static int32_t hxcfe_checkLoaderID( HXCFE_IMGLDR * imgldr_ctx, int32_t moduleID )
{
	int i;
	image_plugin* plugin_ptr;

	plugin_ptr = (image_plugin*)imgldr_ctx->hxcfe->image_handlers;

	if( moduleID >= 0 )
	{
		i=0;
		do
		{
			i++;
		}while( plugin_ptr[i].infos_handler && i<moduleID);

		if( !plugin_ptr[i].infos_handler )
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
	image_plugin* plugin_ptr;

	plugin_ptr = (image_plugin*)imgldr_ctx->hxcfe->image_handlers;

	i=0;
	do
	{
		i++;
	}while( plugin_ptr[i].infos_handler );

	return i;
}

int32_t hxcfe_imgGetLoaderID( HXCFE_IMGLDR * imgldr_ctx, char * container )
{
	int i;
	int ret;
	char * plugin_id;
	image_plugin* plugin_ptr;

	plugin_ptr = (image_plugin*)imgldr_ctx->hxcfe->image_handlers;

	ret=0;
	i=0;
	do
	{
		if( plugin_ptr[i].infos_handler )
		{
			ret = plugin_ptr[i].infos_handler(imgldr_ctx,GETPLUGINID,&plugin_id);
			if((ret==HXCFE_NOERROR)  && !strcmp(container,plugin_id))
			{
				return i;
			}
		}

		i++;
	}while( plugin_ptr[i].infos_handler );

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"hxcfe_searchContainer : Plugin %s not found !",container);

	return -1;
}

int32_t hxcfe_imgGetLoaderAccess( HXCFE_IMGLDR * imgldr_ctx, int32_t moduleID )
{
	int ret;
	plugins_ptr func_ptr;
	image_plugin* plugin_ptr;

	plugin_ptr = (image_plugin*)imgldr_ctx->hxcfe->image_handlers;

	ret = 0;

	if(hxcfe_checkLoaderID(imgldr_ctx,moduleID)==HXCFE_NOERROR)
	{
		ret=plugin_ptr[moduleID].infos_handler(imgldr_ctx,GETFUNCPTR,&func_ptr);
		if(ret==HXCFE_NOERROR)
		{
			imgldr_ctx->selected_id = moduleID;
			imgldr_ctx->selected_subid = 0;

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
	image_plugin* plugin_ptr;
	int sub_id;
	plugin_ptr = (image_plugin*)imgldr_ctx->hxcfe->image_handlers;

	if(hxcfe_checkLoaderID(imgldr_ctx,moduleID)==HXCFE_NOERROR)
	{
		ret = plugin_ptr[moduleID].infos_handler(imgldr_ctx,GETFUNCPTR,&func_ptr);
		if(ret==HXCFE_NOERROR)
		{
			imgldr_ctx->selected_id = moduleID;
			imgldr_ctx->selected_subid = 0;

			if( !plugin_ptr[moduleID].sub_id)
			{
				plugin_ptr[moduleID].infos_handler(imgldr_ctx,GETDESCRIPTION,&desc);
			}
			else
			{
				sub_id = plugin_ptr[moduleID].sub_id;
				if( plugin_ptr[moduleID].infos_handler(imgldr_ctx,SELECTSUBLOADER,&sub_id) == HXCFE_NOERROR )
				{
					plugin_ptr[moduleID].infos_handler(imgldr_ctx,GETDESCRIPTION,&desc);
				}
				else
				{
					return 0;
				}
			}

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
	image_plugin* plugin_ptr;
	int sub_id;

	plugin_ptr = (image_plugin*)imgldr_ctx->hxcfe->image_handlers;

	if(hxcfe_checkLoaderID(imgldr_ctx,moduleID)==HXCFE_NOERROR)
	{
		ret = plugin_ptr[moduleID].infos_handler(imgldr_ctx,GETFUNCPTR,&func_ptr);
		if(ret==HXCFE_NOERROR)
		{
			imgldr_ctx->selected_id = moduleID;
			imgldr_ctx->selected_subid = 0;

			if( !plugin_ptr[moduleID].sub_id)
			{
				plugin_ptr[moduleID].infos_handler(imgldr_ctx,GETPLUGINID,&desc);
			}
			else
			{
				sub_id = plugin_ptr[moduleID].sub_id;
				if( plugin_ptr[moduleID].infos_handler(imgldr_ctx,SELECTSUBLOADER,&sub_id) == HXCFE_NOERROR )
				{
					plugin_ptr[moduleID].infos_handler(imgldr_ctx,GETPLUGINID,&desc);
				}
				else
				{
					return 0;
				}
			}

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
	image_plugin* plugin_ptr;
	int sub_id;

	plugin_ptr = (image_plugin*)imgldr_ctx->hxcfe->image_handlers;

	if(hxcfe_checkLoaderID(imgldr_ctx,moduleID)==HXCFE_NOERROR)
	{
		ret = plugin_ptr[moduleID].infos_handler(imgldr_ctx,GETFUNCPTR,&func_ptr);
		if(ret==HXCFE_NOERROR)
		{
			imgldr_ctx->selected_id = moduleID;
			imgldr_ctx->selected_subid = 0;

			if( !plugin_ptr[moduleID].sub_id)
			{
				plugin_ptr[moduleID].infos_handler(imgldr_ctx,GETEXTENSION,&desc);
			}
			else
			{
				sub_id = plugin_ptr[moduleID].sub_id;
				if( plugin_ptr[moduleID].infos_handler(imgldr_ctx,SELECTSUBLOADER,&sub_id) == HXCFE_NOERROR )
				{
					plugin_ptr[moduleID].infos_handler(imgldr_ctx,GETEXTENSION,&desc);
				}
				else
				{
					return 0;
				}
			}

			return desc;
		}
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Bad module ID : %x !",moduleID);
	}

	return 0;
}

int32_t hxcfe_imgCheckFileCompatibility( HXCFE_IMGLDR * imgldr_ctx, HXCFE_IMGLDR_FILEINFOS * file_infos, char * loadername, char * fileext, int filesizemod)
{
	char temp_ext[16];
	int ret;
	int i,j;

	if(loadername)
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"%s", loadername);

	if( fileext )
	{
		j = 0;
		do
		{
			memset(temp_ext,0,sizeof(temp_ext));

			i = 0;
			while ( i < sizeof(temp_ext) - 1 && fileext[j] != ',' && fileext[j] )
			{
				temp_ext[i] = fileext[j];
				j++;
				i++;
			}

			if(fileext[j] == ',')
				j++;

			ret = hxc_checkfileext( file_infos->path, temp_ext, SYS_PATH_TYPE );
		}while(fileext[j] && !ret);

		if( !ret )
			return HXCFE_BADFILE;
	}

	if( filesizemod )
	{
		if( ( file_infos->file_size % filesizemod ) || (file_infos->file_size <= 0) )
		{
			return HXCFE_BADFILE;
		}
	}

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"%s : %s is an %s file !", loadername, file_infos->path,fileext);

	return HXCFE_VALIDFILE;
}

int32_t hxcfe_preloadImgInfos(HXCFE_IMGLDR * imgldr_ctx, char * imgname, HXCFE_IMGLDR_FILEINFOS * file_infos)
{
	FILE *f;
	struct stat img_stat;

	memset(file_infos,0,sizeof(HXCFE_IMGLDR_FILEINFOS));
	memset(&img_stat,0,sizeof(struct stat));

	hxc_stat( imgname, &img_stat );

	if(img_stat.st_mode&S_IFDIR)
		file_infos->is_dir = 1;
	else
		file_infos->is_dir = 0;

	file_infos->file_size = 0;

	if(!file_infos->is_dir)
	{
		f = hxc_fopen(imgname,"rb");
		if( f == NULL )
			return HXCFE_ACCESSERROR;

		file_infos->file_size = hxc_fgetsize(f);

		hxc_fread(&file_infos->file_header,sizeof(file_infos->file_header),f);

		hxc_fclose(f);
	}

	strncpy(file_infos->file_extension, hxc_getfilenameext( imgname, 0, SYS_PATH_TYPE ), sizeof(file_infos->file_extension)-1);
	file_infos->file_extension[sizeof(file_infos->file_extension)-1] = 0;

	strncpy(file_infos->path, imgname, sizeof(file_infos->path));
	file_infos->path[sizeof(file_infos->path)-1] = 0;

	return HXCFE_NOERROR;
}

int32_t hxcfe_imgAutoSetectLoader( HXCFE_IMGLDR * imgldr_ctx, char* imgname, int32_t moduleID )
{
	int i;
	int ret;
	plugins_ptr func_ptr;
	HXCFE* hxcfe;
	image_plugin* plugin_ptr;
	HXCFE_IMGLDR_FILEINFOS file_infos;

	ret = HXCFE_BADPARAMETER;

	if( imgldr_ctx )
	{
		plugin_ptr = (image_plugin*)imgldr_ctx->hxcfe->image_handlers;

		hxcfe = imgldr_ctx->hxcfe;

		hxcfe->hxc_printf(MSG_INFO_0,"Checking %s",imgname);

		ret = hxcfe_preloadImgInfos(imgldr_ctx, imgname, &file_infos);
		if(ret != HXCFE_NOERROR)
			return ret;

		ret = HXCFE_BADPARAMETER;

		i = moduleID;
		do
		{
			if(plugin_ptr[i].infos_handler && !(plugin_ptr[i].flags & IMAGE_LDR_DISABLED) )
			{
				ret = plugin_ptr[i].infos_handler(imgldr_ctx,GETFUNCPTR,&func_ptr);
				if( ret == HXCFE_NOERROR )
				{
					if(func_ptr.libIsValidDiskFile)
					{
						ret = func_ptr.libIsValidDiskFile(imgldr_ctx,&file_infos);
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
		}while( plugin_ptr[i].infos_handler );

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

	imgldr_ctx = calloc( 1, sizeof(HXCFE_IMGLDR));
	if(imgldr_ctx)
	{
		imgldr_ctx->hxcfe = hxcfe;
		imgldr_ctx->hxc_setprogress=&dummy_progress;
	}

	return imgldr_ctx;
}

void hxcfe_imgDeInitLoader(HXCFE_IMGLDR * imgldr_ctx)
{
	free( imgldr_ctx );

	return;
}

HXCFE_FLOPPY * hxcfe_imgLoadEx( HXCFE_IMGLDR * imgldr_ctx, char* imgname, int32_t moduleID, int32_t * err_ret, void * parameters )
{
	int ret;
	HXCFE* hxcfe;
	HXCFE_FLOPPY * newfloppy;
	plugins_ptr func_ptr;
	image_plugin* plugin_ptr;
	HXCFE_IMGLDR_FILEINFOS file_infos;

	newfloppy = 0;

	if(imgldr_ctx)
	{
		plugin_ptr = (image_plugin*)imgldr_ctx->hxcfe->image_handlers;

		hxcfe = imgldr_ctx->hxcfe;

		hxcfe->hxc_printf(MSG_INFO_0,"Loading %s",imgname);

		ret = hxcfe_preloadImgInfos(imgldr_ctx, imgname, &file_infos);
		if(ret != HXCFE_NOERROR)
		{
			if(err_ret)
				*err_ret = ret;
			return 0;
		}

		if( hxcfe_checkLoaderID(imgldr_ctx,moduleID) == HXCFE_NOERROR )
		{
			ret = plugin_ptr[moduleID].infos_handler(imgldr_ctx,GETFUNCPTR,&func_ptr);
			if( ret == HXCFE_NOERROR )
			{
				ret = func_ptr.libIsValidDiskFile(imgldr_ctx,&file_infos);
				if( ret == HXCFE_VALIDFILE )
				{
					newfloppy = calloc( 1, sizeof(HXCFE_FLOPPY) );
					if(newfloppy)
					{
						hxcfe->hxc_printf(MSG_INFO_0,"file loader found!");
						ret = func_ptr.libLoad_DiskFile(imgldr_ctx,newfloppy,imgname,parameters);
						if(ret != HXCFE_NOERROR)
						{
							free( newfloppy );
							newfloppy = NULL;
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
	return hxcfe_floppyUnload( imgldr_ctx->hxcfe, floppydisk );
}

int32_t hxcfe_imgExport( HXCFE_IMGLDR * imgldr_ctx, HXCFE_FLOPPY * newfloppy, char* imgname, int32_t moduleID )
{
	int ret;
	plugins_ptr func_ptr;
	image_plugin* plugin_ptr;

	plugin_ptr = (image_plugin*)imgldr_ctx->hxcfe->image_handlers;

	if(hxcfe_checkLoaderID(imgldr_ctx,moduleID)==HXCFE_NOERROR)
	{
		ret=plugin_ptr[moduleID].infos_handler(imgldr_ctx->hxcfe,GETFUNCPTR,&func_ptr);
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

int32_t hxcfe_freeFloppy( HXCFE* floppycontext, HXCFE_FLOPPY * floppydisk )
{
	int i,j;

	if(floppydisk)
	{
		if( floppydisk->tracks )
		{
			for(j=0;j<floppydisk->floppyNumberOfTrack;j++)
			{
				if( floppydisk->tracks[j] )
				{
					if( floppydisk->tracks[j]->sides )
					{
						for(i=0;i<floppydisk->floppyNumberOfSide;i++)
						{
							hxcfe_freeSide(floppycontext,floppydisk->tracks[j]->sides[i]);
							floppydisk->tracks[j]->sides[i] = NULL;
						}

						free( floppydisk->tracks[j]->sides );
						floppydisk->tracks[j]->sides = NULL;
					}

					free(floppydisk->tracks[j]);
					floppydisk->tracks[j] = NULL;
				}
			}
			free(floppydisk->tracks);
			floppydisk->tracks = NULL;
		}
	}

	return 0;
}

int32_t hxcfe_floppyUnload( HXCFE* floppycontext, HXCFE_FLOPPY * floppydisk )
{
	hxcfe_freeFloppy( floppycontext, floppydisk );

	free( floppydisk );

	return 0;
}

HXCFE_FLOPPY * hxcfe_floppyDuplicate( HXCFE* floppycontext, HXCFE_FLOPPY * floppydisk )
{
	int i,j;
	HXCFE_FLOPPY * fp;

	fp = NULL;
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
					if( fp->tracks[j] )
					{
						memcpy( fp->tracks[j], floppydisk->tracks[j], sizeof(HXCFE_CYLINDER) );

						fp->tracks[j]->sides = (HXCFE_SIDE**)calloc( 1, sizeof(HXCFE_SIDE*)*fp->tracks[j]->number_of_side );
						if(fp->tracks[j]->sides)
						{
							for(i=0;i<fp->tracks[j]->number_of_side;i++)
							{
								fp->tracks[j]->sides[i] = hxcfe_duplicateSide( floppycontext, floppydisk->tracks[j]->sides[i] );
							}
						}
					}
				}
			}
		}
	}

	return fp;
}

int32_t hxcfe_floppySectorBySectorCopy( HXCFE* floppycontext, HXCFE_FLOPPY * dest_floppy, HXCFE_FLOPPY * src_floppy, int dest_format_ref )
{
	int i,j,l,len;
	int32_t fdcstatus;
	HXCFE_SECTORACCESS* ref_sect_access;
	HXCFE_SECTORACCESS* src_sect_access;
	HXCFE_SECTORACCESS* new_sect_access;
	HXCFE_SECTCFG* sect_ref;
	HXCFE_FLOPPY * ref_floppy;

	unsigned char * sect_buf;
	int types[]={ISOIBM_MFM_ENCODING,ISOIBM_FM_ENCODING,AMIGA_MFM_ENCODING,-1};
	int type_index;
	char tag_string[64+1];
	int error_tag_value;

	if(dest_format_ref)
	{
		ref_floppy = hxcfe_floppyDuplicate( floppycontext, dest_floppy );
	}
	else
	{
		ref_floppy = hxcfe_floppyDuplicate( floppycontext, src_floppy );
	}

	if(!ref_floppy)
		return HXCFE_BADPARAMETER;

	ref_sect_access = hxcfe_initSectorAccess( floppycontext, ref_floppy );
	if(!ref_sect_access)
		return HXCFE_BADPARAMETER;

	new_sect_access = hxcfe_initSectorAccess( floppycontext, dest_floppy );
	if(!new_sect_access)
		return HXCFE_BADPARAMETER;

	src_sect_access = hxcfe_initSectorAccess( floppycontext, src_floppy );
	if(!src_sect_access)
		return HXCFE_BADPARAMETER;

	type_index = 0;

	error_tag_value = hxcfe_getEnvVarValue( floppycontext, "SECTORBYSECTORCOPY_SECTOR_ERROR_TAG" );

	while(types[type_index] != -1)
	{
		for(i=0;i<ref_floppy->floppyNumberOfTrack;i++)
		{
			for(j=0;j<ref_floppy->floppyNumberOfSide;j++)
			{
				hxcfe_resetSearchTrackPosition(ref_sect_access);
				do
				{
					sect_ref = hxcfe_getNextSector( ref_sect_access, i, j, types[type_index]);
					if(sect_ref)
					{
						hxcfe_readSectorData( src_sect_access, i, j, sect_ref->sector, 1, sect_ref->sectorsize, types[type_index], sect_ref->input_data, &fdcstatus );

						sect_buf = NULL;
						if(fdcstatus != FDC_NOERROR)
						{
							switch(fdcstatus)
							{
								case FDC_BAD_DATA_CRC:
									snprintf(tag_string,sizeof(tag_string)-1,"<ERROR DATA_CRC T:%.3d H:%c S:%.3d>",i,'0'+(j&1),sect_ref->sector);
								break;
								case FDC_NO_DATA:
									snprintf(tag_string,sizeof(tag_string)-1,"<ERROR NO_DATA  T:%.3d H:%c S:%.3d>",i,'0'+(j&1),sect_ref->sector);
								break;
								case FDC_SECTOR_NOT_FOUND:
									snprintf(tag_string,sizeof(tag_string)-1,"<ERROR NOTFOUND T:%.3d H:%c S:%.3d>",i,'0'+(j&1),sect_ref->sector);
								break;
								case FDC_ACCESS_ERROR:
									snprintf(tag_string,sizeof(tag_string)-1,"<ERROR ACCESS   T:%.3d H:%c S:%.3d>",i,'0'+(j&1),sect_ref->sector);
								break;
							}

							floppycontext->hxc_printf(MSG_WARNING,"Source file sector error : %s", tag_string );

							if(sect_ref->sectorsize > 0)
							{
								sect_buf = malloc(sect_ref->sectorsize + 1);
								if(sect_buf)
								{
									if(error_tag_value >= 0)
										memset(sect_buf,error_tag_value,sect_ref->sectorsize + 1);
									else
										memset(sect_buf,0,sect_ref->sectorsize + 1);

									if(error_tag_value == -1)
									{
										len = strlen(tag_string);
										if(len)
										{
											for(l=0;l<sect_ref->sectorsize / len;l++)
											{
												strcat((char*)sect_buf,tag_string);
											}
										}
									}
								}
							}
						}

						if(sect_buf)
						{
							if(error_tag_value != -2)
								hxcfe_writeSectorData( new_sect_access, i, j, sect_ref->sector, 1, sect_ref->sectorsize, types[type_index], sect_buf, &fdcstatus );

							free(sect_buf);
						}
						else
						{
							hxcfe_writeSectorData( new_sect_access, i, j, sect_ref->sector, 1, sect_ref->sectorsize, types[type_index], sect_ref->input_data, &fdcstatus );
						}

						hxcfe_freeSectorConfigData( 0, sect_ref );
					}
				}while(sect_ref);
			}
		}

		type_index++;
	}

	hxcfe_floppyUnload( floppycontext, ref_floppy );

	hxcfe_deinitSectorAccess( ref_sect_access );
	hxcfe_deinitSectorAccess( new_sect_access );
	hxcfe_deinitSectorAccess( src_sect_access );

	return HXCFE_NOERROR;
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

uint32_t hxcfe_floppyGetFlags( HXCFE* floppycontext, HXCFE_FLOPPY * newfloppy )
{
	return newfloppy->flags;
}

int32_t hxcfe_floppySetFlags( HXCFE* floppycontext, HXCFE_FLOPPY * newfloppy, uint32_t flags )
{
	newfloppy->flags = flags;

	return HXCFE_NOERROR;
}

int libGetPluginInfo( HXCFE_IMGLDR * imgldr_ctx, uint32_t infotype, void * returnvalue, const char * pluginid, const char * plugindesc, plugins_ptr * pluginfunc, const char * fileext )
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
	int i;

	fb_ctx = 0;

	floppycontext->hxc_printf(MSG_DEBUG,"hxcfe_init_floppy : floppy builder init");

	if(( nb_of_track && (nb_of_track<=256)) && ((nb_of_side>=1) && (nb_of_side<=2) ) )
	{
		fb_ctx = (HXCFE_FLPGEN*)calloc( 1, sizeof(HXCFE_FLPGEN) );
		if( !fb_ctx )
			goto alloc_error;

		fb_ctx->floppycontext = floppycontext;

		fb_ctx->floppydisk = (HXCFE_FLOPPY*)calloc( 1, sizeof(HXCFE_FLOPPY) );
		if(!fb_ctx->floppydisk)
			goto alloc_error;

		fb_ctx->floppydisk->floppyBitRate=DEFAULT_DD_BITRATE;
		fb_ctx->floppydisk->double_step=0;
		fb_ctx->floppydisk->floppyiftype=GENERIC_SHUGART_DD_FLOPPYMODE;
		fb_ctx->floppydisk->floppyNumberOfTrack=nb_of_track;
		fb_ctx->floppydisk->floppyNumberOfSide=nb_of_side;
		fb_ctx->floppydisk->floppySectorPerTrack=-1;

		fb_ctx->floppydisk->tracks = (HXCFE_CYLINDER**)calloc( 1, sizeof(HXCFE_CYLINDER*)*fb_ctx->floppydisk->floppyNumberOfTrack );
		if(!fb_ctx->floppydisk->tracks)
			goto alloc_error;

		fb_ctx->fb_stack_pointer=0;

		fb_ctx->fb_stack = (fb_track_state*)calloc( 1, sizeof(fb_track_state)*STACK_SIZE );
		if(!fb_ctx->fb_stack)
			goto alloc_error;

		fb_ctx->fb_stack[0].interleave=1;
		fb_ctx->fb_stack[0].rpm=300;

		fb_ctx->fb_stack[0].forced_side_id = -1;

		fb_ctx->fb_stack[0].sectors_size = 512;

		for(i=0;i<MAX_NUMBER_OF_INDEX;i++)
		{
			fb_ctx->fb_stack[0].indexlen[i] = 0;
			fb_ctx->fb_stack[0].indexpos[i] = 0;
		}

		fb_ctx->fb_stack[0].indexlen[0] = 2500;
		fb_ctx->fb_stack[0].indexpos[0] = -2500;
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

		fb_ctx->fb_stack[0].sc_stack[0].bitrate = 250000;
		fb_ctx->fb_stack[0].sc_stack[0].fill_byte = 0xF6;
		fb_ctx->fb_stack[0].sc_stack[0].gap3 = 255;
		fb_ctx->fb_stack[0].sc_stack[0].sectorsize = 512;
		fb_ctx->fb_stack[0].sc_stack[0].trackencoding=IBMFORMAT_DD;
	}

	return fb_ctx;

alloc_error:
	floppycontext->hxc_printf(MSG_ERROR,"hxcfe_init_floppy : malloc error");

	if(fb_ctx)
	{
		if(fb_ctx->floppydisk)
		{
			free( fb_ctx->floppydisk->tracks );

			free( fb_ctx->floppydisk );
		}

		free( fb_ctx->fb_stack );

		free( fb_ctx );
	}

	return 0;
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
	fb_ctx->floppydisk->tracks = (HXCFE_CYLINDER**)calloc( 1, sizeof(HXCFE_CYLINDER*)*fb_ctx->floppydisk->floppyNumberOfTrack);
	if( !fb_ctx->floppydisk->tracks)
	{
		return HXCFE_INTERNALERROR;
	}

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

		free( tmptracks );
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
				if(cur_track->sectortab[i].input_data)
					memcpy(cur_track->sectortab[i].input_data,data_to_realloc,cur_track->sectortab[i].sectorsize);

				free( data_to_realloc );
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

int32_t hxcfe_setSideSkew ( HXCFE_FLPGEN* fb_ctx, int32_t skew )
{
	fb_ctx->fb_stack[fb_ctx->fb_stack_pointer].side_skew = skew;
	return HXCFE_NOERROR;
}

int32_t hxcfe_setIndexPosition ( HXCFE_FLPGEN* fb_ctx, int32_t index_number, int32_t position, int32_t allowsector )
{
	if(index_number < MAX_NUMBER_OF_INDEX)
	{
		fb_ctx->fb_stack[fb_ctx->fb_stack_pointer].indexpos[index_number] = position;
	}
	return HXCFE_NOERROR;
}

int32_t hxcfe_setIndexLength ( HXCFE_FLPGEN* fb_ctx, int32_t index_number, int32_t Length )
{
	if(index_number < MAX_NUMBER_OF_INDEX)
	{
		fb_ctx->fb_stack[fb_ctx->fb_stack_pointer].indexlen[index_number] = Length;
	}
	return HXCFE_NOERROR;
}

int32_t hxcfe_setSectorData( HXCFE_FLPGEN* fb_ctx, uint8_t * buffer, int32_t size )
{
	fb_track_state * cur_track;
	HXCFE_SECTCFG * cur_sector;

	cur_track = &fb_ctx->fb_stack[fb_ctx->fb_stack_pointer];
	cur_sector = &cur_track->sc_stack[cur_track->sc_stack_pointer];

	free( cur_sector->input_data );
	cur_sector->input_data = NULL;

	free( cur_sector->weak_bits_mask );
	cur_sector->weak_bits_mask = NULL;

	if(buffer)
	{
		cur_sector->input_data = malloc(size);
		if( cur_sector->input_data )
		{
			memcpy(cur_sector->input_data,buffer,size);
		}
		else
		{
			return HXCFE_INTERNALERROR;
		}
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

int32_t hxcfe_setDiskSectorsHeadID( HXCFE_FLPGEN* fb_ctx, int32_t head )
{
	fb_ctx->fb_stack[fb_ctx->fb_stack_pointer].forced_side_id = head;

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

int32_t hxcfe_setInterfaceMode( HXCFE_FLPGEN* fb_ctx, int32_t mode )
{
	fb_track_state * cur_track;

	cur_track = &fb_ctx->fb_stack[fb_ctx->fb_stack_pointer];
	cur_track->interface_mode = mode;

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
				fb_ctx->floppydisk->tracks[current_fb_track_state->track_number] = allocCylinderEntry(current_fb_track_state->rpm,fb_ctx->floppydisk->floppyNumberOfSide);

			currentcylinder = fb_ctx->floppydisk->tracks[current_fb_track_state->track_number];
			sui_flag=0;

			if(!current_fb_track_state->sectorunderindex)
					sui_flag=NO_SECTOR_UNDER_INDEX;

			currentcylinder->sides[current_fb_track_state->side_number] = tg_generateTrackEx((unsigned short)current_fb_track_state->numberofsector,
																							current_fb_track_state->sectortab,
																							current_fb_track_state->interleave,
																							current_fb_track_state->skew,
																							current_fb_track_state->bitrate,
																							current_fb_track_state->rpm,
																							current_fb_track_state->type,
																							current_fb_track_state->pregap,
																							current_fb_track_state->indexlen[0] | sui_flag,current_fb_track_state->indexpos[0]);

			for(i=1;i<MAX_NUMBER_OF_INDEX;i++)
			{
				if(current_fb_track_state->indexlen[i])
				{
					fillindex(current_fb_track_state->indexpos[i],currentcylinder->sides[current_fb_track_state->side_number],current_fb_track_state->indexlen[i],1,0);
				}
			}

			for(i=0;i<current_fb_track_state->numberofsector;i++)
			{
				hxcfe_freeSectorConfigData( 0, &current_fb_track_state->sectortab[i] );
			}

			fb_ctx->fb_stack_pointer--;

			return HXCFE_NOERROR;
		}
		else
		{
			for(i=0;i<current_fb_track_state->numberofsector;i++)
			{
				hxcfe_freeSectorConfigData( 0, &current_fb_track_state->sectortab[i] );
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

int32_t hxcfe_setDiskFlags( HXCFE_FLPGEN* fb_ctx, int32_t flags )
{
	fb_ctx->disk_flags = flags;

	return HXCFE_NOERROR;
}

int32_t hxcfe_generateDisk( HXCFE_FLPGEN* fb_ctx, HXCFE_FLOPPY* floppy, void * f, uint8_t * diskdata, int32_t buffersize )
{
	int i,j,ret;
	int numberoftracks;
	int numberofsector,sectorsize;
	int bufferoffset,type;
	unsigned int rpm,interleave;
	unsigned char * track_buffer;
	unsigned char fill_byte;
	int image_size;
	int start_file_ofs;
	int skew_per_track;
	int skew_per_side;
	int auto_side_id,side_id;
	fb_track_state * cur_track;
	FILE * filehandle;
	HXCFE_SIDE* tmp_side;
	int disk_bitrate;
	if ( !f && !diskdata )
		return HXCFE_BADPARAMETER;

	filehandle = (FILE*)f;
	image_size = 0;
	start_file_ofs = 0;

	numberoftracks = fb_ctx->floppydisk->floppyNumberOfTrack;

	// Increase the track number to add the blank tracks.
	if( numberoftracks > 75 && numberoftracks < 79 )
	{
		hxcfe_setNumberOfTrack ( fb_ctx, 79 );
	}

	if( numberoftracks > 78 && numberoftracks < 84 )
	{
		hxcfe_setNumberOfTrack ( fb_ctx, 84 );
	}

	if( numberoftracks > 38 && numberoftracks < 42 )
	{
		hxcfe_setNumberOfTrack ( fb_ctx, 42 );
	}

	if( filehandle )
	{
		start_file_ofs = ftell(filehandle);
		fseek(filehandle,0,SEEK_END);
		image_size = ftell(filehandle);
		fseek(filehandle,start_file_ofs,SEEK_SET);
	}
	else
	{
		image_size = buffersize;
	}

	cur_track = &fb_ctx->fb_stack[fb_ctx->fb_stack_pointer];

	sectorsize = cur_track->sectors_size;
	numberofsector = cur_track->numberofsector_min;
	interleave = cur_track->interleave;
	rpm = cur_track->rpm;
	type = cur_track->type;
	skew_per_track = cur_track->skew;
	skew_per_side = cur_track->side_skew;
	auto_side_id = cur_track->forced_side_id;

	fill_byte = cur_track->sc_stack[cur_track->sc_stack_pointer].fill_byte;

	fb_ctx->floppycontext->hxc_printf(MSG_INFO_1,"Image Size:%dkB, %d tracks, %d side(s), %d sectors/track, interleave:%d,rpm:%d",(image_size-start_file_ofs)/1024,
																																						numberoftracks,
																																						fb_ctx->floppydisk->floppyNumberOfSide,
																																						numberofsector,
																																						interleave,
																																						rpm);

	track_buffer = malloc( sectorsize *	numberofsector );
	if( !track_buffer )
		return HXCFE_INTERNALERROR;

	for(i = 0 ; i < numberoftracks ; i++ )
	{
		for(j = 0 ; j < fb_ctx->floppydisk->floppyNumberOfSide ; j++ )
		{
			if( fb_ctx->disk_flags & FLPGEN_SIDES_GROUPED )
			{
				bufferoffset = start_file_ofs + \
								( numberofsector * sectorsize * numberoftracks * j) + \
								( numberofsector * sectorsize * i );
			}
			else
			{
				if( fb_ctx->floppydisk->floppyNumberOfSide == 2 )
				{
					bufferoffset = start_file_ofs + ( numberofsector * sectorsize * ((i<<1) + (j&1)) ) ;
				}
				else
				{
					bufferoffset = start_file_ofs + ( numberofsector * sectorsize * i );
				}
			}

			if( filehandle )
			{
				fseek( filehandle, bufferoffset, SEEK_SET);
				if( (bufferoffset + (numberofsector * sectorsize)) < image_size )
				{
					if( fread( track_buffer, numberofsector * sectorsize, 1, filehandle ) != 1 )
					{
						fb_ctx->floppycontext->hxc_printf(MSG_ERROR,"hxcfe_generateDisk : Error while reading the file image ! Unexpected end of file ?");
					}
				}
				else
				{
					memset(track_buffer, fill_byte, sectorsize * numberofsector);
					if( bufferoffset < image_size )
					{
						if(fread(track_buffer,image_size - bufferoffset,1,f) != 1)
						{
							fb_ctx->floppycontext->hxc_printf(MSG_ERROR,"hxcfe_generateDisk : Error while reading the file image ! Unexpected end of file ?");
						}
					}
				}
			}
			else
			{
				if( (bufferoffset + (numberofsector * sectorsize)) < image_size )
				{
					memcpy(track_buffer,&diskdata[bufferoffset],numberofsector * sectorsize);
				}
				else
				{
					memset(track_buffer, fill_byte, sectorsize * numberofsector);
					if( bufferoffset < image_size )
					{
						memcpy(track_buffer,&diskdata[bufferoffset],image_size - bufferoffset);
					}
				}
			}

			if(!fb_ctx->floppydisk->tracks[i] || !fb_ctx->floppydisk->tracks[i]->sides[j])
			{
				//if(!fb_ctx->floppydisk->tracks[i]->sides[j])
				{
					hxcfe_pushTrack (fb_ctx,rpm,i,j,type);

					hxcfe_setTrackSkew( fb_ctx, (skew_per_track*i) + (skew_per_side*j) );

					if(auto_side_id < 0)
						side_id = j;
					else
						side_id = auto_side_id;

					ret = hxcfe_addSectors(fb_ctx,side_id,i,track_buffer,(sectorsize * numberofsector),numberofsector);

					hxcfe_popTrack (fb_ctx);

					if ( ret != HXCFE_NOERROR )
						goto error;
				}
			}
		}

		if( fb_ctx->floppydisk->floppyNumberOfSide == 2 && (fb_ctx->disk_flags & FLPGEN_FLIP_SIDES) )
		{
			tmp_side = fb_ctx->floppydisk->tracks[i]->sides[0];
			fb_ctx->floppydisk->tracks[i]->sides[0] = fb_ctx->floppydisk->tracks[i]->sides[1];
			fb_ctx->floppydisk->tracks[i]->sides[1] = tmp_side;
		}
	}

	// Add blank tracks.
	for(;i<fb_ctx->floppydisk->floppyNumberOfTrack;i++)
	{
		for(j = 0 ; j < fb_ctx->floppydisk->floppyNumberOfSide ; j++ )
		{
			hxcfe_pushTrack (fb_ctx,rpm,i,j,type);
			hxcfe_popTrack (fb_ctx);
		}
	}

	disk_bitrate = VARIABLEBITRATE;
	if(fb_ctx->floppydisk->tracks[0] && fb_ctx->floppydisk->tracks[0]->sides[0])
	{
		disk_bitrate = fb_ctx->floppydisk->tracks[0]->sides[0]->bitrate;
	}

	for(i = 0 ; i < fb_ctx->floppydisk->floppyNumberOfTrack ; i++ )
	{
		for(j = 0 ; j < fb_ctx->floppydisk->floppyNumberOfSide ; j++ )
		{
			if(fb_ctx->floppydisk->tracks[i] && fb_ctx->floppydisk->tracks[i]->sides[j])
			{
				if( fb_ctx->floppydisk->tracks[i]->sides[j]->bitrate != disk_bitrate )
				{
					disk_bitrate = VARIABLEBITRATE;
				}
			}
		}
	}

	fb_ctx->floppydisk->floppyBitRate = disk_bitrate;

	if ( floppy )
	{
		memcpy(floppy, fb_ctx->floppydisk , sizeof(HXCFE_FLOPPY));
		floppy->floppyiftype = cur_track->interface_mode;
	}

	fb_ctx->floppycontext->hxc_printf(MSG_INFO_1,"track file successfully loaded and encoded!");

	free( track_buffer );

	free( fb_ctx->fb_stack );
	free( fb_ctx );

	return HXCFE_NOERROR;

error:
	free( track_buffer );

	free( fb_ctx->fb_stack );
	free( fb_ctx );

	return HXCFE_INTERNALERROR;
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
																			fb_ctx->fb_stack[0].indexlen[0],
																			fb_ctx->fb_stack[0].indexpos[0]);

						for(i=1;i<MAX_NUMBER_OF_INDEX;i++)
						{
							if(fb_ctx->fb_stack[0].indexlen[i])
							{
								fillindex(fb_ctx->fb_stack[0].indexpos[0],fb_ctx->floppydisk->tracks[j]->sides[i],fb_ctx->fb_stack[0].indexlen[i],1,0);
							}
						}
					}
				}
			}
		}
		fb_ctx->floppydisk->floppyBitRate=bitrate;

		f = fb_ctx->floppydisk;
		free( fb_ctx->fb_stack );
		free( fb_ctx );
	}

	return f;
}

HXCFE_FLOPPY* hxcfe_sanityCheck(HXCFE* floppycontext,HXCFE_FLOPPY * floppydisk)
{
	int numberoftrack,truenumberoftrack;
	int numberofside,oldnumberofside,truenumberofside;
	int needanewpass;
	int i;
	HXCFE_SIDE ** tmpsides;
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
		}

		numberofside = floppydisk->floppyNumberOfSide;

		if(!floppydisk->tracks)
		{
			floppycontext->hxc_printf(MSG_WARNING,"Sanity Checker : No track allocated ? Please Check the Loader !");
			floppydisk->tracks=(HXCFE_CYLINDER**)calloc( 1, sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack );
			if(!floppydisk->tracks)
				goto alloc_error;
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
							floppydisk->tracks[truenumberoftrack]->sides = (HXCFE_SIDE**)calloc( 1, sizeof(HXCFE_SIDE*)*numberofside);
							if(!floppydisk->tracks[truenumberoftrack]->sides)
								goto alloc_error;

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
					currentside=(HXCFE_SIDE*)calloc( 1, sizeof(HXCFE_SIDE) );
					if(!currentside)
						goto alloc_error;
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

					currentside->databuffer = calloc( 1, currentside->tracklen/8 );
					if(!currentside->databuffer)
						goto alloc_error;

					currentside->indexbuffer = calloc( 1, currentside->tracklen/8 );
					if(!currentside->indexbuffer)
						goto alloc_error;

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

alloc_error:
	floppycontext->hxc_printf(MSG_WARNING,"Sanity Checker :  Memory allocation error !",truenumberoftrack);
	return NULL;
}

fs_config fs_config_table[]=
{
	{"fatst",		"",FS_720KB_ATARI_FAT12,0},
	{"fatst902",	"",FS_902KB_ATARI_FAT12,0},
	{"fatst360",	"",FS_360KB_ATARI_FAT12,0},
	{"amigados",	"3\"5        880KB DSDD AmigaDOS",FS_880KB_AMIGADOS,1},

	{"amigados_hd",	"3\"5       1760KB DSHD AmigaDOS",FS_1760KB_AMIGADOS,2},

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
	{"fat738",		"3\"5        738KB DSDD FAT12",FS_738KB_MSDOS_FAT12,0},
	{"fat800",		"3\"5        800KB DSDD FAT12",FS_800KB_MSDOS_FAT12,0},
	{"fat820",		"3\"5        820KB DSDD FAT12",FS_820KB_MSDOS_FAT12,0},

	{"fat1200",		"5\"25       1.2MB DSHD FAT12",FS_5P25_300RPM_1200KB_MSDOS_FAT12,0},
	{"fat1230",		"5\"25       1.23MB DSHD FAT12",FS_5P25_300RPM_1230KB_MSDOS_FAT12,0},

	{"fat1440",		"3\"5        1.44MB DSHD FAT12",FS_1_44MB_MSDOS_FAT12,0},
	{"fat1476",		"3\"5        1.478MB DSHD FAT12",FS_1_476MB_MSDOS_FAT12,0},

	{"fat1600",		"3\"5        1.6MB DSHD FAT12",FS_1_600MB_MSDOS_FAT12,0},
	{"fat1640",		"3\"5        1.64MB DSHD FAT12",FS_1_640MB_MSDOS_FAT12,0},
	{"fat1680",		"3\"5        1.68MB DSHD FAT12",FS_1_68MB_MSDOS_FAT12,0},

	{"fat1722",		"3\"5        1.722MB DSHD FAT12",FS_1_722MB_MSDOS_FAT12,0},
	{"fat1743",		"3\"5        1.743MB DSHD FAT12",FS_1_743MB_MSDOS_FAT12,0},
	{"fat1764",		"3\"5        1.764MB DSHD FAT12",FS_1_764MB_MSDOS_FAT12,0},
	{"fat1785",		"3\"5        1.785MB DSHD FAT12",FS_1_785MB_MSDOS_FAT12,0},

	{"fat2540",		"3\"5        2.50MB DSDD FAT12",FS_2_50MB_MSDOS_FAT12,0},

	{"fat2880",		"3\"5        2.88MB DSED FAT12",FS_2_88MB_MSDOS_FAT12,0},
	{"fat3381",		"3\"5        3.38MB DSHD FAT12",FS_3_38MB_MSDOS_FAT12,0},
	{"fatbigst",	"3\"5        3.42MB DSDD Atari FAT12",FS_3_42MB_ATARI_FAT12,0},

	{"fat5355",		"3\"5        5.35MB DSHD FAT12",FS_5_35MB_MSDOS_FAT12,0},
	{"fat5355b",	"3\"5        5.35MB DSHD FAT12",FS_5_35MB_B_MSDOS_FAT12,0},

	{"fat6789",		"3\"5        6.78MB DSHD FAT12",FS_6_78MB_MSDOS_FAT12,0},
	{"fatbig",		"",FS_16MB_MSDOS_FAT12,0},
	{"fat4572",		"3\"5        4.50MB DSHD FAT12",FS_4_50MB_MSDOS_FAT12,0},
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
	image_plugin* plugin_ptr;

	floppycontext->hxc_printf(MSG_INFO_0,"Create file system (source path : %s) ...",path);

	plugin_ptr = (image_plugin*)floppycontext->image_handlers;

	newfloppy = 0;

	imgldr_ctx = hxcfe_imgInitLoader(floppycontext);
	if(imgldr_ctx)
	{
		if( fsID == FS_880KB_AMIGADOS || fsID == FS_1760KB_AMIGADOS )
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
			ret = plugin_ptr[moduleID].infos_handler(imgldr_ctx,GETFUNCPTR,&func_ptr);
			if(ret==HXCFE_NOERROR)
			{
				newfloppy = calloc( 1, sizeof(HXCFE_FLOPPY) );
				if(newfloppy)
				{
					floppycontext->hxc_printf(MSG_INFO_0,"file loader found!");

					ret=func_ptr.libLoad_DiskFile(imgldr_ctx,newfloppy,path,fs_config_table[i].name);
					if(ret!=HXCFE_NOERROR)
					{
						free( newfloppy );
						newfloppy = NULL;
					}

					if(err_ret)
						*err_ret=ret;

					hxcfe_imgDeInitLoader(imgldr_ctx);
				}

				return newfloppy;
			}
		}

		floppycontext->hxc_printf(MSG_ERROR,"Bad plugin ID : 0x%x",moduleID);

		ret=HXCFE_BADPARAMETER;

		if(err_ret)
			*err_ret=ret;

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
