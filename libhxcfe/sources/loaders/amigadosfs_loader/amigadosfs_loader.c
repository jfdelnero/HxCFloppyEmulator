/*
//
// Copyright (C) 2006-2024 Jean-François DEL NERO
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
// File : amigadosfs_loader.c
// Contains: AMIGADOSFSDK floppy image loader
//
// Written by: Jean-François DEL NERO
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>

#include "types.h"

#include "internal_libhxcfe.h"
#include "libhxcfe.h"
#include "libhxcadaptor.h"
#include "floppy_loader.h"

#include "loaders/common/raw_amiga.h"

#include "amigadosfs_loader.h"

#include "thirdpartylibs/adflib/Lib/adflib.h"
#include "stdboot3.h"

HXCFE* global_floppycontext;

#define DEFAULT_DISK_NAME "AmigaDOS (HxC)"


int AMIGADOSFSDK_libIsValidDiskFile( HXCFE_IMGLDR * imgldr_ctx, HXCFE_IMGLDR_FILEINFOS * imgfile )
{

	int pathlen;
	char * filepath;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"AMIGADOSFSDK_libIsValidDiskFile %s",imgfile->path);

	pathlen=strlen(imgfile->path);
	if(pathlen!=0)
	{
		if( imgfile->is_dir )
		{
			filepath=malloc(pathlen+1);
			if(filepath!=0)
			{
				sprintf(filepath,"%s",imgfile->path);
				hxc_strlower(filepath);

				if(strstr( filepath,".amigados" )!=NULL)
				{
					imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"AMIGADOSFSDK_libIsValidDiskFile : AMIGADOSFSDK file !");
					free(filepath);
					return HXCFE_VALIDFILE;
				}
				else
				{
					imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"AMIGADOSFSDK_libIsValidDiskFile : non AMIGADOSFSDK file ! (.amigados missing)");
					free(filepath);
					return HXCFE_BADFILE;
				}
			}
		}
		else
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"AMIGADOSFSDK_libIsValidDiskFile : non AMIGADOSFSDK file ! (it's not a directory)");
			return HXCFE_BADFILE;
		}
	}
	return HXCFE_BADPARAMETER;
}

static void adlib_printerror(char * msg)
{
	global_floppycontext->hxc_printf(MSG_ERROR,"AdfLib Error: %s",msg);
}

static void adlib_printwarning(char * msg)
{
	global_floppycontext->hxc_printf(MSG_WARNING,"AdfLib Warning: %s",msg);
}

static void adlib_printdebug(char * msg)
{
	global_floppycontext->hxc_printf(MSG_DEBUG,"AdfLib Debug: %s",msg);
}

int ScanFile(HXCFE* floppycontext,struct Volume * adfvolume,char * folder,char * file)
{
	void* hfindfile;
	filefoundinfo FindFileData;
	int bbool;
	int byte_written;
	FILE * ftemp;
	unsigned char  tempbuffer[512];
	struct File* adffile;
	char * fullpath;
	int size,filesize;
	RETCODE  rc;

	hfindfile = hxc_find_first_file(folder,file, &FindFileData);
	if(hfindfile)
	{
		bbool=TRUE;
		while( hfindfile && bbool )
		{
			if(FindFileData.isdirectory)
			{
				if(strcmp(".",FindFileData.filename)!=0 && strcmp("..",FindFileData.filename)!=0)
				{
					if(adfCountFreeBlocks(adfvolume)>4)
					{
						floppycontext->hxc_printf(MSG_INFO_1,"Adding directory %s",FindFileData.filename);
						rc=adfCreateDir(adfvolume,adfvolume->curDirPtr,FindFileData.filename);
						if(rc==RC_OK)
						{
							floppycontext->hxc_printf(MSG_INFO_1,"entering directory %s",FindFileData.filename);
							rc=adfChangeDir(adfvolume, FindFileData.filename);
							if(rc==RC_OK)
							{

								fullpath = malloc(strlen(FindFileData.filename)+strlen(folder)+2);
								if( !fullpath )
									return 0;

								sprintf(fullpath,"%s"DIR_SEPARATOR"%s",folder,FindFileData.filename);

								if(ScanFile(floppycontext,adfvolume,fullpath,file))
								{
									adfParentDir(adfvolume);
									free(fullpath);
									return 1;
								}
								floppycontext->hxc_printf(MSG_INFO_1,"Leaving directory %s",FindFileData.filename);
								free(fullpath);
								adfParentDir( adfvolume);
							}
							else
							{
								floppycontext->hxc_printf(MSG_ERROR,"Cannot enter to the directory %s !",FindFileData.filename);
								return 1;
							}
						}
						else
						{
							floppycontext->hxc_printf(MSG_ERROR,"Cannot Add the directory %s !",FindFileData.filename);
							return 1;
						}
					}
					else
					{
						floppycontext->hxc_printf(MSG_ERROR,"Cannot Add a directory ! : no more free block!!!");
						return 1;
					}

				}
			}
			else
			{
				if(adfCountFreeBlocks(adfvolume)>4)
				{
					floppycontext->hxc_printf(MSG_INFO_1,"Adding file %s, %dB",FindFileData.filename,FindFileData.size);
					adffile = adfOpenFile(adfvolume, FindFileData.filename, "w");
					if(adffile)
					{
						if(FindFileData.size)
						{
							fullpath = malloc(strlen(FindFileData.filename)+strlen(folder)+2);
							if( !fullpath )
								return 0;

							sprintf(fullpath,"%s"DIR_SEPARATOR"%s",folder,FindFileData.filename);

							ftemp=hxc_fopen(fullpath,"rb");
							if(ftemp)
							{
								filesize = hxc_fgetsize(ftemp);

								do
								{
									if(filesize>=512)
									{
										size=512;
									}
									else
									{
										size=filesize;
									}
									hxc_fread(&tempbuffer,size,ftemp);

									byte_written=adfWriteFile(adffile, size, tempbuffer);
									if((byte_written!=size) || (adfCountFreeBlocks(adfvolume)<2) )
									{
										floppycontext->hxc_printf(MSG_ERROR,"Error while writting the file %s. No more free block ?",FindFileData.filename);
										adfCloseFile(adffile);
										hxc_fclose(ftemp);
										free(fullpath);
										return 1;
									}
									filesize=filesize-512;

								}while( (filesize>0) && (byte_written==size));


								/*fileimg=(unsigned char*)malloc(filesize);
								memset(fileimg,0,filesize);
								hxc_fread(fileimg,filesize,ftemp);
								adfWriteFile(adffile, filesize, fileimg);
								free(fileimg);*/

								adfCloseFile(adffile);
								hxc_fclose(ftemp);
								free(fullpath);
							}
							else
							{
									floppycontext->hxc_printf(MSG_ERROR,"Error : Cannot open %s !!!",fullpath);
									free(fullpath);
									return 1;
							}
						}
					}
					else
					{
						floppycontext->hxc_printf(MSG_ERROR,"Error : Cannot create %s, %dB!!!",FindFileData.filename,FindFileData.size);
						 return 1;
					}
				}
				else
				{
					floppycontext->hxc_printf(MSG_ERROR,"Error : Cannot add a file : no more free block");
					return 1;
				}
			}
			bbool=hxc_find_next_file(hfindfile,folder,file,&FindFileData);
		}

	}
	else printf("Error FindFirstFile\n");

	hxc_find_close(hfindfile);
	return 0;
}

