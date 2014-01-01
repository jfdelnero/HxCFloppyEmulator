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
// File : fs_amigados.c
// Contains: AmigaDos File system manager functions
//
// Written by:	DEL NERO Jean Francois
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "libhxcfe.h"
#include "libhxcadaptor.h"
#include "floppy_loader.h"
#include "floppy_utils.h"

#include "thirdpartylibs/adflib/Lib/adflib.h"
#include "thirdpartylibs/adflib/Lib/adf_str.h"
#include "thirdpartylibs/adflib/Lib/adf_nativ.h"
#include "thirdpartylibs/adflib/Lib/adf_err.h"
#include "thirdpartylibs/adflib/Lib/adf_dir.h"

HXCFLOPPYEMULATOR* floppycontext;
FSMNG * gb_fsmng;

//struct Device * adfdevice;
//struct Volume * adfvolume;
extern struct Env adfEnv;


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
				*sector = (lba % fsmng->sectorpertrack);
			}
			else
			{
				*sector = 0;
			}
		}
	}
}

/*
 * myInitDevice
 *
 * must fill 'dev->size'
 */
RETCODE HxCADFLibInitDevice(struct Device* dev, char* name,BOOL ro)
{
    struct nativeDevice* nDev;

    nDev = (struct nativeDevice*)dev->nativeDev;

    nDev = (struct nativeDevice*)malloc(sizeof(struct nativeDevice));
    if (!nDev) {
        (*adfEnv.eFct)("myInitDevice : malloc");
        return RC_ERROR;
    }
    dev->nativeDev = nDev;
    if (!ro)
        /* check if device is writable, if not, force readOnly to TRUE */
        dev->readOnly = FALSE;
    else
        /* mount device as read only */
        dev->readOnly = TRUE;

    dev->size = gb_fsmng->trackperdisk *
				gb_fsmng->sidepertrack *
				gb_fsmng->sectorpertrack *
				gb_fsmng->sectorsize;


    return RC_OK;
}

RETCODE HxCADFLibReadSector(struct Device *dev, long sector, int size, unsigned char* buf)
{
	unsigned long i,c,sector_count;
	int fp_track,fp_head,fp_sector,fdcstatus;
	unsigned char tmpSectBuf[512];
	int remainSize;

	sector_count = size / gb_fsmng->sectorsize;
	if(size&0x1FF) sector_count++;

	remainSize = size;

	lba2chs(gb_fsmng,sector, &fp_track,&fp_head,&fp_sector);

	gb_fsmng->hxcfe->hxc_printf(MSG_DEBUG,"HxCADFLibReadSector : media_read, sector: 0x%.8X, sector count : %d, Track: %d, Side: %d, Sector: %d",sector,sector_count,fp_track,fp_head,fp_sector);
	c=0;
	for(i=0;i<sector_count;i++)
	{
		lba2chs(gb_fsmng,sector + i, &fp_track,&fp_head,&fp_sector);
		if(hxcfe_readSectorFDC (gb_fsmng->fdc,(unsigned char)fp_track,(unsigned char)fp_head,(unsigned char)fp_sector,gb_fsmng->sectorsize,AMIGA_MFM_ENCODING,1,(unsigned char*)&tmpSectBuf,gb_fsmng->sectorsize,&fdcstatus) == 1)
		{
			if(!fdcstatus)
			{
				if( remainSize >= gb_fsmng->sectorsize )
				{
					memcpy(&buf[i*gb_fsmng->sectorsize],tmpSectBuf,gb_fsmng->sectorsize);
					remainSize -= gb_fsmng->sectorsize;
				}
				else
				{
					memcpy(&buf[i*gb_fsmng->sectorsize],tmpSectBuf,remainSize);
					remainSize = 0;
				}
				c++;
			}
			else
			{
				gb_fsmng->hxcfe->hxc_printf(MSG_DEBUG,"HxCADFLibReadSector : media_read !!! ERROR !!!, sector: 0x%.8X, sector count : %d, Track: %d, Side: %d, Sector: %d, FDC Status : 0x%.2x",sector,sector_count,fp_track,fp_head,fp_sector,fdcstatus);
			}
		}
	}

	if( c == sector_count )
		return RC_OK;
	else
		return 0;
}

