#define NO_SECTOR_UNDER_INDEX 0x80000000
#define HARD_SECTORED_DISK    0x40000000


unsigned long us2index(unsigned long startindex,SIDE * track,unsigned long us,unsigned char fill,char fillorder);
unsigned long fillindex(unsigned long startindex,SIDE * track,unsigned long us,unsigned char fill,char fillorder);

CYLINDER* allocCylinderEntry(unsigned short rpm,unsigned char number_of_side);
