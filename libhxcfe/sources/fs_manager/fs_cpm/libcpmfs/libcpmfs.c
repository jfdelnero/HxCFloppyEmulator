/*
//
// Copyright (C) 2006-2016 Jean-François DEL NERO
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
// File : libcpmfs.c
// Contains: CP/M file system manager
//
// Written by:	DEL NERO Jean Francois
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"

#include "libcpmfs.h"

int32_t libcpmfs_init(cpmfs * fs)
{
	memset(fs,0,sizeof(cpmfs));

	return 0;
}

int32_t libcpmfs_deinit(cpmfs * fs)
{
	return 0;
}

int32_t libcpmfs_attach_media(cpmfs * fs, fn_diskio_read rd, fn_diskio_write wr)
{
	fs->rdfn = rd;
	fs->wrfn = wr;
	return 0;
}

int32_t libcpmfs_mountImage(cpmfs * fs)
{
	return 0;
}

int32_t libcpmfs_unmountImage(cpmfs * fs)
{
	return 0;
}

void * libcpmfs_fopen(cpmfs * fs, const char *path, const char *modifiers)
{
	return 0;
}

int32_t libcpmfs_fwrite(cpmfs * fs, const void * data, int32_t size, int32_t count, void *file )
{
	return 0;
}

int32_t libcpmfs_ftell(cpmfs * fs, void *file)
{
	return 0;
}

int32_t libcpmfs_fseek(cpmfs * fs, void *file, int32_t offset, int32_t origin )
{
	return 0;
}

int32_t libcpmfs_fread(cpmfs * fs, void * data, int32_t size, int32_t count, void *file )
{
	return 0;
}

int32_t libcpmfs_feof(cpmfs * fs, void *file)
{
	return 0;
}

int32_t libcpmfs_fclose(cpmfs * fs, void *file)
{
	return 0;
}

cpmfs_dir* libcpmfs_opendir(cpmfs * fs, const char* path, cpmfs_dir *dir)
{
	return 0;
}

int32_t libcpmfs_readdir(cpmfs * fs, cpmfs_dir* dir, cpmfs_entry *entry)
{
	return 0;
}

int32_t libcpmfs_closedir(cpmfs * fs,cpmfs_dir* dir)
{
	return 0;
}

int32_t libcpmfs_createdirectory(cpmfs * fs,const char *path)
{
	return 0;
}

int32_t libcpmfs_remove(cpmfs * fs,const char * filename)
{
	return 0;
}
