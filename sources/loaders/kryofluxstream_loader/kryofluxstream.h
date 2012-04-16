
typedef struct s_index_evt_
{
	unsigned long dump_offset;
	unsigned long clk;
}s_index_evt;

typedef struct s_track_dump_
{
	unsigned long	* track_dump;
	unsigned long	nb_of_pulses;
	s_index_evt		* index_evt_tab;
	unsigned long	nb_of_index;
}s_track_dump;


s_track_dump* DecodeKFStreamFile(HXCFLOPPYEMULATOR* floppycontext,char * file,float timecoef);
void FreeStream(s_track_dump* trackdump);


#define KF_MCLOCK 48054857,14 //(((18432000 * 73) / 14) / 2)
#define KF_SCLOCK ((float)KF_MCLOCK / (float)2)
#define KF_ICLOCK (KF_MCLOCK / 16)

