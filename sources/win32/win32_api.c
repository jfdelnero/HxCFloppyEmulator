/*
//
// Copyright (C) 2006 - 2012 Jean-François DEL NERO
//
// This file is part of HxCFloppyEmulator.
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

#include <windows.h>
#include <stdio.h>

#include "libhxcfe.h"
#include "../usb_hxcfloppyemulator.h"

#include "win32_api.h"

HANDLE eventtab[256];


DWORD WINAPI ThreadProc( LPVOID lpParameter)
{
	threadinit *threadinitptr;
	THREADFUNCTION thread;
	HXCFLOPPYEMULATOR* floppycontext;
	USBHXCFE* hw_context;

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

	threadinitptr=(threadinit*)lpParameter;
	thread=threadinitptr->thread;
	floppycontext=threadinitptr->hxcfloppyemulatorcontext;
	hw_context=threadinitptr->hwcontext;
	thread(floppycontext,hw_context);

	return 0;
}

unsigned long  hxc_createevent(HXCFLOPPYEMULATOR* floppycontext,unsigned char id)
{
	eventtab[id]=CreateEvent(NULL,FALSE,FALSE,NULL);
	return (unsigned long)eventtab[id];
}

int hxc_setevent(HXCFLOPPYEMULATOR* floppycontext,unsigned char id)
{
	SetEvent(eventtab[id]);
	return 0;
}

int hxc_waitevent(HXCFLOPPYEMULATOR* floppycontext,int id,int timeout)
{
	int ret;

	if(timeout==0) timeout=INFINITE;
	ret=WaitForSingleObject(eventtab[id],timeout);

	if(ret==0)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

void hxc_pause(int ms)
{
	Sleep(ms);
}

int hxc_createthread(HXCFLOPPYEMULATOR* floppycontext,void* hwcontext,THREADFUNCTION thread,int priority)
{
	DWORD sit;
	threadinit *threadinitptr;

	threadinitptr=(threadinit*)malloc(sizeof(threadinit));
	threadinitptr->thread=thread;
	threadinitptr->hxcfloppyemulatorcontext=floppycontext;
	threadinitptr->hwcontext=hwcontext;

	CreateThread(NULL,8*1024,&ThreadProc,threadinitptr,0,&sit);

	return sit;
}

int getlistoffile(char * directorypath,char *** filelist)
{
	int numberoffile;
	char ** filepathtab;
	
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
	*filelist=filepathtab;

	return numberoffile;
}


char * getcurrentdirectory(char *currentdirectory,int buffersize)
{
	memset(currentdirectory,0,buffersize);
	if(GetModuleFileName(GetModuleHandle(NULL),currentdirectory,buffersize))
	{
		if(strrchr(currentdirectory,'\\'))
		{
			*((char*)strrchr(currentdirectory,'\\'))=0;
			return currentdirectory;
		}
	}

	return 0;
}

