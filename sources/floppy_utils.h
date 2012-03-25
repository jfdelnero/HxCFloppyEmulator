#define NO_SECTOR_UNDER_INDEX 0x80000000
#define HARD_SECTORED_DISK    0x40000000
#define REVERTED_INDEX        0x20000000


unsigned long us2index(unsigned long startindex,SIDE * track,unsigned long us,unsigned char fill,char fillorder);
unsigned long fillindex(int startindex,SIDE * track,unsigned long us,unsigned char fill,char fillorder);

CYLINDER* allocCylinderEntry(unsigned short rpm,unsigned char number_of_side);
void savebuffer(unsigned char * name,unsigned char * buffer, int size);

char * getfilenamebase(char * fullpath,char * filenamebase);
char * getfilenameext(char * fullpath,char * filenameext);
int getfilenamewext(char * fullpath,char * filenamewext);
int getpathfolder(char * fullpath,char * folder);
int checkfileext(char * path,char *ext);

int getfilesize(char * path);
