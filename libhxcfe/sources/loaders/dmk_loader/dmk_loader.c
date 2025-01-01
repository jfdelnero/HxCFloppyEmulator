/*
//
// Copyright (C) 2006-2025 Jean-Fran�ois DEL NERO
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
// File : dmk_loader.c
// Contains: DMK TRS-80 floppy image loader
//
// Written by: Jean-Fran�ois DEL NERO
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "types.h"

#include "internal_libhxcfe.h"
#include "libhxcfe.h"
#include "./tracks/track_generator.h"

#include "floppy_loader.h"
#include "floppy_utils.h"

#include "dmk_loader.h"

#include "libhxcadaptor.h"

#include "dmk_format.h"

#include "dmk_writer.h"

int DMK_libIsValidDiskFile( HXCFE_IMGLDR * imgldr_ctx, HXCFE_IMGLDR_FILEINFOS * imgfile )
{
	dmk_header *dmk_h;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"DMK_libIsValidDiskFile");
	if(imgfile)
	{
		dmk_h = (dmk_header *)&imgfile->file_header;

		if(dmk_h->track_len)
		{
			if(!dmk_h->track_len)
			{
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"DMK_libIsValidDiskFile : non DMK file ! bad file size !");
				return HXCFE_BADFILE;
			}
			else
			{
				if( ( imgfile->file_size - sizeof(dmk_header) ) % dmk_h->track_len)
				{
					imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"DMK_libIsValidDiskFile : non DMK file ! bad file size !");
					return HXCFE_BADFILE;
				}
			}

			if(
				( (dmk_h->write_protected!=0xFF) && (dmk_h->write_protected!=0x00) ) ||
							( (dmk_h->rsvd_2[0]!=0x00 && dmk_h->rsvd_2[0]!=0x12) ||
							  (dmk_h->rsvd_2[1]!=0x00 && dmk_h->rsvd_2[1]!=0x34) ||
							  (dmk_h->rsvd_2[2]!=0x00 && dmk_h->rsvd_2[2]!=0x56) ||
							  (dmk_h->rsvd_2[3]!=0x00 && dmk_h->rsvd_2[3]!=0x78) )

						||

							(dmk_h->track_number==0 || dmk_h->track_number>90)

						||

							( (dmk_h->track_len>0x36B0) || (dmk_h->track_len<0xB00) )

						)
			{
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"DMK_libIsValidDiskFile : non DMK file ! bad header !");
				return HXCFE_BADFILE;
			}

			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"DMK_libIsValidDiskFile : DMK file !");
			return HXCFE_VALIDFILE;
		}

		if(hxc_checkfileext( imgfile->path,"dmk",SYS_PATH_TYPE))
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"DMK_libIsValidDiskFile : DMK file !");
			return HXCFE_VALIDFILE;
		}

		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"DMK_libIsValidDiskFile : non DMK file !");
		return HXCFE_BADFILE;

	}

	return HXCFE_BADPARAMETER;
}


HXCFE_SIDE* DMKpatchtrack(HXCFE* floppycontext,unsigned char * trackdata, unsigned char * trackclk,unsigned short * idamoffset,unsigned int tracklen,uint32_t * tracktotalsize, dmk_header *dmkh,int s)
{
	int i,j,l;
	unsigned int lastptr,lastdensity,tracksize,bitrate,k;
	unsigned char * track_density;
	unsigned char trackformat;
	unsigned int  final_tracklen;
	unsigned char trackstep;

	int sectorbegin;
	HXCFE_SIDE* currentside;
	track_generator tg;

	lastptr = 0;
	bitrate = 250000;
	if( tracklen > 9000 )
		bitrate = 500000;

	trackstep=1;
	if(dmkh->flags&0xC0)
	{
		trackstep=0;
	}

	memset(trackclk,0xFF,tracklen);

	k=0;
	track_density=malloc(tracklen+4);
	if(!track_density)
		return NULL;

	if(idamoffset[k]&0x8000)
	{
		memset(track_density,ISOFORMAT_DD,tracklen);
		lastdensity=ISOFORMAT_DD;
		trackformat=ISOFORMAT_DD;
	}
	else
	{
		lastdensity=ISOFORMAT_SD;
		memset(track_density,ISOFORMAT_SD,tracklen);
		trackformat=ISOFORMAT_SD;
	}

	floppycontext->hxc_printf(MSG_DEBUG,"----------------------------");

	while(idamoffset[k] && k<64)
	{
		floppycontext->hxc_printf(MSG_DEBUG,"IDAM Code : 0x%.4X",idamoffset[k]);
		if(idamoffset[k]&0x8000)
		{

			if((unsigned int)((idamoffset[k]&0x3FFF)-0x80-1)<tracklen)
				trackclk[((idamoffset[k]&0x3FFF)-0x80-1)]=0x0A;
			if((unsigned int)((idamoffset[k]&0x3FFF)-0x80-2)<tracklen)
				trackclk[((idamoffset[k]&0x3FFF)-0x80-2)]=0x0A;
			if((unsigned int)((idamoffset[k]&0x3FFF)-0x80-3)<tracklen)
				trackclk[((idamoffset[k]&0x3FFF)-0x80-3)]=0x0A;

			j=8;
			i=(idamoffset[k]&0x3FFF)-0x80;
			do
			{
				l=-1;
				if((trackdata[i+j]==0xA1) && (trackdata[i+j+1]==0xA1) && (trackdata[i+j+2]==0xA1) && ((trackdata[i+j+3]&0xF0)==0xF0))
				{
					l=j;
					trackclk[i+j+0]=0x0A;
					trackclk[i+j+1]=0x0A;
					trackclk[i+j+2]=0x0A;
					trackclk[i+j+3]=0xFF;
					j=64;
				}
			   j++;
			}while(j<64);

			sectorbegin=((idamoffset[k]&0x3FFF)-0x80);
			if(sectorbegin) sectorbegin--;
			j=3;
			while(sectorbegin && j && (trackdata[sectorbegin]==0xA1))
			{
				//trackclk[sectorbegin]=0x0A;
				j--;
				sectorbegin--;
			}

			j=12;
			while(sectorbegin && j && (trackdata[sectorbegin]==0x00))
			{
				j--;
				sectorbegin--;
			}

			j=512;
			while(sectorbegin && j && (trackdata[sectorbegin]==0x4E))
			{
				j--;
				sectorbegin--;
			}

			if(sectorbegin && trackdata[sectorbegin]==0xFC) sectorbegin--;
			j=3;
			while(sectorbegin && j && (trackdata[sectorbegin]==0xC2))
			{
				trackclk[sectorbegin]=0x0A;
				j--;
				sectorbegin--;
			}
			j=12;
			while(sectorbegin && j && (trackdata[sectorbegin]==0x00))
			{
				j--;
				sectorbegin--;
			}
			j=512;
			while(sectorbegin && j && (trackdata[sectorbegin]==0x4E))
			{
				j--;
				sectorbegin--;
			}

			while((lastptr<=(unsigned int)sectorbegin) && lastptr<tracklen)
			{
				track_density[lastptr]=lastdensity;
				lastptr++;
			}
			lastdensity=ISOFORMAT_DD;
			floppycontext->hxc_printf(MSG_DEBUG,"MFM %.4X - %.4X",~0x8000&idamoffset[k] ,l);
		}
		else
		{
			trackclk[(idamoffset[k]&0x3FFF)-0x80+0]=0xC7;
			trackclk[(idamoffset[k]&0x3FFF)-0x80+trackstep]=0xC7;

			j = 8 * (trackstep+1);
			i=(idamoffset[k]&0x3FFF)-0x80;
			do
			{
				l=-1;
				if((trackdata[i+j]==0xF8 && trackdata[i+j+trackstep]==0xF8) || (trackdata[i+j]==0xF9 && trackdata[i+j+trackstep]==0xF9)|| (trackdata[i+j]==0xFA && trackdata[i+j+trackstep]==0xFA)|| (trackdata[i+j]==0xFB && trackdata[i+j+trackstep]==0xFB))
				{
					l=j;
					trackclk[i+j+0]=0xC7;
					trackclk[i+j+trackstep]=0xC7;
					j=64;
				}

				j++;
			}while(j<64);

			sectorbegin=((idamoffset[k]&0x3FFF)-0x80);
			if(sectorbegin) sectorbegin--;

			j=12;
			while(sectorbegin && j && (trackdata[sectorbegin]==0x00))
			{
				j--;
				sectorbegin--;
			}

			j=512;
			while(sectorbegin && j && (trackdata[sectorbegin]==0xFF))
			{
				j--;
				sectorbegin--;
			}

			if(sectorbegin && trackdata[sectorbegin]==0xFC)
			{
				trackclk[sectorbegin]=0xD7;
				sectorbegin--;
			}
			if(sectorbegin && trackdata[sectorbegin]==0xFC)
			{
				trackclk[sectorbegin]=0xD7;
				sectorbegin--;
			}

			j=12;
			while(sectorbegin && j && (trackdata[sectorbegin]==0x00))
			{
				j--;
				sectorbegin--;
			}

			while((lastptr<(unsigned int)sectorbegin) && lastptr<tracklen)
			{
				track_density[lastptr]=lastdensity;
				lastptr++;
			}

			lastdensity=ISOFORMAT_SD;

			floppycontext->hxc_printf(MSG_DEBUG,"FM  %.4X - %.4X",idamoffset[k],l);
		}

		k++;
	};

	while(lastptr<tracklen)
	{
		track_density[lastptr]=lastdensity;
		lastptr++;
	}

	final_tracklen=0;
	k=0;
	do
	{
		if(track_density[k]==ISOFORMAT_SD)
		{
			k++;
			k=k+trackstep;
			final_tracklen=final_tracklen+4;
		}
		else
		{
			k++;
			final_tracklen=final_tracklen+2;
		}
	}while(k<tracklen);

	// alloc the track...
	tg_initTrackEncoder(&tg);
	tracksize = final_tracklen*8;

	if(tracksize&0x1F)
		tracksize=(tracksize&(~0x1F))+0x20;

	/*{
		unsigned char fname[512];
		sprintf(fname,"t_%.2dc.bin",s);
		savebuffer(fname,trackclk, tracklen);
		sprintf(fname,"t_%.2dd.bin",s);
		savebuffer(fname,trackdata, tracklen);
		sprintf(fname,"t_%.2db.bin",s);
		savebuffer(fname,track_density, tracklen);
	}*/

	currentside = tg_initTrack(&tg,tracksize,0,trackformat,bitrate,0,0);
	currentside->number_of_sector = k;
	k=0;
	do
	{
		pushTrackCode(&tg,trackdata[k],trackclk[k],currentside,track_density[k]);
		if(track_density[k]==ISOFORMAT_SD)
		{
			k++;
			k=k+trackstep;
		}
		else
		{
			k++;
		}

	}while(k<tracklen);

	free(track_density);

	tg_completeTrack(&tg,currentside,trackformat);

	return currentside;
}

