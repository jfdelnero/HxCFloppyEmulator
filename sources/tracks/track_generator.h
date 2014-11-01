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
#define DEFAULT_HD_BITRATE 500000
#define DEFAULT_DD_BITRATE 250000
#define DEFAULT_AMIGA_BITRATE 253360

#define DEFAULT_DD_RPM 300
#define DEFAULT_AMIGA_RPM 300

typedef struct track_generator_
{
	unsigned long last_bit_offset;

	unsigned short mfm_last_bit;
}track_generator;

#ifndef _HXCFE_SECTCFG_

typedef struct _HXCFE_SECTCFG
{
	unsigned char  head;
	unsigned char  sector;
	unsigned char  sectorsleft;
	unsigned char  cylinder;

	unsigned int   sectorsize;

	unsigned char  use_alternate_sector_size_id;
	unsigned char  alternate_sector_size_id;

	unsigned char  missingdataaddressmark;

	unsigned char  use_alternate_header_crc;	//0x1 -> Bad crc  , 0x2 alternate crc
	unsigned short data_crc;

	unsigned char  use_alternate_data_crc;		//0x1 -> Bad crc  , 0x2 alternate crc
	unsigned short header_crc;

	unsigned char  use_alternate_datamark;
	unsigned char  alternate_datamark;

	unsigned char  use_alternate_addressmark;
	unsigned char  alternate_addressmark;

	unsigned long  startsectorindex;
	unsigned long  startdataindex;
	unsigned long  endsectorindex;

	unsigned char  trackencoding;

	unsigned char  gap3;

	unsigned int   bitrate;

	unsigned char  * input_data;
	unsigned char  fill_byte;
	unsigned char  fill_byte_used;				// Set to indicate that the sector is filled with "fill_byte"
}HXCFE_SECTCFG;

#define _HXCFE_SECTCFG_

#endif

#define       TG_ALLOCTRACK_ALLOCTIMIMGBUFFER   0x01
#define       TG_ALLOCTRACK_ALLOCFLAKEYBUFFER   0x02
#define       TG_ALLOCTRACK_ALLOCENCODINGBUFFER 0x04
#define       TG_ALLOCTRACK_RANDOMIZEDATABUFFER 0x08
#define       TG_ALLOCTRACK_UNFORMATEDBUFFER    0x10

 int             BuildCylinder(unsigned char * mfm_buffer,int mfm_size,unsigned char * track_clk,unsigned char * track_data,int track_size);
 void            BuildFMCylinder(unsigned char * buffer,int fmtracksize,unsigned char * bufferclk,unsigned char * track,int size);

 void            getMFMcode(track_generator *tg,unsigned char data,unsigned char clock,unsigned char * dstbuf);
 void            getFMcode (track_generator *tg,unsigned char data,unsigned char clock,unsigned char * dstbuf);
 int             pushTrackCode(track_generator *tg,unsigned char data,unsigned char clock,HXCFE_SIDE * side,unsigned char trackencoding);

 void            hxcfe_freeSide(HXCFE_SIDE * side);

 void            tg_initTrackEncoder(track_generator *tg);
 unsigned long   tg_computeMinTrackSize(track_generator *tg,unsigned char trackencoding,unsigned int bitrate,unsigned int numberofsector,HXCFE_SECTCFG * sectorconfigtab,unsigned int pregaplen,unsigned long * track_period);
 void            tg_addSectorToTrack(track_generator *tg,HXCFE_SECTCFG * sectorconfig,HXCFE_SIDE * currentside);
 void            tg_completeTrack(track_generator *tg, HXCFE_SIDE * currentside,unsigned char trackencoding);

 HXCFE_SIDE *    tg_initTrack(track_generator *tg,unsigned long tracksize,unsigned short numberofsector,unsigned char trackencoding,unsigned int bitrate,HXCFE_SECTCFG * sectorconfigtab,unsigned short pregap);

 HXCFE_SIDE *    tg_generateTrack(unsigned char * sectors_data,unsigned short sector_size,unsigned short number_of_sector,unsigned char track,unsigned char side,unsigned char sectorid,unsigned char interleave,unsigned char skew,unsigned int bitrate,unsigned short rpm,unsigned char trackencoding,unsigned char gap3,unsigned short pregap, int indexlen,int indexpos);
 HXCFE_SIDE *    tg_generateTrackEx(unsigned short number_of_sector,HXCFE_SECTCFG * sectorconfigtab,unsigned char interleave,unsigned char skew,unsigned int bitrate,unsigned short rpm,unsigned char trackencoding,unsigned short pregap,int indexlen,int indexpos);

 HXCFE_SIDE *    tg_alloctrack(unsigned int bitrate,unsigned char trackencoding,unsigned short rpm,unsigned int tracksize,int indexlen,int indexpos,unsigned char buffertoalloc);

 unsigned long * tg_allocsubtrack_long(unsigned int tracksize,unsigned long initvalue);
 unsigned char * tg_allocsubtrack_char(unsigned int tracksize,unsigned char initvalue);
