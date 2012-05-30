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
// File : fs.c
// Contains: UTF-8 / Ascii path file system functions
//
// Written by:	DEL NERO Jean Francois
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////


#include <assert.h>
#include <stdio.h>
#include <limits.h>
#include <errno.h>
#include <sys/types.h>

#ifdef WIN32
# include <windows.h>
# include <io.h>
# include <fcntl.h>
#else
# include <unistd.h>
#endif

#ifdef WIN32

int convertpath (const char * path, wchar_t * wpath)
{
    if (!MultiByteToWideChar (CP_UTF8, 0, path, -1, wpath, MAX_PATH))
    {
        errno = ENOENT;
        return -1;
    }
    wpath[MAX_PATH] = L'\0';
    return 0;
}
#else

#endif


int hxc_open (const char *filename, int flags, ...)
{
	
#ifdef WIN32
	wchar_t wpath[MAX_PATH+1];
#else
	const char *local_name;
#endif

    unsigned int mode = 0;
    va_list ap;

    va_start (ap, flags);
    if (flags & O_CREAT)
        mode = va_arg (ap, unsigned int);
    va_end (ap);

#ifdef WIN32

	memset(wpath,0,(MAX_PATH+1)*sizeof(wchar_t));
    if(convertpath(filename, wpath)<0)
		return -1;

    return _wopen (wpath, flags, mode);

#else

    local_name = ToLocale (filename);

    if (local_name == NULL)
    {
        errno = ENOENT;
        return -1;
    }

    int fd = open (local_name, flags, mode);

    LocaleFree (local_name);
    return fd;

#endif


}

FILE *hxc_fopen (const char *filename, const char *mode)
{
    int rwflags, oflags;
    int append;
	int fd;
	const char *ptr;
	FILE *stream;

	// Try the classic way (ascii path)	
	stream = fopen(filename,mode);
	

	if( ( stream == NULL ) && (errno == ENOENT) )
	{
		// Try the utf8 way
		rwflags = 0;
		oflags = 0;
		append = 0;

		for (ptr = mode; *ptr; ptr++)
		{
			switch (*ptr)
			{
				case 'r':
					rwflags = O_RDONLY;
					break;
				case 'b':
					oflags |= O_BINARY;
					break;
				case 'a':
					rwflags = O_WRONLY;
					oflags |= O_CREAT;
					append = 1;
					break;

				case 'w':
					rwflags = O_WRONLY;
					oflags |= O_CREAT | O_TRUNC;
					break;

				case '+':
					rwflags = O_RDWR;
					break;

	#ifdef O_TEXT
				case 't':
					oflags |= O_TEXT;
					break;
	#endif
			}
		}

		fd = hxc_open (filename, rwflags | oflags, 0666);
		if (fd == -1)
			return NULL;

		if(append)
		{
			if (lseek (fd, 0, SEEK_END) == -1)
			{
				close (fd);
				return NULL;
			}
		}
		else
		{
			lseek (fd, 0, SEEK_SET);
		}

		stream = fdopen (fd, mode);
		if (stream == NULL)
			close (fd);
	}

    return stream;
}

int hxc_fclose(FILE * f)
{
	return fclose(f);
}
