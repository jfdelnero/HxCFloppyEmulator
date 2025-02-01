/*
//
// Copyright (C) 2006-2025 Jean-Fran�ois DEL NERO
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
// Written by: Jean-Fran�ois DEL NERO
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

#include "fs_fat12/fs_fat12.h"
#include "fs_amigados/fs_amigados.h"
#include "fs_cpm/fs_cpm.h"



int32_t hxcfe_checkFSID(HXCFE* floppycontext,int32_t FSID)
{
	int i;

	floppycontext->hxc_printf(MSG_DEBUG,"hxcfe_checkFSID : %d",FSID);

	i=0;
	do
	{
		i++;
	}while(fs_config_table[i].name && i<FSID);

	if( !fs_config_table[i].name )
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

	floppycontext->hxc_printf(MSG_DEBUG,"hxcfe_getFSDesc : %d",FSID);

	if(hxcfe_checkFSID(floppycontext,FSID)==HXCFE_NOERROR)
	{
		return fs_config_table[FSID].desc;
	}
	else
	{
		floppycontext->hxc_printf(MSG_ERROR,"Bad FS ID : %x !",FSID);
	}

	return 0;
}

const char* hxcfe_getFSName(HXCFE* floppycontext,int32_t FSID)
{

	floppycontext->hxc_printf(MSG_DEBUG,"hxcfe_getFSName : %d",FSID);

	if(hxcfe_checkFSID(floppycontext,FSID)==HXCFE_NOERROR)
	{
		return fs_config_table[FSID].name;
	}
	else
	{
		floppycontext->hxc_printf(MSG_ERROR,"Bad FS ID : %x !",FSID);
	}

	return 0;
}

int32_t hxcfe_selectFS(HXCFE_FSMNG * fsmng, int32_t FSID)
{

	fsmng->hxcfe->hxc_printf(MSG_DEBUG,"hxcfe_selectFS : %d",FSID);

	fsmng->fs_selected = FSID;

	if( fsmng->fs_selected == FS_880KB_AMIGADOS || fsmng->fs_selected == FS_1760KB_AMIGADOS )
		init_amigados(fsmng);
	else
		init_fat12(fsmng);

	return HXCFE_NOERROR;
}

void hxcfe_deinitFsManager(HXCFE_FSMNG * fsmng)
{
	fsmng->hxcfe->hxc_printf(MSG_DEBUG,"hxcfe_deinitFsManager");

	free(fsmng);
}

int32_t hxcfe_mountImage(HXCFE_FSMNG * fsmng, HXCFE_FLOPPY *floppy)
{
	int32_t ret;
	fsmng->hxcfe->hxc_printf(MSG_DEBUG,"hxcfe_mountImage");

/*  if( fsmng->fs_selected == FS_880KB_AMIGADOS )
		return amigados_mountImage(fsmng, floppy);
	else
		return fat12_mountImage(fsmng, floppy);*/

	ret = hxcfe_selectFS(fsmng, 0);
	ret = fat12_mountImage(fsmng, floppy);
	if(ret == HXCFE_NOERROR)
		return ret;

	ret = hxcfe_selectFS(fsmng, FS_880KB_AMIGADOS);
	ret = amigados_mountImage(fsmng, floppy);
	if(ret == HXCFE_NOERROR)
		return ret;

	ret = hxcfe_selectFS(fsmng, FS_1760KB_AMIGADOS);
	ret = amigados_mountImage(fsmng, floppy);
	if(ret == HXCFE_NOERROR)
		return ret;

	return ret;

}

int32_t hxcfe_umountImage(HXCFE_FSMNG * fsmng)
{
	fsmng->hxcfe->hxc_printf(MSG_DEBUG,"hxcfe_umountImage");

	if( fsmng->fs_selected == FS_880KB_AMIGADOS || fsmng->fs_selected == FS_1760KB_AMIGADOS )
		return amigados_umountImage(fsmng);
	else
		return fat12_umountImage(fsmng);
}