RETCODE HxCADFLibWriteSector(struct Device *dev, long sector, int size, unsigned char* buf)
{
	unsigned long i,c,sector_count;
	int fp_track,fp_head,fp_sector,fdcstatus;
	unsigned char tmpSectBuf[512];
	int remainSize;

	sector_count = size / gb_fsmng->sectorsize;
	if(size&0x1FF) sector_count++;

	remainSize = size;

	lba2chs(gb_fsmng,sector, &fp_track,&fp_head,&fp_sector);

	gb_fsmng->hxcfe->hxc_printf(MSG_DEBUG,"HxCADFLibWriteSector : media_write, sector: 0x%.8X, sector count : %d, Track: %d, Side: %d, Sector: %d",sector,sector_count,fp_track,fp_head,fp_sector);
	c = 0;
	for(i=0;i<sector_count;i++)
	{
		lba2chs(gb_fsmng,sector + i, &fp_track,&fp_head,&fp_sector);

		if(remainSize>=512)
		{
			memcpy(tmpSectBuf,&buf[i*gb_fsmng->sectorsize],512);
		}
		else
		{
			hxcfe_readSectorFDC (gb_fsmng->fdc,(unsigned char)fp_track,(unsigned char)fp_head,(unsigned char)fp_sector,gb_fsmng->sectorsize,AMIGA_MFM_ENCODING,1,(unsigned char*)&tmpSectBuf,gb_fsmng->sectorsize,&fdcstatus);
			memcpy(tmpSectBuf,&buf[i*gb_fsmng->sectorsize],remainSize);
		}

		if(hxcfe_writeSectorFDC (gb_fsmng->fdc,(unsigned char)fp_track,(unsigned char)fp_head,(unsigned char)fp_sector,gb_fsmng->sectorsize,AMIGA_MFM_ENCODING,1,(unsigned char*)&tmpSectBuf,gb_fsmng->sectorsize,&fdcstatus) == 1)
		{
			if(!fdcstatus)
			{
				c++;
			}
			else
			{
				gb_fsmng->hxcfe->hxc_printf(MSG_DEBUG,"HxCADFLibWriteSector : media_write  !!! ERROR !!!, sector: 0x%.8X, sector count : %d, Track: %d, Side: %d, Sector: %d",sector,sector_count,fp_track,fp_head,fp_sector,fdcstatus);
			}
		}
	}

	if( c == sector_count )
		return RC_OK;
	else
		return 0;
}

RETCODE HxCADFLibReleaseDevice(struct Device *dev)
{
    struct nativeDevice* nDev;

    nDev = (struct nativeDevice*)dev->nativeDev;

	free(nDev);

    return RC_OK;
}

BOOL HxCADFLibIsDevNative(char *devName)
{
    if(!strcmp("HXCDOSDISKBROWSER",devName))
	{
		return 1;
	}
	return 0;
}

void HxCadfInitNativeFct()
{
    struct nativeFunctions *nFct;

    nFct = (struct nativeFunctions*)adfEnv.nativeFct;

    nFct->adfInitDevice = HxCADFLibInitDevice ;
    nFct->adfNativeReadSector = HxCADFLibReadSector ;
    nFct->adfNativeWriteSector = HxCADFLibWriteSector ;
    nFct->adfReleaseDevice = HxCADFLibReleaseDevice ;
    nFct->adfIsDevNative = HxCADFLibIsDevNative;
}


static void adlib_printerror(char * msg)
{
	floppycontext->hxc_printf(MSG_ERROR,"AdfLib Error: %s",msg);
}

static void adlib_printwarning(char * msg)
{
	floppycontext->hxc_printf(MSG_WARNING,"AdfLib Warning: %s",msg);
}

static void adlib_printdebug(char * msg)
{
	floppycontext->hxc_printf(MSG_DEBUG,"AdfLib Debug: %s",msg);
}

void init_amigados(FSMNG * fsmng)
{
	gb_fsmng = fsmng;
	floppycontext = fsmng->hxcfe;
}

