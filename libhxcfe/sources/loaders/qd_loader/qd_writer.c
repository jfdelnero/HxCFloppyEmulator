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
///////////////////////////////////////////////////////////////////////////////////
//-------------------------------------------------------------------------------//
//-------------------------------------------------------------------------------//
//-----------H----H--X----X-----CCCCC----22222----0000-----0000------11----------//
//----------H----H----X-X-----C--------------2---0----0---0----0--1--1-----------//
//---------HHHHHH-----X------C----------22222---0----0---0----0-----1------------//
//--------H----H----X--X----C----------2-------0----0---0----0-----1-------------//
//-------H----H---X-----X---CCCCC-----222222----0000-----0000----1111------------//
//-------------------------------------------------------------------------------//
//----------------------------------------------------- http://hxc2001.free.fr --//
///////////////////////////////////////////////////////////////////////////////////
// File : qd_writer.c
// Contains: HxC Quickdisk floppy image writer
//
// Written by: Jean-François DEL NERO
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "types.h"

#include "internal_libhxcfe.h"
#include "libhxcfe.h"

#include "qd_loader.h"
#include "qd_format.h"

#include "tracks/luts.h"

#include "libhxcadaptor.h"

int QD_libWrite_DiskFile(HXCFE_IMGLDR* imgldr_ctx,HXCFE_FLOPPY * floppy,char * filename)
{

	FILE * hxcqdfile;
	qdhfefileformatheader header;
	qdtrack * track_blocks;
	unsigned char * mfmtrack;
	int mfmsize;
	int qd_mfmsize;

	unsigned char tmp_sector[512];

	unsigned int i,j,k;
	int l;
	unsigned int tracks_block_size;

	imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"Write QD file %s...",filename);

	track_blocks = NULL;
	mfmtrack = NULL;

	hxcqdfile = hxc_fopen(filename,"wb");
	if(hxcqdfile)
	{
		memset(&tmp_sector,0,sizeof(tmp_sector));
		memset(&header,0,sizeof(qdhfefileformatheader));
		memcpy((char*)&header.HEADERSIGNATURE,"HXCQDDRV",8);

		header.number_of_track = floppy->floppyNumberOfTrack;
		header.number_of_side = floppy->floppyNumberOfSide;
		header.bitRate = floppy->floppyBitRate * 2;

		if(floppy->floppyBitRate == VARIABLEBITRATE)
		{
			header.bitRate = 203128;
		}

		header.track_list_offset = 512;

		fwrite(&header,sizeof(header),1,hxcqdfile);
		fwrite(&tmp_sector,sizeof(tmp_sector) - sizeof(qdhfefileformatheader),1,hxcqdfile);

		imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"%d Tracks, %d side(s)",header.number_of_track,header.number_of_side);

		tracks_block_size = ((header.number_of_track*header.number_of_side)+1)*sizeof(qdtrack);

		if( tracks_block_size & 0x1FF )
		{
			tracks_block_size = (tracks_block_size & ~0x1FF) + 0x200;
		}

		track_blocks = (qdtrack*) malloc(tracks_block_size);
		if(!track_blocks)
			goto error;

		memset(track_blocks,0,tracks_block_size);

		fwrite(track_blocks,tracks_block_size,1,hxcqdfile);

		k = 0;
		i = 0;
		do
		{
			hxcfe_imgCallProgressCallback(imgldr_ctx,i,(header.number_of_track) );

			for(j=0;j<(header.number_of_side);j++)
			{
				mfmsize = floppy->tracks[i]->sides[j]->tracklen;
				if(mfmsize&7)
					mfmsize = (mfmsize/8)+1;
				else
					mfmsize = mfmsize/8;

				if(mfmsize&0x1FF)
					qd_mfmsize = ((mfmsize & ~0x1FF) + 0x200);
				else
					qd_mfmsize = mfmsize;

				track_blocks[k].offset = ftell(hxcqdfile);
				track_blocks[k].track_len = qd_mfmsize;

				mfmtrack = (unsigned char*) malloc(qd_mfmsize);
				if(!mfmtrack)
					goto error;

				memset(mfmtrack,0x01,qd_mfmsize);
				for(l=0;l<mfmsize;l++)
				{
					mfmtrack[l] = LUT_ByteBitsInverter[floppy->tracks[i]->sides[j]->databuffer[l]];
				}

				fwrite(mfmtrack,qd_mfmsize,1,hxcqdfile);

				track_blocks[k].start_sw_position = 0;
				track_blocks[k].stop_sw_position = 0;

				l = 0;
				while(l<mfmsize && !floppy->tracks[i]->sides[j]->indexbuffer[l])
				{
					l++;
				}

				if(l<mfmsize)
				{
					if( ( l & 0x1FF ) >= 0x100 )
						track_blocks[k].start_sw_position = ((l & ~0x1FF) + 0x200);
					else
						track_blocks[k].start_sw_position = ((l & ~0x1FF));

					while(l<mfmsize && floppy->tracks[i]->sides[j]->indexbuffer[l])
					{
						l++;
					}

					while(l<mfmsize && !floppy->tracks[i]->sides[j]->indexbuffer[l])
					{
						l++;
					}

					if(l<mfmsize)
					{
						if( (l & 0x1FF) >= 0x100 )
							track_blocks[k].stop_sw_position = ((l & ~0x1FF) + 0x200);
						else
							track_blocks[k].stop_sw_position = ((l & ~0x1FF));

					}
				}

				fseek(hxcqdfile,512,SEEK_SET);

				fwrite(track_blocks,tracks_block_size,1,hxcqdfile);

				fseek(hxcqdfile,0,SEEK_END);

				free(mfmtrack);
				mfmtrack = NULL;

				k++;
			}

			i++;
		}while(i<(header.number_of_track));

		free(track_blocks);

		hxc_fclose(hxcqdfile);

		return HXCFE_NOERROR;
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"QD_libWrite_DiskFile : Cannot create %s!",filename);

		return HXCFE_ACCESSERROR;
	}

error:
	imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"QD_libWrite_DiskFile : Memory allocation error !");

	free(track_blocks);

	free(mfmtrack);

	if(hxcqdfile)
		hxc_fclose(hxcqdfile);

	return HXCFE_INTERNALERROR;
}

