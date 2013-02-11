/*
//
// Copyright (C) 2006, 2007, 2008, 2009, 2010, 2011 Jean-François DEL NERO
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
#elif defined(__APPLE__) || defined(__linux__)
	#include <dlfcn.h>
#endif

#if defined(__APPLE__)
	#include "../macosx/ftd2xx.h"
#endif

#if defined(__linux__)
	#include "../linux/ftd2xx.h"
#endif

#include "ftdi.h"

typedef FT_STATUS (WINAPI * FT_OPEN)(int deviceNumber,	FT_HANDLE *pHandle);
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


int ftdi_load_lib (HXCFLOPPYEMULATOR* floppycontext)
{
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

	#endif

	return -1;
}

int open_ftdichip(unsigned long * ftdihandle)
{

	int i;

	i=0;
	do
	{
		if(pFT_Open(i,(FT_HANDLE*)ftdihandle)==FT_OK)
		{
			return 0;
		}

		i++;
	}while(i<4);

	*ftdihandle=0;

	return -1;
}

int close_ftdichip(unsigned long ftdihandle)
{
	if(pFT_Close((FT_HANDLE*)ftdihandle)!=FT_OK)
	{
		return -1;
	}
	return 0;
}

int purge_ftdichip(unsigned long ftdihandle,unsigned long buffer)
{
	if(pFT_Purge((FT_HANDLE*)ftdihandle,buffer)!=FT_OK)
	{
		return -1;
	}
	return 0;
}

int setusbparameters_ftdichip(unsigned long ftdihandle,unsigned long buffersizetx,unsigned long buffersizerx)
{
	if(pFT_SetUSBParameters ((FT_HANDLE*)ftdihandle,buffersizerx,buffersizetx)!=FT_OK)
	{
		return -1;
	}
	return 0;
}

int setlatencytimer_ftdichip(unsigned long ftdihandle,unsigned char latencytimer_ms)
{
	if(pFT_SetLatencyTimer ((FT_HANDLE*)ftdihandle,latencytimer_ms)!=FT_OK)
	{
		return -1;
	}
	return 0;
}

int write_ftdichip(unsigned long ftdihandle,unsigned char * buffer,unsigned int size)
{
	int dwWritten;

	if(pFT_Write ((FT_HANDLE*)ftdihandle, buffer, size,&dwWritten)!=FT_OK)
	{
		return -1;
	}

	return dwWritten;
}

int read_ftdichip(unsigned long ftdihandle,unsigned char * buffer,unsigned int size)
{
	int returnvalue;
	if(pFT_Read((FT_HANDLE*)ftdihandle,buffer,size,&returnvalue)!=FT_OK)
	{
		return -1;
	}
	return returnvalue;
}

int getfifostatus_ftdichip(unsigned long ftdihandle,int * txlevel,int *rxlevel,unsigned long * event)
{
	if(pFT_GetStatus((FT_HANDLE*)ftdihandle,rxlevel,txlevel,(DWORD*)event)!=FT_OK)
	{
		return -1;
	}

	return 0;
}

int seteventnotification_ftdichip(unsigned long ftdihandle,unsigned long eventmask,void * event)
{
	if(pFT_SetEventNotification((FT_HANDLE*)ftdihandle,eventmask,event)!=FT_OK)
	{
		return -1;
	}
	return 0;
}

