/*
//
// Copyright (C) 2006-2025 Jean-Fran�ois DEL NERO
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

#include <stdint.h>

#include "internal_libhxcfe.h"
#include "libhxcfe.h"

#ifdef WIN32
#include <windows.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#if defined(WIN32)
	#include <conio.h>
	#include <ftd2xx.h>
	#include <winioctl.h>
#elif defined(__APPLE__)
	#include <dlfcn.h>
	#include "../macosx/ftd2xx.h"
#else // __linux__

	#define FTDILIB 1

	#include <dlfcn.h>
	#include <errno.h>

	#include "../linux/ftd2xx.h"

#endif

#include "ftdi_api.h"

#if !defined(FTDILIB)

typedef FT_STATUS (WINAPI * FT_OPEN)(int32_t deviceNumber,	FT_HANDLE *pHandle);
typedef FT_STATUS (WINAPI * FT_READ)( FT_HANDLE ftHandle,LPVOID lpBuffer,DWORD nBufferSize,LPDWORD lpBytesReturned);
typedef FT_STATUS (WINAPI * FT_WRITE)(FT_HANDLE ftHandle,LPVOID lpBuffer,DWORD nBufferSize,LPDWORD lpBytesWritten);
typedef FT_STATUS (WINAPI * FT_GETSTATUS)(FT_HANDLE ftHandle,DWORD *dwRxBytes,DWORD *dwTxBytes,DWORD *dwEventDWord);
typedef FT_STATUS (WINAPI * FT_PURGE)(FT_HANDLE ftHandle,ULONG Mask);
typedef FT_STATUS (WINAPI * FT_SETUSBPARAMETERS)(FT_HANDLE ftHandle,ULONG ulInTransferSize,ULONG ulOutTransferSize);
typedef FT_STATUS (WINAPI * FT_SETLATENCYTIMER)(FT_HANDLE ftHandle,UCHAR ucLatency);
typedef FT_STATUS (WINAPI * FT_SETEVENTNOTIFICATION)(FT_HANDLE ftHandle,DWORD dwEventMask, PVOID pvArg);
typedef FT_STATUS (WINAPI * FT_CLOSE)(FT_HANDLE ftHandle);

FT_OPEN pFT_Open;
FT_READ pFT_Read;
FT_WRITE pFT_Write;
FT_GETSTATUS pFT_GetStatus;
FT_PURGE pFT_Purge;
FT_SETUSBPARAMETERS pFT_SetUSBParameters;
FT_SETLATENCYTIMER pFT_SetLatencyTimer;
FT_SETEVENTNOTIFICATION pFT_SetEventNotification;
FT_CLOSE pFT_Close;

#else

typedef void * ftdi_context;

typedef ftdi_context ( * P_FTDI_NEW)();
typedef int32_t ( * P_FTDI_INIT)(ftdi_context ftdi);
typedef int32_t ( * P_FTDI_USB_OPEN)(ftdi_context ftdi, int32_t vendor, int32_t product);
typedef int32_t ( * P_FTDI_USB_CLOSE)(ftdi_context ftdi);
typedef int32_t ( * P_FTDI_USB_PURGE_RX_BUFFER)(ftdi_context ftdi);
typedef int32_t ( * P_FTDI_USB_PURGE_TX_BUFFER)(ftdi_context ftdi);
typedef int32_t ( * P_FTDI_USB_PURGE_BUFFERS)(ftdi_context ftdi);
typedef int32_t ( * P_FTDI_READ_DATA_SET_CHUNKSIZE)(ftdi_context ftdi, uint32_t chunksize);
typedef int32_t ( * P_FTDI_WRITE_DATA_SET_CHUNKSIZE)(ftdi_context ftdi, uint32_t chunksize);
typedef int32_t ( * P_FTDI_SET_LATENCY_TIMER)(ftdi_context ftdi, unsigned char latency);
typedef int32_t ( * P_FTDI_WRITE_DATA)(ftdi_context ftdi, unsigned char * buf, int32_t size);
typedef int32_t ( * P_FTDI_READ_DATA)(ftdi_context ftdi, unsigned char * buf, int32_t size);

P_FTDI_NEW                      p_ftdi_new;
P_FTDI_INIT                     p_ftdi_init;
P_FTDI_USB_OPEN                 p_ftdi_usb_open;
P_FTDI_USB_CLOSE                p_ftdi_usb_close;
P_FTDI_USB_PURGE_RX_BUFFER      p_ftdi_usb_purge_rx_buffer;
P_FTDI_USB_PURGE_TX_BUFFER      p_ftdi_usb_purge_tx_buffer;
P_FTDI_USB_PURGE_BUFFERS        p_ftdi_usb_purge_buffers;
P_FTDI_READ_DATA_SET_CHUNKSIZE  p_ftdi_read_data_set_chunksize;
P_FTDI_WRITE_DATA_SET_CHUNKSIZE p_ftdi_write_data_set_chunksize;
P_FTDI_SET_LATENCY_TIMER        p_ftdi_set_latency_timer;
P_FTDI_WRITE_DATA               p_ftdi_write_data;
P_FTDI_READ_DATA                p_ftdi_read_data;

#define BUFFERSIZE 32*1024

EVENT_HANDLE * STOP_THREAD_EVENT;
EVENT_HANDLE * READ_THREAD_EVENT;
int32_t stop_thread;

ftdi_context * ftdic;

typedef struct _DATA_FIFO{
	uint32_t ptr_in;
	uint32_t ptr_out;
	unsigned char BUFFER[BUFFERSIZE];
} DATA_FIFO;

DATA_FIFO tx_fifo;
DATA_FIFO rx_fifo;

unsigned char RXBUFFER[64];

EVENT_HANDLE * createevent()
{
	EVENT_HANDLE* theevent;
	theevent=(EVENT_HANDLE*)malloc(sizeof(EVENT_HANDLE));
	pthread_mutex_init(&theevent->eMutex, NULL);
	pthread_cond_init(&theevent->eCondVar, NULL);
	return theevent;
}

void setevent(EVENT_HANDLE * theevent)
{
	pthread_cond_signal(&theevent->eCondVar);
}

void destroyevent(EVENT_HANDLE * theevent)
{
	pthread_cond_destroy(&theevent->eCondVar);
	free(theevent);
}

int32_t waitevent(EVENT_HANDLE* theevent,int32_t timeout)
{
	struct timeval now;
	struct timespec timeoutstr;
	int32_t retcode;

	pthread_mutex_lock(&theevent->eMutex);
	gettimeofday(&now,0);
	timeoutstr.tv_sec = now.tv_sec + (timeout/1000);
	timeoutstr.tv_nsec = (now.tv_usec * 1000);
	retcode = 0;

	retcode = pthread_cond_timedwait(&theevent->eCondVar, &theevent->eMutex, &timeoutstr);	
	if (retcode == ETIMEDOUT) 
	{
		pthread_mutex_unlock(&theevent->eMutex);
		return 1;
	} 
	else 
	{
		pthread_mutex_unlock(&theevent->eMutex);
		return 0; 
	}
}

// --------------------------------------------------------

typedef int32_t (*RDTHREADFUNCTION) (ftdi_context ftdihandle);

typedef struct listenerthreadinit_
{
	RDTHREADFUNCTION thread;
	ftdi_context ftdihandle;
}listenerthreadinit;

void * FTDIListenerThreadProc( void *lpParameter)
{
	listenerthreadinit *threadinitptr;
	RDTHREADFUNCTION thread;
	
	threadinitptr=(listenerthreadinit*)lpParameter;
	thread=threadinitptr->thread;

	thread(threadinitptr->ftdihandle);

	return 0;
}

int32_t FTDIListener(ftdi_context ftdihandle)
{
	int32_t returnvalue,i;
	
	#ifdef DEBUG
	printf("FTDIListener\n");
	#endif 
	do
	{	
		do
		{
			returnvalue=p_ftdi_read_data(ftdihandle, RXBUFFER, 2);
		}while(returnvalue==0);
		if(returnvalue>0)
		{
			for(i=0;i<returnvalue;i++)
			{				
				rx_fifo.BUFFER[(rx_fifo.ptr_in)&(BUFFERSIZE-1)]=RXBUFFER[i];
				rx_fifo.ptr_in=(rx_fifo.ptr_in+1)&(BUFFERSIZE-1);
			}

			#ifdef DEBUG
				printf("rx : %d %d \n",rx_fifo.ptr_in,returnvalue);
			#endif
			if(READ_THREAD_EVENT)setevent(READ_THREAD_EVENT);
		}
		else
		{
			#ifdef DEBUG
				printf("FTDIListener : ftdi_read_data=%d\n",returnvalue);
			#endif

			stop_thread=1;
			if(READ_THREAD_EVENT)setevent(READ_THREAD_EVENT);
		}

	}while(!stop_thread);
	setevent(STOP_THREAD_EVENT);
	return 0;
}

int32_t createlistenerthread(RDTHREADFUNCTION thread,int32_t priority,ftdi_context ftdihandle)
{
	uint32_t sit;
	pthread_t threadid;
	listenerthreadinit *threadinitptr;
	pthread_attr_t threadattrib;
	struct sched_param param;
	int ret;

	sit = 0;

	//pthread_attr_create(&threadattrib);
	pthread_attr_init(&threadattrib);
	pthread_attr_setinheritsched(&threadattrib,PTHREAD_EXPLICIT_SCHED);
	//pthread_attr_setsched(&threadattrib,SCHED_FIFO);
	//pthread_attr_setprio(&threadattrib,4);
	
	pthread_attr_setschedpolicy(&threadattrib,SCHED_FIFO);

#ifndef __EMSCRIPTEN__
	param.sched_priority = sched_get_priority_max(SCHED_FIFO) ;
#endif

	/* set the new scheduling param */
	pthread_attr_setschedparam (&threadattrib, &param);

	threadinitptr=(listenerthreadinit *)malloc(sizeof(listenerthreadinit));
	threadinitptr->thread=thread;
	threadinitptr->ftdihandle=ftdihandle;

	ret = pthread_create(&threadid,0,FTDIListenerThreadProc, threadinitptr);
	if(ret)
	{
#ifdef DEBUG		
		printf("createlistenerthread : pthread_create failed -> %d",ret);
#endif		
	}
	return sit;
}

