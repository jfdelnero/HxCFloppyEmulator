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

#include "internal_floppy.h"
#include "plugins_id.h"

typedef int (*HXCFE_PRINTF_FUNC)(int MSGTYPE,char * string, ...);
#define _HXCFE_PRINTF_FUNC_

typedef int (*HXCFE_TRACKPOSOUT_FUNC)(unsigned int current,unsigned int total);
#define _HXCFE_TRACKPOSOUT_FUNC_

typedef int (*HXCFE_IMGLDRPROGRESSOUT_FUNC)(unsigned int current,unsigned int total, void * user);
#define _HXCFE_IMGLDRPROGRESSOUT_FUNC_

typedef int (*HXCFE_TDPROGRESSOUT_FUNC)(unsigned int current,unsigned int total,void * td, void * user);
#define _HXCFE_TDPROGRESSOUT_FUNC_

typedef struct _HXCFE
{
	HXCFE_PRINTF_FUNC hxc_printf;
	HXCFE_TRACKPOSOUT_FUNC hxc_settrackpos;
	char CONTAINERTYPE[16];
}HXCFE;

#define _HXCFE_


typedef struct _HXCFE_IMGLDR
{
	HXCFE * hxcfe;
	HXCFE_IMGLDRPROGRESSOUT_FUNC hxc_setprogress;
	void * progress_userdata;
}HXCFE_IMGLDR;

#define _HXCFE_IMGLDR_


typedef struct _HXCFE_XMLLDR
{
	void * xml_parser;
	void * ad;
}HXCFE_XMLLDR;

#define _HXCFE_XMLLDR_


typedef struct _HXCFE_TD
{
    HXCFE * hxcfe;

    int xsize,ysize;
    int x_us,y_us;
    int x_start_us;
    unsigned int * framebuffer;

    void * sl;

    unsigned long enabledtrackmode;

    void * pl;

    HXCFE_TDPROGRESSOUT_FUNC hxc_setprogress;
    void * progress_userdata;
}HXCFE_TD;

#define _HXCFE_TD_


typedef struct _s_index_evt
{
    unsigned long dump_offset;
    unsigned long cellpos;
    unsigned long clk;
}s_index_evt;


typedef struct _HXCFE_TRKSTREAM
{
    unsigned long	* track_dump;
    unsigned long	nb_of_pulses;
    s_index_evt		index_evt_tab[32];
    unsigned long	nb_of_index;
}HXCFE_TRKSTREAM;

#define _HXCFE_TRKSTREAM_


typedef struct _HXCFE_FXSA
{
    HXCFE * hxcfe;

    // step resolution (ps)
    int steptime;

    int phasecorrection;

    int defaultbitrate;

}HXCFE_FXSA;

#define _HXCFE_FXSA_


typedef struct _fs_config
{
    char * name;
    char * desc;
    int    fsID;
    int    type;
}fs_config;

extern fs_config fs_config_table[];

int hxcfe_imgCallProgressCallback(HXCFE_IMGLDR * imgldr_ctx,int cur,int max);
