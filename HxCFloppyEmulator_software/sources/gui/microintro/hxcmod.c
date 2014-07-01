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
// File : hxcmod.c
// Contains: a tiny mod player
//
// Written by: Jean François DEL NERO
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include "hxcmod.h"

///////////////////////////////////////////////////////////////////////////////////

#define EFFECT_ARPEGGIO            0x0 // Supported
#define EFFECT_PORTAMENTO_UP       0x1 // Supported
#define EFFECT_PORTAMENTO_DOWN     0x2 // Supported
#define EFFECT_TONE_PORTAMENTO     0x3 // Supported
#define EFFECT_VIBRATO             0x4 // Supported
#define EFFECT_VOLSLIDE_TONEPORTA  0x5 // Supported
#define EFFECT_VOLSLIDE_VIBRATO    0x6 // Supported
#define EFFECT_VOLSLIDE_TREMOLO    0x7 // - TO BE DONE -
#define EFFECT_SET_PANNING         0x8 // - TO BE DONE -
#define EFFECT_SET_OFFSET          0x9 // Supported
#define EFFECT_VOLUME_SLIDE        0xA // Supported
#define EFFECT_JUMP_POSITION       0xB // Supported
#define EFFECT_SET_VOLUME          0xC // Supported
#define EFFECT_PATTERN_BREAK       0xD // Supported

#define EFFECT_EXTENDED		       0xE
#define EFFECT_E_FINE_PORTA_UP       0x1 // Supported
#define EFFECT_E_FINE_PORTA_DOWN     0x2 // Supported
#define EFFECT_E_GLISSANDO_CTRL      0x3 // - TO BE DONE -
#define EFFECT_E_VIBRATO_WAVEFORM    0x4 // - TO BE DONE -
#define EFFECT_E_SET_FINETUNE        0x5 // - TO BE DONE -
#define EFFECT_E_PATTERN_LOOP        0x6 // - TO BE DONE -
#define EFFECT_E_TREMOLO_WAVEFORM    0x7 // - TO BE DONE -
#define EFFECT_E_SET_PANNING_2       0x8 // - TO BE DONE -
#define EFFECT_E_RETRIGGER_NOTE      0x9 // - TO BE DONE -
#define EFFECT_E_FINE_VOLSLIDE_UP    0xA // Supported
#define EFFECT_E_FINE_VOLSLIDE_DOWN  0xB // Supported
#define EFFECT_E_NOTE_CUT            0xC // - TO BE DONE -
#define EFFECT_E_NOTE_DELAY          0xD // - TO BE DONE -
#define EFFECT_E_PATTERN_DELAY       0xE // - TO BE DONE -
#define EFFECT_E_INVERT_LOOP         0xF // - TO BE DONE -
#define EFFECT_SET_SPEED           0xF0 // Supported
#define EFFECT_SET_TEMPO           0xF2 // Supported

#define NUMMAXCHANNELS 32

#define MAXNOTES 36

///////////////////////////////////////////////////////////////////////////////////

typedef unsigned char	muchar;
typedef unsigned short	muint;
typedef unsigned long	mulong;

///////////////////////////////////////////////////////////////////////////////////