#endif

int32_t ftdi_load_lib (HXCFE* floppycontext)
{

#ifdef DEBUG
	printf("---ftdi_load_lib---\n");
#endif

	#ifdef WIN32

	HMODULE h;

	h = LoadLibrary ("ftd2xx.dll");
	if (h)
	{
		pFT_Write = (FT_WRITE)GetProcAddress (h, "FT_Write");
		pFT_Read = (FT_READ)GetProcAddress (h, "FT_Read");
		pFT_GetStatus = (FT_GETSTATUS)GetProcAddress (h, "FT_GetStatus");
		pFT_Open = (FT_OPEN)GetProcAddress (h, "FT_Open");
		pFT_Close = (FT_CLOSE)GetProcAddress (h, "FT_Close");
		pFT_Purge = (FT_PURGE)GetProcAddress (h, "FT_Purge");
		pFT_SetUSBParameters = (FT_SETUSBPARAMETERS)GetProcAddress (h, "FT_SetUSBParameters");
		pFT_SetLatencyTimer = (FT_SETLATENCYTIMER)GetProcAddress (h, "FT_SetLatencyTimer");
		pFT_SetEventNotification = 	(FT_SETEVENTNOTIFICATION)GetProcAddress (h, "FT_SetEventNotification");

		if(pFT_Write &&  pFT_Read && pFT_GetStatus && pFT_Open && pFT_Purge && pFT_SetUSBParameters && pFT_SetLatencyTimer && pFT_SetEventNotification)
		{
			floppycontext->hxc_printf(MSG_INFO_1,"FTDI library loaded successfully!");
			return 1;
		}
		else
		{
			floppycontext->hxc_printf(MSG_ERROR,"Error while loading FTDI library! Missing entry point ?");
			return -3;
		}
	}
	else
	{
			floppycontext->hxc_printf(MSG_ERROR,"Error while loading FTDI library! library not found !");
			return -1;
	}

	#endif

	#if defined(__APPLE__) || defined(__linux__)

	void* lib_handle;

	#if !defined(FTDILIB)

	#if defined __APPLE__
	lib_handle = dlopen("libftd2xx.dylib", RTLD_LOCAL|RTLD_LAZY);
	#else
	lib_handle = dlopen("/usr/local/lib/libftd2xx.so", RTLD_LOCAL|RTLD_LAZY);
	#endif

	if (!lib_handle) {
		floppycontext->hxc_printf(MSG_ERROR,"Error while loading FTDI library! library not found !");
		return -1;
	}

	pFT_Write = (FT_WRITE)dlsym(lib_handle, "FT_Write");
	pFT_Read = (FT_READ)dlsym(lib_handle, "FT_Read");
	pFT_GetStatus = (FT_GETSTATUS)dlsym(lib_handle, "FT_GetStatus");
	pFT_Open = (FT_OPEN)dlsym(lib_handle, "FT_Open");
	pFT_Close = (FT_CLOSE)dlsym(lib_handle, "FT_Close");
	pFT_Purge = (FT_PURGE)dlsym(lib_handle, "FT_Purge");
	pFT_SetUSBParameters = (FT_SETUSBPARAMETERS)dlsym(lib_handle, "FT_SetUSBParameters");
	pFT_SetLatencyTimer = (FT_SETLATENCYTIMER)dlsym(lib_handle, "FT_SetLatencyTimer");
	pFT_SetEventNotification = 	(FT_SETEVENTNOTIFICATION)dlsym(lib_handle,"FT_SetEventNotification");

	if(pFT_Write &&  pFT_Read && pFT_GetStatus && pFT_Open && pFT_Purge && pFT_SetUSBParameters && pFT_SetLatencyTimer && pFT_SetEventNotification)
	{
		floppycontext->hxc_printf(MSG_INFO_1,"FTDI library loaded successfully!");
		return 1;
	}
	else
	{
		floppycontext->hxc_printf(MSG_ERROR,"Error while loading FTDI library! Missing entry point ?");
		return -3;
	}
	
	#else // defined(FTDILIB)

		lib_handle = dlopen("libftdi.so", RTLD_LOCAL|RTLD_LAZY);

		if (!lib_handle) {
			floppycontext->hxc_printf(MSG_ERROR,"Error while loading FTDI library! libftdi.so not found !");
			return -1;
		}

		p_ftdi_new =                      (P_FTDI_NEW)                     dlsym(lib_handle, "ftdi_new");
		p_ftdi_init =                     (P_FTDI_INIT)                    dlsym(lib_handle, "ftdi_init");
		p_ftdi_usb_open =                 (P_FTDI_USB_OPEN)                dlsym(lib_handle, "ftdi_usb_open");
		p_ftdi_usb_close =                (P_FTDI_USB_CLOSE)               dlsym(lib_handle, "ftdi_usb_close");
		p_ftdi_usb_purge_rx_buffer =      (P_FTDI_USB_PURGE_RX_BUFFER)     dlsym(lib_handle, "ftdi_usb_purge_rx_buffer");
		p_ftdi_usb_purge_tx_buffer =      (P_FTDI_USB_PURGE_TX_BUFFER)     dlsym(lib_handle, "ftdi_usb_purge_tx_buffer");
		p_ftdi_usb_purge_buffers =        (P_FTDI_USB_PURGE_BUFFERS)       dlsym(lib_handle, "ftdi_usb_purge_buffers");	
		p_ftdi_read_data_set_chunksize =  (P_FTDI_READ_DATA_SET_CHUNKSIZE) dlsym(lib_handle, "ftdi_read_data_set_chunksize");
		p_ftdi_write_data_set_chunksize = (P_FTDI_WRITE_DATA_SET_CHUNKSIZE)dlsym(lib_handle, "ftdi_write_data_set_chunksize");
		p_ftdi_set_latency_timer = 	      (P_FTDI_SET_LATENCY_TIMER)       dlsym(lib_handle, "ftdi_set_latency_timer");
		p_ftdi_write_data = 	          (P_FTDI_WRITE_DATA)              dlsym(lib_handle, "ftdi_write_data");
		p_ftdi_read_data =                (P_FTDI_READ_DATA)               dlsym(lib_handle, "ftdi_read_data");

		if( p_ftdi_new && p_ftdi_init &&  p_ftdi_usb_open && p_ftdi_usb_close && p_ftdi_usb_purge_rx_buffer &&
		    p_ftdi_usb_purge_tx_buffer && p_ftdi_usb_purge_buffers && p_ftdi_read_data_set_chunksize && p_ftdi_write_data_set_chunksize &&
			p_ftdi_set_latency_timer && p_ftdi_write_data && p_ftdi_read_data)
		{
			ftdic = p_ftdi_new();
			if(ftdic)
			{
				floppycontext->hxc_printf(MSG_INFO_1,"FTDI library loaded successfully!");
				if( !p_ftdi_init(ftdic) )
				{
					return 1;
				}
			}
		}
		else
		{
			floppycontext->hxc_printf(MSG_ERROR,"Error while loading FTDI library! Missing entry point ?");
			return -3;
		}		
		
		return 1;

	#endif

	#endif

	return -1;
}

