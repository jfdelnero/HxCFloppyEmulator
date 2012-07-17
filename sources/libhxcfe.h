/*
//
// Copyright (C) 2006 - 2012 Jean-François DEL NERO
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

#include "internal_floppy.h"
#include "./tracks/track_generator.h"
#include "plugins_id.h"

///////////////////////////////////
// Functions return error codes

#define HXCFE_VALIDFILE			1
#define HXCFE_NOERROR			0 
#define HXCFE_ACCESSERROR		-1
#define HXCFE_BADFILE			-2
#define HXCFE_FILECORRUPTED		-3 
#define HXCFE_BADPARAMETER		-4
#define HXCFE_INTERNALERROR		-5
#define HXCFE_UNSUPPORTEDFILE	-6

// Output functions 
typedef int (*HXCPRINTF_FUNCTION)(int MSGTYPE,char * string, ...);
typedef int (*DISPLAYTRACKPOS_FUNCTION)(unsigned int current,unsigned int total);


typedef struct HXCFLOPPYEMULATOR_
{
	HXCPRINTF_FUNCTION hxc_printf;
	DISPLAYTRACKPOS_FUNCTION hxc_settrackpos;
	unsigned char CONTAINERTYPE[16];
}HXCFLOPPYEMULATOR;


////////////////////////////////////////////
// Init Function

HXCFLOPPYEMULATOR* hxcfe_init(void);
void hxcfe_deinit(HXCFLOPPYEMULATOR* hxcfe);

const char * hxcfe_getVersion(HXCFLOPPYEMULATOR* floppycontext);
const char * hxcfe_getLicense(HXCFLOPPYEMULATOR* floppycontext);

////////////////////////////////////////////
// stdio printf functions

// Output Message  level
#define MSG_INFO_0 0
#define MSG_INFO_1 1
#define MSG_WARNING 2
#define MSG_ERROR 3
#define MSG_DEBUG 4


int hxcfe_setOutputFunc(HXCFLOPPYEMULATOR* floppycontext,HXCPRINTF_FUNCTION hxc_printf);


////////////////////////////////////////////
// File image functions
int			hxcfe_numberOfLoader(HXCFLOPPYEMULATOR* floppycontext);
int			hxcfe_getLoaderID(HXCFLOPPYEMULATOR* floppycontext,char * container);
int			hxcfe_getLoaderAccess(HXCFLOPPYEMULATOR* floppycontext,int moduleID);
const char* hxcfe_getLoaderDesc(HXCFLOPPYEMULATOR* floppycontext,int moduleID);
const char* hxcfe_getLoaderName(HXCFLOPPYEMULATOR* floppycontext,int moduleID);
const char* hxcfe_getLoaderExt(HXCFLOPPYEMULATOR* floppycontext,int moduleID);


int hxcfe_autoSelectLoader(HXCFLOPPYEMULATOR* floppycontext,char* imgname,int moduleID);
FLOPPY * hxcfe_floppyLoad(HXCFLOPPYEMULATOR* floppycontext,char* imgname,int moduleID,int * err_ret);
int hxcfe_floppyUnload(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * floppydisk);
int hxcfe_floppyExport(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * newfloppy,char* imgname,int moduleID);

int hxcfe_getNumberOfTrack(HXCFLOPPYEMULATOR* floppycontext,FLOPPY *fp);
int hxcfe_getNumberOfSide(HXCFLOPPYEMULATOR* floppycontext,FLOPPY *fp);

////////////////////////////////////////////
// Custom Image/floppy generation functions

#define STACK_SIZE 0x400

typedef struct fb_track_state_
{
	SECTORCONFIG sectorconfig;
	int track_number;
	unsigned char side_number;
	unsigned char interleave;
	unsigned char skew;
	unsigned char type;
	unsigned short rpm;
	int bitrate;

	int indexlen;
	int indexpos;
	int sectorunderindex;

	int numberofsector;
	SECTORCONFIG sectortab[STACK_SIZE];

	int sc_stack_pointer;
	SECTORCONFIG sc_stack[STACK_SIZE];
}fb_track_state;

typedef struct FBuilder_
{
	FLOPPY * floppydisk;
	int fb_stack_pointer;
	fb_track_state * fb_stack;
}FBuilder;

FBuilder* hxcfe_initFloppy(HXCFLOPPYEMULATOR* floppycontext,int nb_of_track,int nb_of_side);

int	hxcfe_pushTrack (FBuilder*,unsigned int rpm,int number,int side,int type);
int hxcfe_setTrackInterleave (FBuilder*,int interleave);
int hxcfe_setTrackSkew (FBuilder*,int skew);

int hxcfe_setIndexPosition (FBuilder*,int position,int allowsector);
int hxcfe_setIndexLength (FBuilder*,int Length);

int hxcfe_setTrackBitrate (FBuilder*,int bitrate);

int hxcfe_addSector (FBuilder* fb,int sectornumber,int side,int track,unsigned char * buffer,int size);

int hxcfe_setSectorBitrate (FBuilder* fb,int bitrate);

int hxcfe_setSectorGap3 (FBuilder* fb,unsigned char Gap3);
int hxcfe_setSectorSizeID (FBuilder* fb,unsigned char sectorsizeid);
int hxcfe_setSectorFill (FBuilder*,unsigned char fill);

int hxcfe_setSectorEncoding (FBuilder*,int encoding);

int hxcfe_setSectorDataCRC (FBuilder*,unsigned short crc);
int hxcfe_setSectorHeaderCRC (FBuilder*,unsigned short crc);

int hxcfe_setSectorDataMark (FBuilder*,unsigned char datamark);

int hxcfe_popTrack (FBuilder* fb);

FLOPPY* hxcfe_getFloppy (FBuilder* fb);

int hxcfe_getFloppySize(HXCFLOPPYEMULATOR* floppycontext,FLOPPY *fp,int * nbsector);

////////////////////////////////////////////
// Read Sector functions

typedef struct SECTORSEARCH_
{
	HXCFLOPPYEMULATOR* hxcfe;
	FLOPPY *fp;
	int bitoffset;
	int cur_track;
	int cur_side;
}SECTORSEARCH;



SECTORSEARCH* hxcfe_initSectorSearch(HXCFLOPPYEMULATOR* floppycontext,FLOPPY *fp);
SECTORCONFIG* hxcfe_getNextSector(SECTORSEARCH* ss,int track,int side,int type);
SECTORCONFIG* hxcfe_searchSector(SECTORSEARCH* ss,int track,int side,int id,int type);
int hxcfe_getSectorSize(SECTORSEARCH* ss,SECTORCONFIG* sc);
unsigned char * hxcfe_getSectorData(SECTORSEARCH* ss,SECTORCONFIG* sc);
int hxcfe_readSectorData(SECTORSEARCH* ss,int track,int side,int sector,int numberofsector,int sectorsize,int type,unsigned char * buffer);
void hxcfe_freeSectorConfig(SECTORSEARCH* ss,SECTORCONFIG* sc);
void hxcfe_deinitSectorSearch(SECTORSEARCH* ss);

////////////////////////////////////////////
// Floppy interfaces setting functions.

#define IBMPC_DD_FLOPPYMODE				0x00
#define IBMPC_HD_FLOPPYMODE				0x01
#define ATARIST_DD_FLOPPYMODE			0x02
#define ATARIST_HD_FLOPPYMODE			0x03
#define AMIGA_DD_FLOPPYMODE				0x04
#define AMIGA_HD_FLOPPYMODE				0x05
#define CPC_DD_FLOPPYMODE				0x06
#define GENERIC_SHUGART_DD_FLOPPYMODE	0x07
#define IBMPC_ED_FLOPPYMODE				0x08
#define MSX2_DD_FLOPPYMODE				0x09
#define C64_DD_FLOPPYMODE				0x0A
#define EMU_SHUGART_FLOPPYMODE			0x0B
#define S950_DD_FLOPPYMODE				0x0C
#define S950_HD_FLOPPYMODE				0x0D

enum {
	DOUBLESTEP = 1,
	INTERFACEMODE = 2
};

enum {
	SET = 0,
	GET = 1
};

int hxcfe_floppyGetSetParams(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * newfloppy,unsigned char dir,unsigned short param,void * value);

int hxcfe_floppyGetInterfaceMode(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * newfloppy);
int hxcfe_floppySetInterfaceMode(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * newfloppy,int ifmode);

int hxcfe_floppyGetDoubleStep(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * newfloppy);
int hxcfe_floppySetDoubleStep(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * newfloppy,int doublestep);

int hxcfe_getFloppyInterfaceModeID(HXCFLOPPYEMULATOR* floppycontext,char * ifmode);

const char * hxcfe_getFloppyInterfaceModeName(HXCFLOPPYEMULATOR* floppycontext,int ifmodeid);
const char * hxcfe_getFloppyInterfaceModeDesc(HXCFLOPPYEMULATOR* floppycontext,int ifmodeid);
const char * hxcfe_getTrackEncodingName(HXCFLOPPYEMULATOR* floppycontext,int trackencodingid);


typedef struct s_sectorlist_
{
	SECTORCONFIG * sectorconfig;
	int x_pos1,y_pos1;
	int x_pos2,y_pos2;
	struct s_sectorlist_ * next_element;
}s_sectorlist;

typedef struct s_trackdisplay_
{
	int xsize,ysize;
	int x_us,y_us;
	int x_start_us;
	unsigned int * framebuffer;

	s_sectorlist * sl;

}s_trackdisplay;

s_trackdisplay * hxcfe_td_init(HXCFLOPPYEMULATOR* floppycontext,unsigned long xsize,unsigned long ysize);
void hxcfe_td_setparams(HXCFLOPPYEMULATOR* floppycontext,s_trackdisplay *td,unsigned long x_us,unsigned long y_us,unsigned long x_start_us);
void hxcfe_td_draw_track(HXCFLOPPYEMULATOR* floppycontext,s_trackdisplay *td,FLOPPY * floppydisk,int track,int side);
s_sectorlist * hxcfe_td_getlastsectorlist(HXCFLOPPYEMULATOR* floppycontext,s_trackdisplay *td);
void hxcfe_td_draw_disk(HXCFLOPPYEMULATOR* floppycontext,s_trackdisplay *td,FLOPPY * floppydisk);
void hxcfe_td_deinit(HXCFLOPPYEMULATOR* floppycontext,s_trackdisplay *td);

