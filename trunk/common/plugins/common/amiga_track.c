/*
//
// Copyright (C) 2006, 2007, 2008, 2009 Jean-François DEL NERO
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
// File : amiga.c
// Contains: amiga track builder/encoder
//
// Written by:	DEL NERO Jean Francois
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include "amiga_track.h" 

#define MFM_FILLB 0xaa
#define MFM_FILLL 0xaaaaaaaa
#define MFM_MASK  0x55555555
#define FLOPPY_GAP_BYTES 720

typedef unsigned char UBY;
typedef unsigned long ULO;


/////////////////////////////////////////////////
// UAE Amiga MFM encoder.
//
/*=============================================*/
/* Will MFM encode one sector from src to dest */
/*=============================================*/

void floppySectorMfmEncode(ULO tra, ULO sec, UBY *src, UBY *dest, ULO sync) 
{
  ULO tmp, x, odd, even, hck = 0, dck = 0;

  /* Preamble and sync */

  *(dest + 0) = 0xaa;
  *(dest + 1) = 0xaa;
  *(dest + 2) = 0xaa;
  *(dest + 3) = 0xaa;
  *(dest + 4) = (UBY) (sync>>8);
  *(dest + 5) = (UBY) (sync & 0xff);
  *(dest + 6) = (UBY) (sync>>8);
  *(dest + 7) = (UBY) (sync & 0xff);

  /* Track and sector info */

  tmp = 0xff000000 | (tra<<16) | (sec<<8) | (11 - sec);
  odd = (tmp & MFM_MASK) | MFM_FILLL;
  even = ((tmp>>1) & MFM_MASK) | MFM_FILLL;
  *(dest +  8) = (UBY) ((even & 0xff000000)>>24);
  *(dest +  9) = (UBY) ((even & 0xff0000)>>16);
  *(dest + 10) = (UBY) ((even & 0xff00)>>8);
  *(dest + 11) = (UBY) ((even & 0xff));
  *(dest + 12) = (UBY) ((odd & 0xff000000)>>24);
  *(dest + 13) = (UBY) ((odd & 0xff0000)>>16);
  *(dest + 14) = (UBY) ((odd & 0xff00)>>8);
  *(dest + 15) = (UBY) ((odd & 0xff));

  /* Fill unused space */

  for (x = 16 ; x < 48; x++)
    *(dest + x) = MFM_FILLB;

  /* Encode data section of sector */

  for (x = 64 ; x < 576; x++) 
  {
    tmp = *(src + x - 64);
    odd = (tmp & 0x55);
    even = (tmp>>1) & 0x55;
    *(dest + x) = (UBY) (even | MFM_FILLB);
    *(dest + x + 512) = (UBY) (odd | MFM_FILLB);
  }

  /* Calculate checksum for unused space */

  for(x = 8; x < 48; x += 4)
    hck ^= (((ULO) *(dest + x))<<24) | (((ULO) *(dest + x + 1))<<16) |
           (((ULO) *(dest + x + 2))<<8) |  ((ULO) *(dest + x + 3));
  even = odd = hck; 
  odd >>= 1;
  even |= MFM_FILLL;
  odd |= MFM_FILLL;
  *(dest + 48) = (UBY) ((odd & 0xff000000)>>24);
  *(dest + 49) = (UBY) ((odd & 0xff0000)>>16);
  *(dest + 50) = (UBY) ((odd & 0xff00)>>8);
  *(dest + 51) = (UBY) (odd & 0xff);
  *(dest + 52) = (UBY) ((even & 0xff000000)>>24);
  *(dest + 53) = (UBY) ((even & 0xff0000)>>16);
  *(dest + 54) = (UBY) ((even & 0xff00)>>8);
  *(dest + 55) = (UBY) (even & 0xff);

  /* Calculate checksum for data section */

  for(x = 64; x < 1088; x += 4)
    dck ^= (((ULO) *(dest + x))<<24) | (((ULO) *(dest + x + 1))<<16) |
           (((ULO) *(dest + x + 2))<< 8) |  ((ULO) *(dest + x + 3));
  even = odd = dck;
  odd >>= 1;
  even |= MFM_FILLL;
  odd |= MFM_FILLL;
  *(dest + 56) = (UBY) ((odd & 0xff000000)>>24);
  *(dest + 57) = (UBY) ((odd & 0xff0000)>>16);
  *(dest + 58) = (UBY) ((odd & 0xff00)>>8);
  *(dest + 59) = (UBY) (odd & 0xff);
  *(dest + 60) = (UBY) ((even & 0xff000000)>>24);
  *(dest + 61) = (UBY) ((even & 0xff0000)>>16);
  *(dest + 62) = (UBY) ((even & 0xff00)>>8);
  *(dest + 63) = (UBY) (even & 0xff);
}

void floppyGapMfmEncode(UBY *dst) 
{
  ULO i;
  for (i = 0; i < FLOPPY_GAP_BYTES; i++) *dst++ = MFM_FILLB;
}


int BuildAmigaTrack(unsigned char * data,unsigned char * mfmdata,int mfmsizebuffer,int tracknumber,int sidenumber,int numberofsector)
{
	int i,j,k;
	int lastbit;
	unsigned char *tempdata;
	unsigned long amigatrack;
	unsigned char c,c1;
	
	j=0;
	lastbit=0;
	tempdata=(char *)malloc(128*1024);
	
	amigatrack=(tracknumber<<1)|sidenumber;
	for(i=0;i<numberofsector;i++)
	{
		floppySectorMfmEncode(amigatrack, i, data+(i*512), tempdata+ (i*(1088)), 0x4489);
	}
	
	floppyGapMfmEncode(tempdata + 11*1088);
	
	lastbit=0;
	k=0;
	for(i=0;i<mfmsizebuffer ;i++)
	{
		c=*(tempdata+i);
		
		*(mfmdata+k)=0;
		for(j=0;j<8;j=j+2)
		{
			c1=(c>>(6-j))&0x3;
			if(c1&0x1)
			{
				*(mfmdata+k)=*(mfmdata+k)|(0x01<<(6-j));
				lastbit=1;
			}
			else
			{
				if(lastbit==0 && (c1&0x2))
				{
					*(mfmdata+k)=*(mfmdata+k)|(0x02<<(6-j));
					
				}
				else
				{
					*(mfmdata+k)=*(mfmdata+k)|(0x00<<(6-j));
				}
				lastbit=0;
			}
		}
		k++;
	}
	
	free(tempdata);
	
	return mfmsizebuffer;
}