int32_t open_ftdichip(void ** ftdihandle)
{
#ifdef DEBUG
	printf("---open_ftdichip---\n");
#endif

#if defined(FTDILIB)

	if( p_ftdi_usb_open(ftdic, 0x0403, 0x6001) < 0 )
	{
		*ftdihandle=0;
		return -1;
	}
	else
	{
		stop_thread=0;
		*ftdihandle=(void*)(ftdic);
		STOP_THREAD_EVENT=createevent();
		READ_THREAD_EVENT=0;
		tx_fifo.ptr_in=0;
		tx_fifo.ptr_out=0;
		memset(tx_fifo.BUFFER,0,BUFFERSIZE);
		rx_fifo.ptr_in=0;
		rx_fifo.ptr_out=0;
		memset(rx_fifo.BUFFER,0,BUFFERSIZE);

		createlistenerthread(FTDIListener,128,ftdic);

		return 0;
	}

#else
	int32_t i;

	i=0;
	do
	{
		if(pFT_Open(i,(FT_HANDLE*)ftdihandle)==FT_OK)
		{
			return 0;
		}

		i++;
	}while(i<4);

#endif

	*ftdihandle=0;

	return -1;
}

int32_t close_ftdichip(void * ftdihandle)
{

#ifdef DEBUG
	printf("---close_ftdichip---\n");
#endif

#if defined(FTDILIB)

	stop_thread=1;
	waitevent(STOP_THREAD_EVENT,10000);
	p_ftdi_usb_close((ftdi_context)ftdihandle);
	destroyevent(STOP_THREAD_EVENT);

#else

	if(pFT_Close((FT_HANDLE*)ftdihandle)!=FT_OK)
	{
		return -1;
	}

#endif
	return 0;
}

