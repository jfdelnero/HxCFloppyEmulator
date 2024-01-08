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
#include "heathkit_loader.h"
#include "heathkit_writer.h"
#include "tracks/sector_extractor.h"
#include "libhxcadaptor.h"

// Main writer function
int Heathkit_libWrite_DiskFile(HXCFE_IMGLDR* imgldr_ctx,HXCFE_FLOPPY * floppy,char * filename)
{
	int nbsector;
	int nbtrack;
	int nbside;
	int sectorsize;

	int writeret;

	FILE * h8dfile;

	unsigned int sectorcnt_s0;
	unsigned int sectorcnt_s1;

	sectorsize = 256;

	hxcfe_imgCallProgressCallback(imgldr_ctx,0,floppy->floppyNumberOfTrack*2 );

	imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"Write Heathkit file %s...",filename);

	sectorcnt_s0 = count_sector(imgldr_ctx->hxcfe,floppy,0,0,0,sectorsize,HEATHKIT_HS_FM_ENCODING,0x0000);
	sectorcnt_s1 = count_sector(imgldr_ctx->hxcfe,floppy,0,0,1,sectorsize,HEATHKIT_HS_FM_ENCODING,0x0000);

	if(sectorcnt_s0 != 10)
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"Error : Disk format doesn't match...",filename);
		return HXCFE_FILECORRUPTED;
	}

	nbtrack = 40;
	while(nbtrack && !count_sector(imgldr_ctx->hxcfe,floppy,0,nbtrack-1,0,sectorsize,HEATHKIT_HS_FM_ENCODING,0x0000))
	{
		nbtrack--;
	}

	nbside = 1;
	if(sectorcnt_s1)
		nbside = 2;

	nbsector = sectorcnt_s0;

	imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"%d sectors (%d bytes), %d tracks, %d sides...",nbsector,sectorsize,nbtrack,nbside);

	writeret = HXCFE_ACCESSERROR;
	h8dfile = hxc_fopen(filename,"wb");
	if(h8dfile)
	{
		writeret = write_raw_file(imgldr_ctx,h8dfile,floppy,0,nbsector,nbtrack,nbside,sectorsize,HEATHKIT_HS_FM_ENCODING,1);

		hxc_fclose(h8dfile);
	}

	return writeret;
}
