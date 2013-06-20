/*
//
// Copyright (C) 2006 - 2013 Jean-François DEL NERO
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

#include "libhxcfe.h"

#include "jv3_format.h"
#include "jv3_loader.h"

#include "jv3_writer.h"

#include "tracks/sector_extractor.h"

#include "libhxcadaptor.h"

int JV3_libWrite_DiskFile(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * floppy,char * filename)
{	
	int i,j,k,nbsector;
	FILE * jv3dskfile;
	char rec_mode;
	int sectorsize;
	int track_cnt;
	int sectorlistoffset,trackinfooffset;

	SECTORSEARCH* ss;
	SECTORCONFIG** sca;


	floppycontext->hxc_printf(MSG_INFO_1,"Write JV3 file %s...",filename);

	jv3dskfile=hxc_fopen(filename,"wb");
	if(jv3dskfile)
	{
		ss=hxcfe_initSectorSearch(floppycontext,floppy);
		
		if(ss)
		{
			for(j=0;j<(int)floppy->floppyNumberOfTrack;j++)
			{
				for(i=0;i<(int)floppy->floppyNumberOfSide;i++)
				{	
					rec_mode=2;
					sca = hxcfe_getAllTrackSectors(ss,j,i,ISOIBM_MFM_ENCODING,&nbsector);
					if(!sca)
					{
						sca = hxcfe_getAllTrackSectors(ss,j,i,ISOIBM_FM_ENCODING,&nbsector);
						rec_mode=1;
						if(!nbsector)
						{
							rec_mode=0;
						}
					}

					if(nbsector)
					{
						for(k=0;k<nbsector;k++) 
						{
						}

						k=0;
						do
						{
							free(sca[k]->input_data);
							free(sca[k]);
							k++;
						}while(k<nbsector);

					}
				}
			}

			hxcfe_deinitSectorSearch(ss);

		}

		hxc_fclose(jv3dskfile);
	}

	return 0;
}