int32_t purge_ftdichip(void* ftdihandle,uint32_t buffer)
{
#if defined(FTDILIB)
	int32_t ret;

	ret = 0;
#endif

#ifdef DEBUG
	printf("---purge_ftdichip---\n");
#endif

#if defined(FTDILIB)

	p_ftdi_usb_purge_rx_buffer((ftdi_context)ftdihandle);
	p_ftdi_usb_purge_tx_buffer((ftdi_context)ftdihandle);
	ret=p_ftdi_usb_purge_buffers((ftdi_context)ftdihandle);

	rx_fifo.ptr_in=0;
	rx_fifo.ptr_out=0;
	
	if(ret)
	{
		return -1;
	}

#else

	if(pFT_Purge((FT_HANDLE*)ftdihandle,buffer)!=FT_OK)
	{
		return -1;
	}

#endif
	return 0;
}

int32_t setusbparameters_ftdichip(void * ftdihandle,uint32_t buffersizetx,uint32_t buffersizerx)
{

#ifdef DEBUG
	printf("---setusbparameters_ftdichip---\n");
#endif

#if defined(FTDILIB)

	p_ftdi_read_data_set_chunksize((ftdi_context)ftdihandle, buffersizerx);
	p_ftdi_write_data_set_chunksize((ftdi_context)ftdihandle, buffersizetx);

#else

	if(pFT_SetUSBParameters ((FT_HANDLE*)ftdihandle,buffersizerx,buffersizetx)!=FT_OK)
	{
		return -1;
	}

#endif
	return 0;
}

