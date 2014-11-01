#define STACK_SIZE 0x80
#define NUMBEROFSECTOR_MAX 0x200

typedef struct _fb_track_state
{
	HXCFE_SECTCFG sectorconfig;
	int track_number;
	unsigned char side_number;
	unsigned char interleave;
	unsigned char start_sector_id;
	unsigned int sectors_size;
	unsigned char skew;
	unsigned char type;
	unsigned short rpm;
	unsigned short pregap;
	int bitrate;

	int indexlen;
	int indexpos;
	int sectorunderindex;

	int numberofsector_min;
	int numberofsector;
	HXCFE_SECTCFG sectortab[NUMBEROFSECTOR_MAX];

	int sc_stack_pointer;
	HXCFE_SECTCFG sc_stack[36];
}fb_track_state;

#ifndef _HXCFE_FLPGEN_

typedef struct _HXCFE_FLPGEN
{
	HXCFE_FLOPPY * floppydisk;
	int fb_stack_pointer;
	fb_track_state * fb_stack;
}HXCFE_FLPGEN;

#define _HXCFE_FLPGEN_
#endif
