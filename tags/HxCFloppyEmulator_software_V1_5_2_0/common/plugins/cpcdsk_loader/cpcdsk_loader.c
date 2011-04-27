/*
//
// Copyright (C) 2006, 2007, 2008, 2009 Jean-François DEL NERO
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
// File : CPCDSK_DiskFile.c
// Contains: CPCDSK floppy image loader and plugins interfaces
//
// Written by:	DEL NERO Jean Francois
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "types.h"
#include "hxc_floppy_emulator.h"
#include "internal_floppy.h"
#include "floppy_loader.h"
#include "floppy_utils.h"

#include "../common/crc.h"
#include "../common/iso_ibm_track.h"

#include "cpcdsk_loader.h"
#include "cpcdsk_format.h"

#include "../common/os_api.h"

int CPCDSK_libIsValidDiskFile(HXCFLOPPYEMULATOR* floppycontext,char * imgfile)
{
	int pathlen;
	char * filepath;
	cpcdsk_fileheader fileheader;
	FILE * f;
	floppycontext->hxc_printf(MSG_DEBUG,"CPCDSK_libIsValidDiskFile %s",imgfile);
	if(imgfile)
	{
		pathlen=strlen(imgfile);
		if(pathlen!=0)
		{
			filepath=malloc(pathlen+1);
			if(filepath!=0)
			{
				sprintf(filepath,"%s",imgfile);
				strlower(filepath);
				
				if(strstr( filepath,".dsk" )!=NULL)
				{
					f=fopen(imgfile,"rb");
					if(f==NULL) 
					{
						free(filepath);
						floppycontext->hxc_printf(MSG_ERROR,"Cannot open the file !");
						return LOADER_ACCESSERROR;
					}
					fread(&fileheader,sizeof(fileheader),1,f);
					fclose(f);
					
					free(filepath);
					fileheader.headertag[34]=0;
					if( !strncmp(fileheader.headertag,"EXTENDED CPC DSK File\r\nDisk-Info\r\n",16) ||
						!strncmp(fileheader.headertag,"MV - CPCEMU Disk-File\r\nDisk-Info\r\n",11) ||
						!strncmp(fileheader.headertag,"MV - CPC",8) 
						)
					{
						floppycontext->hxc_printf(MSG_DEBUG,"CPC Dsk file !");
						return LOADER_ISVALID;
					}
					else
					{
						floppycontext->hxc_printf(MSG_DEBUG,"non CPC Dsk file !(bad header)");
						return LOADER_BADFILE;
					}
					
				}
				else
				{
					floppycontext->hxc_printf(MSG_DEBUG,"non CPC Dsk file !");
					free(filepath);
					return LOADER_BADFILE;
				}
			}
		}
	}
	
	return LOADER_BADPARAMETER;
}



int CPCDSK_libLoad_DiskFile(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	
	FILE * f;
	unsigned int filesize;
	int tracksize,interleave;
	unsigned int j,t,s,i;
	unsigned int sectorposition;
	int tracklen;
	int gap3len,rpm;
	int extendformat;
	cpcdsk_fileheader fileheader;
	cpcdsk_trackheader trackheader;
	cpcdsk_sector sector;
	unsigned char * tracksizetab;
	unsigned int trackposition;
	SECTORCONFIG* sectorconfig;
	unsigned char* trackdatatemp;
	int trackdatatempsize;
	CYLINDER** realloctracktable;
	
	CYLINDER* currentcylinder;
	SIDE* currentside;
	
	floppycontext->hxc_printf(MSG_DEBUG,"CPCDSK_libLoad_DiskFile %s",imgfile);
	
	f=fopen(imgfile,"rb");
	if(f==NULL) 
	{
		floppycontext->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return LOADER_ACCESSERROR;
	}
	
	fseek (f , 0 , SEEK_END); 
	filesize=ftell(f);
	fseek (f , 0 , SEEK_SET); 
	
	if(filesize!=0)
	{
		
		fread(&fileheader,sizeof(fileheader),1,f);
		
		if( !strncmp(fileheader.headertag,"EXTENDED CPC DSK File\r\nDisk-Info\r\n",16))
		{
			extendformat=1;
			floppycontext->hxc_printf(MSG_INFO_1,"Extended CPC Dsk file\n");
		}
		else
		{
			if(	!strncmp(fileheader.headertag,"MV - CPC",8))
			{
				extendformat=0;
				floppycontext->hxc_printf(MSG_INFO_1,"CPC Dsk standard file\n");
			}
			else
			{
				floppycontext->hxc_printf(MSG_ERROR,"non CPC Dsk : Bad header!\n");
				fclose(f);
				return LOADER_BADFILE;
			}
		}
		tracksizetab=0;
		
		if(extendformat)
		{
			// read the tracks size array.
			tracksizetab=(unsigned char *)malloc(fileheader.number_of_sides*fileheader.number_of_tracks);
			fread(tracksizetab,fileheader.number_of_sides*fileheader.number_of_tracks,1,f);
		}
		
		floppydisk->floppyBitRate=250000;
		floppydisk->floppyiftype=CPC_DD_FLOPPYMODE;
		floppydisk->floppyNumberOfTrack=fileheader.number_of_tracks;
		floppydisk->floppyNumberOfSide=fileheader.number_of_sides;
		floppydisk->floppySectorPerTrack=9; // default value
		tracksize=fileheader.size_of_a_track;
		rpm=300;
		
		sectorconfig=0;
		interleave=1;

		floppydisk->tracks=(CYLINDER**)malloc(sizeof(CYLINDER*)*floppydisk->floppyNumberOfTrack);
		memset(floppydisk->tracks,0,sizeof(CYLINDER*)*floppydisk->floppyNumberOfTrack);
		
		floppycontext->hxc_printf(MSG_INFO_1,"%d tracks, %d Side(s)\n",floppydisk->floppyNumberOfTrack,floppydisk->floppyNumberOfSide);
		trackposition=0x100;
		for(i=0;i<(unsigned int)(fileheader.number_of_sides*fileheader.number_of_tracks);i++)
		{
			fseek (f , trackposition , SEEK_SET); 
			fread(&trackheader,sizeof(trackheader),1,f);
			
			if(!strncmp(trackheader.headertag,"Track-Info\r\n",10))//12))
			{	
				
				t=trackheader.track_number;
				s=trackheader.side_number;
				
				if(extendformat)
				{
					tracksize=tracksizetab[i]*256;
				}
				
				if(tracksize)
				{
					// description d'une track au delà du nombre de track max !
					// realloc necessaire
					if(trackheader.track_number>=floppydisk->floppyNumberOfTrack)
					{
						floppydisk->floppyNumberOfTrack=trackheader.track_number+1;
						realloctracktable=(CYLINDER**)malloc(sizeof(CYLINDER*)*trackheader.track_number);
						memset(realloctracktable,0,sizeof(CYLINDER*)*trackheader.track_number);
						memcpy(realloctracktable,floppydisk->tracks,sizeof(CYLINDER*)*floppydisk->floppyNumberOfTrack);
						free(floppydisk->tracks);
						floppydisk->tracks=realloctracktable;
					}
					
					
					if(!floppydisk->tracks[t])
					{
						floppydisk->tracks[t]=(CYLINDER*)malloc(sizeof(CYLINDER));
						currentcylinder=floppydisk->tracks[t];
						currentcylinder->number_of_side=floppydisk->floppyNumberOfSide;
						currentcylinder->sides=(SIDE**)malloc(sizeof(SIDE*)*currentcylinder->number_of_side);
						memset(currentcylinder->sides,0,sizeof(SIDE*)*currentcylinder->number_of_side);
					}
					

					floppycontext->hxc_printf(MSG_DEBUG,"\nn°%d - track:%d side:%d sector:%d sc:%d gap3:%d fill:%x recmode:%d bitrate:%d\n ",i,trackheader.track_number,trackheader.side_number,trackheader.number_of_sector,trackheader.sector_size_code,trackheader.gap3_length,trackheader.filler_byte,trackheader.rec_mode,trackheader.datarate);

					if(trackheader.number_of_sector)
					{
						sectorconfig=(SECTORCONFIG*)malloc(sizeof(SECTORCONFIG)*trackheader.number_of_sector);
						memset(sectorconfig,0,sizeof(SECTORCONFIG)*trackheader.number_of_sector);
						
					
						gap3len=trackheader.gap3_length;
						sectorconfig->sectorsize=trackheader.sector_size_code;
					
					
						trackdatatemp=0;
						trackdatatempsize=0;
						sectorposition=0;
						for(j=0;j<trackheader.number_of_sector;j++)
						{
							fseek (f,trackposition+sizeof(trackheader)+(sizeof(sector)*j), SEEK_SET); 
							fread(&sector,sizeof(sector),1,f);
							
							sectorconfig[j].cylinder=sector.track;
							sectorconfig[j].head=sector.side;
							sectorconfig[j].sector=sector.sector_id;
							sectorconfig[j].sectorsize=sector.sector_size_code;
							switch(sectorconfig[j].sectorsize)
							{
							case 00:
								sectorconfig[j].sectorsize=128;
								break;
							case 01:
								sectorconfig[j].sectorsize=256;
								break;
							case 02:
								sectorconfig[j].sectorsize=512;
								break;
							case 03:
								sectorconfig[j].sectorsize=1024;
								break;
							case 04:
								sectorconfig[j].sectorsize=2048;
								break;
							case 05:
								sectorconfig[j].sectorsize=4096;
								break;
							case 06:
								sectorconfig[j].sectorsize=8192;
								break;
							case 07:
								sectorconfig[j].sectorsize=16384;
								break;
							default:
								sectorconfig[j].sectorsize=512;
								break;
							}
							trackdatatempsize=trackdatatempsize+sectorconfig[j].sectorsize;
							trackdatatemp=realloc(trackdatatemp,trackdatatempsize);
							
							fseek (f , trackposition+0x100+sectorposition/*sizeof(trackheader)+(sizeof(sector)*j)*/, SEEK_SET); 
							fread(trackdatatemp+trackdatatempsize-sectorconfig[j].sectorsize,sectorconfig[j].sectorsize,1,f);
							
							sectorposition=sectorposition+sectorconfig[j].sectorsize;
							
							if(sector.fdc_status_reg1&0x20 && !sector.fdc_status_reg2&0x20)
							{
								sectorconfig[j].use_alternate_header_crc=0x1;
							}
							
							if(sector.fdc_status_reg2&0x20)
							{
								sectorconfig[j].use_alternate_data_crc=0x1;
							}
							
							if(sector.fdc_status_reg2&0x40)
							{
								sectorconfig[j].use_alternate_datamark=1;
								sectorconfig[j].alternate_datamark=0xF8;
							}
							
							floppycontext->hxc_printf(MSG_DEBUG,"%d:%d track id:%d side id:%d sector id %d sector size:%d bad crc:%d sreg1:%x sreg2:%x",trackheader.track_number,trackheader.side_number,sector.track,sector.side,sector.sector_id,sectorconfig[j].sectorsize,sectorconfig[j].use_alternate_data_crc,sector.fdc_status_reg1,sector.fdc_status_reg2);
						}
					}
					
					//
					currentcylinder->floppyRPM=rpm;
					currentcylinder->sides[s]=malloc(sizeof(SIDE));
					currentside=currentcylinder->sides[s];
					memset(currentcylinder->sides[s],0,sizeof(SIDE));
					
					tracklen=(DEFAULT_DD_BITRATE/(rpm/60))/4;
					currentside->number_of_sector=trackheader.number_of_sector;
					currentside->tracklen=tracklen;
					
					currentside->databuffer=malloc(tracklen);
					memset(currentside->databuffer,0,tracklen);
					
					currentside->flakybitsbuffer=0;
					
					currentside->timingbuffer=0;
					currentside->bitrate=DEFAULT_DD_BITRATE;
					currentside->track_encoding=ISOIBM_MFM_ENCODING;

					currentside->indexbuffer=malloc(tracklen);
					memset(currentside->indexbuffer,0,tracklen);
					

					if(trackheader.number_of_sector)
					{

						while((gap3len!=0) && ((int)(currentside->tracklen/2)<ISOIBMGetTrackSize(IBMFORMAT_DD,trackheader.number_of_sector,512,gap3len,sectorconfig)))
						{
							gap3len--;
						};
						
						
						if(!gap3len)
						{
							gap3len=10;
							
							free(currentside->databuffer);
							free(currentside->indexbuffer);					
							
							tracklen=ISOIBMGetTrackSize(IBMFORMAT_DD,trackheader.number_of_sector,512,gap3len,sectorconfig)*2;
							
							currentside->tracklen=tracklen;
							currentside->databuffer=malloc(tracklen);
							memset(currentside->databuffer,0,tracklen);
							
							currentside->bitrate=DEFAULT_DD_BITRATE;

							currentside->flakybitsbuffer=0;
							
							currentside->indexbuffer=malloc(tracklen);
							memset(currentside->indexbuffer,0,tracklen);

							
						}
						
						BuildISOTrack(floppycontext,
							IBMFORMAT_DD,
							currentside->number_of_sector,
							1,
							512,
							trackheader.track_number,
							trackheader.side_number,
							gap3len,
							trackdatatemp,
							currentside->databuffer,
							&(currentside->tracklen),
							interleave,
							0,
							sectorconfig);

						free(sectorconfig);
					}
					
					///////////////////////	
					/*	for(j=0;j<trackheader.number_of_sector;j++)
					{
					
					  if(sectorconfig[j].baddatacrc)
					  {
					  
						if(!currentside->flakybitsbuffer)
						currentside->flakybitsbuffer=malloc(currentside->tracklen);
						
						  for(k=0;k<16;k++)
						  {
						  currentside->databuffer[sectorconfig[j].startdataindex+64+k]=0;
						  currentside->flakybitsbuffer[sectorconfig[j].startdataindex+64+k]=0xFF;
						  }
						  
							
						}
					}*/
					///////////////////////////
					free(trackdatatemp);
					trackdatatemp=0;
					
					
					trackposition=trackposition+tracksize;
				}
				else
				{
					
					floppycontext->hxc_printf(MSG_DEBUG,"\nn°%d - empty !\n ",i);

					if(!floppydisk->tracks[t])
					{
						floppydisk->tracks[t]=(CYLINDER*)malloc(sizeof(CYLINDER));
						currentcylinder=floppydisk->tracks[t];
						currentcylinder->number_of_side=floppydisk->floppyNumberOfSide;
						currentcylinder->sides=(SIDE**)malloc(sizeof(SIDE*)*currentcylinder->number_of_side);
						memset(currentcylinder->sides,0,sizeof(SIDE*)*currentcylinder->number_of_side);
					}

					currentcylinder->floppyRPM=rpm;
					currentcylinder->sides[s]=malloc(sizeof(SIDE));
					currentside=currentcylinder->sides[s];
					memset(currentcylinder->sides[s],0,sizeof(SIDE));
					
					tracklen=(DEFAULT_DD_BITRATE/(rpm/60))/4;
					currentside->bitrate=DEFAULT_DD_BITRATE;
					currentside->track_encoding=ISOIBM_MFM_ENCODING;

					currentside->number_of_sector=0;
					currentside->tracklen=tracklen;
					
					currentside->databuffer=malloc(tracklen);
					memset(currentside->databuffer,0,tracklen);
					
					currentside->flakybitsbuffer=0;
					
					currentside->indexbuffer=malloc(tracklen);
					memset(currentside->indexbuffer,0,tracklen);
					
					currentside->timingbuffer=0;
					currentside->bitrate=DEFAULT_DD_BITRATE;

				}
				
				
			}
			else
			{

				floppycontext->hxc_printf(MSG_ERROR,"bad track header !\n");
			}
			
		}
		
		
		// initialisation des tracks non formatées / non présente dans le fichier.
		for(j=0;j<floppydisk->floppyNumberOfTrack;j++)
		{
			for(i=0;i<floppydisk->floppyNumberOfSide;i++)
			{
				
				
				if(!floppydisk->tracks[j])
				{
					floppydisk->tracks[j]=(CYLINDER*)malloc(sizeof(CYLINDER));
					currentcylinder=floppydisk->tracks[j];
					currentcylinder->number_of_side=floppydisk->floppyNumberOfSide;
					currentcylinder->sides=(SIDE**)malloc(sizeof(SIDE*)*floppydisk->tracks[j]->number_of_side);
					memset(currentcylinder->sides,0,sizeof(SIDE*)*currentcylinder->number_of_side);
				}
				
				if(!floppydisk->tracks[j]->sides[i])
				{
					
					floppydisk->tracks[j]->sides[i]=malloc(sizeof(SIDE));
					memset(floppydisk->tracks[j]->sides[i],0,sizeof(SIDE));
					currentside=floppydisk->tracks[j]->sides[i];
					
					tracklen=(DEFAULT_DD_BITRATE/(rpm/60))/4;
					currentside->number_of_sector=0;
					currentside->tracklen=tracklen;
					currentside->bitrate=DEFAULT_DD_BITRATE;
					
					currentside->databuffer=malloc(tracklen);
					currentside->flakybitsbuffer=malloc(tracklen);
					memset(currentside->databuffer,0,tracklen);
					memset(currentside->flakybitsbuffer,0xFF,tracklen);
					
					currentside->indexbuffer=malloc(tracklen);
					memset(currentside->indexbuffer,0,tracklen);
					
					currentside->timingbuffer=0;
					currentside->bitrate=DEFAULT_DD_BITRATE;
					
				}
			}
		}
		
		
		for(j=0;j<floppydisk->floppyNumberOfTrack;j++)
		{
			for(i=0;i<floppydisk->floppyNumberOfSide;i++)
			{		
				if(floppydisk->tracks[j])
				{
					if(floppydisk->tracks[j]->sides[i])
					{
						currentside=floppydisk->tracks[j]->sides[i];
						currentside->tracklen=currentside->tracklen*8;
						if(currentside->indexbuffer)
							fillindex(currentside->tracklen-1,currentside,2500,TRUE,1);
					}
				}					
			}
		}

		if(tracksizetab) free(tracksizetab);
		//if(sectorconfig) free(sectorconfig);
		
		fclose(f);
		return LOADER_NOERROR;
	}
	
	floppycontext->hxc_printf(MSG_ERROR,"file size=%d !?",filesize);
	fclose(f);
	return LOADER_BADFILE;
}

