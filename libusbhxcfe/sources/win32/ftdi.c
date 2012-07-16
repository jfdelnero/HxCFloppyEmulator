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

#ifdef WIN32
#include <conio.h> 
#include <ftd2xx.h>
#include <winioctl.h>
#endif

#include "ftdi.h"

#ifdef WIN32
typedef FT_STATUS (WINAPI * FT_OPEN)(int deviceNumber,	FT_HANDLE *pHandle);
//extern  FT_OPEN pFT_Open;

typedef FT_STATUS (WINAPI * FT_READ)( FT_HANDLE ftHandle,LPVOID lpBuffer,DWORD nBufferSize,LPDWORD lpBytesReturned);
//extern  FT_READ pFT_Read;

typedef FT_STATUS (WINAPI * FT_WRITE)(FT_HANDLE ftHandle,LPVOID lpBuffer,DWORD nBufferSize,LPDWORD lpBytesWritten);
//extern  FT_WRITE pFT_Write;

typedef FT_STATUS (WINAPI *  FT_GETSTATUS)(FT_HANDLE ftHandle,DWORD *dwRxBytes,DWORD *dwTxBytes,DWORD *dwEventDWord);
//extern  FT_GETSTATUS pFT_GetStatus;
///
typedef FT_STATUS (WINAPI * FT_PURGE)(FT_HANDLE ftHandle,ULONG Mask);
//extern  FT_PURGE pFT_Purge;

typedef FT_STATUS (WINAPI * FT_SETUSBPARAMETERS)(FT_HANDLE ftHandle,ULONG ulInTransferSize,ULONG ulOutTransferSize);
//extern  FT_SETUSBPARAMETERS pFT_SetUSBParameters;

typedef FT_STATUS (WINAPI * FT_SETLATENCYTIMER)(FT_HANDLE ftHandle,UCHAR ucLatency);
//extern  FT_SETLATENCYTIMER pFT_SetLatencyTimer;

typedef FT_STATUS (WINAPI * FT_SETEVENTNOTIFICATION)(FT_HANDLE ftHandle,DWORD dwEventMask, PVOID pvArg);
//extern  FT_SETEVENTNOTIFICATION pFT_SetEventNotification;

typedef FT_STATUS (WINAPI * FT_CLOSE)(FT_HANDLE ftHandle);
//extern  FT_CLOSE pFT_Close;

FT_OPEN pFT_Open;
FT_READ pFT_Read;
FT_WRITE pFT_Write;
FT_GETSTATUS pFT_GetStatus;
FT_PURGE pFT_Purge;
FT_SETUSBPARAMETERS pFT_SetUSBParameters;
FT_SETLATENCYTIMER pFT_SetLatencyTimer;
FT_SETEVENTNOTIFICATION pFT_SetEventNotification;
FT_CLOSE pFT_Close;
#endif

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
	return -1;
}


int open_ftdichip(unsigned long * ftdihandle)
{
#ifdef WIN32
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

    #endif
	*ftdihandle=0;
    
	return -1;	
}

int close_ftdichip(unsigned long ftdihandle)
{
#ifdef WIN32    
	if(pFT_Close((FT_HANDLE*)ftdihandle)!=FT_OK)
	{
		return -1;
	}
#endif
	return 0;
}

int purge_ftdichip(unsigned long ftdihandle,unsigned long buffer)
{
#ifdef WIN32    
	if(pFT_Purge((FT_HANDLE*)ftdihandle,buffer)!=FT_OK)
	{
		return -1;
	}
#endif
	return 0;
}

int setusbparameters_ftdichip(unsigned long ftdihandle,unsigned long buffersizetx,unsigned long buffersizerx)
{
#ifdef WIN32    
	if(pFT_SetUSBParameters ((FT_HANDLE*)ftdihandle,buffersizerx,buffersizetx)!=FT_OK)
	{
		return -1;
	}
#endif
	return 0;
}

int setlatencytimer_ftdichip(unsigned long ftdihandle,unsigned char latencytimer_ms)
{
#ifdef WIN32    
	if(pFT_SetLatencyTimer ((FT_HANDLE*)ftdihandle,latencytimer_ms)!=FT_OK)
	{
		return -1;
	}
#endif
	return 0;
}

int write_ftdichip(unsigned long ftdihandle,unsigned char * buffer,unsigned int size)
{
#ifdef WIN32    
	int dwWritten;
	
	if(pFT_Write ((FT_HANDLE*)ftdihandle, buffer, size,&dwWritten)!=FT_OK)
	{
		return -1;
	}

	return dwWritten;
#else
    return -1;
#endif
}

int read_ftdichip(unsigned long ftdihandle,unsigned char * buffer,unsigned int size)
{
#ifdef WIN32    
	int returnvalue;
	if(pFT_Read((FT_HANDLE*)ftdihandle,buffer,size,&returnvalue)!=FT_OK)
	{
		return -1;
	}
	return returnvalue;
#else
    return -1;
#endif
}

int getfifostatus_ftdichip(unsigned long ftdihandle,int * txlevel,int *rxlevel,unsigned long * event)
{
#ifdef WIN32    
	if(pFT_GetStatus((FT_HANDLE*)ftdihandle,rxlevel,txlevel,event)!=FT_OK)
	{
		return -1;
	}

	return 0;
#else
    return -1;
#endif
}

int seteventnotification_ftdichip(unsigned long ftdihandle,unsigned long eventmask,void * event)
{
#ifdef WIN32
	if(pFT_SetEventNotification((FT_HANDLE*)ftdihandle,eventmask,event)!=FT_OK)
	{
		return -1;
	}
	return 0;
#else
    return -1;
#endif
}
/////////////