int32_t hxcfe_getFreeFsSpace(HXCFE_FSMNG * fsmng)
{
	fsmng->hxcfe->hxc_printf(MSG_DEBUG,"hxcfe_getFreeFsSpace");

	if( fsmng->fs_selected == FS_880KB_AMIGADOS || fsmng->fs_selected == FS_1760KB_AMIGADOS )
		return amigados_getFreeSpace( fsmng );
	else
		return fat12_getFreeSpace( fsmng );
}

int32_t hxcfe_getTotalFsSpace(HXCFE_FSMNG * fsmng)
{
	fsmng->hxcfe->hxc_printf(MSG_DEBUG,"hxcfe_getTotalFsSpace");

	if( fsmng->fs_selected == FS_880KB_AMIGADOS || fsmng->fs_selected == FS_1760KB_AMIGADOS )
		return amigados_getTotalSpace( fsmng );
	else
		return fat12_getTotalSpace( fsmng );
}

int32_t hxcfe_openDir(HXCFE_FSMNG * fsmng, char * path)
{
	fsmng->hxcfe->hxc_printf(MSG_DEBUG,"hxcfe_openDir : %s",path);

	if( fsmng->fs_selected == FS_880KB_AMIGADOS || fsmng->fs_selected == FS_1760KB_AMIGADOS )
		return amigados_openDir(fsmng,path);
	else
		return fat12_openDir(fsmng,path);
}

int32_t hxcfe_readDir(HXCFE_FSMNG * fsmng,int32_t dirhandle,HXCFE_FSENTRY * dirent)
{
	fsmng->hxcfe->hxc_printf(MSG_DEBUG,"hxcfe_readDir : 0x%.8x",dirhandle);

	if( fsmng->fs_selected == FS_880KB_AMIGADOS || fsmng->fs_selected == FS_1760KB_AMIGADOS )
		return amigados_readDir(fsmng,dirhandle,dirent);
	else
		return fat12_readDir(fsmng,dirhandle,dirent);
}

int32_t hxcfe_closeDir(HXCFE_FSMNG * fsmng, int32_t dirhandle)
{
	fsmng->hxcfe->hxc_printf(MSG_DEBUG,"hxcfe_closeDir : 0x%.8x",dirhandle);

	if( fsmng->fs_selected == FS_880KB_AMIGADOS || fsmng->fs_selected == FS_1760KB_AMIGADOS )
		return amigados_closeDir(fsmng, dirhandle);
	else
		return fat12_closeDir(fsmng, dirhandle);
}

int32_t hxcfe_openFile(HXCFE_FSMNG * fsmng, char * filename)
{
	fsmng->hxcfe->hxc_printf(MSG_DEBUG,"hxcfe_openFile : %s",filename);

	if( fsmng->fs_selected == FS_880KB_AMIGADOS || fsmng->fs_selected == FS_1760KB_AMIGADOS )
		return amigados_openFile(fsmng,filename);
	else
		return fat12_openFile(fsmng,filename);
}

int32_t hxcfe_createFile(HXCFE_FSMNG * fsmng, char * filename)
{
	fsmng->hxcfe->hxc_printf(MSG_DEBUG,"hxcfe_createFile : %s",filename);

	if( fsmng->fs_selected == FS_880KB_AMIGADOS || fsmng->fs_selected == FS_1760KB_AMIGADOS )
		return amigados_createFile(fsmng,filename);
	else
		return fat12_createFile(fsmng,filename);
}

int32_t hxcfe_writeFile(HXCFE_FSMNG * fsmng,int32_t filehandle,uint8_t * buffer,int32_t size)
{
	fsmng->hxcfe->hxc_printf(MSG_DEBUG,"hxcfe_writeFile : 0x%.8x - 0x%.8x / %d bytes",filehandle,buffer,size);

	if( fsmng->fs_selected == FS_880KB_AMIGADOS || fsmng->fs_selected == FS_1760KB_AMIGADOS )
		return amigados_writeFile(fsmng,filehandle,buffer,size);
	else
		return fat12_writeFile(fsmng,filehandle,buffer,size);
}

