/*
//
// Copyright (C) 2006-2014 Jean-François DEL NERO
//
// This file is part of the HxCFloppyEmulator library
//
// HxCFloppyEmulator may be used and distributed without restriction provided
// that this copyright statement is not removed from the file and that any
// derivative work contains the original copyright notice and the associated
// disclaimer.
//
// HxCFloppyEmulator is free software; you can redistribute it
// and/or modify  it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// HxCFloppyEmulator is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//   See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with HxCFloppyEmulator; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//
*/

/*
by Steven Seagal » Sat May 24, 2014 10:33 am
Below is the specification of a new disk image format I propose.
This could change as Steem v3.7 will not be released before a while. Remarks, critics welcome.

I edit this first post to keep specification up-to-date.

STW Version 1.0
===============================================================================

STW is yet another Atari ST disk image format, devised for Steem SSE as
of v3.7.

The W in STW stands for 'write', ST is a reference to well known ST format
and of course to the Atari ST itself.

The purpose of this new format is to allow emulation of all WD1772 (floppy
disk controller) commands in Steem SSE, and keep the results of command
Write Track (Format).

We want to emulate what a drive on the Atari ST, controlled by the WD1772
itself, was able to do. If a disk could be copied by one of the software
copiers (ProCopy, ACopy, etc.) then it should work in Steem.
But we don't try to replicate the work of other devices (Amiga, Trace
duplicator, Hardcopier, Discovery cartridge).
Only what you could do with your standard ST.

Like ST, STW is mainly a dump of the tracks, but with clock info as well
as data bytes. Hence each data byte (8bit) is represented by one word (16bit)
in the image.

All track bytes are included, not just the sector data but also format bytes
(gaps and address marks).

This increases file size but reduces complexity of the image itself
(no need for apart ID field tables etc.) and allows to handle more cases,
such as data inside gaps.
This also forces one to write a lower-level (at byte level) emulation able to
handle such raw images.

This makes the files more than twice as big as an ST image, about 2 MB
uncompressed.

STW disks are created in Steem's Disk manager, on right click.
Then they're used like any other disk image, but you must format them in
the GEM or use a copier or other ST disk utility on them. Before that they're
just full of random data.

STW is meant to be simple, there's no support for "HFE" or other formats,
it's another story.

Note
----

All words are, of course, in big endian format (Motorola), as opposed to
Intel's little endian.

Header
------

4 bytes: "STW", case-sensitive, null-terminated C string
1 word: version in hex ($0100 to begin with, meaning 1.00)
If the version number is $200 or higher, Steem 3.7.0 will refuse to handle
the image.

1 byte: #sides. Should be 2.
1 byte: #tracks. Should be 84.
1 word: #bytes per track. Should be 6256.

The parameters for the number of sides, tracks, bytes are meant to be
constant, so that you may format the same image as a single sided disk
and later as double sided.
The number of tracks and of bytes is open for discussion.
There's no option for those in Steem.

Tracks
------

There are normally 84 tracks per side, 2 sides.

The order of tracks is:

Side 0, track 0 - side 1, track 0 - side 0, track 1 - side 1, track 1 - ...

Track header
------------
Each track is preceded by a 5 bytes header to help debugging and
navigating with a hex editor.
3 bytes: "TRK" (no null character termination).
1 byte: side
1 byte: track
This may be seen as "metaformat", as those headers should never
change during the life of the STW image.

Track data
----------
Track data is represented by, normally, 6256 words, as specified in
the header (1 word per data byte).

Each word is made up of 1 clock byte and 1 data byte, encoded in MFM.
Address marks have a missing clock bit (bit 5 for $A1, bit 4 for $C2).
So the $A1 address marks are encoded $4489.

MFM encoding is overkill, but it's funnier that way.
We could as well have written clock and data byte separately, and spare
some code.
*/

// All structure datas are packed !
#pragma pack(1)

typedef struct STWIMG_
{
	uint8_t  headername[4];			// "STW\0"

	uint16_t version;				// 0x0100

	uint8_t  number_of_side;
	uint8_t  number_of_track;

	uint16_t bytes_per_tracks;
}STWIMG;

typedef struct STWTRACKIMG_
{
	uint8_t  trkheader[3];			// "TRK"
	uint8_t  side_number;
	uint8_t  track_number;
}STWTRACKIMG;

#pragma pack()
