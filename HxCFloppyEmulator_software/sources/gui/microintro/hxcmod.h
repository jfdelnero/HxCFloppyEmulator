#pragma pack(1)

#ifndef MODPLAY_DEF
#define MODPLAY_DEF

#define NUMMAXCHANNELS 32

#define MAXNOTES 36

typedef unsigned char	muchar;
typedef unsigned short	muint;
typedef unsigned long	mulong;

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

typedef struct {
	unsigned long pattern;
	unsigned long order;
	unsigned long bufferpos;
} modevent;

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
	short   decalperiod2;
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

	modevent* modev;

	muint   number_of_channels;
	//muint freqtab[288+8];

} modcontext;

	modcontext * modload(char * filename,char * modmemory,char * debug);
	int     modplay(modcontext * mod);
	void    fillbuffer(muchar * buffer, unsigned long nbsample,modcontext * mod);
	void    worknote(note * nptr, channel * cptr,char t,modcontext * mod);
	void    workeffect(note * nptr, channel * cptr);

	void    modregevent(modevent *mevent,modcontext * mod);

#endif

#pragma pack()