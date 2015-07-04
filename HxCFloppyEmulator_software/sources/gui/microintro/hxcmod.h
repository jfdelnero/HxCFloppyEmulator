///////////////////////////////////////////////////////////////////////////////////
//-------------------------------------------------------------------------------//
//-------------------------------------------------------------------------------//
//-----------H----H--X----X-----CCCCC----22222----0000-----0000------11----------//
//----------H----H----X-X-----C--------------2---0----0---0----0--1--1-----------//
//---------HHHHHH-----X------C----------22222---0----0---0----0-----1------------//
//--------H----H----X--X----C----------2-------0----0---0----0-----1-------------//
//-------H----H---X-----X---CCCCC-----222222----0000-----0000----1111------------//
//-------------------------------------------------------------------------------//
//----------------------------------------------------- http://hxc2001.free.fr --//
///////////////////////////////////////////////////////////////////////////////////
// File : hxcmod.h
// Contains: a tiny mod player
//
// Written by: Jean François DEL NERO
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#ifndef MODPLAY_DEF
#define MODPLAY_DEF

typedef struct track_state_
{
	unsigned char instrument_number;
	unsigned short cur_period;
	unsigned char  cur_volume;
	unsigned short cur_effect;
}track_state;

typedef struct tracker_state_
{
	int number_of_tracks;
	int bpm;
	int speed;
	int cur_pattern;
	int cur_pattern_pos;
	int cur_pattern_table_pos;
	unsigned int buf_index;
	track_state tracks[32];
}tracker_state;

typedef struct tracker_state_instrument_
{
	char name[22];
	int active;
}tracker_state_instrument;

typedef struct tracker_buffer_state_
{
	int nb_max_of_state;
	int nb_of_state;
	int cur_rd_index;
	int sample_step;
	char name[64];
	tracker_state_instrument instruments[31];
	tracker_state * track_state_buf;
}tracker_buffer_state;

void * hxcmod_load(void * moddata, int size);
void hxcmod_fillbuffer(void * modctx,unsigned short * buffer, unsigned long nbsample,tracker_buffer_state * trkbuf);
void hxcmod_unload(void * modcontext);

#endif

