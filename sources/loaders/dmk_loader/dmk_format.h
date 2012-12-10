
#define SINGLE_SIDE    0x10
#define SINGLE_DENSITY 0x40
#define IGNORE_DENSITY 0x80

typedef struct dmk_header_
{
	unsigned char  write_protected;
	unsigned char  track_number;
	unsigned short track_len;
	unsigned char  flags;
	unsigned char  rsvd_1[(0xB-0x5)+1];
	unsigned char  rsvd_2[(0xF-0xC)+1];	
}dmk_header;



