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

#include "libhxcfe.h"
#include "linux_api.h"
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <pthread.h>
#include <sched.h>
#include <errno.h>

//EVENT_HANDLE * eventtab[256];

void strlwr(char *string)
{
	int i;

	i=0;
	while (string[i])
	{
		string[i] = tolower(string[i]);
		i++;
	}
}


int getlistoffile(unsigned char * directorypath,unsigned char *** filelist)
{
	int numberoffile;
	char ** filepathtab;
/*
	HANDLE findfilehandle;
	WIN32_FIND_DATA FindFileData;

	filepathtab=0;
	numberoffile=0;

	findfilehandle=FindFirstFile(directorypath,&FindFileData);
	if(findfilehandle!=INVALID_HANDLE_VALUE)
	{

		do
		{
			filepathtab=(char **) realloc(filepathtab,sizeof(char*)*(numberoffile+1));
			filepathtab[numberoffile]=(char*)malloc(strlen(FindFileData.cFileName)+1);
			strcpy(filepathtab[numberoffile],FindFileData.cFileName);
			numberoffile++;
		}while(FindNextFile(findfilehandle,&FindFileData));

		FindClose(findfilehandle);
	}
	*filelist=filepathtab;*/

	return 0;//numberoffile;
}


char * getcurrentdirectory(char *currentdirectory,int buffersize)
{
	memset(currentdirectory,0,buffersize);
/*	if(GetModuleFileName(GetModuleHandle(NULL),currentdirectory,buffersize))
	{
		if(strrchr(currentdirectory,'\\'))
		{
			*((char*)strrchr(currentdirectory,'\\'))=0;
			return currentdirectory;
		}
	}*/



	return 0;
}


int loaddiskplugins(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * newfloppy,char *pluginpath)
{
/*	HMODULE module;

//	module=LoadLibrary(pluginpath);
	if(module)
	{
			newfloppy->IsValidDiskFile=(ISVALIDDISKFILE )GetProcAddress(module,"IsValidDiskFile");
			newfloppy->UnLoadDiskFile=(UNLOADDISKFILE )GetProcAddress(module,"UnLoadDiskFile");
			newfloppy->GetFloppyPropretiesInt=(GETFLOPPYPROPRETIESINT)GetProcAddress(module,"GetFloppyPropretiesInt");
			newfloppy->GetTrackData=(GETTRACKDATA)GetProcAddress(module,"GetTrackData");
			newfloppy->GetIndexData=(GETINDEXDATA)GetProcAddress(module,"GetIndexData");
			newfloppy->LoadDiskFile=(LOADDISKFILE)GetProcAddress(module,"LoadDiskFile");
			if(newfloppy->IsValidDiskFile && newfloppy->UnLoadDiskFile && newfloppy->GetFloppyPropretiesInt && newfloppy->GetTrackData && newfloppy->GetIndexData && newfloppy->LoadDiskFile)
			{
				return 1;
			}
			else
			{
				FreeLibrary(module);
			}
	}*/

	return 0;
}

long find_first_file(char *folder,char *file,filefoundinfo* fileinfo)
{
	return -1;
}

long find_next_file(long handleff,char *folder,char *file,filefoundinfo* fileinfo)
{
	return 0;
}

long find_close(long handle)
{

	return 0;
}

char * strupper(char * str)
{
	int i;

	i=0;
	while(str[i])
	{

		if(str[i]>='a' && str[i]<='z')
		{
			str[i]=str[i]+('A'-'a');
		}
		i++;
	}

	return str;
}


char * strlower(char * str)
{
	int i;

	i=0;
	while(str[i])
	{

		if(str[i]>='A' && str[i]<='Z')
		{
			str[i]=str[i]+('a'-'A');
		}
		i++;
	}

	return str;
}
