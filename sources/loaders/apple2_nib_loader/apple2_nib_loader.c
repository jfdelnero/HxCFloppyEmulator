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
// File : apple2_nib_loader.c
// Contains: Apple 2 nib floppy image loader
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

#include "apple2_nib_loader.h"

#include "libhxcadaptor.h"

#define NIB_TRACK_SIZE 6656

//#define HDDD_A2_SUPPORT 1

unsigned short ext_a2_bit_expander[]=
{
	0x0000,0x0002,0x0008,0x000a,0x0020,0x0022,0x0028,0x002a,
	0x0080,0x0082,0x0088,0x008a,0x00a0,0x00a2,0x00a8,0x00aa,
	0x0200,0x0202,0x0208,0x020a,0x0220,0x0222,0x0228,0x022a,
	0x0280,0x0282,0x0288,0x028a,0x02a0,0x02a2,0x02a8,0x02aa,
	0x0800,0x0802,0x0808,0x080a,0x0820,0x0822,0x0828,0x082a,
	0x0880,0x0882,0x0888,0x088a,0x08a0,0x08a2,0x08a8,0x08aa,
	0x0a00,0x0a02,0x0a08,0x0a0a,0x0a20,0x0a22,0x0a28,0x0a2a,
	0x0a80,0x0a82,0x0a88,0x0a8a,0x0aa0,0x0aa2,0x0aa8,0x0aaa,
	0x2000,0x2002,0x2008,0x200a,0x2020,0x2022,0x2028,0x202a,
	0x2080,0x2082,0x2088,0x208a,0x20a0,0x20a2,0x20a8,0x20aa,
	0x2200,0x2202,0x2208,0x220a,0x2220,0x2222,0x2228,0x222a,
	0x2280,0x2282,0x2288,0x228a,0x22a0,0x22a2,0x22a8,0x22aa,
	0x2800,0x2802,0x2808,0x280a,0x2820,0x2822,0x2828,0x282a,
	0x2880,0x2882,0x2888,0x288a,0x28a0,0x28a2,0x28a8,0x28aa,
	0x2a00,0x2a02,0x2a08,0x2a0a,0x2a20,0x2a22,0x2a28,0x2a2a,
	0x2a80,0x2a82,0x2a88,0x2a8a,0x2aa0,0x2aa2,0x2aa8,0x2aaa,
	0x8000,0x8002,0x8008,0x800a,0x8020,0x8022,0x8028,0x802a,
	0x8080,0x8082,0x8088,0x808a,0x80a0,0x80a2,0x80a8,0x80aa,
	0x8200,0x8202,0x8208,0x820a,0x8220,0x8222,0x8228,0x822a,
	0x8280,0x8282,0x8288,0x828a,0x82a0,0x82a2,0x82a8,0x82aa,
	0x8800,0x8802,0x8808,0x880a,0x8820,0x8822,0x8828,0x882a,
	0x8880,0x8882,0x8888,0x888a,0x88a0,0x88a2,0x88a8,0x88aa,
	0x8a00,0x8a02,0x8a08,0x8a0a,0x8a20,0x8a22,0x8a28,0x8a2a,
	0x8a80,0x8a82,0x8a88,0x8a8a,0x8aa0,0x8aa2,0x8aa8,0x8aaa,
	0xa000,0xa002,0xa008,0xa00a,0xa020,0xa022,0xa028,0xa02a,
	0xa080,0xa082,0xa088,0xa08a,0xa0a0,0xa0a2,0xa0a8,0xa0aa,
	0xa200,0xa202,0xa208,0xa20a,0xa220,0xa222,0xa228,0xa22a,
	0xa280,0xa282,0xa288,0xa28a,0xa2a0,0xa2a2,0xa2a8,0xa2aa,
	0xa800,0xa802,0xa808,0xa80a,0xa820,0xa822,0xa828,0xa82a,
	0xa880,0xa882,0xa888,0xa88a,0xa8a0,0xa8a2,0xa8a8,0xa8aa,
	0xaa00,0xaa02,0xaa08,0xaa0a,0xaa20,0xaa22,0xaa28,0xaa2a,
	0xaa80,0xaa82,0xaa88,0xaa8a,0xaaa0,0xaaa2,0xaaa8,0xaaaa
};

int Apple2_nib_libIsValidDiskFile(HXCFLOPPYEMULATOR* floppycontext,char * imgfile)
{
	int filesize;

	floppycontext->hxc_printf(MSG_DEBUG,"Apple2_nib_libIsValidDiskFile");

	if( hxc_checkfileext(imgfile,"nib") )
	{
		filesize=hxc_getfilesize(imgfile);
		if(filesize<0)
		{
			floppycontext->hxc_printf(MSG_ERROR,"Apple2_nib_libIsValidDiskFile : Cannot open %s !",imgfile);
			return HXCFE_ACCESSERROR;
		}

		if(filesize != (NIB_TRACK_SIZE*35) )
		{
			floppycontext->hxc_printf(MSG_DEBUG,"Apple2_nib_libIsValidDiskFile : non Apple 2 NIB file - bad file size !");
			return HXCFE_BADFILE;
		}

		floppycontext->hxc_printf(MSG_DEBUG,"Apple2_nib_libIsValidDiskFile : Apple 2 NIB file !");
		return HXCFE_VALIDFILE;
	}
	else
	{
		floppycontext->hxc_printf(MSG_DEBUG,"Apple2_nib_libIsValidDiskFile : non Apple 2 NIB file !");
		return HXCFE_BADFILE;
	}

	return HXCFE_BADPARAMETER;
}

