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

#include "internal_libhxcfe.h"
#include "tracks/track_generator.h"
#include "libhxcfe.h"

#include "sdd_speccydos_loader.h"
#include "sdd_speccydos_writer.h"
#include "tracks/sector_extractor.h"
#include "sddfileformat.h"
#include "libhxcadaptor.h"
#include "types.h"

// Main writer function
int SDDSpeccyDos_libWrite_DiskFile(HXCFE_IMGLDR* imgldr_ctx,HXCFE_FLOPPY * floppy,char * filename)
{
	int i,j,k,id;
	int nbsector;
	unsigned int sectorsize;

	FILE * sdddskfile;
	HXCFE_SECTORACCESS* ss;
	HXCFE_SECTCFG* sc;

	unsigned char * flat_track;

	imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"Write SDD file %s...",filename);

	sectorsize = 256;
	nbsector = 0;

	// Get the number of sector per track.
	ss = hxcfe_initSectorAccess(imgldr_ctx->hxcfe,floppy);
	if(ss)
	{
		id = 1;
		do
		{
			sc = hxcfe_searchSector(ss,0,0,id,ISOIBM_MFM_ENCODING);
			if(sc)
			{
				if(sc->sectorsize == sectorsize)
				{
					nbsector = id;

					if(sc->input_data)
						free(sc->input_data);
					free(sc);
				}
				else
				{
					if(sc->input_data)
						free(sc->input_data);
					free(sc);
					sc = 0;
				}
			}
			id++;
		}while(sc);

		hxcfe_deinitSectorAccess(ss);
	}

	if(nbsector)
	{
		flat_track = malloc(nbsector * sectorsize);
		if(flat_track)
		{
			memset(flat_track,0,nbsector * sectorsize);

			sdddskfile = hxc_fopen(filename,"wb");
			if(sdddskfile)
			{
				ss = hxcfe_initSectorAccess(imgldr_ctx->hxcfe,floppy);
				if(ss)
				{
					for(i = 0; i < (int)floppy->floppyNumberOfSide; i++)
					{
						for(j = 0; (j < (int)floppy->floppyNumberOfTrack); j++)
						{
							hxcfe_imgCallProgressCallback(imgldr_ctx, j + (i*floppy->floppyNumberOfTrack),floppy->floppyNumberOfTrack*2);
							for(k=0;k<(int)(sectorsize/16)*nbsector;k++)
							{
								memcpy(&flat_track[k*16],"<MISSINGSECTOR!>",16);
							}

							for(k=0;k<nbsector;k++)
							{
								sc = hxcfe_searchSector(ss,j,i,k+1,ISOIBM_MFM_ENCODING);
								if(sc)
								{
									if(sc->sectorsize == sectorsize)
									{
										if(sc->input_data)
										{
											memcpy((void*)&flat_track[k*sectorsize],sc->input_data,sc->sectorsize);
											free(sc->input_data);
										}
										free(sc);
									}
								}

							}

							fwrite(flat_track,nbsector * sectorsize,1,sdddskfile);
						}
					}

					hxcfe_deinitSectorAccess(ss);
				}

				hxc_fclose(sdddskfile);
			}

			if(flat_track)
				free(flat_track);
		}
	}

	return 0;
}