int DMK_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	FILE * f;
	unsigned int filesize;
	int i,j;
	unsigned int file_offset;
	uint32_t tracktotalsize;
	unsigned char* trackdata;
	unsigned char* trackclk;
	int rpm;
	int numberoftrack,numberofside;
	HXCFE_CYLINDER* currentcylinder;
	HXCFE_SIDE* currentside;
	dmk_header dmk_h;
	unsigned short idam_offset_table[64];

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"DMK_libLoad_DiskFile %s",imgfile);

	f = hxc_fopen(imgfile,"rb");
	if( f == NULL )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	filesize = hxc_fgetsize(f);

	if( filesize )
	{
		hxc_fread(&dmk_h,sizeof(dmk_header),f);
		floppydisk->floppyBitRate = 250000;

		if( dmk_h.track_len > 9000 )
			floppydisk->floppyBitRate = 500000;

		numberofside = 2;
		if(dmk_h.flags & SINGLE_SIDE)
			numberofside--;

		numberoftrack = dmk_h.track_number;

		floppydisk->floppyNumberOfTrack = numberoftrack;
		floppydisk->floppyNumberOfSide = numberofside;
		floppydisk->floppySectorPerTrack = -1;
		floppydisk->floppyiftype = GENERIC_SHUGART_DD_FLOPPYMODE;

		floppydisk->tracks = (HXCFE_CYLINDER**)malloc(sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);
		if( !floppydisk->tracks )
		{
			hxc_fclose(f);
			return HXCFE_INTERNALERROR;
		}

		rpm = 300; // normal rpm

		if( floppydisk->floppyBitRate == 500000 && dmk_h.track_len < 11000 ) // 360 RPM disk ?
			rpm = 360;

		if( floppydisk->floppyBitRate == 250000 && dmk_h.track_len < 5800 ) // 360 RPM disk ?
			rpm = 360;

		imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"filesize:%dkB, %d tracks, %d side(s), %d sectors/track, rpm:%d",filesize/1024,floppydisk->floppyNumberOfTrack,floppydisk->floppyNumberOfSide,floppydisk->floppySectorPerTrack,rpm);

		trackdata=(unsigned char*)malloc(dmk_h.track_len+16);
		trackclk=(unsigned char*)malloc(dmk_h.track_len+16);

		if( !trackdata || !trackclk )
		{
			free(trackdata);
			free(trackclk);
			hxc_fclose(f);
			return HXCFE_INTERNALERROR;
		}

		for(j=0;j<floppydisk->floppyNumberOfTrack;j++)
		{
			floppydisk->tracks[j]=allocCylinderEntry(rpm,floppydisk->floppyNumberOfSide);
			currentcylinder=floppydisk->tracks[j];

			for(i=0;i<floppydisk->floppyNumberOfSide;i++)
			{
				hxcfe_imgCallProgressCallback(imgldr_ctx, (j<<1) + (i&1),floppydisk->floppyNumberOfTrack*2);

				file_offset=sizeof(dmk_header)+((dmk_h.track_len)*(j*floppydisk->floppyNumberOfSide))+((dmk_h.track_len)*(i&1));
				fseek (f , file_offset , SEEK_SET);
				hxc_fread(&idam_offset_table,128,f);
				hxc_fread(trackdata,dmk_h.track_len-128,f);
				memset(trackclk,0xFF,dmk_h.track_len-128);

				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Track %d Side %d Tracklen %d TTableOffset:0x%.8x",j,i,dmk_h.track_len,file_offset);

				currentside=DMKpatchtrack(imgldr_ctx->hxcfe,trackdata, trackclk,idam_offset_table,dmk_h.track_len-128,&tracktotalsize, &dmk_h,j);
				currentcylinder->sides[i]=currentside;
				fillindex(-2500,currentside,2500,TRUE,1);
			}
		}

		free(trackclk);
		free(trackdata);

		imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"track file successfully loaded and encoded!");

		hxc_fclose(f);

		return HXCFE_NOERROR;
	}

	imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"file size=%d !?",filesize);
	hxc_fclose(f);
	return HXCFE_BADFILE;
}

int DMK_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]="TRS80_DMK";
	static const char plug_desc[]="TRS80 DMK Loader";
	static const char plug_ext[]="dmk";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   DMK_libIsValidDiskFile,
		(LOADDISKFILE)      DMK_libLoad_DiskFile,
		(WRITEDISKFILE)     DMK_libWrite_DiskFile,
		(GETPLUGININFOS)    DMK_libGetPluginInfo
	};

	return libGetPluginInfo(
			imgldr_ctx,
			infotype,
			returnvalue,
			plug_id,
			plug_desc,
			&plug_funcs,
			plug_ext
			);
}