int32_t setlatencytimer_ftdichip(void* ftdihandle,unsigned char latencytimer_ms)
{

#ifdef DEBUG
	printf("---setlatencytimer_ftdichip---\n");
#endif

#if defined(FTDILIB)

	if(p_ftdi_set_latency_timer((ftdi_context)ftdihandle, latencytimer_ms)<0)
	{
		return -1;
	}

#else

	if(pFT_SetLatencyTimer ((FT_HANDLE*)ftdihandle,latencytimer_ms)!=FT_OK)
	{
		return -1;
	}

#endif

	return 0;
}

int32_t write_ftdichip(void* ftdihandle,unsigned char * buffer,uint32_t size)
{
#if defined(FTDILIB)
	int32_t dwWritten;
#else
	DWORD dwWritten;
#endif

#ifdef DEBUG
	printf("---write_ftdichip---\n");
#endif
	
#if defined(FTDILIB)

	dwWritten=p_ftdi_write_data((ftdi_context)ftdihandle, buffer, size);
	if(dwWritten<0)
	{
		return -1;
	}

	return dwWritten;
#else

	if(pFT_Write ((FT_HANDLE*)ftdihandle, buffer, size,&dwWritten)!=FT_OK)
	{
		return -1;
	}

	return (int32_t)dwWritten;
#endif

}

