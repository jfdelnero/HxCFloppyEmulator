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
// File : arburg_track.c
// Contains: Arburg track builder/encoder
//
// Written by:	DEL NERO Jean Francois
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "internal_libhxcfe.h"
#include "tracks/track_generator.h"
#include "libhxcfe.h"

#include "crc.h"
#include "arburg_track.h"

#include "trackutils.h"

extern unsigned char bit_inverter_emuii[];

int BuildArburgTrack(HXCFE* floppycontext,unsigned int tracknumber,unsigned int sidenumber,unsigned char* datain,unsigned char * fmdata,unsigned long * fmsizebuffer,int trackformat)
{
	/*
		Arburg Track format:
		Sync : 0xFF
		Data : 0x9FE Bytes
		Checksum : 1 Low Byte
		Checksum : 1 High Byte
		Sync? : OxDF Bytes (High bytes checksum)
	*/

	unsigned int i,j,k;
	unsigned char *tempdata;
	unsigned char *tempclock;
	unsigned long finalsize;
	unsigned long current_buffer_size;

	unsigned short checksum;

	unsigned long sectorsize;
	unsigned long buffersize;


	buffersize=*fmsizebuffer/8;

	sectorsize=0x9FE;

	current_buffer_size=buffersize/4;
	finalsize=1 + sectorsize + 1 + 0xDF;

	if(finalsize<=current_buffer_size)
	{

		j=0;

		tempdata=(unsigned char *)malloc((buffersize/4)+1);
		tempclock=(unsigned char *)malloc((buffersize/4)+1);

		if(tempdata && tempclock)
		{

			memset(tempclock,0xFF,(buffersize/4)+1);
			memset(tempdata, 0x00,(buffersize/4)+1);


			/////////////////////////////////////////////////////////////////////////////////////////////
			//Track GAP
			for(k=0;k<(56*8);k++)
			{
				setfieldbit(tempdata,bit_inverter_emuii[0x00],j,8);
				j=j+8;
			}

			/////////////////////////////////////////////////////////////////////////////////////////////
			//Sector Header
			setfieldbit(tempdata,bit_inverter_emuii[0xFF],j,8);
			j=j+8;

			/////////////////////////////////////////////////////////////////////////////////////////////
			//Sector Data

			checksum = 0;
			for(k=0;k<ARBURB_DATATRACK_SIZE-2;k++)
			{
				checksum = checksum + datain[k];
			}


			for(k=0;k<ARBURB_DATATRACK_SIZE-2;k++)
			{
				setfieldbit(tempdata,bit_inverter_emuii[datain[k]],j,8);
				j=j+8;
			}

			#ifndef IGNORE_ARBURG_CHECKSUM

			for(k=ARBURB_DATATRACK_SIZE-2;k<ARBURB_DATATRACK_SIZE;k++)
			{
				setfieldbit(tempdata,bit_inverter_emuii[datain[k]],j,8);
				j=j+8;
			}

			/////////////////////////////////////////////////////////////////////////////////////////////
			//Checksum
			if( ( datain[ARBURB_DATATRACK_SIZE-2] != (checksum&0xFF) ) || ( datain[ARBURB_DATATRACK_SIZE-1] != (checksum>>8) ) )
			{
				floppycontext->hxc_printf(MSG_WARNING,"BuildArburgTrack : Block Checksum error !");
			}

			#else

			setfieldbit(tempdata,bit_inverter_emuii[checksum&0xFF],j,8);
			j=j+8;
			setfieldbit(tempdata,bit_inverter_emuii[checksum>>8],j,8);
			j=j+8;

			#endif


			if((j/8)<=current_buffer_size)
			{
				for(i=j;i<(current_buffer_size*8);i=i+8)
				{
					if(j+8<(current_buffer_size*8))
						setfieldbit(tempdata,bit_inverter_emuii[checksum>>8],j,8);
					else
						setfieldbit(tempdata,bit_inverter_emuii[checksum>>8],j,(current_buffer_size*8)-j);

					j=j+8;
				}
			}

			BuildFMCylinder(fmdata,buffersize,tempclock,tempdata,(buffersize)/4);

			free(tempdata);
			free(tempclock);
			
			return 0;
		}
		else
		{
			if(tempdata)
				free(tempdata);
			if(tempclock)
				free(tempclock);
			
			return -1;
		}

	}
	else
	{
		floppycontext->hxc_printf(MSG_ERROR,"BuildArburgTrack : No enough space on this track !");
		return finalsize;
	}
}

