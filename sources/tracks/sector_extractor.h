
typedef struct sect_sector_
{
	int32_t type;
	int32_t sectorsize;
	int32_t track_id;
	int32_t side_id;
	int32_t sector_id;
	uint8_t *buffer;
}sect_sector;

typedef struct sect_track_
{
	uint8_t side;
	uint8_t track;
	int32_t number_of_sector;
	sect_sector ** sectorlist;

}sect_track;

typedef struct sect_floppy_
{
	int32_t number_of_side;
	int32_t number_of_track;
	sect_track ** tracklist;
}sect_floppy;

int analysis_and_extract_sector_MFM(HXCFE* floppycontext,HXCFE_SIDE * track,sect_track * sectors);
int analysis_and_extract_sector_AMIGAMFM(HXCFE* floppycontext,HXCFE_SIDE * track,sect_track * sectors);
int analysis_and_extract_sector_FM(HXCFE* floppycontext,HXCFE_SIDE * track,sect_track * sectors);
int analysis_and_extract_sector_EMUIIFM(HXCFE* floppycontext,HXCFE_SIDE * track,sect_track * sectors);

int write_raw_file(HXCFE_IMGLDR * imgldr_ctx,FILE * f,HXCFE_FLOPPY * fp,int32_t startidsector,int32_t sectorpertrack,int32_t nboftrack,int32_t nbofside,int32_t sectorsize,int32_t tracktype,int32_t sidefilelayout);

int count_sector(HXCFE* floppycontext,HXCFE_FLOPPY * fp,int32_t startidsector,int32_t track,int32_t side,int32_t sectorsize,int32_t tracktype);
