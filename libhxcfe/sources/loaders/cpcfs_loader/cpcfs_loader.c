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
// File : cpcfs_loader.c
// Contains: CPCFSDK floppy image loader
//
// Written by: Jean-François DEL NERO
// Assisted-by: Claude Opus 5.5
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////
//
// This loader creates an Amstrad CPC AMSDOS virtual disk from a host folder :
//
//  - *.cpcfs folders    : DATA format disk   (sector IDs 0xC1-0xC9, 178KB).
//  - *.cpcsysfs folders : SYSTEM format disk (sector IDs 0x41-0x49, 2 reserved
//                         tracks, 169KB). The reserved tracks are left blank.
//
//  - Files at the root of the folder are added to the user area 0.
//  - Files in "USERx" (x : 0-15) sub folders are added to the user area x.
//
// The AMSDOS file system management itself is done by the fs_manager
// fs_cpc module : this loader only formats a blank disk and copies the
// host files into it through the file system manager.
//
///////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

#include "types.h"

#include "internal_libhxcfe.h"
#include "tracks/track_generator.h"
#include "sector_search.h"
#include "fdc_ctrl.h"
#include "fs_manager/fs_manager.h"
#include "libhxcfe.h"
#include "fs_manager/fs_manager_access.h"
#include "libhxcadaptor.h"
#include "floppy_loader.h"
#include "floppy_utils.h"

#include "loaders/common/raw_iso.h"

#include "plugins_id.h"

#include "cpcfs_loader.h"

// AMSDOS DATA format : 40 tracks, 1 side, 9 x 512 bytes sectors (IDs 0xC1-0xC9)
#define CPCFS_TRACKS            40
#define CPCFS_SIDES             1
#define CPCFS_SECTORS           9
#define CPCFS_SECTOR_SIZE       512
#define CPCFS_DATA_SECTOR_ID    0xC1
#define CPCFS_SYSTEM_SECTOR_ID  0x41
#define CPCFS_FORMAT_FILLER     0xE5
#define CPCFS_MAX_FILE_SIZE     ( CPCFS_TRACKS * CPCFS_SECTORS * CPCFS_SECTOR_SIZE )
#define CPCFS_MAX_USER          15

int CPCFSDK_libIsValidDiskFile( HXCFE_IMGLDR * imgldr_ctx, HXCFE_IMGLDR_FILEINFOS * imgfile )
{
	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"CPCFSDK_libIsValidDiskFile");

	if(imgfile)
	{
		if( imgfile->is_dir )
		{
			if( hxc_checkfileext(imgfile->path,"cpcfs",SYS_PATH_TYPE) ||
				hxc_checkfileext(imgfile->path,"cpcsysfs",SYS_PATH_TYPE) )
			{
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"CPCFSDK_libIsValidDiskFile : CPCFS folder !");
				return HXCFE_VALIDFILE;
			}

			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"CPCFSDK_libIsValidDiskFile : non CPCFS folder ! (.cpcfs/.cpcsysfs missing)");
			return HXCFE_BADFILE;
		}

		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"CPCFSDK_libIsValidDiskFile : non CPCFS folder ! (it's not a directory)");
		return HXCFE_BADFILE;
	}

	return HXCFE_BADPARAMETER;
}

// "USERx" host folder name -> user area number. -1 if not a user area folder.
static int get_user_area(const char * name)
{
	int i,user;

	if( strlen(name) < 5 || strlen(name) > 6 )
		return -1;

	if( ( name[0] != 'U' && name[0] != 'u' ) ||
		( name[1] != 'S' && name[1] != 's' ) ||
		( name[2] != 'E' && name[2] != 'e' ) ||
		( name[3] != 'R' && name[3] != 'r' ) )
		return -1;

	user = 0;
	for(i=4;name[i];i++)
	{
		if( name[i] < '0' || name[i] > '9' )
			return -1;

		user = (user * 10) + (name[i] - '0');
	}

	if( user > CPCFS_MAX_USER )
		return -1;

	return user;
}