static int changedir(FSMNG * fsmng,char * path,SECTNUM * curdir,int dir)
{
	int i,ret;
	char tmppath[512];
	char * tmpptr;
	struct bEntryBlock entry;
	struct Volume * adfvolume;

	adfvolume = (struct Volume *)fsmng->volume;

	if(strlen(path))
	{

		i = 0;
		if(path[i]=='/')
		{
			ret = adfToRootDir(adfvolume);
			i++;
		}

		while(path[i])
		{
			tmpptr = strchr(&path[i],'/');
			memset(tmppath,0,sizeof(tmppath));
			if(tmpptr)
			{
				strncpy(tmppath,&path[i],tmpptr-&path[i]);
				i = i + (tmpptr-&path[i]) + 1;
			}
			else
			{
				strcpy(tmppath,&path[i]);
				i = i + strlen(&path[i]);
			}

			if(strlen(tmppath))
				ret = adfChangeDir(adfvolume,tmppath);
			else
				break;
		}
	}

	if(!ret)
	{
		ret = adfReadEntryBlock(adfvolume,adfvolume->curDirPtr,&entry);
		if(!ret)
		{
			if(dir)
			{
				if(entry.secType == ST_DIR || entry.secType == ST_ROOT)
				{
					if(curdir)
						*curdir = adfvolume->curDirPtr;

					return 0;
				}
				else
					return 1;
			}
			else
			{
				if(entry.secType == ST_FILE || entry.secType == ST_LFILE)
				{
					if(curdir)
						*curdir = adfvolume->curDirPtr;

					return 0;
				}
				else
					return 1;
			}
		}

	}
	return ret;
}

int amigados_mountImage(FSMNG * fsmng, FLOPPY *floppy)
{
	unsigned char sectorbuffer[512];
	int nbsector,fdcstatus,badsectorfound;
	struct Volume * adfvolume;
	struct Device * adfdevice;

	badsectorfound = 0;

	if(!floppy || !fsmng)
		return HXCFE_BADPARAMETER;

	adfEnvInitDefault();
	HxCadfInitNativeFct();
	adfChgEnvProp(PR_EFCT,adlib_printerror);
	adfChgEnvProp(PR_WFCT,adlib_printwarning);
	adfChgEnvProp(PR_VFCT,adlib_printdebug);

	fsmng->fp = floppy;

	if(fsmng->fp->floppyNumberOfTrack>83)
		fsmng->trackperdisk = 83;
	else
		fsmng->trackperdisk = fsmng->fp->floppyNumberOfTrack;
	fsmng->sectorpertrack = 11;
	fsmng->sidepertrack = fsmng->fp->floppyNumberOfSide;
	fsmng->sectorsize = 512;

	memset(fsmng->dirhandletable,0xFF,sizeof(fsmng->dirhandletable));
	memset(fsmng->handletable,0xFF,sizeof(fsmng->handletable));

	if(fsmng->fdc)
		hxcfe_deinitFDC (fsmng->fdc);

	fsmng->fdc = hxcfe_initFDC (fsmng->hxcfe);
	if(fsmng->fdc)
	{
		if(hxcfe_insertDiskFDC (fsmng->fdc,floppy) == HXCFE_NOERROR)
		{
			// Count the number of sector
			nbsector = 0;
			while(hxcfe_readSectorFDC(fsmng->fdc,40,0,(unsigned char)(nbsector),512,AMIGA_MFM_ENCODING,1,(unsigned char*)sectorbuffer,sizeof(sectorbuffer),&fdcstatus))
			{
				if(fdcstatus == FDC_BAD_DATA_CRC)
					badsectorfound++;
				nbsector++;
			}

			fsmng->sidepertrack = 2;
			fsmng->sectorpertrack = nbsector;

			if(fsmng->sectorpertrack && !badsectorfound)
			{
				fsmng->hxcfe->hxc_printf(MSG_DEBUG,"AMIGADOSFS : %d Sectors per track (%d Bytes per sector)",fsmng->sectorpertrack,fsmng->sectorsize);

				fsmng->device = (void*)adfMountDev("HXCDOSDISKBROWSER",0);

				adfdevice = (struct Device *)fsmng->device;
				if(adfdevice)
				{
					fsmng->volume = (void*)adfMount(adfdevice, 0, 0);
					adfvolume = (struct Volume *)fsmng->volume;
					if(adfvolume)
					{
						floppycontext->hxc_printf(MSG_DEBUG,"adfMount ok");
						return HXCFE_NOERROR;
					}
				}
			}
		}
	}

	return HXCFE_INTERNALERROR;
}

int amigados_umountImage(FSMNG * fsmng)
{
	if(fsmng->fdc)
	{
		hxcfe_deinitFDC (fsmng->fdc);
		fsmng->fdc = 0;
	}

	return HXCFE_NOERROR;
}

