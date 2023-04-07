#pragma pack(1)

// Header:
// -------
// 0x00 - Media byte (See below)
// 0x01-0xA0 - 160 sector present bytes, like DCU/DCP
// 0xA1-0xAA - 10 bytes of 0x00
// 0xAB-0xB7 - 13 byte string "DIFC HEADER  "
// 0xB8-0xFD - 70 bytes of 0x00
// 0xFE-0xFF - WORD (0x16?)
// 0x100 - Start of data (Stored in C/H/S sequence)

// Media Byte:
// -----------
// DIM    DCP  Format  Geometry
// 0x00 = 0x02 (2HS)   (8sec/trk 1232k)
// 0x01 = 0x02 (2HS)   (9sec/trk 1440k)
// 0x02 = 0x01 (2HC)   (15sec/trk 1200k) [80/2/15/512]
// 0x03 = 0x09 (2HQ)   (18sec/trk 1440k) IBM 1.44MB 2HD format

/*
#define DCP_DISK_2HD_08     0x01    //  2HD- 8 sectors (1.25MB)
#define DCP_DISK_2HD_15     0x02    //  2HD-15 sectors (1.21MB)
#define DCP_DISK_2HQ_18     0x03    //  2HQ-18 sectors (1.44MB)
#define DCP_DISK_2DD_08     0x04    //  2DD- 8 sectors ( 640KB)
#define DCP_DISK_2DD_09     0x05    //  2DD- 9 sectors ( 720KB)
#define DCP_DISK_2HD_09     0x08    //  2HD- 9 sectors (1.44MB)
#define DCP_DISK_2HD_BAS    0x11    //  2HD-BASIC
#define DCP_DISK_2DD_BAS    0x19    //  2DD-BASIC
#define DCP_DISK_2HD_26     0x21    //  2HD-26 sectors
*/

typedef struct dim_x68k_header_
{
	uint8_t  media_byte;           // Media byte
	uint8_t  sectors_present[160]; // 160 sector present bytes, like DCU/DCP
	uint8_t  pad1[10];             // 10 bytes of 0x00
	uint8_t  difc_sign[13];        // 13 byte string "DIFC HEADER  "
	uint8_t  pad2[70];             // 70 bytes of 0x00
	uint16_t word;                 // WORD (0x16?)
}dim_x68k_header;
// 0x100 - Start of data (Stored in C/H/S sequence)

#pragma pack()
