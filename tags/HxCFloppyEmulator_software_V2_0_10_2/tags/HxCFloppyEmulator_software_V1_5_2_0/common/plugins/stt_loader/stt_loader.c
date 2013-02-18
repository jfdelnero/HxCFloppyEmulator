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
// File : stt_loader.c
// Contains: STT floppy image loader and plugins interfaces
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

#include "stt_loader.h"

#include "../common/os_api.h"

#include "sttfileformat.h"


int STT_libIsValidDiskFile(HXCFLOPPYEMULATOR* floppycontext,char * imgfile)
{
	int pathlen,filesize;
	char * filepath;
	FILE * f;
	stt_header STTHEADER;
	floppycontext->hxc_printf(MSG_DEBUG,"STT_libIsValidDiskFile %s",imgfile);
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
				
				if(strstr( filepath,".stt" )!=NULL)
				{

					f=fopen(imgfile,"rb");
					if(f==NULL) 
					{
						floppycontext->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
						return LOADER_ACCESSERROR;
					}
					
					fseek (f , 0 , SEEK_END); 
					filesize=ftell(f);
					fseek (f , 0 , SEEK_SET); 
					
					STTHEADER.stt_signature=0;
					fread(&STTHEADER,sizeof(stt_header),1,f);

					fclose(f);
					
					
					if(STTHEADER.stt_signature!=0x4D455453) //"STEM"
					{
						free(filepath);
						floppycontext->hxc_printf(MSG_DEBUG,"non STT IMG file - bad signature !");
						return LOADER_BADFILE;
					}

					floppycontext->hxc_printf(MSG_DEBUG,"STT file !");
					free(filepath);
					return LOADER_ISVALID;
				}
				else
				{
					floppycontext->hxc_printf(MSG_DEBUG,"non STT file !");
					free(filepath);
					return LOADER_BADFILE;
				}
			}
		}
	}
	
	return LOADER_BADPARAMETER;
}



