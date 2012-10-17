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
FLOPPY * hxcfe_floppyLoadEx(HXCFLOPPYEMULATOR* floppycontext,char* imgname,int moduleID,int * err_ret,void * parameters);
int hxcfe_floppyUnload(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * floppydisk);
int hxcfe_floppyExport(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * newfloppy,char* imgname,int moduleID);

int hxcfe_getNumberOfTrack(HXCFLOPPYEMULATOR* floppycontext,FLOPPY *fp);
int hxcfe_getNumberOfSide(HXCFLOPPYEMULATOR* floppycontext,FLOPPY *fp);

////////////////////////////////////////////
// Custom Image/floppy generation functions

#define STACK_SIZE 0x80
#define NUMBEROFSECTOR_MAX 0x200

typedef struct fb_track_state_
{
	SECTORCONFIG sectorconfig;
	int track_number;
	unsigned char side_number;
	unsigned char interleave;
	unsigned char start_sector_id;
	unsigned int sectors_size;
	unsigned char skew;
	unsigned char type;
	unsigned short rpm;
	unsigned short pregap;
	int bitrate;

	int indexlen;
	int indexpos;
	int sectorunderindex;

	int numberofsector_min;
	int numberofsector;
	SECTORCONFIG sectortab[NUMBEROFSECTOR_MAX];

	int sc_stack_pointer;
	SECTORCONFIG sc_stack[36];
}fb_track_state;

typedef struct FBuilder_
{
	FLOPPY * floppydisk;
	int fb_stack_pointer;
	fb_track_state * fb_stack;
}FBuilder;

FBuilder* hxcfe_initFloppy(HXCFLOPPYEMULATOR* floppycontext,int nb_of_track,int nb_of_side);
int hxcfe_setNumberOfTrack (FBuilder* fb,unsigned short numberoftrack);
int hxcfe_setNumberOfSide (FBuilder* fb,unsigned char numberofside);
int hxcfe_setNumberOfSector (FBuilder* fb,unsigned short numberofsector);
int hxcfe_setSectorSize(FBuilder* fb,int size);
int hxcfe_setStartSectorID(FBuilder* fb,unsigned char startsectorid);

int	hxcfe_pushTrack (FBuilder*,unsigned int rpm,int number,int side,int type);
int hxcfe_pushTrackPFS (FBuilder* fb,int number,int side);

int hxcfe_setTrackInterleave (FBuilder*,int interleave);
int hxcfe_setTrackSkew (FBuilder*,int skew);

int hxcfe_setTrackPreGap (FBuilder* fb,unsigned short pregap);

int hxcfe_setIndexPosition (FBuilder*,int position,int allowsector);
int hxcfe_setIndexLength (FBuilder*,int Length);

int hxcfe_setTrackBitrate (FBuilder*,int bitrate);

int hxcfe_addSector (FBuilder* fb,int sectornumber,int side,int track,unsigned char * buffer,int size);

int hxcfe_addSectors(FBuilder* fb,int side,int track,unsigned char * trackdata,int buffersize,int numberofsectors);

int hxcfe_pushSector (FBuilder* fb);

int hxcfe_setSectorBitrate (FBuilder* fb,int bitrate);

int hxcfe_setSectorGap3 (FBuilder* fb,unsigned char Gap3);
int hxcfe_setSectorSizeID (FBuilder* fb,unsigned char sectorsizeid);
int hxcfe_setSectorFill (FBuilder*,unsigned char fill);

int hxcfe_setSectorTrackID(FBuilder* fb,unsigned char track);
int hxcfe_setSectorHeadID(FBuilder* fb,unsigned char head);
int hxcfe_setSectorID(FBuilder* fb,unsigned char id);

int hxcfe_setSectorEncoding (FBuilder*,int encoding);

int hxcfe_setSectorDataCRC (FBuilder*,unsigned short crc);
int hxcfe_setSectorHeaderCRC (FBuilder*,unsigned short crc);

int hxcfe_setSectorDataMark (FBuilder*,unsigned char datamark);

int hxcfe_setSectorData(FBuilder* fb,unsigned char * buffer,int size);

int hxcfe_popSector (FBuilder* fb);

