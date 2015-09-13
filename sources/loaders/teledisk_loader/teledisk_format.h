#define TD0_SIG_NORMAL          "TD"    // Normal compression (RLE)
#define TD0_SIG_ADVANCED        "td"    // Huffman compression also used for everything after TD0_HEADER

#pragma pack(1)
typedef struct _TELEDISK_HEADER
{
	uint8_t	TXT[2];
	uint8_t	SeqVal;		// Volume sequence (zero for the first)
	uint8_t	ChkSig;		// Check signature for multi-volume sets (all must match)
	uint8_t	TDVer;		// Teledisk version used to create the file (11 = v1.1)
	uint8_t	Dens;		// Source disk density (0 = 250K bps,  1 = 300K bps,  2 = 500K bps ; +128 = single-density FM)
	uint8_t	DrvType;	// Source drive type (1 = 360K, 2 = 1.2M, 3 = 720K, 4 = 1.44M)
	uint8_t	TrkDens;	// 0 = source matches media density, 1 = double density, 2 = quad density)
	uint8_t	DosMode;	// Non-zero if disk was analysed according to DOS allocation
	uint8_t	Surface;	// Disk sides stored in the image
	uint8_t	CRC[2];		// 16-bit CRC for this header
} TELEDISK_HEADER;

// Optional comment block, present if bit 7 is set in bTrackDensity above
typedef struct _TELEDISK_COMMENT
{
	uint8_t CRC[2];					// 16-bit CRC covering the comment block
	uint16_t Len;					// Comment block length
	uint8_t  bYear, bMon, bDay;		// Date of disk creation
	uint8_t  bHour, bMin, bSec;		// Time of disk creation
//  BYTE    abData[];				// Comment data, in null-terminated blocks
}TELEDISK_COMMENT;

typedef struct _TELEDISK_TRACK_HEADER
{
	uint8_t SecPerTrk;			// Number of sectors in track
	uint8_t PhysCyl;			// Physical track we read from
	uint8_t PhysSide;			// Physical side we read from
	uint8_t CRC;				// Low 8-bits of track header CRC
} TELEDISK_TRACK_HEADER;

typedef struct _TELEDISK_SECTOR_HEADER
{
	uint8_t Cyl;				// Track number in ID field
	uint8_t Side;				// Side number in ID field
	uint8_t SNum;				// Sector number in ID field
	uint8_t SLen;				// Sector size indicator:  (128 << bSize) gives the real size
	uint8_t Syndrome;			// Flags detailing special sector conditions
	uint8_t CRC;				// Low 8-bits of sector header CRC
}TELEDISK_SECTOR_HEADER;

#pragma pack()
