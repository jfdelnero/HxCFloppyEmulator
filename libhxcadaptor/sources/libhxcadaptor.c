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
#else
	#include <sys/time.h>
	#include <pthread.h>
	#include <sched.h>
#endif

#include <errno.h>

#include "libhxcfe.h"
#include "usb_hxcfloppyemulator.h"
#include "libhxcadaptor.h"

#ifdef WIN32
	HANDLE eventtab[256];
#else
	typedef struct _EVENT_HANDLE{
		pthread_cond_t eCondVar;
		pthread_mutex_t eMutex;
		int iVar;
	} EVENT_HANDLE;

	EVENT_HANDLE * eventtab[256];
#endif

#ifdef WIN32

DWORD WINAPI ThreadProc( LPVOID lpParameter)
{
	threadinit *threadinitptr;
	THREADFUNCTION thread;
	HXCFLOPPYEMULATOR* floppycontext;
	USBHXCFE * hw_context;

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

	threadinitptr=(threadinit*)lpParameter;
	thread=threadinitptr->thread;
	floppycontext=threadinitptr->hxcfloppyemulatorcontext;
	hw_context=threadinitptr->hwcontext;
	thread(floppycontext,hw_context);

	return 0;
}
#else
void * ThreadProc( void *lpParameter)
{
	threadinit *threadinitptr;
	THREADFUNCTION thread;
	HXCFLOPPYEMULATOR* floppycontext;
	USBHXCFE * hw_context;

	threadinitptr=(threadinit*)lpParameter;
	thread=threadinitptr->thread;
	floppycontext=threadinitptr->hxcfloppyemulatorcontext;
	hw_context=threadinitptr->hwcontext;
	thread(floppycontext,hw_context);

	return 0;
}
#endif

int hxc_setevent(HXCFLOPPYEMULATOR* floppycontext,unsigned char id)
{
#ifdef WIN32
	SetEvent(eventtab[id]);
#else

#endif
	return 0;
}

unsigned long hxc_createevent(HXCFLOPPYEMULATOR* floppycontext,unsigned char id)
{
#ifdef WIN32

	eventtab[id]=CreateEvent(NULL,FALSE,FALSE,NULL);
	return (unsigned long)eventtab[id];

#else

	eventtab[id]=(EVENT_HANDLE*)malloc(sizeof(EVENT_HANDLE));
	pthread_mutex_init(&eventtab[id]->eMutex, NULL);
	pthread_cond_init(&eventtab[id]->eCondVar, NULL);
	return (unsigned long)eventtab[id];

#endif
}

int hxc_waitevent(HXCFLOPPYEMULATOR* floppycontext,int id,int timeout)
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
	int ret;

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

void hxc_pause(int ms)
{
#ifdef WIN32

#else
	sleep(1);
#endif
}

int hxc_createthread(HXCFLOPPYEMULATOR* floppycontext,void* hwcontext,THREADFUNCTION thread,int priority)
{
#ifdef WIN32
	DWORD sit;
	threadinit *threadinitptr;

	threadinitptr=(threadinit*)malloc(sizeof(threadinit));
	threadinitptr->thread=thread;
	threadinitptr->hxcfloppyemulatorcontext=floppycontext;
	threadinitptr->hwcontext=hwcontext;

	CreateThread(NULL,8*1024,&ThreadProc,threadinitptr,0,&sit);

	return sit;
#else

	unsigned long sit;
	pthread_t threadid;
	pthread_attr_t threadattrib;
	threadinit *threadinitptr;
	struct sched_param param;

	pthread_attr_init(&threadattrib);

	pthread_attr_setschedpolicy(&threadattrib,SCHED_FIFO);
	param.sched_priority = sched_get_priority_max(SCHED_FIFO);
	/* set the new scheduling param */
	pthread_attr_setschedparam (&threadattrib, &param);

	threadinitptr=(threadinit *)malloc(sizeof(threadinit));
	threadinitptr->thread=thread;
	threadinitptr->hxcfloppyemulatorcontext=floppycontext;
	threadinitptr->hwcontext=hwcontext;

	pthread_create(&threadid, &threadattrib,ThreadProc, threadinitptr);

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
