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
#include "msa_loader.h"
#include "msa_writer.h"
#include "msa_format.h"
#include "tracks/sector_extractor.h"
#include "libhxcadaptor.h"

unsigned short msapacktrack(unsigned char * inputtrack,int insize,unsigned char * outputtrack)
{
	int last_byte,bytecnt;
	int i,j,k;
	unsigned short fsize;

	last_byte = inputtrack[0];
	bytecnt = 1;
	j = 2;
	i = 1;
	do
	{
		if( last_byte != inputtrack[i] )
		{
			if(bytecnt >= 4  )
			{
				// rle
				outputtrack[j++] = 0xE5;
				outputtrack[j++] = last_byte;
				outputtrack[j++] = (bytecnt >> 8)&0xFF;
				outputtrack[j++] = (bytecnt)&0xFF;
			}
			else
			{
				if(last_byte == 0xE5)
				{
					outputtrack[j++] = 0xE5;
					outputtrack[j++] = last_byte; // E5
					outputtrack[j++] = (bytecnt >> 8)&0xFF;
					outputtrack[j++] = (bytecnt)&0xFF;
				}
				else
				{
					// direct
					for(k=0;k<bytecnt;k++)
					{
						outputtrack[j++] = last_byte;
					}
				}
			}

			bytecnt = 1;
		}
		else
		{
			bytecnt++;
		}

		last_byte = inputtrack[i];
		i++;

	}while(i<insize);

	if(bytecnt)
	{
		if(bytecnt >= 4  )
		{
			// rle
			outputtrack[j++] = 0xE5;
			outputtrack[j++] = last_byte;
			outputtrack[j++] = (bytecnt >> 8)&0xFF;
			outputtrack[j++] = (bytecnt)&0xFF;
		}
		else
		{
			if(last_byte == 0xE5)
			{
				outputtrack[j++] = 0xE5;
				outputtrack[j++] = last_byte; // E5
				outputtrack[j++] = (bytecnt >> 8)&0xFF;
				outputtrack[j++] = (bytecnt)&0xFF;
			}
			else
			{
				// direct
				for(k=0;k<bytecnt;k++)
				{
					outputtrack[j++] = last_byte;
				}
			}
		}
	}

	if((j-2)>=insize)
	{	//Packed track too big : Discard compression
		fsize = insize + 2;
		outputtrack[0] = (insize>>8)&0xFF;
		outputtrack[1] = insize&0xFF;
		memcpy(&outputtrack[2],inputtrack,insize);
	}
	else
	{
		fsize = j;
		outputtrack[0] = ( ( j-2 ) >> 8 ) & 0xFF;
		outputtrack[1] =  (j-2) & 0xFF;
	}

	return fsize;
}

// Main writer function
int MSA_libWrite_DiskFile(HXCFE_IMGLDR* imgldr_ctx,HXCFE_FLOPPY * floppy,char * filename)
{
	int i,j,k,id;
	int nbsector;

	FILE * msadskfile;
	HXCFE_SECTORACCESS* ss;
	HXCFE_SECTCFG* sc;

	unsigned char * flat_track;
	unsigned char * packed_track;
	unsigned short outsize;
	msa_header msah;

	imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"Write MSA file %s...",filename);

	memset(&msah,0,sizeof(msa_header));
	msah.sign[0] = 0xE;
	msah.sign[1] = 0xF;

	msah.number_of_side = BIGENDIAN_WORD( (floppy->floppyNumberOfSide - 1));
	msah.number_of_track = BIGENDIAN_WORD( (floppy->floppyNumberOfTrack - 1));

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
				if(sc->sectorsize == 512)
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

	msah.number_of_sector_per_track = BIGENDIAN_WORD(nbsector);

	if(nbsector)
	{
		packed_track =  malloc(nbsector * 512 * 4);
		flat_track = malloc(nbsector * 512);
		if(flat_track && packed_track)
		{
			memset(packed_track,0,nbsector * 512 * 4);
			memset(flat_track,0,nbsector * 512);

			msadskfile = hxc_fopen(filename,"wb");
			if(msadskfile)
			{
				fwrite(&msah,sizeof(msa_header),1,msadskfile);

				ss = hxcfe_initSectorAccess(imgldr_ctx->hxcfe,floppy);
				if(ss)
				{
					for(j = 0; (j < (int)floppy->floppyNumberOfTrack); j++)
					{
						for(i = 0; i < (int)floppy->floppyNumberOfSide; i++)
						{
							hxcfe_imgCallProgressCallback(imgldr_ctx,(j<<1) | (i&1),floppy->floppyNumberOfTrack*2 );

							for(k=0;k<nbsector;k++)
							{
								sc = hxcfe_searchSector(ss,j,i,k+1,ISOIBM_MFM_ENCODING);
								if(sc)
								{
									if(sc->sectorsize == 512)
									{
										if(sc->input_data)
										{
											memcpy((void*)&flat_track[k*512],sc->input_data,sc->sectorsize);
											free(sc->input_data);
										}
										free(sc);
									}
								}
							}

							outsize = msapacktrack(flat_track,k*512,packed_track);
							fwrite(packed_track,outsize,1,msadskfile);
						}
					}

					hxcfe_deinitSectorAccess(ss);
				}

				hxc_fclose(msadskfile);
			}

			if(flat_track)
				free(flat_track);

			if(packed_track)
				free(packed_track);
		}
	}

	return 0;
}
