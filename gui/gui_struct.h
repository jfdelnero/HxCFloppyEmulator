
typedef struct _guicontext
{
	char bufferfilename[4096];
	char buffertext[4096];

	char * logfile;

	int loadstatus;

	unsigned char autoselectmode;
	unsigned char twistedcable;
}guicontext;


typedef struct track_type_
{
	int id;
	char * name;
	int tracktype;
}track_type;

enum
{
	FM_TRACK_TYPE,
	FMIBM_TRACK_TYPE,
	MFM_TRACK_TYPE,
	MFMIBM_TRACK_TYPE,
	GCR_TRACK_TYPE
};
