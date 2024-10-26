// Description from http://fileformats.archiveteam.org/wiki/ATR

/*
 Format

 Files have a 16-byte header, followed by a raw sector-by-sector dump of the disk.

 Single density images are 92,176 bytes long.
 Enhanced density images are 133,136 bytes long.
 Double density images are 183,952 bytes long.
*/

#pragma pack(1)

// 16 bytes header - little endian format
typedef struct atr_header_
{
	// Identification <WORD>: ($9602). This word is the 16 bit sum of the individual ASCII values of the string of bytes: "NICKATARI".
	uint16_t sign;

	// Size of disk image <WORD>: The size is expressed in "paragraphs". A paragraph is sixteen bytes, thus Size = Image size in bytes / 16.
	uint16_t image_size; // in paragraphs (size/16)

	// Sector size <WORD>: 128 ($80) or 256 ($100) bytes per sector.
	// Note that the original documentation only specifies two sector sizes, in practice however there is also sector size 512 ($200).
	// 512 byte sectors were introduced by SpartaDOS X, and are sometimes used to create large ATR images.
	// Some Atari Emulators and Peripheral emulators such as Altirra and AspeQt can make use of the 512 byte sectors in ATR files.
	uint16_t sector_size;

	// High part of size <WORD>: in paragraphs (added by REV 3.00)
	uint16_t image_size_high;  //  high part of size, in paragraphs

	// Disk flags <BYTE>: Flags such as copy protection and write protect.
	// The 9th byte of the header contains information in individual bits.
	// Bit 4 = 1 means the disk image is treated as copy protected (has bad sectors).
	// Bit 5 = 1 means the disk is write protected.
	uint8_t  flags;

	// 1st bad sector <WORD>: The 10th and 11th bytes of the header is a word which contains the number of the first bad sector.
	uint16_t bad_sector;

	// 5 SPARE header bytes (contain zeroes)
	uint8_t  unused[5];
}atr_header;


// After the header comes the disk image.
// This is just a continuous string of bytes, with the first 128 bytes being the contents of disk sector 1, the second being sector 2, etc.
// The first 3 sectors of an Atari disk must be 128 bytes long even if the rest of the sectors are 256 bytes or more.
//
// Note that not all software will recognize or use the 1st bad sector header data, and some software do not follow the "first 3 sectors must be 128 bytes long" rule.

#pragma pack()

