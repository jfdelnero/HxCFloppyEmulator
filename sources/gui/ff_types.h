
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
	FF_V9T9,
	FF_EHFE
};