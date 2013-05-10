/*
//
// Copyright (C) 2006 - 2013 Jean-François DEL NERO
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
// File : utils.cxx
// Contains: 
//
// Written by:	DEL NERO Jean Francois
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char * URIfilepathparser(char * URI,int size)
{
	char * path,c;
	int i,j;

	if(URI)
	{
		if(!strncmp(URI,"file://",7))
		{
			path = (char*)malloc(size + 1);
			if(path)
			{
				memset(path,0,size + 1);
				i=7;
				j=0;
				while(URI[i] && i < size)
				{
					if(URI[i] == '%')
					{
						if( URI[i + 1] && URI[i +2] )
						{
							c = URI[i + 1];
							if(c >= '0' && c<='9')
							{
								c = c - '0';
							}
							else
							{
								if(c >= 'a' && c<='f')
								{
									c = (c - 'a') + 10;
								}
								else
								{
									c = (c - 'A') + 10;
								}
							}

							path[j] = ( c * 0x10 );

							c = URI[i + 2];
							if(c >= '0' && c<='9')
							{
								c = c - '0';
							}
							else
							{
								if(c >= 'a' && c<='f')
								{
									c = (c - 'a') + 10;
								}
								else
								{
									c = (c - 'A') + 10;
								}

							}

							path[j] = path[j] | ( c * 0x1 );

							j++;
							i = i + 3;
						}
						else
						{
							i++;
						}
	
							
					}
					else
					{
						path[j] = URI[i];
						j++;
						i++;
					}
				
					path[j] = 0;
				}
			}
			
			return path;
		}
		else
		{
			path = 0;
			if(size)
			{
				path = (char*)malloc( size + 1);
				if(path)
				{
					strncpy(path,URI,size);
				}
			}
			return path;
		}
	}
	else
	{
		return URI;
	}
}