int Apple2_nib_libLoad_DiskFile(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * floppydisk,char * imgfile,void * parameters)
{
	FILE * f;
	unsigned int filesize;
	unsigned int i,j;
	unsigned int file_offset;
	char* trackdata;
	unsigned char  trackformat;
	unsigned short rpm;
	CYLINDER* currentcylinder;

	unsigned short pulses;

#ifdef HDDD_A2_SUPPORT
	unsigned short fm_pulses;
#endif
	unsigned char  data_nibble;

	floppycontext->hxc_printf(MSG_DEBUG,"Apple2_nib_libLoad_DiskFile %s",imgfile);

	f=hxc_fopen(imgfile,"rb");
	if(f==NULL)
	{
		floppycontext->hxc_printf(MSG_ERROR,"Cannot open %s !",imgfile);
		return HXCFE_ACCESSERROR;
	}

	fseek (f , 0 , SEEK_END);
	filesize=ftell(f);
	fseek (f , 0 , SEEK_SET);

	if(filesize!=0)
	{
		trackformat=ISOFORMAT_DD;

		floppydisk->floppyNumberOfSide=2;
		floppydisk->floppySectorPerTrack=-1;
		floppydisk->floppyNumberOfTrack= filesize / NIB_TRACK_SIZE;

		floppydisk->floppyBitRate=250000;
#ifdef HDDD_A2_SUPPORT
		floppydisk->floppyBitRate = floppydisk->floppyBitRate * 2;
#endif
		floppydisk->floppyiftype=GENERIC_SHUGART_DD_FLOPPYMODE;
		floppydisk->tracks=(CYLINDER**)malloc(sizeof(CYLINDER*)*floppydisk->floppyNumberOfTrack);

		rpm=300;

		floppycontext->hxc_printf(MSG_INFO_1,"filesize:%dkB, %d tracks, %d side(s)",filesize/1024,floppydisk->floppyNumberOfTrack,floppydisk->floppyNumberOfSide);

		trackdata=(unsigned char*)malloc(NIB_TRACK_SIZE);

		for(j=0;j<floppydisk->floppyNumberOfTrack;j++)
		{

			floppydisk->tracks[j]=allocCylinderEntry(rpm,floppydisk->floppyNumberOfSide);
			currentcylinder=floppydisk->tracks[j];

			file_offset= NIB_TRACK_SIZE * j;

			fseek (f , file_offset , SEEK_SET);
			fread(trackdata,NIB_TRACK_SIZE,1,f);

#ifdef HDDD_A2_SUPPORT
			currentcylinder->sides[0] = tg_alloctrack(floppydisk->floppyBitRate,trackformat,rpm,NIB_TRACK_SIZE * 2 * 8 * 2,1000,0,0);
			currentcylinder->sides[1] = tg_alloctrack(floppydisk->floppyBitRate,trackformat,rpm,NIB_TRACK_SIZE * 2 * 8 * 2,1000,0,0);
#else
			currentcylinder->sides[0] = tg_alloctrack(floppydisk->floppyBitRate,trackformat,rpm,NIB_TRACK_SIZE * 2 * 8,1000,0,0);
			currentcylinder->sides[1] = tg_alloctrack(floppydisk->floppyBitRate,trackformat,rpm,NIB_TRACK_SIZE * 2 * 8,1000,0,0);
#endif
			for(i=0;i<NIB_TRACK_SIZE;i++)
			{
				data_nibble = trackdata[i];
				pulses = ext_a2_bit_expander[ data_nibble ];

#ifdef HDDD_A2_SUPPORT
				// Add the FM Clocks
				fm_pulses = ext_a2_bit_expander[pulses >> 8] | 0x2222;
				currentcylinder->sides[0]->databuffer[(i*4)+0] = fm_pulses >> 8;
				currentcylinder->sides[0]->databuffer[(i*4)+1] = fm_pulses &  0xFF;

				// Add the FM Clocks
				fm_pulses = ext_a2_bit_expander[pulses &  0xFF] | 0x2222;
				currentcylinder->sides[0]->databuffer[(i*4)+2] = fm_pulses >> 8;
				currentcylinder->sides[0]->databuffer[(i*4)+3] = fm_pulses &  0xFF;
#else
				currentcylinder->sides[0]->databuffer[(i*2)+0] = pulses >> 8;
				currentcylinder->sides[0]->databuffer[(i*2)+1] = pulses &  0xFF;
#endif
			}

#ifdef HDDD_A2_SUPPORT
			memset(currentcylinder->sides[1]->databuffer,0xAA,NIB_TRACK_SIZE * 2 * 2);
#else
			memset(currentcylinder->sides[1]->databuffer,0xAA,NIB_TRACK_SIZE * 2);
#endif
		}

		free(trackdata);

		floppycontext->hxc_printf(MSG_INFO_1,"track file successfully loaded and encoded!");

		hxc_fclose(f);
		return HXCFE_NOERROR;

	}

	floppycontext->hxc_printf(MSG_ERROR,"file size=%d !?",filesize);
	hxc_fclose(f);
	return HXCFE_BADFILE;
}

int Apple2_nib_libGetPluginInfo(HXCFLOPPYEMULATOR* floppycontext,unsigned long infotype,void * returnvalue)
{

	static const char plug_id[]="APPLE2_NIB";
	static const char plug_desc[]="Apple II NIB Loader";
	static const char plug_ext[]="nib";

	plugins_ptr plug_funcs=
	{
		(ISVALIDDISKFILE)	Apple2_nib_libIsValidDiskFile,
		(LOADDISKFILE)		Apple2_nib_libLoad_DiskFile,
		(WRITEDISKFILE)		0,
		(GETPLUGININFOS)	Apple2_nib_libGetPluginInfo
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