int amigados_getFreeSpace(FSMNG * fsmng)
{
	struct Volume * adfvolume;

	adfvolume = (struct Volume *)fsmng->volume;

	return adfCountFreeBlocks(adfvolume) * 512;
}

int amigados_getTotalSpace(FSMNG * fsmng)
{
	struct Device * adfdevice;

	adfdevice = (struct Device *)fsmng->device;

	return adfdevice->size;
}

int amigados_openDir(FSMNG * fsmng, char * path)
{
	int i;
	SECTNUM snum;

	if( changedir(fsmng,path,&snum,1) ==  RC_OK)
	{
		i = 0;
		while((long)fsmng->dirhandletable[i]!=-1 && i<128)
		{
			i++;
		}

		if(i == 128)
		{
			return HXCFE_ACCESSERROR;
		}

		fsmng->dirhandletable[i] = (void*)snum;

		fsmng->dirindex[i] = 0;

		return i+1;
	}

	return HXCFE_ACCESSERROR;
}

int amigados_readDir(FSMNG * fsmng,int dirhandle,FSENTRY * dirent)
{
	struct List *list, *cell;
	struct Entry *entry;
	int i;
	struct Volume * adfvolume;

	adfvolume = (struct Volume *)fsmng->volume;

	if(dirhandle<128)
	{
		if((long)fsmng->dirhandletable[dirhandle-1]!=-1)
		{
			/* saves the head of the list */
			cell = list = adfGetDirEnt(adfvolume,(long)fsmng->dirhandletable[dirhandle-1]);

			i=0;
			/* while cell->next is NULL, the last cell */
			while(cell && ( i <= fsmng->dirindex[dirhandle-1] ) )
			{
				entry = (struct Entry*)cell->content;

				strcpy(dirent->entryname,entry->name);
				dirent->size = entry->size;

				dirent->isdir = 0;
				if(entry->type == ST_DIR)
					dirent->isdir = 1;

				dirent->flags = 0;

				cell = cell->next;
				i++;
			}


			/* frees the list and the content */

			fsmng->dirindex[dirhandle-1]++;

			adfFreeDirList(list);

			if(i==fsmng->dirindex[dirhandle-1])
			{
				return HXCFE_VALIDFILE;
			}

			return HXCFE_NOERROR;
		}
	}

	return HXCFE_ACCESSERROR;
}

int amigados_closeDir(FSMNG * fsmng, int dirhandle)
{
	if(dirhandle<128)
	{
		if(fsmng->dirhandletable[dirhandle-1]!=(void*)-1)
		{
			fsmng->dirhandletable[dirhandle-1] = (void*)-1;
			return HXCFE_NOERROR;
		}
	}
	return HXCFE_ACCESSERROR;
}

int amigados_openFile(FSMNG * fsmng, char * filename)
{
	struct File *file;
	int i;
	SECTNUM snum;
	char filen[256];
	struct Volume * adfvolume;

	adfvolume = (struct Volume *)fsmng->volume;

	i = 0;
	while(((long)fsmng->handletable[i]!=-1) && i<128)
	{
		i++;
	}
	if(i == 128) return HXCFE_ACCESSERROR;

	if( changedir(fsmng,filename,&snum,0) ==  RC_OK)
	{
		if( adfParentDir(adfvolume) == RC_OK )
		{
			hxc_getfilenamebase(filename,filen);

			file = adfOpenFile(adfvolume, filen, "r");
			if(file)
			{
				fsmng->handletable[i] = file;
				return i+1;
			}
		}
	}

	return HXCFE_ACCESSERROR;
}

int amigados_createFile(FSMNG * fsmng, char * filename)
{
	struct File *file;
	int i;
	char filen[256];
	char folderpath[256];
	SECTNUM snum;
	struct Volume * adfvolume;

	adfvolume = (struct Volume *)fsmng->volume;

	i = 0;
	while((long)fsmng->handletable[i]!=-1 && i<128)
	{
		i++;
	}
	if(i == 128) return HXCFE_ACCESSERROR;

	hxc_getpathfolder(filename,folderpath);

	if( changedir(fsmng,folderpath,&snum,1) ==  RC_OK)
	{
		hxc_getfilenamebase(filename,filen);

		file = adfOpenFile(adfvolume, filen, "w");
		if(file)
		{
			fsmng->handletable[i] = file;
			return i+1;
		}
	}

	return HXCFE_ACCESSERROR;
}

