/*
//
// Copyright (C) 2006-2024 Jean-François DEL NERO
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

#include "types.h"

#include "internal_libhxcfe.h"
#include "tracks/track_generator.h"
#include "libhxcfe.h"

#include "cpcdsk_format.h"
#include "cpcdsk_loader.h"

#include "libhxcadaptor.h"

#include "floppy_utils.h"

#include "version.h"

int CPCDSK_libWrite_DiskFile(HXCFE_IMGLDR* imgldr_ctx,HXCFE_FLOPPY * floppy,char * filename)
{
	int32_t i,j,k,l,m,nbsector;
	FILE * outfile;
	char * log_str;
	char   tmp_str[256];
	char   disk_info_block[256];
	char rec_mode;
	int32_t sectorsize;
	int32_t tracksize;
	int32_t track_cnt,weak_sector;
	int32_t sectorlistoffset,trackinfooffset;
	cpcdsk_fileheader * cpcdsk_fh;
	cpcdsk_trackheader cpcdsk_th;
	cpcdsk_sector cpcdsk_s;
	int flag_limit_sector_size;
	int flag_discard_unformatted_2side;

	HXCFE_SECTORACCESS* ss;
	HXCFE_SECTCFG** sca;

	if( !imgldr_ctx || !floppy || !filename )
	{
		return HXCFE_BADPARAMETER;
	}

	imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"Write CPCDSK file %s...",filename);

	flag_limit_sector_size = hxcfe_getEnvVarValue( imgldr_ctx->hxcfe, "CPCDSK_WRITER_LIMIT_SECTOR_SIZE" );
	flag_discard_unformatted_2side = hxcfe_getEnvVarValue( imgldr_ctx->hxcfe, "CPCDSK_WRITER_DISCARD_UNFORMATTED_SIDE" );

	outfile = hxc_fopen(filename,"wb");
	if( !outfile )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot create %s !",filename);
		return HXCFE_ACCESSERROR;
	}

	memset(disk_info_block,0,0x100);

	cpcdsk_fh = (cpcdsk_fileheader *)&disk_info_block;
	memcpy((char*)&cpcdsk_fh->headertag,"EXTENDED CPC DSK File\r\nDisk-Info\r\n",sizeof(cpcdsk_fh->headertag));

	snprintf((char*)&cpcdsk_fh->creatorname,sizeof(cpcdsk_fh->creatorname),"HxC%s",STR_FILE_VERSION2);
	cpcdsk_fh->creatorname[sizeof(cpcdsk_fh->creatorname)-1] = 0;

	fwrite(&disk_info_block,0x100,1,outfile);
	track_cnt=0;

	ss = hxcfe_initSectorAccess(imgldr_ctx->hxcfe, floppy);
	if(!ss)
		goto error;

	for(j=0;j<(int)floppy->floppyNumberOfTrack;j++)
	{
		for(i=0;i<(int)floppy->floppyNumberOfSide;i++)
		{
			log_str = hxc_dyn_sprintfcat(NULL,"track:%.2d:%d file offset:0x%.6x, sectors: ",j,i,(unsigned int)ftell(outfile));

			hxcfe_imgCallProgressCallback(imgldr_ctx,(j<<1) | (i&1),floppy->floppyNumberOfTrack*2 );

			rec_mode = 2;  // MFM
			sca = hxcfe_getAllTrackSectors(ss,j,i,ISOIBM_MFM_ENCODING,&nbsector);
			if(!sca)
			{
				sca = hxcfe_getAllTrackSectors(ss,j,i,ISOIBM_FM_ENCODING,&nbsector);
				rec_mode = 1; // FM
				if(!nbsector)
				{
					rec_mode = 0; // Unknown
				}
			}

			memset(&cpcdsk_th,0,sizeof(cpcdsk_trackheader));
			sprintf((char*)&cpcdsk_th.headertag,"Track-Info\r\n");
			cpcdsk_th.side_number=i;
			cpcdsk_th.track_number=j;
			cpcdsk_th.gap3_length=78;
			cpcdsk_th.filler_byte=0xE5;
			cpcdsk_th.number_of_sector = nbsector;
			cpcdsk_th.rec_mode = rec_mode;

			switch(floppy->tracks[j]->sides[i]->bitrate)
			{
				case 250000:
					cpcdsk_th.datarate=1;
					break;
				case 500000:
					cpcdsk_th.datarate=2;
					break;
				case 1000000:
					cpcdsk_th.datarate=3;
					break;
				default:
					cpcdsk_th.datarate=0;
					break;
			}

			if(cpcdsk_fh->number_of_sides<(i+1))
				cpcdsk_fh->number_of_sides=i+1;

			if(cpcdsk_fh->number_of_tracks<(j+1))
				cpcdsk_fh->number_of_tracks=j+1;

			if(nbsector)
			{
				cpcdsk_th.sector_size_code=size_to_code(sca[0]->sectorsize);
			}

			trackinfooffset=ftell(outfile);

			if(nbsector)
			{
				fwrite(&cpcdsk_th,sizeof(cpcdsk_trackheader),1,outfile);

				sectorlistoffset = ftell(outfile);

				memset(&cpcdsk_s,0,sizeof(cpcdsk_sector));
				for(k=0;k<nbsector;k++)
				{
					fwrite(&cpcdsk_s,sizeof(cpcdsk_sector),1,outfile);
				}

				memset(tmp_str,0,0x100);
				fwrite(&tmp_str,0x100-(((sizeof(cpcdsk_sector)*nbsector)+sizeof(cpcdsk_trackheader))%0x100),1,outfile);

				sectorsize=sca[0]->sectorsize;

				k=0;
				do
				{
					memset(&cpcdsk_s,0,sizeof(cpcdsk_sector));

					weak_sector = 0;

					if(sca[k]->sectorsize!=sectorsize)
					{
						sectorsize=-1;
					}

					cpcdsk_s.sector_id = sca[k]->sector;
					cpcdsk_s.side = sca[k]->head;
					cpcdsk_s.track = sca[k]->cylinder;
					cpcdsk_s.sector_size_code = size_to_code(sca[k]->sectorsize);

					if(flag_limit_sector_size)
					{
						// Limit the sector data size to 6144...
						// Some emulators don't like bigger sectors...
						if(sca[k]->sectorsize > 6144)
							sca[k]->sectorsize = 6144;
					}

					cpcdsk_s.data_length = sca[k]->sectorsize;

					// ID part CRC ERROR ?
					if(sca[k]->use_alternate_header_crc)
					{
						cpcdsk_s.fdc_status_reg1 |= 0x20;
					}

					// Data part CRC ERROR ?
					if(sca[k]->use_alternate_data_crc)
					{
						cpcdsk_s.fdc_status_reg1 |= 0x20;
						cpcdsk_s.fdc_status_reg2 |= 0x20;
						if( sca[k]->weak_bits_mask )
						{
							l = 0;
							do
							{
								if( sca[k]->weak_bits_mask[l] )
								{
									weak_sector = 1;
								}
								l++;
							}while( !weak_sector && l < sca[k]->sectorsize );
						}
					}

					// Deleted Data Address Mark ?
					if(sca[k]->alternate_datamark)
					{
						if(sca[k]->alternate_datamark == 0xF8)
						{
							cpcdsk_s.fdc_status_reg2 |= 0x40;
						}
					}

					if( weak_sector )
					{
						cpcdsk_s.data_length *= 3;
					}

					fseek(outfile,sectorlistoffset+(k*sizeof(cpcdsk_sector)),SEEK_SET);
					fwrite(&cpcdsk_s,sizeof(cpcdsk_sector),1,outfile);

					fseek(outfile,0,SEEK_END);
					if(sca[k]->input_data)
					{
						if(weak_sector)
						{
							unsigned char * tmp_buf;

							tmp_buf = malloc(sca[k]->sectorsize);
							if(tmp_buf)
							{
								for(l=0;l<3;l++)
								{
									memcpy(tmp_buf, sca[k]->input_data, sca[k]->sectorsize);
									for(m=0;m<sca[k]->sectorsize;m++)
									{
										tmp_buf[m] = tmp_buf[m] ^ ( rand() & sca[k]->weak_bits_mask[m] );
									}

									fwrite(tmp_buf,sca[k]->sectorsize,1,outfile);
								}

								free(tmp_buf);
							}
						}
						else
						{
							fwrite(sca[k]->input_data,sca[k]->sectorsize,1,outfile);
						}
					}
					else
					{
						for(l=0;l<(int)sca[k]->sectorsize;l++)
						{
							fputc(sca[k]->fill_byte,outfile);
						}
					}

					log_str = hxc_dyn_sprintfcat(log_str,"%d ",sca[k]->sector);

					k++;

				}while(k<nbsector);

				k=0;
				while(k<nbsector)
				{
					hxcfe_freeSectorConfig( ss, sca[k] );
					k++;
				};

				free(sca);

				if(sectorsize!=-1)
				{
					log_str = hxc_dyn_sprintfcat(log_str,",%dB/s",sectorsize);
				}

				tracksize = (ftell(outfile)-trackinfooffset);
				disk_info_block[sizeof(cpcdsk_fileheader)+track_cnt] = (char)( tracksize / 256 );
				if(tracksize & 0xFF)
				{
					disk_info_block[sizeof(cpcdsk_fileheader)+track_cnt]++;
					//Padding...
					memset(&tmp_str,0,256);
					fwrite(&tmp_str,256 - (tracksize & 0xFF),1,outfile);
				}
			}
			else
			{
				// Unformatted track ...
				// A size of "0" indicates an unformatted track.
				// In this case there is no data, and no track information block for this track in the image file!
				disk_info_block[sizeof(cpcdsk_fileheader)+track_cnt] = 0;
			}

			track_cnt++;

			if(log_str)
				imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,log_str);

			free(log_str);
		}
	}

	hxcfe_deinitSectorAccess(ss);

	// It appears that some emulators don't like 2 sides disk
	// with the second side unformatted
	if( floppy->floppyNumberOfSide == 2 && flag_discard_unformatted_2side)
	{
		i = 0;
		while(i<floppy->floppyNumberOfTrack && !disk_info_block[sizeof(cpcdsk_fileheader)+ 1 + i*2])
		{
			i++;
		}

		if(i==floppy->floppyNumberOfTrack)
		{
			// Side 2 empty, discard it.
			i = 0;
			while(i<floppy->floppyNumberOfTrack && !disk_info_block[sizeof(cpcdsk_fileheader)+ 1 + i*2])
			{
				disk_info_block[sizeof(cpcdsk_fileheader) + i] = disk_info_block[sizeof(cpcdsk_fileheader) + i*2];
				if(i)
					disk_info_block[sizeof(cpcdsk_fileheader) + i*2] = 0;
				i++;
			}
			cpcdsk_fh->number_of_sides = 1;
		}
	}

	fseek(outfile,0,SEEK_SET);
	fwrite(&disk_info_block,0x100,1,outfile);

	hxc_fclose(outfile);

	return HXCFE_NOERROR;

error:
	if(outfile)
		hxc_fclose(outfile);

	return HXCFE_INTERNALERROR;
}
