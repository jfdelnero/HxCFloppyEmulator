/*
//
// Copyright (C) 2006-2024 Jean-François DEL NERO
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
DFI image format

Format originator: DiscFerret software.

Overall format

All integers in this file format are stored in big-endian form.

The DFI file consists of a 4-byte magic string, followed by a series of disc sample blocks.

The magic string is "DFER" for old-style DiscFerret images, or "DFE2" for new-style DiscFerret images.

Each sample block has a header --

-----------------------

uint16_be cylinder;
uint16_be head;
uint16_be sector;
uint32_be data_length;

-----------------------

The cylinder number starts at zero and counts up to the number of cylinders on the disk.
The head number follows the same rule (starts at zero, increments for each additional head).
The sector number is optional, and only used for hard-sectored discs.
For soft-sectored discs, it is set to zero. Data_length indicates the number of bytes of data which follow.

Decoding data

----------------------------------------------------------------------
--- Old-style images ---

carry = 0
For every byte in the stream:
  if (byte AND 0x7f) == 0x00:
    carry = carry + 127
  else:
    emit((byte AND 0x7f) + carry)
    carry = 0
if carry > 0:
  emit(carry)

----------------------------------------------------------------------
--- New-style images ---

carry = 0   // running carry
abspos = 0  // absolute timing position in stream
For every byte in the stream:
  if ((byte AND 0x7f) == 0x7f):    // if lower 7 bit value is 0x7f
    carry = carry + 127            // ... then there was a carry
    abspos = abspos + 127
  else if (byte AND 0x80) != 0:    // if high bit set in byte
    carry = carry + (byte & 0x7F)  // add lower 7 bit value to carry and absolute-position
    abspos = abspos + (byte & 0x7F)
    add_index_position(abspos)     // this store was caused by an index pulse: save its absolute position
  else:                            // here byte < 0x7f
    emit((byte AND 0x7f) + carry)  // this store was caused by a data transition: store the time-delta since last transition
    abspos = abspos + (byte & 0x7F)
    carry = 0                      // reset carry

// Carry may be nonzero at the end of this loop. In this case, there was an incomplete transition, which should be discarded.

// emit()                 stores the timing delta for a data pulse
// add_index_position()   stores the absolute timing position of an index pulse
//

*/


// Header Flags
#define INDEXMARK     0x01
#define DISK_96TPI    0x02
#define DISK_360RPM   0x04
#define DISK_RWENABLE 0x08

#pragma pack(1)

typedef struct dfi_header_
{
	uint8_t  sign[4];               // "DFER" or "DFE2"
}dfi_header;

typedef struct dfi_block_header_
{
	uint16_t cylinder;              // big endian !
	uint16_t head;                  // big endian !
	uint16_t sector;                // big endian !
	uint32_t data_length;           // big endian !
}dfi_block_header;

#pragma pack()