static short periodtable[]=
{
	856,808,762,720,678,640,604,570,538,508,480,453,
	428,404,381,360,339,320,302,285,269,254,240,226,
	214,202,190,180,170,160,151,143,135,127,120,113,
	850,802,757,715,674,637,601,567,535,505,477,450,
	425,401,379,357,337,318,300,284,268,253,239,225,
	213,201,189,179,169,159,150,142,134,126,119,113,
	844,796,752,709,670,632,597,563,532,502,474,447,
	422,398,376,355,335,316,298,282,266,251,237,224,
	211,199,188,177,167,158,149,141,133,125,118,112,
	838,791,746,704,665,628,592,559,528,498,470,444,
	419,395,373,352,332,314,296,280,264,249,235,222,
	209,198,187,176,166,157,148,140,132,125,118,111,
	832,785,741,699,660,623,588,555,524,495,467,441,
	416,392,370,350,330,312,294,278,262,247,233,220,
	208,196,185,175,165,156,147,139,131,124,117,110,
	826,779,736,694,655,619,584,551,520,491,463,437,
	413,390,368,347,328,309,292,276,260,245,232,219,
	206,195,184,174,164,155,146,138,130,123,116,109,
	820,774,730,689,651,614,580,547,516,487,460,434,
	410,387,365,345,325,307,290,274,258,244,230,217,
	205,193,183,172,163,154,145,137,129,122,115,109,
	814,768,725,684,646,610,575,543,513,484,457,431,
	407,384,363,342,323,305,288,272,256,242,228,216,
	204,192,181,171,161,152,144,136,128,121,114,108,
	907,856,808,762,720,678,640,604,570,538,508,480,
	453,428,404,381,360,339,320,302,285,269,254,240,
	226,214,202,190,180,170,160,151,143,135,127,120,
	900,850,802,757,715,675,636,601,567,535,505,477,
	450,425,401,379,357,337,318,300,284,268,253,238,
	225,212,200,189,179,169,159,150,142,134,126,119,
	894,844,796,752,709,670,632,597,563,532,502,474,
	447,422,398,376,355,335,316,298,282,266,251,237,
	223,211,199,188,177,167,158,149,141,133,125,118,
	887,838,791,746,704,665,628,592,559,528,498,470,
	444,419,395,373,352,332,314,296,280,264,249,235,
	222,209,198,187,176,166,157,148,140,132,125,118,
	881,832,785,741,699,660,623,588,555,524,494,467,
	441,416,392,370,350,330,312,294,278,262,247,233,
	220,208,196,185,175,165,156,147,139,131,123,117,
	875,826,779,736,694,655,619,584,551,520,491,463,
	437,413,390,368,347,328,309,292,276,260,245,232,
	219,206,195,184,174,164,155,146,138,130,123,116,
	868,820,774,730,689,651,614,580,547,516,487,460,
	434,410,387,365,345,325,307,290,274,258,244,230,
	217,205,193,183,172,163,154,145,137,129,122,115,
	862,814,768,725,684,646,610,575,543,513,484,457,
	431,407,384,363,342,323,305,288,272,256,242,228,
	216,203,192,181,171,161,152,144,136,128,121,114
};

static short sintable[]={
	0, 24, 49, 74, 97,120,141,161,
	180,197,212,224,235,244,250,253,
	255,253,250,244,235,224,212,197,
	180,161,141,120, 97, 74, 49, 24
};

///////////////////////////////////////////////////////////////////////////////////

#pragma pack(1)

typedef struct {
	muchar  name[22];
	muint   length;
	muchar  finetune;
	muchar  volume;
	muint   reppnt;
	muint   replen;
} sample;

typedef struct {
	muchar  sampperiod;
	muchar  period;
	muchar  sampeffect;
	muchar  effect;
} note;

typedef struct {
	muchar  title[20];
	sample  samples[31];
	muchar  length;
	muchar  protracker;
	muchar  patterntable[128];
	muchar  signature[4];
	muchar  speed;
} module;

#pragma pack()

///////////////////////////////////////////////////////////////////////////////////

typedef struct {
	char *  sampdata;
	muint   sampnum;
	muint   length;
	muint   reppnt;
	muint   replen;
	muint   period;
	muchar  volume;
	mulong  ticks;
	muchar  effect;
	muchar  parameffect;

	short   decalperiod;
	short   portaspeed;
	short   portaperiod;
	short   vibraperiod;
	short   Arpperiods[3];
	muchar  ArpIndex;

	short   oldk;
	muchar  oldoffsetconfig;
	muchar  volumeslide;

	muchar  vibraparam;
	muchar  vibrapointeur;

	muchar  finetune;
} channel;

typedef struct {
	module  song;
	char *  sampledata[31];
	note *  patterndata[64];

	mulong  playrate;
	muint   tablepos;
	muint   patternpos;
	muchar  bpm;
	mulong  patternticks;
	mulong  patterntickse;
	mulong  patternticksaim;
	mulong  sampleticksconst;

	mulong  samplenb;

	channel channels[NUMMAXCHANNELS];

	muint   number_of_channels;

} modcontext;

