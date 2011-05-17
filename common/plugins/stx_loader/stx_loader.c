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
// File : STX_DiskFile.c
// Contains: STX floppy image loader and plugins interfaces
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
#include "../common/track_generator.h"

#include "stx_loader.h"

#include "pasti_format.h"

#include "../common/os_api.h"

int STX_libIsValidDiskFile(HXCFLOPPYEMULATOR* floppycontext,char * imgfile)
{
	int pathlen;
	char * filepath;
	FILE * f;
	pasti_fileheader * fileheader;

	floppycontext->hxc_printf(MSG_DEBUG,"STX_libIsValidDiskFile %s",imgfile);
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

				if(strstr( filepath,".stx" )!=NULL)
				{

					f=fopen(imgfile,"rb");
					if(f==NULL) 
					{
						free(filepath);
						return LOADER_ACCESSERROR;
					}
				
					fileheader=(pasti_fileheader*)malloc(sizeof(pasti_fileheader));
					fread( fileheader, sizeof(pasti_fileheader), 1, f );    

					fclose(f);

					free(filepath);

					if(strcmp(fileheader->headertag,"RSY"))
					{
						free(fileheader);
						floppycontext->hxc_printf(MSG_DEBUG,"non STX file (bad header)!");
						return LOADER_BADFILE;

					}
					else
					{
						free(fileheader);
						floppycontext->hxc_printf(MSG_DEBUG,"STX file !");
						return LOADER_ISVALID;
					}
					
				}
				else
				{
					floppycontext->hxc_printf(MSG_DEBUG,"non STX file !");
					free(filepath);
					return LOADER_BADFILE;
				}
			}
		}
	}

	return LOADER_BADPARAMETER;
}


typedef struct sectorcfg_
{
int size;
unsigned int trackoffset;
int weakdata_offset;
}sectorcfg;


