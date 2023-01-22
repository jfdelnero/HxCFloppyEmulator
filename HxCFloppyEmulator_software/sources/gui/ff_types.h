
typedef struct ff_type_
{
	int id;
	const char * name;
	const char * plug_id;
	const char * ext;
}ff_type;

enum
{
	FF_HFE=0,
	FF_MFM,
	FF_AFI,
	FF_VTR,
	FF_RAW,
	FF_IMD,
	FF_ADF,
	FF_ADZ,
	FF_JV3,
	FF_DMK,
	FF_VDK,
	FF_TRD,
	FF_SDDSPECCY,
	FF_V9T9,
	FF_D88,
	FF_ST,
	FF_MSA,
	FF_DIM,
	FF_STX,
	FF_STW,
	FF_FD,
	FF_HDDDA2_HFE,
	FF_EHFE,
	FF_HFEV3,
	FF_HFESTREAM,
	FF_ARBG,
	FF_SKF,
	FF_IPF,
	FF_SCP,
	FF_BMP,
	FF_DISK_BMP,
	FF_XML,
	FF_AMSTRADDSK,
	FF_ORICDSK,
	FF_NORTHSTAR,
	FF_HEATHKIT,
	FF_HXCQD,
	FF_APPLEII_DO,
	FF_APPLEII_PO,
	FF_FDX68_RAWFDX
};
