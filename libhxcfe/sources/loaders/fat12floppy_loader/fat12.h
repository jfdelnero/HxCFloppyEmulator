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
#define  CHSTOADR(track,head,sector)  (track*(numberofsectorpertrack*numberofside))+(head*numberofsectorpertrack)+(512*sector)

typedef struct FATCONFIG_
{
	uint16_t sectorsize;
	uint16_t nbofsector;

	uint16_t reservedsector;

	uint8_t  numberoffat;
	uint16_t nbofsectorperfat;
	uint8_t  clustersize;

	uint16_t numberofrootentries;
}FATCONFIG;


int setclusterptr(unsigned char *fattable,int index,int ptr);
int findfreecluster(unsigned char *fattable,int nbofcluster);
unsigned char * findfreeentry(unsigned char *entriestable);
int ScanFileAndAddToFAT(HXCFE* floppycontext,char * folder,char * file, unsigned char * fattable,unsigned char *entriestable,unsigned char *datatable,int parentcluster,FATCONFIG * fatconfig,int numberofcluster);
