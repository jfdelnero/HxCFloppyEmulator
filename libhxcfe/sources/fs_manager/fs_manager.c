/*
//
// Copyright (C) 2006-2026 Jean-François DEL NERO
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
// File : fs_manager.c
// Contains: File system manager functions
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
#include "sector_search.h"
#include "fdc_ctrl.h"
#include "fs_manager.h"
#include "libhxcfe.h"
#include "fs_manager_access.h"

#include "fs_fat12/fs_fat12.h"
#include "fs_amigados/fs_amigados.h"
#include "fs_cpm/fs_cpm.h"

fs_config fs_config_table[]=
{
	{"fatst",       "",FS_720KB_ATARI_FAT12,0,                                                        (fn_fsmng_getplugininfos)fat12_libGetPluginInfo},
	{"fatst902",    "",FS_902KB_ATARI_FAT12,0,                                                        (fn_fsmng_getplugininfos)fat12_libGetPluginInfo},
	{"fatst360",    "",FS_360KB_ATARI_FAT12,0,                                                        (fn_fsmng_getplugininfos)fat12_libGetPluginInfo},

	{"amigados",    "3\"5        880KB DSDD AmigaDOS",FS_880KB_AMIGADOS,1,                            (fn_fsmng_getplugininfos)amigados_libGetPluginInfo},
	{"amigados_hd", "3\"5       1760KB DSHD AmigaDOS",FS_1760KB_AMIGADOS,2,                           (fn_fsmng_getplugininfos)amigados_libGetPluginInfo},

	{"fat160a",     "5\"25 & 8\" 160KB SSDD 300RPM FAT12",FS_5P25_300RPM_160KB_MSDOS_FAT12,0,         (fn_fsmng_getplugininfos)fat12_libGetPluginInfo},
	{"fat160b",     "5\"25 & 8\" 160KB SSDD 360RPM FAT12",FS_5P25_360RPM_160KB_MSDOS_FAT12,0,         (fn_fsmng_getplugininfos)fat12_libGetPluginInfo},

	{"fat180a",     "5\"25       180KB SSDD 300RPM FAT12",FS_5P25_300RPM_180KB_MSDOS_FAT12,0,         (fn_fsmng_getplugininfos)fat12_libGetPluginInfo},
	{"fat180b",     "5\"25       180KB SSDD 360RPM FAT12",FS_5P25_360RPM_180KB_MSDOS_FAT12,0,         (fn_fsmng_getplugininfos)fat12_libGetPluginInfo},

	{"fat320ssa",   "5\"25       320KB SSDD 300RPM FAT12",FS_5P25_SS_300RPM_320KB_MSDOS_FAT12,0,      (fn_fsmng_getplugininfos)fat12_libGetPluginInfo},
	{"fat320ssb",   "5\"25       320KB SSDD 360RPM FAT12",FS_5P25_SS_360RPM_320KB_MSDOS_FAT12,0,      (fn_fsmng_getplugininfos)fat12_libGetPluginInfo},

	{"fat320dsa",   "5\"25       320KB DSDD 300RPM FAT12",FS_5P25_DS_300RPM_320KB_MSDOS_FAT12,0,      (fn_fsmng_getplugininfos)fat12_libGetPluginInfo},
	{"fat320dsb",   "5\"25       320KB DSDD 360RPM FAT12",FS_5P25_DS_360RPM_320KB_MSDOS_FAT12,0,      (fn_fsmng_getplugininfos)fat12_libGetPluginInfo},

	{"fat360a",     "5\"25 & 8\" 360KB DSDD 300RPM FAT12",FS_5P25_DS_300RPM_360KB_MSDOS_FAT12,0,      (fn_fsmng_getplugininfos)fat12_libGetPluginInfo},
	{"fat360b",     "5\"25 & 8\" 360KB DSDD 360RPM FAT12",FS_5P25_DS_360RPM_360KB_MSDOS_FAT12,0,      (fn_fsmng_getplugininfos)fat12_libGetPluginInfo},

	{"fat640",      "3\"5        640KB DSDD FAT12",FS_3P5_DS_300RPM_640KB_MSDOS_FAT12,0,              (fn_fsmng_getplugininfos)fat12_libGetPluginInfo},

	{"fat720",      "3\"5        720KB DSDD FAT12",FS_720KB_MSDOS_FAT12,0,                            (fn_fsmng_getplugininfos)fat12_libGetPluginInfo},
	{"fat738",      "3\"5        738KB DSDD FAT12",FS_738KB_MSDOS_FAT12,0,                            (fn_fsmng_getplugininfos)fat12_libGetPluginInfo},
	{"fat800",      "3\"5        800KB DSDD FAT12",FS_800KB_MSDOS_FAT12,0,                            (fn_fsmng_getplugininfos)fat12_libGetPluginInfo},
	{"fat820",      "3\"5        820KB DSDD FAT12",FS_820KB_MSDOS_FAT12,0,                            (fn_fsmng_getplugininfos)fat12_libGetPluginInfo},

	{"fat1200",     "5\"25       1.2MB DSHD FAT12",FS_5P25_300RPM_1200KB_MSDOS_FAT12,0,               (fn_fsmng_getplugininfos)fat12_libGetPluginInfo},
	{"fat1230",     "5\"25       1.23MB DSHD FAT12",FS_5P25_300RPM_1230KB_MSDOS_FAT12,0,              (fn_fsmng_getplugininfos)fat12_libGetPluginInfo},

	{"fat1440",     "3\"5        1.44MB DSHD FAT12",FS_1_44MB_MSDOS_FAT12,0,                          (fn_fsmng_getplugininfos)fat12_libGetPluginInfo},
	{"fat1476",     "3\"5        1.478MB DSHD FAT12",FS_1_476MB_MSDOS_FAT12,0,                        (fn_fsmng_getplugininfos)fat12_libGetPluginInfo},

	{"fat1600",     "3\"5        1.6MB DSHD FAT12",FS_1_600MB_MSDOS_FAT12,0,                          (fn_fsmng_getplugininfos)fat12_libGetPluginInfo},
	{"fat1640",     "3\"5        1.64MB DSHD FAT12",FS_1_640MB_MSDOS_FAT12,0,                         (fn_fsmng_getplugininfos)fat12_libGetPluginInfo},
	{"fat1680",     "3\"5        1.68MB DSHD FAT12",FS_1_68MB_MSDOS_FAT12,0,                          (fn_fsmng_getplugininfos)fat12_libGetPluginInfo},

	{"fat1722",     "3\"5        1.722MB DSHD FAT12",FS_1_722MB_MSDOS_FAT12,0,                        (fn_fsmng_getplugininfos)fat12_libGetPluginInfo},
	{"fat1743",     "3\"5        1.743MB DSHD FAT12",FS_1_743MB_MSDOS_FAT12,0,                        (fn_fsmng_getplugininfos)fat12_libGetPluginInfo},
	{"fat1764",     "3\"5        1.764MB DSHD FAT12",FS_1_764MB_MSDOS_FAT12,0,                        (fn_fsmng_getplugininfos)fat12_libGetPluginInfo},
	{"fat1785",     "3\"5        1.785MB DSHD FAT12",FS_1_785MB_MSDOS_FAT12,0,                        (fn_fsmng_getplugininfos)fat12_libGetPluginInfo},

	{"fat2540",     "3\"5        2.50MB DSDD FAT12",FS_2_50MB_MSDOS_FAT12,0,                          (fn_fsmng_getplugininfos)fat12_libGetPluginInfo},

	{"fat2880",     "3\"5        2.88MB DSED FAT12",FS_2_88MB_MSDOS_FAT12,0,                          (fn_fsmng_getplugininfos)fat12_libGetPluginInfo},
	{"fat3381",     "3\"5        3.38MB DSHD FAT12",FS_3_38MB_MSDOS_FAT12,0,                          (fn_fsmng_getplugininfos)fat12_libGetPluginInfo},
	{"fatbigst",    "3\"5        3.42MB DSDD Atari FAT12",FS_3_42MB_ATARI_FAT12,0,                    (fn_fsmng_getplugininfos)fat12_libGetPluginInfo},

	{"fat5355",     "3\"5        5.35MB DSHD FAT12",FS_5_35MB_MSDOS_FAT12,0,                          (fn_fsmng_getplugininfos)fat12_libGetPluginInfo},
	{"fat5355b",    "3\"5        5.35MB DSHD FAT12",FS_5_35MB_B_MSDOS_FAT12,0,                        (fn_fsmng_getplugininfos)fat12_libGetPluginInfo},

	{"fat6789",     "3\"5        6.78MB DSHD FAT12",FS_6_78MB_MSDOS_FAT12,0,                          (fn_fsmng_getplugininfos)fat12_libGetPluginInfo},
	{"fatbig",      "",FS_16MB_MSDOS_FAT12,0,                                                         (fn_fsmng_getplugininfos)fat12_libGetPluginInfo},
	{"fat4572",     "3\"5        4.50MB DSHD FAT12",FS_4_50MB_MSDOS_FAT12,0,                          (fn_fsmng_getplugininfos)fat12_libGetPluginInfo},

	{0,0,0,0, NULL}
};

static int find_FSID_idx(int32_t FSID)
{
	int i;

	i=0;
	do
	{
		i++;
	}while(fs_config_table[i].name && (fs_config_table[i].fsID != FSID) );

	if(fs_config_table[i].name)
	{
		return i;
	}

	return -1;
}

int32_t hxcfe_checkFSID(HXCFE* floppycontext,int32_t FSID)
{
	int idx;

	floppycontext->hxc_printf(MSG_DEBUG,"hxcfe_checkFSID : %d",FSID);

	idx = find_FSID_idx(FSID);

	if( idx < 0 )
	{
		floppycontext->hxc_printf(MSG_DEBUG,"hxcfe_checkFSID : Error !");
		return HXCFE_BADPARAMETER;
	}
	else
	{
		return HXCFE_NOERROR;
	}
}

HXCFE_FSMNG * hxcfe_initFsManager(HXCFE * hxcfe)
{
	HXCFE_FSMNG * fsmng;

	hxcfe->hxc_printf(MSG_DEBUG,"hxcfe_initFsManager");

	fsmng = malloc(sizeof(HXCFE_FSMNG));
	if(fsmng)
	{
		memset(fsmng,0,sizeof(HXCFE_FSMNG));
		fsmng->hxcfe = hxcfe;
		fsmng->sectorpertrack = 9;
		fsmng->sidepertrack = 2;
		fsmng->trackperdisk = 80;
		fsmng->sectorsize = 512;
		fsmng->fs_selected = -1;

		fsmng->fn = calloc(1, sizeof(fs_plugins_ptr));
		return fsmng;
	}
	hxcfe->hxc_printf(MSG_DEBUG,"hxcfe_initFsManager Error!");
	return 0;
}

int32_t hxcfe_getFSID(HXCFE* floppycontext, char * fsname)
{
	int i;

	floppycontext->hxc_printf(MSG_DEBUG,"hxcfe_getFSID : %s",fsname);

	i = 0;
	while(fs_config_table[i].name)
	{
		if( !strcmp(fs_config_table[i].name,fsname) )
		{
			return fs_config_table[i].fsID;
		}
		i++;
	}
	return 0;
}

int32_t hxcfe_numberOfFS(HXCFE* floppycontext)
{
	int i;

	i = 0;
	while(fs_config_table[i].name)
	{
		i++;
	}

	floppycontext->hxc_printf(MSG_DEBUG,"hxcfe_numberOfFS : return %d",i);

	return i;
}

const char* hxcfe_getFSDesc(HXCFE* floppycontext,int32_t FSID)
{
	int idx;

	floppycontext->hxc_printf(MSG_DEBUG,"hxcfe_getFSDesc : %d",FSID);

	idx = find_FSID_idx(FSID);

	if(idx >= 0)
	{
		return fs_config_table[idx].desc;
	}
	else
	{
		floppycontext->hxc_printf(MSG_ERROR,"Bad FS ID : %x !",FSID);
	}

	return 0;
}

const char* hxcfe_getFSName(HXCFE* floppycontext,int32_t FSID)
{
	int idx;

	floppycontext->hxc_printf(MSG_DEBUG,"hxcfe_getFSName : %d",FSID);

	idx = find_FSID_idx(FSID);

	if(idx >= 0)
	{
		return fs_config_table[idx].name;
	}
	else
	{
		floppycontext->hxc_printf(MSG_ERROR,"Bad FS ID : %x !",FSID);
	}

	return 0;
}

int32_t hxcfe_selectFS(HXCFE_FSMNG * fsmng, int32_t FSID)
{
	int idx;

	fsmng->hxcfe->hxc_printf(MSG_DEBUG,"hxcfe_selectFS : %d",FSID);

	idx = find_FSID_idx(FSID);

	if(idx >= 0)
	{
		fsmng->fs_selected = idx;

		fs_config_table[idx].fsgetplug(fsmng, GETFSFUNCPTR, fsmng->fn);

		((fs_plugins_ptr*)(fsmng->fn))->fsmng_init(fsmng);

		return HXCFE_NOERROR;
	}
	else
	{
		fsmng->fs_selected = -1;
		return HXCFE_BADPARAMETER;
	}

}

void hxcfe_deinitFsManager(HXCFE_FSMNG * fsmng)
{
	fsmng->hxcfe->hxc_printf(MSG_DEBUG,"hxcfe_deinitFsManager");

	free(fsmng);
}

int32_t hxcfe_mountImage(HXCFE_FSMNG * fsmng, HXCFE_FLOPPY *floppy)
{
	int i;
	int32_t ret;
	int fs_list[]={FS_720KB_MSDOS_FAT12,FS_880KB_AMIGADOS,FS_1760KB_AMIGADOS, -1};

	fsmng->hxcfe->hxc_printf(MSG_DEBUG,"hxcfe_mountImage");

	i = 0;
	do
	{
		ret = hxcfe_selectFS(fsmng, fs_list[i]);

		ret = HXCFE_UNSUPPORTEDFILE;
		if(fsmng->fs_selected >= 0)
		{
			if( ((fs_plugins_ptr*)(fsmng->fn))->fsmng_mountImage )
				ret = ((fs_plugins_ptr*)(fsmng->fn))->fsmng_mountImage(fsmng, floppy);
		}

		i++;
	}while(fs_list[i] >= 0 && ret != HXCFE_NOERROR);

	return ret;

}

int32_t hxcfe_umountImage(HXCFE_FSMNG * fsmng)
{
	fsmng->hxcfe->hxc_printf(MSG_DEBUG,"hxcfe_umountImage");

	if(fsmng->fs_selected >= 0)
	{
		if( ((fs_plugins_ptr*)(fsmng->fn))->fsmng_umountImage )
			return ((fs_plugins_ptr*)(fsmng->fn))->fsmng_umountImage(fsmng);
	}

	return HXCFE_UNSUPPORTEDFILE;
}

int32_t hxcfe_getFreeFsSpace(HXCFE_FSMNG * fsmng)
{
	fsmng->hxcfe->hxc_printf(MSG_DEBUG,"hxcfe_getFreeFsSpace");

	if(fsmng->fs_selected >= 0)
	{
		if( ((fs_plugins_ptr*)(fsmng->fn))->fsmng_getFreeSpace )
			return ((fs_plugins_ptr*)(fsmng->fn))->fsmng_getFreeSpace(fsmng);
	}

	return HXCFE_UNSUPPORTEDFILE;
}

int32_t hxcfe_getTotalFsSpace(HXCFE_FSMNG * fsmng)
{
	fsmng->hxcfe->hxc_printf(MSG_DEBUG,"hxcfe_getTotalFsSpace");

	if(fsmng->fs_selected >= 0)
	{
		if( ((fs_plugins_ptr*)(fsmng->fn))->fsmng_getTotalSpace )
			return ((fs_plugins_ptr*)(fsmng->fn))->fsmng_getTotalSpace(fsmng);
	}

	return HXCFE_UNSUPPORTEDFILE;
}

int32_t hxcfe_openDir(HXCFE_FSMNG * fsmng, char * path)
{
	fsmng->hxcfe->hxc_printf(MSG_DEBUG,"hxcfe_openDir : %s",path);

	if(fsmng->fs_selected >= 0)
	{
		if( ((fs_plugins_ptr*)(fsmng->fn))->fsmng_openDir )
			return ((fs_plugins_ptr*)(fsmng->fn))->fsmng_openDir(fsmng,path);
	}

	return HXCFE_UNSUPPORTEDFILE;
}

int32_t hxcfe_readDir(HXCFE_FSMNG * fsmng,int32_t dirhandle,HXCFE_FSENTRY * dirent)
{
	fsmng->hxcfe->hxc_printf(MSG_DEBUG,"hxcfe_readDir : 0x%.8x",dirhandle);

	if(fsmng->fs_selected >= 0)
	{
		if( ((fs_plugins_ptr*)(fsmng->fn))->fsmng_readDir )
			return ((fs_plugins_ptr*)(fsmng->fn))->fsmng_readDir(fsmng,dirhandle,dirent);
	}

	return HXCFE_UNSUPPORTEDFILE;
}

int32_t hxcfe_closeDir(HXCFE_FSMNG * fsmng, int32_t dirhandle)
{
	fsmng->hxcfe->hxc_printf(MSG_DEBUG,"hxcfe_closeDir : 0x%.8x",dirhandle);

	if(fsmng->fs_selected >= 0)
	{
		if( ((fs_plugins_ptr*)(fsmng->fn))->fsmng_closeDir )
			return ((fs_plugins_ptr*)(fsmng->fn))->fsmng_closeDir(fsmng,dirhandle);
	}

	return HXCFE_UNSUPPORTEDFILE;
}

int32_t hxcfe_openFile(HXCFE_FSMNG * fsmng, char * filename)
{
	fsmng->hxcfe->hxc_printf(MSG_DEBUG,"hxcfe_openFile : %s",filename);

	if(fsmng->fs_selected >= 0)
	{
		if( ((fs_plugins_ptr*)(fsmng->fn))->fsmng_openFile )
			return ((fs_plugins_ptr*)(fsmng->fn))->fsmng_openFile(fsmng,filename);
	}

	return HXCFE_UNSUPPORTEDFILE;
}

int32_t hxcfe_createFile(HXCFE_FSMNG * fsmng, char * filename)
{
	fsmng->hxcfe->hxc_printf(MSG_DEBUG,"hxcfe_createFile : %s",filename);

	if(fsmng->fs_selected >= 0)
	{
		if( ((fs_plugins_ptr*)(fsmng->fn))->fsmng_createFile )
			return ((fs_plugins_ptr*)(fsmng->fn))->fsmng_createFile(fsmng,filename);
	}

	return HXCFE_UNSUPPORTEDFILE;
}

int32_t hxcfe_writeFile(HXCFE_FSMNG * fsmng,int32_t filehandle,uint8_t * buffer,int32_t size)
{
	fsmng->hxcfe->hxc_printf(MSG_DEBUG,"hxcfe_writeFile : 0x%.8x - 0x%.8x / %d bytes",filehandle,buffer,size);

	if(fsmng->fs_selected >= 0)
	{
		if( ((fs_plugins_ptr*)(fsmng->fn))->fsmng_writeFile )
			return ((fs_plugins_ptr*)(fsmng->fn))->fsmng_writeFile(fsmng,filehandle,buffer,size);
	}

	return HXCFE_UNSUPPORTEDFILE;
}

int32_t hxcfe_readFile( HXCFE_FSMNG * fsmng,int32_t filehandle,uint8_t * buffer,int32_t size)
{
	fsmng->hxcfe->hxc_printf(MSG_DEBUG,"hxcfe_readFile : 0x%.8x - 0x%.8x / %d bytes",filehandle,buffer,size);

	if(fsmng->fs_selected >= 0)
	{
		if( ((fs_plugins_ptr*)(fsmng->fn))->fsmng_readFile )
			return ((fs_plugins_ptr*)(fsmng->fn))->fsmng_readFile(fsmng,filehandle,buffer,size);
	}

	return HXCFE_UNSUPPORTEDFILE;
}

int32_t hxcfe_deleteFile(HXCFE_FSMNG * fsmng, char * filename)
{
	fsmng->hxcfe->hxc_printf(MSG_DEBUG,"hxcfe_deleteFile : %s",filename);

	if(fsmng->fs_selected >= 0)
	{
		if( ((fs_plugins_ptr*)(fsmng->fn))->fsmng_deleteFile )
			return ((fs_plugins_ptr*)(fsmng->fn))->fsmng_deleteFile(fsmng,filename);
	}

	return HXCFE_UNSUPPORTEDFILE;
}

int32_t hxcfe_closeFile(HXCFE_FSMNG * fsmng, int32_t filehandle)
{
	fsmng->hxcfe->hxc_printf(MSG_DEBUG,"hxcfe_closeFile : 0x%.8x",filehandle);

	if(fsmng->fs_selected >= 0)
	{
		if( ((fs_plugins_ptr*)(fsmng->fn))->fsmng_closeFile )
			return ((fs_plugins_ptr*)(fsmng->fn))->fsmng_closeFile(fsmng,filehandle);
	}

	return HXCFE_UNSUPPORTEDFILE;
}

int32_t hxcfe_createDir( HXCFE_FSMNG * fsmng, char * foldername)
{
	fsmng->hxcfe->hxc_printf(MSG_DEBUG,"hxcfe_createDir : %s",foldername);

	if(fsmng->fs_selected >= 0)
	{
		if( ((fs_plugins_ptr*)(fsmng->fn))->fsmng_createDir )
			return ((fs_plugins_ptr*)(fsmng->fn))->fsmng_createDir(fsmng,foldername);
	}

	return HXCFE_UNSUPPORTEDFILE;
}

int32_t hxcfe_removeDir( HXCFE_FSMNG * fsmng, char * foldername)
{
	fsmng->hxcfe->hxc_printf(MSG_DEBUG,"hxcfe_removeDir : %s",foldername);

	if(fsmng->fs_selected >= 0)
	{
		if( ((fs_plugins_ptr*)(fsmng->fn))->fsmng_removeDir )
			return ((fs_plugins_ptr*)(fsmng->fn))->fsmng_removeDir(fsmng,foldername);
	}

	return HXCFE_UNSUPPORTEDFILE;
}

int32_t hxcfe_fseek( HXCFE_FSMNG * fsmng,int32_t filehandle,int32_t offset,int32_t origin)
{
	fsmng->hxcfe->hxc_printf(MSG_DEBUG,"hxcfe_fseek : 0x%.8x - 0x%.8x (%d) ",filehandle,offset,origin);

	if(fsmng->fs_selected >= 0)
	{
		if( ((fs_plugins_ptr*)(fsmng->fn))->fsmng_fseek )
			return ((fs_plugins_ptr*)(fsmng->fn))->fsmng_fseek(fsmng,filehandle,offset,origin);
	}

	return HXCFE_UNSUPPORTEDFILE;
}

int32_t hxcfe_ftell( HXCFE_FSMNG * fsmng,int32_t filehandle)
{
	fsmng->hxcfe->hxc_printf(MSG_DEBUG,"hxcfe_ftell : 0x%.8x",filehandle);

	if(fsmng->fs_selected >= 0)
	{
		if( ((fs_plugins_ptr*)(fsmng->fn))->fsmng_ftell )
			return ((fs_plugins_ptr*)(fsmng->fn))->fsmng_ftell(fsmng,filehandle);
	}

	return HXCFE_UNSUPPORTEDFILE;
}

int libGetFSPluginInfo( HXCFE_FSMNG * fsmng_ctx, uint32_t infotype, void * returnvalue, const char * pluginid, const char * plugindesc, fs_plugins_ptr * pluginfunc )
{
	if(fsmng_ctx)
	{
		if(returnvalue)
		{
			switch(infotype)
			{
				case GETFSPLUGINID:
					*(char**)(returnvalue)=(char*)pluginid;
					break;

				case GETFSDESCRIPTION:
					*(char**)(returnvalue)=(char*)plugindesc;
					break;

				case GETFSFUNCPTR:
					memcpy(returnvalue,pluginfunc,sizeof(fs_plugins_ptr));
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