// Copy one host file into the CPC disk.
// Returns 0 if done or skipped, -1 if the disk is full / on access error.
static int add_file(HXCFE * hxcfe, HXCFE_FSMNG * fsmng, char * hostpath, char * name, int user)
{
	FILE * f;
	int32_t size,wsize,filehandle;
	unsigned char * buffer;
	char cpcpath[64];

	if( name[0] == '.' )
	{
		hxcfe->hxc_printf(MSG_DEBUG,"CPCFSDK : Hidden file %s skipped",hostpath);
		return 0;
	}

	if( strlen(name) > 12 )
	{
		hxcfe->hxc_printf(MSG_WARNING,"CPCFSDK : %s skipped (the name doesn't fit the 8.3 AMSDOS format)",hostpath);
		return 0;
	}

	if(user)
		snprintf(cpcpath,sizeof(cpcpath),"/USER%d/%s",user,name);
	else
		snprintf(cpcpath,sizeof(cpcpath),"/%s",name);

	// AMSDOS names are upper case : two host files may end up with the same name.
	filehandle = hxcfe_openFile(fsmng, cpcpath);
	if( filehandle > 0 )
	{
		hxcfe_closeFile(fsmng, filehandle);
		hxcfe->hxc_printf(MSG_WARNING,"CPCFSDK : %s skipped (%s already present on the disk)",hostpath,cpcpath);
		return 0;
	}

	f = hxc_fopen(hostpath,"rb");
	if(!f)
	{
		hxcfe->hxc_printf(MSG_ERROR,"CPCFSDK : Can't open %s !",hostpath);
		return -1;
	}

	size = hxc_fgetsize(f);
	if( size < 0 || size > CPCFS_MAX_FILE_SIZE )
	{
		hxcfe->hxc_printf(MSG_ERROR,"CPCFSDK : %s is too big (%d bytes) !",hostpath,size);
		hxc_fclose(f);
		return -1;
	}

	buffer = NULL;
	if(size)
	{
		buffer = malloc(size);
		if(!buffer)
		{
			hxc_fclose(f);
			return -1;
		}

		if( fread(buffer,size,1,f) != 1 )
		{
			hxcfe->hxc_printf(MSG_ERROR,"CPCFSDK : %s read error !",hostpath);
			free(buffer);
			hxc_fclose(f);
			return -1;
		}
	}

	hxc_fclose(f);

	filehandle = hxcfe_createFile(fsmng, cpcpath);
	if( filehandle <= 0 )
	{
		// Invalid AMSDOS name or full directory.
		hxcfe->hxc_printf(MSG_WARNING,"CPCFSDK : %s skipped (invalid AMSDOS file name or directory full)",hostpath);
		free(buffer);
		return 0;
	}

	wsize = 0;
	if(size)
		wsize = hxcfe_writeFile(fsmng, filehandle, buffer, size);

	free(buffer);

	if( hxcfe_closeFile(fsmng, filehandle) != HXCFE_NOERROR || wsize != size )
	{
		hxcfe->hxc_printf(MSG_ERROR,"CPCFSDK : Can't add %s (%d bytes) : No more space on the disk !",hostpath,size);
		hxcfe_deleteFile(fsmng, cpcpath);
		return -1;
	}

	hxcfe->hxc_printf(MSG_INFO_1,"CPCFSDK : %s added (%d bytes)",cpcpath,size);

	return 0;
}

// Add the content of a host folder to a user area (and the USERx sub folders if root is set).
static int add_folder(HXCFE * hxcfe, HXCFE_FSMNG * fsmng, char * folder, int user, int root)
{
	void * ffhandle;
	filefoundinfo ffi;
	char * fullpath;
	int ret,subuser,found;

	ret = 0;

	ffhandle = hxc_find_first_file(folder, "*.*", &ffi);
	if(!ffhandle)
		return 0;

	do
	{
		if( strcmp(ffi.filename,".") && strcmp(ffi.filename,"..") )
		{
			fullpath = malloc(strlen(folder) + strlen(ffi.filename) + 2);
			if(!fullpath)
			{
				ret = -1;
				break;
			}

			sprintf(fullpath,"%s/%s",folder,ffi.filename);

			if(ffi.isdirectory)
			{
				subuser = get_user_area(ffi.filename);
				if( root && subuser >= 0 )
				{
					ret = add_folder(hxcfe, fsmng, fullpath, subuser, 0);
				}
				else
				{
					hxcfe->hxc_printf(MSG_WARNING,"CPCFSDK : Folder %s skipped (only USER0-USER15 folders are supported)",fullpath);
				}
			}
			else
			{
				ret = add_file(hxcfe, fsmng, fullpath, ffi.filename, user);
			}

			free(fullpath);

			if(ret < 0)
				break;
		}

		found = hxc_find_next_file(ffhandle, folder, "*.*", &ffi);
	}while(found);

	hxc_find_close(ffhandle);

	return ret;
}

// SYSTEM or DATA format ? Selected by the file system name (hxcfe_generateFloppy)
// or by the host folder extension.
static int is_system_format(char * imgfile, void * parameters)
{
	if(parameters)
		return !strcmp((char*)parameters,"amsdossys");

	if( imgfile && strlen(imgfile) )
		return hxc_checkfileext(imgfile,"cpcsysfs",SYS_PATH_TYPE);

	return 0;
}

