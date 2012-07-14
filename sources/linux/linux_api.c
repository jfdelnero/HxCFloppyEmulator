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
