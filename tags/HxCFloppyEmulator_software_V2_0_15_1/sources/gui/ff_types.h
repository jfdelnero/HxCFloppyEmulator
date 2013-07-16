
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
	FF_JV3,
	FF_DMK,
	FF_V9T9,
	FF_D88,
	FF_MSA,
	FF_HDDDA2_HFE,
	FF_EHFE
};