#define NO_SECTOR_UNDER_INDEX 0x80000000
#define HARD_SECTORED_DISK    0x40000000
#define REVERTED_INDEX        0x20000000


int32_t us2index(int32_t startindex,HXCFE_SIDE * track,uint32_t us,unsigned char fill,char fillorder);
int32_t fillindex(int32_t startindex,HXCFE_SIDE * track,uint32_t us,unsigned char fill,char fillorder);

HXCFE_CYLINDER* allocCylinderEntry(int32_t rpm,int32_t number_of_side);
void savebuffer(char * name,unsigned char * buffer, int size);
double GetTrackPeriod(HXCFE* floppycontext,HXCFE_SIDE * curside);
double MeasureTrackTiming(HXCFE* floppycontext,HXCFE_SIDE * curside,uint32_t startpulse,uint32_t endpulse);

int floppyTrackTypeIdentification(HXCFE* floppycontext,HXCFE_FLOPPY *fp);

unsigned char  size_to_code(uint32_t size);
