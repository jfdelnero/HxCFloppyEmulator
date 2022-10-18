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
void hxcfe_FxStream_JitterFilter(HXCFE_FXSA * fxs,HXCFE_TRKSTREAM * stream);

typedef struct streamconv_
{
	HXCFE* hxcfe;

	uint32_t stream_in_mode;
	uint32_t bitstream_pos;
	uint32_t start_bitstream_pos;
	uint32_t end_bitstream_pos;

	int      start_revolution;
	int      end_revolution;

	HXCFE_SIDE * track;
	HXCFE_FXSA * fxs;

	float    stream_period_ps;
	uint64_t stream_time_offset_ps;
	uint64_t stream_prev_time_offset_ps;
	uint64_t stream_total_time_ps;

	float    overflow_value;
	double   conv_error;
	int      rollover;

	int      current_revolution;

	uint8_t  index_state;
	uint8_t  old_index_state;
	uint8_t  index_event;
	uint8_t  stream_end_event;

	int      stream_source;
}streamconv;

streamconv * initStreamConvert(HXCFE* hxcfe, HXCFE_SIDE * track, float stream_period_ps, float overflowvalue,int start_revolution,float start_offset,int end_revolution,float end_offset);
uint32_t StreamConvert_getNextPulse(streamconv * sc);
uint32_t StreamConvert_search_index(streamconv * sc, int index);
uint32_t StreamConvert_setPosition(streamconv * sc, int revolution, float offset);
void deinitStreamConvert(streamconv * sc);
