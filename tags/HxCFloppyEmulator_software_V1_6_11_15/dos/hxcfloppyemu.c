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
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "hxc_floppy_emulator.h"
#include "internal_floppy.h"
#include "floppy_loader.h"
#include "mfm_file_writer.h"

//#include "win32_api.h"



HXCFLOPPYEMULATOR * flopemu;


int CUI_affiche(int MSGTYPE,char * chaine, ...)
{
	//if(MSGTYPE!=MSG_DEBUG)
	{
		va_list marker;
		va_start( marker, chaine );

		vprintf(chaine,marker);
		printf("\n");

		va_end( marker );
	}
    return 0;
}


int main(int argc, char* argv[])
{
	FLOPPY * thefloppydisk;
	int ret;
	unsigned char old_trackpos,trackpos;

	printf("HxC Floppy Emulator\n");
	flopemu=(HXCFLOPPYEMULATOR*)malloc(sizeof(HXCFLOPPYEMULATOR));

	flopemu->hxc_printf=&CUI_affiche;

	initHxCFloppyEmulator(flopemu);



		if(argv[1])
		{
			thefloppydisk=(FLOPPY *)malloc(sizeof(FLOPPY));
			memset(thefloppydisk,0,sizeof(FLOPPY));

			ret=floppy_load(flopemu,thefloppydisk,argv[1]);

			if(ret!=LOADER_NOERROR)
			{
				switch(ret)
				{
					case LOADER_UNSUPPORTEDFILE:
						printf("Load error!: Image file not yet supported!\n");
					break;
					case LOADER_FILECORRUPT:
						printf("Load error!: File corrupted ? Read error ?\n");
					break;
					case LOADER_ACCESSERROR:
						printf("Load error!:  Read file error!\n");
					break;
					default:
						printf("Load error! error %d\n",ret);
					break;
				}
				free(thefloppydisk);
			}
			else
			{
				write_MFM_file(flopemu,thefloppydisk,"test.mfm");	

				floppy_unload(flopemu,thefloppydisk);
				
			}
		}
	free(flopemu);
	return 0;
}

