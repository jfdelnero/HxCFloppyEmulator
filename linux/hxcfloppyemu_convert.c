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
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h> 
#include <time.h>

#include "hxc_floppy_emulator.h"

int CUI_affiche(int MSGTYPE,char * chaine, ...)
{
	if(MSGTYPE!=MSG_DEBUG)
	{
		va_list marker;
		va_start( marker, chaine );     
		
		vprintf(chaine,marker);
		printf("\n");

		va_end( marker );
	}
    return 0;
}

char * gettype(char * str_type)
{	
	if(!strcmp(str_type,"-HFE"))
	{
		return PLUGIN_HXC_HFE;	
	}

	if(!strcmp(str_type,"-AFI"))
	{
		return PLUGIN_HXC_AFI;	
	}

	if(!strcmp(str_type,"-CPCDSK"))
	{
		return PLUGIN_AMSTRADCPC_DSK;	
	}

	if(!strcmp(str_type,"-RAW"))
	{
		return PLUGIN_RAW_LOADER;	
	}

	if(!strcmp(str_type,"-MFM"))
	{
		return PLUGIN_HXC_MFM;
	}

	return 0;
}



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

int main(int argc, char* argv[])
{
	int ret;
	char filename[512];
	char * output_file_type;

	FLOPPY * floppydisk;
	HXCFLOPPYEMULATOR* hxcfe;

	hxcfe=hxcfe_init();
	hxcfe_set_outputfunc(hxcfe,&CUI_affiche);

	printf("HxC Floppy Emulator : Floppy image file converter\n");
	printf("Copyright (C) 2006-2011 Jean-Francois DEL NERO\n");
   	printf("This program comes with ABSOLUTELY NO WARRANTY\n");
   	printf("This is free software, and you are welcome to redistribute it\n");
   	printf("under certain conditions;\n\n");
		
	if(argv[1] && argv[2])
	{
		output_file_type=gettype(argv[2]);
		if(output_file_type)
		{
			hxcfe_select_container(hxcfe,"AUTOSELECT");
			floppydisk=hxcfe_floppy_load(hxcfe,argv[1],&ret);
			
			if(ret!=HXCFE_NOERROR || !floppydisk)
			{
				switch(ret)
				{
					case HXCFE_UNSUPPORTEDFILE:
						printf("Load error!: Image file not yet supported!\n");
					break;
					case HXCFE_FILECORRUPTED:
						printf("Load error!: File corrupted ? Read error ?\n");
					break;
					case HXCFE_ACCESSERROR:
						printf("Load error!:  Read file error!\n");
					break;
					default:
						printf("Load error! error %d\n",ret);
					break;
				}
			}
			else
			{
				get_filename(argv[1],filename);
				strcat(filename,".hfe");
				
				hxcfe_select_container(hxcfe,output_file_type);

				hxcfe_floppy_export(hxcfe,floppydisk,filename);
				
				hxcfe_floppy_unload(hxcfe,floppydisk);
			}
		}
		else
		{
			printf("Syntax: %s [image-file] [-HFE/-AFI/-CPCDSK/-RAW/-MFM]\n",argv[0]);
		}
	}
	else
	{
		printf("Syntax: %s [image-file] [-HFE/-AFI/-CPCDSK/-RAW/-MFM]\n",argv[0]);
	}

	hxcfe_deinit(hxcfe);
	
	return 0;
}


