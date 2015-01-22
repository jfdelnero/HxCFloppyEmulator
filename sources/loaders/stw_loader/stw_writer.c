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

#include "tracks/trackutils.h"

#include "libhxcfe.h"

#include "stw_loader.h"
#include "stw_format.h"

#include "libhxcadaptor.h"

int STW_libWrite_DiskFile(HXCFE_IMGLDR* imgldr_ctx,HXCFE_FLOPPY * floppy,char * filename)
{

	FILE * stwfile;

	STWTRACKIMG stwtrackdesc;
	STWIMG stwheader;
	unsigned char * mfmtrack;
	int mfmsize;
	unsigned int i,j;
	int k;

	imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"Write STW file %s...",filename);

	stwfile=hxc_fopen(filename,"w+b");
	if(stwfile)
	{

		sprintf((char*)&stwheader.headername,"STW");
		stwheader.version = BIGENDIAN_WORD( 0x0100 );
		stwheader.number_of_track = floppy->floppyNumberOfTrack;
		stwheader.number_of_side = floppy->floppyNumberOfSide;
		stwheader.bytes_per_tracks = BIGENDIAN_WORD(6256);

		imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"%d Tracks, %d side(s)",stwheader.number_of_track,stwheader.number_of_side);

		mfmsize = 0;
		for( i = 0 ; i < stwheader.number_of_track ; i++ )
		{
			for( j = 0 ; j < stwheader.number_of_side ; j++ )
			{
				if(mfmsize<floppy->tracks[i]->sides[j]->tracklen)
					mfmsize = floppy->tracks[i]->sides[j]->tracklen;
			}
		}

		if(mfmsize&7)
			mfmsize=(mfmsize/8)+1;
		else
			mfmsize=mfmsize/8;

		stwheader.bytes_per_tracks = BIGENDIAN_WORD(mfmsize/2);

		fwrite(&stwheader,sizeof(stwheader),1,stwfile);

		mfmtrack=(unsigned char*) malloc(mfmsize);

		for( i = 0 ; i < stwheader.number_of_track ; i++ )
		{
			for( j = 0 ; j < stwheader.number_of_side ; j++ )
			{
				hxcfe_imgCallProgressCallback(imgldr_ctx,i,(stwheader.number_of_track) );

				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Write Track %d:%d [%x] %x bytes",i,j,ftell(stwfile),mfmsize);

				sprintf((char*)&stwtrackdesc.trkheader,"TRK");
				stwtrackdesc.side_number = j;
				stwtrackdesc.track_number = i;
				fwrite(&stwtrackdesc,sizeof(stwtrackdesc),1,stwfile);

				memset(mfmtrack,0xAA,mfmsize);

				for(k=0;k<floppy->tracks[i]->sides[j]->tracklen;k++)
				{
					setbit(mfmtrack,k,getbit(floppy->tracks[i]->sides[j]->databuffer,k));
				}

				fwrite(mfmtrack, mfmsize, 1 ,stwfile);
			}
		}

		free(mfmtrack);

		hxc_fclose(stwfile);

		return 0;
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot create %s!",filename);

		return -1;
	}
}
