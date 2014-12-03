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
// File : stx_writer.c
// Contains: STX floppy image writer
//
// Written by:	DEL NERO Jean Francois
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "internal_libhxcfe.h"
#include "tracks/track_generator.h"
#include "libhxcfe.h"
#include "stx_loader.h"
#include "stx_writer.h"
#include "pasti_format.h"
#include "floppy_utils.h"
#include "tracks/sector_extractor.h"
#include "libhxcadaptor.h"
#include "types.h"

// Main writer function
int STX_libWrite_DiskFile(HXCFE_IMGLDR* imgldr_ctx,HXCFE_FLOPPY * floppy,char * filename)
{
	int i,j;
	FILE * stxdskfile;
	HXCFE_SECTORACCESS* ss;
	HXCFE_SECTCFG* sc;

	int cur_track_file_offset;
	int sect_cnt;
	int tracksize;
	int tracksectsize;

	pasti_fileheader header;
	pasti_trackheader track_header;
	pasti_sector sector_header;

	imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"Write MSA file %s...",filename);

	memset(&header,0,sizeof(pasti_fileheader));
	header.headertag[0] = 'R';
	header.headertag[1] = 'S';
	header.headertag[2] = 'Y';
	header.headertag[3] = 0;
	header.codeversion1 = 3;
	header.codeversion2 = 1;

	header.number_of_track = (unsigned char)floppy->floppyNumberOfTrack;
	if(floppy->floppyNumberOfSide&1)
		header.number_of_track |= 0x80;

	stxdskfile = hxc_fopen(filename,"w+b");
	if(stxdskfile)
	{
		fwrite(&header,sizeof(pasti_fileheader),1,stxdskfile);

		for(i=0;i<floppy->floppyNumberOfTrack;i++)
		{
			for(j=0;j<floppy->floppyNumberOfSide;j++)
			{
				hxcfe_imgCallProgressCallback(imgldr_ctx,(i<<1) | (j&1),floppy->floppyNumberOfTrack*2 );

				fseek(stxdskfile,0,SEEK_END);

				memset(&track_header,0,sizeof(pasti_trackheader));
				track_header.track_code = (j<<7) | i;
				track_header.flags = 0x0001;
				track_header.Tvalue = (unsigned short)((floppy->tracks[i]->sides[j]->tracklen / 8) / 2);

				tracksize = sizeof(pasti_trackheader);
				cur_track_file_offset = ftell(stxdskfile);

				fwrite(&track_header,sizeof(pasti_trackheader),1,stxdskfile);

				ss = hxcfe_initSectorAccess(imgldr_ctx->hxcfe,floppy);
				if(ss)
				{
					do
					{
						sc = hxcfe_getNextSector(ss,i,j,ISOIBM_MFM_ENCODING);
						if(sc)
						{
							memset(&sector_header,0,sizeof(pasti_sector));

							sector_header.sector_num = sc->sector;
							sector_header.track_num = sc->cylinder;
							sector_header.side_num = sc->head;

							sector_header.sector_pos_timing = (unsigned short)((MeasureTrackTiming(imgldr_ctx->hxcfe,floppy->tracks[i]->sides[j],0,sc->startsectorindex)*1000000)/4);

							if(sc->endsectorindex >= sc->startdataindex)
								sector_header.sector_speed_timing = (unsigned short)(((MeasureTrackTiming(imgldr_ctx->hxcfe,floppy->tracks[i]->sides[j],sc->startdataindex,sc->endsectorindex)*1000000)));
							else
								sector_header.sector_speed_timing = (unsigned short)((( MeasureTrackTiming(imgldr_ctx->hxcfe,floppy->tracks[i]->sides[j],0,sc->endsectorindex) +
																    MeasureTrackTiming(imgldr_ctx->hxcfe,floppy->tracks[i]->sides[j],floppy->tracks[i]->sides[j]->tracklen - sc->startdataindex,floppy->tracks[i]->sides[j]->tracklen) ) * 1000000));

							sector_header.header_crc = (sc->header_crc>>8) | (sc->header_crc<<8);

							sector_header.FDC_status = 0x00;
							if(sc->use_alternate_data_crc)
								sector_header.FDC_status = 0x88;
							sector_header.sector_size = size_to_code(sc->sectorsize);

							fwrite(&sector_header,sizeof(pasti_sector),1,stxdskfile);

							tracksize += sizeof(pasti_sector);
						}
					}while(sc);

					hxcfe_resetSearchTrackPosition(ss);

					sect_cnt = 0;
					tracksectsize = 0;

					do
					{
						sc = hxcfe_getNextSector(ss,i,j,ISOIBM_MFM_ENCODING);
						if(sc)
						{
							fseek(stxdskfile,cur_track_file_offset + sizeof(pasti_trackheader) + (sizeof(pasti_sector)*sect_cnt),SEEK_SET);

							memset(&sector_header,0,sizeof(pasti_sector));
							fread(&sector_header,sizeof(pasti_sector),1,stxdskfile);

							fseek(stxdskfile,0,SEEK_END);

							sector_header.sector_pos = tracksectsize;
							fwrite(sc->input_data,sc->sectorsize,1,stxdskfile);

							fseek(stxdskfile,cur_track_file_offset + sizeof(pasti_trackheader) + (sizeof(pasti_sector)*sect_cnt),SEEK_SET);

							fwrite(&sector_header,sizeof(pasti_sector),1,stxdskfile);

							sect_cnt++;
							tracksize += sc->sectorsize;
							tracksectsize += sc->sectorsize;

						}
					}while(sc);

					hxcfe_deinitSectorAccess(ss);
				}

				track_header.tracksize = tracksize;
				track_header.numberofsector = sect_cnt;

				fseek(stxdskfile,cur_track_file_offset,SEEK_SET);
				fwrite(&track_header,sizeof(pasti_trackheader),1,stxdskfile);

			}
		}

		fclose(stxdskfile);
	}

	return 0;
}
