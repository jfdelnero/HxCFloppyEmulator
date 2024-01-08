/*
//
// Copyright (C) 2006-2024 Jean-François DEL NERO
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

typedef struct sect_sector_
{
	int32_t type;
	int32_t sectorsize;
	int32_t track_id;
	int32_t side_id;
	int32_t sector_id;
	uint8_t *buffer;
}sect_sector;

typedef struct sect_track_
{
	uint8_t side;
	uint8_t track;
	int32_t number_of_sector;
	sect_sector ** sectorlist;

}sect_track;

typedef struct sect_floppy_
{
	int32_t number_of_side;
	int32_t number_of_track;
	sect_track ** tracklist;
}sect_floppy;

int analysis_and_extract_sector_MFM(HXCFE* floppycontext,HXCFE_SIDE * track,sect_track * sectors);
int analysis_and_extract_sector_AMIGAMFM(HXCFE* floppycontext,HXCFE_SIDE * track,sect_track * sectors);
int analysis_and_extract_sector_FM(HXCFE* floppycontext,HXCFE_SIDE * track,sect_track * sectors);

int write_raw_file(HXCFE_IMGLDR * imgldr_ctx,FILE * f,HXCFE_FLOPPY * fp,int32_t startidsector,int32_t sectorpertrack,int32_t nboftrack,int32_t nbofside,int32_t sectorsize,int32_t tracktype,int32_t sidefilelayout);

int count_sector(HXCFE* floppycontext,HXCFE_FLOPPY * fp,int32_t startidsector,int32_t track,int32_t side,int32_t sectorsize,int32_t tracktype,uint32_t flags);

void checkEmptySector(HXCFE_SECTCFG * sector);
