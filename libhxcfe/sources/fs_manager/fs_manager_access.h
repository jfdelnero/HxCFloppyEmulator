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

#ifndef _HXCFE_FSMNG_ACCESS_H_

typedef void    (*fn_fsmng_init)           (HXCFE_FSMNG * fsmng);
typedef int32_t (*fn_fsmng_mountImage)     (HXCFE_FSMNG * fsmng, HXCFE_FLOPPY *floppy);
typedef int32_t (*fn_fsmng_umountImage)    (HXCFE_FSMNG * fsmng);
typedef int32_t (*fn_fsmng_getFreeSpace)   (HXCFE_FSMNG * fsmng);
typedef int32_t (*fn_fsmng_getTotalSpace)  (HXCFE_FSMNG * fsmng);

typedef int32_t (*fn_fsmng_deleteFile)     (HXCFE_FSMNG * fsmng, char * filename);

typedef int32_t (*fn_fsmng_openFile)       (HXCFE_FSMNG * fsmng, char * filename);
typedef int32_t (*fn_fsmng_createFile)     (HXCFE_FSMNG * fsmng, char * filename);
typedef int32_t (*fn_fsmng_writeFile)      (HXCFE_FSMNG * fsmng, int32_t filehandle, unsigned char * buffer, int32_t size);
typedef int32_t (*fn_fsmng_readFile)       (HXCFE_FSMNG * fsmng, int32_t filehandle, unsigned char * buffer, int32_t size);
typedef int32_t (*fn_fsmng_ftell)          (HXCFE_FSMNG * fsmng, int32_t filehandle);
typedef int32_t (*fn_fsmng_fseek)          (HXCFE_FSMNG * fsmng, int32_t filehandle, int32_t offset, int32_t origin);
typedef int32_t (*fn_fsmng_closeFile)      (HXCFE_FSMNG * fsmng, int32_t filehandle);

typedef int32_t (*fn_fsmng_createDir)      (HXCFE_FSMNG * fsmng, char * foldername);
typedef int32_t (*fn_fsmng_removeDir)      (HXCFE_FSMNG * fsmng, char * foldername);

typedef int32_t (*fn_fsmng_openDir)        (HXCFE_FSMNG * fsmng, char * path);
typedef int32_t (*fn_fsmng_readDir)        (HXCFE_FSMNG * fsmng, int32_t dirhandle, HXCFE_FSENTRY * dirent);
typedef int32_t (*fn_fsmng_closeDir)       (HXCFE_FSMNG * fsmng, int32_t dirhandle);

typedef int32_t (*fn_fsmng_getplugininfos) (HXCFE_FSMNG * fsmng, uint32_t infotype, void * returnvalue);

typedef struct fs_plugins_ptr_
{
	fn_fsmng_init            fsmng_init;

	fn_fsmng_mountImage      fsmng_mountImage;
	fn_fsmng_umountImage     fsmng_umountImage;

	fn_fsmng_getFreeSpace    fsmng_getFreeSpace;
	fn_fsmng_getTotalSpace   fsmng_getTotalSpace;
	fn_fsmng_deleteFile      fsmng_deleteFile;

	fn_fsmng_openFile        fsmng_openFile;
	fn_fsmng_closeFile       fsmng_closeFile;

	fn_fsmng_createFile      fsmng_createFile;
	fn_fsmng_writeFile       fsmng_writeFile;
	fn_fsmng_readFile        fsmng_readFile;
	fn_fsmng_ftell           fsmng_ftell;
	fn_fsmng_fseek           fsmng_fseek;
	fn_fsmng_createDir       fsmng_createDir;
	fn_fsmng_removeDir       fsmng_removeDir;
	fn_fsmng_openDir         fsmng_openDir;
	fn_fsmng_readDir         fsmng_readDir;
	fn_fsmng_closeDir        fsmng_closeDir;

	fn_fsmng_getplugininfos  fsmng_getplugininfos;
}fs_plugins_ptr;

int libGetFSPluginInfo( HXCFE_FSMNG * fsmng_ctx, uint32_t infotype, void * returnvalue, const char * pluginid, const char * plugindesc, fs_plugins_ptr * pluginfunc );

enum {
	GETFSPLUGINID = 1,
	GETFSDESCRIPTION = 2,
	GETFSFUNCPTR = 3
};


typedef struct _fs_config
{
	char * name;
	char * desc;
	int32_t    fsID;
	int32_t    type;

	fn_fsmng_getplugininfos fsgetplug;
}fs_config;

extern fs_config fs_config_table[];

#define _HXCFE_FSMNG_ACCESS_H_

#endif
