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
//	File	:	fs.c
//	Contains:	UTF-8	/	Ascii	path	file	system	functions
//
//	Written	by:	DEL	NERO	Jean	Francois
//
//	Change	History	(most	recent	first):
///////////////////////////////////////////////////////////////////////////////////

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#include <stdint.h>

#ifdef	WIN32
#	include <windows.h>
#	include <io.h>
#	include <fcntl.h>
#	include <direct.h>
#else
#	include <unistd.h>
#	include <dirent.h>

#ifdef	OSX
#	include <mach-o/dyld.h>
#endif

#endif


#include "libhxcfe.h"
#include "libhxcadaptor.h"


#ifdef WIN32


int	convertpath	(const char * path, wchar_t * wpath)
{
	if (!MultiByteToWideChar (CP_UTF8, 0, path, -1, wpath, MAX_PATH))
	{
		errno = ENOENT;
		return -1;
	}
	wpath[MAX_PATH] = L'\0';
	return 0;
}
#else

#endif


int hxc_open (const char *filename, int flags, ...)
{

#ifdef WIN32
	wchar_t wpath[MAX_PATH+1];
#else
	const char *local_name;
#endif

	unsigned int mode = 0;
	va_list ap;

	va_start (ap, flags);
	if (flags & O_CREAT)
		mode = va_arg (ap, unsigned int);
	va_end (ap);

#ifdef WIN32

	memset(wpath,0,(MAX_PATH+1)*sizeof(wchar_t));
	if(convertpath(filename, wpath)<0)
		return -1;

	return _wopen (wpath, flags, mode);

#else

	local_name = filename;
	//local_name = (const char *)ToLocale (filename);

	if (local_name == NULL)
	{
		errno = ENOENT;
		return -1;
	}

	int fd = open (local_name, flags, mode);

	//LocaleFree (local_name);
	return fd;

#endif


}

FILE *hxc_fopen (const char *filename, const char *mode)
{
	int rwflags, oflags;
	int append;
	int fd;
	const char *ptr;
	FILE *stream;

	// Try the classic way (ascii path)
	stream = fopen(filename,mode);

	if( ( stream == NULL ) && (errno == ENOENT) )
	{
		// Try the utf8 way
		rwflags = 0;
		oflags = 0;
		append = 0;

		for(ptr = mode;*ptr;ptr++)
		{
			switch (*ptr)
			{
				case 'r':
					rwflags = O_RDONLY;
					break;
		#ifdef	O_BINARY
				case 'b':
					oflags |= O_BINARY;
					break;
		#endif
				case 'a':
					rwflags = O_WRONLY;
					oflags |= O_CREAT;
					append = 1;
					break;

				case 'w':
					rwflags = O_WRONLY;
					oflags |= O_CREAT | O_TRUNC;
					break;

				case '+':
					rwflags = O_RDWR;
					break;

	#ifdef	O_TEXT
				case 't':
					oflags |= O_TEXT;
					break;
	#endif
			}
		}

		fd = hxc_open (filename, rwflags|oflags, 0666);
		if(fd==-1)
			return NULL;

		if(append)
		{
#if defined (WIN32)
			if(_lseek(fd,0,SEEK_END) == -1)
			{
				_close (fd);

				return NULL;
			}
#else
			if(lseek(fd,0,SEEK_END) == -1)
			{
				close (fd);

				return NULL;
			}
#endif
		}
		else
		{
#if defined (WIN32)
			_lseek(fd,0,SEEK_SET);
#else
			lseek(fd,0,SEEK_SET);
#endif
		}

#if defined (WIN32)
		stream = _fdopen(fd,mode);
		if(stream == NULL)
		{
			_close (fd);
		}
#else
		stream = fdopen(fd,mode);
		if(stream == NULL)
		{
			close (fd);
		}
#endif
	}

	return stream;
}

int hxc_fread(void * ptr, size_t size, FILE *f)
{
	if( fread(ptr,size,1,f) != 1 )
	{
		return 1; // Error
	}
	else
	{
		return 0; // No Error
	}
}

int	hxc_fclose(FILE * f)
{
	return fclose(f);
}

int hxc_statex( const char *filename, struct stat *buf)
{

#if defined (WIN32)
	wchar_t wpath[MAX_PATH+1];

	if(	convertpath(filename, wpath) < 0 )
		return -1;

	return _wstat (wpath,(struct _stat *)buf);
#else
	int res;
	const char *local_name;

	local_name = filename;//ToLocale( filename );

	if( local_name != NULL )
	{
		res = lstat( local_name, buf );
		// LocaleFree( local_name );
		return res;
	}

	errno = ENOENT;

	return -1;
#endif

}

int hxc_stat( const char *filename, struct stat *buf)
{
 return hxc_statex(filename,buf);
}

