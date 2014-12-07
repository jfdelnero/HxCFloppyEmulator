/*
//
// Copyright (C) 2006-2014 Jean-François DEL NERO
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

#include "jv3_format.h"
#include "jv3_loader.h"

#include "jv3_writer.h"

#include "tracks/sector_extractor.h"

#include "libhxcadaptor.h"

// Return a value that can be used in the JV3 flags field
unsigned char jv3flags(unsigned char num,unsigned char mask)
{
	while(mask % 2 == 0)
	{
		num <<= 1;
		mask >>= 1;
	}
	return num;
}

// Main writer function
int JV3_libWrite_DiskFile(HXCFE_IMGLDR* imgldr_ctx,HXCFE_FLOPPY * floppy,char * filename)
{
	int i,j,k,l;
	int nbsector;
	int sectorcount;
	int sectorsize;
	unsigned char density;
	unsigned char flags;
	FILE * jv3dskfile;
	JV3SectorHeader sectorheader[JV3_HEADER_MAX*2];  //JV3 allows for 2 sets of headers + data
	unsigned char * sectordata[JV3_HEADER_MAX*2];
	unsigned int sectorsizes[JV3_HEADER_MAX*2];
	HXCFE_SECTORACCESS* ss;
	HXCFE_SECTCFG** sca;

	imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"Write JV3 file %s...",filename);

	jv3dskfile = hxc_fopen(filename,"wb");
	if(jv3dskfile)
	{
		ss = hxcfe_initSectorAccess(imgldr_ctx->hxcfe,floppy);
		if(ss)
		{
			//Create sectorheader fields
			sectorsize = 0;
			sectorcount = 0;
			for(j = 0; (j < (int)floppy->floppyNumberOfTrack) && (sectorcount < JV3_HEADER_MAX*2); j++)
			{
				for(i = 0; (i < (int)floppy->floppyNumberOfSide) && (sectorcount < JV3_HEADER_MAX*2); i++)
				{
					hxcfe_imgCallProgressCallback(imgldr_ctx,(j<<1) + (i&1),(2*floppy->floppyNumberOfTrack) );

					sca = hxcfe_getAllTrackISOSectors(ss,j,i,&nbsector);
					if(sca)
					{
						for(k = 0; (k < nbsector) && (sectorcount < JV3_HEADER_MAX*2); k++)
						{
							density = 0;
							if(sca[k]->trackencoding == ISOFORMAT_DD )
							{
								density = 1;
							}

							sectordata[sectorcount] = sca[k]->input_data;
							sectorsizes[sectorcount] = sca[k]->sectorsize;
							sectorheader[sectorcount].track = sca[k]->cylinder;
							sectorheader[sectorcount].sector = sca[k]->sector;
							flags = 0;
							flags |= jv3flags(density,JV3_DENSITY);
							flags |= jv3flags(sca[k]->head,JV3_SIDE);
							switch (sca[k]->use_alternate_datamark?sca[k]->alternate_datamark:0xfb)
							{
								case 0xfb: flags |= jv3flags( (unsigned char)(density?JV3_DAM_FB_DD:JV3_DAM_FB_SD), JV3_DAM); break;
								case 0xf8: flags |= jv3flags( (unsigned char)(density?JV3_DAM_F8_DD:JV3_DAM_F8_SD), JV3_DAM); break;
								case 0xfa: flags |= jv3flags(JV3_DAM_FA_SD,JV3_DAM); break;
								case 0xf9: flags |= jv3flags(JV3_DAM_F9_SD,JV3_DAM); break;
							}
							switch (sca[k]->sectorsize)
							{
								case 128: flags |= jv3flags(JV3_SIZE_USED_128,JV3_SIZE); break;
								case 256: flags |= jv3flags(JV3_SIZE_USED_256,JV3_SIZE); break;
								case 512: flags |= jv3flags(JV3_SIZE_USED_512,JV3_SIZE); break;
								case 1024: flags |= jv3flags(JV3_SIZE_USED_1024,JV3_SIZE); break;
							}
							if ((sca[k]->use_alternate_header_crc) || (sca[k]->use_alternate_data_crc))
							{
								flags |= jv3flags(1,JV3_ERROR);
							}
							sectorheader[sectorcount].flags = flags;
							if (sca[k]->sectorsize > sectorsize) sectorsize = sca[k]->sectorsize;

							sectorcount++;

							free(sca[k]);  //still using sca[k]->input_data, so free it later
						}

						free(sca);
					}
				}
			}

			//Fill remaining sectorheader fields with empty values
			switch (sectorsize)
			{
				case 128: flags = JV3_FREEF | jv3flags(JV3_SIZE_FREE_128,JV3_SIZE); break;
				case 256: flags = JV3_FREEF | jv3flags(JV3_SIZE_FREE_256,JV3_SIZE); break;
				case 512: flags = JV3_FREEF | jv3flags(JV3_SIZE_FREE_512,JV3_SIZE); break;
				case 1024: flags = JV3_FREEF | jv3flags(JV3_SIZE_FREE_1024,JV3_SIZE); break;
				default: flags = JV3_FREE;
			}

			for (i = sectorcount; i < JV3_HEADER_MAX*2; i++)
			{
				sectorheader[i].track = JV3_FREE;
				sectorheader[i].sector = JV3_FREE;
				sectorheader[i].flags = flags;
			}

			//Finally write everything to the file
			for(i = 0; i < sectorcount; i++)
			{
				if (i % JV3_HEADER_MAX == 0)  //write headers
				{
					fwrite(sectorheader+i,sizeof(JV3SectorHeader),JV3_HEADER_MAX,jv3dskfile);
					fputc(0xff,jv3dskfile);  //write protect off
				}
				if(sectordata[i])
				{
					fwrite(sectordata[i],sizeof(unsigned char),sectorsizes[i],jv3dskfile);
					free(sectordata[i]);
				}
				else
				{	
					for(l=0;l<(int)sectorsizes[i];l++)
					{
						fputc(0,jv3dskfile);
					}
				}
			}

			hxcfe_deinitSectorAccess(ss);
		}

		hxc_fclose(jv3dskfile);
	}

	return 0;
}
