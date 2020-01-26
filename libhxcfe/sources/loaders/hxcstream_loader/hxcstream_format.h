#pragma pack(1)

#define HXCSTREAM_HEADERSIGN 0x484B4843
#define HXCSTREAM_CHUNKBLOCK_METADATA_ID       0x00000000
#define HXCSTREAM_CHUNKBLOCK_PACKEDIOSTREAM_ID 0x00000001
#define HXCSTREAM_CHUNKBLOCK_PACKEDSTREAM_ID   0x00000002

typedef struct _chunk_header
{
	uint32_t header; // CHKH -  0x43, 0x48, 0x4B, 0x48 - 484B4843
	uint32_t size;   // Header + data +CRC
	uint32_t packet_number;
}chunk_header;

typedef struct _chunkblock_header
{
	uint32_t type;
	uint32_t payload_size;
	// data
}chunkblock_header;

typedef struct _metadata_header
{
	uint32_t type;         // 0x00000000 - Metadata text buffer
	uint32_t payload_size;
	// data
}metadata_header;

typedef struct _packed_io_header
{
	uint32_t type;   // 0x00000001 - LZ4 packed 16 bits IO dump
	uint32_t payload_size;
	uint32_t packed_size;
	uint32_t unpacked_size;
	// packed_data
}packed_io_header;

typedef struct _packed_stream_header
{
	uint32_t type;   // 0x00000002 - LZ4 packed stream
	uint32_t payload_size;
	uint32_t packed_size;
	uint32_t unpacked_size;
	uint32_t number_of_pulses;
	// packed_data
}packed_stream_header;

// Data + CRC32

#pragma pack()