int AMIGADOSFSDK_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	struct Device * adfdevice;
	struct Volume * adfvolume;
	unsigned char * flatimg;
	unsigned char * flatimg2;
	char * repname;
	int flatimgsize;
	int numberoftrack;
	int numberofsectorpertrack;
	int ret;
	struct stat repstate;
	struct tm * ts;
	struct DateTime reptime;
	char * disk_format_name;

//  FILE * debugadf;
	int rc;

	numberoftrack = 80;
	numberofsectorpertrack = 11;
	if(parameters)
	{
		disk_format_name = (char*)parameters;
		if(!strcmp(disk_format_name,"amigados_hd"))
		{
			numberofsectorpertrack = 22;
		}
	}

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"AMIGADOSFSDK_libLoad_DiskFile %s",imgfile);

	hxc_stat(imgfile,&repstate);

	ts = localtime(&repstate.st_ctime);

	if(repstate.st_mode&S_IFDIR || !strlen(imgfile) )
	{
		global_floppycontext=imgldr_ctx->hxcfe;
		adfEnvInitDefault();
		adfChgEnvProp(PR_EFCT,adlib_printerror);
		adfChgEnvProp(PR_WFCT,adlib_printwarning);
		adfChgEnvProp(PR_VFCT,adlib_printdebug);

		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"ADFLib %s %s",adfGetVersionNumber(), adfGetVersionDate());

		adfdevice = adfCreateMemoryDumpDevice(numberoftrack, 2, numberofsectorpertrack,&flatimg,&flatimgsize);
		if(adfdevice)
		{

			if( strlen(imgfile) < strlen(DEFAULT_DISK_NAME) )
			{
				repname = (char *)malloc( strlen(DEFAULT_DISK_NAME) + 1 );
			}
			else
			{
				repname = (char *)malloc( strlen(imgfile) + 1 );
			}

			if( !repname )
			{
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Alloc error!");
				adfUnMountDev(adfdevice);
				return HXCFE_INTERNALERROR;
			}

			memset(repname,0,strlen(imgfile)+1);
			hxc_getfilenamebase( imgfile, repname, SYS_PATH_TYPE );

			if(!strlen(repname))
				strcpy(repname, DEFAULT_DISK_NAME);

			memset(&reptime,0,sizeof(struct DateTime));
			if(ts)
			{
				reptime.year = ts->tm_year; /* since 1900 */
				reptime.mon  = ts->tm_mon+1;
				reptime.day  = ts->tm_mday;
				reptime.hour = ts->tm_hour;
				reptime.min  = ts->tm_min;
				reptime.sec  = ts->tm_sec;
			}

			rc = adfCreateFlop(adfdevice, repname, 0,&reptime );

			free(repname);

			if (rc==RC_OK)
			{
				adfvolume = adfMount(adfdevice, 0, 0);
				if(adfvolume)
				{
					imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"adfCreateFlop ok");

					if(adfInstallBootBlock(adfvolume, stdboot3)!=RC_OK)
					{
						imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"adflib: adfInstallBootBlock error!");
					}

					if(strlen(imgfile))
					{
						if(ScanFile(imgldr_ctx->hxcfe,adfvolume,imgfile,"*.*"))
						{
							imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"ScanFile error!");
							return HXCFE_INTERNALERROR;
						}
					}

					flatimg2 = (unsigned char*)malloc(flatimgsize);
					if( flatimg2 )
					{
						memcpy(flatimg2,flatimg,flatimgsize);
						/*////////
						debugadf = hxc_fopen("d:\\debug.adf","wb");
						if(debugadf)
						{
							fwrite(flatimg2,flatimgsize,1,debugadf);
							hxc_fclose(debugadf);
						}
						//////////*/
					}
					adfUnMountDev(adfdevice);

				}
				else
				{
					imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"adflib: adfMount error!");
					return HXCFE_INTERNALERROR;
				}
			}
			else
			{
				imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"adflib: Error while creating the virtual floppy!");
				return HXCFE_INTERNALERROR;
			}
		}
		else
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"adflib: adfCreateMemoryDumpDevice error!");
			return HXCFE_INTERNALERROR;
		}

		ret = raw_amiga_loader(imgldr_ctx, floppydisk, 0, flatimg2, flatimgsize );

		return ret;
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"not a directory !");
		return HXCFE_BADFILE;
	}
}

int AMIGADOSFSDK_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]="AMIGA_FS";
	static const char plug_desc[]="AMIGA FS Loader";
	static const char plug_ext[]="amigados";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   AMIGADOSFSDK_libIsValidDiskFile,
		(LOADDISKFILE)      AMIGADOSFSDK_libLoad_DiskFile,
		(WRITEDISKFILE)     0,
		(GETPLUGININFOS)    AMIGADOSFSDK_libGetPluginInfo
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
