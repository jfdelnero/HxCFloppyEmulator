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
	typedef struct cfgrawfile_
	{
		uint8_t  sectorpertrack;
		uint8_t  sectorsize;
		uint32_t numberoftrack;
		uint8_t  tracktype;
		uint8_t  sidecfg;	
		uint32_t gap3;
		uint32_t rpm;
		uint32_t bitrate;
		uint8_t  interleave;
		uint8_t  firstidsector;
		uint8_t  skew;
		uint8_t  autogap3;
		uint8_t  fillvalue;
		uint8_t  intersidesectornumbering;
		uint8_t  sideskew;
	}cfgrawfile;

	enum
	{
		FM_TRACK_TYPE,
		FMIBM_TRACK_TYPE,
		MFM_TRACK_TYPE,
		MFMIBM_TRACK_TYPE,
		GCR_TRACK_TYPE
	};

	typedef struct track_type_
	{
		int32_t id;
		char * name;
		int32_t tracktype;
	}track_type;

	typedef struct sectorsize_type_
	{
		int32_t id;
		char * name;

	}sectorsize_type;
	
	#define TWOSIDESFLOPPY 0x02
	#define SIDE_INVERTED 0x04
	#define SIDE0_FIRST 0x08

int32_t RAW_libGetPluginInfo(HXCFE_IMGLDR * imgldr_ctx,uint32_t infotype,void * returnvalue);
