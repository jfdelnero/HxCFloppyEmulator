/*
//
// Copyright (C) 2006 - 2013 Jean-François DEL NERO
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

#include "libhxcfe.h"

#include "d88_format.h"
#include "d88_writer.h"

#include "tracks/sector_extractor.h"

#include "libhxcadaptor.h"

unsigned char  size_to_code_d88(unsigned long size)
{

	switch(size)
	{
		case 128:
			return 0;
		break;
		case 256:
			return 1;
		break;
		case 512:
			return 2;
		break;
		case 1024:
			return 3;
		break;
		case 2048:
			return 4;
		break;
		case 4096:
			return 5;
		break;
		case 8192:
			return 6;
		break;
		case 16384:
			return 7;
		break;
		default:
			return 0;
		break;
	}
}

int D88_libWrite_DiskFile(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * floppy,char * filename)
{
	int i,j,k,nbsector;
	FILE * d88file;
	char * log_str;
	char   tmp_str[256];
	int track_cnt,density;
	d88_fileheader d88_fh;
	d88_sector d88_s;
	unsigned long tracktable[164];

	int mfmdd_found;
	int mfmhd_found;
	int fmdd_found;
	int fmhd_found;

	int maxtrack;

	SECTORSEARCH* ss;
	SECTORCONFIG** sca;

	floppycontext->hxc_printf(MSG_INFO_1,"Write D88 file %s...",filename);

	log_str=0;

	mfmdd_found = 0;
	mfmhd_found = 0;
	fmdd_found = 0;
	fmhd_found = 0;
	maxtrack = 0;

	d88file=hxc_fopen(filename,"wb");
	if(d88file)
	{
		memset(&d88_fh,0,sizeof(d88_fileheader));
		sprintf((char*)&d88_fh.name,"HxCFE");
		fwrite(&d88_fh,sizeof(d88_fileheader),1,d88file);

		memset(tracktable,0,sizeof(tracktable));
		fwrite(&tracktable, sizeof(tracktable),1,d88file);

		track_cnt=0;

		ss = hxcfe_initSectorSearch(floppycontext,floppy);
		if( ss )
		{
			for(j=0;j<(int)floppy->floppyNumberOfTrack;j++)
			{
				for(i=0;i<(int)floppy->floppyNumberOfSide;i++)
				{
					sprintf(tmp_str,"track:%.2d:%d file offset:0x%.6x, sectors: ",j,i,(unsigned int)ftell(d88file));

					log_str=0;
					log_str=realloc(log_str,strlen(tmp_str)+1);
					memset(log_str,0,strlen(tmp_str)+1);
					strcat(log_str,tmp_str);

					if((j<<1 | i) <164)
					{
						sca = hxcfe_getAllTrackISOSectors(ss,j,i,&nbsector);
						if(sca)
						{

							tracktable[(j<<1 | i)] = ftell(d88file);

							for(k = 0; k < nbsector; k++)
							{

								memset(&d88_s,0,sizeof(d88_sector));

								density = 0;

								if(sca[k]->trackencoding == ISOFORMAT_DD )
								{
									density = 1;

									if(sca[k]->bitrate>400000)
										mfmhd_found++;
									else
										mfmdd_found++;
								}

								if(	sca[k]->trackencoding == ISOFORMAT_SD )
								{

									if(sca[k]->bitrate>400000)
										fmhd_found++;
									else
										fmdd_found++;
								}

								d88_s.cylinder = j;
								d88_s.head = i;
								d88_s.number_of_sectors = nbsector;
								d88_s.sector_id = sca[k]->sector;

								if(sca[k]->use_alternate_datamark && sca[k]->alternate_datamark == 0xF8)
									d88_s.deleted_data = 0x10;

								if(!density)
									d88_s.density = 0x40;

								d88_s.sector_size = size_to_code_d88(sca[k]->sectorsize);
								d88_s.sector_length = sca[k]->sectorsize;
								if(!sca[k]->input_data)
									d88_s.sector_length = 0;

								if( sca[k]->alternate_addressmark != 0xFE )
								{
									d88_s.sector_status = 0xE0; // NO ADDRESS MARK
								}
								else
								{
									if( sca[k]->use_alternate_header_crc )
									{
										d88_s.sector_status = 0xA0; // ID CRC ERROR
									}
									else
									{
										if( sca[k]->alternate_datamark != 0xFB && sca[k]->alternate_datamark != 0xF8 )
										{
											d88_s.sector_status = 0xF0; // NO DATA MARK
										}
										else
										{
											if( sca[k]->use_alternate_data_crc )
											{
												d88_s.sector_status = 0xB0; // DATA CRC ERROR
											}
											else
											{
												if( sca[k]->alternate_datamark == 0xF8 )
													d88_s.sector_status = 0x10; // DELETED SECTOR
											}
										}
									}
								}

								fwrite(&d88_s,sizeof(d88_sector),1,d88file);

								if(d88_s.sector_length)
									fwrite(sca[k]->input_data,sca[k]->sectorsize,1,d88file);

								free(sca[k]->input_data);
								free(sca[k]);
							}

							if(i==0)
								maxtrack++;

							free(sca);
						}
					}

					floppycontext->hxc_printf(MSG_INFO_1,log_str);
					free(log_str);

				}
			}

			hxcfe_deinitSectorSearch(ss);

		}

		// Media flag. 00h = 2D, 10h = 2DD, 20h = 2HD.
		if(	(maxtrack >= 46) )
		{
			// 2DD : 300RPM, 250Kb/s, 2 sides, 80 tracks floppy format
			// 2DD
			d88_fh.media_flag = 0x10;
		}

		if(	(maxtrack >= 46) && (mfmhd_found>mfmdd_found))
		{
			// 2HD : 360RPM, 500Kb/s, 2 sides, 77 tracks floppy format
			// 2HD
			d88_fh.media_flag = 0x20;
		}

		if(maxtrack < 46)
		{
			// 2D : 300RPM, 250Kb/s, 2 sides, 40 tracks floppy format . 
			// 2D
			d88_fh.media_flag = 0x00;
		}

		d88_fh.file_size = ftell(d88file);
		fseek(d88file,0,SEEK_SET);
		fwrite(&d88_fh,sizeof(d88_fileheader),1,d88file);
		fwrite(&tracktable, sizeof(tracktable),1,d88file);

		hxc_fclose(d88file);
	}

	return 0;
}
