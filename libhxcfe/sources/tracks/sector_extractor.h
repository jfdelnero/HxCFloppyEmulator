
typedef struct sect_sector_
{
	int type;
	int sectorsize;
	int track_id;
	int side_id;
	int sector_id;
	unsigned char *buffer;
}sect_sector;

typedef struct sect_track_
{
	unsigned char side;
	unsigned char track;
	int number_of_sector;
	sect_sector ** sectorlist;

}sect_track;

typedef struct sect_floppy_
{
	int number_of_side;
	int number_of_track;
	sect_track ** tracklist;
}sect_floppy;

int analysis_and_extract_sector_MFM(HXCFE* floppycontext,HXCFE_SIDE * track,sect_track * sectors);
int analysis_and_extract_sector_AMIGAMFM(HXCFE* floppycontext,HXCFE_SIDE * track,sect_track * sectors);
int analysis_and_extract_sector_FM(HXCFE* floppycontext,HXCFE_SIDE * track,sect_track * sectors);
int analysis_and_extract_sector_EMUIIFM(HXCFE* floppycontext,HXCFE_SIDE * track,sect_track * sectors);

