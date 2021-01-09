typedef struct stathisto_
{
	uint32_t val;
	uint32_t occurence;
	float pourcent;
}stathisto;

typedef struct pulses_link_
{
	int32_t * forward_link;
	int32_t * backward_link;
	int32_t number_of_pulses;
}pulses_link;

typedef struct pulsesblock_
{
	int32_t timeoffset;
	int64_t tickoffset;

	int32_t timelength;
	int32_t ticklength;
	int32_t start_index;
	int32_t end_index;
	int32_t number_of_pulses;

	int32_t state;
	int32_t overlap_offset;
	int32_t overlap_size;

	int32_t locked;
}pulsesblock;

typedef struct track_blocks_
{

	pulsesblock * blocks;

	uint32_t number_of_blocks;

}track_blocks;


typedef struct s_match_
{
	int32_t yes;
	int32_t no;
	int32_t offset;
}s_match;

HXCFE_SIDE* ScanAndDecodeStream(HXCFE* floppycontext,HXCFE_FXSA * fxs, int initialvalue,HXCFE_TRKSTREAM * track,pulses_link * pl,uint32_t start_index, short rpm,int phasecorrection, int flags);
int cleanupTrack(HXCFE_SIDE *curside);
HXCFE_FLOPPY * makefloppyfromtrack(HXCFE_SIDE * side);
void freefloppy(HXCFE_FLOPPY * fp);
void computehistogram(uint32_t *indata,int size,uint32_t *outdata);
int detectpeaks(HXCFE* floppycontext, pll_stat *pll, uint32_t *histogram);
