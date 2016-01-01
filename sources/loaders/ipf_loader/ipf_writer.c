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
// File : ipf_writer.c
// Contains: IPF floppy image writer
//
// Written by:	DEL NERO Jean Francois
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "types.h"

#include "internal_libhxcfe.h"
#include "libhxcfe.h"

#include "floppy_loader.h"
#include "floppy_utils.h"

#include "ipf_loader.h"

#include "libhxcadaptor.h"

#include "ipf_format.h"
#include "tracks/std_crc32.h"
#include "tracks/sector_extractor.h"

void write_record_header(unsigned char * r_sign,unsigned char * data, unsigned long size, FILE * f)
{
	ipf_header ipfh;

	memset(&ipfh,0,sizeof(ipf_header));
	ipfh.id[0] = r_sign[0];
	ipfh.id[1] = r_sign[1];
	ipfh.id[2] = r_sign[2];
	ipfh.id[3] = r_sign[3];
	ipfh.len = BIGENDIAN_DWORD( (sizeof(ipf_header) + size) );

	ipfh.crc = 0;
	ipfh.crc = std_crc32(ipfh.crc, &ipfh, sizeof(ipf_header));
	ipfh.crc = std_crc32(ipfh.crc, data, size);

	ipfh.crc = BIGENDIAN_DWORD( ipfh.crc );
	fwrite(&ipfh,sizeof(ipf_header),1,f);
	if(data)
		fwrite(data,size,1,f);

}

// Main writer function
int IPF_libWrite_DiskFile(HXCFE_IMGLDR* imgldr_ctx,HXCFE_FLOPPY * floppy,char * filename)
{
	int nbsector;
	int nbtrack;
	int nbside;
	int sectorsize;
	int i,j;

	int writeret;

	FILE * file;

	unsigned int sectorcnt_s0;
	unsigned int sectorcnt_s1;

	ipf_info   ipfi;
	ipf_img    ipfimg;
	ipf_data   ipfd;

	sectorsize = 512;

	hxcfe_imgCallProgressCallback(imgldr_ctx,0,floppy->floppyNumberOfTrack*2 );

	imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"Write IPF file %s...",filename);

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
		write_record_header((unsigned char*)"CAPS",0,0, file);

		memset(&ipfi,0,sizeof(ipf_info));

		ipfi.disk_num =  BIGENDIAN_DWORD(1);
		ipfi.enc_rev =   BIGENDIAN_DWORD(1);
		ipfi.min_cyl =   BIGENDIAN_DWORD(0);
		ipfi.min_head =  BIGENDIAN_DWORD(0);
		ipfi.max_cyl =   BIGENDIAN_DWORD(floppy->floppyNumberOfTrack - 1);
		ipfi.max_head =  BIGENDIAN_DWORD(floppy->floppyNumberOfSide - 1);
		ipfi.origin =    BIGENDIAN_DWORD(0);
		ipfi.type =      BIGENDIAN_DWORD(1);
		ipfi.platform[0] = BIGENDIAN_DWORD(1);
		ipfi.release = IPF_ID;
		ipfi.revision = IPF_ID;
		ipfi.user_id = IPF_ID;
		write_record_header((unsigned char*)"INFO",(unsigned char*)&ipfi,sizeof(ipf_info), file);

		for(i=0;i<floppy->floppyNumberOfTrack;i++)
		{
			for(j=0;j<floppy->floppyNumberOfSide;j++)
			{
				memset(&ipfimg,0,sizeof(ipf_img));
				ipfimg.cyl = BIGENDIAN_DWORD(i);
				ipfimg.head = BIGENDIAN_DWORD(j);
				write_record_header((unsigned char*)"IMGE",(unsigned char*)&ipfimg,sizeof(ipf_img), file);
			}
		}

		for(i=0;i<floppy->floppyNumberOfTrack;i++)
		{
			for(j=0;j<floppy->floppyNumberOfSide;j++)
			{
				memset(&ipfd,0,sizeof(ipf_data));
				write_record_header((unsigned char*)"DATA",(unsigned char*)&ipfd,sizeof(ipf_data), file);
			}
		}

		hxc_fclose(file);
	}

	writeret = 0;

	return writeret;
}
