/*
//
// Copyright (C) 2006 - 2012 Jean-François DEL NERO
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

#include "ti99v9t9_loader.h"

#include "tracks/sector_extractor.h"


int TI99V9T9_libWrite_DiskFile(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * floppy,char * filename)
{	
	int i,j,k;
	FILE * ti99v9t9file;
	int nbsector,imagesize;
	
	int numberofsector,numberofside,numberoftrack;
	int bitrate;
	int density;
	int interleave;
	int file_offset;
	unsigned int sectorsize;
	unsigned char * diskimage;
	int error;
	SECTORSEARCH* ss;
	SECTORCONFIG* sc;

	floppycontext->hxc_printf(MSG_INFO_1,"Write TI99 V9T9 file %s...",filename);
	
	imagesize=hxcfe_getFloppySize(floppycontext,floppy,&nbsector);

	floppycontext->hxc_printf(MSG_INFO_1,"Disk size : %d Bytes %d Sectors",imagesize,nbsector);

	numberofsector=9;
	numberofside=1;
	numberoftrack=40;
	bitrate=250000;
	density=ISOIBM_FM_ENCODING;
	interleave=4;
	sectorsize = 256;

	switch(imagesize)
	{
		case 1*40*9*256:
			numberofside=1;
			numberoftrack=40;
			numberofsector=9;
			bitrate=250000;
			density=ISOIBM_FM_ENCODING;
			interleave=4;
		break;
				
		case 2*40*9*256:
			// 180kbytes: either DSSD or 18-sector-per-track SSDD.
			// We assume DSSD since DSSD is more common and is supported by
			// the original TI SD disk controller.
			numberofside=2;
			numberoftrack=40;
			numberofsector=9;
			bitrate=250000;
			density=ISOIBM_FM_ENCODING;
			interleave=4;
		break;
				
		case 1*40*16*256:
			// 160kbytes: 16-sector-per-track SSDD (standard format for TI
			// DD disk controller prototype, and the TI hexbus disk
			// controller?) */
			numberofside=1;
			numberoftrack=40;
			numberofsector=16;
			bitrate=250000;
			density=ISOIBM_MFM_ENCODING;
			interleave=9;
		break;
				
		case 2*40*16*256:
			// 320kbytes: 16-sector-per-track DSDD (standard format for TI
			// DD disk controller prototype, and TI hexbus disk
			// controller?)
			numberofside=2;
			numberoftrack=40;
			numberofsector=16;
			bitrate=250000;
			density=ISOIBM_MFM_ENCODING;
			interleave=9;
			break;
				
		case 2*40*18*256:
			//  360kbytes: 18-sector-per-track DSDD (standard format for most
			// third-party DD disk controllers, but reportedly not supported by
			// the original TI DD disk controller prototype)
			numberofside=2;
			numberoftrack=40;
			numberofsector=18;
			bitrate=250000;
			density=ISOIBM_MFM_ENCODING;
			interleave=5;
			break;
				
		case 2*80*18*256:
			// 720kbytes: 18-sector-per-track 80-track DSDD (Myarc only)
			numberofside=2;
			numberoftrack=80;
			numberofsector=18;
			bitrate=250000;
			density=ISOIBM_MFM_ENCODING;
			interleave=5;
			break;
				
			case 2*80*36*256:
			// 1.44Mbytes: DSHD (Myarc only)
			numberofside=2;
			numberoftrack=80;
			numberofsector=36;
			bitrate=500000;
			density=ISOIBM_MFM_ENCODING;
			interleave=11;
			break;
				
		default:
			floppycontext->hxc_printf(MSG_ERROR,"Bad image size!..");
			return 0;
			break;
	}

	error = 0;
	imagesize = numberofsector * numberoftrack * numberofside * sectorsize;
	diskimage = malloc(imagesize) ;
	if(diskimage)
	{
		memset( diskimage ,0xF6 , numberofsector * numberoftrack * numberofside * sectorsize);

		ss = hxcfe_initSectorSearch(floppycontext,floppy);
		if(ss)
		{
			for(i=0;i<numberofside;i++)
			{
				for(j=0;j<numberoftrack;j++)
				{
					for(k=0;k<numberofsector;k++)
					{
						sc = hxcfe_searchSector(ss,j,i,k,density);
						if(sc)
						{
							if(sc->use_alternate_data_crc)
								floppycontext->hxc_printf(MSG_ERROR,"Warning : Bad Data CRC : T:%d H:%d S:%d Size :%dB",j,i,k,sc->sectorsize);
	
							if(sc->sectorsize == sectorsize)
							{
								if(i==0)
								{
									file_offset=(j*numberofsector)*sectorsize + ( k * sectorsize );	
								}
								else
								{
									file_offset=(  numberoftrack      *numberofsector*sectorsize) +
												(((numberoftrack-1)-j)*numberofsector*sectorsize) +
												( k * sectorsize );
								}
								memcpy(&diskimage[file_offset], sc->input_data, sectorsize);
							}
							else
							{
								error++;
								floppycontext->hxc_printf(MSG_ERROR,"Bad Sector Size : T:%d H:%d S:%d Size :%dB, Should be %dB",j,i,k,sc->sectorsize,sectorsize);
							}

							hxcfe_freeSectorConfig(ss,sc);
						}
						else
						{
							floppycontext->hxc_printf(MSG_ERROR,"Sector not found : T:%d H:%d S:%d",j,i,k);
						}
					}
				}
			}

			if(!error)
			{
				ti99v9t9file=fopen(filename,"wb");
				if(ti99v9t9file)
				{
					fwrite(diskimage,imagesize,1,ti99v9t9file);
					fclose(ti99v9t9file);
				}
				else
				{
					free(diskimage);
					hxcfe_deinitSectorSearch(ss);
					return HXCFE_ACCESSERROR;
				}
			}
		}

		free(diskimage);
		hxcfe_deinitSectorSearch(ss);

		if(!error)
			return HXCFE_NOERROR;
		else
		{
			floppycontext->hxc_printf(MSG_ERROR,"This disk have some errors !");
			return HXCFE_FILECORRUPTED;
		}
	}
	else
	{
		return HXCFE_INTERNALERROR;
	}
	
	return 0;
}
