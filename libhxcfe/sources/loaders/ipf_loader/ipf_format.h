#pragma pack(1)

#define IPF_ID 0x843265bb

#define ENCOD_CAPS 1
#define ENCOD_SPS  2

typedef struct ipf_header_
{
	uint8_t  id[4];
	uint32_t len;
	uint32_t crc;
}ipf_header;

typedef struct ipf_info_
{
	uint32_t type;
	uint32_t encoder;
	uint32_t enc_rev;
	uint32_t release;
	uint32_t revision;
	uint32_t origin;
	uint32_t min_cyl;
	uint32_t max_cyl;
	uint32_t min_head;
	uint32_t max_head;
	uint32_t date;
	uint32_t time;
	uint32_t platform[4];
	uint32_t disk_num;
	uint32_t user_id;
	uint32_t reserved[3];
}ipf_info;

typedef struct ipf_img_
{
	uint32_t cyl;
	uint32_t head;
	uint32_t den_type;
	uint32_t sig_type;
	uint32_t trk_size;
	uint32_t start_pos;
	uint32_t start_bit;
	uint32_t data_bits;
	uint32_t gap_bits;
	uint32_t trk_bits;
	uint32_t blk_cnt;
	uint32_t process;
	uint32_t flags;
	uint32_t dat_chunk;
	uint32_t reserved[3];
}ipf_img;

typedef struct ipf_data_
{
	uint32_t size;
	uint32_t bsize;
	uint32_t dcrc;
	uint32_t dat_chunk;
}ipf_data;

typedef struct ipf_block_
{
	uint32_t blockbits;
	uint32_t gapbits;
	union {
		struct
		{
			uint32_t block_size;
			uint32_t gap_size;
		} caps;

		struct {
			uint32_t gap_offset;
			uint32_t cell_type;
		} sps;
	} u;

	uint32_t enc_type;
	uint32_t flag;
	uint32_t gap_value;
	uint32_t data_offset;
}ipf_block;

enum chunkcode
{
	chunk_End = 0,
	chunk_Sync,
	chunk_Data,
	chunk_Gap,
	chunk_Raw,
	chunk_Flaky
};

enum dentype
{
	denNoise     = 1,
	denUniform   = 2,
	denCopylock  = 3,
	denSpeedlock = 6
};

#pragma pack()

// Records sizes (big endian 32-bit fields)
#define IPF_RECORD_HEADER_SIZE        12
#define IPF_INFO_BLOCK_SIZE           84
#define IPF_IMGE_BLOCK_SIZE           68
#define IPF_DATA_BLOCK_SIZE           16
#define IPF_BLOCK_DESCRIPTOR_SIZE     32

// INFO record
#define IPF_MEDIA_FLOPPY              1

#define IPF_PLATFORM_NONE             0
#define IPF_PLATFORM_AMIGA            1
#define IPF_PLATFORM_ATARI_ST         2
#define IPF_PLATFORM_PC               3
#define IPF_PLATFORM_AMSTRAD_CPC      4
#define IPF_PLATFORM_SPECTRUM         5
#define IPF_PLATFORM_SAM_COUPE        6
#define IPF_PLATFORM_ARCHIMEDES       7
#define IPF_PLATFORM_C64              8
#define IPF_PLATFORM_ATARI_8BIT       9

// IMGE record
#define IPF_SIGNAL_2US                1
#define IPF_IMGE_FLAG_FUZZY           0x00000001

// Block descriptor
#define IPF_CELL_TYPE_2US             1
#define IPF_BLOCK_ENC_MFM             1
#define IPF_BLOCK_ENC_RAW             2
#define IPF_BLOCK_FLAG_FORWARD_GAP    0x00000001
#define IPF_BLOCK_FLAG_BACKWARD_GAP   0x00000002
#define IPF_BLOCK_FLAG_DATA_IN_BITS   0x00000004

// Data/Gap stream elements header : size width (bits 7-5) | type (bits 4-0)
#define IPF_STREAM_SIZE_WIDTH_SHIFT   5
#define IPF_STREAM_TYPE_MASK          0x1F

// Writer internal track flags
#define IPF_TRACK_NOISE               0x01
#define IPF_TRACK_WEAK                0x02
