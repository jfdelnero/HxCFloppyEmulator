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

#ifdef __cplusplus
extern "C" {
#endif

#include "plugins_id.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// Functions return error codes                                                                   //
////////////////////////////////////////////////////////////////////////////////////////////////////

#define HXCFE_VALIDFILE                  1
#define HXCFE_NOERROR                    0
#define HXCFE_ACCESSERROR               -1
#define HXCFE_BADFILE                   -2
#define HXCFE_FILECORRUPTED             -3
#define HXCFE_BADPARAMETER              -4
#define HXCFE_INTERNALERROR             -5
#define HXCFE_UNSUPPORTEDFILE           -6

////////////////////////////////////////////////////////////////////////////////////////////////////
// Define HxCFE lib types if needed                                                               //
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _HXCFE_
typedef void HXCFE;
#define _HXCFE_
#endif

#ifndef _HXCFE_FLOPPY_
typedef void HXCFE_FLOPPY;
#define _HXCFE_FLOPPY_
#endif

#ifndef _HXCFE_SIDE_
typedef void HXCFE_SIDE;
#define _HXCFE_SIDE_
#endif

#ifndef _HXCFE_IMGLDR_
typedef void HXCFE_IMGLDR;
#define _HXCFE_IMGLDR_
#endif

#ifndef _HXCFE_XMLLDR_
typedef void HXCFE_XMLLDR;
#define _HXCFE_XMLLDR_
#endif

#ifndef _HXCFE_TD_
typedef void HXCFE_TD;
#define _HXCFE_TD_
#endif

#ifndef _HXCFE_TRKSTREAM_
typedef void HXCFE_TRKSTREAM;
#define _HXCFE_TRKSTREAM_
#endif

#ifndef _HXCFE_FXSA_
typedef void HXCFE_FXSA;
#define _HXCFE_FXSA_
#endif

#ifndef _HXCFE_FLPGEN_
typedef void HXCFE_FLPGEN;
#define _HXCFE_FLPGEN_
#endif

#ifndef _HXCFE_SECTORACCESS_
typedef void HXCFE_SECTORACCESS;
#define _HXCFE_SECTORACCESS_
#endif

#ifndef _HXCFE_FSMNG_
typedef void HXCFE_FSMNG;
#define _HXCFE_FSMNG_
#endif

#ifndef _HXCFE_FDCCTRL_
typedef void HXCFE_FDCCTRL;
#define _HXCFE_FDCCTRL_
#endif

#ifndef _HXCFE_SECTCFG_
typedef void HXCFE_SECTCFG;
#define _HXCFE_SECTCFG_
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
// UI Output callback functions                                                                   //
////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _HXCFE_PRINTF_FUNC_
typedef int (*HXCFE_PRINTF_FUNC)(int MSGTYPE,char * string, ...);
#define _HXCFE_PRINTF_FUNC_
#endif

#ifndef _HXCFE_TRACKPOSOUT_FUNC_
typedef int (*HXCFE_TRACKPOSOUT_FUNC)(unsigned int current,unsigned int total);
#define _HXCFE_TRACKPOSOUT_FUNC_
#endif

#ifndef _HXCFE_IMGLDRPROGRESSOUT_FUNC_
typedef int (*HXCFE_IMGLDRPROGRESSOUT_FUNC)(unsigned int current,unsigned int total, void * user);
#define _HXCFE_IMGLDRPROGRESSOUT_FUNC_
#endif

#ifndef _HXCFE_TDPROGRESSOUT_FUNC_
typedef int (*HXCFE_TDPROGRESSOUT_FUNC)(unsigned int current,unsigned int total,void * td, void * user);
#define _HXCFE_TDPROGRESSOUT_FUNC_
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
// Init Function                                                                                  //
////////////////////////////////////////////////////////////////////////////////////////////////////

HXCFE*             hxcfe_init(void);
void               hxcfe_deinit(HXCFE* hxcfe);

const char *       hxcfe_getVersion(HXCFE* floppycontext);
const char *       hxcfe_getLicense(HXCFE* floppycontext);

////////////////////////////////////////////////////////////////////////////////////////////////////
// stdio printf functions                                                                         //
////////////////////////////////////////////////////////////////////////////////////////////////////

// Output Message level
#define MSG_INFO_0                       0
#define MSG_INFO_1                       1
#define MSG_WARNING                      2
#define MSG_ERROR                        3
#define MSG_DEBUG                        4

int                hxcfe_setOutputFunc(HXCFE* floppycontext,HXCFE_PRINTF_FUNC hxc_printf);

////////////////////////////////////////////////////////////////////////////////////////////////////
// File image functions                                                                           //
////////////////////////////////////////////////////////////////////////////////////////////////////
HXCFE_IMGLDR *     hxcfe_imgInitLoader(HXCFE * hxcfe);

int                hxcfe_imgGetNumberOfLoader(HXCFE_IMGLDR * imgldr_ctx);
int                hxcfe_imgGetLoaderID(HXCFE_IMGLDR * imgldr_ctx,char * container);
int                hxcfe_imgGetLoaderAccess(HXCFE_IMGLDR * imgldr_ctx,int moduleID);
const char*        hxcfe_imgGetLoaderDesc(HXCFE_IMGLDR * imgldr_ctx,int moduleID);
const char*        hxcfe_imgGetLoaderName(HXCFE_IMGLDR * imgldr_ctx,int moduleID);
const char*        hxcfe_imgGetLoaderExt(HXCFE_IMGLDR * imgldr_ctx,int moduleID);
int                hxcfe_imgAutoSetectLoader(HXCFE_IMGLDR * imgldr_ctx,char* imgname,int moduleID);
HXCFE_FLOPPY *     hxcfe_imgLoad(HXCFE_IMGLDR * imgldr_ctx,char* imgname,int moduleID,int * err_ret);
HXCFE_FLOPPY *     hxcfe_imgLoadEx(HXCFE_IMGLDR * imgldr_ctx,char* imgname,int moduleID,int * err_ret,void * parameters);
int                hxcfe_imgUnload(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * floppydisk);
int                hxcfe_imgExport(HXCFE_IMGLDR * imgldr_ctx,HXCFE_FLOPPY * newfloppy,char* imgname,int moduleID);
int                hxcfe_imgSetProgressCallback(HXCFE_IMGLDR * imgldr_ctx,HXCFE_IMGLDRPROGRESSOUT_FUNC progress_func,void * userdata);

void               hxcfe_imgDeInitLoader(HXCFE_IMGLDR * imgldr_ctx);

int                hxcfe_getNumberOfTrack(HXCFE* floppycontext,HXCFE_FLOPPY *fp);
int                hxcfe_getNumberOfSide(HXCFE* floppycontext,HXCFE_FLOPPY *fp);

////////////////////////////////////////////////////////////////////////////////////////////////////
// Custom Image/floppy generation functions                                                       //
////////////////////////////////////////////////////////////////////////////////////////////////////

#define IBMFORMAT_SD                     0x01
#define IBMFORMAT_DD                     0x02
#define ISOFORMAT_SD                     0x03
#define ISOFORMAT_DD                     0x04
#define ISOFORMAT_DD11S                  0x05
#define AMIGAFORMAT_DD                   0x06
#define TYCOMFORMAT_SD                   0x07
#define MEMBRAINFORMAT_DD                0x08
#define EMUFORMAT_SD                     0x09
#define APPLE2_GCR5A3                    0x0A
#define APPLE2_GCR6A2                    0x0B
#define ARBURG_DAT                       0x0C
#define ARBURG_SYS                       0x0D
#define UKNCFORMAT_DD                    0x0E
#define DIRECT_ENCODING                  0xFE

HXCFE_FLPGEN*      hxcfe_initFloppy(HXCFE* floppycontext,int nb_of_track,int nb_of_side);

int                hxcfe_setNumberOfTrack (HXCFE_FLPGEN* fb_ctx,unsigned short numberoftrack);
int                hxcfe_setNumberOfSide (HXCFE_FLPGEN* fb_ctx,unsigned char numberofside);
int                hxcfe_setNumberOfSector (HXCFE_FLPGEN* fb_ctx,unsigned short numberofsector);
int                hxcfe_setSectorSize(HXCFE_FLPGEN* fb_ctx,int size);
int                hxcfe_setStartSectorID(HXCFE_FLPGEN* fb_ctx,unsigned char startsectorid);
int                hxcfe_setTrackType(HXCFE_FLPGEN* fb_ctx,int type);

int                hxcfe_pushTrack (HXCFE_FLPGEN* fb_ctx,unsigned int rpm,int number,int side,int type);
int                hxcfe_pushTrackPFS (HXCFE_FLPGEN* fb_ctx,int number,int side);

int                hxcfe_setTrackInterleave (HXCFE_FLPGEN* fb_ctx,int interleave);
int                hxcfe_setTrackSkew (HXCFE_FLPGEN* fb_ctx,int skew);

int                hxcfe_setTrackPreGap (HXCFE_FLPGEN* fb_ctx,unsigned short pregap);

int                hxcfe_setIndexPosition (HXCFE_FLPGEN* fb_ctx,int position,int allowsector);
int                hxcfe_setIndexLength (HXCFE_FLPGEN* fb_ctx,int Length);

int                hxcfe_setTrackBitrate (HXCFE_FLPGEN* fb_ctx,int bitrate);

int                hxcfe_addSector (HXCFE_FLPGEN* fb_ctx,int sectornumber,int side,int track,unsigned char * buffer,int size);

int                hxcfe_addSectors(HXCFE_FLPGEN* fb_ctx,int side,int track,unsigned char * trackdata,int buffersize,int numberofsectors);

int                hxcfe_pushSector (HXCFE_FLPGEN* fb_ctx);

int                hxcfe_setSectorBitrate (HXCFE_FLPGEN* fb_ctx,int bitrate);

int                hxcfe_setSectorGap3 (HXCFE_FLPGEN* fb_ctx,unsigned char Gap3);
int                hxcfe_setSectorSizeID (HXCFE_FLPGEN* fb_ctx,unsigned char sectorsizeid);
int                hxcfe_setSectorFill (HXCFE_FLPGEN* fb_ctx,unsigned char fill);

int                hxcfe_setSectorTrackID(HXCFE_FLPGEN* fb_ctx,unsigned char track);
int                hxcfe_setSectorHeadID(HXCFE_FLPGEN* fb_ctx,unsigned char head);
int                hxcfe_setSectorID(HXCFE_FLPGEN* fb_ctx,unsigned char id);

int                hxcfe_setSectorEncoding (HXCFE_FLPGEN* fb_ctx,int encoding);

int                hxcfe_setSectorDataCRC (HXCFE_FLPGEN* fb_ctx,unsigned short crc);
int                hxcfe_setSectorHeaderCRC (HXCFE_FLPGEN* fb_ctx,unsigned short crc);

int                hxcfe_setSectorDataMark (HXCFE_FLPGEN* fb_ctx,unsigned char datamark);

int                hxcfe_setSectorData(HXCFE_FLPGEN* fb_ctx,unsigned char * buffer,int size);

int                hxcfe_popSector (HXCFE_FLPGEN* fb_ctx);

int                hxcfe_popTrack (HXCFE_FLPGEN* fb_ctx);

int                hxcfe_setRPM(HXCFE_FLPGEN* fb_ctx,unsigned short rpm);

unsigned short     hxcfe_getCurrentNumberOfSector (HXCFE_FLPGEN* fb_ctx);
unsigned char      hxcfe_getCurrentNumberOfSide (HXCFE_FLPGEN* fb_ctx);
unsigned short     hxcfe_getCurrentNumberOfTrack (HXCFE_FLPGEN* fb_ctx);
int                hxcfe_getCurrentSectorSize(HXCFE_FLPGEN* fb_ctx);
unsigned char      hxcfe_getCurrentTrackType (HXCFE_FLPGEN* fb_ctx);
unsigned short     hxcfe_getCurrentRPM (HXCFE_FLPGEN* fb_ctx);
int                hxcfe_getCurrentSkew (HXCFE_FLPGEN* fb_ctx);

HXCFE_FLOPPY*      hxcfe_getFloppy (HXCFE_FLPGEN* fb_ctx);

int                hxcfe_generateDisk(HXCFE_FLPGEN* fb_ctx,unsigned char * diskdata,int buffersize);

int                hxcfe_getFloppySize(HXCFE* floppycontext,HXCFE_FLOPPY *fp,int * nbsector);

////////////////////////////////////////////////////////////////////////////////////////////////////
// XML based Raw File loader                                                                      //
////////////////////////////////////////////////////////////////////////////////////////////////////

HXCFE_XMLLDR*      hxcfe_initXmlFloppy(HXCFE* floppycontext);

int                hxcfe_numberOfXmlLayout(HXCFE_XMLLDR* xmlfb_ctx);
int                hxcfe_getXmlLayoutID(HXCFE_XMLLDR* xmlfb_ctx,char * container);
const char*        hxcfe_getXmlLayoutDesc(HXCFE_XMLLDR* xmlfb_ctx,int moduleID);
const char*        hxcfe_getXmlLayoutName(HXCFE_XMLLDR* xmlfb_ctx,int moduleID);

int                hxcfe_selectXmlFloppyLayout(HXCFE_XMLLDR* xmlfb_ctx,int layoutid);
int                hxcfe_setXmlFloppyLayoutFile(HXCFE_XMLLDR* xmlfb_ctx,char * filepath);

HXCFE_FLOPPY*      hxcfe_generateXmlFloppy (HXCFE_XMLLDR* xmlfb_ctx,unsigned char * rambuffer,unsigned buffersize);
HXCFE_FLOPPY*      hxcfe_generateXmlFileFloppy (HXCFE_XMLLDR* xmlfb_ctx,char *file);

void               hxcfe_deinitXmlFloppy(HXCFE_XMLLDR* xmlfb_ctx);

////////////////////////////////////////////////////////////////////////////////////////////////////
// Sector Search,Read,Write functions                                                             //
////////////////////////////////////////////////////////////////////////////////////////////////////

HXCFE_SECTORACCESS* hxcfe_initSectorAccess(HXCFE* floppycontext,HXCFE_FLOPPY *fp);

HXCFE_SECTCFG*     hxcfe_getNextSector(HXCFE_SECTORACCESS* ss_ctx,int track,int side,int type);
HXCFE_SECTCFG*     hxcfe_searchSector (HXCFE_SECTORACCESS* ss_ctx,int track,int side,int id,int type);
HXCFE_SECTCFG**    hxcfe_getAllTrackSectors(HXCFE_SECTORACCESS* ss_ctx,int track,int side,int type,int * nb_sectorfound);
HXCFE_SECTCFG**    hxcfe_getAllTrackISOSectors(HXCFE_SECTORACCESS* ss_ctx,int track,int side,int * nb_sectorfound);
int                hxcfe_getSectorSize(HXCFE_SECTORACCESS* ss_ctx,HXCFE_SECTCFG* sc);
unsigned char *    hxcfe_getSectorData(HXCFE_SECTORACCESS* ss_ctx,HXCFE_SECTCFG* sc);

int                hxcfe_readSectorData     (HXCFE_SECTORACCESS* ss_ctx,int track,int side,int sector,int numberofsector,int sectorsize,int type,unsigned char * buffer,int * fdcstatus);
int                hxcfe_writeSectorData    (HXCFE_SECTORACCESS* ss_ctx,int track,int side,int sector,int numberofsector,int sectorsize,int type,unsigned char * buffer,int * fdcstatus);

void               hxcfe_freeSectorConfig  (HXCFE_SECTORACCESS* ss_ctx,HXCFE_SECTCFG* sc);

void               hxcfe_deinitSectorAccess(HXCFE_SECTORACCESS* ss_ctx);

////////////////////////////////////////////////////////////////////////////////////////////////////
// FDC style functions                                                                            //
////////////////////////////////////////////////////////////////////////////////////////////////////

#define FDC_NOERROR                      0x00
#define FDC_BAD_DATA_CRC                 0x01
#define FDC_NO_DATA                      0x02
#define FDC_SECTOR_NOT_FOUND             0x03

HXCFE_FDCCTRL *    hxcfe_initFDC (HXCFE* floppycontext);

int                hxcfe_insertDiskFDC (HXCFE_FDCCTRL * fdc,HXCFE_FLOPPY *fp);
int                hxcfe_readSectorFDC (HXCFE_FDCCTRL * fdc,unsigned char track,unsigned char side,unsigned char sector,int sectorsize,int mode,int nbsector,unsigned char * buffer,int buffer_size,int * fdcstatus);
int                hxcfe_writeSectorFDC (HXCFE_FDCCTRL * fdc,unsigned char track,unsigned char side,unsigned char sector,int sectorsize,int mode,int nbsector,unsigned char * buffer,int buffer_size,int * fdcstatus);

void               hxcfe_deinitFDC (HXCFE_FDCCTRL * fdc);

int                hxcfe_FDC_READSECTOR  (HXCFE* floppycontext,HXCFE_FLOPPY *fp,unsigned char track,unsigned char side,unsigned char sector,int sectorsize,int mode,int nbsector,unsigned char * buffer,int buffer_size,int * fdcstatus);
int                hxcfe_FDC_WRITESECTOR (HXCFE* floppycontext,HXCFE_FLOPPY *fp,unsigned char track,unsigned char side,unsigned char sector,int sectorsize,int mode,int nbsector,unsigned char * buffer,int buffer_size,int * fdcstatus);
int                hxcfe_FDC_FORMAT      (HXCFE* floppycontext,unsigned char track,unsigned char side,unsigned char nbsector,int sectorsize,int sectoridstart,int skew,int interleave,int mode,int * fdcstatus);
int                hxcfe_FDC_SCANSECTOR  (HXCFE* floppycontext,unsigned char track,unsigned char side,int mode,unsigned char * sector,unsigned char * buffer,int buffer_size,int * fdcstatus);

////////////////////////////////////////////////////////////////////////////////////////////////////
// Floppy interfaces setting functions.                                                           //
////////////////////////////////////////////////////////////////////////////////////////////////////

// Floppy Interface modes

#define IBMPC_DD_FLOPPYMODE              0x00
#define IBMPC_HD_FLOPPYMODE              0x01
#define ATARIST_DD_FLOPPYMODE            0x02
#define ATARIST_HD_FLOPPYMODE            0x03
#define AMIGA_DD_FLOPPYMODE              0x04
#define AMIGA_HD_FLOPPYMODE              0x05
#define CPC_DD_FLOPPYMODE                0x06
#define GENERIC_SHUGART_DD_FLOPPYMODE    0x07
#define IBMPC_ED_FLOPPYMODE              0x08
#define MSX2_DD_FLOPPYMODE               0x09
#define C64_DD_FLOPPYMODE                0x0A
#define EMU_SHUGART_FLOPPYMODE           0x0B
#define S950_DD_FLOPPYMODE               0x0C
#define S950_HD_FLOPPYMODE               0x0D

// Track Encoding types

#define ISOIBM_MFM_ENCODING              0x00
#define AMIGA_MFM_ENCODING               0x01
#define ISOIBM_FM_ENCODING               0x02
#define EMU_FM_ENCODING                  0x03
#define TYCOM_FM_ENCODING                0x04
#define MEMBRAIN_MFM_ENCODING            0x05
#define APPLEII_GCR1_ENCODING            0x06
#define APPLEII_GCR2_ENCODING            0x07
#define APPLEII_HDDD_A2_GCR1_ENCODING    0x08
#define APPLEII_HDDD_A2_GCR2_ENCODING    0x09
#define ARBURGDAT_ENCODING               0x0A
#define ARBURGSYS_ENCODING               0x0B

#define UNKNOWN_ENCODING                 0xFF

enum {
    DOUBLESTEP = 1,
    INTERFACEMODE = 2
};

enum {
    SET = 0,
    GET = 1
};

int                hxcfe_floppyGetSetParams(HXCFE* floppycontext,HXCFE_FLOPPY * newfloppy,unsigned char dir,unsigned short param,void * value);

int                hxcfe_floppyGetInterfaceMode(HXCFE* floppycontext,HXCFE_FLOPPY * newfloppy);
int                hxcfe_floppySetInterfaceMode(HXCFE* floppycontext,HXCFE_FLOPPY * newfloppy,int ifmode);

int                hxcfe_floppyGetDoubleStep(HXCFE* floppycontext,HXCFE_FLOPPY * newfloppy);
int                hxcfe_floppySetDoubleStep(HXCFE* floppycontext,HXCFE_FLOPPY * newfloppy,int doublestep);

int                hxcfe_getFloppyInterfaceModeID(HXCFE* floppycontext,char * ifmode);

const char *       hxcfe_getFloppyInterfaceModeName(HXCFE* floppycontext,int ifmodeid);
const char *       hxcfe_getFloppyInterfaceModeDesc(HXCFE* floppycontext,int ifmodeid);
const char *       hxcfe_getTrackEncodingName(HXCFE* floppycontext,int trackencodingid);

////////////////////////////////////////////////////////////////////////////////////////////////////
// Track analyser functions                                                                       //
////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct s_sectorlist_
{
    HXCFE_SECTCFG * sectorconfig;

    int side,track;

    // track mode
    int x_pos1,y_pos1;
    int x_pos2,y_pos2;

    // disc mode
    float start_angle,end_angle;
    int   diameter,thickness;

    struct s_sectorlist_ * next_element;
}s_sectorlist;

typedef struct s_pulseslist_
{
    int side,track;

    int pulse_number;

    int x_pos1,x_pos2;

    struct s_pulseslist_ * next_element;
}s_pulseslist;

HXCFE_TD*          hxcfe_td_init(HXCFE* floppycontext,unsigned long xsize,unsigned long ysize);
void               hxcfe_td_setparams(HXCFE_TD *td,unsigned long x_us,unsigned long y_us,unsigned long x_start_us);
void               hxcfe_td_activate_analyzer(HXCFE_TD *td,int TRACKTYPE,int enable);
void               hxcfe_td_draw_track(HXCFE_TD *td,HXCFE_FLOPPY * floppydisk,int track,int side);
s_sectorlist *     hxcfe_td_getlastsectorlist(HXCFE_TD *td);
void               hxcfe_td_draw_disk(HXCFE_TD *td,HXCFE_FLOPPY * floppydisk);
void *             hxcfe_td_getframebuffer(HXCFE_TD *td);
int                hxcfe_td_getframebuffer_xres(HXCFE_TD *td);
int                hxcfe_td_getframebuffer_yres(HXCFE_TD *td);
int                hxcfe_td_setProgressCallback(HXCFE_TD *td,HXCFE_TDPROGRESSOUT_FUNC progress_func,void * userdata);
void               hxcfe_td_deinit(HXCFE_TD *td);
s_pulseslist *     hxcfe_td_getlastpulselist(HXCFE_TD *td);

////////////////////////////////////////////////////////////////////////////////////////////////////
// Flux Stream Analyzer                                                                           //
////////////////////////////////////////////////////////////////////////////////////////////////////

HXCFE_FXSA *       hxcfe_initFxStream(HXCFE * hxcfe);

void               hxcfe_FxStream_setResolution(HXCFE_FXSA * fxs,int step);
void               hxcfe_FxStream_setBitrate(HXCFE_FXSA * fxs,int bitrate);
void               hxcfe_FxStream_setPhaseCorrectionFactor(HXCFE_FXSA * fxs,int phasefactor);

HXCFE_TRKSTREAM *  hxcfe_FxStream_ImportStream(HXCFE_FXSA * fxs,void * stream,int wordsize,unsigned int nbword);
void               hxcfe_FxStream_AddIndex(HXCFE_FXSA * fxs,HXCFE_TRKSTREAM * std,unsigned long streamposition);
HXCFE_SIDE *       hxcfe_FxStream_AnalyzeAndGetTrack(HXCFE_FXSA * fxs,HXCFE_TRKSTREAM * std);
void               hxcfe_FxStream_FreeStream(HXCFE_FXSA * fxs,HXCFE_TRKSTREAM * stream);

int                hxcfe_FxStream_GetNumberOfRevolution(HXCFE_FXSA * fxs,HXCFE_TRKSTREAM * std);
unsigned long      hxcfe_FxStream_GetRevolutionPeriod(HXCFE_FXSA * fxs,HXCFE_TRKSTREAM * std,int revolution);
unsigned long      hxcfe_FxStream_GetMeanRevolutionPeriod(HXCFE_FXSA * fxs,HXCFE_TRKSTREAM * std);

void               hxcfe_deinitFxStream(HXCFE_FXSA * fxs);

////////////////////////////////////////////////////////////////////////////////////////////////////
// Track edition functions                                                                        //
////////////////////////////////////////////////////////////////////////////////////////////////////

HXCFE_SIDE *       hxcfe_getSide(HXCFE_FLOPPY * fp,int track,int side);
int                hxcfe_getTrackBitrate(HXCFE_FLOPPY * fp,int track,int side);
unsigned char      hxcfe_getTrackEncoding(HXCFE_FLOPPY * fp,int track,int side);
unsigned long      hxcfe_getTrackLength(HXCFE_FLOPPY * fp,int track,int side);
unsigned short     hxcfe_getTrackRPM(HXCFE_FLOPPY * fp,int track);
unsigned char      hxcfe_getTrackNumberOfSide(HXCFE_FLOPPY * fp,int track);

int                hxcfe_shiftTrackData(HXCFE_SIDE * side,long bitoffset);

void               AdjustTrackPeriod(HXCFE* floppycontext,HXCFE_SIDE * curside_S0,HXCFE_SIDE * curside_S1);

int                hxcfe_getCellState(HXCFE* floppycontext,HXCFE_SIDE * currentside,unsigned long cellnumber);
int                hxcfe_setCellState(HXCFE* floppycontext,HXCFE_SIDE * currentside,unsigned long cellnumber,int state);

int                hxcfe_removeCell(HXCFE* floppycontext,HXCFE_SIDE * currentside,unsigned long cellnumber);
int                hxcfe_insertCell(HXCFE* floppycontext,HXCFE_SIDE * currentside,unsigned long cellnumber,int state);

int                hxcfe_getCellFlakeyState(HXCFE* floppycontext,HXCFE_SIDE * currentside,unsigned long cellnumber);
int                hxcfe_setCellFlakeyState(HXCFE* floppycontext,HXCFE_SIDE * currentside,unsigned long cellnumber,int state);

int                hxcfe_getCellIndexState(HXCFE* floppycontext,HXCFE_SIDE * currentside,unsigned long cellnumber);
int                hxcfe_setCellIndexState(HXCFE* floppycontext,HXCFE_SIDE * currentside,unsigned long cellnumber,int state);

int                hxcfe_getCellBitrate(HXCFE* floppycontext,HXCFE_SIDE * currentside,unsigned long cellnumber);
int                hxcfe_setCellBitrate(HXCFE* floppycontext,HXCFE_SIDE * currentside,unsigned long cellnumber,unsigned long bitrate);

unsigned char      hxcfe_getSectorConfigEncoding(HXCFE* floppycontext,HXCFE_SECTCFG* sc);
unsigned char      hxcfe_getSectorConfigSectorID(HXCFE* floppycontext,HXCFE_SECTCFG* sc);
unsigned char      hxcfe_getSectorConfigSideID(HXCFE* floppycontext,HXCFE_SECTCFG* sc);
unsigned char      hxcfe_getSectorConfigSizeID(HXCFE* floppycontext,HXCFE_SECTCFG* sc);
unsigned char      hxcfe_getSectorConfigTrackID(HXCFE* floppycontext,HXCFE_SECTCFG* sc);
unsigned short     hxcfe_getSectorConfigHCRC(HXCFE* floppycontext,HXCFE_SECTCFG* sc);
unsigned short     hxcfe_getSectorConfigDCRC(HXCFE* floppycontext,HXCFE_SECTCFG* sc);
unsigned int       hxcfe_getSectorConfigSectorSize(HXCFE* floppycontext,HXCFE_SECTCFG* sc);
unsigned long      hxcfe_getSectorConfigStartSectorIndex(HXCFE* floppycontext,HXCFE_SECTCFG* sc);
unsigned long      hxcfe_getSectorConfigStartDataIndex(HXCFE* floppycontext,HXCFE_SECTCFG* sc);
unsigned long      hxcfe_getSectorConfigEndSectorIndex(HXCFE* floppycontext,HXCFE_SECTCFG* sc);
unsigned char *    hxcfe_getSectorConfigInputData(HXCFE* floppycontext,HXCFE_SECTCFG* sc);
unsigned char      hxcfe_getSectorConfigDataMark(HXCFE* floppycontext,HXCFE_SECTCFG* sc);
unsigned char      hxcfe_getSectorConfigHCRCStatus(HXCFE* floppycontext,HXCFE_SECTCFG* sc);
unsigned char      hxcfe_getSectorConfigDCRCStatus(HXCFE* floppycontext,HXCFE_SECTCFG* sc);

////////////////////////////////////////////////////////////////////////////////////////////////////
// File system functions                                                                          //
////////////////////////////////////////////////////////////////////////////////////////////////////

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
    FS_16MB_MSDOS_FAT12,
    FS_4_50MB_MSDOS_FAT12,
    FS_2_50MB_MSDOS_FAT12
};