int32_t read_ftdichip(void* ftdihandle,unsigned char * buffer,uint32_t size)
{
#if defined(FTDILIB)
	int32_t nb_of_byte;
#else
	DWORD returnvalue;
#endif

#ifdef DEBUG
	printf("---read_ftdichip---\n");
#endif

#if defined(FTDILIB)

	nb_of_byte=0;
	while((rx_fifo.ptr_in != rx_fifo.ptr_out) && (nb_of_byte<size))
	{
		buffer[nb_of_byte]=rx_fifo.BUFFER[(rx_fifo.ptr_out)&(BUFFERSIZE-1)];
		nb_of_byte++;
		rx_fifo.ptr_out=(rx_fifo.ptr_out+1)&(BUFFERSIZE-1);
	}

	if(stop_thread)
	{
		return -1;
	}
	else
	{
		return nb_of_byte;
	}

#else

	if(pFT_Read((FT_HANDLE*)ftdihandle,buffer,size,&returnvalue)!=FT_OK)
	{
		return -1;
	}

	return (int32_t)returnvalue;

#endif
}

int32_t getfifostatus_ftdichip(void* ftdihandle,int32_t * txlevel,int32_t *rxlevel,uint32_t * event)
{
#if defined(FTDILIB)
	int32_t nb_of_byte,ptr_out;
#endif

#ifdef DEBUG
	printf("---getfifostatus_ftdichip---\n");
#endif

#if defined(FTDILIB)

	if(rx_fifo.ptr_in != rx_fifo.ptr_out)
	{
		nb_of_byte=0;
		ptr_out=rx_fifo.ptr_out&(BUFFERSIZE-1);
		while((rx_fifo.ptr_in != ptr_out))
		{
			nb_of_byte++;
			ptr_out=(ptr_out+1)&(BUFFERSIZE-1);
		}
		*event=FT_EVENT_RXCHAR;
		*rxlevel=nb_of_byte;
		//printf("%d...\n",*rxlevel);
	}
	else
	{
		*txlevel=0;
		*rxlevel=0;
		*event=0;
	}

	return 0;

#else
	DWORD tmp_rxlevel,tmp_txlevel;

	tmp_rxlevel = (DWORD)(*rxlevel);
	tmp_txlevel = (DWORD)(*txlevel);

	if(pFT_GetStatus((FT_HANDLE*)ftdihandle,&tmp_rxlevel,&tmp_txlevel,(DWORD*)event)!=FT_OK)
	{
		*rxlevel = (int32_t)tmp_rxlevel;
		*txlevel = (int32_t)tmp_txlevel;

		return -1;
	}

	*rxlevel = (int32_t)tmp_rxlevel;
	*txlevel = (int32_t)tmp_txlevel;

	return 0;

#endif
}

int32_t seteventnotification_ftdichip(void* ftdihandle,uint32_t eventmask,void * event)
{

#ifdef DEBUG
	printf("---seteventnotification_ftdichip---\n");
#endif

#ifdef FTDILIB

	READ_THREAD_EVENT=(EVENT_HANDLE*)event;

#else

	if(pFT_SetEventNotification((FT_HANDLE*)ftdihandle,eventmask,event)!=FT_OK)
	{
		return -1;
	}

#endif

	return 0;
}

