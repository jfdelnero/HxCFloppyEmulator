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
	int32_t  last_bit_offset;
	uint16_t mfm_last_bit;
}track_generator;

#ifndef _HXCFE_SECTCFG_

typedef struct _HXCFE_SECTCFG
{
	int32_t        head;
	int32_t        sector;
	int32_t        sectorsleft;
	int32_t        cylinder;

	int32_t        sectorsize;

	int32_t        use_alternate_sector_size_id;
	int32_t        alternate_sector_size_id;

	int32_t        missingdataaddressmark;

	int32_t        use_alternate_header_crc;	//0x1 -> Bad crc  , 0x2 alternate crc
	uint32_t       data_crc;

	int32_t        use_alternate_data_crc;		//0x1 -> Bad crc  , 0x2 alternate crc
	uint32_t       header_crc;

	int32_t        use_alternate_datamark;
	int32_t        alternate_datamark;

	int32_t        use_alternate_addressmark;
	int32_t        alternate_addressmark;

	int32_t        startsectorindex;
	int32_t        startdataindex;
	int32_t        endsectorindex;

	int32_t        trackencoding;

	int32_t        gap3;

	int32_t        bitrate;

	uint8_t     * input_data;
	uint8_t       fill_byte;
	uint8_t       fill_byte_used;				// Set to indicate that the sector is filled with "fill_byte"
}HXCFE_SECTCFG;

#define _HXCFE_SECTCFG_

#endif

#define       TG_ALLOCTRACK_ALLOCTIMIMGBUFFER   0x01
#define       TG_ALLOCTRACK_ALLOCFLAKEYBUFFER   0x02
#define       TG_ALLOCTRACK_ALLOCENCODINGBUFFER 0x04
#define       TG_ALLOCTRACK_RANDOMIZEDATABUFFER 0x08
#define       TG_ALLOCTRACK_UNFORMATEDBUFFER    0x10

 int32_t         BuildCylinder(uint8_t * mfm_buffer,int32_t mfm_size,uint8_t * track_clk,uint8_t * track_data,int32_t track_size);
 void            BuildFMCylinder(uint8_t * buffer,int32_t fmtracksize,uint8_t * bufferclk,uint8_t * track,int32_t size);

 void            getMFMcode(track_generator *tg,uint8_t data,uint8_t clock,unsigned char * dstbuf);
 void            getFMcode (track_generator *tg,uint8_t data,uint8_t clock,unsigned char * dstbuf);
 int32_t         pushTrackCode(track_generator *tg,uint8_t data,uint8_t clock,HXCFE_SIDE * side,int32_t trackencoding);

 void            hxcfe_freeSide(HXCFE_SIDE * side);

 void            tg_initTrackEncoder(track_generator *tg);
 int32_t         tg_computeMinTrackSize(track_generator *tg,int32_t trackencoding,int32_t bitrate,int32_t numberofsector,HXCFE_SECTCFG * sectorconfigtab,int32_t pregaplen,int32_t * track_period);
 void            tg_addSectorToTrack(track_generator *tg,HXCFE_SECTCFG * sectorconfig,HXCFE_SIDE * currentside);
 void            tg_completeTrack(track_generator *tg, HXCFE_SIDE * currentside,int32_t trackencoding);
 HXCFE_SIDE *    tg_initTrack(track_generator *tg,int32_t tracksize,int32_t numberofsector,int32_t trackencoding,int32_t bitrate,HXCFE_SECTCFG * sectorconfigtab,int32_t pregap);

 HXCFE_SIDE *    tg_generateTrack(uint8_t * sectors_data,int32_t sector_size,int32_t number_of_sector,int32_t track,int32_t side,int32_t sectorid,int32_t interleave,int32_t skew,int32_t bitrate,int32_t rpm,int32_t trackencoding,int32_t gap3,int32_t pregap, int32_t indexlen,int32_t indexpos);
 HXCFE_SIDE *    tg_generateTrackEx(int32_t number_of_sector,HXCFE_SECTCFG * sectorconfigtab,int32_t interleave,int32_t skew,int32_t bitrate,int32_t rpm,int32_t trackencoding,int32_t pregap,int32_t indexlen,int32_t indexpos);

 HXCFE_SIDE *    tg_alloctrack(int32_t bitrate,int32_t trackencoding,int32_t rpm,int32_t tracksize,int32_t indexlen,int32_t indexpos,int32_t buffertoalloc);


 uint32_t * tg_allocsubtrack_long( int32_t tracksize, uint32_t initvalue );
 uint8_t  * tg_allocsubtrack_char( int32_t tracksize, uint8_t initvalue );
