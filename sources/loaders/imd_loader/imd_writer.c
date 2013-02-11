/*
//
// Copyright (C) 2006 - 2012 Jean-Fran�ois DEL NERO
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
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "version.h"

#include "libhxcfe.h"

#include "imd_format.h"
#include "tracks/sector_extractor.h"

#include "libhxcadaptor.h"

extern unsigned char size_to_code(unsigned long size);

int IMD_libWrite_DiskFile(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * floppy,char * filename)
{	
	int i,j,k,l,nbsector;
	FILE * imdfile;
	char * log_str;
	char   tmp_str[256];
	char rec_mode;

	unsigned char sector_numbering_map[256];
	unsigned char cylinder_numbering_map[256];
	unsigned char side_numbering_map[256];

	int track_cnt;
	int sectorlistoffset,trackinfooffset;
	imd_trackheader imd_th;

	struct tm * ts;
	time_t currenttime;

	SECTORSEARCH* ss;
	SECTORCONFIG** sca;

//	struct DateTime reptime;


	floppycontext->hxc_printf(MSG_INFO_1,"Write IMD file %s...",filename);

	log_str=0;
	imdfile=hxc_fopen(filename,"wb");
	if(imdfile)
	{

		currenttime=time (NULL);
		ts=localtime(&currenttime);

		fprintf(imdfile,"IMD 1.17: %.2d/%.2d/%.4d %.2d:%.2d:%.2d\r\n",ts->tm_mday,ts->tm_mon,ts->tm_year+1900,ts->tm_hour,ts->tm_min,ts->tm_sec);
		fprintf(imdfile,"File generated by the HxC Floppy Emulator software v%s\r\n",STR_FILE_VERSION2);
		fprintf(imdfile,"%c",0x1A);

		memset(sector_numbering_map,0,0x100);
		memset(cylinder_numbering_map,0,0x100);
		memset(side_numbering_map,0,0x100);

		track_cnt=0;

		ss=hxcfe_initSectorSearch(floppycontext,floppy);
		if(ss)
		{

			for(j=0;j<(int)floppy->floppyNumberOfTrack;j++)
			{
				for(i=0;i<(int)floppy->floppyNumberOfSide;i++)
				{
					sprintf(tmp_str,"track:%.2d:%d file offset:0x%.6x, sectors: ",j,i,(unsigned int)ftell(imdfile));

					log_str=0;
					log_str=realloc(log_str,strlen(tmp_str)+1);
					memset(log_str,0,strlen(tmp_str)+1);
					strcat(log_str,tmp_str);

					rec_mode=0;
					rec_mode=2;

					sca = hxcfe_getAllTrackSectors(ss,j,i,ISOIBM_MFM_ENCODING,&nbsector);
					if(!sca)
					{
						sca = hxcfe_getAllTrackSectors(ss,j,i,ISOIBM_FM_ENCODING,&nbsector);
						rec_mode=1;
						if(!nbsector)
						{
							rec_mode=0;
							sca = hxcfe_getAllTrackSectors(ss,j,i,AMIGA_MFM_ENCODING,&nbsector);
						}
					}

					memset(&imd_th,0,sizeof(imd_trackheader));

					imd_th.physical_head=i;

					l=0;
					while((l<nbsector) && sca[l]->head == i )
					{
						l++;
					}
					if(l!=nbsector)
					{
						imd_th.physical_head=imd_th.physical_head | 0x40;
					}


					l=0;
					while((l<nbsector) && sca[l]->cylinder == j)
					{
						l++;
					}
					if(l!=nbsector)
					{
						imd_th.physical_head=imd_th.physical_head | 0x80;
					}

					imd_th.physical_cylinder=j;
					imd_th.number_of_sector=nbsector;

					imd_th.track_mode_code=rec_mode;

					switch(floppy->tracks[j]->sides[i]->bitrate)
					{
						case 250000:
							imd_th.track_mode_code=2;
							break;
						case 300000:
							imd_th.track_mode_code=1;
							break;
						case 500000:
							imd_th.track_mode_code=0;
							break;
						default:
							imd_th.track_mode_code=2;
							break;
					}

					if(rec_mode==2)
					{
						imd_th.track_mode_code=imd_th.track_mode_code+3;
					}

					if(nbsector)
					{
						imd_th.sector_size_code=size_to_code(sca[0]->sectorsize);
					}

					trackinfooffset=ftell(imdfile);
					fwrite(&imd_th,sizeof(imd_trackheader),1,imdfile);
					sectorlistoffset=ftell(imdfile);

					for(k=0;k<nbsector;k++)
					{
						sector_numbering_map[k] = sca[k]->sector;
						cylinder_numbering_map[k] = sca[k]->cylinder;
						side_numbering_map[k]= sca[k]->head;
					}

					fwrite(sector_numbering_map,imd_th.number_of_sector,1,imdfile);
					if(imd_th.physical_head & 0x80)fwrite(cylinder_numbering_map,imd_th.number_of_sector,1,imdfile);
					if(imd_th.physical_head & 0x40)fwrite(side_numbering_map,imd_th.number_of_sector,1,imdfile);

					if(nbsector)
					{

						k=0;
						do
						{


							l=0;
							while((l<(int)sca[k]->sectorsize) && sca[k]->input_data[l]==sca[k]->input_data[0])
							{
								l++;
							}

							if(l!=(int)sca[k]->sectorsize)
							{
								fputs("\1",imdfile);
								fwrite(sca[k]->input_data,sca[k]->sectorsize,1,imdfile);
							}
							else
							{
								fputs("\2",imdfile);
								fwrite(sca[k]->input_data,1,1,imdfile);
							}

							sprintf(tmp_str,"%d ",sca[k]->sector);
							log_str=realloc(log_str,strlen(log_str)+strlen(tmp_str)+1);
							strcat(log_str,tmp_str);
							k++;

						}while(k<nbsector);


						k=0;
						do
						{
							if(sca[k])
							{
								if(sca[k]->input_data)
									free(sca[k]->input_data);
								free(sca[k]);
							}
							k++;
						}while(k<nbsector);


						log_str=realloc(log_str,strlen(log_str)+strlen(tmp_str)+1);
						strcat(log_str,tmp_str);

					}

					track_cnt++;

					floppycontext->hxc_printf(MSG_INFO_1,log_str);
					free(log_str);

				}

			}

			hxcfe_deinitSectorSearch(ss);
		}

		hxc_fclose(imdfile);
	}

	return 0;
}