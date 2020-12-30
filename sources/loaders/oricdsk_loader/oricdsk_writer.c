/*
//
// Copyright (C) 2006-2021 Jean-François DEL NERO
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

#include "oricdsk_format.h"
#include "oricdsk_loader.h"

#include "libhxcadaptor.h"

#include "floppy_utils.h"
#include "tracks/trackutils.h"

#include "version.h"

void setoricdsktrackbit(unsigned char * input_data,int bit_offset,int state)
{
	if(state)
	{
		input_data[bit_offset>>3] = input_data[bit_offset>>3] |  (0x80 >> ( bit_offset&0x7 ) );
	}
	else
	{
		input_data[bit_offset>>3] = input_data[bit_offset>>3] & ~(0x80 >> ( bit_offset&0x7 ) );
	}

	return;
}

#define ORICDSK_TRACK_SIZE 6400

int OricDSK_libWrite_DiskFile(HXCFE_IMGLDR* imgldr_ctx,HXCFE_FLOPPY * floppy,char * filename)
{
	int32_t i,j,k;
	FILE * oricdskfile;
	char   tmp_str[256];
	oricdsk_fileheader  * header;

	int32_t nbsector_mfm,nbsector_fm;

	unsigned char * track_buf = NULL;
	unsigned char * tmp_fm_track_buf = NULL;
	int32_t databitoffset;
	int32_t track_size;

	HXCFE_SECTORACCESS* ss = NULL;
	HXCFE_SECTCFG** sca_mfm = NULL;
	HXCFE_SECTCFG** sca_fm = NULL;

	imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"Write Oric DSK file %s...",filename);

	oricdskfile = hxc_fopen(filename,"wb");
	if( oricdskfile )
	{
		memset(&tmp_str,0,sizeof(tmp_str));

		header = (oricdsk_fileheader*)&tmp_str;

		memcpy((char*)&header->headertag,"MFM_DISK",8);

		header->number_of_side = floppy->floppyNumberOfSide;
		header->number_of_tracks = floppy->floppyNumberOfTrack;
		header->number_of_sectors_geometrie = 1;

		fwrite((char*)&tmp_str,sizeof(tmp_str),1,oricdskfile);

		track_buf = malloc(ORICDSK_TRACK_SIZE);
		if(!track_buf)
		{
			hxc_fclose(oricdskfile);

			return HXCFE_INTERNALERROR;
		}

		ss=hxcfe_initSectorAccess(imgldr_ctx->hxcfe,floppy);
		if(ss)
		{
			for(i=0;i<(int)floppy->floppyNumberOfSide;i++)
			{
				for(j=0;j<(int)floppy->floppyNumberOfTrack;j++)
				{
					track_size = 6250;

					memset(track_buf,0x4E,ORICDSK_TRACK_SIZE);

					hxcfe_imgCallProgressCallback(imgldr_ctx, (j<<1) + (i&1),floppy->floppyNumberOfTrack*2);

					nbsector_mfm = 0;
					nbsector_fm = 0;

					// Byte align the track...
					// TODO : Align each sectors ID Mark and Data Mark individually
					//        (Needed for stream based images sources)

					sca_mfm = hxcfe_getAllTrackSectors(ss,j,i,ISOIBM_MFM_ENCODING,&nbsector_mfm);
					if(sca_mfm)
					{
						if(sca_mfm[0]->startsectorindex&0xF)
						{
							hxcfe_shiftTrackData( imgldr_ctx->hxcfe, floppy->tracks[j]->sides[i], 0x10 - (sca_mfm[0]->startsectorindex&0xF) );
						}

						k=0;
						do
						{
							hxcfe_freeSectorConfig(ss,sca_mfm[k]);
							k++;
						}while(k<nbsector_mfm);

						if(sca_mfm)
							free(sca_mfm);

						sca_mfm = hxcfe_getAllTrackSectors(ss,j,i,ISOIBM_MFM_ENCODING,&nbsector_mfm);
					}

					sca_fm = hxcfe_getAllTrackSectors(ss,j,i,ISOIBM_FM_ENCODING,&nbsector_fm);
					if(sca_fm)
					{
						if(sca_fm[0]->startsectorindex&0xF)
						{
							hxcfe_shiftTrackData( imgldr_ctx->hxcfe, floppy->tracks[j]->sides[i], 0x10 - (sca_fm[0]->startsectorindex&0xF) );

							k=0;
							do
							{
								hxcfe_freeSectorConfig(ss,sca_fm[k]);
								k++;
							}while(k<nbsector_fm);

							if(sca_fm)
								free(sca_fm);

							sca_fm = hxcfe_getAllTrackSectors(ss,j,i,ISOIBM_FM_ENCODING,&nbsector_fm);
						}
					}

					if(nbsector_mfm)
					{
						if(sca_mfm[0]->startsectorindex&1)
						{
							databitoffset = 0;
						}
						else
						{
							databitoffset = 1;
						}

						for(k=0;k<track_size*8*2;k=k+2)
						{
							if(getbit(floppy->tracks[j]->sides[i]->databuffer,k+databitoffset))
							{
								setoricdsktrackbit(track_buf,k/2,0xFF);
							}
							else
							{
								setoricdsktrackbit(track_buf,k/2,0x00);
							}
						}

						k=0;
						do
						{
							hxcfe_freeSectorConfig(ss,sca_mfm[k]);
							k++;
						}while(k<nbsector_mfm);

						if(sca_mfm)
							free(sca_mfm);
					}

					if(nbsector_fm)
					{
						tmp_fm_track_buf = malloc(track_size+1);
						if(tmp_fm_track_buf)
						{
							memset(tmp_fm_track_buf,0xAA,track_size);

							databitoffset = ( 3 - (sca_fm[0]->startsectorindex&3) );

							for(k=0;k<track_size*8*2;k=k+4)
							{
								if(getbit(floppy->tracks[j]->sides[i]->databuffer,k+databitoffset))
								{
									setoricdsktrackbit(tmp_fm_track_buf,k/4,0xFF);
								}
								else
								{
									setoricdsktrackbit(tmp_fm_track_buf,k/4,0x00);
								}
							}

							for(k=0;k<track_size;k=k+2)
							{
								track_buf[k + 0] = tmp_fm_track_buf[k/2];
								track_buf[k + 1] = tmp_fm_track_buf[k/2];
							}

							free(tmp_fm_track_buf);
						}

						k=0;
						do
						{
							hxcfe_freeSectorConfig(ss,sca_fm[k]);
							k++;
						}while(k<nbsector_fm);

						if(sca_fm)
							free(sca_fm);
					}

					fwrite(track_buf,ORICDSK_TRACK_SIZE,1,oricdskfile);
				}
			}

			hxcfe_deinitSectorAccess(ss);
		}

		free(track_buf);

		hxc_fclose(oricdskfile);
	}
	else
	{
		return HXCFE_ACCESSERROR;
	}

	return 0;
}
