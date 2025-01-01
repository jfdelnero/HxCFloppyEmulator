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
// File : fs_fat12.c
// Contains: DOS/FAT12 File system manager functions
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
#include "fs_manager/fs_manager.h"
#include "libhxcfe.h"

#include "floppy_loader.h"
#include "floppy_utils.h"

#include "thirdpartylibs/FATIOlib/fat_opts.h"
#include "thirdpartylibs/FATIOlib/fat_misc.h"
#include "thirdpartylibs/FATIOlib/fat_defs.h"
#include "thirdpartylibs/FATIOlib/fat_filelib.h"
//#include "thirdpartylibs/FATIOlib/conf.h"


fn_diskio_read media_read_callback;
fn_diskio_write media_write_callback;

static HXCFE_FSMNG * gb_fsmng;

static void lba2chs(HXCFE_FSMNG * fsmng,int32_t lba, int32_t *track, int32_t *head, int32_t *sector)
{

	if(fsmng)
	{
		if(track)
		{
			if(fsmng->sectorpertrack &&  fsmng->sidepertrack)
			{
				*track = lba / (fsmng->sectorpertrack * fsmng->sidepertrack);
			}
			else
			{
				*track = 0;
			}
		}

		if(head)
		{
			if(fsmng->sectorpertrack && fsmng->sidepertrack)
			{
				*head = (lba / fsmng->sectorpertrack) % fsmng->sidepertrack;
			}
			else
			{
				*head = 0;
			}
		}

		if(sector)
		{
			if(fsmng->sectorpertrack)
			{
				*sector = (lba % fsmng->sectorpertrack) + 1;
			}
			else
			{
				*sector = 0;
			}
		}
	}
}

static int32_t media_read(uint32_t sector, uint8_t *buffer, uint32_t sector_count)
{
	uint32_t i,c;
	int32_t fp_track,fp_head,fp_sector,fdcstatus;

	fp_track = 0;
	fp_sector = 0;
	fp_head = 0;

	lba2chs(gb_fsmng,sector, &fp_track,&fp_head,&fp_sector);
	gb_fsmng->hxcfe->hxc_printf(MSG_DEBUG,"FAT12FS : media_read, sector: 0x%.8X, sector count : %d, Track: %d, Side: %d, Sector: %d",sector,sector_count,fp_track,fp_head,fp_sector);
	c=0;
	for(i=0;i<sector_count;i++)
	{
		lba2chs(gb_fsmng,sector + i, &fp_track,&fp_head,&fp_sector);
		if(hxcfe_readSectorFDC (gb_fsmng->fdc,(unsigned char)fp_track,(unsigned char)fp_head,(unsigned char)fp_sector,gb_fsmng->sectorsize,ISOIBM_MFM_ENCODING,1,&buffer[i*gb_fsmng->sectorsize],gb_fsmng->sectorsize,&fdcstatus) == 1)
		{
			if(!fdcstatus)
			{
				c++;
			}
			else
			{
				gb_fsmng->hxcfe->hxc_printf(MSG_DEBUG,"FAT12FS : media_read !!! ERROR !!!, sector: 0x%.8X, sector count : %d, Track: %d, Side: %d, Sector: %d, FDC Status : 0x%.2x",sector,sector_count,fp_track,fp_head,fp_sector,fdcstatus);
			}
		}
	}

	if( c == sector_count )
		return 1;
	else
		return 0;
}

