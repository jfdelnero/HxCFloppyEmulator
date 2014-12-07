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
// File : fs_cpm.c
// Contains: CP/M File system manager functions
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
#include "sector_search.h"
#include "fdc_ctrl.h"
#include "fs_manager/fs_manager.h"
#include "libhxcfe.h"

#include "floppy_loader.h"
#include "floppy_utils.h"

#include "libcpmfs/libcpmfs.h"

HXCFE* floppycontext;
HXCFE_FSMNG * gb_fsmng;

cpmfs fs;

void init_cpm(HXCFE_FSMNG * fsmng)
{
	gb_fsmng = fsmng;
	libcpmfs_init(&fs);
}

static int media_read(uint32_t sector, unsigned char *buffer, uint32_t sector_count)
{
		return 0;
}

static int media_write(uint32_t sector, unsigned char *buffer,uint32_t sector_count)
{
		return 0;
}

int cpm_mountImage(HXCFE_FSMNG * fsmng, HXCFE_FLOPPY *floppy)
{
	unsigned char sectorbuffer[1024];
	int nbsector,fdcstatus,badsectorfound;

	badsectorfound = 0;

	if(!floppy || !fsmng)
		return HXCFE_BADPARAMETER;

	fsmng->fp = floppy;

	fsmng->trackperdisk = fsmng->fp->floppyNumberOfTrack;
	fsmng->sectorpertrack = 9;
	fsmng->sidepertrack = fsmng->fp->floppyNumberOfSide;
	fsmng->sectorsize = 512;

	if(fsmng->fdc)
		hxcfe_deinitFDC (fsmng->fdc);

	fsmng->fdc = hxcfe_initFDC (fsmng->hxcfe);
	if(fsmng->fdc)
	{
		if(hxcfe_insertDiskFDC (fsmng->fdc,floppy) == HXCFE_NOERROR)
		{
			// Count the number of sector

			// Side 1 checking
			nbsector = 0;
			while(hxcfe_readSectorFDC(fsmng->fdc,0,1,(unsigned char)(1+nbsector),512,ISOIBM_MFM_ENCODING,1,(unsigned char*)sectorbuffer,sizeof(sectorbuffer),&fdcstatus))
			{
				if(fdcstatus == FDC_BAD_DATA_CRC)
					badsectorfound++;
				nbsector++;
			}

			if(nbsector)
				fsmng->sidepertrack = 2;

			nbsector = 0;
			while(hxcfe_readSectorFDC(fsmng->fdc,0,0,(unsigned char)(1+nbsector),512,ISOIBM_MFM_ENCODING,1,(unsigned char*)sectorbuffer,sizeof(sectorbuffer),&fdcstatus))
			{
				if(fdcstatus == FDC_BAD_DATA_CRC)
					badsectorfound++;
				nbsector++;
			}

			// Retry with 1024 bytes per sector
			if(!nbsector)
			{
				while(hxcfe_readSectorFDC(fsmng->fdc,0,0,(unsigned char)(1+nbsector),1024,ISOIBM_MFM_ENCODING,1,(unsigned char*)sectorbuffer,sizeof(sectorbuffer),&fdcstatus))
				{
					if(fdcstatus == FDC_BAD_DATA_CRC)
						badsectorfound++;

					nbsector++;
				}

				if(nbsector)
					fsmng->sectorsize = 1024;
			}

			fsmng->sectorpertrack = nbsector;


			gb_fsmng->hxcfe->hxc_printf(MSG_DEBUG,"CPMFS : %d Sectors per track (%d Bytes per sector)",fsmng->sectorpertrack,fsmng->sectorsize);

			if(fsmng->sectorpertrack && !badsectorfound)
			{
				/*if ((err=Device_open(&drive.dev,image,O_RDONLY,devopts)))
				{
					fprintf(stderr,"%s: can not open %s (%s)\n",cmd,image,err);
				}*/

				/* Attach media access functions to library*/
				if (libcpmfs_attach_media(&fs, media_read, media_write))
				{
					gb_fsmng->hxcfe->hxc_printf(MSG_DEBUG,"CPMFS : Media attach failed");
				}

				return HXCFE_NOERROR;
			}
		}
	}

	return HXCFE_INTERNALERROR;
}

int cpm_umountImage(HXCFE_FSMNG * fsmng)
{
	if(fsmng->fdc)
	{
		hxcfe_deinitFDC (fsmng->fdc);
		fsmng->fdc = 0;
	}

	return HXCFE_NOERROR;
}

int cpm_getFreeSpace(HXCFE_FSMNG * fsmng)
{
	return 0;
}

int cpm_getTotalSpace(HXCFE_FSMNG * fsmng)
{
	return 0;
}

int cpm_openDir(HXCFE_FSMNG * fsmng, char * path)
{
	cpmfs_dir * dir;
	int i;

	dir = malloc(sizeof(cpmfs_dir));
	if(dir)
	{
		memset(dir,0,sizeof(cpmfs_dir));

		i = 0;
		while(fsmng->dirhandletable[i] && i<128)
		{
			i++;
		}

		if(i == 128)
		{
			free(dir);
			return HXCFE_ACCESSERROR;
		}

		if(libcpmfs_opendir(&fs,path, dir))
		{
			fsmng->dirhandletable[i] = dir;
			return i+1;
		}
		else
		{
			free(dir);

			return HXCFE_ACCESSERROR;
		}
	}

	return HXCFE_ACCESSERROR;
}

