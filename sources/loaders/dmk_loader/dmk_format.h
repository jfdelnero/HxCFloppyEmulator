
#define SINGLE_SIDE    0x10
#define SINGLE_DENSITY 0x40
#define IGNORE_DENSITY 0x80

#pragma pack(1)

typedef struct dmk_header_
{
	uint8_t  write_protected;
	uint8_t  track_number;
	uint16_t track_len;
	uint8_t  flags;
	uint8_t  rsvd_1[(0xB-0x5)+1];
	uint8_t  rsvd_2[(0xF-0xC)+1];	
}dmk_header;

#pragma pack()
