	typedef struct cfgrawfile_
	{
		unsigned char sectorpertrack;
		unsigned char sectorsize;
		unsigned long numberoftrack;
		unsigned char tracktype;
		unsigned char sidecfg;	
		unsigned long gap3;
		unsigned long rpm;
		unsigned long bitrate;
		unsigned char interleave;
		unsigned char firstidsector;
		unsigned char skew;
		unsigned char autogap3;
		unsigned char fillvalue;
		unsigned char intersidesectornumbering;
		unsigned char sideskew;
	}cfgrawfile;


	typedef struct track_type_
	{
		int id;
		char * name;
		int tracktype;
	}track_type;

int loadfloppy(char *filename);
int loadrawfile(HXCFLOPPYEMULATOR* floppycontext,cfgrawfile * rfc,char * file);