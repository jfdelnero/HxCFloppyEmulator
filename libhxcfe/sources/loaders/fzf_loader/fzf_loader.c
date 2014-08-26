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
// File : fzf_loader.c
// Contains: Casio FZF floppy image loader
//
// Written by:	DEL NERO Jean Francois
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "types.h"
#include "libhxcfe.h"

#include "floppy_loader.h"
#include "floppy_utils.h"

#include "fzf_loader.h"

#include "libhxcadaptor.h"

char * fzffiletype[]={
	"full",
	"bank",
	"voice",
	"effects"
};


int FZF_libIsValidDiskFile(HXCFLOPPYEMULATOR* floppycontext,char * imgfile)
{
	int filesize;

	floppycontext->hxc_printf(MSG_DEBUG,"FZF_libIsValidDiskFile");

	if( hxc_checkfileext(imgfile,"fzf") )
	{

		filesize=hxc_getfilesize(imgfile);
		if(filesize<0)
		{
			floppycontext->hxc_printf(MSG_ERROR,"FZF_libIsValidDiskFile : Cannot open %s !",imgfile);
			return HXCFE_ACCESSERROR;
		}

		floppycontext->hxc_printf(MSG_DEBUG,"FZF_libIsValidDiskFile : FZF file !");
		return HXCFE_VALIDFILE;
	}
	else
	{
		floppycontext->hxc_printf(MSG_DEBUG,"FZF_libIsValidDiskFile : non FZF file !");
		return HXCFE_BADFILE;
	}

	return HXCFE_BADPARAMETER;
}

