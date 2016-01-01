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
// File : cpcdsk_loader.c
// Contains: CPCDSK floppy image loader
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
#include "tracks/track_generator.h"
#include "libhxcfe.h"

#include "floppy_loader.h"
#include "floppy_utils.h"

#include "cpcdsk_loader.h"
#include "cpcdsk_format.h"

#include "libhxcadaptor.h"

int CPCDSK_libIsValidDiskFile(HXCFE_IMGLDR * imgldr_ctx,char * imgfile)
{
	cpcdsk_fileheader fileheader;
	FILE * f;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"CPCDSK_libIsValidDiskFile");

	if(hxc_checkfileext(imgfile,"dsk"))
	{

		f=hxc_fopen(imgfile,"rb");
		if(f==NULL)
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"CPCDSK_libIsValidDiskFile : Cannot open %s !",imgfile);
			return HXCFE_ACCESSERROR;
		}

		fread(&fileheader,sizeof(fileheader),1,f);
		hxc_fclose(f);

		if( !strncmp((char*)&fileheader.headertag,"EXTENDED CPC DSK File\r\nDisk-Info\r\n",16) ||
			!strncmp((char*)&fileheader.headertag,"MV - CPCEMU Disk-File\r\nDisk-Info\r\n",11) ||
			!strncmp((char*)&fileheader.headertag,"MV - CPC",8)
			)
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"CPCDSK_libIsValidDiskFile : CPC Dsk file !");
			return HXCFE_VALIDFILE;
		}
		else
		{
			imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"CPCDSK_libIsValidDiskFile : non CPC Dsk file !(bad header)");
			return HXCFE_BADFILE;
		}
	}
	else
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"CPCDSK_libIsValidDiskFile : non CPC Dsk file !");
		return HXCFE_BADFILE;
	}
}



