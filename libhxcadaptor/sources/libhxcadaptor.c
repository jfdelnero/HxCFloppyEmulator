/*
//
// Copyright (C) 2006-2023 Jean-François DEL NERO
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
// File : libhxcadaptor.c
// Contains: "Glue"/Os depend functions
//
// Written by:	DEL NERO Jean Francois
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#ifdef WIN32
	#include <windows.h>
	#include <direct.h>
#else
	#include <sys/time.h>
	#include <pthread.h>
	#include <sched.h>
#endif

#include <errno.h>

#include <stdint.h>

#include "internal_libhxcfe.h"
#include "libhxcfe.h"
#include "usb_hxcfloppyemulator.h"
#include "libhxcadaptor.h"

#ifdef WIN32
	HANDLE eventtab[256];
	CRITICAL_SECTION criticalsectiontab[256];
#else
	typedef struct _EVENT_HANDLE{
		pthread_cond_t eCondVar;
		pthread_mutex_t eMutex;
		int iVar;
	} EVENT_HANDLE;

	EVENT_HANDLE * eventtab[256];

	pthread_mutex_t criticalsectiontab[256];
#endif

#ifdef WIN32

DWORD WINAPI ThreadProc( LPVOID lpParameter)
{
	threadinit *threadinitptr;
	THREADFUNCTION thread;
	HXCFE* floppycontext;
	USBHXCFE * hw_context;

	if( lpParameter )
	{
		//SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

		threadinitptr=(threadinit*)lpParameter;
		thread=threadinitptr->thread;
		floppycontext=threadinitptr->hxcfloppyemulatorcontext;
		hw_context=threadinitptr->hwcontext;
		thread(floppycontext,hw_context);

		free(threadinitptr);
	}

	return 0;
}
#else
void * ThreadProc( void *lpParameter)
{
	threadinit *threadinitptr;
	THREADFUNCTION thread;
	HXCFE* floppycontext;
	USBHXCFE * hw_context;

	threadinitptr=(threadinit*)lpParameter;
	if( threadinitptr )
	{

		thread=threadinitptr->thread;
		floppycontext=threadinitptr->hxcfloppyemulatorcontext;
		hw_context=threadinitptr->hwcontext;
		thread(floppycontext,hw_context);

		free(threadinitptr);
	}

	return 0;
}
#endif

int hxc_setevent(HXCFE* floppycontext,unsigned char id)
{
#ifdef WIN32
	SetEvent(eventtab[id]);
#else
	pthread_mutex_lock(&eventtab[id]->eMutex);
	pthread_cond_signal(&eventtab[id]->eCondVar);
	pthread_mutex_unlock(&eventtab[id]->eMutex);
#endif
	return 0;
}

uintptr_t hxc_createevent(HXCFE* floppycontext,unsigned char id)
{
#ifdef WIN32

	eventtab[id] = CreateEvent(NULL,FALSE,FALSE,NULL);
	return (uintptr_t)eventtab[id];

#else

	eventtab[id]=(EVENT_HANDLE*)malloc(sizeof(EVENT_HANDLE));
	if(eventtab[id])
	{
		pthread_mutex_init(&eventtab[id]->eMutex, NULL);
		pthread_cond_init(&eventtab[id]->eCondVar, NULL);
	}
	return (uintptr_t)eventtab[id];
#endif
}

int hxc_waitevent(HXCFE* floppycontext,int id,int timeout)
{

#ifdef WIN32
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
#else
	struct timeval now;
	struct timespec timeoutstr;
	int retcode;

	pthread_mutex_lock(&eventtab[id]->eMutex);
	gettimeofday(&now,0);
	timeoutstr.tv_sec = now.tv_sec + (timeout/1000);
	timeoutstr.tv_nsec = (now.tv_usec * 1000) ;//+(timeout*1000000);
	retcode = 0;

	retcode = pthread_cond_timedwait(&eventtab[id]->eCondVar, &eventtab[id]->eMutex, &timeoutstr);
	if (retcode == ETIMEDOUT)
	{
		pthread_mutex_unlock(&eventtab[id]->eMutex);
		return 1;
	}
	else
	{
		pthread_mutex_unlock(&eventtab[id]->eMutex);
		return 0;
	}
#endif
}

void* hxc_createcriticalsection(HXCFE* floppycontext,unsigned char id)
{
#ifdef WIN32

	InitializeCriticalSection(&criticalsectiontab[id]);
	return (void*)&criticalsectiontab[id];

#else
	//create mutex attribute variable
	pthread_mutexattr_t mAttr;

	pthread_mutexattr_init(&mAttr);

#if defined(PTHREAD_MUTEX_RECURSIVE)
	// setup recursive mutex for mutex attribute
	pthread_mutexattr_settype(&mAttr, PTHREAD_MUTEX_RECURSIVE);
#elif defined (PTHREAD_MUTEX_RECURSIVE_NP)
	pthread_mutexattr_settype(&mAttr, PTHREAD_MUTEX_RECURSIVE_NP);
#endif
	// Use the mutex attribute to create the mutex
	pthread_mutex_init(&criticalsectiontab[id], &mAttr);

	// Mutex attribute can be destroy after initializing the mutex variable
	pthread_mutexattr_destroy(&mAttr);

	return (void*)&criticalsectiontab[id];
#endif
}


void hxc_entercriticalsection(HXCFE* floppycontext,unsigned char id)
{
#ifdef WIN32
	EnterCriticalSection( &criticalsectiontab[id] );
#else
	pthread_mutex_lock( &criticalsectiontab[id] );
#endif
}

void hxc_leavecriticalsection(HXCFE* floppycontext,unsigned char id)
{
#ifdef WIN32
	LeaveCriticalSection( &criticalsectiontab[id] );
#else
	pthread_mutex_unlock( &criticalsectiontab[id] );
#endif
}

void hxc_destroycriticalsection(HXCFE* floppycontext,unsigned char id)
{
#ifdef WIN32
	DeleteCriticalSection(&criticalsectiontab[id]);
#else
	pthread_mutex_destroy (&criticalsectiontab[id]);
#endif
}


#ifndef WIN32
void hxc_msleep (unsigned int ms) {
    int microsecs;
    struct timeval tv;
    microsecs = ms * 1000;
    tv.tv_sec  = microsecs / 1000000;
    tv.tv_usec = microsecs % 1000000;
    select (0, NULL, NULL, NULL, &tv);
}
#endif

void hxc_pause(int ms)
{
#ifdef WIN32
	Sleep(ms);
#else
	hxc_msleep(ms);
#endif
}

int hxc_createthread(HXCFE* floppycontext,void* hwcontext,THREADFUNCTION thread,int priority)
{
#ifdef WIN32
	DWORD sit;
	HANDLE thread_handle;

	threadinit *threadinitptr;

	sit = 0;

	threadinitptr = (threadinit*)malloc(sizeof(threadinit));
	if(threadinitptr)
	{
		threadinitptr->thread=thread;
		threadinitptr->hxcfloppyemulatorcontext=floppycontext;
		threadinitptr->hwcontext=hwcontext;

		thread_handle = CreateThread(NULL,8*1024,&ThreadProc,threadinitptr,0,&sit);

		if(!thread_handle)
		{
			floppycontext->hxc_printf(MSG_ERROR,"hxc_createthread : CreateThread failed -> 0x.8X", GetLastError());
		}
	}

	return sit;
#else

	unsigned long sit;
	int ret;
	pthread_t threadid;
	pthread_attr_t threadattrib;
	threadinit *threadinitptr;
	struct sched_param param;

	sit = 0;

	pthread_attr_init(&threadattrib);

	pthread_attr_setinheritsched(&threadattrib, PTHREAD_EXPLICIT_SCHED);

	if(priority)
	{
		pthread_attr_setschedpolicy(&threadattrib,SCHED_FIFO);
		param.sched_priority = sched_get_priority_max(SCHED_FIFO);
	}
	else
	{
		pthread_attr_setschedpolicy(&threadattrib,SCHED_OTHER);
		param.sched_priority = sched_get_priority_max(SCHED_OTHER);
	}
	/* set the new scheduling param */
	pthread_attr_setschedparam (&threadattrib, &param);

	threadinitptr = (threadinit*)malloc(sizeof(threadinit));
	if(threadinitptr)
	{
		threadinitptr->thread=thread;
		threadinitptr->hxcfloppyemulatorcontext=floppycontext;
		threadinitptr->hwcontext=hwcontext;

		ret = pthread_create(&threadid, &threadattrib,ThreadProc, threadinitptr);
		if(ret)
		{
			floppycontext->hxc_printf(MSG_ERROR,"hxc_createthread : pthread_create failed -> %d",ret);
			free(threadinitptr);
		}
	}
	return sit;