int STT_libLoad_DiskFile(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	
	FILE * f;
	unsigned int filesize;
	unsigned int i,j,k,l;
	unsigned int file_offset;
	unsigned char* trackdata;
	int tracklen;
	int gap3len,interleave,rpm;
	int sectorsize,trackformat;
	unsigned long file_track_list_offset;
	CYLINDER* currentcylinder;
	SIDE* currentside;
	SECTORCONFIG* sectorconfig;
	stt_header STTHEADER;
	stt_track_offset STTTRACKOFFSET;
	stt_track_header STTTRACKHEADER;
	stt_sector       STTSECTOR;
	
	floppycontext->hxc_printf(MSG_DEBUG,"STT_libLoad_DiskFile %s",imgfile);
	
	f=fopen(imgfile,"rb");
	if(f==NULL) 
	{
		floppycontext->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return LOADER_ACCESSERROR;
	}
	
	fseek (f , 0 , SEEK_END); 
	filesize=ftell(f);
	fseek (f , 0 , SEEK_SET); 

	STTHEADER.stt_signature=0;
	fread(&STTHEADER,sizeof(stt_header),1,f);

				
	if(STTHEADER.stt_signature!=0x4D455453) //"STEM"
	{
		floppycontext->hxc_printf(MSG_DEBUG,"non STT IMG file - bad signature !");
		fclose(f);

		return LOADER_BADFILE;
	}

	file_track_list_offset=ftell(f);
	
	floppydisk->floppyNumberOfTrack=STTHEADER.number_of_tracks;
	floppydisk->floppyNumberOfSide=STTHEADER.number_of_sides;
	
	floppydisk->floppyBitRate=250000;
	floppydisk->floppyiftype=ATARIST_DD_FLOPPYMODE;
	floppydisk->tracks=(CYLINDER**)malloc(sizeof(CYLINDER*)*floppydisk->floppyNumberOfTrack);
	memset(floppydisk->tracks,0,sizeof(CYLINDER*)*floppydisk->floppyNumberOfTrack);
			
	rpm=300; // normal rpm
	
	interleave=1;
	gap3len=80;
	sectorsize=512;
	
	floppycontext->hxc_printf(MSG_INFO_1,"filesize:%dkB, %d tracks, %d side(s), %d sectors/track, gap3:%d, interleave:%d,rpm:%d",filesize/1024,floppydisk->floppyNumberOfTrack,floppydisk->floppyNumberOfSide,floppydisk->floppySectorPerTrack,gap3len,interleave,rpm);
				
	tracklen=(DEFAULT_DD_BITRATE/(rpm/60))/4;
		
	for(i=0;i<floppydisk->floppyNumberOfSide;i++)
	{

		for(j=0;j<floppydisk->floppyNumberOfTrack;j++)
		{
			if(!floppydisk->tracks[j])
			{
				floppydisk->tracks[j]=(CYLINDER*)malloc(sizeof(CYLINDER));
				currentcylinder=floppydisk->tracks[j];
				currentcylinder->number_of_side=floppydisk->floppyNumberOfSide;
				currentcylinder->sides=(SIDE**)malloc(sizeof(SIDE*)*currentcylinder->number_of_side);
				memset(currentcylinder->sides,0,sizeof(SIDE*)*currentcylinder->number_of_side);
			}

			currentcylinder=floppydisk->tracks[j];
			
		
			fseek (f, file_track_list_offset, SEEK_SET); 
			fread((void*)&STTTRACKOFFSET,sizeof(stt_track_offset),1,f);
			file_track_list_offset=file_track_list_offset+sizeof(stt_track_offset);
			
			fseek (f, STTTRACKOFFSET.track_offset, SEEK_SET); 

			fread((void*)&STTTRACKHEADER,sizeof(stt_track_header),1,f);
			floppycontext->hxc_printf(MSG_INFO_1,"Track: %d, Side: %d, Number of sector: %d, Tracks Flags:0x%.8x, Sector Flags:0x%.8x",j,i,STTTRACKHEADER.number_of_sectors,STTTRACKHEADER.tracks_flags,STTTRACKHEADER.sectors_flags);
	
			
			trackdata=0;
			l=0;
			sectorconfig=malloc(sizeof(SECTORCONFIG)*STTTRACKHEADER.number_of_sectors);
			memset(sectorconfig,0,sizeof(SECTORCONFIG)*STTTRACKHEADER.number_of_sectors);
			for(k=0;k<STTTRACKHEADER.number_of_sectors;k++)
			{
				fread((void*)&STTSECTOR,sizeof(stt_sector),1,f);
				
				floppycontext->hxc_printf(MSG_INFO_1,"Sector id: %d, Side id: %d, Track id: %d, Sector size:%d",STTSECTOR.sector_nb_id,STTSECTOR.side_nb_id,STTSECTOR.track_nb_id,STTSECTOR.data_len);
	
				sectorconfig[k].sector=STTSECTOR.sector_nb_id;
				sectorconfig[k].head=STTSECTOR.side_nb_id;
				sectorconfig[k].sectorsize=STTSECTOR.data_len;
				sectorconfig[k].use_alternate_sector_size_id=1;
				sectorconfig[k].alternate_sector_size_id=STTSECTOR.sector_len_code;
				sectorconfig[k].cylinder=STTSECTOR.track_nb_id;
				//sectorconfig[k].use_alternate_header_crc=0x2;
				sectorconfig[k].header_crc=(STTSECTOR.crc_byte_1<<8) | STTSECTOR.crc_byte_2;
								
				file_offset=ftell(f);

				
				trackdata=realloc(trackdata,l+STTSECTOR.data_len);
				fseek (f, STTSECTOR.data_offset + STTTRACKOFFSET.track_offset, SEEK_SET); 
				fread((void*)&trackdata[l],STTSECTOR.data_len,1,f);
				l=l+STTSECTOR.data_len;
				fseek (f, file_offset, SEEK_SET); 

			}


			currentcylinder->floppyRPM=rpm;
					
			currentcylinder->sides[i]=malloc(sizeof(SIDE));
			memset(currentcylinder->sides[i],0,sizeof(SIDE));
			currentside=currentcylinder->sides[i];
	
			tracklen=(DEFAULT_DD_BITRATE/(rpm/60))/4;
			currentside->number_of_sector=STTTRACKHEADER.number_of_sectors;
			gap3len=80;

			trackformat=ISOFORMAT_DD;
			if(currentside->number_of_sector==11 || currentside->number_of_sector==12)
			{
				trackformat=ISOFORMAT_DD11S;
			}
			
			while((2*ISOIBMGetTrackSize(trackformat,currentside->number_of_sector,sectorsize,gap3len,sectorconfig)>=tracklen) && (gap3len>3))
			{
				gap3len--;
			}

			if((2*ISOIBMGetTrackSize(trackformat,currentside->number_of_sector,sectorsize,gap3len,sectorconfig))>tracklen)
			{
				tracklen=(ISOIBMGetTrackSize(ISOFORMAT_DD,currentside->number_of_sector,sectorsize,gap3len,sectorconfig)*2);
			}

									
			currentside->tracklen=tracklen;
					
			currentside->databuffer=malloc(currentside->tracklen);
			memset(currentside->databuffer,0,currentside->tracklen);
					
			currentside->flakybitsbuffer=0;
				
			currentside->timingbuffer=0;
			currentside->bitrate=(4*tracklen)*5;
			currentside->track_encoding=ISOIBM_MFM_ENCODING;

			currentside->indexbuffer=malloc(currentside->tracklen);
			memset(currentside->indexbuffer,0,currentside->tracklen);						
					
			BuildISOTrack(floppycontext,trackformat,currentside->number_of_sector,1,sectorsize,j,i,gap3len,trackdata,currentside->databuffer,&currentside->tracklen,interleave,0,sectorconfig);

			currentside->tracklen=currentside->tracklen*8;

			fillindex(currentside->tracklen-1,currentside,2500,TRUE,1);
		}
	}

	free(trackdata);
			
	floppycontext->hxc_printf(MSG_INFO_1,"track file successfully loaded and encoded!");
		
	fclose(f);
	return LOADER_NOERROR;

}