int hxc_find_first_file(char *folder, char *file, filefoundinfo* fileinfo)
{
#if defined (WIN32)

	HANDLE hfindfile;
	char *folderstr;
	WIN32_FIND_DATAW FindFileData;
	wchar_t	wpath[MAX_PATH+1];

	if(file)
	{
		folderstr=(char *) malloc(strlen(folder)+strlen(file)+2);
		sprintf((char *)folderstr,"%s\\%s",folder,file);
	}
	else
	{
		folderstr = (char *) malloc(strlen(folder)+1);
		sprintf((char *)folderstr,"%s",folder);
	}

	convertpath (folderstr, wpath);
	hfindfile = FindFirstFileW(wpath, &FindFileData);
	if(hfindfile!=INVALID_HANDLE_VALUE)
	{
		WideCharToMultiByte(CP_UTF8,0,FindFileData.cFileName,-1,fileinfo->filename,sizeof(fileinfo->filename),NULL,NULL);
		//sprintf(fileinfo->filename,"%s",FindFileData.cFileName);

		fileinfo->isdirectory = 0;

		if(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			fileinfo->isdirectory = 1;
		}

		fileinfo->size = FindFileData.nFileSizeLow;
		free(folderstr);
		return (int)hfindfile;
	}
	else
	{
		free(folderstr);
		return -1;
	}

#else
	struct dirent *d;
	DIR * dir;
	struct stat fileStat;
	char * tmpstr;

	dir = opendir (folder);
	if(dir)
	{
		d = readdir (dir);
		if(d)
		{
			tmpstr = malloc (strlen(folder) + strlen(d->d_name) + 4 );
			if(tmpstr)
			{
				strcpy(tmpstr,folder);
				strcat(tmpstr,"/");
				strcat(tmpstr,d->d_name);

				memset(&fileStat,0,sizeof(struct stat));
				if(!lstat (tmpstr, &fileStat))
				{

					if ( S_ISDIR ( fileStat.st_mode ) )
						fileinfo->isdirectory=1;
					else
						fileinfo->isdirectory=0;

					fileinfo->size=fileStat.st_size;

					strncpy(fileinfo->filename,d->d_name,256);

					free(tmpstr);
					return (int)dir;
				}

				free(tmpstr);
			}

			closedir (dir);
			dir=(DIR *)-1;

		}

		closedir (dir);
		dir=(DIR *)-1;
	}
	else
	{
		dir=(DIR *)-1;
	}

	return (int)dir;

#endif

	return -1;
}

int hxc_find_next_file(int handleff, char *folder, char *file, filefoundinfo* fileinfo)
{
	int ret;
#if defined (WIN32)
	WIN32_FIND_DATAW FindFileData;


	ret=FindNextFileW((HANDLE)handleff,&FindFileData);
	if(ret)
	{
		WideCharToMultiByte(CP_UTF8,0,FindFileData.cFileName,-1,fileinfo->filename,sizeof(fileinfo->filename),NULL,NULL);
		//sprintf(fileinfo->filename,"%s",FindFileData.cFileName);

		fileinfo->isdirectory=0;

		if( FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
		{
			fileinfo->isdirectory = 1;
		}

		fileinfo->size = FindFileData.nFileSizeLow;
	}
#else
	struct dirent *d;
	DIR * dir;
	struct stat fileStat;
	char * tmpstr;

	dir = (DIR*) handleff;
	d = readdir (dir);

	ret = 0;
	if(d)
	{
		tmpstr = malloc (strlen(folder) + strlen(d->d_name) + 4 );
		if(tmpstr)
		{
			strcpy(tmpstr,folder);
			strcat(tmpstr,"/");
			strcat(tmpstr,d->d_name);

			if(!lstat (tmpstr, &fileStat))
			{
				if ( S_ISDIR ( fileStat.st_mode ) )
					fileinfo->isdirectory=1;
				else
					fileinfo->isdirectory=0;

				fileinfo->size=fileStat.st_size;
				strncpy(fileinfo->filename,d->d_name,256);

				ret = 1;
				free(tmpstr);
				return ret;
			}

			free(tmpstr);
		}
    }

#endif

	return ret;
}

int hxc_find_close(int handle)
{
#if defined (WIN32)
	FindClose((void*)handle);
#else
	DIR * dir;
	dir = (DIR*) handle;
	if(dir)
		closedir(dir);
#endif
	return 0;
}

char * hxc_getcurrentdirectory(char *currentdirectory,int buffersize)
{
	memset(currentdirectory,0,buffersize);
#if defined (WIN32)
	if(GetModuleFileName(GetModuleHandle(NULL),currentdirectory,buffersize))
	{
		if(strrchr(currentdirectory,'\\'))
		{
			*((char*)strrchr(currentdirectory,'\\')) = 0;
			return currentdirectory;
		}
	}
#else

	#if defined (OSX)
	if (_NSGetExecutablePath(currentdirectory, &buffersize) == 0)
	{
		if(strrchr(currentdirectory,'/'))
		{
			*((char*)strrchr(currentdirectory,'/')) = 0;
			return currentdirectory;
		}
	}
	#endif

#endif

	return 0;
}

int hxc_mkdir(char * folder)
{

#ifdef WIN32
	_mkdir(folder);
#else
	mkdir(folder,0777);
#endif
	return 0;
}
