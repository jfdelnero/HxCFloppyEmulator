#define NO_SECTOR_UNDER_INDEX 0x80000000
#define HARD_SECTORED_DISK    0x40000000
#define REVERTED_INDEX        0x20000000


unsigned long us2index(unsigned long startindex,HXCFE_SIDE * track,unsigned long us,unsigned char fill,char fillorder);
unsigned long fillindex(int startindex,HXCFE_SIDE * track,unsigned long us,unsigned char fill,char fillorder);

HXCFE_CYLINDER* allocCylinderEntry(unsigned short rpm,unsigned char number_of_side);
void savebuffer(char * name,unsigned char * buffer, int size);
double GetTrackPeriod(HXCFE* floppycontext,HXCFE_SIDE * curside);
double MeasureTrackTiming(HXCFE* floppycontext,HXCFE_SIDE * curside,unsigned long startpulse,unsigned long endpulse);

int floppyTrackTypeIdentification(HXCFE* floppycontext,HXCFE_FLOPPY *fp);

unsigned char  size_to_code(unsigned long size);