#endif

}

#ifndef WIN32
/*void strlwr(char *string)
{
	int i;

	i=0;
	while (string[i])
	{
		string[i] = tolower(string[i]);
		i++;
	}
}*/
#endif

char * hxc_strupper(char * str)
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

char * hxc_strlower(char * str)
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

char * hxc_getfilenamebase(char * fullpath,char * filenamebase, int type)
{
	int len,i;
	char separator;

	if(fullpath)
	{
		len=strlen(fullpath);

		separator = DIR_SEPARATOR_CHAR; // system type by default

		switch(type)
		{
			case SYS_PATH_TYPE:  // System based
				separator = DIR_SEPARATOR_CHAR;
			break;

			case UNIX_PATH_TYPE:    // Unix style
				separator = '/';
			break;

			case WINDOWS_PATH_TYPE: // Windows style
				separator = '\\';
			break;
		}

		i=0;
		if(len)
		{
			i=len-1;
			while(i &&	( fullpath[i] != separator && fullpath[i]!=':') )
			{
				i--;
			}

			if( fullpath[i] == separator || fullpath[i]==':' )
			{
				i++;
			}

			if(i>len)
			{
				i=len;
			}
		}

		if(filenamebase)
		{
			strcpy(filenamebase,&fullpath[i]);
		}

		return &fullpath[i];
	}

	return 0;
}

