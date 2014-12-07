#pragma pack(1)

typedef struct gkh_header_
{
	uint32_t header_tag;   // "TDDF"
	uint8_t  type;         // I
	uint8_t  version;		 // 0x01
	uint16_t numberoftags;
}gkh_header;


typedef struct image_type_tag_
{
	uint8_t  tagtype;      // 0x0A
	uint8_t  datatype;     // I
	uint16_t nboftrack;		
	uint16_t nbofheads;
	uint16_t nbofsectors;
	uint16_t sectorsize;
}image_type_tag;

typedef struct image_location_tag_
{
	uint8_t  tagtype;      // 0x0B
	uint8_t  datatype;     // I
	uint32_t longword1;
	uint32_t fileoffset;
}image_location_tag;

#pragma pack()