int cpm_readDir(HXCFE_FSMNG * fsmng,int dirhandle,HXCFE_FSENTRY * dirent)
{
	int ret;
	cpmfs_entry entry;

	if(dirhandle<128)
	{
		if(fsmng->dirhandletable[dirhandle-1])
		{
			ret = libcpmfs_readdir(&fs, fsmng->dirhandletable[dirhandle-1], &entry);
			if(!ret)
			{
				strcpy(dirent->entryname,(char*)entry.filename);
				dirent->size = entry.size;

				dirent->isdir = 0;
				if(entry.is_dir)
					dirent->isdir = 1;

				dirent->flags = 0;

				return HXCFE_VALIDFILE;
			}

			return HXCFE_NOERROR;
		}
	}

	return HXCFE_ACCESSERROR;
}

int cpm_closeDir(HXCFE_FSMNG * fsmng, int dirhandle)
{
	if(dirhandle<128)
	{
		if(fsmng->dirhandletable[dirhandle-1])
		{
			free(fsmng->dirhandletable[dirhandle-1]);
			fsmng->dirhandletable[dirhandle-1] = 0;
			return HXCFE_NOERROR;
		}
	}
	return HXCFE_ACCESSERROR;
}

int cpm_openFile(HXCFE_FSMNG * fsmng, char * filename)
{
	void *file;
	int i;

	i = 0;
	while(fsmng->handletable[i] && i<128)
	{
		i++;
	}
	if(i == 128) return HXCFE_ACCESSERROR;

	file = libcpmfs_fopen(&fs,filename, "r");
	if(file)
	{
		fsmng->handletable[i] = file;
		return i+1;
	}

	return HXCFE_ACCESSERROR;
}

int cpm_createFile(HXCFE_FSMNG * fsmng, char * filename)
{
    void *file;
	int i;

	i = 0;
	while(fsmng->handletable[i] && i<128)
	{
		i++;
	}
	if(i == 128) return HXCFE_ACCESSERROR;

	file = libcpmfs_fopen(&fs,filename, "wb");
	if(file)
	{
		fsmng->handletable[i] = file;
		return i+1;
	}

	return HXCFE_ACCESSERROR;
}

int cpm_writeFile(HXCFE_FSMNG * fsmng,int filehandle,unsigned char * buffer,int size)
{
	int byteswrite;
	if(filehandle<128)
	{
		if(fsmng->handletable[filehandle-1])
		{
			byteswrite = libcpmfs_fwrite(&fs,buffer, 1, size, fsmng->handletable[filehandle-1]);
			return byteswrite;
		}
	}
	return HXCFE_ACCESSERROR;
}

int cpm_readFile( HXCFE_FSMNG * fsmng,int filehandle,unsigned char * buffer,int size)
{
	int bytesread;
	if(filehandle && filehandle<128)
	{
		if(fsmng->handletable[filehandle-1])
		{
			bytesread = libcpmfs_fread(&fs,buffer, 1, size, fsmng->handletable[filehandle-1]);
			return bytesread;
		}
	}
	return HXCFE_ACCESSERROR;
}

int cpm_deleteFile(HXCFE_FSMNG * fsmng, char * filename)
{
	if(libcpmfs_remove(&fs,filename)>=0)
	{
		return HXCFE_NOERROR;
	}

	return HXCFE_ACCESSERROR;
}

int cpm_closeFile( HXCFE_FSMNG * fsmng,int filehandle)
{
	if(filehandle && filehandle<128)
	{
		if(fsmng->handletable[filehandle-1])
		{
			libcpmfs_fclose(&fs,fsmng->handletable[filehandle-1]);
			fsmng->handletable[filehandle-1] = 0;
			return HXCFE_NOERROR;
		}
	}
	return HXCFE_ACCESSERROR;
}

int cpm_createDir( HXCFE_FSMNG * fsmng,char * foldername)
{
	if(libcpmfs_createdirectory(&fs,foldername))
	{
		return HXCFE_NOERROR;
	}
	return HXCFE_ACCESSERROR;
}

int cpm_removeDir( HXCFE_FSMNG * fsmng,char * foldername)
{
	return cpm_deleteFile(fsmng, foldername);
}

int cpm_ftell( HXCFE_FSMNG * fsmng,int filehandle)
{
	if(filehandle && filehandle<128)
	{
		if(fsmng->handletable[filehandle-1])
		{
			return libcpmfs_ftell(&fs,fsmng->handletable[filehandle-1]);
		}
	}

	return HXCFE_ACCESSERROR;
}

int cpm_fseek( HXCFE_FSMNG * fsmng,int filehandle,int32_t offset,int origin)
{
	if(filehandle && filehandle<128)
	{
		if(fsmng->handletable[filehandle-1])
		{
			return libcpmfs_fseek(&fs,fsmng->handletable[filehandle-1],offset,origin);
		}
	}

	return HXCFE_ACCESSERROR;
}