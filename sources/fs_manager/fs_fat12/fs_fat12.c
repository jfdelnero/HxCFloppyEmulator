/*
//
// Copyright (C) 2006 - 2013 Jean-François DEL NERO
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
// Written by:	DEL NERO Jean Francois
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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
HXCFLOPPYEMULATOR* floppycontext;
FSMNG * gb_fsmng;

static void lba2chs(FSMNG * fsmng,int lba, int *track, int *head, int *sector)
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

int media_read(unsigned long sector, unsigned char *buffer, unsigned long sector_count)
{
	unsigned long i,c;
	int fp_track,fp_head,fp_sector,fdcstatus;

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

int media_write(unsigned long sector, unsigned char *buffer,unsigned long sector_count)
{
	unsigned long i,c;
	int fp_track,fp_head,fp_sector,fdcstatus;

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

void init_fat12(FSMNG * fsmng)
{
	gb_fsmng = fsmng;
}

int fat12_mountImage(FSMNG * fsmng, FLOPPY *floppy)
{
	unsigned char sectorbuffer[1024];
	int nbsector,fdcstatus,badsectorfound;

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

int fat12_umountImage(FSMNG * fsmng)
{
	if(fsmng->fdc)
	{
		fiol_shutdown();
		hxcfe_deinitFDC (fsmng->fdc);
		fsmng->fdc = 0;
	}

	return HXCFE_NOERROR;
}

int fat12_getFreeSpace(FSMNG * fsmng)
{
	return fiol_getFreeSpace();
}

int fat12_getTotalSpace(FSMNG * fsmng)
{
	return fiol_getTotalSpace();
}

int fat12_openDir(FSMNG * fsmng, char * path)
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

int fat12_readDir(FSMNG * fsmng,int dirhandle,FSENTRY * dirent)
{
	int ret;
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

int fat12_closeDir(FSMNG * fsmng, int dirhandle)
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

int fat12_openFile(FSMNG * fsmng, char * filename)
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

int fat12_createFile(FSMNG * fsmng, char * filename)
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

int fat12_writeFile(FSMNG * fsmng,int filehandle,unsigned char * buffer,int size)
{
	int byteswrite;
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

int fat12_readFile( FSMNG * fsmng,int filehandle,unsigned char * buffer,int size)
{
	int bytesread;
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

int fat12_deleteFile(FSMNG * fsmng, char * filename)
{
	if(fiol_remove(filename)>=0)
	{
		return HXCFE_NOERROR;
	}

	return HXCFE_ACCESSERROR;
}

int fat12_closeFile( FSMNG * fsmng,int filehandle)
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

int fat12_createDir( FSMNG * fsmng,char * foldername)
{
	if(fiol_createdirectory(foldername))
	{
		return HXCFE_NOERROR;
	}
	return HXCFE_ACCESSERROR;
}

int fat12_removeDir( FSMNG * fsmng,char * foldername)
{
	return fat12_deleteFile(fsmng, foldername);
}

int fat12_ftell( FSMNG * fsmng,int filehandle)
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

int fat12_fseek( FSMNG * fsmng,int filehandle,long offset,int origin)
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