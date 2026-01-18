/*
//
// Copyright (C) 2006-2026 Jean-François DEL NERO
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

//
// MFI file format documentation used :
// https://github.com/mamedev/mame/blob/master/src/lib/formats/mfi_dsk.cpp
//
/*
  Mess floppy image structure:

  - header with signature, number of cylinders, number of heads.  Min
    track and min head are considered to always be 0.  The two top bits
    of the cylinder count is the resolution: 0=tracks, 1=half tracks,
    2=quarter tracks.

  - vector of track descriptions, looping on cylinders with the given
    resolution and sub-lopping on heads, each description composed of:
    - offset of the track data in bytes from the start of the file
    - size of the compressed track data in bytes (0 for unformatted)
    - size of the uncompressed track data in bytes (0 for unformatted)

  - track data

  All values are 32-bits lsb first.

  Track data is zlib-compressed independently for each track using the
  simple "compress" function.

  Track data consists of a series of 32-bits lsb-first values
  representing magnetic cells.  Bits 0-27 indicate the position,
  delta-packed (e.g. difference with the previous position, starts at
  0), and bits 28-31 the types.  Type can be:

  - 0, MG_F -> Flux orientation change
  - 1, MG_N -> Non-magnetized zone (neutral)
  - 2, MG_D -> Damaged zone, reads as neutral but cannot be changed by writing
  - 3, MG_E -> End of zone

  Tracks data is aligned so that the index pulse is at the start for soft-
  sectored disks. For hard-sectored disks, the sector hole for the first
  sector is at the start and the index hole is half a sector from the end
  of the track.

  The position is the angular position in units of 1/200,000,000th of
  a turn.  A size in such units, not coincidentally at all, is also
  the flyover time in nanoseconds for a perfectly stable 300rpm drive.
  That makes the standard cell size of a MFM 3.5" DD floppy at 2000
  exactly for instance (2us).  Smallest expected cell size is 500 (ED
  density drives).

  An unformatted track is equivalent to a pair (MG_N, 0), (MG_E,
  199999999) but is encoded as zero-size.

  The "track splice" information indicates where to start writing
  if you try to rewrite a physical disk with the data.  Some
  preservation formats encode that information, it is guessed for
  others.  The write track function of fdcs should set it.  The
  representation is the angular position relative to the index, for
  soft-sectored disks, and the first sector hole for hard-sectored
  disks.

  The media type is divided in two parts.  The first half
  indicate the physical form factor, i.e. all medias with that
  form factor can be physically inserted in a reader that handles
  it.  The second half indicates the variants which are usually
  detectable by the reader, such as density and number of sides.

  TODO: big-endian support
*/

#define CYLINDER_MASK    0x3fffffff
#define RESOLUTION_SHIFT 30

#define TIME_MASK        0x0fffffff
#define MG_MASK          0xf0000000
#define MG_SHIFT         28

#define OLD_MG_A  (0 << MG_SHIFT)
#define OLD_MG_B  (1 << MG_SHIFT)
#define OLD_MG_N  (2 << MG_SHIFT)
#define OLD_MG_D  (3 << MG_SHIFT)

#define MG_F      (0 << MG_SHIFT)    // MG_F -> Flux orientation change
#define MG_N      (1 << MG_SHIFT)    // MG_N -> Non-magnetized zone (neutral)
#define MG_D      (2 << MG_SHIFT)    // MG_D -> Damaged zone, reads as neutral but cannot be changed by writing
#define MG_E      (3 << MG_SHIFT)    // MG_E -> End of zone

#pragma pack(1)

typedef struct mfiheader_t_
{
	uint8_t  mfi_signature[16];      // "MESSFLOPPYIMAGE" or "MAMEFLOPPYIMAGE"
	uint32_t cyl_count;              //
	uint32_t head_count;             //
	uint32_t form_factor;
	uint32_t variant;
} mfiheader_t;

//   Track Header information :

typedef struct mfitrack_t_
{
	uint32_t offset;
	uint32_t compressed_size;
	uint32_t uncompressed_size;
	uint32_t write_splice;
} mfitrack_t;

#pragma pack()
