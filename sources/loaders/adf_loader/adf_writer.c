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

#include "libhxcfe.h"

#include "adf_loader.h"

#include "tracks/sector_extractor.h"

#include "libhxcadaptor.h"

int ADF_libWrite_DiskFile(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * floppy,char * filename)
{
	int i,j,k,s;
	FILE * rawfile;
	unsigned char blankblock[512];
	int sectorsize,track_type_id;
	int systblockfound;

	SECTORSEARCH* ss;
	SECTORCONFIG* sc;

	floppycontext->hxc_printf(MSG_INFO_1,"Write ADF file %s...",filename);

	track_type_id=0;

	systblockfound = 0;

	memset(blankblock,0x00,sizeof(blankblock));

	if((floppy->floppyNumberOfTrack < 80) || (floppy->floppyNumberOfSide != 2) )
	{
		return HXCFE_BADPARAMETER;
	}

	rawfile=hxc_fopen(filename,"wb");
	if(rawfile)
	{
		ss=hxcfe_initSectorSearch(floppycontext,floppy);
		if(ss)
		{
			for(j=0;j<80;j++)
			{
				for(i=0;i<2;i++)
				{
					for(s=0;s<11;s++)
					{
						sc = hxcfe_searchSector (ss,j,i,s,AMIGA_MFM_ENCODING);

						if(sc)
						{
							sectorsize = sc->sectorsize;
							if(sectorsize == 512)
							{
								fwrite(sc->input_data,sectorsize,1,rawfile);
							}
							else
							{
								memset(blankblock,0x00,sizeof(blankblock));
								for(k=0;k<32;k++)
									strcat((char*)blankblock,">MISSING BLOCK<!");
								fwrite(blankblock,sizeof(blankblock),1,rawfile);
							}

							if(sc->input_data)
								free(sc->input_data);

							free(sc);
						}
						else
						{
							floppycontext->hxc_printf(MSG_WARNING,"T%.2dH%dS%d : Amiga Sector not found !?!...",j,i,s);
							// Sector Not found ?!?
							// Put a blank data sector instead...
							memset(blankblock,0x00,sizeof(blankblock));
							for(k=0;k<31;k++)
								strcat((char*)blankblock,">MISSING BLOCK<!");
							fwrite(blankblock,sizeof(blankblock),1,rawfile);
						}
					}
				}
			}
			hxcfe_deinitSectorSearch(ss);
		}
		hxc_fclose(rawfile);
	}

	return 0;
}