int CPCFSDK_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	HXCFE * hxcfe;
	HXCFE_FSMNG * fsmng;
	fs_plugins_ptr * fsfuncs;
	raw_iso_cfg rawcfg;
	unsigned char * flatimg;
	int32_t dksize;
	struct stat staterep;
	int ret,system_format;

	hxcfe = imgldr_ctx->hxcfe;

	hxcfe->hxc_printf(MSG_DEBUG,"CPCFSDK_libLoad_DiskFile %s",imgfile);

	system_format = is_system_format(imgfile, parameters);

	hxcfe->hxc_printf(MSG_INFO_1,"CPCFSDK : Creating an AMSDOS %s format disk",system_format?"SYSTEM":"DATA");

	// Blank formatted AMSDOS disk : all sectors filled with 0xE5
	// (empty directory, all blocks free).
	raw_iso_setdefcfg(&rawcfg);

	rawcfg.number_of_tracks = CPCFS_TRACKS;
	rawcfg.number_of_sides = CPCFS_SIDES;
	rawcfg.number_of_sectors_per_track = CPCFS_SECTORS;
	rawcfg.sector_size = CPCFS_SECTOR_SIZE;
	if(system_format)
		rawcfg.start_sector_id = CPCFS_SYSTEM_SECTOR_ID;
	else
		rawcfg.start_sector_id = CPCFS_DATA_SECTOR_ID;
	rawcfg.interleave = 2;
	rawcfg.gap3 = 0x4E;
	rawcfg.bitrate = 250000;
	rawcfg.rpm = 300;
	rawcfg.track_format = IBMFORMAT_DD;
	rawcfg.interface_mode = CPC_DD_FLOPPYMODE;
	rawcfg.fill_value = CPCFS_FORMAT_FILLER;

	dksize = CPCFS_TRACKS * CPCFS_SIDES * CPCFS_SECTORS * CPCFS_SECTOR_SIZE;

	flatimg = (unsigned char*)malloc(dksize);
	if(!flatimg)
		return HXCFE_INTERNALERROR;

	memset(flatimg,CPCFS_FORMAT_FILLER,dksize);

	ret = raw_iso_loader(imgldr_ctx, floppydisk, 0, flatimg, dksize, &rawcfg);

	free(flatimg);

	if(ret != HXCFE_NOERROR)
		return ret;

	if( !imgfile || !strlen(imgfile) )
	{
		// No source path : blank disk.
		return HXCFE_NOERROR;
	}

	// Mount the new disk with the AMSDOS file system manager and add the files.
	fsmng = hxcfe_initFsManager(hxcfe);
	if(!fsmng)
	{
		hxcfe_freeFloppy(hxcfe, floppydisk);
		return HXCFE_INTERNALERROR;
	}

	ret = HXCFE_INTERNALERROR;

	if( hxcfe_selectFS(fsmng, FS_CPC_AMSDOS) == HXCFE_NOERROR )
	{
		fsfuncs = (fs_plugins_ptr *)fsmng->fn;

		if( fsfuncs->fsmng_mountImage(fsmng, floppydisk) == HXCFE_NOERROR )
		{
			memset(&staterep,0,sizeof(struct stat));
			hxc_stat(imgfile,&staterep);

			if( staterep.st_mode & S_IFDIR )
			{
				if( add_folder(hxcfe, fsmng, imgfile, 0, 1) < 0 )
					ret = HXCFE_BADFILE;
				else
					ret = HXCFE_NOERROR;
			}
			else
			{
				if( add_file(hxcfe, fsmng, imgfile, hxc_getfilenamebase(imgfile,NULL,SYS_PATH_TYPE), 0) < 0 )
					ret = HXCFE_BADFILE;
				else
					ret = HXCFE_NOERROR;
			}

			hxcfe->hxc_printf(MSG_INFO_1,"CPCFSDK : %d bytes free on the disk",hxcfe_getFreeFsSpace(fsmng));

			hxcfe_umountImage(fsmng);
		}
		else
		{
			hxcfe->hxc_printf(MSG_ERROR,"CPCFSDK : Can't mount the new AMSDOS disk !");
		}
	}

	hxcfe_deinitFsManager(fsmng);

	if(ret != HXCFE_NOERROR)
		hxcfe_freeFloppy(hxcfe, floppydisk);

	return ret;
}

int CPCFSDK_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]=PLUGIN_CPC_FS;
	static const char plug_desc[]="Amstrad CPC AMSDOS FS Loader";
	static const char plug_ext[]="cpcfs";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   CPCFSDK_libIsValidDiskFile,
		(LOADDISKFILE)      CPCFSDK_libLoad_DiskFile,
		(WRITEDISKFILE)     0,
		(GETPLUGININFOS)    CPCFSDK_libGetPluginInfo
	};

	return libGetPluginInfo(
			imgldr_ctx,
			infotype,
			returnvalue,
			plug_id,
			plug_desc,
			&plug_funcs,
			plug_ext
			);
}