int STX_libLoad_DiskFile(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	
	FILE * f;
	unsigned int i,j,k,l,t,t2,debug_i,o;
	unsigned char trackformat;
	unsigned char interleave;
	unsigned short temp_val;
	unsigned short index_sync;
	unsigned char numberoftrackperside;
	unsigned int numberoftrack,tracknumber,sidenumber;
	unsigned int numberofside;
	unsigned int trackmode;
	pasti_fileheader * fileheader;
	pasti_trackheader trackheader;
	pasti_sector * sector;
	SECTORCONFIG* sectorconfig;
	int * real_sector_size_list;
	int * sectorheader_index;
	int * sectordata_index;
	unsigned char * temptrack;
	unsigned char * temptrack2;
	unsigned char * tempclock;
	int trackpos,trackheaderpos;
	int tracksize;
	char tempstring[512];
	int presenceside[2];
	int numberofweaksector;
	int lastindex;
	int weaksectortotalsize;
	int tracklen;
	unsigned char crctable[32];
	sectorcfg * sectorcfgs;
	unsigned char CRC16_High;
	unsigned char CRC16_Low;
	
	CYLINDER* currentcylinder;
	SIDE* currentside;
	
	floppycontext->hxc_printf(MSG_DEBUG,"STX_libLoad_DiskFile %s",imgfile);
	
	f=fopen(imgfile,"rb");
	if(f==NULL) 
	{
		floppycontext->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return LOADER_ACCESSERROR;
	}
	
	floppycontext->hxc_printf(MSG_DEBUG,"Image Info: %s\n",imgfile);
	
	fileheader=(pasti_fileheader*)malloc(sizeof(pasti_fileheader));
	fread( fileheader, sizeof(pasti_fileheader), 1, f ); 
	
	
	if(!strcmp(fileheader->headertag,"RSY"))
	{
		numberoftrack=fileheader->number_of_track;
		t=ftell(f);
		
		//comptage track / side 
		presenceside[0]=0;
		presenceside[1]=0;
		numberoftrackperside=0;
		for(i=0;i<numberoftrack;i++)
		{
			trackheaderpos=ftell(f);
			fread( &trackheader, sizeof(trackheader), 1, f ); 	
			
			presenceside[trackheader.track_code>>7]=1;
			if((trackheader.track_code&(unsigned char)0x7F)>numberoftrackperside)
			{
				numberoftrackperside=trackheader.track_code&0x7F;
			}
			fseek(f,trackheaderpos,SEEK_SET);
			
			fseek(f,trackheader.tracksize,SEEK_CUR);
		}
		
		numberoftrackperside++;
		
		fseek(f,t,SEEK_SET);
		
		if(presenceside[0] && presenceside[1])
		{
			numberofside=2;
		}
		else
		{
			numberofside=1;
		}
		
		floppydisk->floppySectorPerTrack=-1;
		floppydisk->floppyNumberOfSide=numberofside;
		floppydisk->floppyNumberOfTrack=numberoftrackperside;
		floppydisk->floppyiftype=ATARIST_DD_FLOPPYMODE;
		floppydisk->floppyBitRate=250000;
		
		//STXFloppylist[floppyid]->floppyRPM=300;
		//STXFloppylist[floppyid]->floppyBitRate=250000;
		
		
		trackformat=ISOFORMAT_DD;
		
		sprintf(tempstring,"File header :");
		for(debug_i=0;debug_i<sizeof(pasti_fileheader);debug_i++)
		{
			sprintf(&tempstring[strlen(tempstring)],"%.2X ",*(((unsigned char*)fileheader)+debug_i));
		}
		sprintf(&tempstring[strlen(tempstring)],"\n");
		
		floppycontext->hxc_printf(MSG_DEBUG,"%s",tempstring);
		floppycontext->hxc_printf(MSG_DEBUG,"Number of track : %d (%d), Number of side: %d\n",numberoftrack,numberoftrackperside,numberofside);
		floppycontext->hxc_printf(MSG_DEBUG,"Tracks :");
		
		if(floppydisk->floppyNumberOfTrack)
		{
			
			
			
			floppydisk->tracks=(CYLINDER**)malloc(sizeof(CYLINDER*)*floppydisk->floppyNumberOfTrack);
			memset(floppydisk->tracks,0,sizeof(CYLINDER*)*floppydisk->floppyNumberOfTrack);
			for(i=0;i<numberoftrack;i++)
			{
				//lecture descripteur track
				trackheaderpos=ftell(f);
				fread( &trackheader, sizeof(trackheader), 1, f ); 
				
				// debug //
				sprintf(tempstring,"\nn°%.3d ",i);
				sprintf(&tempstring[strlen(tempstring)],"T:%d S:%d Sectors:%d ",trackheader.track_code&0x7F,trackheader.track_code>>7,trackheader.numberofsector);
				sprintf(&tempstring[strlen(tempstring)],"Size:%d ",trackheader.tracksize);
				sprintf(&tempstring[strlen(tempstring)],"Track header:");
				for(debug_i=0;debug_i<sizeof(trackheader);debug_i++)
				{
					sprintf(&tempstring[strlen(tempstring)],"%.2X ",*(((unsigned char*)&trackheader)+debug_i));
				}
				floppycontext->hxc_printf(MSG_DEBUG,"%s",tempstring);
				//////////
				
				tracknumber=trackheader.track_code&0x7F;
				sidenumber=trackheader.track_code>>7;
				
				trackmode=0;
				if(trackheader.unknowvalue&0x40)
				{
					trackmode=1;
				}
				
				numberofweaksector=0;
				weaksectortotalsize=0;
				tracksize=0;
				
				switch(trackmode)
				{
					//track contenant uniquement les informations secteurs
					// -> encodage "standard"
				case 0:
					floppycontext->hxc_printf(MSG_DEBUG,"READ SECTOR track");
					if(trackheader.numberofsector)
					{
						sector=malloc(sizeof(pasti_sector)*trackheader.numberofsector);
						sectorconfig=(SECTORCONFIG *) malloc(sizeof(SECTORCONFIG)* trackheader.numberofsector);
						memset(sectorconfig,0,sizeof(SECTORCONFIG)* trackheader.numberofsector);
						
						// lecture de l'ensemble des descripteurs de secteur
						for(j=0;j<(trackheader.numberofsector);j++)
						{
							fread( &sector[j], sizeof(pasti_sector), 1, f ); 
							
							sectorconfig[j].use_alternate_data_crc=0;
							
							sectorconfig[j].missingdataaddressmark=0;
							sectorconfig[j].use_alternate_datamark=0;
							sectorconfig[j].use_alternate_addressmark=0;
							sectorconfig[j].head=sector[j].side_num;
							sectorconfig[j].cylinder=sector[j].track_num;
							sectorconfig[j].sector=sector[j].sector_num;
							sectorconfig[j].use_alternate_header_crc=0x2;
							sectorconfig[j].header_crc=sector[j].header_crc;
							sectorconfig[j].gap3=255;
							sectorconfig[j].bitrate=floppydisk->floppyBitRate;
							sectorconfig[j].trackencoding=trackformat;
							
							
							sectorconfig[j].sectorsize=0;
							switch(sector[j].sector_size)
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
								//tempdata[j]=02;
								break;
							}
							tracksize=tracksize+sectorconfig[j].sectorsize;
							
							// debug //
							sprintf(tempstring,"Sector:%.2d Size:%.8d Cylcode:%.2d HeadCode:%d SectorCode:%.2d Size:%d",j,sectorconfig[j].sectorsize,sectorconfig[j].cylinder,sectorconfig[j].head,sectorconfig[j].sector,sectorconfig[j].sectorsize);
							sprintf(&tempstring[strlen(tempstring)]," Sector header:");
							for(debug_i=0;debug_i<sizeof(pasti_sector);debug_i++) 
							{	
								sprintf(&tempstring[strlen(tempstring)],"%.2X",*(((unsigned char*)&sector[j])+debug_i));
								if(debug_i==3 || debug_i==5 ||  debug_i==7 || debug_i==8 || debug_i==9 || debug_i==10 || debug_i==11 || debug_i==13 || debug_i==15)
								{
									sprintf(&tempstring[strlen(tempstring)]," ");
								}
							}
							floppycontext->hxc_printf(MSG_DEBUG,"%s",tempstring);
							//////////
							
							
							if((sector[j].FDC_status&0x88)==0x88)
							{
								numberofweaksector++;
								weaksectortotalsize=weaksectortotalsize+sectorconfig[j].sectorsize;
							}
							
							
						}
						
					}
					
					//position des donnees
					
					trackpos=ftell(f)+ weaksectortotalsize;
					fseek(f,trackpos,SEEK_SET);
					
					if(trackheader.unknowvalue&0x80)
					{
						fread( &temp_val, sizeof(unsigned short), 1, f ); 
						floppycontext->hxc_printf(MSG_DEBUG,"Unknow value %x",temp_val);
					}	
					
					// lecture des secteurs
					temptrack=(unsigned char*)malloc(tracksize);				
					o=0;
					for(j=0;j<(trackheader.numberofsector);j++)
					{
						fseek(f,trackpos,SEEK_SET);
						fseek(f,sector[j].sector_pos,SEEK_CUR);
						
						fread(&temptrack[o],sectorconfig[j].sectorsize,1,f);
						sectorconfig[j].input_data=&temptrack[o];
						
						o=o+sectorconfig[j].sectorsize;
					}
					
					interleave=1;
					
					// Allocation track
					
					if(!floppydisk->tracks[tracknumber])
					{
						floppydisk->tracks[tracknumber]=(CYLINDER*)malloc(sizeof(CYLINDER));
						currentcylinder=floppydisk->tracks[tracknumber];
						currentcylinder->number_of_side=0;
						
						currentcylinder->sides=(SIDE**)malloc(sizeof(SIDE*)*2);
						memset(currentcylinder->sides,0,sizeof(SIDE*)*2);
					}
					
					currentcylinder->number_of_side++;
					currentcylinder->sides[sidenumber]=tg_generatetrackEx(floppydisk->floppySectorPerTrack,(SECTORCONFIG *)&sectorconfig,interleave,0,floppydisk->floppyBitRate,300,trackformat,2500,-2500);
					currentside=currentcylinder->sides[sidenumber]; 
					
					currentside->flakybitsbuffer=malloc(currentside->tracklen/8);
					currentside->timingbuffer=malloc((currentside->tracklen/8)*sizeof(unsigned long));					
					memset(currentside->flakybitsbuffer,0,currentside->tracklen/8);
					
					// bitrate
					currentside->bitrate=VARIABLEBITRATE;
					for(k=0;k<currentside->tracklen;k++)
					{
						currentside->timingbuffer[k]=DEFAULT_DD_BITRATE;
					}
					
					// encodage track
					//
					trackformat=ISOFORMAT_DD;
					if(trackheader.numberofsector==11) trackformat=ISOFORMAT_DD11S;
					
					// generation flakey bits
					for(j=0;j<(trackheader.numberofsector);j++)
					{
						
						if((sector[j].FDC_status&0x88)==0x88)
						{
							for(k=0;k<16;k++)
							{
								
								currentside->databuffer[sectorconfig[j].startdataindex+64+k]=0;
								currentside->flakybitsbuffer[sectorconfig[j].startdataindex+64+k]=0xFF;
								//	currentside->timingbuffer[sectorconfig[j].startdataindex+64+k]=currentside->timingbuffer[sectorconfig[j].startdataindex+64+k]*2;
							}
						}
					}
					if(trackheader.numberofsector)
					{
						free(sectorconfig);
						free(sector);
					}
					
					
					free(temptrack);
					
					break;
					
					
					///////////////////////////////////////////////////////////////////////////////////////////////
					///////////////////////////////////////////////////////////////////////////////////////////////
					///////////////////////////////////////////////////////////////////////////////////////////////
					///////////////////////////////////track data+sectors datas////////////////////////////////////
					///////////////////////////////////////////////////////////////////////////////////////////////
				case 1:
					floppycontext->hxc_printf(MSG_DEBUG,"READTRACK track");
					
					
					// allocation buffers secteurs
					if(trackheader.numberofsector)
					{
						sector=malloc(sizeof(pasti_sector)*trackheader.numberofsector);
						sectorconfig=(SECTORCONFIG *) malloc(sizeof(SECTORCONFIG)* trackheader.numberofsector);
						real_sector_size_list=(int *) malloc(sizeof(int)* trackheader.numberofsector);
						//sectorheader_index=(int *) malloc(sizeof(int)* trackheader.numberofsector);
						sectordata_index=(int *) malloc(sizeof(int)* trackheader.numberofsector*2);
                        
						sectorcfgs=(sectorcfg*)malloc(sizeof(sectorcfg)* trackheader.numberofsector);
						memset(sectorcfgs,0,sizeof(sectorcfg)*trackheader.numberofsector);
					}
					else
					{
						sector=0;
						sectorconfig=0;
						real_sector_size_list=0;
						sectorheader_index=0;
						sectordata_index=0;
						sectorcfgs=0;
					}
					
					
					for(j=0;j<(unsigned int)(trackheader.numberofsector*2);j++)
					{
						sectordata_index[j]=-1;
					}
					
					// lecture de l'ensemble des descripteurs de secteur
					for(j=0;j<(trackheader.numberofsector);j++)
					{
						fread( &sector[j], sizeof(pasti_sector), 1, f ); 
					}
					
					// analyse configuration secteurs
					// calcul taille secteur
					for(j=0;j<(trackheader.numberofsector);j++)
					{
						sectorconfig[j].use_alternate_data_crc=0;
                        
						sectorconfig[j].missingdataaddressmark=0;
						sectorconfig[j].head=sector[j].side_num;
						sectorconfig[j].cylinder=sector[j].track_num;
						sectorconfig[j].sector=sector[j].sector_num;
						sectorconfig[j].use_alternate_header_crc=0x2;
						sectorconfig[j].header_crc=sector[j].header_crc;
						
						
						sectorconfig[j].sectorsize=0;
						switch(sector[j].sector_size)
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
							//tempdata[j]=02;
							break;
						}
						tracksize=tracksize+sectorconfig[j].sectorsize;
                        
						
						if(j+1<trackheader.numberofsector)
						{
							real_sector_size_list[j]=sectorconfig[j].sectorsize;//(sector[j+1].sector_pos)-(sector[j].sector_pos);
						}
						else
						{
							real_sector_size_list[j]=sectorconfig[j].sectorsize;//(trackheader.tracksize-((sizeof(pasti_sector)*trackheader.numberofsector)+sizeof(trackheader)))-(sector[j].sector_pos);
						}
						
						
						sectorcfgs[j].weakdata_offset=weaksectortotalsize;
						sectorcfgs[j].size=real_sector_size_list[j];
						
						if((sector[j].FDC_status&0x88)==0x88)
						{
							numberofweaksector++;
							
							weaksectortotalsize=weaksectortotalsize+sectorconfig[j].sectorsize;//real_sector_size_list[j];//sectorconfig[j].sectorsize;
						}
						
						
						// debug //
						sprintf(tempstring,"Sector:%.2d Size:%.8d Cylcode:%.2d HeadCode:%d SectorCode:%.2d Size:%d Real_Size:%d",j,sectorconfig[j].sectorsize,sectorconfig[j].cylinder,sectorconfig[j].head,sectorconfig[j].sector,sectorconfig[j].sectorsize,real_sector_size_list[j]);
						sprintf(&tempstring[strlen(tempstring)]," Sector header:");
						for(debug_i=0;debug_i<sizeof(pasti_sector);debug_i++) 
						{   
							sprintf(&tempstring[strlen(tempstring)],"%.2X",*(((unsigned char*)&sector[j])+debug_i));
							if(debug_i==3 || debug_i==5 ||  debug_i==7 || debug_i==8 || debug_i==9 || debug_i==10 || debug_i==11 || debug_i==13 || debug_i==15)
							{
								sprintf(&tempstring[strlen(tempstring)]," ");
							}
						}
						floppycontext->hxc_printf(MSG_DEBUG,"%s",tempstring);
						//////////
						
						
                        
					}
					
					trackpos=ftell(f)+ weaksectortotalsize;
					
					
					
					fseek(f,trackpos,SEEK_SET);
					
					if(trackheader.unknowvalue&0x80)
					{
						//trackpos=trackpos+2;
						fread( &index_sync, sizeof(unsigned short), 1, f ); 
						floppycontext->hxc_printf(MSG_DEBUG,"Index sync pos %x",temp_val);
					}	
					
					//trackpos=trackpos+2;
					fread( &temp_val, sizeof(unsigned short), 1, f ); 
					floppycontext->hxc_printf(MSG_DEBUG,"READTRACK datas size %x",temp_val);
					
					//trackpos=trackpos+temp_val;
					temptrack=(unsigned char*)malloc(temp_val);
					
					tracklen=temp_val;
					
					if(temptrack)
					{
						fread( temptrack, temp_val, 1, f ); 
						
						temptrack2=(unsigned char*)malloc(temp_val*2);
						tempclock=(unsigned char*)malloc(temp_val*2);
						memset(tempclock,0xFF,temp_val*2);
						memset(temptrack2,0,temp_val*2);
						
						t=0;
						t2=0;
						l=0;
						while( (l<(unsigned int)temp_val)  && (t<(unsigned int)(((trackheader.numberofsector*2)+1))) && (t2< ((unsigned int)(trackheader.numberofsector)+1)) )
						{
							if((temptrack[l]==0xA1) && (temptrack[l+1]==0xA1) && ((temptrack[l+2]==0xFB) || (temptrack[l+2]==0xFE)))
							{
								
								if( (temptrack[l+2]==0xFB))
									floppycontext->hxc_printf(MSG_DEBUG,"[%x] header",l);
								sectordata_index[t]=l-1;
								t++;
							}
							
							if((temptrack[l]==0xA1) && (temptrack[l+1]==0xA1) && (temptrack[l+2]==0xFE))
							{
								
								//sectorheader_index[t2]=l-1;
								
								if(((sector[t2].header_crc&0xff)==(temptrack[l+7])) && (((sector[t2].header_crc>>8)&0xff)==(temptrack[l+8])) )
								{
									floppycontext->hxc_printf(MSG_DEBUG,"[%x] crc %.2X%.2X ok !! ",l,temptrack[l+7],temptrack[l+8]);
								}
								else
								{
									floppycontext->hxc_printf(MSG_DEBUG,"[%x] crc %.2X%.2X ERROR !! (%.4x)",l,temptrack[l+7],temptrack[l+8],sector[t2].header_crc);
								}
								t2++;
							}
							l++;
							
						}
						
						floppycontext->hxc_printf(MSG_DEBUG,"%d <A1 A1 FB>   %d <A1 A1 FE>",t,t2);
						
						
						lastindex=0;
						memcpy(temptrack2,temptrack,temp_val);
						for(j=0;j<(unsigned int)(trackheader.numberofsector*2);j++)
						{
							floppycontext->hxc_printf(MSG_DEBUG,"----sectordata_index[%d]=%d lastindex=%d---",j,sectordata_index[j],lastindex);
							if(sectordata_index[j]!=-1)
							{
								
								//memcpy(&temptrack2[lastindex],&temptrack[lastindex],sectordata_index[j]-lastindex);
								lastindex=sectordata_index[j];
								
								if(temptrack[lastindex+3]==0xFE)
								{
									if(temptrack[lastindex]!=0xA1)
									{
										
										for(t=0;t<12;t++)
										{
											//	temptrack2[lastindex-1-t]=0;
										}
										temptrack2[lastindex]=0xA1;
										tempclock[lastindex]= 0x0A;
										lastindex++;
										temptrack2[lastindex]=0xA1;
										tempclock[lastindex]= 0x0A;
										lastindex++;
										temptrack2[lastindex]=0xA1;
										tempclock[lastindex]= 0x0A;
										lastindex++;
										
										temptrack2[lastindex]=0xFE;
										lastindex++;
										
										temptrack2[lastindex]=sectorconfig[j/2].cylinder;
										lastindex++;
										temptrack2[lastindex]=sectorconfig[j/2].head;
										lastindex++;
										temptrack2[lastindex]=sectorconfig[j/2].sector;
										lastindex++;
										temptrack2[lastindex]=sector[j/2].sector_size;
										lastindex++;
										temptrack2[lastindex]=sectorconfig[j/2].header_crc&0xff;
										lastindex++;
										temptrack2[lastindex]=(sectorconfig[j/2].header_crc>>8)&0xff;
										lastindex++;
										
									}
								}
								else
								{
									
									if(temptrack[lastindex+3]==0xFB)
									{
										if(temptrack[lastindex]!=0xA1)
										{
											
											
											for(t=0;t<12;t++)
											{
												//	temptrack2[lastindex-1-t]=0;
											}										
											temptrack2[lastindex]=0xA1;
											tempclock[lastindex]=0x0A;
											lastindex++;
											temptrack2[lastindex]=0xA1;
											tempclock[lastindex]=0x0A;
											lastindex++;
											temptrack2[lastindex]=0xA1;
											tempclock[lastindex]=0x0A;
											lastindex++;
											
											temptrack2[lastindex]=0xFB;
											lastindex++;
											
											sectorcfgs[j/2].trackoffset=lastindex;
											
											
											fseek(f,trackpos,SEEK_SET);
											fseek(f,sector[j/2].sector_pos,SEEK_CUR);
											
											floppycontext->hxc_printf(MSG_DEBUG,"read %d bytes of data sector at %x",real_sector_size_list[j/2],ftell(f));
											fread(&temptrack2[lastindex],real_sector_size_list[j/2],1,f);
											lastindex=lastindex+real_sector_size_list[j/2];
											
											
											//02 CRC The CRC of the data 
											CRC16_Init(&CRC16_High,&CRC16_Low,(unsigned char*)&crctable,0x1021,0xFFFF);
											
											for(t=0;t<(unsigned int)(real_sector_size_list[j/2]+4);t++)  CRC16_Update(&CRC16_High,&CRC16_Low, temptrack2[lastindex-(real_sector_size_list[j/2]+4)+t],(unsigned char*)&crctable);
											
											
											
											temptrack2[lastindex]=CRC16_High;
											lastindex++;
											
											temptrack2[lastindex]=CRC16_Low;
											
											
											if((sector[j/2].FDC_status&0x8)==0x8)
											{
												//temptrack2[lastindex]=CRC16_Low+2;
											}
											lastindex++;
											
											//lastindex=lastindex-real_sector_size_list[j/2];
											//lastindex++;
											
											
											floppycontext->hxc_printf(MSG_DEBUG,"data crc result: %.2X%.2X  %d bytes",CRC16_High,CRC16_Low,real_sector_size_list[j/2]+4);
											
											
										}
									}
									
								}
							}
						}
						///////////
						if(!floppydisk->tracks[tracknumber])
						{
							floppydisk->tracks[tracknumber]=(CYLINDER*)malloc(sizeof(CYLINDER));
							currentcylinder=floppydisk->tracks[tracknumber];
							currentcylinder->number_of_side=0;
							
							currentcylinder->sides=(SIDE**)malloc(sizeof(SIDE*)*2);
							memset(currentcylinder->sides,0,sizeof(SIDE*)*2);
						}
						
						currentcylinder->number_of_side++;
						currentcylinder->sides[sidenumber]=malloc(sizeof(SIDE));
						currentside=currentcylinder->sides[sidenumber]; 
						memset(currentcylinder->sides[sidenumber],0,sizeof(SIDE));
						
						
						currentside->track_encoding=ISOIBM_MFM_ENCODING;
						currentside->tracklen=tracklen*2;//(DEFAULT_DD_BITRATE/(DEFAULT_DD_RPM/60)/4);
						if(!trackheader.numberofsector)
							currentside->tracklen=tracklen*2;
						currentside->databuffer=malloc(currentside->tracklen);
						currentside->indexbuffer=malloc(currentside->tracklen);
						currentside->flakybitsbuffer=malloc(currentside->tracklen);
						currentside->timingbuffer=malloc(currentside->tracklen*sizeof(unsigned long));
						
						memset(currentside->databuffer,0,currentside->tracklen);
						memset(currentside->indexbuffer,0,currentside->tracklen);
						memset(currentside->flakybitsbuffer,0,currentside->tracklen);
						
						// bitrate
						currentside->bitrate=VARIABLEBITRATE;
						for(k=0;k<currentside->tracklen;k++)
						{
							currentside->timingbuffer[k]=DEFAULT_DD_BITRATE;
						}
						
						if(trackheader.unknowvalue&0x80)
						{
							tempclock[index_sync]=0x14;
							temptrack2[index_sync]=0xc2;
						}	
						
						
						BuildCylinder(currentside->databuffer,
							currentside->tracklen,
							tempclock,
							temptrack2,
							lastindex);
						
						
						for(j=0;j<(trackheader.numberofsector);j++)
						{
							
							
							
							for(l=(sectorcfgs[j].trackoffset*2);l<((sectorcfgs[j].trackoffset*2)+(sectorcfgs[j].size*2));l++)
							{
								currentside->timingbuffer[l]=(unsigned long)(((float)(sectorcfgs[j].size*8)/(float)sector[j].sector_speed_timing)*(float)1000000);
							}
							
							if((sector[j].FDC_status&0x88)==0x88)
							{
								fseek(f,trackpos-weaksectortotalsize,SEEK_SET);
								fseek(f,sectorcfgs[j].weakdata_offset,SEEK_CUR);
								
								fread(temptrack,sectorcfgs[j].size,1,f);
								//currentside->databuffer	
								
								for(l=(sectorcfgs[j].trackoffset*2);l<((sectorcfgs[j].trackoffset*2)+(sectorcfgs[j].size*2));l++)
								{
									if(!(l&1))
									{
										if(!(temptrack[l>>1]&0x80))
										{
											
											currentside->databuffer[l]=currentside->databuffer[l]&(~0xC0);
											currentside->flakybitsbuffer[l]=currentside->flakybitsbuffer[l]|(0xC0);
										}
										
										if(!(temptrack[l>>1]&0x40))
										{
											currentside->databuffer[l]=currentside->databuffer[l]&(~0x30);
											currentside->flakybitsbuffer[l]=currentside->flakybitsbuffer[l]|(0x30);
										}
										
										if(!(temptrack[l>>1]&0x20))
										{
											currentside->databuffer[l]=currentside->databuffer[l]&(~0x0C);
											currentside->flakybitsbuffer[l]=currentside->flakybitsbuffer[l]|(0x0C);
										}
										
										if(!(temptrack[l>>1]&0x10))
										{
											currentside->databuffer[l]=currentside->databuffer[l]&(~0x03);
											currentside->flakybitsbuffer[l]=currentside->flakybitsbuffer[l]|(0x03);
											
										}
										
									}
									else
									{
										
										if(!(temptrack[l>>1]&0x8))
										{
											
											currentside->databuffer[l]=currentside->databuffer[l]&(~0xC0);
											currentside->flakybitsbuffer[l]=currentside->flakybitsbuffer[l]|(0xC0);
										}
										
										if(!(temptrack[l>>1]&0x4))
										{
											currentside->databuffer[l]=currentside->databuffer[l]&(~0x30);
											currentside->flakybitsbuffer[l]=currentside->flakybitsbuffer[l]|(0x30);
										}
										
										if(!(temptrack[l>>1]&0x2))
										{
											currentside->databuffer[l]=currentside->databuffer[l]&(~0x0C);
											currentside->flakybitsbuffer[l]=currentside->flakybitsbuffer[l]|(0x0C);
										}
										
										if(!(temptrack[l>>1]&0x1))
										{
											currentside->databuffer[l]=currentside->databuffer[l]&(~0x03);
											currentside->flakybitsbuffer[l]=currentside->flakybitsbuffer[l]|(0x03);
											
										}
										
										
									}
									//	
								}
								
							}
						}
						
						////////
						
						free(temptrack2);
						free(tempclock);
						
						
						free(temptrack);
					}
					
					break;
				}
											
				fseek(f,trackheaderpos,SEEK_SET);	
				fseek(f,trackheader.tracksize,SEEK_CUR);	
				
				
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
							if(currentside->indexbuffer)
								fillindex(0,currentside,2500,TRUE,1);
						}
					}					
				}
			}
			return LOADER_NOERROR;
	}
	else
	{
		floppycontext->hxc_printf(MSG_ERROR,"non STX/pasti image (bad header)",imgfile);
		return LOADER_BADFILE;
	}
	
	return LOADER_INTERNALERROR;
}

