
#pragma pack(1)

typedef struct atr_header_
{
	uint16_t sign;
	uint16_t image_size; // in paragraphs (size/16)
	uint16_t sector_size;
	uint16_t image_size_high;  //  high part of size, in paragraphs
	uint8_t  flags;
	uint16_t bad_sector;
	uint8_t  unused[5];
}atr_header;

#pragma pack()
