/*
//
// Copyright (C) 2006-2012 Jean-François DEL NERO
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
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#include "libhxcfe.h"

#include "usb_hxcfloppyemulator.h"

int verbose;

int CUI_affiche(int MSGTYPE,char * chaine, ...)
{
	if(MSGTYPE!=MSG_DEBUG || verbose)
	{
		va_list marker;
		va_start( marker, chaine );     
		
		vprintf(chaine,marker);
		printf("\n");

		va_end( marker );
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

int isOption(int argc, char* argv[],char * paramtosearch,char * argtoparam)
{
	int param=1;
	int i,j;

	char option[512];

	memset(option,0,512);
	while(param<=argc)
	{
		if(argv[param])
		{			
			if(argv[param][0]=='-')
			{
				memset(option,0,512);

				j=0;
				i=1;
				while( argv[param][i] && argv[param][i]!=':')
				{
					option[j]=argv[param][i];
					i++;
					j++;
				}

				if( !strcmp(option,paramtosearch) )
				{
					if(argtoparam)
					{
						if(argv[param][i]==':')
						{	
							i++;
							j=0;
							while( argv[param][i] )
							{
								argtoparam[j]=argv[param][i];
								i++;
								j++;
							}
							argtoparam[j]=0;
							return 1;
						}
						else
						{
							return -1;
						}
					}
					else
					{
						return 1;
					}
				}
			}
		}
		param++;
	}
	
	return 0;
}

void printhelp(char* argv[])
{
	printf("Options:\n");
	printf("  -help \t\t\t: This help\n");
	printf("  -license\t\t\t: Print the license\n");
	printf("  -verbose\t\t\t: Verbose mode\n");
	printf("  -modulelist\t\t\t: List modules in the libhxcfe [FORMAT]\n");
	printf("  -rawlist\t\t\t: Disk layout list [DISKLAYOUT]\n");
	printf("  -interfacelist\t\t: Floppy interfaces mode list [INTERFACE_MODE]\n");
	printf("  -finput:[filename]\t\t: Input file image \n");
	printf("  -conv:[FORMAT] \t\t: Convert the input file\n");
	printf("  -uselayout:[DISKLAYOUT] \t\t: Use the Layout [DISKLAYOUT]\n");
	printf("  -usb:[DRIVE] \t\t\t: start the usb floppy emulator\n");
	printf("  -infos\t\t\t: Print informations about the input file\n");
	printf("  -ifmode:[INTERFACE_MODE]\t: Select the floppy interface mode\n");
	printf("  -singlestep\t\t\t: Force the single step mode\n");
	printf("  -doublestep\t\t\t: Force the double step mode\n");
	printf("\n");
}

void printlibmodule(HXCFLOPPYEMULATOR* hxcfe)
{
	
	int i,j;
	int numberofloader;
	const char* ptr;

	printf("---------------------------------------------------------------------------\n");
	printf("-                   libhxcfe file type support list                       -\n");
	printf("---------------------------------------------------------------------------\n");
	printf("MODULE ID          ACCESS    DESCRIPTION                         Extension\n\n");
	
	i=0;
	numberofloader=hxcfe_numberOfLoader(hxcfe);
	while(i<numberofloader)
	{
		ptr=hxcfe_getLoaderName(hxcfe,i);
		printf("%s",ptr );
		for(j=0;j<(int)(20-strlen(ptr));j++) printf(" ");
		
		printf("(%c%c)",hxcfe_getLoaderAccess(hxcfe,i)&1?'R':' ',hxcfe_getLoaderAccess(hxcfe,i)&2?'W':' ');
		
		ptr=hxcfe_getLoaderDesc(hxcfe,i);
		printf(" :  %s",ptr);
		
		for(j=0;j<(int)(38-strlen(ptr));j++) printf(" ");
		printf("(*.%s)\n",hxcfe_getLoaderExt(hxcfe,i));
		i++;
	}
	
	printf("\n%d Loaders\n\n",hxcfe_numberOfLoader(hxcfe));

}


XmlFloppyBuilder* hxcfe_initXmlFloppy(HXCFLOPPYEMULATOR* floppycontext);
void hxcfe_deinitXmlFloppy(XmlFloppyBuilder* context);

int         hxcfe_numberOfXmlLayout(XmlFloppyBuilder* context);
int         hxcfe_getXmlLayoutID(XmlFloppyBuilder* context,char * container);
const char* hxcfe_getXmlLayoutDesc(XmlFloppyBuilder* context,int moduleID);
const char* hxcfe_getXmlLayoutName(XmlFloppyBuilder* context,int moduleID);

int         hxcfe_selectXmlFloppyLayout(XmlFloppyBuilder* context,int layoutid);

FLOPPY*     hxcfe_generateXmlFloppy (XmlFloppyBuilder* context,unsigned char * rambuffer,unsigned buffersize);
FLOPPY*     hxcfe_generateXmlFileFloppy (XmlFloppyBuilder* context,char *file);


void printdisklayout(HXCFLOPPYEMULATOR* hxcfe)
{
	
	int i,j;
	int numberoflayout;
	XmlFloppyBuilder* xfb;
	const char* ptr;

	xfb = hxcfe_initXmlFloppy(hxcfe);

	printf("---------------------------------------------------------------------------\n");
	printf("-                     libhxcfe Raw Disk Layout list                       -\n");
	printf("---------------------------------------------------------------------------\n");
	printf("\n");
	
	i=0;
	numberoflayout = hxcfe_numberOfXmlLayout(xfb);
	while(i<numberoflayout)
	{
		ptr=hxcfe_getXmlLayoutName(xfb,i);
		printf("%s",ptr );
		for(j=0;j<(int)(20-strlen(ptr));j++) printf(" ");
				
		ptr=hxcfe_getXmlLayoutDesc(xfb,i);
		printf(" :  %s",ptr);
		
		for(j=0;j<(int)(38-strlen(ptr));j++) printf(" ");
		printf("\n");
		i++;
	}
	
	printf("\n%d Layout\n\n",hxcfe_numberOfXmlLayout(xfb));

	hxcfe_deinitXmlFloppy(xfb);

}


void printinterfacemode(HXCFLOPPYEMULATOR* hxcfe)
{
	
	int i,j;
	int numberofifmode;
	const char* ptr;

	printf("---------------------------------------------------------------------------\n");
	printf("-                        Interface mode list                              -\n");
	printf("---------------------------------------------------------------------------\n");
	printf("Interface ID                  (code)   DESCRIPTION                         \n\n");

	i=0;
	numberofifmode=0;
	while(hxcfe_getFloppyInterfaceModeName(hxcfe,i))
	{
		ptr=hxcfe_getFloppyInterfaceModeName(hxcfe,i);
		printf("%s",ptr );
		for(j=0;j<(int)(30-strlen(ptr));j++) printf(" ");
		
		printf("(0x%.2X)",i);
		
		ptr=hxcfe_getFloppyInterfaceModeDesc(hxcfe,i);
		printf(" : %s\n",ptr);
		i++;
	}
	
	printf("\n%d Modes\n\n",i);

}

int convertfile(HXCFLOPPYEMULATOR* hxcfe,char * infile,char * outfile,char * outformat,int ifmode)
{
	int loaderid;
	int ret;
	FLOPPY * floppydisk;

	loaderid=hxcfe_autoSelectLoader(hxcfe,infile,0);
	if(loaderid>=0)
	{
		floppydisk=hxcfe_floppyLoad(hxcfe,infile,loaderid,&ret);
				
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
			if(ifmode<0)
			{
				ifmode=hxcfe_floppyGetInterfaceMode(hxcfe,floppydisk);
			}

			loaderid=hxcfe_getLoaderID(hxcfe,outformat);
			if(loaderid>=0)
			{
				hxcfe_floppySetInterfaceMode(hxcfe,floppydisk,ifmode);
				hxcfe_floppyExport(hxcfe,floppydisk,outfile,loaderid);
			}
			else
			{
				printf("Cannot Find the Loader %s ! Please use the -modulelist option to see possible values.\n",outformat);
			}
		
			hxcfe_floppyUnload(hxcfe,floppydisk);
		}
	}

	return 0;
}

int convertrawfile(HXCFLOPPYEMULATOR* hxcfe,char * infile,char * layout,char * outfile,char * outformat,int ifmode)
{
	int layoutid,loaderid;
	FLOPPY * floppydisk;
	XmlFloppyBuilder* rfb;

	rfb=hxcfe_initXmlFloppy(hxcfe);
		
	layoutid = hxcfe_getXmlLayoutID(rfb,layout);

	if(layoutid>=0)
	{

		hxcfe_selectXmlFloppyLayout(rfb,layoutid);
		
		if(strlen(infile))
			floppydisk = hxcfe_generateXmlFileFloppy(rfb,infile);
		else
			floppydisk = hxcfe_generateXmlFloppy(rfb,0,0);

		hxcfe_deinitXmlFloppy(rfb);
				
		if(!floppydisk)
		{
			printf("Load error!\n");
		}
		else
		{
			if(ifmode<0)
			{
				ifmode=hxcfe_floppyGetInterfaceMode(hxcfe,floppydisk);
			}

			loaderid=hxcfe_getLoaderID(hxcfe,outformat);
			if(loaderid>=0)
			{
				hxcfe_floppySetInterfaceMode(hxcfe,floppydisk,ifmode);
				hxcfe_floppyExport(hxcfe,floppydisk,outfile,loaderid);
			}
			else
			{
				printf("Cannot Find the Loader %s ! Please use the -modulelist option to see possible values.\n",outformat);
			}
		
			hxcfe_floppyUnload(hxcfe,floppydisk);
		}
	}
	else
	{
		printf("Layout unknown !: %s\n",layout);
		printf("Please use the option -rawlist for the layout list.\n");
	}

	return 0;
}

int usbload(HXCFLOPPYEMULATOR* hxcfe,char * infile,int drive,int doublestep,int ifmode)
{   
	int loaderid;
	int ret;
	FLOPPY * floppydisk;
	USBHXCFE * usbfe;

	printf("Starting usb emulation - %s\n",infile);

	usbfe=libusbhxcfe_init(hxcfe);
	if(usbfe)
	{			
		loaderid=hxcfe_autoSelectLoader(hxcfe,infile,0);
		if(loaderid>=0)
		{
			floppydisk=hxcfe_floppyLoad(hxcfe,infile,loaderid,&ret);
				
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
				if(ifmode<0)
				{
					ifmode=hxcfe_floppyGetInterfaceMode(hxcfe,floppydisk);
				}

				if(doublestep<0)
				{
					doublestep=hxcfe_floppyGetDoubleStep(hxcfe,floppydisk);
				}

				drive=drive&3;
				libusbhxcfe_setInterfaceMode(hxcfe,usbfe,ifmode,doublestep,drive);
				libusbhxcfe_loadFloppy(hxcfe,usbfe,floppydisk);

				printf("Interface mode : %s\n",hxcfe_getFloppyInterfaceModeName(hxcfe,ifmode));
				printf("Select line : %d\n",drive);
				printf("Double Step : %s\n",doublestep?"yes":"no");

				printf("type q and enter to quit\n");
				do
				{
				}while(getchar()!='q');

				libusbhxcfe_ejectFloppy(hxcfe,usbfe);
					
				libusbhxcfe_deInit(hxcfe,usbfe);
				hxcfe_floppyUnload(hxcfe,floppydisk);
			}
		}
	}
	return 0;
}


int infofile(HXCFLOPPYEMULATOR* hxcfe,char * infile)
{
	int loaderid;
	int ret;
	FLOPPY * floppydisk;
	int ifmode,nbofsector;

	printf("---------------------------------------------------------------------------\n");
	printf("-                        File informations                                -\n");
	printf("---------------------------------------------------------------------------\n");

	printf("File: %s\n",infile);

	loaderid=hxcfe_autoSelectLoader(hxcfe,infile,0);
	if(loaderid>=0)
	{
		floppydisk=hxcfe_floppyLoad(hxcfe,infile,loaderid,&ret);
				
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
			ifmode=hxcfe_floppyGetInterfaceMode(hxcfe,floppydisk);
			printf("\n");
			printf("File type : %s - %s\n",hxcfe_getLoaderName(hxcfe,loaderid),hxcfe_getLoaderDesc(hxcfe,loaderid));
			printf("Floppy interface mode : %s\n",hxcfe_getFloppyInterfaceModeName(hxcfe,ifmode),hxcfe_getFloppyInterfaceModeDesc(hxcfe,ifmode));
			printf("Number of Track : %d\n",hxcfe_getNumberOfTrack(hxcfe,floppydisk) );
			printf("Number of Side : %d\n",hxcfe_getNumberOfSide(hxcfe,floppydisk) );
			printf("Total Size : %d Bytes, ",hxcfe_getFloppySize (hxcfe,floppydisk,&nbofsector)); 
			printf("Number of sectors : %d",nbofsector); 

			ifmode=hxcfe_floppyGetInterfaceMode(hxcfe,floppydisk);

			//loaderid=hxcfe_getLoaderID(hxcfe,outformat);

			hxcfe_floppyUnload(hxcfe,floppydisk);

			printf("\n");
		}
	}

	return 0;
}


int main(int argc, char* argv[])
{
	char filename[512];
	char ofilename[512];
	char outputformat[512];
	char layoutformat[128];
	char temp[512];
	int loaderid;
	int interfacemode;
	int doublestep;

	HXCFLOPPYEMULATOR* hxcfe;

	verbose=0;
	doublestep=-1;

	hxcfe=hxcfe_init();
	hxcfe_setOutputFunc(hxcfe,&CUI_affiche);

	printf("HxC Floppy Emulator : Floppy image file converter\n");
	printf("Copyright (C) 2006-2012 Jean-Francois DEL NERO\n");
   	printf("This program comes with ABSOLUTELY NO WARRANTY\n");
   	printf("This is free software, and you are welcome to redistribute it\n");
   	printf("under certain conditions;\n\n");
	
	printf("libhxcfe version : %s\n\n",hxcfe_getVersion(hxcfe));
	
	// License print...
	if(isOption(argc,argv,"license",0)>0)
	{
		printf("License :\n%s\n\n",hxcfe_getLicense(hxcfe));
	}

	// Verbose option...
	if(isOption(argc,argv,"verbose",0)>0)
	{
		printf("verbose mode\n");
		verbose=1;
	}

	// help option...
	if(isOption(argc,argv,"help",0)>0)
	{
		printhelp(argv);
	}

	memset(filename,0,sizeof(filename));

	// Input file name option
	if(isOption(argc,argv,"finput",(char*)&filename)>0)
	{
		printf("Input file : %s\n",filename);
	}

	// Output file name option
	memset(ofilename,0,512);
	isOption(argc,argv,"foutput",(char*)&ofilename);

	// Module list option
	if(isOption(argc,argv,"modulelist",0)>0)
	{
		printlibmodule(hxcfe);
	}

	// Interface mode list option
	if(isOption(argc,argv,"interfacelist",0)>0)
	{
		printinterfacemode(hxcfe);
	}

	// Interface mode list option
	if(isOption(argc,argv,"rawlist",0)>0)
	{
		printdisklayout(hxcfe);
	}

	// Interface mode option
	interfacemode=-1;
	if(isOption(argc,argv,"ifmode",(char*)&temp)>0)
	{
		interfacemode=hxcfe_getFloppyInterfaceModeID(hxcfe,temp);
		if(interfacemode<0)
		{
			printf("Unknown Interface mode ! : %s\n",temp);
			printf("Please use the -interfacelist option for possible interface modes.\n\n");
			hxcfe_deinit(hxcfe);
			return -1;
		}
	}

	if(isOption(argc,argv,"infos",0)>0)
	{
		infofile(hxcfe,filename);
	}

	// Convert a file ?
	if(isOption(argc,argv,"conv",0)>0)
	{
		strcpy(outputformat,PLUGIN_HXC_HFE);
		isOption(argc,argv,"conv",(char*)&outputformat);

		loaderid=hxcfe_getLoaderID(hxcfe,outputformat);
		if(loaderid>=0)
		{
			if(!strlen(ofilename))
			{
				get_filename(filename,ofilename);
				strcat(ofilename,".");
				strcat(ofilename,hxcfe_getLoaderExt(hxcfe,loaderid));
			}

			printf("Output file : %s\n",ofilename);

		}

		if(isOption(argc,argv,"uselayout",(char*)&layoutformat)>0)
		{
			convertrawfile(hxcfe,filename,layoutformat,ofilename,outputformat,interfacemode);
		}
		else
		{
			convertfile(hxcfe,filename,ofilename,outputformat,interfacemode);
		}
	}

	if(isOption(argc,argv,"singlestep",0)>0)
	{
		doublestep=0;
	}

	if(isOption(argc,argv,"doublestep",0)>0)
	{
		doublestep=0xFF;
	}

	// Input file name option
	if(isOption(argc,argv,"usb",(char*)&temp)>0)
	{
		usbload(hxcfe,filename,temp[0]-'0',doublestep,interfacemode);
	}

	if( (isOption(argc,argv,"help",0)<=0) && 
		(isOption(argc,argv,"license",0)<=0) &&
		(isOption(argc,argv,"modulelist",0)<=0) &&
		(isOption(argc,argv,"interfacelist",0)<=0) &&
		(isOption(argc,argv,"conv",0)<=0) &&
		(isOption(argc,argv,"usb",0)<=0) &&
		(isOption(argc,argv,"rawlist",0)<=0) &&
		(isOption(argc,argv,"infos",0)<=0 )
		)
	{
		printhelp(argv);
	}

	hxcfe_deinit(hxcfe);

	return 0;
}