typedef struct FSENTRY_
{

    int isdir;
    int size;
    char entryname[512];
    unsigned long flags;

}HXCFE_FSENTRY;

int                hxcfe_getFSID(HXCFE* floppycontext, char * fsname);
int                hxcfe_numberOfFS(HXCFE* floppycontext);
const char*        hxcfe_getFSDesc(HXCFE* floppycontext,int FSID);
const char*        hxcfe_getFSName(HXCFE* floppycontext,int FSID);

HXCFE_FLOPPY *     hxcfe_generateFloppy(HXCFE* floppycontext,char* path,int fsID,int * err_ret);

HXCFE_FSMNG *      hxcfe_initFsManager(HXCFE * hxcfe);

int                hxcfe_selectFS(HXCFE_FSMNG * fsmng, int fsid);

int                hxcfe_mountImage(HXCFE_FSMNG * fsmng, HXCFE_FLOPPY *floppy);
int                hxcfe_umountImage(HXCFE_FSMNG * fsmng);

int                hxcfe_getFreeFsSpace(HXCFE_FSMNG * fsmng);
int                hxcfe_getTotalFsSpace(HXCFE_FSMNG * fsmng);

int                hxcfe_openDir(HXCFE_FSMNG * fsmng, char * path);
int                hxcfe_readDir(HXCFE_FSMNG * fsmng,int dirhandle,HXCFE_FSENTRY * dirent);
int                hxcfe_closeDir(HXCFE_FSMNG * fsmng, int dirhandle);

