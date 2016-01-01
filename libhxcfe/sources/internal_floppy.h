/*
//
// Copyright (C) 2006-2016 Jean-François DEL NERO
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
////////////////////////////////////////////////////////////////////////////////////////////////////

#define VARIABLEBITRATE                    -1
#define VARIABLEENCODING                 1

typedef struct _HXCFE_SIDE
{
    int32_t         number_of_sector;        // Number of sectors per track (if know) -> -1 if unknow.
    uint8_t       * databuffer;              // data buffer.
    int32_t         bitrate;                 // if equal to VARIABLEBITRATE timingbuffer must be used.
    uint32_t      * timingbuffer;            // bitrate buffer.
    uint8_t       * flakybitsbuffer;         // if equal to zero no flakey/weak bits information.
    uint8_t       * indexbuffer;             // index state buffer signal 1->asserted 0->non asserted.
    uint8_t       * track_encoding_buffer;   // track encoding indication buffer.
    
    int32_t         track_encoding;

    int32_t         tracklen;                // databuffer/timingbuffer/flakybitsbuffer/indexbuffer length
}HXCFE_SIDE;
#define _HXCFE_SIDE_



typedef struct _HXCFE_CYLINDER
{
    int32_t         floppyRPM;                // rotation par minute (informatif/optionnel)
    int32_t         number_of_side;
    HXCFE_SIDE  **  sides;
}HXCFE_CYLINDER;
#define _HXCFE_CYLINDER_



typedef struct _HXCFE_FLOPPY
{
    int32_t         floppyBitRate;

    int32_t         floppyNumberOfSide;
    int32_t         floppyNumberOfTrack;
    int32_t         floppySectorPerTrack;

    int32_t         floppyiftype;
    int32_t         double_step;

    HXCFE_CYLINDER ** tracks;
}HXCFE_FLOPPY;
#define _HXCFE_FLOPPY_