///////////////////////////////////////////////////////////////////////////////////

static modcontext g_modcontext;

///////////////////////////////////////////////////////////////////////////////////

static void memcopy(void * dest,void *source,unsigned long size)
{
	unsigned long i;
	unsigned char * d,*s;
	d=(unsigned char*)dest;
	s=(unsigned char*)source;
	for(i=0;i<size;i++)
	{
		d[i]=s[i];
	}
}

static void memclear(void * dest,unsigned char value,unsigned long size)
{
	unsigned long i;
	unsigned char * d;
	d=(unsigned char*)dest;
	for(i=0;i<size;i++)
	{
		d[i]=value;
	}
}

static int getnote(unsigned short period,int finetune,short * perdiodtable)
{
	short * fineperiodtable;
	int i;

	fineperiodtable = &perdiodtable[(finetune&0xF) * MAXNOTES];

    for(i = 0; i < MAXNOTES; i++)
    {
        if(period >= fineperiodtable[i])
        {
            return i;
        }
    }

	return MAXNOTES;
}

static void worknote(note * nptr, channel * cptr,char t,modcontext * mod)
{
	muint sample, period, effect, operiod;
	muint curnote, arpnote;

	sample = (nptr->sampperiod & 0xF0) | (nptr->sampeffect >> 4);
	period = ((nptr->sampperiod & 0xF) << 8) | nptr->period;
	effect = ((nptr->sampeffect & 0xF) << 8) | nptr->effect;

	operiod=cptr->period;

	if (period != 0 || sample != 0)
	{
		if (sample != 0 && sample<32)
		{
			cptr->sampnum = sample-1;
		}

		if (period != 0 || sample != 0)
		{
			cptr->sampdata =(char *) mod->sampledata[cptr->sampnum];
			cptr->length = mod->song.samples[cptr->sampnum].length;
			cptr->reppnt = mod->song.samples[cptr->sampnum].reppnt;
			cptr->replen = mod->song.samples[cptr->sampnum].replen;

			cptr->finetune = mod->song.samples[cptr->sampnum].finetune;

			if(effect>>8!=4 && effect>>8!=6)
			{
				cptr->vibraperiod=0;
				cptr->vibrapointeur=0;
			}

		}

		if((sample != 0) && (effect>>8) != EFFECT_VOLSLIDE_TONEPORTA)
		{
			cptr->volume = mod->song.samples[cptr->sampnum].volume;
		}

		if(( (effect>>8) != EFFECT_TONE_PORTAMENTO && (effect>>8)!=EFFECT_VOLSLIDE_TONEPORTA) /*&& (period != 0 || sample != 0)*/)
		{
			if (period!=0)
				cptr->ticks = 0;
		}

		cptr->decalperiod=0;
		if(period!=0)
		{
			cptr->period = period;
		}
	}

	cptr->effect = 0;
	cptr->parameffect = 0;

	switch (effect >> 8)
	{
		case EFFECT_ARPEGGIO:
			/*
			[0]: Arpeggio
			Where [0][x][y] means "play note, note+x semitones, note+y
			semitones, then return to original note". The fluctuations are
			carried out evenly spaced in one pattern division. They are usually
			used to simulate chords, but this doesn't work too well. They are
			also used to produce heavy vibrato. A major chord is when x=4, y=7.
			A minor chord is when x=3, y=7.
			*/

			if(effect&0xff)
			{
				cptr->effect = EFFECT_ARPEGGIO;
				cptr->parameffect = effect&0xff;

				cptr->ArpIndex = 0;

				curnote = getnote(cptr->period,cptr->finetune,(short*)&periodtable);

				cptr->Arpperiods[0] = cptr->period;

				arpnote = curnote + (((cptr->parameffect>>4)&0xF));
				if(arpnote>=MAXNOTES) arpnote = MAXNOTES - 1;
				cptr->Arpperiods[1] = periodtable[((cptr->finetune&0xF)*MAXNOTES) + arpnote ];

				arpnote = curnote + (((cptr->parameffect)&0xF));
				if(arpnote>=MAXNOTES) arpnote = MAXNOTES - 1;
				cptr->Arpperiods[2] = periodtable[((cptr->finetune&0xF)*MAXNOTES) + arpnote];
			}
		break;

		case EFFECT_PORTAMENTO_UP:
			/*
			[1]: Slide up
			Where [1][x][y] means "smoothly decrease the period of current
			sample by x*16+y after each tick in the division". The
			ticks/division are set with the 'set speed' effect (see below). If
			the period of the note being played is z, then the final period
			will be z - (x*16 + y)*(ticks - 1). As the slide rate depends on
			the speed, changing the speed will change the slide. You cannot
			slide beyond the note B3 (period 113).
			*/

			cptr->effect = EFFECT_PORTAMENTO_UP;
			cptr->parameffect = effect&0xff;
		break;

		case EFFECT_PORTAMENTO_DOWN:
			/*
			[2]: Slide down
			Where [2][x][y] means "smoothly increase the period of current
			sample by x*16+y after each tick in the division". Similar to [1],
			but lowers the pitch. You cannot slide beyond the note C1 (period
			856).
			*/

			cptr->effect = EFFECT_PORTAMENTO_DOWN;
			cptr->parameffect=effect&0xff;
		break;

		case EFFECT_TONE_PORTAMENTO:
			/*
			[3]: Slide to note
			Where [3][x][y] means "smoothly change the period of current sample
			by x*16+y after each tick in the division, never sliding beyond
			current period". The period-length in this channel's division is a
			parameter to this effect, and hence is not played. Sliding to a
			note is similar to effects [1] and [2], but the slide will not go
			beyond the given period, and the direction is implied by that
			period. If x and y are both 0, then the old slide will continue.
			*/

			cptr->effect = EFFECT_TONE_PORTAMENTO;
			if((effect&0xff)!=0)
			{
				cptr->portaspeed=(short)(effect&0xff);
			}

			if(period!=0)
			{
				cptr->portaperiod=period;
				cptr->period=operiod;
			}
		break;

		case EFFECT_VIBRATO:
			/*
			[4]: Vibrato
			Where [4][x][y] means "oscillate the sample pitch using a
			particular waveform with amplitude y/16 semitones, such that (x *
			ticks)/64 cycles occur in the division". The waveform is set using
			effect [14][4]. By placing vibrato effects on consecutive
			divisions, the vibrato effect can be maintained. If either x or y
			are 0, then the old vibrato values will be used.
			*/

			cptr->effect = EFFECT_VIBRATO;
			if( ( effect & 0xff ) != 0 )
				cptr->vibraparam = effect & 0xff;
		break;

		case EFFECT_VOLSLIDE_TONEPORTA:
			/*
			[5]: Continue 'Slide to note', but also do Volume slide
			Where [5][x][y] means "either slide the volume up x*(ticks - 1) or
			slide the volume down y*(ticks - 1), at the same time as continuing
			the last 'Slide to note'". It is illegal for both x and y to be
			non-zero. You cannot slide outside the volume range 0..64. The
			period-length in this channel's division is a parameter to this
			effect, and hence is not played.
			*/

			cptr->effect=EFFECT_VOLSLIDE_TONEPORTA;
			if( ( effect & 0xFF ) != 0 )
				cptr->volumeslide = ( effect & 0xFF );
		break;

		case EFFECT_VOLSLIDE_VIBRATO:
			/*
			[6]: Continue 'Vibrato', but also do Volume slide
			Where [6][x][y] means "either slide the volume up x*(ticks - 1) or
			slide the volume down y*(ticks - 1), at the same time as continuing
			the last 'Vibrato'". It is illegal for both x and y to be non-zero.
			You cannot slide outside the volume range 0..64.
			*/

			cptr->effect=EFFECT_VOLSLIDE_VIBRATO;
			if( (effect & 0xFF) != 0 )
				cptr->volumeslide = (effect & 0xFF);
		break;

		case EFFECT_SET_OFFSET:
			/*
			[9]: Set sample offset
			Where [9][x][y] means "play the sample from offset x*4096 + y*256".
			The offset is measured in words. If no sample is given, yet one is
			still playing on this channel, it should be retriggered to the new
			offset using the current volume.
			*/

			if((effect & 0xFF)!=0)
			{
				cptr->ticks=((effect & 0xFF) * 256)*((cptr->period - cptr->decalperiod )/mod->sampleticksconst);
				cptr->oldoffsetconfig=(effect & 0xFF);
			}
			else
			{
				cptr->ticks=((cptr->oldoffsetconfig) * 256)*((cptr->period - cptr->decalperiod )/mod->sampleticksconst);

			}
		break;

		case EFFECT_VOLUME_SLIDE:
			/*
			[10]: Volume slide
			Where [10][x][y] means "either slide the volume up x*(ticks - 1) or
			slide the volume down y*(ticks - 1)". If both x and y are non-zero,
			then the y value is ignored (assumed to be 0). You cannot slide
			outside the volume range 0..64.
			*/

			cptr->effect = EFFECT_VOLUME_SLIDE;
			cptr->volumeslide = (effect & 0xFF);
		break;

		case EFFECT_JUMP_POSITION:
			/*
			[11]: Position Jump
			Where [11][x][y] means "stop the pattern after this division, and
			continue the song at song-position x*16+y". This shifts the
			'pattern-cursor' in the pattern table (see above). Legal values for
			x*16+y are from 0 to 127.
			*/

			mod->tablepos = (effect & 0xFF);
		break;

		case EFFECT_SET_VOLUME:
			/*
			[12]: Set volume
			Where [12][x][y] means "set current sample's volume to x*16+y".
			Legal volumes are 0..64.
			*/

			cptr->volume = (effect & 0xFF);
		break;

		case EFFECT_PATTERN_BREAK:
			/*
			[13]: Pattern Break
			Where [13][x][y] means "stop the pattern after this division, and
			continue the song at the next pattern at division x*10+y" (the 10
			is not a typo). Legal divisions are from 0 to 63 (note Protracker
			exception above).
			*/

			mod->patternpos = ( ((effect>>4)&0xF)*10 + (effect&0xF) ) * mod->number_of_channels;
			mod->tablepos++;
		break;

		case EFFECT_EXTENDED:
			switch((effect>>4)&0xF)
			{
				case EFFECT_E_FINE_PORTA_UP:
					/*
					[14][1]: Fineslide up
					Where [14][1][x] means "decrement the period of the current sample
					by x". The incrementing takes place at the beginning of the
					division, and hence there is no actual sliding. You cannot slide
					beyond the note B3 (period 113).
					*/

					cptr->period -= (effect & 0xF);
					if(cptr->period < 113)
						cptr->period = 113;
				break;

				case EFFECT_E_FINE_PORTA_DOWN:
					/*
					[14][2]: Fineslide down
					Where [14][2][x] means "increment the period of the current sample
					by x". Similar to [14][1] but shifts the pitch down. You cannot
					slide beyond the note C1 (period 856).
					*/

					cptr->period += (effect & 0xF);
					if(cptr->period > 856)
						cptr->period = 856;
				break;


				case EFFECT_E_FINE_VOLSLIDE_UP:
					/*
					[14][10]: Fine volume slide up
					Where [14][10][x] means "increment the volume of the current sample
					by x". The incrementing takes place at the beginning of the
					division, and hence there is no sliding. You cannot slide beyond
					volume 64.
					*/

					cptr->volume += (effect & 0xF);
					if( cptr->volume>64 )
						cptr->volume = 64;
				break;

				case EFFECT_E_FINE_VOLSLIDE_DOWN:
					/*
					[14][11]: Fine volume slide down
					Where [14][11][x] means "decrement the volume of the current sample
					by x". Similar to [14][10] but lowers volume. You cannot slide
					beyond volume 0.
					*/

					cptr->volume -= (effect & 0xF);
					if( cptr->volume > 200 )
						cptr->volume = 0;
				break;
				default:

				break;
			}
		break;

		case 0xF:
			/*
			[15]: Set speed
			Where [15][x][y] means "set speed to x*16+y". Though it is nowhere
			near that simple. Let z = x*16+y. Depending on what values z takes,
			different units of speed are set, there being two: ticks/division
			and beats/minute (though this one is only a label and not strictly
			true). If z=0, then what should technically happen is that the
			module stops, but in practice it is treated as if z=1, because
			there is already a method for stopping the module (running out of
			patterns). If z<=32, then it means "set ticks/division to z"
			otherwise it means "set beats/minute to z" (convention says that
			this should read "If z<32.." but there are some composers out there
			that defy conventions). Default values are 6 ticks/division, and
			125 beats/minute (4 divisions = 1 beat). The beats/minute tag is
			only meaningful for 6 ticks/division. To get a more accurate view
			of how things work, use the following formula:
									 24 * beats/minute
				  divisions/minute = -----------------
									  ticks/division
			Hence divisions/minute range from 24.75 to 6120, eg. to get a value
			of 2000 divisions/minute use 3 ticks/division and 250 beats/minute.
			If multiple "set speed" effects are performed in a single division,
			the ones on higher-numbered channels take precedence over the ones
			on lower-numbered channels. This effect has a large number of
			different implementations, but the one described here has the
			widest usage.
			*/

			if((effect&0xFF)<0x21)
			{
				mod->song.speed = effect&0xFF;
				mod->patternticksaim = (long)mod->song.speed * ((mod->playrate * 5 ) / (((long)2 * (long)mod->bpm)));
			}

			if((effect&0xFF)>=0x21)
			{
				///	 HZ = 2 * BPM / 5
				mod->bpm = effect&0xFF;
				mod->patternticksaim = (long)mod->song.speed * ((mod->playrate * 5 ) / (((long)2 * (long)mod->bpm)));
			}

		break;

		default:
		// Unsupported effect
		break;

	}


}