int32_t hxcfe_readFile( HXCFE_FSMNG * fsmng,int32_t filehandle,uint8_t * buffer,int32_t size)
{
	fsmng->hxcfe->hxc_printf(MSG_DEBUG,"hxcfe_readFile : 0x%.8x - 0x%.8x / %d bytes",filehandle,buffer,size);

	if( fsmng->fs_selected == FS_880KB_AMIGADOS || fsmng->fs_selected == FS_1760KB_AMIGADOS )
		return amigados_readFile( fsmng,filehandle,buffer,size);
	else
		return fat12_readFile( fsmng,filehandle,buffer,size);
}

int32_t hxcfe_deleteFile(HXCFE_FSMNG * fsmng, char * filename)
{
	fsmng->hxcfe->hxc_printf(MSG_DEBUG,"hxcfe_deleteFile : %s",filename);

	if( fsmng->fs_selected == FS_880KB_AMIGADOS || fsmng->fs_selected == FS_1760KB_AMIGADOS )
		return amigados_deleteFile(fsmng,filename);
	else
		return fat12_deleteFile(fsmng,filename);
}

int32_t hxcfe_closeFile(HXCFE_FSMNG * fsmng, int32_t filehandle)
{
	fsmng->hxcfe->hxc_printf(MSG_DEBUG,"hxcfe_closeFile : 0x%.8x",filehandle);

	if( fsmng->fs_selected == FS_880KB_AMIGADOS || fsmng->fs_selected == FS_1760KB_AMIGADOS )
		return amigados_closeFile(fsmng,filehandle);
	else
		return fat12_closeFile(fsmng,filehandle);
}

int32_t hxcfe_createDir( HXCFE_FSMNG * fsmng, char * foldername)
{
	fsmng->hxcfe->hxc_printf(MSG_DEBUG,"hxcfe_createDir : %s",foldername);

	if( fsmng->fs_selected == FS_880KB_AMIGADOS || fsmng->fs_selected == FS_1760KB_AMIGADOS )
		return amigados_createDir( fsmng,foldername);
	else
		return fat12_createDir( fsmng,foldername);
}

int32_t hxcfe_removeDir( HXCFE_FSMNG * fsmng, char * foldername)
{
	fsmng->hxcfe->hxc_printf(MSG_DEBUG,"hxcfe_removeDir : %s",foldername);

	if( fsmng->fs_selected == FS_880KB_AMIGADOS || fsmng->fs_selected == FS_1760KB_AMIGADOS )
		return amigados_removeDir( fsmng,foldername);
	else
		return fat12_removeDir( fsmng,foldername);
}

int32_t hxcfe_fseek( HXCFE_FSMNG * fsmng,int32_t filehandle,int32_t offset,int32_t origin)
{
	fsmng->hxcfe->hxc_printf(MSG_DEBUG,"hxcfe_fseek : 0x%.8x - 0x%.8x (%d) ",filehandle,offset,origin);

	if( fsmng->fs_selected == FS_880KB_AMIGADOS || fsmng->fs_selected == FS_1760KB_AMIGADOS )
		return amigados_fseek(fsmng,filehandle,offset,origin);
	else
		return fat12_fseek(fsmng,filehandle,offset,origin);

}

int32_t hxcfe_ftell( HXCFE_FSMNG * fsmng,int32_t filehandle)
{
	fsmng->hxcfe->hxc_printf(MSG_DEBUG,"hxcfe_ftell : 0x%.8x",filehandle);

	if( fsmng->fs_selected == FS_880KB_AMIGADOS || fsmng->fs_selected == FS_1760KB_AMIGADOS )
		return amigados_ftell( fsmng,filehandle);
	else
		return fat12_ftell( fsmng,filehandle);
}