int                hxcfe_getFirstFile(HXCFE_FSMNG * fsmng, HXCFE_FSENTRY * dirent, char * rootdir);
int                hxcfe_getNextFile(HXCFE_FSMNG * fsmng, HXCFE_FSENTRY * dirent);

int                hxcfe_openFile(HXCFE_FSMNG * fsmng, char * filename);
int                hxcfe_createFile(HXCFE_FSMNG * fsmng, char * filename);
int                hxcfe_writeFile(HXCFE_FSMNG * fsmng,int filehandle,unsigned char * buffer,int size);
int                hxcfe_readFile(HXCFE_FSMNG * fsmng,int filehandle,unsigned char * buffer,int size);
int                hxcfe_deleteFile(HXCFE_FSMNG * fsmng, char * filename);
int                hxcfe_closeFile(HXCFE_FSMNG * fsmng, int filehandle);

int                hxcfe_fseek( HXCFE_FSMNG * fsmng,int filehandle,long offset,int origin);
int                hxcfe_ftell( HXCFE_FSMNG * fsmng,int filehandle);

int                hxcfe_createDir( HXCFE_FSMNG * fsmng,char * foldername);
int                hxcfe_removeDir( HXCFE_FSMNG * fsmng,char * foldername);

void               hxcfe_deinitFsManager(HXCFE_FSMNG * fsmng);

#ifdef __cplusplus
}
#endif
