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
////////////////////////////////////////////////////////////////////////////////////////////////////

#define VARIABLEBITRATE                    -1
#define VARIABLEENCODING                 1

typedef struct _HXCFE_SIDE
{
    unsigned int    number_of_sector;        // Number of sectors per track (if know) -> -1 if unknow.
    unsigned char * databuffer;              // data buffer.
    long            bitrate;                 // if equal to VARIABLEBITRATE timingbuffer must be used.
    unsigned long * timingbuffer;            // bitrate buffer.
    unsigned char * flakybitsbuffer;         // if equal to zero no flakey/weak bits information.
    unsigned char * indexbuffer;             // index state buffer signal 1->asserted 0->non asserted.
    unsigned char * track_encoding_buffer;   // track encoding indication buffer.
    
    unsigned char   track_encoding;

    unsigned long   tracklen;                // databuffer/timingbuffer/flakybitsbuffer/indexbuffer length
}HXCFE_SIDE;
#define _HXCFE_SIDE_



typedef struct _HXCFE_CYLINDER
{
    unsigned short  floppyRPM;                // rotation par minute (informatif/optionnel)
    unsigned char   number_of_side;
    HXCFE_SIDE  **  sides;
}HXCFE_CYLINDER;
#define _HXCFE_CYLINDER_



typedef struct _HXCFE_FLOPPY
{
    unsigned int    floppyBitRate;

    unsigned char   floppyNumberOfSide;
    unsigned short  floppyNumberOfTrack;
    unsigned short  floppySectorPerTrack;

    unsigned short  floppyiftype;
    unsigned char   double_step;

    HXCFE_CYLINDER ** tracks;
}HXCFE_FLOPPY;
#define _HXCFE_FLOPPY_
