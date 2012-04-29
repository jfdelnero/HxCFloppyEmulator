/*
//
// Copyright (C) 2006 - 2012 Jean-François DEL NERO
//
// This file is part of HxCFloppyEmulator.
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
///////////////////////////////////////////////////////////////////////////////////
//-------------------------------------------------------------------------------//
//-------------------------------------------------------------------------------//
//-----------H----H--X----X-----CCCCC----22222----0000-----0000------11----------//
//----------H----H----X-X-----C--------------2---0----0---0----0--1--1-----------//
//---------HHHHHH-----X------C----------22222---0----0---0----0-----1------------//
//--------H----H----X--X----C----------2-------0----0---0----0-----1-------------//
//-------H----H---X-----X---CCCCC-----222222----0000-----0000----1111------------//
//-------------------------------------------------------------------------------//
//----------------------------------------------------- http://hxc2001.free.fr --//
///////////////////////////////////////////////////////////////////////////////////
// File : usb_hxcfloppyemulator.h
// Contains: USB HxC FE support functions
//
// Written by:	DEL NERO Jean Francois
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////


#define NEXTPACKET_EVENT 0x1
#define TRACKCHANGED_EVENT 0x2
#define INCOMINGPACKET_EVENT 0x4

#define STATUS_ERROR 0
#define STATUS_LOOKINGFOR 1
#define STATUS_ONLINE 2


typedef struct _USBStats
{
	unsigned long totaldataout;
	unsigned long dataout;
	
	unsigned long synclost;

	unsigned long packetsize;
	unsigned long totalpacketsent;
	unsigned long packetsent;
	
}USBStats;

typedef struct usbtrack_
{
	unsigned char * usbtrack;
	unsigned char * randomusbtrack;
	unsigned long tracklen;
}usbtrack;

typedef struct USBHXCFE_
{
	USBStats usbstats;

	unsigned char number_of_track;
	usbtrack precalcusbtrack[256];

	unsigned char *randomlut;
	unsigned long hw_handle;
	
	unsigned char interface_mode;
	unsigned int  drive_select_source;
	unsigned char double_step;

	unsigned char current_track;

	//	unsigned char floppyloaded;
	unsigned char floppychanged;

	unsigned long trackbuffer_pos;

	unsigned char start_emulation;
	unsigned char stop_emulation;
	unsigned char running;

	unsigned char status; // 0 -> error  1->lookingfor 2->online

}USBHXCFE;

USBHXCFE * libusbhxcfe_init(HXCFLOPPYEMULATOR* floppycontext);
int libusbhxcfe_deInit(HXCFLOPPYEMULATOR* floppycontext,USBHXCFE * hwif);
int libusbhxcfe_loadFloppy(HXCFLOPPYEMULATOR* floppycontext,USBHXCFE * hwif,FLOPPY * floppydisk);
int libusbhxcfe_ejectFloppy(HXCFLOPPYEMULATOR* floppycontext,USBHXCFE * hwif);
int libusbhxcfe_getStats(HXCFLOPPYEMULATOR* floppycontext,USBHXCFE * hwif,USBStats* stats,int clear);
int libusbhxcfe_setInterfaceMode(HXCFLOPPYEMULATOR* floppycontext,USBHXCFE * hwif,int interfacemode,int doublestep,int drive);
int libusbhxcfe_setUSBBufferSize(HXCFLOPPYEMULATOR* floppycontext,USBHXCFE * hwif,int size);
int libusbhxcfe_getInterfaceMode(HXCFLOPPYEMULATOR* floppycontext,USBHXCFE * hwif);
int libusbhxcfe_getDoubleStep(HXCFLOPPYEMULATOR* floppycontext,USBHXCFE * hwif);
int libusbhxcfe_getDrive(HXCFLOPPYEMULATOR* floppycontext,USBHXCFE * hwif);
int libusbhxcfe_getCurTrack(HXCFLOPPYEMULATOR* floppycontext,USBHXCFE * hwif);