static void workeffect(note * nptr, channel * cptr)
{
	short t;

	switch(cptr->effect)
	{
		case EFFECT_ARPEGGIO:

			if(cptr->parameffect)
			{
				cptr->decalperiod = cptr->period - cptr->Arpperiods[cptr->ArpIndex];

				cptr->ArpIndex++;
				if(cptr->ArpIndex>2)
					cptr->ArpIndex = 0;
			}
		break;

		case EFFECT_PORTAMENTO_UP:

			cptr->period -= cptr->parameffect;

			if(cptr->period < 113 || cptr->period > 856)
				cptr->period = 113;

		break;

		case EFFECT_PORTAMENTO_DOWN:

			cptr->period += cptr->parameffect;

			if(cptr->period > 856)
				cptr->period = 856;

		break;

		case EFFECT_VOLSLIDE_TONEPORTA:
		case EFFECT_TONE_PORTAMENTO:

			if(cptr->period != cptr->portaperiod)
			{
				if( cptr->period > cptr->portaperiod )
				{
					if(cptr->period - cptr->portaperiod >= cptr->portaspeed)
					{
						cptr->period -= cptr->portaspeed;
					}
					else
					{
						cptr->period = cptr->portaperiod;
					}
				}
				else
				{
					if( cptr->portaperiod - cptr->period >= cptr->portaspeed)
					{
						cptr->period += cptr->portaspeed;
					}
					else
					{
						cptr->period = cptr->portaperiod;
					}
				}
			}

			if(cptr->effect==EFFECT_VOLSLIDE_TONEPORTA)
			{
				if(cptr->volumeslide>0x0F)
				{
					cptr->volume = cptr->volume + (cptr->volumeslide>>4);

					if(cptr->volume>63)
						cptr->volume = 63;

				}
				else
				{
					cptr->volume = cptr->volume - (cptr->volumeslide);

					if(cptr->volume>63)
						cptr->volume=0;
				}

			}
		break;

		case EFFECT_VOLSLIDE_VIBRATO:
		case EFFECT_VIBRATO:

			t = cptr->period - cptr->decalperiod - cptr->vibraperiod;

			cptr->vibraperiod=((cptr->vibraparam&0xF)*sintable[cptr->vibrapointeur&0x1F])>>7;

			if(cptr->vibrapointeur>31) cptr->vibraperiod=-cptr->vibraperiod;

			cptr->vibrapointeur=(cptr->vibrapointeur+((cptr->vibraparam>>4)&0xf))&0x1F;

			cptr->ticks= ( ( ( cptr->period - cptr->decalperiod - cptr->vibraperiod) *cptr->ticks)/t);

			if(cptr->effect == EFFECT_VOLSLIDE_VIBRATO)
			{
				if(cptr->volumeslide>0xF)
				{
					cptr->volume=cptr->volume+(cptr->volumeslide>>4);

					if(cptr->volume>64)cptr->volume=64;
				}
				else
				{
					cptr->volume=cptr->volume-(cptr->volumeslide);

					if(cptr->volume>63)cptr->volume=0;
				}
			}

		break;

		case EFFECT_VOLUME_SLIDE:

			if(cptr->volumeslide>0xF)
			{
				cptr->volume += (cptr->volumeslide>>4);

				if( cptr->volume > 64 )
					cptr->volume = 64;
			}
			else
			{
				cptr->volume -= (cptr->volumeslide&0xf);

				if( cptr->volume > 64 )
					cptr->volume = 0;
			}
		break;

		default:
		break;

	}

}

