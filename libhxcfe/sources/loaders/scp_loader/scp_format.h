/*
//
// Copyright (C) 2006 - 2013 Jean-François DEL NERO
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
The SuperCard Pro image file format will handle flux level images for any type of disk,
be it 5.25" or 3.5", GCR, MFM, etc.

All longwords are in little endian format.  The words used for the flux length in the
actual track data are in big endian format... sorry, that's just how it worked out for
the SuperCard Pro hardware.

The header contains the ASCII of "SCP" as the first 3 bytes. If this is not found, then
the file is not ours.

BYTE 0x03 is the version/revision as a byte.  This is encoded as (Version<<4|Revision),
so that 0x39= version 3.9 of the format.

BYTE 0x04 is the disk type and represents the type of disk for the image.

BYTE 0x05 is the number of revolutions, which determines how many revolutions for each track
is contained in the image.

BYTES 0x06 and 0x07 are the start track and end track bytes.  Tracks are numbered 0-163.

BYTE 0x08 is the flag bits.  This byte contains information about how the image was produced.
The bits are defined as follows:

Bit 0 - INDEX, cleared if the image did not use the index mark for queuing tracks
               set is the image used the index mark to queue tracks

Bit 1 - TPI, cleared if the drive is a 48TPI drive
             set if the drive is a 96TPI drive

Bit 2 - RPM, cleared if the drive is a 300 RPM drive
             set if the drive is a 360 RPM drive

Bit 3 - TYPE, cleared if the image is a read-only type
              set if the image is read/write capable

It should be noted that most images will be read-only (write protected) for emulation
usage.  The read/write capable images contain padded space to allow the track to
change size within the image.

BYTE 0x09 is the width of the bit cell time.  Normally this is always 16 bits, but if the
value is non-zero, then it represents the number of bits for each bit cell entry.  For
example, if this byte was set to 8, then each bit cell entry would be 8 bits wide, not
the normal 16 bits.  This is for future expansion, and may never be actually used.

BYTES 0x0A and 0x0B are reserved for future use.

BYTES 0x0C-0x0F are the 32 bit checksum of data starting from offset 0x10 through the
end of the image file.  Checksum is standard addition, with a wrap beyond 32 bits
rolling over.

BYTES 0x10-0x13 are the 32 bit offset (from the start of the file) to the 1st track's
data header.  There is an offset entry for every track, with up to 164 tracks supported.
This means that disk images of up to 82 tracks with sides 0/1 are possible.  If no
track data for a track is present, then the entry will contain a longword of zeros
(0x00000000).  The 1st track header should start at offset 0x000002A8, but could
technically start anywhere in the file.  Since offsets are used, track data does not
have to be in any order, so you could append track data to the file and change the
header to point to the appended data.  For simplicity it is recommended that you follow
the normal image format shown below, with all tracks in sequential order.

As stated, each offset entry points to a track data header.  That header contains
information about the flux data for that track.  The help recover corrupt files,
each entry has its own header starting with "TRK" as the first 3 bytes of the header.

BYTE 0x03 contains the track number (0-163).  For single sided disks, tracks 0-41 (48TPI)
or 0-83 (96TPI) are used.  For double-sided disks, the actual track number is this value
divided by 2.  The remainder (0 or 1) is the head.  0 represents the bottom head and
1 represents the top head.  For example, 0x0B would be 11/2 = track 5, head 1 (top).
Likewise, 0x50 would be 80/2 = track 40, head 0 (bottom).

BYTES 0x04-0x07 contain the index time for the 1st revolution.  This is the time from
index pulse to index pulse for the track data.  This will be either ~360RPMs or ~300 RPMs
depending the drive.  It is important to have this exact time as it is necessary when
writing an image back to a real disk.  The index time is a value in nanoseconds/25ns for
one revolution.  Multiply the value by 25 to get the exact value.

BYTES 0x08-0x0B contain the length of the track in bit cells (flux transitions).  If a
16 bit width is used (selected above), then this value *2 will be the total number of
bytes of data that is used for that track (16 bit = 2 bytes per word).

There are 5 entries (starting at BYTES 0x08-0x0B), one for each revolution (up to
5 revolutions) contained in a table that follows the track number.  If an entry contains
all zeros (0x00000000) then no more data is available for that track.

BYTES 0x2C-0x2F are the 32 bit checksum of all of the track data (all revolutions combined).

BYTE 0x30 is the start of the 1st track's data.  Normally when multiple revolutions are used
the tracks are stored end to end, like the disk is spinning multiple times.  No break
or skew of any kind results from storage like this unless writing support is necessary
in which case there is padded space between the tracks.

After the last byte of track data there will be a timestamp.  This timestamp is an
ASCII string, ie: 10/15/2013 5:52:30 PM



; ------------------------------------------------------------------
; SCP IMAGE FILE FORMAT
; ------------------------------------------------------------------
;
; 0x00              "SCP" (ASCII CHARS)
; 0x03              VERSION (nibbles major/minor)
; 0x04              DISK TYPE (0=CBM, 1=AMIGA, 2=APPLE II, 3=ATARI
;                   ST, 4=ATARI 800, 5=MAC 800, 6=360K/720K, 7=1.44MB
; 0x05              NUMBER OF REVOLUTIONS (2=default)
; 0x06              START TRACK (0-165)
; 0x07              END TRACK (0-165)
; 0x08              FLAG BITS (0=INDEX, 1=TPI, 2=RPM, 3=TYPE)
; 0x09              WIDTH OF BIT CELL TIMES (0=16 BITS, >0=WIDTH)
; 0x0A-0x0B         RESERVED (2 BYTES)
; 0x0C-0x0F         32 BIT CHECKSUM OF DATA FROM 0x10-EOF
; 0x10              OFFSET TO 1st TRACK DATA HEADER (4 bytes of 0 if track is skipped)
; 0x14              OFFSET TO 2nd TRACK DATA HEADER (4 bytes of 0 if track is skipped)
; 0x18              OFFSET TO 3rd TRACK DATA HEADER (4 bytes of 0 if track is skipped)
; ....
; 0x02A4-0x02A7     RESERVED (4 BYTES)
; 0x02A8            START OF 1st TRACK DATA HEADER (SEE BELOW FOR STRUCTURE INFO)
;
; ....              END OF TRACK DATA
; ????              TIMESTAMP (AS ASCII STRING - ie. 7/17/2013 12:45:49 PM)

; ## FILE FORMAT DEFINES ##

IFF_ID = 0x00                      ; "SCP" (ASCII CHARS)
IFF_VER = 0x03                     ; version (nibbles major/minor)
IFF_DISKTYPE = 0x04                ; disk type (0=CBM, 1=AMIGA, 2=APPLE II, 3=ATARI ST, 4=ATARI 800, 5=MAC 800, 6=360K/720K, 7=1.44MB)
IFF_NUMREVS = 0x05                 ; number of revolutions (2=default)
IFF_START = 0x06                   ; start track (0-165)
IFF_END = 0x07                     ; end track (0-165)
IFF_FLAGS = 0x08                   ; flag bits (0=INDEX READ, 1=HALF TRACKS)
IFF_RSRVED = 0x09                  ; 3 bytes of reserved space
IFF_CHECKSUM = 0x0C                ; 32 bit checksum of data added together starting at 0x0010 through EOF
IFF_THOFFSET = 0x10                ; first track data header offset
IFF_THDSTART = 0x2A8               ; start of first track data header/track data


; ------------------------------------------------------------------
; TRACK DATA HEADER FORMAT
; ------------------------------------------------------------------
;
; 0000              'TRK' (ASCII CHARS)             - 3 chars
; 0003              TRACK NUMBER                    - 1 byte
; 0004              INDEX TIME (1st REVOLUTION)     - 4 bytes
; 0008              TRACK LENGTH (1st REVOLUTION)   - 4 bytes
; 000C              DATA OFFSET (1st REVOLUTION)    - 4 bytes (offset is from start of Track Data Header - should always be 0x00000044 for this entry)
; 0010              INDEX TIME (2nd REVOLUTION)     - 4 bytes of 0x00000000 for no data
; 0014              TRACK LENGTH (2nd REVOLUTION)   - 4 bytes of 0x00000000 for no data
; 0018              DATA OFFSET (2nd REVOLUTION)    - 4 bytes of 0x00000000 for no data
; 001C              INDEX TIME (3rd REVOLUTION)     - 4 bytes of 0x00000000 for no data
; 0020              TRACK LENGTH (3rd REVOLUTION)   - 4 bytes of 0x00000000 for no data
; 0024              DATA OFFSET (3rd REVOLUTION)    - 4 bytes of 0x00000000 for no data
; 0028              INDEX TIME (4th REVOLUTION)     - 4 bytes of 0x00000000 for no data
; 002C              TRACK LENGTH (4th REVOLUTION)   - 4 bytes of 0x00000000 for no data
; 0030              DATA OFFSET (4th REVOLUTION)    - 4 bytes of 0x00000000 for no data
; 0034              INDEX TIME (5th REVOLUTION)     - 4 bytes of 0x00000000 for no data
; 0038              TRACK LENGTH (5th REVOLUTION)   - 4 bytes of 0x00000000 for no data
; 003C              DATA OFFSET (5th REVOLUTION)    - 4 bytes of 0x00000000 for no data
; 0040              32 BIT CHECKSUM OF USED TRACK DATA
;
; 0044              START OF DATA FOR 1st REVOLUTION (not mandatory, but recommended default)
; ....              END OF TRACK DATA
;
; INDEX TIME = 32 BIT VALUE, TIME IN NANOSECONDS/25ns FOR ONE REVOLUTION
;
; i.e. 0x7A1200 = 8000000, 8000000*25 = 200000000 = 200.000000ms
;
; TRACK DATA = 16 BIT VALUE, TIME IN NANOSECONDS/25ns FOR ONE BIT CELL TIME
;
; i.e. 0x00DA = 218, 218*25 = 5450ns = 5.450us

; ## TRACK DATA HEADER DEFINES ##

TDH_ID = &H0                       ; "TRK" (ASCII CHARS)
TDH_TRACKNUM = &H3                 ; track number
TDH_INDEXTIME = &H4                ; index pulse time (1st revolution)
TDH_LENGTH = &H8                   ; length of track (1st revolution)
TDH_OFFSET = &HC                   ; offset to track data from start of TDH (1st revolution)
TDH_INDEXTIME2 = &H10              ; index pulse time (2nd revolution)
TDH_LENGTH2 = &H14                 ; length of track (2nd revolution)
TDH_OFFSET2 = &H18                 ; offset to track data from start of TDH (2nd revolution)
TDH_INDEXTIME3 = &H1C              ; index pulse time (3rd revolution)
TDH_LENGTH3 = &H20                 ; length of track (3rd revolution)
TDH_OFFSET3 = &H24                 ; offset to track data from start of TDH (3rd revolution)
TDH_INDEXTIME4 = &H28              ; index pulse time (4th revolution)
TDH_LENGTH4 = &H2C                 ; length of track (4th revolution)
TDH_OFFSET4 = &H30                 ; offset to track data from start of TDH (4th revolution)
TDH_INDEXTIME5 = &H34              ; index pulse time (5th revolution)
TDH_LENGTH5 = &H38                 ; length of track (5th revolution)
TDH_OFFSET5 = &H3C                 ; offset to track data from start of TDH (5th revolution)
TDH_Checksum = &H40                ; 32 bit checksum
TDH_SIZEOF = &H44                  ; total length of track data header
*/

// Header Flags
#define INDEXMARK     0x01
#define DISK_96TPI    0x02
#define DISK_360RPM   0x04
#define DISK_RWENABLE 0x08

#pragma pack(1)

typedef struct scp_header_
{
	unsigned char sign[3];				// "SCP"
	unsigned char version;				// Version<<4|Revision
	unsigned char disk_type;
	unsigned char number_of_revolution;
	unsigned char start_track;
	unsigned char end_track;
	unsigned char flags;
	unsigned char RFU_0;
	unsigned char RFU_1;
	unsigned char RFU_2;
	unsigned long file_data_checksum;
}scp_header;

typedef struct scp_index_pos_
{
	unsigned long index_time;
	unsigned long track_lenght;
	unsigned long track_offset;
}scp_index_pos;

typedef struct scp_track_header_
{
	unsigned char trk_sign[3];				// "TRK"
	unsigned char track_number;
	scp_index_pos index_position[5];
	unsigned long track_data_checksum;
	//unsigned long total_lenght;
}scp_track_header;

#pragma pack()
