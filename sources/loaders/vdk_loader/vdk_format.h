#pragma pack(1)

typedef struct vdk_header_
{
	uint16_t signature;      // "dk" -> 0x6B64
	uint16_t header_size;    //Header size (little-endian)
	uint8_t  version;        //Version of VDK format
	uint8_t  comp_version;   //Backwards compatibility version
	uint8_t  file_source_id; //Identity of file source
	uint8_t  file_source_ver;//Version of file source
	uint8_t  number_of_track;//Number of tracks
	uint8_t  number_of_sides;//Number of sides
	uint8_t  flags;		     //Flags
	uint8_t  name_len;       //Compression flags and name length
}vdk_header;

#pragma pack()