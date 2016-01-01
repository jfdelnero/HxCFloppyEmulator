/*
//
// Copyright (C) 2006-2016 Jean-François DEL NERO
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void get_filename(char * path,char * filename)
{
	int i,done;

	i=strlen(path);
	done=0;
	while(i && !done)
	{
		i--;

		if(path[i]=='/')
		{
			done=1;
			i++;
		}
	}

	sprintf(filename,"%s",&path[i]);

	i=0;
	while(filename[i])
	{
		if(filename[i]=='.')
		{
			filename[i]='_';
		}

		i++;
	}

	return;
}
