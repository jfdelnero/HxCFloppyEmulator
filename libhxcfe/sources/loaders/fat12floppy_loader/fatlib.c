#include <string.h>
#include <stdlib.h>

#include "types.h"

#include "fatlib.h"

int initFATPartion(int fattype,int numberofsector,FATPARTITION * fatpart)
{
	int ret;

	switch(fattype)
	{
		case FATLIB_FAT12:
			break;
		case FATLIB_FAT16:
			return -1;
			break;
		case FATLIB_FAT32:
			return -1;
			break;
		default:
			return -1;
			break;
	}

	ret = 0;

	if(fatpart)
	{
		fatpart->FATType = fattype;
		fatpart->rawdata = (unsigned char*)malloc( numberofsector * 512 );

		if(fatpart->rawdata)
			memset(fatpart->rawdata,0xF6,numberofsector*512);
		else
		{
			fatpart->FATType = 0;
			ret = -1;
		}
	}

	return ret;
}

