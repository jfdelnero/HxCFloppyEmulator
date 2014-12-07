/*
//
// Copyright (C) 2006-2014 Jean-François DEL NERO
//
// This file is part of HxCFloppyEmulator.
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
// File : variablebitrate.h
// Contains: USB HxC FE data generator 
//
// Written by:	DEL NERO Jean Francois
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#define FLOPPYEMUFREQ 16000000

typedef struct trackzone_
{
	int32_t  start;
	int32_t  end;
	uint32_t bitrate;

	uint8_t  code1;
	uint32_t code1lenint;

	uint8_t  code2;
	uint32_t code2lenint;

}trackzone;

typedef struct trackpart_
{
	uint8_t code;
	int32_t len;
}trackpart;

#define GRANULA 64

int32_t GetNewTrackRevolution(HXCFE* floppycontext,uint8_t * index_h0,uint8_t * datah0,uint32_t lendatah0,uint8_t * datah1,uint32_t lendatah1,uint8_t * randomh0,uint8_t * randomh1,int32_t fixedbitrateh0,uint32_t * timeh0,int32_t fixedbitrateh1,uint32_t * timeh1,uint8_t ** finalbuffer_param,uint8_t ** randomfinalbuffer_param,uint8_t readysignal,uint8_t diskchange,uint8_t writeprotect,uint8_t amigaready,uint8_t selectconfig);




