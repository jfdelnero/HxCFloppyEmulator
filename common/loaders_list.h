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

#include "../common/plugins/adf_loader/adf_loader.h"
#include "../common/plugins/adz_loader/adz_loader.h"
#include "../common/plugins/dms_loader/dms_loader.h"
#include "../common/plugins/msa_loader/msa_loader.h"
#include "../common/plugins/amigadosfs_loader/amigadosfs_loader.h"
#ifdef IPF_SUPPORT 
	#include "../common/plugins/ipf_loader/ipf_loader.h"
#endif
#include "../common/plugins/st_loader/st_loader.h"
#include "../common/plugins/stx_loader/stx_loader.h"
#include "../common/plugins/cpcdsk_loader/cpcdsk_loader.h"
#include "../common/plugins/img_loader/img_loader.h"
#include "../common/plugins/copyqm_loader/copyqm_loader.h"
#include "../common/plugins/oricdsk_loader/oricdsk_loader.h"
#include "../common/plugins/mfm_loader/mfm_loader.h"
#include "../common/plugins/msx_loader/msx_loader.h"
#include "../common/plugins/fat12floppy_loader/fat12floppy_loader.h"
#include "../common/plugins/smc_loader/snes_smc_loader.h"
#include "../common/plugins/hfe_loader/hfe_loader.h"
#include "../common/plugins/imd_loader/imd_loader.h"
#include "../common/plugins/afi_loader/afi_loader.h"
#include "../common/plugins/d64_loader/d64_loader.h"
#include "../common/plugins/d81_loader/d81_loader.h"
#include "../common/plugins/trd_loader/trd_loader.h"
#include "../common/plugins/scl_loader/scl_loader.h"
#include "../common/plugins/sap_loader/sap_loader.h"
#include "../common/plugins/jv1_loader/jv1_loader.h"
#include "../common/plugins/jv3_loader/jv3_loader.h"
#include "../common/plugins/vtr_loader/vtr_loader.h"
#include "../common/plugins/d88_loader/d88_loader.h"
#include "../common/plugins/hdm_loader/hdm_loader.h"
#include "../common/plugins/ti99pc99_loader/ti99pc99_loader.h"
#include "../common/plugins/ti99v9t9_loader/ti99v9t9_loader.h"
#include "../common/plugins/apridisk_loader/apridisk_loader.h"
#include "../common/plugins/ede_loader/ede_loader.h"
#include "../common/plugins/fd_loader/fd_loader.h"
#include "../common/plugins/vdk_loader/vdk_loader.h"
#include "../common/plugins/dpx_loader/dpx_loader.h"
#include "../common/plugins/ensoniq_mirage_loader/ensoniq_mirage_loader.h"
#include "../common/plugins/emax_loader/emax_loader.h"
#include "../common/plugins/mgt_loader/mgt_loader.h"
#include "../common/plugins/sad_loader/sad_loader.h"
#include "../common/plugins/stt_loader/stt_loader.h"
#include "../common/plugins/prophet_loader/prophet_loader.h"
#include "../common/plugins/teledisk_loader/teledisk_loader.h"
#include "../common/plugins/emuii_raw_loader/emuii_raw_loader.h"
#include "../common/plugins/emuii_loader/emuii_loader.h"
#include "../common/plugins/emui_raw_loader/emui_raw_loader.h"
#include "../common/plugins/jvc_loader/jvc_loader.h"
#include "../common/plugins/dim_loader/dim_loader.h"
#include "../common/plugins/dmk_loader/dmk_loader.h"
#include "../common/plugins/raw_loader/raw_loader.h"
#include "../common/plugins/acornadf_loader/acornadf_loader.h"
#include "../common/plugins/vegasdsk_loader/vegasdsk_loader.h"
#include "../common/plugins/camputerslynxldf_loader/camputerslynxldf_loader.h"
#include "../common/plugins/extadf_loader/extadf_loader.h"
#include "../common/plugins/oldextadf_loader/oldextadf_loader.h"
#include "../common/plugins/fdi_loader/fdi_loader.h"
#include "../common/plugins/adl_loader/adl_loader.h"
#include "../common/plugins/ssd_dsd_loader/ssd_dsd_loader.h"
#include "../common/plugins/krz_loader/krz_loader.h"
#include "../common/plugins/w30_loader/w30_loader.h"
#include "../common/plugins/fei_loader/fei_loader.h"
#include "../common/plugins/svd_loader/svd_loader.h"
#include "../common/plugins/imz_loader/imz_loader.h"
#include "../common/plugins/gkh_loader/gkh_loader.h"

