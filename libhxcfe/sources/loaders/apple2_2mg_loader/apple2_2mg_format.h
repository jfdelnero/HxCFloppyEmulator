/*
GS WorldView
Summer 2003
http://apple2.org.za/gswv/a2zine/Docs/DiskImage_2MG_Info.txt


2MG (or 2IMG) Disk Image Files

These are disk images (e.g. .dsk or .nib) with a prefix which
includes information about size, format, sector ordering,
volume number, locked/unlocked, etc..

2MG files may also have a Comment and/or extra file information
added following the disk image data.

2MG format can accommodate disk images ranging from 5.25"
diskette up through hard disk.

Typically, the image files have names ending with ".2mg".

ProDOS File Type-  $E0
ProDOS Aux Type-   $0130

Emulators which can use 2MG images include Bernie ][ the Rescue,
Catakig, Sweet 16, XGS.

On an Apple II, ASIMOV2 (for IIgs) is the usual utility for
creating 2MG files and for converting them to disk.



2MG (or 2IMG) Disk Image Format

Part of File              Length

Prefix-                   64 bytes (usual size of Prefix)
Disk Data-                varies (e.g. 143,360 for 5.25" disk)
Comment (optional)-       varies (often not present)
Creator added (optional)- varies (often not present)



Prefix Format

0000-0003: 32 49 4D 47   "2IMG"    ID for 2MG format (ASCII Text)
0004-0007: 58 47 53 21   "XGS!"    Creator ID (ASCII Text) **
0008-0009: 40 00                   Header size ($0040= 64 bytes)
000A-000B: 01 00                   Version number
000C-000F: 01 00 00 00             Image Format
									00= DOS 3.3 sector order
									01= ProDOS sector order
									02= NIB data

**Note: "Creator" refers to application creating the image.
Here are ID's in use by various applications:

ASIMOV2-              "!nfc"
Bernie ][ the Rescue- "B2TR"
Catakig-              "CTKG"
Sheppy's ImageMaker-  "ShIm"
Sweet 16-             "WOOF"
XGS-                  "XGS!"



0010-0013: 00 00 00 00  (Flags & DOS 3.3 Volume Number)

The four-byte flags field contains bit flags and data relating to
the disk image. Bits not defined should be zero.

Bit   Description

31    Locked? If Bit 31 is 1 (set), the disk image is locked. The
	  emulator should allow no changes of disk data-- i.e. the disk
	  should be viewed as write-protected.

8     DOS 3.3 Volume Number? If Bit 8 is 1 (set), then Bits 0-7
	  specify the DOS 3.3 Volume Number. If Bit 8 is 0 and the
	  image is in DOS 3.3 order (Image Format = 0), then Volume
	  Number will be taken as 254.

7-0   The DOS 3.3 Volume Number, usually 1 through 254,
	  if Bit 8 is 1 (set). Otherwise, these bits should be 0.


0014-0017: 18 01 00 00  (ProDOS Blocks = 280 for 5.25")

The number of 512-byte blocks in the disk image- this value
should be zero unless the image format is 1 (ProDOS order).
Note: ASIMOV2 sets to $118 whether or not format is ProDOS.


0018-001B: 40 00 00 00  (Offset to disk data = 64 bytes)

Offset to the first byte of disk data in the image file
from the beginning of the file- disk data must come before
any Comment and Creator-specific chunks.


001C-001F: 00 30 02 00  (Bytes of disk data = 143,360 for 5.25")

Length of the disk data in bytes. (For ProDOS should be
512 x Number of blocks)


0020-0023: 00 00 00 00  (Offset to optional Comment)

Offset to the first byte of the image Comment- zero if there
is no Comment. The Comment must come after the data chunk,
but before the creator-specific chunk. The Comment, if it
exists, should be raw text; no length byte or C-style null
terminator byte is required (that's what the next field is for).


0024-0027: 00 00 00 00  (Length of optional Comment)

Length of the Comment chunk- zero if there's no Comment.


0028-002B: 00 00 00 00  (Offset to optional Creator data)

Offset to the first byte of the Creator-specific data chunk-
zero if there is none.


002C-002F: 00 00 00 00  (Length of optional Creator data)

Length of the Creator-specific data chunk- zero if there is no
creator-specific data.


0030-003F: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00

Reserved space- at present all must be zero.




Credits

Thanks to Eric Shepherd and Roger Johnstone for 2MG info.


Rubywand
*/

#pragma pack(1)

typedef struct _A2_2MG_header
{
	unsigned char sign[4]; // 2IMG
	unsigned char creator[4];
	uint16_t header_size;
	uint16_t version;
	uint32_t format;
	uint32_t flags;
	uint32_t prodos_blocks;
	uint32_t data_offset;
	uint32_t data_size;
	uint32_t comment_offset;
	uint32_t comment_size;
	uint32_t creatordata_offset;
	uint32_t creatordata_size;
	unsigned char reserved[16];
}A2_2MG_header;

#pragma pack()

