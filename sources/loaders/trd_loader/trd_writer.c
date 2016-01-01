/*
//
// Copyright (C) 2006-2016 Jean-François DEL NERO
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
#include "trd_loader.h"
#include "trd_writer.h"
#include "tracks/sector_extractor.h"
#include "libhxcadaptor.h"

// Main writer function
int TRD_libWrite_DiskFile(HXCFE_IMGLDR* imgldr_ctx,HXCFE_FLOPPY * floppy,char * filename)
{
	int i,j,k,id;
	int nbsector;
	int nbtrack;
	int nbside;
	int sectorsize;
	int file_offset;
	int found;

	FILE * trddskfile;
	HXCFE_SECTORACCESS* ss;
	HXCFE_SECTCFG* sc;

	unsigned char datasector[256];
	unsigned int sectorcnt;
	int layoutformat;

	imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"Write TRD file %s...",filename);

	layoutformat = 0;
	sectorcnt = 0;

	// Get the number of sector per track.
	ss = hxcfe_initSectorAccess(imgldr_ctx->hxcfe,floppy);
	if(ss)
	{

		for(id = 1;id<=16;id++)
		{
			sc = hxcfe_searchSector(ss,0,0,id,ISOIBM_MFM_ENCODING);
			if(sc)
			{
				if(sc->sectorsize == 256)
				{
					sectorcnt++;
				}

				hxcfe_freeSectorConfig(sc,sc);
			}
		}

		if(sectorcnt!=16)
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"Error : Disk format doesn't match...",filename);
			hxcfe_deinitSectorAccess(ss);
			return HXCFE_FILECORRUPTED;
		}

		// 80 tracks?
		sectorcnt = 0;
		for(id = 1;id<=16;id++)
		{
			sc = hxcfe_searchSector(ss,42,0,id,ISOIBM_MFM_ENCODING);
			if(sc)
			{
				if(sc->sectorsize == 256)
				{
					sectorcnt++;
				}

				hxcfe_freeSectorConfig(sc,sc);
			}
		}

		if(sectorcnt==16)
		{
			layoutformat = 2;
		}


		// Double sided ?
		sectorcnt = 0;
		for(id = 1;id<=16;id++)
		{
			sc = hxcfe_searchSector(ss,0,1,id,ISOIBM_MFM_ENCODING);
			if(sc)
			{
				if(sc->sectorsize == 256)
				{
					sectorcnt++;
				}

				hxcfe_freeSectorConfig(ss,sc);
			}
		}

		if(sectorcnt==16)
		{
			layoutformat |= 1;
		}

		hxcfe_deinitSectorAccess(ss);
	}

	switch(layoutformat)
	{
		case 0:
			nbsector = 16;
			nbtrack = 40;
			nbside = 1;
			sectorsize = 256;
		break;
		case 1:
			nbsector = 16;
			nbtrack = 40;
			nbside = 2;
			sectorsize = 256;
		break;
		case 2:
			nbsector = 16;
			nbtrack = 80;
			nbside = 1;
			sectorsize = 256;
		break;
		case 3:
			nbsector = 16;
			nbtrack = 80;
			nbside = 2;
			sectorsize = 256;
		break;
		default:
			nbsector = 16;
			nbtrack = 40;
			nbside = 1;
			sectorsize = 256;
		break;
	}

	imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"%d sectors (%d bytes), %d tracks, %d sides...",nbsector,sectorsize,nbtrack,nbside);

	memset(datasector,0,sizeof(datasector));
	sprintf((char*)datasector,"!!! Missing Sector !!!\n");

	ss = hxcfe_initSectorAccess(imgldr_ctx->hxcfe,floppy);
	if(ss)
	{

		trddskfile = hxc_fopen(filename,"wb");
		if(trddskfile)
		{

			for(i=0;i<nbsector*nbtrack*nbside;i++)
			{
				fwrite(datasector,256,1,trddskfile);
			}

			for(k=0;k<nbtrack;k++)
			{
				for(j=0;j<nbside;j++)
				{
					hxcfe_imgCallProgressCallback(imgldr_ctx,(i<<1) + (j&1),nbside*nbtrack );

					for(i=0;i<nbsector;i++)
					{
						file_offset=(sectorsize*(k*nbsector*nbside))+
									(sectorsize*(nbsector)*j) + (sectorsize*i);

						found = 0;
						sc = hxcfe_searchSector(ss,k,j,i+1,ISOIBM_MFM_ENCODING);
						if(sc)
						{
							if(sc->sectorsize == 256)
							{
								if(sc->input_data)
								{
									fseek (trddskfile , file_offset , SEEK_SET);
									fwrite(sc->input_data,256,1,trddskfile);
									found = 1;
								}
							}
							hxcfe_freeSectorConfig(ss,sc);
						}

						if(!found)
						{
							imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Not found : Sector %d (%d bytes), Track %d, Side %d...",i,sectorsize,k,j);
						}
					}
				}
			}
			hxc_fclose(trddskfile);
		}

		hxcfe_deinitSectorAccess(ss);
	}

	return 0;
}
