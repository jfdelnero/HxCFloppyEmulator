#define SAD_SIGNATURE          "Aley's disk backup"

#pragma pack(1)
// Format of a SAD image header (22 bytes)
typedef struct
{
    uint8_t abSignature[sizeof SAD_SIGNATURE - 1];

    uint8_t bSides;             // Number of sides on the disk
    uint8_t bTracks;            // Number of tracks per side
    uint8_t bSectors;           // Number of sectors per track
    uint8_t bSectorSizeDiv64;   // Sector size divided by 64
}
SAD_HEADER;

#pragma pack()