/*
//
// Copyright (C) 2006-2026 Jean-François DEL NERO
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "types.h"

#include "internal_libhxcfe.h"
#include "tracks/track_generator.h"
#include "sector_search.h"
#include "fdc_ctrl.h"
#include "fs_manager.h"
#include "libhxcfe.h"
#include "fs_manager_access.h"
#include "fs_manager_list.h"

#include "fs_fat12/fs_fat12.h"
#include "fs_amigados/fs_amigados.h"
#include "fs_cpm/fs_cpm.h"

const fn_fsmng_getplugininfos staticfsplugins[]=
{
	(fn_fsmng_getplugininfos)fat12_libGetPluginInfo,
	(fn_fsmng_getplugininfos)amigados_libGetPluginInfo,
	(fn_fsmng_getplugininfos)cpm_libGetPluginInfo,
	0
};

