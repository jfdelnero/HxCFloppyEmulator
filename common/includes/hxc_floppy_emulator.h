/*
//
// Copyright (C) 2006, 2007, 2008, 2009, 2010, 2011 Jean-François DEL NERO
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

#include "internal_floppy.h"
#include "../plugins/common/track_generator.h"
#include "plugins_id.h"

//////////////////////////////////////////////////////////
// Output Message  level
#define MSG_INFO_0 0
#define MSG_INFO_1 1
#define MSG_WARNING 2
#define MSG_ERROR 3
#define MSG_DEBUG 4
// Output functions 
typedef int (*HXCPRINTF_FUNCTION)(int MSGTYPE,char * string, ...);
typedef int (*DISPLAYTRACKPOS_FUNCTION)(unsigned int current,unsigned int total);

//////////////////////////////////////////////////////////

#define HXCFE_VALIDFILE			1
#define HXCFE_NOERROR			0 
#define HXCFE_ACCESSERROR		-1
#define HXCFE_BADFILE			-2
#define HXCFE_FILECORRUPTED		-3 
#define HXCFE_BADPARAMETER		-4
#define HXCFE_INTERNALERROR		-5
#define HXCFE_UNSUPPORTEDFILE	-6

typedef struct HXCFLOPPYEMULATOR_
{
	HXCPRINTF_FUNCTION hxc_printf;
	DISPLAYTRACKPOS_FUNCTION hxc_settrackpos;
	unsigned char CONTAINERTYPE[16];
}HXCFLOPPYEMULATOR;


////////////////////////////////////////////////////////////////////////////////////////////
// Init Function

HXCFLOPPYEMULATOR* hxcfe_init(void);
int hxcfe_getversion(HXCFLOPPYEMULATOR* floppycontext,char * version,unsigned int * size1,char *copyright,unsigned int * size2);
int hxcfe_set_outputfunc(HXCFLOPPYEMULATOR* floppycontext,HXCPRINTF_FUNCTION hxc_printf);
void hxcfe_deinit(HXCFLOPPYEMULATOR* hxcfe);

////////////////////////////////////////////////////////////////////////////////////////////
// File image functions

int hxcfe_getcontainerid(HXCFLOPPYEMULATOR* floppycontext,int index,char * id,char * desc);
int hxcfe_select_container(HXCFLOPPYEMULATOR* floppycontext,char * container);

FLOPPY * hxcfe_floppy_load(HXCFLOPPYEMULATOR* floppycontext,char* imgname,int * err_ret);
int hxcfe_floppy_unload(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * floppydisk);
int hxcfe_floppy_export(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * newfloppy,char* imgname);

int hxcfe_getNumberOfTrack(HXCFLOPPYEMULATOR* floppycontext,FLOPPY *fp);
int hxcfe_getNumberOfSide(HXCFLOPPYEMULATOR* floppycontext,FLOPPY *fp);

////////////////////////////////////////////////////////////////////////////////////////////
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

FBuilder* hxcfe_init_floppy(HXCFLOPPYEMULATOR* floppycontext,int nb_of_track,int nb_of_side);

int	hxcfe_pushTrack (FBuilder*,unsigned int rpm,int number,int side,int type);
int hxcfe_setTrackInterleave(FBuilder*,int interleave);
int hxcfe_setTrackSkew(FBuilder*,int skew);

int hxcfe_setIndexPosition(FBuilder*,int position,int allowsector);
int hxcfe_setIndexLength(FBuilder*,int Length);

int hxcfe_setTrackBitrate(FBuilder*,int bitrate);

int hxcfe_addSector(FBuilder* fb,int sectornumber,int side,int track,unsigned char * buffer,int size);

int hxcfe_setSectorBitrate(FBuilder* fb,int bitrate);

int hxcfe_setSectorGap3(FBuilder* fb,unsigned char Gap3);
int hxcfe_setSectorSizeID(FBuilder* fb,unsigned char sectorsizeid);
int hxcfe_setSectorFill(FBuilder*,unsigned char fill);

int hxcfe_setSectorEncoding(FBuilder*,int encoding);

int hxcfe_setSectorDataCRC(FBuilder*,unsigned short crc);
int hxcfe_setSectorHeaderCRC(FBuilder*,unsigned short crc);

int hxcfe_setSectorDataMark(FBuilder*,unsigned char datamark);

int hxcfe_popTrack (FBuilder* fb);

FLOPPY* hxcfe_get_floppy(FBuilder* fb);

int hxcfe_getfloppysize(HXCFLOPPYEMULATOR* floppycontext,FLOPPY *fp,int * nbsector);

//
////////////////////////////////////////////////////////////////////////////////////////////
typedef struct SECTORSEARCH_
{
	HXCFLOPPYEMULATOR* hxcfe;
	FLOPPY *fp;
	int bitoffset;
	int cur_track;
	int cur_side;
}SECTORSEARCH;



SECTORSEARCH* hxcfe_init_sectorsearch(HXCFLOPPYEMULATOR* floppycontext,FLOPPY *fp);
SECTORCONFIG* hxcfe_getnextsector(SECTORSEARCH* ss,int track,int side);
SECTORCONFIG* hxcfe_searchsector(SECTORSEARCH* ss,int track,int side,int id);
int hxcfe_getsectorsize(SECTORSEARCH* ss,SECTORCONFIG* sc);
unsigned char * hxcfe_getsectordata(SECTORSEARCH* ss,SECTORCONFIG* sc);
int hxcfe_readsectordata(SECTORSEARCH* ss,int track,int side,int sector,int numberofsector,int sectorsize,unsigned char * buffer);
void hxcfe_free_sectorconfig(SECTORSEARCH* ss,SECTORCONFIG* sc);
void hxcfe_deinit_sectorsearch(SECTORSEARCH* ss);

////////////////////////////////////////////////////////////////////////////////////////////
// Floppy functions

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

enum {
	DOUBLESTEP = 1,
	INTERFACEMODE = 2
};

enum {
	SET = 0,
	GET = 1
};

int hxcfe_floppy_getset_params(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * newfloppy,unsigned char dir,unsigned short param,void * value);

