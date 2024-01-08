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

/*Changelog
  Jan  2017 :  FD_writer.c (based on ST_writer)  by T. Missonier (sourcezax@users.sourceforge.net) */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "types.h"

#include "internal_libhxcfe.h"
#include "tracks/track_generator.h"
#include "libhxcfe.h"
#include "fd_loader.h"
#include "fd_writer.h"
#include "tracks/sector_extractor.h"
#include "libhxcadaptor.h"

// Main writer function

//special thomson fd raw (side by side)
int write_raw_FDfile(HXCFE_IMGLDR * imgldr_ctx,FILE * f,HXCFE_FLOPPY * fp,int32_t startidsector,int32_t sectorpertrack,int32_t nboftrack,int32_t nbofside,int32_t sectorsize,int32_t tracktype,int32_t sidefilelayout)
{
	int sect,trk,side,i;
	HXCFE_SECTORACCESS* ss;
	HXCFE_SECTCFG * scfg;
	int badsect,missingsect;
	const char * badsectmess = "!! BAD SECTOR !!";
	const char * misssectmess= "!!  MISSING   !!";

	badsect = 0;
	missingsect = 0;

	if(f && fp)
	{
		ss = hxcfe_initSectorAccess( imgldr_ctx->hxcfe, fp );
		if(ss)
		{
			hxcfe_setSectorAccessFlags( ss, SECTORACCESS_IGNORE_SIDE_ID);

			for( side = 0 ; side < nbofside; side++ )
			{
				for( trk = 0 ; trk < nboftrack ; trk++ )
				{
					for( sect = 0 ; sect < sectorpertrack ; sect++ )
					{
						scfg = hxcfe_searchSector ( ss, trk, side, startidsector + sect, tracktype );
						if( scfg )
						{
							if( scfg->use_alternate_data_crc || !scfg->input_data )
							{
								badsect++;
							}

							if( ( scfg->sectorsize == sectorsize ) && scfg->input_data )
							{
								fwrite( scfg->input_data, scfg->sectorsize, 1, f );
							}
							else
							{
								for( i = 0 ; i < sectorsize ; i++ )
								{
									fputc(badsectmess[i&0xF],f);
								}
							}

							hxcfe_freeSectorConfig( ss , scfg );
						}
						else
						{
							missingsect++;
							for( i = 0 ; i < sectorsize ; i++ )
							{
								fputc(misssectmess[i&0xF],f);
							}
						}
					}

					hxcfe_imgCallProgressCallback(imgldr_ctx,trk*2,nboftrack*2 );
				}
			}

			hxcfe_deinitSectorAccess(ss);
		}
	}

	if(badsect || missingsect)
		return HXCFE_FILECORRUPTED;
	else
		return HXCFE_NOERROR;
}
// Main writer function
int FD_libWrite_DiskFile(HXCFE_IMGLDR* imgldr_ctx,HXCFE_FLOPPY * floppy,char * filename)
{
	int nbsector;
	int nbtrack;
	int nbside;
	int sectorsize;

	int writeret;

	FILE * fdfile;

	unsigned int sectorcnt_s0;
	unsigned int sectorcnt_s1;

	sectorsize = 256;

	hxcfe_imgCallProgressCallback(imgldr_ctx,0,floppy->floppyNumberOfTrack*2 );

	imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"Write FD file %s...",filename);

	sectorcnt_s0 = count_sector(imgldr_ctx->hxcfe,floppy,1,0,0,sectorsize,ISOIBM_MFM_ENCODING,SECTORACCESS_IGNORE_SIDE_ID);
	sectorcnt_s1 = count_sector(imgldr_ctx->hxcfe,floppy,1,0,1,sectorsize,ISOIBM_MFM_ENCODING,SECTORACCESS_IGNORE_SIDE_ID);

	if(sectorcnt_s0!=16){
		imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"Error : Disk format doesn't match...",filename);
                return HXCFE_FILECORRUPTED;
	}

	nbtrack = 80;
	while(nbtrack && !count_sector(imgldr_ctx->hxcfe,floppy,1,nbtrack-1,0,sectorsize,ISOIBM_MFM_ENCODING,SECTORACCESS_IGNORE_SIDE_ID))
	{
		nbtrack--;
	}

	nbside = 1;
	if(sectorcnt_s1==16)
		nbside = 2;

	nbsector = sectorcnt_s0;

	imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"%d sectors (%d bytes), %d tracks, %d sides...",nbsector,sectorsize,nbtrack,nbside);

	writeret = HXCFE_ACCESSERROR;

	fdfile = hxc_fopen(filename,"wb");
	if(fdfile)
	{
		writeret = write_raw_FDfile(imgldr_ctx,fdfile,floppy,1,16,80,nbside,sectorsize,ISOIBM_MFM_ENCODING,0);
		hxc_fclose(fdfile);

		writeret = HXCFE_NOERROR;
	}

	return writeret;
}