int hxcfe_popTrack (FBuilder* fb);

int hxcfe_setRPM(FBuilder* fb,unsigned short rpm);

unsigned short hxcfe_getCurrentNumberOfSector (FBuilder* fb);
unsigned char  hxcfe_getCurrentNumberOfSide (FBuilder* fb);
unsigned short hxcfe_getCurrentNumberOfTrack (FBuilder* fb);
int            hxcfe_getCurrentSectorSize(FBuilder* fb);
unsigned char  hxcfe_getCurrentTrackType (FBuilder* fb);
unsigned short hxcfe_getCurrentRPM (FBuilder* fb);
int            hxcfe_getCurrentSkew (FBuilder* fb);

FLOPPY* hxcfe_getFloppy (FBuilder* fb);

int hxcfe_generateDisk(FBuilder* fb,unsigned char * diskdata,int buffersize);

int hxcfe_getFloppySize(HXCFLOPPYEMULATOR* floppycontext,FLOPPY *fp,int * nbsector);

////////////////////////////////////////////
// Raw File loader /

typedef struct XmlFloppyBuilder_
{
	void * xml_parser;
	void * ad;
}XmlFloppyBuilder;

XmlFloppyBuilder* hxcfe_initXmlFloppy(HXCFLOPPYEMULATOR* floppycontext);
void hxcfe_deinitXmlFloppy(XmlFloppyBuilder* context);

int         hxcfe_numberOfXmlLayout(XmlFloppyBuilder* context);
int         hxcfe_getXmlLayoutID(XmlFloppyBuilder* context,char * container);
const char* hxcfe_getXmlLayoutDesc(XmlFloppyBuilder* context,int moduleID);
const char* hxcfe_getXmlLayoutName(XmlFloppyBuilder* context,int moduleID);

int         hxcfe_selectXmlFloppyLayout(XmlFloppyBuilder* context,int layoutid);

FLOPPY*     hxcfe_generateXmlFloppy (XmlFloppyBuilder* context,unsigned char * rambuffer,unsigned buffersize);
FLOPPY*     hxcfe_generateXmlFileFloppy (XmlFloppyBuilder* context,char *file);

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
SECTORCONFIG* hxcfe_searchSector (SECTORSEARCH* ss,int track,int side,int id,int type);
int hxcfe_getSectorSize(SECTORSEARCH* ss,SECTORCONFIG* sc);
unsigned char * hxcfe_getSectorData(SECTORSEARCH* ss,SECTORCONFIG* sc);

int hxcfe_readSectorData     (SECTORSEARCH* ss,int track,int side,int sector,int numberofsector,int sectorsize,int type,unsigned char * buffer);
int hxcfe_writeSectorData    (SECTORSEARCH* ss,int track,int side,int sector,int numberofsector,int sectorsize,int type,unsigned char * buffer);

void hxcfe_freeSectorConfig  (SECTORSEARCH* ss,SECTORCONFIG* sc);

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


////////////////////////////////////////////
// Track analyser functions

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

////////////////////////////////////////////
// FDC style functions
int hxcfe_FDC_READSECTOR  (HXCFLOPPYEMULATOR* floppycontext,FLOPPY *fp,unsigned char track,unsigned char side,unsigned char sector,int sectorsize,int mode,int nbsector,unsigned char * buffer,int buffer_size);
int hxcfe_FDC_WRITESECTOR (HXCFLOPPYEMULATOR* floppycontext,FLOPPY *fp,unsigned char track,unsigned char side,unsigned char sector,int sectorsize,int mode,int nbsector,unsigned char * buffer,int buffer_size);
int hxcfe_FDC_FORMAT      (HXCFLOPPYEMULATOR* floppycontext,unsigned char track,unsigned char side,unsigned char nbsector,int sectorsize,int sectoridstart,int skew,int interleave,int mode);
int hxcfe_FDC_SCANSECTOR  (HXCFLOPPYEMULATOR* floppycontext,unsigned char track,unsigned char side,int mode,unsigned char * sector,unsigned char * buffer,int buffer_size);

////////////////////////////////////////////
// File system functions

FLOPPY * hxcfe_generateFloppy(HXCFLOPPYEMULATOR* floppycontext,char* path,int fsID,int * err_ret);

