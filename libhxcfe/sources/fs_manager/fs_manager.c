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
// File : fs_manager.c
// Contains: File system manager functions
//
// Written by:	DEL NERO Jean Francois
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "libhxcfe.h"
#include "fs_manager.h"
#include "fs_fat12/fs_fat12.h"

int hxcfe_checkFSID(HXCFLOPPYEMULATOR* floppycontext,int FSID)
{
	int i;

	i=0;
	do
	{
		i++;
	}while(fs_config_table[i].name && i<FSID);

	if( !fs_config_table[i].name )
	{
		return HXCFE_BADPARAMETER;
	}
	else
	{
		return HXCFE_NOERROR;
	}
}


FSMNG * hxcfe_initFsManager(HXCFLOPPYEMULATOR * hxcfe)
{
	FSMNG * fsmng;

	fsmng = malloc(sizeof(FSMNG));
	if(fsmng)
	{
		memset(fsmng,0,sizeof(FSMNG));
		fsmng->hxcfe = hxcfe;
		fsmng->sectorpertrack = 9;
		fsmng->sidepertrack = 2;
		fsmng->trackperdisk = 80;
		fsmng->sectorsize = 512;
		return fsmng;
	}
	return 0;
}

int hxcfe_getFSID(HXCFLOPPYEMULATOR* floppycontext, char * fsname)
{
	int i;

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

int	hxcfe_numberOfFS(HXCFLOPPYEMULATOR* floppycontext)
{
	int i;

	i = 0;
	while(fs_config_table[i].name)
	{
		i++;
	}
	return i;
}

const char* hxcfe_getFSDesc(HXCFLOPPYEMULATOR* floppycontext,int FSID)
{
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

const char* hxcfe_getFSName(HXCFLOPPYEMULATOR* floppycontext,int FSID)
{
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

int hxcfe_selectFS(FSMNG * fsmng, int fsid)
{
	fsmng->fs_selected = fsid;
	init_fat12(fsmng);
	return HXCFE_NOERROR;
}

void hxcfe_deinitFsManager(FSMNG * fsmng)
{
	free(fsmng);
}

int hxcfe_mountImage(FSMNG * fsmng, FLOPPY *floppy)
{
	return fat12_mountImage(fsmng, floppy);
}

int hxcfe_umountImage(FSMNG * fsmng)
{
	return fat12_umountImage(fsmng);
}

int hxcfe_openDir(FSMNG * fsmng, char * path)
{
	return fat12_openDir(fsmng,path);
}

int hxcfe_readDir(FSMNG * fsmng,int dirhandle,FSENTRY * dirent)
{
	return fat12_readDir(fsmng,dirhandle,dirent);
}

int hxcfe_closeDir(FSMNG * fsmng, int dirhandle)
{
	return fat12_closeDir(fsmng, dirhandle);
}

int hxcfe_openFile(FSMNG * fsmng, char * filename)
{
	return fat12_openFile(fsmng,filename);
}

int hxcfe_createFile(FSMNG * fsmng, char * filename)
{
	return fat12_createFile(fsmng,filename);
}

int hxcfe_writeFile(FSMNG * fsmng,int filehandle,char * buffer,int size)
{
	return fat12_writeFile(fsmng,filehandle,buffer,size);
}

int hxcfe_readFile( FSMNG * fsmng,int filehandle,char * buffer,int size)
{
	return fat12_readFile( fsmng,filehandle,buffer,size);
}

int hxcfe_deleteFile(FSMNG * fsmng, char * filename)
{
	return fat12_deleteFile(fsmng,filename);
}

int hxcfe_closeFile(FSMNG * fsmng, int filehandle)
{
	return fat12_closeFile(fsmng,filehandle);
}

int hxcfe_createDir( FSMNG * fsmng,char * foldername)
{
	return fat12_createDir( fsmng,foldername);
}

int hxcfe_removeDir( FSMNG * fsmng,char * foldername)
{
	return fat12_removeDir( fsmng,foldername);
}

int hxcfe_fseek( FSMNG * fsmng,int filehandle,long offset,int origin)
{
	return fat12_fseek(fsmng,filehandle,offset,origin);
}

int hxcfe_ftell( FSMNG * fsmng,int filehandle)
{
	return fat12_ftell( fsmng,filehandle);
}
