
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <direct.h>

#include "hxc_floppy_emulator.h"
#include "licensetxt.h"
#include "./usb_floppyemulator/usb_hxcfloppyemulator.h"

#include "win32_api.h"



HXCFLOPPYEMULATOR * flopemu;
HWINTERFACE * hwif;
int output_file_format;

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



int browse_and_convert_directory(HXCFLOPPYEMULATOR* floppycontext,char * folder,char * destfolder,char * file)
{
	long hfindfile;
	filefoundinfo FindFileData;
	int bbool;
	int ret,i;
	char * destinationfolder;
	char * destinationfile;
	FLOPPY * thefloppydisk;
	unsigned char * fullpath;

	hfindfile=find_first_file(folder,file, &FindFileData); 
	if(hfindfile!=-1)
	{
		bbool=1;
		while(hfindfile!=-1 && bbool)
		{
			if(FindFileData.isdirectory)
			{
				if(strcmp(".",FindFileData.filename)!=0 && strcmp("..",FindFileData.filename)!=0)
				{
					destinationfolder=malloc(strlen(FindFileData.filename)+strlen(destfolder)+2);
					sprintf(destinationfolder,"%s\\%s",destfolder,FindFileData.filename);

					printf("Creating directory %s\n",destinationfolder);
					mkdir(destinationfolder);

					fullpath=malloc(strlen(FindFileData.filename)+strlen(folder)+2);
		            sprintf(fullpath,"%s\\%s",folder,FindFileData.filename);

					floppycontext->hxc_printf(MSG_INFO_1,"Entering directory %s",FindFileData.filename);
					if(browse_and_convert_directory(floppycontext,fullpath,destinationfolder,file))
					{
						free(destinationfolder);
						free(fullpath);
						return 1;
					}
					free(destinationfolder);
					free(fullpath);
					floppycontext->hxc_printf(MSG_INFO_1,"Leaving directory %s",FindFileData.filename);
					
				}
			}
			else
			{			
				floppycontext->hxc_printf(MSG_INFO_1,"converting file %s, %dB",FindFileData.filename,FindFileData.size);
				
				if(FindFileData.size)
				{

					fullpath=malloc(strlen(FindFileData.filename)+strlen(folder)+2);
		            sprintf(fullpath,"%s\\%s",folder,FindFileData.filename);

					thefloppydisk=(FLOPPY*)malloc(sizeof(FLOPPY));
			        ret=floppy_load(floppycontext,thefloppydisk,fullpath);
		            free(fullpath);

					if(ret!=HXCFE_NOERROR)
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
						destinationfile=malloc(strlen(FindFileData.filename)+strlen(destfolder)+2+4);
					    sprintf(destinationfile,"%s\\%s",destfolder,FindFileData.filename);
						i=strlen(destinationfile);
						do
						{
							i--;
						}while(i && destinationfile[i]!='.');

						if(i)
						{
                          destinationfile[i]='_';
						}
					
					    //printf("Creating file %s\n",destinationfile);

						switch(output_file_format)
						{
						case 0:
							strcat(destinationfile,".hfe");
							write_HFE_file(flopemu,thefloppydisk,destinationfile,-1,0);
							break;

						case 1:
							strcat(destinationfile,".afi");
							write_AFI_file(flopemu,thefloppydisk,destinationfile);
							break;

						case 2:
							strcat(destinationfile,".mfm");
							write_MFM_file(flopemu,thefloppydisk,destinationfile);
							break;

						case 3:
							strcat(destinationfile,".img");
							write_RAW_file(flopemu,thefloppydisk,destinationfile);
							break;

						case 4:
							strcat(destinationfile,".vtr");
							write_vtrucco_file(flopemu,thefloppydisk,destinationfile,-1);
							break;

						case 5:
							strcat(destinationfile,".dsk");
							write_CPCDSK_file(flopemu,thefloppydisk,destinationfile);
							break;

						}
						
						
						floppy_unload(floppycontext,thefloppydisk);
						free(destinationfile);

					}
					free(thefloppydisk);

				}	
			}
			
			bbool=find_next_file(hfindfile,folder,file,&FindFileData);	
		}
		
	}
	else printf("Error FindFirstFile\n");
	
	find_close(hfindfile);
	
	return 0;
}


int main(int argc, char* argv[])
{
	printf("HxC Floppy Emulator : Floppy image file converter V2.0\n");
	printf("Copyright (C) 2006-2011 Jean-Francois DEL NERO\n");
    printf("This program comes with ABSOLUTELY NO WARRANTY\n");
    printf("This is free software, and you are welcome to redistribute it\n");
    printf("under certain conditions; use the -lic option for details.\n\n");


	flopemu=(HXCFLOPPYEMULATOR*)malloc(sizeof(HXCFLOPPYEMULATOR));
	
	flopemu->hxc_printf=&CUI_affiche;
	
	output_file_format=0;

	if(argv[1] && argv[2])
	{
		if(argv[3])
		{
			if(!strcmp(argv[3],"-HFE"))
			{
				output_file_format=0;
				printf("HFE output file format.\n");
			}

			if(!strcmp(argv[3],"-AFI"))
			{
				output_file_format=1;
				printf("AFI output file format.\n");
			}

			if(!strcmp(argv[3],"-MFM"))
			{
				output_file_format=2;
				printf("MFM output file format.\n");
			}

			if(!strcmp(argv[3],"-IMG"))
			{
				output_file_format=3;
				printf("IMG output file format.\n");
			}

			if(!strcmp(argv[3],"-VTR"))
			{
				output_file_format=4;
				printf("VTrucco output file format.\n");
			}

			if(!strcmp(argv[3],"-DSK"))
			{
				output_file_format=5;
				printf("CPC DSK output file format.\n");
			}
				
		}
		initHxCFloppyEmulator(flopemu);
		browse_and_convert_directory(flopemu,argv[1],argv[2],"*.*");		
	}
	else
	{
		if(argv[1])
		{
			if(!strcmp(argv[1],"-lic"))
			{
				printf("%s",licensetxt);
			}
			else
			{
				printf("Syntax:\n %s [source dir] [destination dir] [-HFE/-AFI/-MFM/-IMG/-VTR/-DSK]\n",argv[0]);
			}
		}
		else
		{
			printf("Syntax:\n %s [source dir] [destination dir] [-HFE/-AFI/-MFM/-IMG/-VTR/-DSK]\n",argv[0]);
		}
	}

	free(flopemu);
	return 0;
}

