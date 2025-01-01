/*
//
// Copyright (C) 2006-2025 Jean-Fran�ois DEL NERO
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
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>

#include "types.h"

#include "internal_libhxcfe.h"
#include "libhxcfe.h"

#include "floppy_loader.h"
#include "fat12.h"
#include "fatlib.h"

#include "libhxcadaptor.h"

int ScanFileAndAddToFAT(HXCFE* floppycontext,char * folder,char * file, unsigned char * fattable,unsigned char *entriestable,unsigned char *datatable,int parentcluster,FATCONFIG * fatconfig,int numberofcluster)
{
	void* hfindfile;
	filefoundinfo FindFileData;
	int bbool;
	FILE * ftemp;
	unsigned char * newentry;
	unsigned char * subnewentry;
	char tempstr[256];
	char * ptr;
	int lefttoread;
	int fatclusternb;
	char * fullpath;
	fat_directory_entry *entry;
	fat_directory_entry *subentry;
	int i,j;
	struct stat repstate;
	struct tm * ts;

	if(strlen(folder))
	{
		hfindfile = hxc_find_first_file(folder,file, &FindFileData);
		if(hfindfile)
		{
			bbool=1;
			while(hfindfile && bbool)
			{
				if(FindFileData.isdirectory)
				{
					if(strcmp(".",FindFileData.filename)!=0 && strcmp("..",FindFileData.filename)!=0)
					{
						newentry=findfreeentry(entriestable);
						entry=(fat_directory_entry *)newentry;

						memset(entry->DIR_Name,0x20,8+3);
						sprintf((char*)tempstr,"%s",FindFileData.filename);

						floppycontext->hxc_printf(MSG_INFO_1,"Adding directory %s",FindFileData.filename);

						hxc_strupper(tempstr);

						ptr = strchr(tempstr,'.');
						if(ptr)
						{
							memcpy(&entry->DIR_Name[8],ptr+1,strlen(ptr+1));
							*ptr = '\0';
						}

						memcpy(entry->DIR_Name,tempstr,strlen(tempstr));
						entry->DIR_Attr=entry->DIR_Attr|0x10;
						entry->DIR_FileSize=FindFileData.size;

						fatclusternb=findfreecluster(fattable,numberofcluster);
						if(fatclusternb==-1)
						{
							floppycontext->hxc_printf(MSG_ERROR,"Cannot add this directory ! : No more cluster free !");
							hxc_find_close(hfindfile);
							return 1;
						}
						memset(&datatable[(fatclusternb-2)*fatconfig->sectorsize*fatconfig->clustersize],0,fatconfig->sectorsize*fatconfig->clustersize);

						entry->DIR_FstClusLO=fatclusternb;
						//*( (unsigned short*) &newentry[0x1A])=fatclusternb;
						setclusterptr(fattable,fatclusternb,0xFFF);

						subnewentry=findfreeentry(&datatable[(fatclusternb-2)*fatconfig->sectorsize*fatconfig->clustersize]);
						subentry=(fat_directory_entry *)subnewentry;

						memset(subentry->DIR_Name,' ',sizeof(subentry->DIR_Name)); // "."
						subentry->DIR_Name[0] = '.';

						//memcpy(subnewentry,".          ",strlen(".          "));
						subentry->DIR_Attr=0x10;
						subentry->DIR_FstClusLO=fatclusternb;

						subnewentry=findfreeentry(&datatable[(fatclusternb-2)*fatconfig->sectorsize*fatconfig->clustersize]);
						subentry=(fat_directory_entry *)subnewentry;

						memset(subentry->DIR_Name,' ',sizeof(subentry->DIR_Name)); // ".."
						subentry->DIR_Name[0] = '.';
						subentry->DIR_Name[1] = '.';

						subentry->DIR_Attr=0x10;
						subentry->DIR_FstClusLO=parentcluster;
						//*( (unsigned short*) &subnewentry[0x1A])=parentcluster;

						floppycontext->hxc_printf(MSG_INFO_1,"Entering directory %s",FindFileData.filename);

						fullpath = malloc(strlen(FindFileData.filename)+strlen(folder)+2);
						sprintf(fullpath,"%s"DIR_SEPARATOR"%s",folder,FindFileData.filename);

						if(ScanFileAndAddToFAT(floppycontext,fullpath,file,fattable,&datatable[(fatclusternb-2)*fatconfig->sectorsize*fatconfig->clustersize],datatable,fatclusternb,fatconfig,numberofcluster))
						{
							free(fullpath);
							hxc_find_close(hfindfile);
							return 1;
						}
						free(fullpath);

						floppycontext->hxc_printf(MSG_INFO_1,"Leaving directory %s",FindFileData.filename);

					}
				}
				else
				{
					floppycontext->hxc_printf(MSG_INFO_1,"Adding file %s, %dB",FindFileData.filename,FindFileData.size);

					sprintf(tempstr,"%s",FindFileData.filename);
					hxc_strupper(tempstr);

					newentry=findfreeentry(entriestable);
					entry=(fat_directory_entry *)newentry;
					memset(entry->DIR_Name,0x20,8+3);

					i=0;
					while(tempstr[i]  && (i<8) && tempstr[i]!='.')
					{

						if(tempstr[i]==' ')
						{
							newentry[i]='_';
						}
						else
						{
							newentry[i]=tempstr[i];
						}
						i++;
					}

					ptr = strchr(tempstr,'.');
					if( ptr )
					{
						i=0;
						while(tempstr[i]!='.')
						{
							i++;
						}

						j=0;
						i++;
						while(tempstr[i] && (j<3))
						{
							if(tempstr[i]==' ')
							{
								newentry[8+j]='_';
							}
							else
							{
								newentry[8+j]=tempstr[i];
							}
							i++;
							j++;
						}

						memcpy(newentry+8,ptr+1,strlen(ptr+1));
						*ptr = '\0';
					}

					entry->DIR_FileSize=FindFileData.size;
					if(FindFileData.size)
					{
						lefttoread=FindFileData.size;
						fatclusternb=findfreecluster(fattable,numberofcluster);
						memset(&datatable[(fatclusternb-2)*fatconfig->sectorsize*fatconfig->clustersize],0,fatconfig->sectorsize*fatconfig->clustersize);

						if(fatclusternb==-1)
						{
							floppycontext->hxc_printf(MSG_ERROR,"Cannot add this file ! : No more cluster free !");
							hxc_find_close(hfindfile);
							return 1;
						}
						entry->DIR_FstClusLO=fatclusternb;

						if(file)
						{
							fullpath=malloc(strlen(FindFileData.filename)+strlen(folder)+2);
							sprintf(fullpath,"%s"DIR_SEPARATOR"%s",folder,FindFileData.filename);
						}
						else
						{
							fullpath=malloc(strlen(folder)+1);
							sprintf(fullpath,"%s",folder);
						}

						hxc_stat(fullpath,&repstate);
						ts=localtime(&repstate.st_ctime);
						if(ts)
						{
							entry->DIR_CrtDate=  (((ts->tm_year-80) & 0x7F)<<9) | (((ts->tm_mon+1) & 0xF)<<5) | (ts->tm_mday & 0x1F);
							entry->DIR_CrtTime= ((ts->tm_hour & 0x1F)<<11) | ((ts->tm_min & 0x3F)<<5) | ((ts->tm_sec/2) & 0x1F);
						}
						else
						{
							entry->DIR_CrtDate= 0;
							entry->DIR_CrtTime= 0;
						}

						hxc_stat(fullpath,&repstate);
						ts=localtime(&repstate.st_mtime);
						if(ts)
						{
							entry->DIR_WrtDate=  (((ts->tm_year-80) & 0x7F)<<9) | (((ts->tm_mon+1) & 0xF)<<5) | (ts->tm_mday & 0x1F);
							entry->DIR_WrtTime= ((ts->tm_hour & 0x1F)<<11) | ((ts->tm_min  & 0x3F)<<5) | ((ts->tm_sec/2) & 0x1F);
						}
						else
						{
							entry->DIR_WrtDate= 0;
							entry->DIR_WrtTime= 0;
						}

						ftemp=hxc_fopen(fullpath,"rb");
						if(ftemp)
						{
							do
							{
								fatclusternb=findfreecluster(fattable,numberofcluster);
								if(fatclusternb==-1)
								{
									floppycontext->hxc_printf(MSG_ERROR,"Error while adding this file ! : No more cluster free !");
									free(fullpath);
									hxc_find_close(hfindfile);
									hxc_fclose(ftemp);
									return 1;
								}
								memset(&datatable[(fatclusternb-2)*fatconfig->sectorsize*fatconfig->clustersize],0x00,fatconfig->sectorsize*fatconfig->clustersize);
								hxc_fread(&datatable[(fatclusternb-2)*fatconfig->sectorsize*fatconfig->clustersize],fatconfig->sectorsize*fatconfig->clustersize,ftemp);
								setclusterptr(fattable,fatclusternb,0xFFF);
								if(lefttoread>(fatconfig->sectorsize*fatconfig->clustersize))
								{
									setclusterptr(fattable,fatclusternb,findfreecluster(fattable,numberofcluster));
								}
								lefttoread=lefttoread-(fatconfig->sectorsize*fatconfig->clustersize);
							}while(lefttoread>0);

							hxc_fclose(ftemp);
						}
						else
						{
							floppycontext->hxc_printf(MSG_ERROR,"Error while adding this file ! : Access error !");
						}
						free(fullpath);

					}
				}

				bbool=hxc_find_next_file(hfindfile,folder,file,&FindFileData);
			}

			hxc_find_close(hfindfile);
		}
		else
		{
			floppycontext->hxc_printf(MSG_ERROR,"Error FindFirstFile !");
		}
	}
	else
	{
		floppycontext->hxc_printf(MSG_INFO_0,"Empty Path -> Empty floppy");
	}

	return 0;
}

unsigned char * findfreeentry(unsigned char *entriestable)
{
	int i;

	i=0;
	while(entriestable[i]!=0)
	{
		i=i+32;
	}

	return &entriestable[i];
}

int findfreecluster(unsigned char *fattable,int nbofcluster)
{
	int i;
	int freecluster;

	i=24;
	do
	{
		freecluster=0;
		if(i&7)
		{
			if(!fattable[(i/8)+1] && !(fattable[i/8]&0xF0))
			{
				freecluster=1;
			}

		}
		else
		{

			if(!fattable[i/8] && !(fattable[(i/8)+1]&0x0F))
			{
				freecluster=1;
			}
		}

		i=i+12;
	}while(!freecluster && (((i-12)/12)<nbofcluster));

	if(((i-12)/12)<nbofcluster)
	{
		return (i-12)/12;
	}
	else
	{
		return -1;
	}

}

int setclusterptr(unsigned char *fattable,int index,int ptr)
{
	int i;

	i=index*12;

	if(i&7)
	{
		fattable[i/8]=(fattable[i/8]&0x0F)|((ptr&0xF)<<4);
		fattable[(i/8)+1]=((ptr>>4)&0xFF);
	}
	else
	{
		fattable[i/8]=(ptr&0xFF);
		fattable[(i/8)+1]=(fattable[(i/8)+1]&0xF0)|((ptr>>8)&0xF);
	}

	return ptr;
}



