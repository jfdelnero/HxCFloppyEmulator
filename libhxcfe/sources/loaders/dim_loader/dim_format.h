
#pragma pack(1)
typedef struct dim_header_
{
	uint16_t id_header;        // 0x0000 Word ID Header (0x4242('BB'))
	uint8_t  unused1;
	uint8_t  used_sector_only; // 0x0003 Byte Image contains all sectors (0) or used sectors (1)
	uint16_t unused2;
	uint8_t  side;             // 0x0006 Byte Sides (0 or 1; add 1 to this to get correct number of sides)
	uint8_t  unused3;
	uint8_t  nbsector;         // 0x0008 Byte Sectors per track
    uint8_t  unused4;
	uint8_t  start_track;      // 0x000A Byte Start Track (0 based)
    uint8_t  unused5;
    uint8_t  end_track;        // 0x000C Byte Ending Track (0 based)
    uint8_t  density;			 // 0x000D Byte Double-Density(0) or High-Density (1)
    uint8_t  sectorsizeh;       // sector size (bytes)
    uint8_t  sectorsizel;       // sector size (bytes)
}dim_header;
#pragma pack()
