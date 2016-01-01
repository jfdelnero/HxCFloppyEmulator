/*
//
// Copyright (C) 2006-2016 Jean-François DEL NERO
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

#ifdef __cplusplus
extern "C" {
#endif

#define NEXTPACKET_EVENT 0x1
#define TRACKCHANGED_EVENT 0x2
#define INCOMINGPACKET_EVENT 0x4

#define STATUS_ERROR 0
#define STATUS_LOOKINGFOR 1
#define STATUS_ONLINE 2


typedef struct _USBStats
{
	uint32_t totaldataout;
	uint32_t dataout;
	
	uint32_t synclost;

	uint32_t packetsize;
	uint32_t totalpacketsent;
	uint32_t packetsent;
	
}USBStats;

typedef struct usbtrack_
{
	uint8_t       * usbtrack;
	uint8_t       * randomusbtrack;
	uint32_t tracklen;
}usbtrack;

typedef struct USBHXCFE_
{
	USBStats usbstats;

	uint8_t       number_of_track;
	usbtrack precalcusbtrack[256];

	uint8_t       *randomlut;
	uint32_t hw_handle;
	
	uint8_t       interface_mode;
	uint32_t      drive_select_source;
	uint8_t       double_step;

	uint8_t       current_track;

	//	unsigned char floppyloaded;
	uint8_t       floppychanged;

	uint32_t      trackbuffer_pos;

	uint8_t       start_emulation;
	uint8_t       stop_emulation;
	uint8_t       running;

	uint8_t       status; // 0 -> error  1->lookingfor 2->online

}USBHXCFE;

USBHXCFE * libusbhxcfe_init(HXCFE* floppycontext);
int32_t libusbhxcfe_deInit(HXCFE* floppycontext,USBHXCFE * hwif);
int32_t libusbhxcfe_loadFloppy(HXCFE* floppycontext,USBHXCFE * hwif,HXCFE_FLOPPY * floppydisk);
int32_t libusbhxcfe_ejectFloppy(HXCFE* floppycontext,USBHXCFE * hwif);
int32_t libusbhxcfe_getStats(HXCFE* floppycontext,USBHXCFE * hwif,USBStats* stats,int32_t clear);
int32_t libusbhxcfe_setInterfaceMode(HXCFE* floppycontext,USBHXCFE * hwif,int32_t interfacemode,int32_t doublestep,int32_t drive);
int32_t libusbhxcfe_setUSBBufferSize(HXCFE* floppycontext,USBHXCFE * hwif,int32_t size);
int32_t libusbhxcfe_getInterfaceMode(HXCFE* floppycontext,USBHXCFE * hwif);
int32_t libusbhxcfe_getDoubleStep(HXCFE* floppycontext,USBHXCFE * hwif);
int32_t libusbhxcfe_getDrive(HXCFE* floppycontext,USBHXCFE * hwif);
int32_t libusbhxcfe_getCurTrack(HXCFE* floppycontext,USBHXCFE * hwif);

#ifdef __cplusplus
}
#endif
