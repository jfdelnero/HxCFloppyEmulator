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
#include "dim_loader.h"
#include "dim_writer.h"
#include "tracks/sector_extractor.h"
#include "libhxcadaptor.h"

#include "dim_format.h"

// Main writer function
int DIM_libWrite_DiskFile(HXCFE_IMGLDR* imgldr_ctx,HXCFE_FLOPPY * floppy,char * filename)
{
	int nbsector;
	int nbtrack;
	int nbside;
	int sectorsize;

	int writeret;
	int i;

	FILE * file;

	unsigned int sectorcnt_s0;
	unsigned int sectorcnt_s1;

	dim_header header;

	sectorsize = 512;

	hxcfe_imgCallProgressCallback(imgldr_ctx,0,floppy->floppyNumberOfTrack*2 );

	imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"Write DIM file %s...",filename);

	sectorcnt_s0 = count_sector(imgldr_ctx->hxcfe,floppy,1,0,0,sectorsize,ISOIBM_MFM_ENCODING);
	sectorcnt_s1 = count_sector(imgldr_ctx->hxcfe,floppy,1,0,1,sectorsize,ISOIBM_MFM_ENCODING);

	if(sectorcnt_s0>21 || sectorcnt_s0<9)
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"Error : Disk format doesn't match...",filename);
		return HXCFE_FILECORRUPTED;
	}

	nbtrack = 85;
	while(nbtrack && !count_sector(imgldr_ctx->hxcfe,floppy,1,nbtrack-1,0,sectorsize,ISOIBM_MFM_ENCODING))
	{
		nbtrack--;
	}

	nbside = 1;
	if(sectorcnt_s1)
		nbside = 2;

	nbsector = sectorcnt_s0;

	imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"%d sectors (%d bytes), %d tracks, %d sides...",nbsector,sectorsize,nbtrack,nbside);

	file = hxc_fopen(filename,"wb");
	if(file)
	{
		memset(&header,0,sizeof(dim_header));

		header.id_header = 0x4242;
		header.side = nbside - 1;
		header.nbsector = nbsector;
		header.start_track = 0;
		header.end_track = nbtrack - 1;
		if(nbsector>12)
			header.density = 1;
		else
			header.density = 0;

		fwrite(&header,sizeof(dim_header),1,file);
		
		for(i=0;i<16;i++)
			fputc(0x00,file);

		writeret = write_raw_file(imgldr_ctx,file,floppy,1,nbsector,nbtrack,nbside,sectorsize,ISOIBM_MFM_ENCODING,0);

		hxc_fclose(file);
	}

	return writeret;
}
