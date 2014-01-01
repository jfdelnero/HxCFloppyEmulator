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

#include "./loaders/adf_loader/adf_loader.h"
#include "./loaders/adz_loader/adz_loader.h"
#include "./loaders/dms_loader/dms_loader.h"
#include "./loaders/msa_loader/msa_loader.h"
#include "./loaders/amigadosfs_loader/amigadosfs_loader.h"
#ifdef IPF_SUPPORT 
	#include "./loaders/ipf_loader/ipf_loader.h"
#endif
#include "./loaders/st_loader/st_loader.h"
#include "./loaders/stx_loader/stx_loader.h"
#include "./loaders/cpcdsk_loader/cpcdsk_loader.h"
#include "./loaders/img_loader/img_loader.h"
#include "./loaders/copyqm_loader/copyqm_loader.h"
#include "./loaders/oricdsk_loader/oricdsk_loader.h"
#include "./loaders/mfm_loader/mfm_loader.h"
#include "./loaders/msx_loader/msx_loader.h"
#include "./loaders/fat12floppy_loader/fat12floppy_loader.h"
#include "./loaders/smc_loader/snes_smc_loader.h"
#include "./loaders/hfe_loader/hfe_loader.h"
#include "./loaders/imd_loader/imd_loader.h"
#include "./loaders/afi_loader/afi_loader.h"
#include "./loaders/d64_loader/d64_loader.h"
#include "./loaders/d81_loader/d81_loader.h"
#include "./loaders/trd_loader/trd_loader.h"
#include "./loaders/scl_loader/scl_loader.h"
#include "./loaders/sap_loader/sap_loader.h"
#include "./loaders/jv1_loader/jv1_loader.h"
#include "./loaders/jv3_loader/jv3_loader.h"
#include "./loaders/vtr_loader/vtr_loader.h"
#include "./loaders/d88_loader/d88_loader.h"
#include "./loaders/hdm_loader/hdm_loader.h"
#include "./loaders/ti99pc99_loader/ti99pc99_loader.h"
#include "./loaders/ti99v9t9_loader/ti99v9t9_loader.h"
#include "./loaders/apridisk_loader/apridisk_loader.h"
#include "./loaders/ede_loader/ede_loader.h"
#include "./loaders/fd_loader/fd_loader.h"
#include "./loaders/vdk_loader/vdk_loader.h"
#include "./loaders/dpx_loader/dpx_loader.h"
#include "./loaders/ensoniq_mirage_loader/ensoniq_mirage_loader.h"
#include "./loaders/emax_loader/emax_loader.h"
#include "./loaders/mgt_loader/mgt_loader.h"
#include "./loaders/sad_loader/sad_loader.h"
#include "./loaders/stt_loader/stt_loader.h"
#include "./loaders/prophet_loader/prophet_loader.h"
#include "./loaders/teledisk_loader/teledisk_loader.h"
#include "./loaders/emuii_raw_loader/emuii_raw_loader.h"
#include "./loaders/emuii_loader/emuii_loader.h"
#include "./loaders/emui_raw_loader/emui_raw_loader.h"
#include "./loaders/jvc_loader/jvc_loader.h"
#include "./loaders/dim_loader/dim_loader.h"
#include "./loaders/dmk_loader/dmk_loader.h"
#include "./loaders/raw_loader/raw_loader.h"
#include "./loaders/acornadf_loader/acornadf_loader.h"
#include "./loaders/vegasdsk_loader/vegasdsk_loader.h"
#include "./loaders/camputerslynxldf_loader/camputerslynxldf_loader.h"
#include "./loaders/extadf_loader/extadf_loader.h"
#include "./loaders/oldextadf_loader/oldextadf_loader.h"
#include "./loaders/fdi_loader/fdi_loader.h"
#include "./loaders/adl_loader/adl_loader.h"
#include "./loaders/ssd_dsd_loader/ssd_dsd_loader.h"
#include "./loaders/krz_loader/krz_loader.h"
#include "./loaders/w30_loader/w30_loader.h"
#include "./loaders/fei_loader/fei_loader.h"
#include "./loaders/fzf_loader/fzf_loader.h"
#include "./loaders/svd_loader/svd_loader.h"
#include "./loaders/imz_loader/imz_loader.h"
#include "./loaders/gkh_loader/gkh_loader.h"
#include "./loaders/kryofluxstream_loader/kryofluxstream_loader.h"
#include "./loaders/system24_loader/system24_loader.h"
#include "./loaders/apple2_nib_loader/apple2_nib_loader.h"
#include "./loaders/sdd_speccydos_loader/sdd_speccydos_loader.h"
#include "./loaders/apple2_do_loader/apple2_do_loader.h"
#include "./loaders/arburg_raw_loader/arburg_raw_loader.h"
#include "./loaders/scp_loader/scp_loader.h"
#include "./loaders/bmp_loader/bmp_loader.h"
#include "./loaders/xml_loader/xml_loader.h"

const GETPLUGININFOS staticplugins[]=
{
	(GETPLUGININFOS)KryoFluxStream_libGetPluginInfo,

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
	(GETPLUGININFOS)HFE_HDDD_A2_libGetPluginInfo,
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
	//(GETPLUGININFOS)KRZ_libGetPluginInfo,
	(GETPLUGININFOS)VEGASDSK_libGetPluginInfo,
	(GETPLUGININFOS)DMK_libGetPluginInfo,
	(GETPLUGININFOS)TI99PC99_libGetPluginInfo,
	(GETPLUGININFOS)ApriDisk_libGetPluginInfo,
	(GETPLUGININFOS)EDE_libGetPluginInfo,
	(GETPLUGININFOS)FAT12FLOPPY_libGetPluginInfo,
	(GETPLUGININFOS)GKH_libGetPluginInfo,
	(GETPLUGININFOS)FD_libGetPluginInfo,
	(GETPLUGININFOS)FZF_libGetPluginInfo,
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
	(GETPLUGININFOS)System24_libGetPluginInfo,
	(GETPLUGININFOS)SCP_libGetPluginInfo,
	(GETPLUGININFOS)Apple2_nib_libGetPluginInfo,
	(GETPLUGININFOS)Apple2_do_libGetPluginInfo,
	(GETPLUGININFOS)SDDSpeccyDos_libGetPluginInfo,
	(GETPLUGININFOS)BMP_libGetPluginInfo,
	(GETPLUGININFOS)ARBURG_RAW_libGetPluginInfo,
	(GETPLUGININFOS)XML_libGetPluginInfo,
	(GETPLUGININFOS)-1
};
