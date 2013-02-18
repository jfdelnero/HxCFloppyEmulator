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

#include "hxc_floppy_emulator.h"
#include "internal_floppy.h"
#include "floppy_loader.h"
#include "dos_api.h"

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


void * ThreadProc( void *lpParameter)
{

	return 0;
}


int hxc_setevent(HXCFLOPPYEMULATOR* floppycontext,unsigned char id)
{
	return 0;
}

unsigned long  hxc_createevent(HXCFLOPPYEMULATOR* floppycontext,unsigned char id)
{
	return (unsigned long)0;
}

int hxc_waitevent(HXCFLOPPYEMULATOR* floppycontext,int id,int timeout)
{

	
}

void hxc_pause(int ms)
{
}

int hxc_createthread(HXCFLOPPYEMULATOR* floppycontext,void * hwcontext,THREADFUNCTION thread,int priority)
{
	return 0;
}

int getlistoffile(unsigned char * directorypath,unsigned char *** filelist)
{
	int numberoffile;
	char ** filepathtab;

	return 0;//numberoffile;
}


char * getcurrentdirectory(char *currentdirectory,int buffersize)
{
	memset(currentdirectory,0,buffersize);

	return 0;
}


int loaddiskplugins(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * newfloppy,char *pluginpath)
{

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

