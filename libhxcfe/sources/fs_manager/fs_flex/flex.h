#define FLEX_SECTOR_SIZE 256

typedef struct flex_SIR_
{
	char     volumeLabel[11];
	uint16_t volumeNumber; //(Hi,Lo)
	uint8_t  firstFreeTrack;
	uint8_t  firstFreeSector;
	uint8_t  lastFreeTrack;
	uint8_t  lastFreeSector;
	uint16_t freeSectors; //(Hi,Lo)
	uint8_t  dateMonth;
	uint8_t  dateDay;
	uint8_t  dateYear;
	uint8_t  endTrack;
	uint8_t  endSector;
}flex_SIR;

typedef struct flex_DIR_
{
	char     fileName[8];
	char     fileExtension[3];
	uint16_t notUsed1; //(Hi,Lo)
	uint8_t  startTrack;
	uint8_t  startSector;
	uint8_t  endTrack;
	uint8_t  endSector;
	uint16_t totalNumberOfSectors; //(Hi,Lo)
	uint8_t  randomFileFlag;
	uint8_t  notUsed2;
	uint8_t  dateMonth;
	uint8_t  dateDay;
	uint8_t  dateYear;
}flex_DIR;

typedef struct flex_DATA_
{
	uint8_t  nextTrack;
	uint8_t  nextSector;
	uint16_t sectorSequenceNumber; //(Hi,Lo)
	uint8_t  data[FLEX_SECTOR_SIZE - (1 + 1 + 2)];
}flex_DATA;