int amigados_writeFile(FSMNG * fsmng,int filehandle,unsigned char * buffer,int size)
{
	int byteswrite;

	byteswrite = 0;

	if(filehandle<128)
	{
		if((long)fsmng->handletable[filehandle-1]!=-1)
		{
			if(amigados_getFreeSpace(fsmng) >= size)
			{
				byteswrite = adfWriteFile((struct File*)fsmng->handletable[filehandle-1], size, buffer);
			}
			return byteswrite;
		}
	}
	return HXCFE_ACCESSERROR;
}

int amigados_readFile( FSMNG * fsmng,int filehandle,unsigned char * buffer,int size)
{
	int bytesread;
	if(filehandle && filehandle<128)
	{
		if((long)fsmng->handletable[filehandle-1]!=-1)
		{
			bytesread = adfReadFile((struct File*)fsmng->handletable[filehandle-1], size, buffer);
			return bytesread;
		}
	}
	return HXCFE_ACCESSERROR;
}

int amigados_deleteFile(FSMNG * fsmng, char * filename)
{
	char filen[256];
	char folderpath[256];
	SECTNUM snum;
	struct Volume * adfvolume;

	adfvolume = (struct Volume *)fsmng->volume;

	hxc_getpathfolder(filename,folderpath);

	if( changedir(fsmng,folderpath,&snum,1) ==  RC_OK)
	{
		hxc_getfilenamebase(filename,filen);

		if(adfRemoveEntry(adfvolume, adfvolume->curDirPtr, filen) == RC_OK)
		{
			return HXCFE_NOERROR;
		}
	}

	return HXCFE_ACCESSERROR;
}

int amigados_closeFile( FSMNG * fsmng,int filehandle)
{
	if(filehandle && filehandle<128)
	{
		if((long)fsmng->handletable[filehandle-1]!=-1)
		{
			adfCloseFile((struct File *)fsmng->handletable[filehandle-1]);
			fsmng->handletable[filehandle-1] = (void*)-1;
			return HXCFE_NOERROR;
		}
	}
	return HXCFE_ACCESSERROR;
}

int amigados_createDir( FSMNG * fsmng,char * foldername)
{
	char filen[256];
	char folderpath[256];
	SECTNUM snum;
	struct Volume * adfvolume;

	adfvolume = (struct Volume *)fsmng->volume;

	hxc_getpathfolder(foldername,folderpath);

	if( changedir(fsmng,folderpath,&snum,1) ==  RC_OK)
	{
		hxc_getfilenamebase(foldername,filen);

		if(adfCreateDir(adfvolume, adfvolume->curDirPtr, filen) == RC_OK)
		{
			return HXCFE_NOERROR;
		}
	}

	return HXCFE_ACCESSERROR;
}

int amigados_removeDir( FSMNG * fsmng,char * foldername)
{
	return amigados_deleteFile(fsmng, foldername);
}

int amigados_ftell( FSMNG * fsmng,int filehandle)
{
	struct File * file;
	if(filehandle && filehandle<128)
	{
		if((long)fsmng->handletable[filehandle-1]!=-1)
		{
			file = (struct File *)fsmng->handletable[filehandle-1];
			return file->pos;
		}
	}
	return HXCFE_ACCESSERROR;
}

int amigados_fseek( FSMNG * fsmng,int filehandle,long offset,int origin)
{
	struct File * file;
	if(filehandle && filehandle<128)
	{
		if((long)fsmng->handletable[filehandle-1]!=-1)
		{
			file = (struct File *)fsmng->handletable[filehandle-1];
			switch(origin)
			{
				case SEEK_SET:
					adfFileSeek((struct File *)fsmng->handletable[filehandle-1], offset);
					return HXCFE_NOERROR;
				break;
				case SEEK_CUR:
					adfFileSeek((struct File *)fsmng->handletable[filehandle-1], offset + file->pos);
					return HXCFE_NOERROR;
				break;
				case SEEK_END:
					adfFileSeek((struct File *)fsmng->handletable[filehandle-1], 0xFFFFFFFF);
					if((unsigned long)offset<file->pos)
						adfFileSeek((struct File *)fsmng->handletable[filehandle-1], file->pos - offset);
					else
						adfFileSeek((struct File *)fsmng->handletable[filehandle-1], 0);
					return HXCFE_NOERROR;
				break;
			}

			return HXCFE_ACCESSERROR;
		}
	}

	return HXCFE_ACCESSERROR;
}