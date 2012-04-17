/*
//
// Copyright (C) 2006-2012 Jean-François DEL NERO
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
// File : cb_usbhxcfecfg_window.cxx
// Contains: USB HxC Setting window
//
// Written by:	DEL NERO Jean Francois
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include "usbhxcfecfg_window.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "fileselector.h"


extern "C"
{
	#include "libhxcfe.h"
	#include "usb_hxcfloppyemulator.h"
}

#include "loader.h"

extern HXCFLOPPYEMULATOR * flopemu;
extern USBHXCFE * usbhxcfe;


/*platform platformlist[]=
{
	{ AMIGA_DD_FLOPPYMODE,"Amiga","DS0","DS1","DS2","MTRON"},
	{ AMIGA_HD_FLOPPYMODE,"Amiga HD","DS0","DS1","DS2","MTRON"},
	{ ATARIST_DD_FLOPPYMODE,"Atari ST","D0SEL","D1SEL","-","MTRON"},
	{ ATARIST_HD_FLOPPYMODE,"Atari ST HD","D0SEL","D1SEL","-","MTRON"},
	{ IBMPC_DD_FLOPPYMODE,"IBM PC 720kB","MOTEA","DRVSB","DRVSA","MOTEB"},
	{ IBMPC_HD_FLOPPYMODE,"IBM PC 1.44MB","MOTEA","DRVSB","DRVSA","MOTEB"},
	{ IBMPC_ED_FLOPPYMODE,"IBM PC 2.88MB","MOTEA","DRVSB","DRVSA","MOTEB"},
	{ CPC_DD_FLOPPYMODE,"Amstrad CPC","Drive Select 0","Drive Select 1","-","MOTOR ON"},
	{ MSX2_DD_FLOPPYMODE,"MSX 2","DS0","DS1","DS2","MTRON"},
	{ GENERIC_SHUGART_DD_FLOPPYMODE,"Generic Shugart","DS0","DS1","DS2","MTRON"},
	{ EMU_SHUGART_FLOPPYMODE,"Emu Shugart","DS0","DS1","DS2","MTRON"},
	{ C64_DD_FLOPPYMODE,"C64 1541","NA","NA","NA","NA"},
	{ -1,"?","DS0","DS1","DS2","MTRON"}
};*/


void tick_usb(void *v) {
	char tempstr[512];
//	unsigned long packetsize;
	unsigned long datathroughput;
	unsigned long period;
	float packetpersecond;
	USBStats stats;
	int status;
	usbhxcfecfg_window *window;
	
	window=(usbhxcfecfg_window *)v;
	
	status=libusbhxcfe_getStats(flopemu,usbhxcfe,&stats,0);

	sprintf(tempstr,"%d (%d p/s)",stats.totalpacketsent,stats.packetsent);
	stats.packetsent=0;
	window->strout_packetsent->value((const char*)tempstr);				
			
	window->valout_synclost->value(stats.synclost);

	if(stats.totaldataout<(1024*1024))
	{			
		sprintf(tempstr,"%d bytes",stats.totaldataout);
	}
	else
	{
		if(stats.totaldataout<(1024*1024*1024))
		{
			sprintf(tempstr,"%4.2f MB",(float)(stats.totaldataout)/(float)(1024*1024));
		}
		else
		{
			sprintf(tempstr,"%4.2f GB",(float)(stats.totaldataout)/(float)(1024*1024*1024));
		}				
	}
	window->strout_datasent->value((const char*)tempstr);
				
				
	datathroughput=stats.dataout;
	sprintf(tempstr,"%d bytes/second",stats.dataout);
	window->strout_datathroughput->value((const char*)tempstr);
				
				
	if(stats.packetsize)
	{
		packetpersecond=(float)datathroughput/(float)stats.packetsize;
		if(packetpersecond)
		{
			period=(unsigned long)(1000/(float)packetpersecond);
		}
		else
		{
			period=0;
		}
	}
				
	sprintf(tempstr,"%d ms",period*2);
	window->strout_maxsettletime->value((const char*)tempstr);
				
	sprintf(tempstr,"%d ms",period);
	window->strout_minsettletime->value((const char*)tempstr);

	switch(status)
	{
		case STATUS_ERROR:
			window->strout_usbhfestatus->value((const char*)"FTDI D2XX Driver not installed!");
		break;

		case STATUS_LOOKINGFOR:
			window->strout_usbhfestatus->value((const char*)"USB HxC Floppy Emulator not detected!");
		break;

		case STATUS_ONLINE:
			window->strout_usbhfestatus->value((const char*)"USB HxC Floppy Emulator ready!");
		break;

		default:
			window->strout_usbhfestatus->value((const char*)"Unknow status !");
		break;
					
	}


	switch( libusbhxcfe_getDrive(flopemu,usbhxcfe) & 0x3 )
	{
		case 0:
			window->rbt_ds0->set();
			window->rbt_ds1->clear();
			window->rbt_ds2->clear();
			window->rbt_ds3->clear();
		break;

		case 1:
			window->rbt_ds0->clear();
			window->rbt_ds1->set();
			window->rbt_ds2->clear();
			window->rbt_ds3->clear();
		break;
		
		case 2:
			window->rbt_ds0->clear();
			window->rbt_ds1->clear();
			window->rbt_ds2->set();
			window->rbt_ds3->clear();
		break;
		case 3:
			window->rbt_ds0->clear();
			window->rbt_ds1->clear();
			window->rbt_ds2->clear();
			window->rbt_ds3->set();
		break;
	}
			
	if( libusbhxcfe_getDrive(flopemu,usbhxcfe) >3)
	{
		window->chk_disabledrive->set();
	}
	else
	{
		window->chk_disabledrive->clear();
	}

	if(libusbhxcfe_getDoubleStep(flopemu,usbhxcfe))
	{
		window->chk_doublestep->set();
	}
	else
	{
		window->chk_doublestep->clear();
	}


	Fl::repeat_timeout(0.50, tick_usb, v);
}