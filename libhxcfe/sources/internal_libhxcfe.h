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

#include "internal_floppy.h"


typedef int (*HXCFE_PRINTF_FUNC)(int MSGTYPE, const char * string, ...);
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

	void * image_handlers;

	void * envvar;

	void * scriptctx;

	char * license;
}HXCFE;

#define _HXCFE_


typedef struct _HXCFE_IMGLDR
{
	HXCFE * hxcfe;
	HXCFE_IMGLDRPROGRESSOUT_FUNC hxc_setprogress;
	void * progress_userdata;

	int selected_id;
	int selected_subid;

}HXCFE_IMGLDR;

typedef struct _HXCFE_IMGLDR_FILEINFOS
{
	int is_dir;
	char path[1024];
	char file_extension[32];
	int file_size;
	unsigned char file_header[512];
}HXCFE_IMGLDR_FILEINFOS;

int32_t hxcfe_preloadImgInfos(HXCFE_IMGLDR * imgldr_ctx, char * imgname, HXCFE_IMGLDR_FILEINFOS * file_infos);
int32_t hxcfe_imgCheckFileCompatibility( HXCFE_IMGLDR * imgldr_ctx, HXCFE_IMGLDR_FILEINFOS * file_infos, char * loadername, char * fileext, int filesizemod);

#define _HXCFE_IMGLDR_

typedef struct _HXCFE_XMLLDR
{
	void * xml_parser;
	void * ad;
}HXCFE_XMLLDR;

#define _HXCFE_XMLLDR_

#define MAX_MARKER 32

typedef struct _HXCFE_TD_MARKER
{
	int32_t cell_pos;

	uint32_t type;

	uint32_t color;

	uint32_t flags;

}HXCFE_TD_MARKER;

typedef struct _HXCFE_TD
{
	HXCFE * hxcfe;

	int32_t    xsize,ysize;
	int32_t    x_us,y_us;
	int32_t    x_start_us;
	uint32_t   flags;
	uint32_t * framebuffer;
	void * sl;

	uint32_t enabledtrackmode;
	int32_t  disk_type;
	int32_t  bands_type;

	uint32_t noloop_trackmode;

	void * pl;

	HXCFE_TDPROGRESSOUT_FUNC hxc_setprogress;

	char * name;

	void * progress_userdata;

	HXCFE_TD_MARKER markers[MAX_MARKER];
}HXCFE_TD;

#define _HXCFE_TD_

typedef struct pll_stat_
{
	// current cell size (1 cell size)
	int32_t pump_charge;

	// current window phase
	int32_t phase;

	// center value
	int32_t pll_max;
	int32_t pivot;
	int32_t pll_min;

	int32_t last_error;

	// last pulse phase
	int32_t lastpulsephase;

	int     tick_freq;
	int     pll_min_max_percent;

	int     fast_correction_ratio_n,fast_correction_ratio_d;
	int     slow_correction_ratio_n,slow_correction_ratio_d;

	int     inter_band_rejection;

	int     max_pll_error_ticks;

	int     band_mode;
	int     bands_separators[16];
	int     track;
	int     side;
}pll_stat;

typedef struct _HXCFE_FXSA
{
	HXCFE * hxcfe;

	// step resolution (ps)
	int32_t steptime;

	int32_t phasecorrection;

	int32_t defaultbitrate;

	int32_t filter;
	int32_t filterpasses;

	int     analysis_window_size;
	int     analysis_rev2rev_max_pulses_jitter;

	pll_stat pll;

	int     out_index_length_us;

	int     weak_cell_threshold;

	int32_t sector_recovery;
}HXCFE_FXSA;

#define _HXCFE_FXSA_


typedef struct _fs_config
{
	char * name;
	char * desc;
	int32_t    fsID;
	int32_t    type;
}fs_config;

extern fs_config fs_config_table[];

int32_t hxcfe_imgCallProgressCallback( HXCFE_IMGLDR * imgldr_ctx, int32_t cur, int32_t max );
