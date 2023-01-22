#pragma pack(1)

/*
---------------------
The .MSA File Format
---------------------

(http://members.multimania.co.uk/stappz/msa.html)

The following text was grabbed from the Hatari source.

For those interested, an MSA file is made up as follows:

Header:

Word  ID marker, should be $0E0F
Word  Sectors per track
Word  Sides (0 or 1; add 1 to this to get correct number of sides)
Word  Starting track (0-based)
Word  Ending track (0-based)

Individual tracks follow the header in alternating side order, e.g. a double sided disk is stored as:

TRACK 0, HXCFE_SIDE 0
TRACK 0, HXCFE_SIDE 1
TRACK 1, HXCFE_SIDE 0
TRACK 1, HXCFE_SIDE 1
TRACK 2, HXCFE_SIDE 0
TRACK 2, HXCFE_SIDE 1

...and so on. Track blocks are made up as follows:

Word  Data length
Bytes  Data

If the data length is equal to 512 x the sectors per track value, it is an uncompressed track and you can
merely copy the data to the appropriate track of the disk. However, if the data length value is less than
512 x the sectors per track value it is a compressed track.

Compressed tracks use simple a Run Length Encoding (RLE) compression method. You can directly copy any data
bytes until you find an $E5 byte. This signals a compressed run, and is made up as follows:

Byte  Marker - $E5
Byte  Data byte
Word  Run length

So, if MSA found six $AA bytes in a row it would encode it as:
$E5AA0006

What happens if there's an actual $E5 byte on the disk? Well, logically enough, it is encoded as:
$E5E50001

This is obviously bad news if a disk consists of lots of data like $E500E500E500E500... but if MSA makes a track
bigger when attempting to compress it, it just stores the uncompressed version instead.

MSA only compresses runs of at least 4 identical bytes (after all, it would be wasteful to store 4 bytes for
a run of only 3 identical bytes!). There is one exception to this rule: if a run of 2 or 3 $E5 bytes is found,
that is stored appropriately enough as a run. Again, it would be wasteful to store 4 bytes for every single $E5 byte.

The hacked release of MSA that enables the user to turn off compression completely simply stops MSA 
from trying this compression and produces MSA images that are completely uncompressed. 
This is okay because it is possible for MSA to produce such an image anyway, 
and such images are therefore 100% compatible with normal MSA versions (and MSA-to-ST of course).

Note from The Atari Mafia:
All words in an *.msa file are in Motorola byte order not Intel byte order.
So on an Intel PC (for emulator or converter use) you must swap the high and the low byte to achieve Intel byte order.

Hopefully this helps more and more programmers to use the .MSA file format with their emulators and disk converters.
*/

typedef struct msa_header_
{
	uint8_t  sign[2]; // 0x0E 0x0F
	uint16_t number_of_sector_per_track;
	uint16_t number_of_side;
	uint16_t start_track;
	uint16_t number_of_track;
}msa_header;

#pragma pack()
