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