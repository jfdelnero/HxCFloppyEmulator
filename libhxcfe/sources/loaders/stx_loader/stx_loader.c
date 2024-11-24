/*
//
// Copyright (C) 2006-2024 Jean-Fran�ois DEL NERO
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
// File : stx_loader.c
// Contains: STX floppy image loader
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
#include "tracks/track_generator.h"
#include "libhxcfe.h"

#include "tracks/encoding/mfm_encoding.h"

#include "floppy_loader.h"
#include "floppy_utils.h"

#include "stx_loader.h"
#include "stx_writer.h"

#include "pasti_format.h"

#include "libhxcadaptor.h"
#include "tracks/crc.h"

// Enable the debug outputs by default
#define PASTI_DBG 1

int STX_libIsValidDiskFile( HXCFE_IMGLDR * imgldr_ctx, HXCFE_IMGLDR_FILEINFOS * imgfile )
{
	pasti_fileheader * fileheader;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"STX_libIsValidDiskFile");

	if(hxc_checkfileext(imgfile->path,"stx",SYS_PATH_TYPE))
	{
		fileheader = (pasti_fileheader*)imgfile->file_header;

		if(strcmp((char*)&fileheader->headertag,"RSY"))
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"STX_libIsValidDiskFile : non STX file (bad header)!");
			return HXCFE_BADFILE;
		}
		else
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"STX_libIsValidDiskFile : STX file !");
			return HXCFE_VALIDFILE;
		}
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"STX_libIsValidDiskFile : non STX file !");
		return HXCFE_BADFILE;
	}
}

unsigned char getchunk(unsigned char c)
{
	int i;
	unsigned char r;

	r=0;
	for(i=0;i<4;i++)
	{
		if((c>>i)&1)
		{
			r=r | (0x3<<(i*2));
		}
	}

	return r;
}


////////////////////////////////////////////////////
// Scan a track and make an sectors position list
////////////////////////////////////////////////////
int * getSectorHeaderOffset(HXCFE* floppycontext,unsigned char * trackbuffer,int buffersize,int numsector)
{
	int * offsetlist;
	int i,t;

	offsetlist=0;

	if(numsector)
	{
		offsetlist = malloc(sizeof(int) * numsector);
		if( !offsetlist )
			return NULL;

		for(i=0;i<numsector;i++)
			offsetlist[i] = 0xFFFFFFFF;

		t=0;
		i=0;
		while( i<buffersize && (t<numsector) )
		{
			if( (trackbuffer[i%buffersize]==0xA1) && (trackbuffer[(i+1)%buffersize]==0xA1) && ((trackbuffer[(i+2)%buffersize]==0xFE) || (trackbuffer[(i+2)%buffersize]==0xFF)) )
			{
				if(1)//trackbuffer[i-1]!=0xA1)
				{
					offsetlist[t]=i-1;
					floppycontext->hxc_printf(MSG_DEBUG,"[0x%X] header",offsetlist[t]);
					t++;
				}
			}

			i++;
		}
	}

	return offsetlist;
}


/////////////////////////////////////////////////////////////////
// This function try to found index/data mark and patch them.
// Used on tracks without sector.
/////////////////////////////////////////////////////////////////
void patchtrack(HXCFE* floppycontext,unsigned char * trackbuffer,unsigned char * trackclock,int buffersize,int tracknum)
{
	int lastindex;
	int i,doff;

	lastindex=1;
	i=0;
	while(i<(buffersize-4))
	{
		if( ( (trackbuffer[lastindex]==0xE1) || (trackbuffer[lastindex%buffersize]==0x0A) || (trackbuffer[lastindex%buffersize]==0x14) || (trackbuffer[(lastindex)%buffersize]==0xC2) ) && trackbuffer[(lastindex+1)%buffersize]==0xA1 && trackbuffer[(lastindex+2)%buffersize]==0xA1)
		{
			doff=lastindex;
			//trackbuffer[lastindex-1]=0x00;
			trackbuffer[lastindex]=0xA1;
			trackclock [lastindex]=0x0A;
			lastindex=(lastindex+1)%buffersize;
			trackbuffer[lastindex]=0xA1;
			trackclock [lastindex]=0x0A;
			lastindex=(lastindex+1)%buffersize;
			trackbuffer[lastindex]=0xA1;
			trackclock [lastindex]=0x0A;

		floppycontext->hxc_printf(MSG_DEBUG,"Patch track 0x%.2X - Offset :0x%.8X Data:%.2X %.2X %.2X %.2X",
			tracknum,
			doff,
			trackbuffer[doff%buffersize],
			trackbuffer[(doff+1)%buffersize],
			trackbuffer[(doff+2)%buffersize],
			trackbuffer[(doff+3)%buffersize]
			);

		}
		i++;
		lastindex++;
	};


}

unsigned char patchbyte(unsigned char * buffer,unsigned char * maskbuffer,int buffersize,int index,unsigned char byte)
{
	if(maskbuffer && (index>=buffersize))
	{

		if(maskbuffer[index%buffersize])
		{
			buffer[index%buffersize]=byte;
			maskbuffer[index%buffersize]=0x00;
		}
		else
			return 0xFF;
	}
	else
		buffer[index%buffersize]=byte;

	return 0x00;
}
/////////////////////////////////////////////////////////////////
// This function patch the track with a sector header and data
/////////////////////////////////////////////////////////////////
void addsector(HXCFE* floppycontext,unsigned char * trackbuffer,unsigned char * trackclock,unsigned char * trackmask,int buffersize,int * offsetlist,int numsector,HXCFE_SECTCFG * sector)
{
	int lastindex,shoff,doff;
	int i;

	shoff=-1;
	doff=-1;

	if(!numsector)
	{
		lastindex = offsetlist[numsector] % buffersize;

		shoff = lastindex;

		sector[numsector].startsectorindex = lastindex*2;

		//trackbuffer[lastindex-1]=0x00;
		patchbyte(trackbuffer,trackmask,buffersize,lastindex,0xA1);
		patchbyte(trackclock ,0        ,buffersize,lastindex++,0x0A);
		patchbyte(trackbuffer,trackmask,buffersize,lastindex,0xA1);
		patchbyte(trackclock ,0        ,buffersize,lastindex++,0x0A);
		patchbyte(trackbuffer,trackmask,buffersize,lastindex,0xA1);
		patchbyte(trackclock ,0        ,buffersize,lastindex++,0x0A);
		//patchbyte(trackbuffer,trackmask,buffersize,lastindex,0xFE);
		lastindex++;
		patchbyte(trackbuffer,trackmask,buffersize,lastindex++,(unsigned char)sector[numsector].cylinder);
		patchbyte(trackbuffer,trackmask,buffersize,lastindex++,(unsigned char)sector[numsector].head);
		patchbyte(trackbuffer,trackmask,buffersize,lastindex++,(unsigned char)sector[numsector].sector);
		patchbyte(trackbuffer,trackmask,buffersize,lastindex++,(unsigned char)sector[numsector].alternate_sector_size_id);
		patchbyte(trackbuffer,trackmask,buffersize,lastindex++,(unsigned char)(sector[numsector].header_crc&0xff));
		patchbyte(trackbuffer,trackmask,buffersize,lastindex++,(unsigned char)((sector[numsector].header_crc>>8)&0xff));

		if(sector[numsector].input_data)
		{
			i=0;
			while((trackbuffer[lastindex%buffersize]!=0xA1 || trackbuffer[(lastindex+1)%buffersize]!=0xA1 || trackbuffer[(lastindex+2)%buffersize]!=0xFB) && i<64)
			{
				i++;
				lastindex++;
			};

			if(i<64)
			{
				lastindex--;

				doff=lastindex%buffersize;

				patchbyte(trackbuffer,trackmask,buffersize,lastindex,0xA1);
				patchbyte(trackclock ,0        ,buffersize,lastindex++,0x0A);
				patchbyte(trackbuffer,trackmask,buffersize,lastindex,0xA1);
				patchbyte(trackclock ,0        ,buffersize,lastindex++,0x0A);
				patchbyte(trackbuffer,trackmask,buffersize,lastindex,0xA1);
				patchbyte(trackclock ,0        ,buffersize,lastindex++,0x0A);
				patchbyte(trackbuffer,trackmask,buffersize,lastindex++,0xFB);

				sector[numsector].startdataindex=(lastindex%buffersize)*2;

				for(i=0;i<sector[numsector].sectorsize;i++)
				{
					if(patchbyte(trackbuffer,trackmask,buffersize,lastindex,sector[numsector].input_data[i]))
					{
						i=sector[numsector].sectorsize;
					}
					lastindex++;
				}
			}
		}
	}
	else
	{

		if(offsetlist[numsector] > ((sector[numsector-1].startdataindex/2)+sector[numsector-1].sectorsize))
		{
			lastindex = offsetlist[numsector]-2;
		}
		else
		{
			lastindex = (sector[numsector-1].startsectorindex/2)+3;
		}

		i=0;
		while((trackbuffer[lastindex%buffersize]!=0xA1 || trackbuffer[(lastindex+1)%buffersize]!=0xA1 || ( (trackbuffer[(lastindex+2)%buffersize]!=0xFE) && (trackbuffer[(lastindex+2)%buffersize]!=0xFF) ) ) && i<(sector[numsector-1].sectorsize+64))
		{
			i++;
			lastindex++;
		};

		if(i==(sector[numsector-1].sectorsize+64))
		{
			lastindex = offsetlist[numsector];
		}
		else
		{
			lastindex--;
		}

		shoff = lastindex;

		if(lastindex!=0xFFFFFFFF)
		{
			sector[numsector].startsectorindex=(lastindex%buffersize)*2;

			//trackbuffer[lastindex-1]=0x00;
			patchbyte(trackbuffer,trackmask,buffersize,lastindex,0xA1);
			patchbyte(trackclock ,0        ,buffersize,lastindex++,0x0A);
			patchbyte(trackbuffer,trackmask,buffersize,lastindex,0xA1);
			patchbyte(trackclock ,0        ,buffersize,lastindex++,0x0A);
			patchbyte(trackbuffer,trackmask,buffersize,lastindex,0xA1);
			patchbyte(trackclock ,0        ,buffersize,lastindex++,0x0A);
			//patchbyte(trackbuffer,trackmask,buffersize,lastindex++,0xFE);
			lastindex++;
			patchbyte(trackbuffer,trackmask,buffersize,lastindex++,(unsigned char)sector[numsector].cylinder);
			patchbyte(trackbuffer,trackmask,buffersize,lastindex++,(unsigned char)sector[numsector].head);
			patchbyte(trackbuffer,trackmask,buffersize,lastindex++,(unsigned char)sector[numsector].sector);
			patchbyte(trackbuffer,trackmask,buffersize,lastindex++,(unsigned char)sector[numsector].alternate_sector_size_id);
			patchbyte(trackbuffer,trackmask,buffersize,lastindex++,(unsigned char)(sector[numsector].header_crc&0xff));
			patchbyte(trackbuffer,trackmask,buffersize,lastindex++,(unsigned char)((sector[numsector].header_crc>>8)&0xff));

			if(sector[numsector].input_data)
			{
				i=0;
				while((trackbuffer[lastindex%buffersize]!=0xA1 || trackbuffer[(lastindex+1)%buffersize]!=0xA1 || trackbuffer[(lastindex+2)%buffersize]!=0xFB) && i<64)
				{
					i++;
					lastindex++;
				};

				lastindex--;

				doff=lastindex%buffersize;

				patchbyte(trackbuffer,trackmask,buffersize,lastindex,0xA1);
				patchbyte(trackclock ,0        ,buffersize,lastindex++,0x0A);

				patchbyte(trackbuffer,trackmask,buffersize,lastindex,0xA1);
				patchbyte(trackclock ,0        ,buffersize,lastindex++,0x0A);

				patchbyte(trackbuffer,trackmask,buffersize,lastindex,0xA1);
				patchbyte(trackclock ,0        ,buffersize,lastindex++,0x0A);

				patchbyte(trackbuffer,trackmask,buffersize,lastindex++,0xFB);

				sector[numsector].startdataindex=(lastindex%buffersize)*2;

				for(i=0;i<sector[numsector].sectorsize;i++)
				{
					if(patchbyte(trackbuffer,trackmask,buffersize,lastindex,sector[numsector].input_data[i]))
					{
						i=sector[numsector].sectorsize;
					}
					lastindex++;
				}
			}
		}
	}

	floppycontext->hxc_printf(MSG_DEBUG,"Patch track - Track ID: 0x%.2X, Side ID: 0x%.2X, Sector ID: 0x%.2X Size:0x%.4X (Id:0x%.2X), Header Offset :0x%.8X, Data Offset :0x%.8X - 0x%.8X (0x%.3X)",
		sector[numsector].cylinder,
		sector[numsector].head,
		sector[numsector].sector,
		sector[numsector].sectorsize,
		sector[numsector].alternate_sector_size_id,
		shoff,
		doff,
		doff+sector[numsector].sectorsize,
		doff-shoff
		);
}

int STX_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	FILE * f;
	int t;
	int i,j,k,l;
	int trackformat;
	int interleave;
	unsigned short temp_val;
	unsigned short index_sync;
	unsigned char numberoftrackperside;
	int tracknumber,sidenumber;
	unsigned int numberoftrack;
	int numberofside;
	int trackmode;
	pasti_fileheader  fileheader;
	pasti_trackheader trackheader;
	pasti_sector * sector;
	HXCFE_SECTCFG* sectorconfig;
	int * sectordata_index;
	unsigned char * temptrack;
	unsigned char * tempclock;
	unsigned char * tempmask;

	int trackpos,trackheaderpos;
	int tracksize;
	int presenceside[2];
	unsigned int lastindex;
	int weaksectortotalsize;
	int tracklen;
	unsigned char crctable[32];
	unsigned char CRC16_High;
	unsigned char CRC16_Low;
	int sectorlistoffset;
	int weaksectoroffset;
	unsigned char * weaksectorbuffer;

	HXCFE_CYLINDER* currentcylinder;
	HXCFE_SIDE* currentside;

#ifdef PASTI_DBG
	char tempstring[512];
	unsigned int debug_i;
#endif

	currentcylinder = 0;

	sector = NULL;
	sectorconfig = NULL;
	sectordata_index = NULL;
	temptrack = NULL;
	tempclock = NULL;
	tempmask = NULL;
	weaksectorbuffer = NULL;

	interleave = 1;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"STX_libLoad_DiskFile %s",imgfile);

	f = hxc_fopen(imgfile,"rb");
	if( f == NULL )
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Image Info: %s\n",imgfile);

	hxc_fread( &fileheader, sizeof(pasti_fileheader), f );

	if(!strcmp((char*)&fileheader.headertag,"RSY"))
	{
		numberoftrack = fileheader.number_of_track;
		t=ftell(f);

		//comptage track / side
		presenceside[0]=0;
		presenceside[1]=0;
		numberoftrackperside=0;
		sectordata_index=0;

		for(i=0;i<numberoftrack;i++)
		{
			trackheaderpos=ftell(f);
			hxc_fread( &trackheader, sizeof(trackheader), f );

			presenceside[trackheader.track_code>>7]=1;
			if((trackheader.track_code&(unsigned char)0x7F)>numberoftrackperside)
			{
				numberoftrackperside=trackheader.track_code&0x7F;
			}
			fseek(f,trackheaderpos,SEEK_SET);

			fseek(f,trackheader.track_block_size,SEEK_CUR);
		}

		numberoftrackperside++;

		fseek(f,t,SEEK_SET);

		if(presenceside[0] && presenceside[1])
		{
			numberofside=2;
		}
		else
		{
			numberofside=1;
		}

		floppydisk->floppySectorPerTrack=-1;
		floppydisk->floppyNumberOfSide=numberofside;
		floppydisk->floppyNumberOfTrack=numberoftrackperside;
		floppydisk->floppyiftype=ATARIST_DD_FLOPPYMODE;
		floppydisk->floppyBitRate=250000;

		//STXFloppylist[floppyid]->floppyRPM=300;
		//STXFloppylist[floppyid]->floppyBitRate=250000;

		trackformat=ISOFORMAT_DD;

#ifdef PASTI_DBG
		sprintf(tempstring,"File header :");
		for(debug_i=0;debug_i<sizeof(pasti_fileheader);debug_i++)
		{
			sprintf(&tempstring[strlen(tempstring)],"%.2X ",*(((unsigned char*)&fileheader)+debug_i));
		}
		sprintf(&tempstring[strlen(tempstring)],"\n");

		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"%s",tempstring);

		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Number of track : %d (%d), Number of side: %d\n",numberoftrack,numberoftrackperside,numberofside);
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Tracks :");
#endif
		if(floppydisk->floppyNumberOfTrack)
		{
			floppydisk->tracks = (HXCFE_CYLINDER**)malloc(sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);
			if( !floppydisk->tracks )
			{
				hxc_fclose(f);
				return HXCFE_INTERNALERROR;
			}

			memset(floppydisk->tracks,0,sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);

			for(i=0;i<numberoftrack;i++)
			{
				hxcfe_imgCallProgressCallback(imgldr_ctx,i,numberoftrack);

				//lecture descripteur track
				trackheaderpos=ftell(f);
				hxc_fread( &trackheader, sizeof(trackheader), f );

				///////////////////////////// debug ///////////////////////////////
#ifdef PASTI_DBG
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"");
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"T-T-T-T-T-T-T-T-T-T-T-T-T-T-T-T-T-T-T-T-T-T-T-T-T-T-T-T-T-T-T-T-T-T-T-T-T");
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"T-T-T-T-T-T-T-T- Track Header %.3d Offset 0x%.8X -T-T-T-T-T-T-T-T-T-T",i,trackheaderpos);
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"T-T-T-T-T-T-T-T-T-T-T-T-T-T-T-T-T-T-T-T-T-T-T-T-T-T-T-T-T-T-T-T-T-T-T-T-T");
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Track %.3d",trackheader.track_code&0x7F);
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Side %.3d" ,trackheader.track_code>>7);
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Number of Sector %.3d" ,trackheader.numberofsector);
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Track Block Size : 0x%.8X" ,trackheader.track_block_size);
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Track Flags: 0x%.4X" ,trackheader.flags);

				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Track header:");
				tempstring[0]=0;
				for(debug_i=0;debug_i<sizeof(trackheader);debug_i++)
				{
					sprintf(&tempstring[strlen(tempstring)],"%.2X ",*(((unsigned char*)&trackheader)+debug_i));
				}
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"%s",tempstring);
				imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"---------------------------------------------");
#endif
				///////////////////////////// debug ///////////////////////////////

				tracknumber=trackheader.track_code&0x7F;
				sidenumber=trackheader.track_code>>7;

				trackmode=0;
				if(trackheader.flags&0x40)
				{
					trackmode=1;
				}

				weaksectortotalsize=0;
				tracksize=0;

				sectorlistoffset=ftell(f);

				switch(trackmode)
				{
					//track contenant uniquement les informations secteurs
					// -> encodage "standard"
					case 0:
#ifdef PASTI_DBG
						imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"READ SECTOR track");
#endif
						if(trackheader.numberofsector > 0)
						{
							sector = malloc(sizeof(pasti_sector)*trackheader.numberofsector);
							sectorconfig = (HXCFE_SECTCFG *) malloc(sizeof(HXCFE_SECTCFG)* trackheader.numberofsector);
							if( !sector || !sectorconfig )
								goto alloc_error;

							memset(sectorconfig,0,sizeof(HXCFE_SECTCFG)* trackheader.numberofsector);
							memset(sector,0,sizeof(pasti_sector)*trackheader.numberofsector);

							// lecture de l'ensemble des descripteurs de secteur
							for(j=0;j<(trackheader.numberofsector);j++)
							{
								// Chargement du descripteur de secteur.
								if(trackheader.flags&0x01)
								{
									fseek(f,sectorlistoffset + (sizeof(pasti_sector)*j) ,SEEK_SET);
									hxc_fread( &sector[j], sizeof(pasti_sector), f );

									sectorconfig[j].use_alternate_data_crc=0;

									if( sector[j].FDC_status & 0x08 ) // Bad Data CRC ?
									{
										sectorconfig[j].use_alternate_data_crc = 0x01;
									}

									sectorconfig[j].use_alternate_datamark=0;

									if( sector[j].FDC_status & 0x20 ) // Deleted sector ?
									{
										sectorconfig[j].use_alternate_datamark = 0xFF;
										sectorconfig[j].alternate_datamark = 0xF8;
									}

									if( sector[j].FDC_status & 0x10 ) // data block not found ?
									{
										sectorconfig[j].use_alternate_datamark = 0xFF;
										sectorconfig[j].alternate_datamark = 0x00;
										sectorconfig[j].flags |= TRACKGEN_NO_DATA;
									}

									sectorconfig[j].missingdataaddressmark=0;
									sectorconfig[j].use_alternate_addressmark=0;
									sectorconfig[j].head=sector[j].side_num;
									sectorconfig[j].cylinder=sector[j].track_num;
									sectorconfig[j].sector=sector[j].sector_num;
									sectorconfig[j].use_alternate_header_crc=0x2;
									sectorconfig[j].header_crc=sector[j].header_crc;
									sectorconfig[j].gap3=255;
									sectorconfig[j].bitrate=floppydisk->floppyBitRate;
									sectorconfig[j].trackencoding=trackformat;

									sectorconfig[j].sectorsize=0;
									sectorconfig[j].sectorsize=128<<sector[j].sector_size;


									///////////////////////////// debug ///////////////////////////////
#ifdef PASTI_DBG
									imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"------ Sector Header %.3d - Offset 0x%.8X ------",j,sectorlistoffset + (sizeof(pasti_sector)*j));
									imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Track: %.3d",sector[j].track_num);
									imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Side: %.3d" ,sector[j].side_num);
									imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Sector ID: %.3d" ,sector[j].sector_num);
									imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Sector size: 0x%.2X (%d bytes)" ,sector[j].sector_size,128<<sector[j].sector_size);
									imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Sector Flags: 0x%.2X" ,sector[j].sector_flags);
									imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Sector FDC Status: 0x%.2X" ,sector[j].FDC_status);
									imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Sector Header CRC : 0x%.4X" ,sector[j].header_crc);
									imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Sector data Offset: 0x%.8X" ,sector[j].sector_pos);
									imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Index-Sector Timing: %d" ,sector[j].sector_pos_timing);
									imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Read Sector Timing: %d" ,sector[j].sector_speed_timing);
									imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Sector header:");
									tempstring[0]=0;
									for(debug_i=0;debug_i<sizeof(pasti_sector);debug_i++)
									{
										sprintf(&tempstring[strlen(tempstring)],"%.2X",*(((unsigned char*)&sector[j])+debug_i));
										if(debug_i==3 || debug_i==5 ||  debug_i==7 || debug_i==8 || debug_i==9 || debug_i==10 || debug_i==11 || debug_i==13 || debug_i==15)
										{
											sprintf(&tempstring[strlen(tempstring)]," ");
										}
									}
									imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"%s",tempstring);
									imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"------------------------------------------------");
#endif
									///////////////////////////// debug ///////////////////////////////

									if((sector[j].FDC_status&0x80)==0x80)
									{
										weaksectortotalsize=weaksectortotalsize+sectorconfig[j].sectorsize;
									}
								}
								else
								{
									sector[j].sector_pos=(512*j);
									sectorconfig[j].use_alternate_data_crc=0;

									sectorconfig[j].missingdataaddressmark=0;
									sectorconfig[j].use_alternate_datamark=0;
									sectorconfig[j].use_alternate_addressmark=0;
									sectorconfig[j].head=trackheader.track_code>>7;
									sectorconfig[j].cylinder=trackheader.track_code&0x7F;
									sectorconfig[j].sector=j+1;
									sectorconfig[j].use_alternate_header_crc=0x0;
									sectorconfig[j].gap3=255;
									sectorconfig[j].bitrate=floppydisk->floppyBitRate;
									sectorconfig[j].trackencoding=trackformat;
									sectorconfig[j].sectorsize=512;

									///////////////////////////// debug ///////////////////////////////
#ifdef PASTI_DBG
									imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"------ Sector %.3d : No Header / unprotected -------",j);
									imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Track: %.3d",sectorconfig[j].cylinder);
									imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Side: %.3d" ,sectorconfig[j].head);
									imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Sector ID: %.3d" ,sectorconfig[j].sector);
									imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Sector size: %d bytes" ,sectorconfig[j].sectorsize);
									imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"---------------------------------------------------");
#endif
									///////////////////////////// debug ///////////////////////////////

								}
							}
						}

						// Calcul de la position des donnees
						weaksectoroffset=ftell(f);
						trackpos=ftell(f)+ weaksectortotalsize;
						fseek(f,trackpos,SEEK_SET);

						if(trackheader.flags&0x80)
						{
							hxc_fread( &temp_val, sizeof(unsigned short), f );
#ifdef PASTI_DBG
							imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Unknow value 0x%X",temp_val);
#endif
						}

						// lecture des secteurs
						for(j=0;j<trackheader.numberofsector && sector;j++)
						{
#ifdef PASTI_DBG
							imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Reading Sector data %d at: 0x%.8X",j,trackpos + sector[j].sector_pos);
#endif
							fseek(f,trackpos + sector[j].sector_pos,SEEK_SET);
							sectorconfig[j].input_data = malloc(sectorconfig[j].sectorsize);
							if(sectorconfig[j].input_data)
								hxc_fread(sectorconfig[j].input_data,sectorconfig[j].sectorsize,f);
						}

						// Allocation track
						if(!floppydisk->tracks[tracknumber])
						{
							floppydisk->tracks[tracknumber]=(HXCFE_CYLINDER*)malloc(sizeof(HXCFE_CYLINDER));
							memset(floppydisk->tracks[tracknumber],0,sizeof(HXCFE_CYLINDER));
							floppydisk->tracks[tracknumber]->floppyRPM=300;
							currentcylinder=floppydisk->tracks[tracknumber];
							currentcylinder->number_of_side=0;

							currentcylinder->sides=(HXCFE_SIDE**)malloc(sizeof(HXCFE_SIDE*)*2);
							memset(currentcylinder->sides,0,sizeof(HXCFE_SIDE*)*2);
						}

						currentcylinder->number_of_side++;
						currentcylinder->sides[sidenumber]=tg_generateTrackEx(trackheader.numberofsector,(HXCFE_SECTCFG *)sectorconfig,interleave,0,floppydisk->floppyBitRate,300,trackformat,0,2500,0);
						currentside=currentcylinder->sides[sidenumber];

						currentside->flakybitsbuffer=tg_allocsubtrack_char(currentside->tracklen,0x00);

						currentside->bitrate=VARIABLEBITRATE;
						currentside->timingbuffer=tg_allocsubtrack_long(currentside->tracklen,DEFAULT_DD_BITRATE);

						// encodage track
						trackformat=ISOFORMAT_DD;
						if(trackheader.numberofsector==11)
							trackformat=ISOFORMAT_DD11S;

						// generation flakey bits
						for(j=0;j<(trackheader.numberofsector);j++)
						{
							if((sector[j].FDC_status&0x80)==0x80)
							{
								///////////////////////////// debug ///////////////////////////////
#ifdef PASTI_DBG
								imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"----------- Flakey Buffer - Sector %.3d -----------",j);
								imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"File Offset: 0x%.8X Size : %d Bytes",weaksectoroffset,128<<sector[j].sector_size);
								imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"---------------------------------------------------");
#endif
								///////////////////////////// debug ///////////////////////////////

								weaksectorbuffer = malloc(128<<sector[j].sector_size);
								if(weaksectorbuffer)
								{
									fseek(f,weaksectoroffset,SEEK_SET);
									hxc_fread(weaksectorbuffer,128<<sector[j].sector_size, f);

									for(k=0;(k<(128<<sector[j].sector_size)*2);k=k+2)
									{
										currentside->flakybitsbuffer[(sectorconfig[j].startdataindex+k+0)%(currentside->tracklen/8)]=getchunk((unsigned char)((~weaksectorbuffer[k/2])>>4));
										currentside->flakybitsbuffer[(sectorconfig[j].startdataindex+k+1)%(currentside->tracklen/8)]=getchunk((unsigned char)(~weaksectorbuffer[k/2]));
									}

									free(weaksectorbuffer);
									weaksectorbuffer = NULL;

									weaksectoroffset=weaksectoroffset+(128<<sector[j].sector_size);
								}
							}
						}


						for(j=0;j<(trackheader.numberofsector);j++)
						{
							for(l=(sectorconfig[j].startdataindex);l<((sectorconfig[j].startdataindex)+(sectorconfig[j].sectorsize*2));l++)
							{
								if(sector[j].sector_speed_timing)
									currentside->timingbuffer[l%(currentside->tracklen/8)]=(uint32_t)(((float)(sectorconfig[j].sectorsize*8)/(float)sector[j].sector_speed_timing)*(float)1000000);
								else
									currentside->timingbuffer[l%(currentside->tracklen/8)]=floppydisk->floppyBitRate;
							}
						}

						if(trackheader.numberofsector)
						{
							for(j=0;j<trackheader.numberofsector;j++)
							{
								hxcfe_freeSectorConfigData( 0, &sectorconfig[j] );
							}

							free(sectorconfig);
							sectorconfig = NULL;

							free(sector);
							sector = NULL;
						}
					break;


					///////////////////////////////////////////////////////////////////////////////////////////////
					///////////////////////////////////////////////////////////////////////////////////////////////
					///////////////////////////////////////////////////////////////////////////////////////////////
					///////////////////////////////////track data+sectors datas////////////////////////////////////
					///////////////////////////////////////////////////////////////////////////////////////////////
					case 1:

						imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"READTRACK track");

						// allocation buffers secteurs
						if(trackheader.numberofsector > 0)
						{
							sector = malloc(sizeof(pasti_sector)*trackheader.numberofsector);
							sectorconfig = (HXCFE_SECTCFG *) malloc(sizeof(HXCFE_SECTCFG)* trackheader.numberofsector);
							if( !sector || !sectorconfig )
								goto alloc_error;

							memset(sector,0,sizeof(pasti_sector)*trackheader.numberofsector);
							memset(sectorconfig,0,sizeof(HXCFE_SECTCFG)* trackheader.numberofsector);
						}
						else
						{
							sector=0;
							sectorconfig=0;
						}

						// lecture de l'ensemble des descripteurs de secteur
						for(j=0;j<(trackheader.numberofsector);j++)
						{
							fseek(f,sectorlistoffset + (sizeof(pasti_sector)*j) ,SEEK_SET);
							hxc_fread( &sector[j], sizeof(pasti_sector), f );
						}

						// analyse configuration secteurs
						// calcul taille secteur
						for(j=0;j<(trackheader.numberofsector);j++)
						{
							sectorconfig[j].use_alternate_data_crc=0;

							sectorconfig[j].missingdataaddressmark=0;
							sectorconfig[j].head=sector[j].side_num;
							sectorconfig[j].cylinder=sector[j].track_num;
							sectorconfig[j].sector=sector[j].sector_num;
							sectorconfig[j].use_alternate_header_crc=0x2;
							sectorconfig[j].header_crc=sector[j].header_crc;

							if(sector[j].sector_size<8)
								sectorconfig[j].sectorsize=128<<sector[j].sector_size;
							else
								sectorconfig[j].sectorsize=0;

							sectorconfig[j].use_alternate_data_crc=0;

							if( sector[j].FDC_status & 0x08 ) // Bad Data CRC ?
							{
								sectorconfig[j].use_alternate_data_crc = 0x01;
							}

							sectorconfig[j].use_alternate_datamark=0;

							if( sector[j].FDC_status & 0x20 ) // Deleted sector ?
							{
								sectorconfig[j].use_alternate_datamark = 0xFF;
								sectorconfig[j].alternate_datamark = 0xF8;
							}

							if( sector[j].FDC_status & 0x10 ) // data block not found ?
							{
								sectorconfig[j].use_alternate_datamark = 0xFF;
								sectorconfig[j].alternate_datamark = 0x00;
								sectorconfig[j].flags |= TRACKGEN_NO_DATA;
							}

							sectorconfig[j].alternate_sector_size_id=sector[j].sector_size;
							tracksize=tracksize+sectorconfig[j].sectorsize;

							if((sector[j].FDC_status&0x80)==0x80)
							{
								weaksectortotalsize=weaksectortotalsize+sectorconfig[j].sectorsize;
							}

							///////////////////////////// debug ///////////////////////////////
#ifdef PASTI_DBG
							sprintf(tempstring,"Sector:%.2d Size:%.8d Cylcode:%.2d HeadCode:%d SectorCode:%.2d Size:%d",j,sectorconfig[j].sectorsize,sectorconfig[j].cylinder,sectorconfig[j].head,sectorconfig[j].sector,sectorconfig[j].sectorsize);
							sprintf(&tempstring[strlen(tempstring)]," Sector header:");
							for(debug_i=0;debug_i<sizeof(pasti_sector);debug_i++)
							{
								sprintf(&tempstring[strlen(tempstring)],"%.2X",*(((unsigned char*)&sector[j])+debug_i));
								if(debug_i==3 || debug_i==5 ||  debug_i==7 || debug_i==8 || debug_i==9 || debug_i==10 || debug_i==11 || debug_i==13 || debug_i==15)
								{
									sprintf(&tempstring[strlen(tempstring)]," ");
								}
							}
							imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"%s",tempstring);
#endif
							///////////////////////////// debug ///////////////////////////////

							///////////////////////////// debug ///////////////////////////////
#ifdef PASTI_DBG
							imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"------ Sector Header %.3d - Offset 0x%.8X ------",j,sectorlistoffset + (sizeof(pasti_sector)*j));
							imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Track: %.3d",sector[j].track_num);
							imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Side: %.3d" ,sector[j].side_num);
							imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Sector ID: %.3d" ,sector[j].sector_num);
							imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Sector size: 0x%.2X (%d bytes)" ,sector[j].sector_size,128<<sector[j].sector_size);
							imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Sector Flags: 0x%.2X" ,sector[j].sector_flags);
							imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Sector FDC Status: 0x%.2X" ,sector[j].FDC_status);
							imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Sector Header CRC : 0x%.4X" ,sector[j].header_crc);
							imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Sector data Offset: 0x%.8X" ,sector[j].sector_pos);
							imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Index-Sector Timing: %d" ,sector[j].sector_pos_timing);
							imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Read Sector Timing: %d" ,sector[j].sector_speed_timing);
							imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Sector header:");
							tempstring[0]=0;
							for(debug_i=0;debug_i<sizeof(pasti_sector);debug_i++)
							{
								sprintf(&tempstring[strlen(tempstring)],"%.2X",*(((unsigned char*)&sector[j])+debug_i));
								if(debug_i==3 || debug_i==5 ||  debug_i==7 || debug_i==8 || debug_i==9 || debug_i==10 || debug_i==11 || debug_i==13 || debug_i==15)
								{
									sprintf(&tempstring[strlen(tempstring)]," ");
								}
							}
							imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"%s",tempstring);
							imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"------------------------------------------------");
#endif
									///////////////////////////// debug ///////////////////////////////

						}

						// Calcul de la position des donnees
						weaksectoroffset=ftell(f);
						trackpos=ftell(f)+ weaksectortotalsize;
						fseek(f,trackpos,SEEK_SET);

						if(trackheader.flags&0x80)
						{
							hxc_fread( &index_sync, sizeof(unsigned short), f );
#ifdef PASTI_DBG
							imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"Index sync pos 0x%X",index_sync);
#endif
						}
						hxc_fread( &temp_val, sizeof(unsigned short), f );

						tracklen=temp_val;

#ifdef PASTI_DBG
						imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"READTRACK data size 0x%X (File offset 0x%.8X)",tracklen,ftell(f));
#endif

						temptrack=(unsigned char*)malloc(tracklen);
						tempclock=(unsigned char*)malloc(tracklen);
						tempmask=(unsigned char*)malloc(tracklen);

						if(temptrack && tempclock && tempmask)
						{
							hxc_fread( temptrack, tracklen, f );
							memset(tempclock,0xFF,tracklen);
							memset(tempmask,0xFF,tracklen);

							sectordata_index=getSectorHeaderOffset(imgldr_ctx->hxcfe,temptrack,tracklen,trackheader.numberofsector);

							lastindex=0;

							for(j=0;j<(trackheader.numberofsector);j++)
							{
								if(sectorconfig[j].sectorsize)
								{
									sectorconfig[j].input_data=malloc(sectorconfig[j].sectorsize);
									if(sectorconfig[j].input_data)
										memset(sectorconfig[j].input_data,0,sectorconfig[j].sectorsize);
								}

								fseek(f,trackpos,SEEK_SET);
								fseek(f,sector[j].sector_pos,SEEK_CUR);

								hxc_fread(sectorconfig[j].input_data,sectorconfig[j].sectorsize,f);
							}

							for(j=0;j<trackheader.numberofsector;j++)
							{
								addsector(imgldr_ctx->hxcfe,temptrack,tempclock,tempmask,tracklen,sectordata_index,j,sectorconfig);
							}

							if(!trackheader.numberofsector)
								patchtrack(imgldr_ctx->hxcfe,temptrack,tempclock,tracklen,i);

							// Second pass : CRC recalculation
							for(j=0;j<(trackheader.numberofsector);j++)
							{
								if(sectorconfig[j].startdataindex!=-1 && sectorconfig[j].sectorsize)
								{
									lastindex=(sectorconfig[j].startdataindex/2)-4;

									if(temptrack[(lastindex+3)%tracklen]==0xFB)
									{
										//02 CRC The CRC of the data
										CRC16_Init(&CRC16_High,&CRC16_Low,(unsigned char*)&crctable,0x1021,0xFFFF);

										for(t=0;t<(int)(sectorconfig[j].sectorsize+4);t++)
										{
												CRC16_Update(&CRC16_High,&CRC16_Low, temptrack[(lastindex+t)%tracklen],(unsigned char*)&crctable);
										}

										lastindex=(lastindex+sectorconfig[j].sectorsize+4)%tracklen;

										if(!((sector[j].FDC_status&0x8)==0x8))
										{
											temptrack[lastindex]=CRC16_High;
											lastindex=(lastindex+1)%tracklen;

											temptrack[lastindex]=CRC16_Low;
											lastindex=(lastindex+1)%tracklen;
										}
										else
										{
											CRC16_High=temptrack[lastindex];
											lastindex=(lastindex+1)%tracklen;

											CRC16_Low =temptrack[lastindex];
											lastindex=(lastindex+1)%tracklen;

										}

										imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG," - sectordata_index[0x%.4X]=0x%.8X , data crc result: %.2X%.2X  %d bytes",j,sectorconfig[j].startsectorindex,CRC16_High,CRC16_Low,sectorconfig[j].sectorsize+4);
									}
								}
							}

							if(!floppydisk->tracks[tracknumber])
							{
								floppydisk->tracks[tracknumber]=allocCylinderEntry(300,2);
								currentcylinder=floppydisk->tracks[tracknumber];
							}

							currentcylinder->sides[sidenumber]=tg_alloctrack(DEFAULT_DD_BITRATE,ISOIBM_MFM_ENCODING,300,tracklen*2*8,2500,0,TG_ALLOCTRACK_ALLOCTIMIMGBUFFER|TG_ALLOCTRACK_ALLOCFLAKEYBUFFER);
							currentside=currentcylinder->sides[sidenumber];

							if(trackheader.flags&0x80)
							{
								if(index_sync<tracklen)
								{
									tempclock[index_sync]=0x14;
									temptrack[index_sync]=0xc2;
								}
							}

							BuildMFMCylinder(currentside->databuffer,
								currentside->tracklen/8,
								tempclock,
								temptrack,
								tracklen);

							//  flakey bits generation
							for(j=0;j<(trackheader.numberofsector);j++)
							{
								if((sector[j].FDC_status&0x80)==0x80)
								{
									///////////////////////////// debug ///////////////////////////////
	#ifdef PASTI_DBG
									imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"----------- Flakey Buffer - Sector %.3d -----------",j);
									imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"File Offset: 0x%.8X Size : %d Bytes",weaksectoroffset,128<<sector[j].sector_size);
									imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"---------------------------------------------------");
	#endif
									///////////////////////////// debug ///////////////////////////////

									weaksectorbuffer=malloc(128<<sector[j].sector_size);
									if(weaksectorbuffer)
									{
										fseek(f,weaksectoroffset,SEEK_SET);
										hxc_fread(weaksectorbuffer,128<<sector[j].sector_size,f);

										for(k=0;(k<(128<<sector[j].sector_size)*2);k=k+2)
										{
											currentside->flakybitsbuffer[((sectorconfig[j].startdataindex)+k+0)%(currentside->tracklen/8)]=getchunk((unsigned char)((~weaksectorbuffer[k/2])>>4));
											currentside->flakybitsbuffer[((sectorconfig[j].startdataindex)+k+1)%(currentside->tracklen/8)]=getchunk((unsigned char)(~weaksectorbuffer[k/2]));
										}

										free(weaksectorbuffer);
										weaksectorbuffer = NULL;
										weaksectoroffset += (128<<sector[j].sector_size);
									}
								}
							}


							for(j=0;j<(trackheader.numberofsector);j++)
							{
								for(l=(sectorconfig[j].startdataindex);l<((sectorconfig[j].startdataindex)+(sectorconfig[j].sectorsize*2));l++)
								{
									if(sector[j].sector_speed_timing)
										currentside->timingbuffer[l%(currentside->tracklen/8)]=(uint32_t)(((float)(sectorconfig[j].sectorsize*8)/(float)sector[j].sector_speed_timing)*(float)1000000);
									else
										currentside->timingbuffer[l%(currentside->tracklen/8)]=250000;
								}
							}

							free(tempclock);
							tempclock = NULL;

							free(temptrack);
							temptrack = NULL;

							free(tempmask);
							tempmask = NULL;

							if(trackheader.numberofsector)
							{
								for(j=0;j<trackheader.numberofsector;j++)
								{
									hxcfe_freeSectorConfigData( 0, &sectorconfig[j] );
								}
							}

							free(sector);
							sector = NULL;

							free(sectorconfig);
							sectorconfig = NULL;

							free(sectordata_index);
							sectordata_index = NULL;
						}
						else
						{
							goto alloc_error;
						}

					break;
				}

				fseek(f,trackheaderpos,SEEK_SET);
				fseek(f,trackheader.track_block_size,SEEK_CUR);
			}
		}

		floppydisk->floppyNumberOfSide=1;
		for(i=0;i<floppydisk->floppyNumberOfTrack;i++)
		{
			if(floppydisk->tracks[i])
			{
				if(floppydisk->tracks[i]->number_of_side==2)
				{
					floppydisk->floppyNumberOfSide=2;
				}
			}
		}

		for(i=0;i<floppydisk->floppyNumberOfTrack;i++)
		{
			if(floppydisk->tracks[i])
			{
				floppydisk->tracks[i]->number_of_side=floppydisk->tracks[i]->number_of_side;
			}
		}

		for(i=0;i<floppydisk->floppyNumberOfTrack;i++)
		{
			if(floppydisk->tracks[i])
			{
				if(!floppydisk->tracks[i]->sides[0])
				{
					floppydisk->tracks[i]->sides[0]=tg_generateTrack(0,512,0 ,(unsigned char)i,(unsigned char)0,1,interleave,(unsigned char)(0),250000,currentcylinder->floppyRPM,ISOFORMAT_DD,255,0,2500| NO_SECTOR_UNDER_INDEX,0);
				}

				if( floppydisk->floppyNumberOfSide == 2 )
				{
					if(!floppydisk->tracks[i]->sides[1])
					{
						floppydisk->tracks[i]->sides[1]=tg_generateTrack(0,512,0 ,(unsigned char)i,(unsigned char)1,1,interleave,(unsigned char)(0),250000,currentcylinder->floppyRPM,ISOFORMAT_DD,255,0,2500| NO_SECTOR_UNDER_INDEX,0);
					}
				}
			}
			else
			{
				floppydisk->tracks[i]=(HXCFE_CYLINDER*)malloc(sizeof(HXCFE_CYLINDER));
				memset(floppydisk->tracks[i],0,sizeof(HXCFE_CYLINDER));
				floppydisk->tracks[i]->floppyRPM=300;

				currentcylinder=floppydisk->tracks[i];
				currentcylinder->number_of_side=floppydisk->floppyNumberOfSide;

				currentcylinder->sides=(HXCFE_SIDE**)malloc(sizeof(HXCFE_SIDE*)*2);
				memset(currentcylinder->sides,0,sizeof(HXCFE_SIDE*)*2);
				floppydisk->tracks[i]->sides[0]=tg_generateTrack(0,512,0 ,(unsigned char)i,(unsigned char)0,1,interleave,(unsigned char)(0),250000,currentcylinder->floppyRPM,ISOFORMAT_DD,255,0,2500| NO_SECTOR_UNDER_INDEX,0);

				if(floppydisk->tracks[i]->number_of_side==2)
				{
					floppydisk->tracks[i]->sides[1]=tg_generateTrack(0,512,0 ,(unsigned char)i,(unsigned char)1,1,interleave,(unsigned char)(0),250000,currentcylinder->floppyRPM,ISOFORMAT_DD,255,0,2500| NO_SECTOR_UNDER_INDEX,0);
				}
			}

		}

		hxcfe_sanityCheck(imgldr_ctx->hxcfe,floppydisk);

		hxc_fclose(f);

		imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_0,"Pasti image loading done!");

		return HXCFE_NOERROR;
	}
	else
	{
		hxc_fclose(f);

		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"non STX/pasti image (bad header)",imgfile);
		return HXCFE_BADFILE;
	}

alloc_error:
	hxc_fclose(f);

	free(sector);
	free(sectorconfig);
	free(sectordata_index);
	free(temptrack);
	free(tempclock);
	free(tempmask);
	free(weaksectorbuffer);

	hxcfe_freeFloppy(imgldr_ctx->hxcfe, floppydisk );

	return HXCFE_INTERNALERROR;
}

int STX_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{
	static const char plug_id[]="ATARIST_STX";
	static const char plug_desc[]="Atari ST STX/Pasti Loader";
	static const char plug_ext[]="stx";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)   STX_libIsValidDiskFile,
		(LOADDISKFILE)      STX_libLoad_DiskFile,
		(WRITEDISKFILE)     STX_libWrite_DiskFile,
		(GETPLUGININFOS)    STX_libGetPluginInfo
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