enum
{
	FS_720KB_ATARI_FAT12=0,
	FS_902KB_ATARI_FAT12,
	FS_360KB_ATARI_FAT12,
	FS_880KB_AMIGADOS,
	
	FS_5P25_300RPM_160KB_MSDOS_FAT12,
	FS_5P25_360RPM_160KB_MSDOS_FAT12,
	
	FS_5P25_300RPM_180KB_MSDOS_FAT12,
	FS_5P25_360RPM_180KB_MSDOS_FAT12,
		
	FS_5P25_SS_300RPM_320KB_MSDOS_FAT12,
	FS_5P25_SS_360RPM_320KB_MSDOS_FAT12,
		
	FS_5P25_DS_300RPM_320KB_MSDOS_FAT12,
	FS_5P25_DS_360RPM_320KB_MSDOS_FAT12,
		
	FS_5P25_DS_300RPM_360KB_MSDOS_FAT12,
	FS_5P25_DS_360RPM_360KB_MSDOS_FAT12,
		
	FS_3P5_DS_300RPM_640KB_MSDOS_FAT12,
		
	FS_720KB_MSDOS_FAT12,
		
	FS_5P25_300RPM_1200KB_MSDOS_FAT12,
		
	FS_1_44MB_MSDOS_FAT12,
	FS_1_68MB_MSDOS_FAT12,
	FS_2_88MB_MSDOS_FAT12,
	FS_3_38MB_MSDOS_FAT12,
	FS_4_23MB_ATARI_FAT12,
	FS_6_78MB_MSDOS_FAT12,
	FS_16MB_MSDOS_FAT12
};

typedef struct _fs_config
{
	char * name;
	char * desc;
	int		fsID;
	int		type;
}fs_config;


typedef struct _FSMNG
{
	HXCFLOPPYEMULATOR * hxcfe;
	int fs_selected;

	// Mounted Floppy disk
	FLOPPY *fp;

	// mounted disk image geometry
	int sectorpertrack;
	int sidepertrack;
	int trackperdisk;
	int sectorsize;

	void * handletable[128];
	void * dirhandletable[128];
	

}FSMNG;

typedef struct FSENTRY_
{

	int isdir;
	int size;
	char entryname[512];
	unsigned long flags;

}FSENTRY;

extern fs_config fs_config_table[];

int hxcfe_getFSID(HXCFLOPPYEMULATOR* floppycontext, char * fsname);
int	hxcfe_numberOfFS(HXCFLOPPYEMULATOR* floppycontext);
const char* hxcfe_getFSDesc(HXCFLOPPYEMULATOR* floppycontext,int FSID);
const char* hxcfe_getFSName(HXCFLOPPYEMULATOR* floppycontext,int FSID);

FSMNG * hxcfe_initFsManager(HXCFLOPPYEMULATOR * hxcfe);
int hxcfe_selectFS(FSMNG * fsmng, int fsid);


int hxcfe_mountImage(FSMNG * fsmng, FLOPPY *floppy);
int hxcfe_umountImage(FSMNG * fsmng);

int hxcfe_openDir(FSMNG * fsmng, char * path);
int hxcfe_readDir(FSMNG * fsmng,int dirhandle,FSENTRY * dirent);
int hxcfe_closeDir(FSMNG * fsmng, int dirhandle);

int hxcfe_getFirstFile(FSMNG * fsmng, FSENTRY * dirent, char * rootdir);
int hxcfe_getNextFile(FSMNG * fsmng, FSENTRY * dirent);

int hxcfe_openFile(FSMNG * fsmng, char * filename);
int hxcfe_writeFile(FSMNG * fsmng,int filehandle,char * buffer,int size);
int hxcfe_readFile( FSMNG * fsmng,int filehandle,char * buffer,int size);
int hxcfe_deleteFile(FSMNG * fsmng, char * filename);
int hxcfe_closeFile(FSMNG * fsmng, int filehandle);

int hxcfe_fseek( FSMNG * fsmng,int filehandle,long offset,int origin);
int hxcfe_ftell( FSMNG * fsmng,int filehandle);

void hxcfe_deinitFsManager(FSMNG * fsmng);