int CPCDSK_libLoad_DiskFile(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk,char * imgfile,void * parameters)
{

	FILE * f;
	unsigned int filesize;
	int tracksize;
	int j,t,s,i;
	int sectorposition;
	int tracklen;
	int gap3len,interleave;
	int rpm;
	int extendformat;
	cpcdsk_fileheader fileheader;
	cpcdsk_trackheader trackheader;
	cpcdsk_sector sector;
	unsigned char * tracksizetab;
	int trackposition;
	HXCFE_SECTCFG* sectorconfig;
	HXCFE_CYLINDER** realloctracktable;

	HXCFE_CYLINDER* currentcylinder;
	HXCFE_SIDE* currentside;

	imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"CPCDSK_libLoad_DiskFile %s",imgfile);

	f=hxc_fopen(imgfile,"rb");
	if(f==NULL)
	{
		imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	fseek (f , 0 , SEEK_END);
	filesize=ftell(f);
	fseek (f , 0 , SEEK_SET);

	if(filesize!=0)
	{

		fread(&fileheader,sizeof(fileheader),1,f);

		if( !strncmp((char*)&fileheader.headertag,"EXTENDED CPC DSK File\r\nDisk-Info\r\n",16))
		{
			extendformat=1;
			imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"Extended CPC Dsk file\n");
		}
		else
		{
			if(	!strncmp((char*)&fileheader.headertag,"MV - CPC",8))
			{
				extendformat=0;
				imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"CPC Dsk standard file\n");
			}
			else
			{
				imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"non CPC Dsk : Bad header!\n");
				hxc_fclose(f);
				return HXCFE_BADFILE;
			}
		}
		tracksizetab=0;

		if(extendformat)
		{
			// read the tracks size array.
			tracksizetab=(unsigned char *)malloc(fileheader.number_of_sides*fileheader.number_of_tracks);
			fread(tracksizetab,fileheader.number_of_sides*fileheader.number_of_tracks,1,f);
		}

		floppydisk->floppyBitRate=250000;
		floppydisk->floppyiftype=CPC_DD_FLOPPYMODE;
		floppydisk->floppyNumberOfTrack=fileheader.number_of_tracks;
		floppydisk->floppyNumberOfSide=fileheader.number_of_sides;
		floppydisk->floppySectorPerTrack=9; // default value
		tracksize=fileheader.size_of_a_track;
		rpm=300;

		sectorconfig=0;
		interleave=1;

		floppydisk->tracks=(HXCFE_CYLINDER**)malloc(sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);
		memset(floppydisk->tracks,0,sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);

		imgldr_ctx->hxcfe->hxc_printf(MSG_INFO_1,"%d tracks, %d Side(s)\n",floppydisk->floppyNumberOfTrack,floppydisk->floppyNumberOfSide);

		trackposition=0x100;

		for(i=0;i<(fileheader.number_of_sides*fileheader.number_of_tracks);i++)
		{

			hxcfe_imgCallProgressCallback(imgldr_ctx,i,fileheader.number_of_sides*fileheader.number_of_tracks);

			fseek (f , trackposition , SEEK_SET);

			memset(&trackheader,0,sizeof(trackheader));

			if( ( ftell(f) == (int32_t)trackposition ) && (fread(&trackheader,sizeof(trackheader),1,f) ==  1) )
			{

				t=trackheader.track_number;
				s=trackheader.side_number;

				if(!strncmp((char*)&trackheader.headertag,"Track-Info\r\n",10) && (t<fileheader.number_of_tracks))//12))
				{
					if(extendformat)
					{
						tracksize=tracksizetab[i]*256;
					}

					if(tracksize)
					{
						// description d'une track au delà du nombre de track max !
						// realloc necessaire
						if(trackheader.track_number>=floppydisk->floppyNumberOfTrack)
						{
							floppydisk->floppyNumberOfTrack=trackheader.track_number+1;
							realloctracktable=(HXCFE_CYLINDER**)malloc(sizeof(HXCFE_CYLINDER*)*trackheader.track_number);
							memset(realloctracktable,0,sizeof(HXCFE_CYLINDER*)*trackheader.track_number);
							memcpy(realloctracktable,floppydisk->tracks,sizeof(HXCFE_CYLINDER*)*floppydisk->floppyNumberOfTrack);
							free(floppydisk->tracks);
							floppydisk->tracks=realloctracktable;
						}


						if(!floppydisk->tracks[t])
						{
							floppydisk->tracks[t]=allocCylinderEntry(rpm,floppydisk->floppyNumberOfSide);
							currentcylinder=floppydisk->tracks[t];
						}


						imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"\nnumber %d - track:%d side:%d sector:%d sc:%d gap3:%d fill:%x recmode:%d bitrate:%d\n ",i,trackheader.track_number,trackheader.side_number,trackheader.number_of_sector,trackheader.sector_size_code,trackheader.gap3_length,trackheader.filler_byte,trackheader.rec_mode,trackheader.datarate);

						if(trackheader.number_of_sector)
						{
							sectorconfig=(HXCFE_SECTCFG*)malloc(sizeof(HXCFE_SECTCFG)*trackheader.number_of_sector);
							memset(sectorconfig,0,sizeof(HXCFE_SECTCFG)*trackheader.number_of_sector);


							gap3len=trackheader.gap3_length;
							sectorconfig->sectorsize=trackheader.sector_size_code;

							sectorposition=0;
							for(j=0;j<trackheader.number_of_sector;j++)
							{
								fseek (f,trackposition+sizeof(trackheader)+(sizeof(sector)*j), SEEK_SET);
								fread(&sector,sizeof(sector),1,f);

								sectorconfig[j].cylinder=sector.track;
								sectorconfig[j].head=sector.side;
								sectorconfig[j].sector=sector.sector_id;
								sectorconfig[j].sectorsize=128<<(sector.sector_size_code&7);
								sectorconfig[j].input_data=malloc(sectorconfig[j].sectorsize);

								fseek (f, trackposition+0x100+sectorposition/*sizeof(trackheader)+(sizeof(sector)*j)*/, SEEK_SET);
								fread(sectorconfig[j].input_data,sectorconfig[j].sectorsize,1,f);

								sectorposition=sectorposition+sectorconfig[j].sectorsize;

								// ID part CRC ERROR ?
								if((sector.fdc_status_reg1&0x20) && !(sector.fdc_status_reg2&0x20))
								{
									sectorconfig[j].use_alternate_header_crc=0x1;
								}

								// Data part CRC ERROR ?
								if((sector.fdc_status_reg1&0x20) &&  (sector.fdc_status_reg2&0x20))
								{
									sectorconfig[j].use_alternate_data_crc=0x1;
								}

								// Deleted Data Address Mark ?
								if(sector.fdc_status_reg2&0x40)
								{
									sectorconfig[j].use_alternate_datamark=1;
									sectorconfig[j].alternate_datamark=0xF8;
								}

								sectorconfig[j].bitrate=floppydisk->floppyBitRate;
								sectorconfig[j].gap3=gap3len;
								sectorconfig[j].trackencoding=IBMFORMAT_DD;

								imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"%d:%d track id:%d side id:%d sector id %d sector size (id):%d sector size :%d bad crc:%d sreg1:%x sreg2:%x",trackheader.track_number,trackheader.side_number,sector.track,sector.side,sector.sector_id,sectorconfig[j].sectorsize,sector.data_length,sectorconfig[j].use_alternate_data_crc,sector.fdc_status_reg1,sector.fdc_status_reg2);
							}
						}

						//
						currentcylinder->floppyRPM=rpm;
						currentcylinder->sides[s]=tg_generateTrackEx(trackheader.number_of_sector,sectorconfig,interleave,0,floppydisk->floppyBitRate,300,IBMFORMAT_DD,0,2500|NO_SECTOR_UNDER_INDEX,-2500);

						for(j=0;j<trackheader.number_of_sector;j++)
						{
							if(sectorconfig[j].input_data)
								free(sectorconfig[j].input_data);
						}

						currentside=currentcylinder->sides[s];


						if(trackheader.number_of_sector)
						{
							free(sectorconfig);
							sectorconfig=0;
						}

						///////////////////////
						/*	for(j=0;j<trackheader.number_of_sector;j++)
						{

						  if(sectorconfig[j].baddatacrc)
						  {

							if(!currentside->flakybitsbuffer)
							currentside->flakybitsbuffer=malloc(currentside->tracklen);

							  for(k=0;k<16;k++)
							  {
							  currentside->databuffer[sectorconfig[j].startdataindex+64+k]=0;
							  currentside->flakybitsbuffer[sectorconfig[j].startdataindex+64+k]=0xFF;
							  }


							}
						}*/
						///////////////////////////

						trackposition=trackposition+tracksize;
					}
					else
					{

						imgldr_ctx->hxcfe->hxc_printf(MSG_DEBUG,"\nnnumber %d - empty !\n ",i);

						if(!floppydisk->tracks[t])
						{
							floppydisk->tracks[t]=allocCylinderEntry(rpm,floppydisk->floppyNumberOfSide);
						}

						currentcylinder = floppydisk->tracks[t];

						tracklen=((DEFAULT_DD_BITRATE/(rpm/60))/4)*8;
						currentcylinder->sides[s] = tg_alloctrack(floppydisk->floppyBitRate,ISOIBM_MFM_ENCODING,currentcylinder->floppyRPM,tracklen,2500,-2500,TG_ALLOCTRACK_RANDOMIZEDATABUFFER);
					}
				}
				else
				{

					imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"bad track header !\n");
				}
			}
			else
			{
				imgldr_ctx->hxcfe->hxc_printf(MSG_WARNING,"EOF ! Missing track(s) ?\n");
			}

		}

		// initialisation des tracks non formatées / non présente dans le fichier.
		for(j=0;j<floppydisk->floppyNumberOfTrack;j++)
		{
			for(i=0;i<floppydisk->floppyNumberOfSide;i++)
			{
				if(!floppydisk->tracks[j])
				{
					floppydisk->tracks[j]=allocCylinderEntry(rpm,floppydisk->floppyNumberOfSide);
				}

				currentcylinder = floppydisk->tracks[j];

				if(!floppydisk->tracks[j]->sides[i])
				{
					tracklen=((DEFAULT_DD_BITRATE/(rpm/60))/4)*8;
					floppydisk->tracks[j]->sides[i]=tg_alloctrack(floppydisk->floppyBitRate,ISOIBM_MFM_ENCODING,currentcylinder->floppyRPM,tracklen,2500,-2500,TG_ALLOCTRACK_ALLOCFLAKEYBUFFER|TG_ALLOCTRACK_RANDOMIZEDATABUFFER|TG_ALLOCTRACK_UNFORMATEDBUFFER);
				}
			}
		}

		if(tracksizetab) free(tracksizetab);
		//if(sectorconfig) free(sectorconfig);

		hxc_fclose(f);

		hxcfe_sanityCheck(imgldr_ctx->hxcfe,floppydisk);

		return HXCFE_NOERROR;
	}

	imgldr_ctx->hxcfe->hxc_printf(MSG_ERROR,"file size=%d !?",filesize);
	hxc_fclose(f);
	return HXCFE_BADFILE;
}

int CPCDSK_libWrite_DiskFile(HXCFE_IMGLDR* imgldr_ctx,HXCFE_FLOPPY * floppy,char * filename);

int CPCDSK_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue)
{

	static const char plug_id[]="AMSTRADCPC_DSK";
	static const char plug_desc[]="Amstrad CPC DSK Loader";
	static const char plug_ext[]="dsk";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)	CPCDSK_libIsValidDiskFile,
		(LOADDISKFILE)		CPCDSK_libLoad_DiskFile,
		(WRITEDISKFILE)		CPCDSK_libWrite_DiskFile,
		(GETPLUGININFOS)	CPCDSK_libGetPluginInfo
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