int FZF_libLoad_DiskFile(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	FILE * f;
	unsigned int  filesize;
	unsigned int  i,j;
	unsigned int  file_offset;
	unsigned char * fzf_file;
	unsigned char  gap3len,interleave,trackformat,skew,c;
	unsigned short rpm;
	unsigned short sectorsize;
	CYLINDER* currentcylinder;

	unsigned char  number_of_banks;
	unsigned char  number_of_voices;
	unsigned short pcm_size;
	unsigned char  file_type;
	unsigned char* floppy_data;

	unsigned int  nbblock;
	char filename[512];

	floppycontext->hxc_printf(MSG_DEBUG,"FZF_libLoad_DiskFile %s",imgfile);

	f=hxc_fopen(imgfile,"rb");
	if(f==NULL)
	{
		floppycontext->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	fseek (f , 0 , SEEK_END);
	filesize=ftell(f);
	fseek (f , 0 , SEEK_SET);

	nbblock = filesize / 1024;
	if(filesize & 0x3FF ) nbblock++;

	fzf_file = malloc ( nbblock * 1024 );
	if(!fzf_file)
	{
		hxc_fclose(f);
		return HXCFE_INTERNALERROR;
	}

	memset(fzf_file,0,nbblock * 1024);
	fread(fzf_file,1,filesize,f);

	hxc_fclose(f);

	floppydisk->floppyNumberOfTrack=80;
	floppydisk->floppyNumberOfSide=2;
	floppydisk->floppySectorPerTrack=8;
	floppydisk->floppyBitRate=500000;
	floppydisk->floppyiftype=GENERIC_SHUGART_DD_FLOPPYMODE;
	sectorsize = 1024;
	rpm=360;
	gap3len=255;
	interleave=3;
	skew=0;
	trackformat=IBMFORMAT_DD;

	floppy_data = malloc(floppydisk->floppyNumberOfTrack * floppydisk->floppyNumberOfSide * floppydisk->floppySectorPerTrack * sectorsize);
	if(!floppy_data)
	{
		free(fzf_file);
		return HXCFE_INTERNALERROR;
	}
	memset(floppy_data , 0x5A , floppydisk->floppyNumberOfTrack * floppydisk->floppyNumberOfSide * floppydisk->floppySectorPerTrack * sectorsize);

	if (nbblock>1279)
	{
		nbblock=1276;
	}

	file_type = fzf_file[ 0x3e8 + 5 ];
	number_of_banks = fzf_file[ 0x3e8 + 7 ];
	number_of_voices = fzf_file[ 0x3e8 + 8 ];
	pcm_size = (fzf_file[ 0x3e8 + 0x0d ]<<8) | fzf_file[ 0x3e8 + 0x0c ];

	for( i = 0; i  < nbblock; i++)
	{
		memcpy(&floppy_data[(i+4)*sectorsize],&fzf_file[i*sectorsize],sectorsize);
	}

	// Fragment list - Sector 3
	memset(&floppy_data[(sectorsize*3)+0],0,sectorsize);
	floppy_data[(sectorsize*3)+0]=3; // Sector
	floppy_data[(sectorsize*3)+2]=(3+nbblock)&0xff;
	floppy_data[(sectorsize*3)+3]=(3+nbblock)>>8;
	floppy_data[(sectorsize*3)+1018]=number_of_banks;
	floppy_data[(sectorsize*3)+1020]=number_of_voices;
	floppy_data[(sectorsize*3)+1022]=(pcm_size&0xff);
	floppy_data[(sectorsize*3)+1023]=(pcm_size>>8)&0xff;

	// Directory - Sector 2
	memset(&floppy_data[(sectorsize*2)+0],0,sectorsize);

	// Sector 1
	memset(&floppy_data[(sectorsize*1)+0],0,sectorsize);
	memcpy(&floppy_data[(sectorsize*1)+0],"FULL-DATA-FZ",12);
	floppy_data[(sectorsize*1)+12] = file_type;
	floppy_data[(sectorsize*1)+13] = 0;
	floppy_data[(sectorsize*1)+14] = 3;
	floppy_data[(sectorsize*1)+15] = 0;

	// BAM - Sector 0
	memset(&floppy_data[(sectorsize*0)+0],0,sectorsize);
	memset(&floppy_data[(sectorsize*0)+288],0xFF,736);

	for(i=0;i<nbblock+3;i+=8)
	{
		for(j=0;j<8;j++)
		{
			if((i>>3)*8+j<nbblock+3)
			{
				floppy_data[ 0x80+ (i>>3) ] = floppy_data[ 0x80 + (i>>3) ] | (1<<j);
			}
		}
	}

	hxc_getfilenamewext(imgfile,filename);

	//Disk name
	for(i=0;i<12;i++)
	{

		if (i>strlen(filename))
		{
			c=0x20;
		}
		else
		{
			if ( (c=filename[i])<0x20) c=0x20;
		}

		floppy_data[i]=c;
		floppy_data[i+0x10]=c;
	}
	floppy_data[0x0e]=2;

	//////////////////////////////////////////////////////////////

	floppydisk->tracks=(CYLINDER**)malloc(sizeof(CYLINDER*)*floppydisk->floppyNumberOfTrack);

	floppycontext->hxc_printf(MSG_INFO_1,"filesize:%dB (%d blocks), %d tracks, %d side(s), %d sectors/track, gap3:%d, interleave:%d,rpm:%d",filesize,nbblock,floppydisk->floppyNumberOfTrack,floppydisk->floppyNumberOfSide,floppydisk->floppySectorPerTrack,gap3len,interleave,rpm);
	floppycontext->hxc_printf(MSG_INFO_1,"File type : %s (%d), Number of banks : %d, Number of voices : %d, PCM size : %d",fzffiletype[file_type&3],file_type,number_of_banks,number_of_voices,pcm_size);

	for(j=0;j<floppydisk->floppyNumberOfTrack;j++)
	{

		floppydisk->tracks[j]=allocCylinderEntry(rpm,floppydisk->floppyNumberOfSide);
		currentcylinder=floppydisk->tracks[j];

		for(i=0;i<floppydisk->floppyNumberOfSide;i++)
		{
			file_offset=(sectorsize*(j*floppydisk->floppySectorPerTrack*floppydisk->floppyNumberOfSide))+
				        (sectorsize*(floppydisk->floppySectorPerTrack)*i);
			currentcylinder->sides[i]=tg_generateTrack(&floppy_data[file_offset],sectorsize,floppydisk->floppySectorPerTrack,(unsigned char)j,(unsigned char)i,1,interleave,(unsigned char)(((j<<1)|(i&1))*skew),floppydisk->floppyBitRate,currentcylinder->floppyRPM,trackformat,gap3len,200,2500,-2500);
		}
	}

	floppycontext->hxc_printf(MSG_INFO_1,"track file successfully loaded and encoded!");

	free(floppy_data);
	free(fzf_file);

	return HXCFE_NOERROR;
}

int FZF_libGetPluginInfo(HXCFLOPPYEMULATOR* floppycontext,unsigned long infotype,void * returnvalue)
{

	static const char plug_id[]="CASIO_FZF";
	static const char plug_desc[]="Casio FZF file Loader";
	static const char plug_ext[]="fzf";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)	FZF_libIsValidDiskFile,
		(LOADDISKFILE)		FZF_libLoad_DiskFile,
		(WRITEDISKFILE)		0,
		(GETPLUGININFOS)	FZF_libGetPluginInfo
	};

	return libGetPluginInfo(
			floppycontext,
			infotype,
			returnvalue,
			plug_id,
			plug_desc,
			&plug_funcs,
			plug_ext
			);
}