const GETPLUGININFOS staticplugins[]=
{
	(GETPLUGININFOS)DMS_libGetPluginInfo,
	(GETPLUGININFOS)ADZ_libGetPluginInfo,
	(GETPLUGININFOS)EXTADF_libGetPluginInfo,
	(GETPLUGININFOS)OLDEXTADF_libGetPluginInfo,
	(GETPLUGININFOS)FDI_libGetPluginInfo,
	(GETPLUGININFOS)ADF_libGetPluginInfo,
	(GETPLUGININFOS)ACORNADF_libGetPluginInfo,
	(GETPLUGININFOS)CPCDSK_libGetPluginInfo,
	(GETPLUGININFOS)DIM_libGetPluginInfo,
	(GETPLUGININFOS)STX_libGetPluginInfo,
	(GETPLUGININFOS)STT_libGetPluginInfo,
	(GETPLUGININFOS)CopyQm_libGetPluginInfo,
	(GETPLUGININFOS)TeleDisk_libGetPluginInfo,
	(GETPLUGININFOS)MSA_libGetPluginInfo,
	(GETPLUGININFOS)IMZ_libGetPluginInfo,
	(GETPLUGININFOS)MFM_libGetPluginInfo,
	(GETPLUGININFOS)OricDSK_libGetPluginInfo,
	(GETPLUGININFOS)ST_libGetPluginInfo,
	(GETPLUGININFOS)W30_libGetPluginInfo,
	#ifdef IPF_SUPPORT
	(GETPLUGININFOS)IPF_libGetPluginInfo,
	#endif
	(GETPLUGININFOS)TI99V9T9_libGetPluginInfo,
	(GETPLUGININFOS)AMIGADOSFSDK_libGetPluginInfo,
	(GETPLUGININFOS)Prophet_libGetPluginInfo,
	(GETPLUGININFOS)IMG_libGetPluginInfo,
	(GETPLUGININFOS)MSX_libGetPluginInfo,
	(GETPLUGININFOS)FAT12FLOPPY_libGetPluginInfo,
	(GETPLUGININFOS)HFE_libGetPluginInfo,
	(GETPLUGININFOS)EXTHFE_libGetPluginInfo,
	(GETPLUGININFOS)VTR_libGetPluginInfo,
	(GETPLUGININFOS)IMD_libGetPluginInfo,
	(GETPLUGININFOS)AFI_libGetPluginInfo,
	(GETPLUGININFOS)D64_libGetPluginInfo,
	(GETPLUGININFOS)D81_libGetPluginInfo,
	(GETPLUGININFOS)TRD_libGetPluginInfo,
	(GETPLUGININFOS)SCL_libGetPluginInfo,
	(GETPLUGININFOS)SAP_libGetPluginInfo,
	(GETPLUGININFOS)JV1_libGetPluginInfo,
	(GETPLUGININFOS)JV3_libGetPluginInfo,
	(GETPLUGININFOS)JVC_libGetPluginInfo,
	(GETPLUGININFOS)SVD_libGetPluginInfo,
	(GETPLUGININFOS)D88_libGetPluginInfo,
	(GETPLUGININFOS)HDM_libGetPluginInfo,
	(GETPLUGININFOS)RAW_libGetPluginInfo,
	(GETPLUGININFOS)snes_smc_libGetPluginInfo,
	(GETPLUGININFOS)KRZ_libGetPluginInfo,
	(GETPLUGININFOS)VEGASDSK_libGetPluginInfo,
	(GETPLUGININFOS)DMK_libGetPluginInfo,
	(GETPLUGININFOS)TI99PC99_libGetPluginInfo,
	(GETPLUGININFOS)ApriDisk_libGetPluginInfo,
	(GETPLUGININFOS)EDE_libGetPluginInfo,
	(GETPLUGININFOS)GKH_libGetPluginInfo,
	(GETPLUGININFOS)FD_libGetPluginInfo,
	(GETPLUGININFOS)VDK_libGetPluginInfo,
	(GETPLUGININFOS)DPX_libGetPluginInfo,
	(GETPLUGININFOS)Ensoniq_mirage_libGetPluginInfo,
	(GETPLUGININFOS)EMAX_libGetPluginInfo,
	(GETPLUGININFOS)MGT_libGetPluginInfo,
	(GETPLUGININFOS)SAD_libGetPluginInfo,
	(GETPLUGININFOS)EMUII_RAW_libGetPluginInfo,
	(GETPLUGININFOS)EMUII_libGetPluginInfo,
	(GETPLUGININFOS)EMUI_RAW_libGetPluginInfo,
	(GETPLUGININFOS)CAMPUTERSLYNX_libGetPluginInfo,
	(GETPLUGININFOS)ADL_libGetPluginInfo,
	(GETPLUGININFOS)DSD_libGetPluginInfo,
	(GETPLUGININFOS)FEI_libGetPluginInfo,
	(GETPLUGININFOS)-1
};