///////////////////////////////////////////////////////////////////////////////////

void * hxcmod_load(void * moddata)
{
	muint	i, max;
	unsigned short t;
	sample	*sptr;
	modcontext * mod;
	unsigned char * modmemory;

	mod = 0;
	modmemory = (unsigned char *)moddata;
	if(modmemory)
	{
		//mod=(modcontext *)malloc(sizeof(modcontext));

		mod = (modcontext *) &g_modcontext;
		if(mod)
		{
			memclear(mod,0,sizeof(modcontext));

			mod->number_of_channels = 4;

			memcopy(&(mod->song.title),modmemory,1084);

			// traitement des mods 15 samples
			/*if (memcmp(&(song.signature), "M.K.", 4) != 0 && memcmp(&(song.signature), "FLT4", 4) != 0 &&  memcmp(&(song.signature), "FLT8", 4) != 0)
			{
				// decalage...
				memcopy(&(song.signature), "M.K.", 4);
				memcopy(&(song.length), &(song.samples[15]), 130);
				memset(&(song.samples[15]), 0, 480);
				modmemory=modmemory+600;
			}*/

			modmemory += 1084;

			//Chargement des patterns
			for (i = max = 0; i < 128; i++)
			{
				while (max <= mod->song.patterntable[i])
				{
					mod->patterndata[max] = (note*)modmemory;
					modmemory += (256*mod->number_of_channels);
					max++;
				}
			}

			for (i = 0; i < 31; i++)
				mod->sampledata[i]=0;

			// Samples loading
			for (i = 0, sptr = mod->song.samples; i <31; i++, sptr++)
			{
				t= (sptr->length &0xFF00)>>8 | (sptr->length &0xFF)<<8;
				sptr->length=t*2;

				t= (sptr->reppnt &0xFF00)>>8 | (sptr->reppnt &0xFF)<<8;
				sptr->reppnt=t*2;

				t= (sptr->replen &0xFF00)>>8 | (sptr->replen &0xFF)<<8;
				sptr->replen=t*2;


				if (sptr->length == 0) continue;

				mod->sampledata[i] = modmemory;
				modmemory += sptr->length;

				if (sptr->replen + sptr->reppnt > sptr->length)
					sptr->replen = sptr->length - sptr->reppnt;

			}

			// States init

			mod->playrate = 44100;
			mod->tablepos = 0;
			mod->patternpos = 0;
			mod->song.speed = 6;
			mod->bpm = 125;
			mod->samplenb = 0;

			mod->patternticks = (((long)mod->song.speed * mod->playrate * 5)/ (2 * mod->bpm)) + 1;
			mod->patternticksaim = ((long)mod->song.speed * mod->playrate * 5) / (2 * mod->bpm);

			mod->sampleticksconst = 3546894UL / mod->playrate; //8448*428/playrate;

			for(i=0;i<(mod->number_of_channels);i++)
			{
				mod->channels[i].volume = 0;
				mod->channels[i].period = 448;
				mod->channels[i].oldoffsetconfig=0;
			}
		}
	}

	return (void*)mod;
}