int pushArburgSysByte(unsigned char byte, int bitoffset, unsigned char * track,unsigned long tracksize)
{
	int k;

	for(k=0;k<8;k++)
	{
		if(byte&(0x01<<k))
		{
			bitoffset = (bitoffset + 1) % (tracksize);
			track[bitoffset>>3] = track[bitoffset>>3] & ~(0x80>> (bitoffset&7));
			bitoffset = (bitoffset + 1) % (tracksize);
			track[bitoffset>>3] = track[bitoffset>>3] & ~(0x80>> (bitoffset&7));
			bitoffset = (bitoffset + 1) % (tracksize);
			track[bitoffset>>3] = track[bitoffset>>3] | (0x80>> (bitoffset&7));
		}
		else
		{
			bitoffset = (bitoffset + 1) % (tracksize);
			track[bitoffset>>3] = track[bitoffset>>3] & ~(0x80>> (bitoffset&7));
			bitoffset = (bitoffset + 1) % (tracksize);
			track[bitoffset>>3] = track[bitoffset>>3] | (0x80>> (bitoffset&7));
		}
	}

	return bitoffset;
}

int BuildArburgSysTrack(HXCFE* floppycontext,unsigned int tracknumber,unsigned int sidenumber,unsigned char* datain,unsigned char * fmdata,unsigned long * fmsizebuffer,int trackformat)
{
	/*
		Arburg Track format:
		Sync : 0xFF
		Data : 0xEFE Bytes
		Checksum : 1 Low Byte
		Checksum : 1 High Byte
		Sync? : OxDF Bytes (High bytes checksum)
	*/

	unsigned int k;
	unsigned short checksum;
	int bitoffset,newbitoffset;

	bitoffset = 0;

	memset(fmdata, 0x44,*fmsizebuffer/8);

	/////////////////////////////////////////////////////////////////////////////////////////////
	//Track GAP
	for(k=0;k<(60*8);k++)
	{
		bitoffset = pushArburgSysByte(0x00,bitoffset,fmdata,*fmsizebuffer);
	}

	/////////////////////////////////////////////////////////////////////////////////////////////
	//Sector Header
	bitoffset = pushArburgSysByte(0xFF,bitoffset,fmdata,*fmsizebuffer);

	/////////////////////////////////////////////////////////////////////////////////////////////
	//Sector Data
	checksum = 0;
	for(k=0;k<(ARBURB_SYSTEMTRACK_SIZE-2);k++)
	{
		checksum = checksum + datain[k];
	}

	for(k=0;k<(ARBURB_SYSTEMTRACK_SIZE-2);k++)
	{
		bitoffset = pushArburgSysByte(datain[k],bitoffset,fmdata,*fmsizebuffer);
	}

	#ifndef IGNORE_ARBURG_CHECKSUM

	for(k=ARBURB_SYSTEMTRACK_SIZE-2;k<ARBURB_SYSTEMTRACK_SIZE;k++)
	{
		bitoffset = pushArburgSysByte(datain[k],bitoffset,fmdata,*fmsizebuffer);
	}

	if( ( datain[ARBURB_SYSTEMTRACK_SIZE-2] != (checksum&0xFF) ) || ( datain[ARBURB_SYSTEMTRACK_SIZE-1] != (checksum>>8) ) )
	{
		floppycontext->hxc_printf(MSG_WARNING,"BuildArburgSysTrack : Block Checksum error !");
	}

	#else

	// Low byte
	bitoffset = pushArburgSysByte((unsigned char)(checksum&0xFF),bitoffset,fmdata,*fmsizebuffer);
	// High byte
	bitoffset = pushArburgSysByte((unsigned char)(checksum>>8),bitoffset,fmdata,*fmsizebuffer);

	#endif

	newbitoffset = bitoffset;
	do
	{
		bitoffset = newbitoffset;
		newbitoffset = pushArburgSysByte((unsigned char)(checksum>>8),bitoffset,fmdata,*fmsizebuffer);
	}while(newbitoffset > bitoffset );


	return 0;
}
