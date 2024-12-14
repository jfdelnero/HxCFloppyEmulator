/*
PRI File Format (2020-03-26)
========================================================================

File structure
------------------------------------------------------------------------

<file header chunk>
<chunk 0>
<chunk 1>
...
<chunk n>
<end chunk>

All integers are in big-endian format.

Unknown chunks should be skipped.


Chunk format
------------------------------------------------------------------------

offset	size	description

0	4	Chunk ID
4	4	Chunk size (n)
0	n	Chunk data
n	4	Chunk CRC

	- The size does not include the chunk ID, chunk size or chunk CRC
	  fields.

	- The chunk CRC covers the chunk ID, chunk size and chunk data.


CHUNK "PRI ": File header chunk
------------------------------------------------------------------------

0	4	Magic ('PRI ')
4	4	Size (4)
0	2	Version (0)
2	2	Reserved
4	4	CRC


CHUNK "END ": End chunk
------------------------------------------------------------------------

0	4	Magic ('END ')
4	4	Size (0)
0	4	CRC (0x3d64af78)

	- This chunk marks the end of the file. Any data that follows
	  should be ignored.


CHUNK "TEXT": Comments
------------------------------------------------------------------------

0	4	Magic ('TEXT')
4	4	Size (n)
0	n	Data
n	4	CRC

	- Comments should be UTF-8, with lines separated by LF (0x0a).

	- If there are multiple TEXT chunks, their contents should be
	  concatenated.


CHUNK "TRAK": Track header
------------------------------------------------------------------------

0	4	Magic ('TRAK')
4	4	Size (16)
0	4	Cylinder
4	4	Head
8	4	Track length in bits
12	4	Bit clock rate
16	4	CRC

The TRAK chunk starts a new track. All following chunks until the next
TRAK chunk or until the END chunk refer to this track.


CHUNK "DATA": Track data
------------------------------------------------------------------------

0	4	Magic ('DATA')
4	4	Size (n)
0	n	Track data
n	4	CRC

	- The most significant bit of every byte comes first.

	- The DATA chunk may be shorter than the track length in the
	  preceding TRAK chunk suggests. In that case the remainder of
	  the track data should be set to 0.


CHUNK "WEAK": Weak bit mask
------------------------------------------------------------------------

0	4	Magic ('WEAK')
4	4	Size (8*n)
0	4	Bit offset
4	4	Weak bit mask
...
8*n	4	CRC

	- The MSB of the bit mask corresponds to the bit offset specified.


CHUNK "BCLK": Alternate bit clock
------------------------------------------------------------------------

0	4	Magic ('BCLK')
4	4	Size (8*n)
0	4	Bit offset
4	4	New bit clock
...
8*n	4	CRC

The new bit clock starting at the specified bit offset is
(new_bit_clock / 65536) * track_bit_clock

If the new bit clock is 0, the default track bit clock is restored.


CRC
---

	- The algorithm used is big-endian CRC-32 with generator
	  polynomial 0x1edc6f41. The CRC value is initialized to 0.

	unsigned long pri_crc (const unsigned char *buf, unsigned cnt)
	{
		unsigned      i, j;
		unsigned long crc;

		crc = 0;

		for (i = 0; i < cnt; i++) {
			crc ^= (unsigned long) (buf[i] & 0xff) << 24;

			for (j = 0; j < 8; j++) {
				if (crc & 0x80000000) {
					crc = (crc << 1) ^ 0x1edc6f41;
				}
				else {
					crc = crc << 1;
				}
			}
		}

		return (crc & 0xffffffff);
	}
*/

#pragma pack(1)

typedef struct pri_chunk_header_
{
	uint32_t chunk_id;
	uint32_t size;    // data[] size, without the crc32
	uint8_t  data[];
	//uint32_t crc32;
}pri_chunk_header;

typedef struct pri_track_
{
	uint32_t cylinder;
	uint32_t head;    // data[] size, without the crc32
	uint32_t track_len;
	uint32_t bitclock_rate;
}pri_track;

#pragma pack()

#define CHUNKID_PRI  0x20495250 //'PRI '
#define CHUNKID_END  0x20444E45 //'END '
#define CHUNKID_TEXT 0x54584554 //'TEXT'
#define CHUNKID_TRAK 0x4B415254 //'TRAK'
#define CHUNKID_DATA 0x41544144 //'DATA'
#define CHUNKID_WEAK 0x4B414557 //'WEAK'
#define CHUNKID_BCLK 0x4B4C4342 //'BCLK'