void hxcmod_fillbuffer(void * modctx,unsigned short * buffer, unsigned long nbsample)
{
	unsigned long i, j;
	unsigned long k;
	unsigned char c;
	short l,r;
	short ll,lr;
	short tl,tr;
	modcontext * mod;
	note	*nptr;
	channel *cptr;

	mod = (modcontext *)modctx;

	if(mod && buffer)
	{
		ll=0;
		lr=0;

		for (i = 0; i < nbsample; i++)
		{
			//---------------------------------------
			if (mod->patternticks++ > mod->patternticksaim)
			{
				nptr = mod->patterndata[mod->song.patterntable[mod->tablepos]];
				nptr = nptr + mod->patternpos;
				cptr = mod->channels;

				mod->patternpos += mod->number_of_channels;
				mod->patternticks = 0;
				mod->patterntickse=0;

				for(c=0;c<mod->number_of_channels;c++)
				{
					worknote((note*)(nptr+c), (channel*)(cptr+c),(char)(c+1),mod);
				}

				if (mod->patternpos == 64*mod->number_of_channels)
				{
					mod->tablepos++;
					mod->patternpos = 0;
					if(mod->tablepos >= mod->song.length) mod->tablepos = 0;
				}

			}

			if (mod->patterntickse++ > (mod->patternticksaim/mod->song.speed))
			{

				nptr = mod->patterndata[mod->song.patterntable[mod->tablepos]];
				nptr = nptr + mod->patternpos;
				cptr = mod->channels;

				for(c=0;c<mod->number_of_channels;c++)
				{
					workeffect(nptr+c, cptr+c);
				}

				mod->patterntickse=0;
			}
			//---------------------------------------

			l=0;
			r=0;

			for (j =0, cptr = mod->channels; (j < mod->number_of_channels) && (cptr->period!=0); j++, cptr++)
			{
				k=0;
				if(((long)cptr->period - (long)cptr->decalperiod - (long)cptr->vibraperiod )!=0)
				k = (((mod->sampleticksconst * cptr->ticks) / ((long)cptr->period - (long)cptr->decalperiod - (long)cptr->vibraperiod )));

				cptr->ticks++;

				if(cptr->replen<=2)
				{
					if (k >= (cptr->length))
					{
						cptr->length = 0;
						cptr->reppnt = 0;
						cptr->ticks = 0;
						k=0;
					}
				}
				else
				{
					if (k >= (unsigned long)(cptr->replen+cptr->reppnt))
					{
						k=cptr->reppnt;
						cptr->ticks = (((long)cptr->period - (long)cptr->decalperiod - (long)cptr->vibraperiod )*k)/(mod->sampleticksconst);
					}
				}

				if(cptr->sampdata!=0 && (((j&3)==1) || ((j&3)==2) ))
					r += ( (cptr->sampdata[k] *  cptr->volume));

				if(cptr->sampdata!=0 && (((j&3)==0) || ((j&3)==3) ))
					l += ( (cptr->sampdata[k] *  cptr->volume));
			}

			tr=r;
			tl=l;

			l=(l+ll)>>1;
			r=(r+lr)>>1;

			buffer[(i*2)]   = (l+(r>>1));
			buffer[(i*2)+1] = (r+(l>>1));

			ll=tl;
			lr=tr;
		}

		mod->samplenb=mod->samplenb+nbsample;
	}
}

void hxcmod_unload(void * modctx)
{
	modcontext * mod;

	mod = (modcontext *)modctx;

	if(mod)
		memclear(mod,0,sizeof(modcontext));
}