char * hxc_getfilenameext(char * fullpath,char * filenameext, int type )
{
	char * filename;
	int len,i;

	filename=hxc_getfilenamebase(fullpath,0,type);

	if(filename)
	{
		len=strlen(filename);

		i=0;
		if(len)
		{
			i=len-1;

			while(i &&	( filename[i] != '.' ) )
			{
				i--;
			}

			if( filename[i] == '.' )
			{
				i++;
			}
			else
			{
				i=len;
			}

			if(i>len)
			{
				i=len;
			}
		}

		if(filenameext)
		{
			strcpy(filenameext,&filename[i]);
		}

		return &filename[i];
	}

	return 0;
}

int hxc_getfilenamewext(char * fullpath,char * filenamewext, int type)
{
	char * filename;
	char * ext;
	int len;

	len = 0;
	if(fullpath)
	{
		filename = hxc_getfilenamebase(fullpath,0,type);
		ext = hxc_getfilenameext(fullpath,0,type);

		len = ext-filename;

		if(len && filename[len-1]=='.')
		{
			len--;
		}

		if(filenamewext)
		{
			memcpy(filenamewext,filename,len);
			filenamewext[len]=0;
		}
	}

	return len;
}

int hxc_getpathfolder(char * fullpath,char * folder,int type)
{
	int len;
	char * filenameptr;

	len = 0;
	if(fullpath)
	{
		filenameptr = hxc_getfilenamebase(fullpath,0,type);

		len = filenameptr-fullpath;

		if(folder)
		{
			memcpy(folder,fullpath,len);
			folder[len]=0;
		}
	}

	return len;
}

int hxc_checkfileext(char * path,char *ext,int type)
{
	char pathext[16];
	char srcext[16];

	if(path && ext)
	{
		if( ( strlen(hxc_getfilenameext(path,0,type)) < 16 )  && ( strlen(ext) < 16 ))
		{
			hxc_getfilenameext(path,(char*)&pathext,type);
			hxc_strlower(pathext);

			strcpy((char*)srcext,ext);
			hxc_strlower(srcext);

			if(!strcmp(pathext,srcext))
			{
				return 1;
			}
		}
	}
	return 0;
}

int hxc_getfilesize(char * path)
{
	int filesize;
	FILE * f;

	filesize=-1;

	if(path)
	{
		f=hxc_fopen(path,"rb");
		if(f)
		{
			fseek (f , 0 , SEEK_END);
			filesize=ftell(f);

			fclose(f);
		}
	}

	return filesize;
}

#ifdef WIN32

#if defined(_MSC_VER) && _MSC_VER < 1900

#define va_copy(dest, src) (dest = src)

int vsnprintf(char *s, size_t n, const char *fmt, va_list ap)
{
	int ret;
	va_list ap_copy;

	if (n == 0)
		return 0;
	else if (n > INT_MAX)
		return 0;
	memset(s, 0, n);
	va_copy(ap_copy, ap);
	ret = _vsnprintf(s, n - 1, fmt, ap_copy);
	va_end(ap_copy);

	return ret;
}

int snprintf(char *s, size_t n, const char *fmt, ...)
{
	va_list ap;
	int ret;

	va_start(ap, fmt);
	ret = vsnprintf(s, n, fmt, ap);
	va_end(ap);

	return ret;
}

#endif

#endif