static int32_t media_write(uint32_t sector, uint8_t *buffer,uint32_t sector_count)
{
	uint32_t i,c;
	int32_t fp_track,fp_head,fp_sector,fdcstatus;

	fp_track = 0;
	fp_sector = 0;
	fp_head = 0;
	
	lba2chs(gb_fsmng,sector, &fp_track,&fp_head,&fp_sector);
	gb_fsmng->hxcfe->hxc_printf(MSG_DEBUG,"FAT12FS : media_write, sector: 0x%.8X, sector count : %d, Track: %d, Side: %d, Sector: %d",sector,sector_count,fp_track,fp_head,fp_sector);

	c = 0;
	for(i=0;i<sector_count;i++)
	{
		lba2chs(gb_fsmng,sector + i, &fp_track,&fp_head,&fp_sector);
		if(hxcfe_writeSectorFDC (gb_fsmng->fdc,(unsigned char)fp_track,(unsigned char)fp_head,(unsigned char)fp_sector,gb_fsmng->sectorsize,ISOIBM_MFM_ENCODING,1,&buffer[i*gb_fsmng->sectorsize],gb_fsmng->sectorsize,&fdcstatus) == 1)
		{
			if(!fdcstatus)
			{
				c++;
			}
			else
			{
				gb_fsmng->hxcfe->hxc_printf(MSG_DEBUG,"FAT12FS : media_write  !!! ERROR !!!, sector: 0x%.8X, sector count : %d, Track: %d, Side: %d, Sector: %d",sector,sector_count,fp_track,fp_head,fp_sector,fdcstatus);
			}
		}
	}

	if( c == sector_count )
		return 1;
	else
		return 0;
}

void init_fat12(HXCFE_FSMNG * fsmng)
{
	gb_fsmng = fsmng;
}

int32_t fat12_mountImage(HXCFE_FSMNG * fsmng, HXCFE_FLOPPY *floppy)
{
	unsigned char sectorbuffer[1024];
	int32_t nbsector,fdcstatus,badsectorfound;

	badsectorfound = 0;

	media_read_callback = media_read;
	media_write_callback = media_write;

	fiol_init();

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


			gb_fsmng->hxcfe->hxc_printf(MSG_DEBUG,"FAT12FS : %d Sectors per track (%d Bytes per sector)",fsmng->sectorpertrack,fsmng->sectorsize);

			if(fsmng->sectorpertrack && !badsectorfound)
			{
				/* Attach media access functions to library*/
				if (fiol_attach_media(media_read_callback, media_write_callback) != FAT_INIT_OK)
				{
					gb_fsmng->hxcfe->hxc_printf(MSG_DEBUG,"FAT12FS : Media attach failed");
				}

				return HXCFE_NOERROR;
			}
		}
	}

	return HXCFE_INTERNALERROR;
}

int32_t fat12_umountImage(HXCFE_FSMNG * fsmng)
{
	if(fsmng->fdc)
	{
		fiol_shutdown();
		hxcfe_deinitFDC (fsmng->fdc);
		fsmng->fdc = 0;
	}

	return HXCFE_NOERROR;
}

int32_t fat12_getFreeSpace(HXCFE_FSMNG * fsmng)
{
	return fiol_getFreeSpace();
}

int32_t fat12_getTotalSpace(HXCFE_FSMNG * fsmng)
{
	return fiol_getTotalSpace();
}

