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

#include "apple2_do_loader.h"

#include "tracks/sector_extractor.h"

#include "libhxcadaptor.h"

int Apple2_do_libWrite_DiskFile(HXCFE_IMGLDR* imgldr_ctx,HXCFE_FLOPPY * floppy,char * filename)
{
	int32_t i,j,k,l,nbsector,sector;
	FILE * outfile;
	char * log_str;
	int32_t sectorsize,track_type_id;
	unsigned char * sector_mapping;

	HXCFE_SECTORACCESS* ss;
	HXCFE_SECTCFG** sca;

	if( hxc_checkfileext(filename,"po",SYS_PATH_TYPE) )
	{
		sector_mapping = (unsigned char *)&PhysicalToLogicalSectorMap_ProDos;
		imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"Write PO file %s...",filename);
	}
	else
	{
		sector_mapping = (unsigned char *)&PhysicalToLogicalSectorMap_Dos33;
		imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"Write DO file %s...",filename);
	}

	track_type_id = 0;
	sectorsize = -1;

	outfile = hxc_fopen(filename,"wb");
	if( !outfile )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot create %s !",filename);
		return HXCFE_ACCESSERROR;
	}

	ss = hxcfe_initSectorAccess(imgldr_ctx->hxcfe,floppy);
	if( !ss )
		goto error;

	for(j=0;j<(int)floppy->floppyNumberOfTrack;j++)
	{
		for(i=0;i<(int)floppy->floppyNumberOfSide;i++)
		{
			hxcfe_imgCallProgressCallback(imgldr_ctx,(j<<1) + (i&1),(2*floppy->floppyNumberOfTrack) );

			log_str = hxc_dyn_sprintfcat(NULL,"track:%.2d:%d file offset:0x%.6x, sectors: ",j,i,(unsigned int)ftell(outfile));

			sca = NULL;

			k=0;
			do
			{
				nbsector = 0;
				switch(track_type_id)
				{
					case 0:
						sca = hxcfe_getAllTrackSectors(ss,j,i,APPLEII_GCR1_ENCODING,&nbsector);
					break;
					case 1:
						sca = hxcfe_getAllTrackSectors(ss,j,i,APPLEII_GCR2_ENCODING,&nbsector);
					break;
					case 2:
						sca = hxcfe_getAllTrackSectors(ss,j,i,APPLEII_HDDD_A2_GCR1_ENCODING,&nbsector);
					break;
					case 3:
						sca = hxcfe_getAllTrackSectors(ss,j,i,APPLEII_HDDD_A2_GCR2_ENCODING,&nbsector);
					break;
				}

				if(!nbsector)
					track_type_id=(track_type_id+1)%4;

				k++;

			}while(!nbsector && k<7);

			if(sca && nbsector)
			{
				int total_track_size;

				total_track_size = 0;
				for(l=0;l<nbsector;l++)
				{
					if(sca[l]->sectorsize > 0)
						total_track_size += sca[l]->sectorsize;
				}

				if(total_track_size)
				{
					unsigned char *track_buffer = malloc(total_track_size);
					if(track_buffer)
					{
						memset(track_buffer,0,total_track_size);
						sectorsize = sca[0]->sectorsize;
						for(l=0;l<256;l++)
						{

							k=0;
							do
							{
								if(sca[k]->sector==l)
								{
									if(sca[k]->sectorsize!=sectorsize)
									{
										sectorsize=-1;
									}

									sector = sector_mapping[(sca[k]->sector)];

									if(sectorsize > 0 && sca[k]->input_data && ((sector * sectorsize) < total_track_size) && sector < 16)
										memcpy(&track_buffer[sector*sectorsize],sca[k]->input_data,sectorsize);

									log_str = hxc_dyn_sprintfcat(NULL,"%d ",sca[k]->sector);

									break;
								}

								k++;
							}while(k<nbsector);
						}

						fwrite(track_buffer,total_track_size,1,outfile);

						free(track_buffer);
					}
				}

				k=0;
				do
				{
					hxcfe_freeSectorConfig( ss, sca[k] );
					k++;
				}while(k<nbsector);

				if(sectorsize!=-1)
				{
					log_str = hxc_dyn_sprintfcat(log_str,",%dB/s",sectorsize);
				}
			}

			if(log_str)
				imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,log_str);

			free(log_str);
		}
	}

	hxcfe_deinitSectorAccess(ss);

	hxc_fclose(outfile);

	return HXCFE_NOERROR;

error:
	if(outfile)
		hxc_fclose(outfile);

	return HXCFE_INTERNALERROR;
}