int32_t fat12_openDir(HXCFE_FSMNG * fsmng, char * path)
{
	FL_DIR * dir;
	int i;

	dir = malloc(sizeof(FL_DIR));
	if(dir)
	{
		memset(dir,0,sizeof(FL_DIR));

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

		if(fiol_opendir(path, dir))
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

int32_t fat12_readDir(HXCFE_FSMNG * fsmng,int32_t dirhandle,HXCFE_FSENTRY * dirent)
{
	int32_t ret;
	fl_dirent entry;

	if(dirhandle<128)
	{
		if(fsmng->dirhandletable[dirhandle-1])
		{
			ret = fiol_readdir(fsmng->dirhandletable[dirhandle-1], &entry);
			if(!ret)
			{
				strcpy(dirent->entryname,entry.filename);
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

int32_t fat12_closeDir(HXCFE_FSMNG * fsmng, int32_t dirhandle)
{
	if(dirhandle<128)
	{
		if(fsmng->dirhandletable[dirhandle-1])
		{
			fiol_closedir(fsmng->dirhandletable[dirhandle-1]);
			free(fsmng->dirhandletable[dirhandle-1]);
			fsmng->dirhandletable[dirhandle-1] = 0;
			return HXCFE_NOERROR;
		}
	}
	return HXCFE_ACCESSERROR;
}

int32_t fat12_openFile(HXCFE_FSMNG * fsmng, char * filename)
{
	FL_FILE *file;
	int i;

	i = 0;
	while(fsmng->handletable[i] && i<128)
	{
		i++;
	}
	if(i == 128) return HXCFE_ACCESSERROR;

	file = fiol_fopen(filename, "r");
	if(file)
	{
		fsmng->handletable[i] = file;
		return i+1;
	}

	return HXCFE_ACCESSERROR;
}

int32_t fat12_createFile(HXCFE_FSMNG * fsmng, char * filename)
{
	FL_FILE *file;
	int i;

	i = 0;
	while(fsmng->handletable[i] && i<128)
	{
		i++;
	}
	if(i == 128) return HXCFE_ACCESSERROR;

	file = fiol_fopen(filename, "wb");
	if(file)
	{
		fsmng->handletable[i] = file;
		return i+1;
	}

	return HXCFE_ACCESSERROR;
}

int32_t fat12_writeFile(HXCFE_FSMNG * fsmng,int32_t filehandle,unsigned char * buffer,int32_t size)
{
	int32_t byteswrite;
	if(filehandle<128)
	{
		if(fsmng->handletable[filehandle-1])
		{
			byteswrite = fiol_fwrite(buffer, 1, size, fsmng->handletable[filehandle-1]);
			return byteswrite;
		}
	}
	return HXCFE_ACCESSERROR;
}

int32_t fat12_readFile( HXCFE_FSMNG * fsmng,int32_t filehandle,unsigned char * buffer,int32_t size)
{
	int32_t bytesread;
	if(filehandle && filehandle<128)
	{
		if(fsmng->handletable[filehandle-1])
		{
			bytesread = fiol_fread(buffer, 1, size, fsmng->handletable[filehandle-1]);
			return bytesread;
		}
	}
	return HXCFE_ACCESSERROR;
}

int32_t fat12_deleteFile(HXCFE_FSMNG * fsmng, char * filename)
{
	if(fiol_remove(filename)>=0)
	{
		return HXCFE_NOERROR;
	}

	return HXCFE_ACCESSERROR;
}

int32_t fat12_closeFile( HXCFE_FSMNG * fsmng,int32_t filehandle)
{
	if(filehandle && filehandle<128)
	{
		if(fsmng->handletable[filehandle-1])
		{
			fiol_fclose(fsmng->handletable[filehandle-1]);
			fsmng->handletable[filehandle-1] = 0;
			return HXCFE_NOERROR;
		}
	}
	return HXCFE_ACCESSERROR;
}

int32_t fat12_createDir( HXCFE_FSMNG * fsmng,char * foldername)
{
	if(fiol_createdirectory(foldername))
	{
		return HXCFE_NOERROR;
	}
	return HXCFE_ACCESSERROR;
}

int32_t fat12_removeDir( HXCFE_FSMNG * fsmng,char * foldername)
{
	if(fiol_removedirectory(foldername)>=0)
	{
		return HXCFE_NOERROR;
	}

	return HXCFE_ACCESSERROR;
}

int32_t fat12_ftell( HXCFE_FSMNG * fsmng,int32_t filehandle)
{
	if(filehandle && filehandle<128)
	{
		if(fsmng->handletable[filehandle-1])
		{
			return fiol_ftell(fsmng->handletable[filehandle-1]);
		}
	}

	return HXCFE_ACCESSERROR;
}

int32_t fat12_fseek( HXCFE_FSMNG * fsmng,int32_t filehandle,int32_t offset,int32_t origin)
{
	if(filehandle && filehandle<128)
	{
		if(fsmng->handletable[filehandle-1])
		{
			return fiol_fseek(fsmng->handletable[filehandle-1],offset,origin);
		}
	}

	return HXCFE_ACCESSERROR;
}
