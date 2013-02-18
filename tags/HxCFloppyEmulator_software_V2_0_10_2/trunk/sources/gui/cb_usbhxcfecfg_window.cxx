/*
//
// Copyright (C) 2006 - 2013 Jean-François DEL NERO
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
#include "main.h"
#include "loader.h"

extern s_gui_context * guicontext;

void tick_usb(void *v) {
	char tempstr[512];
	unsigned long datathroughput;
	unsigned long period;
	float packetpersecond;
	USBStats stats;
	int status;
	usbhxcfecfg_window *window;
	
	window=(usbhxcfecfg_window *)v;
	
	status=libusbhxcfe_getStats(guicontext->hxcfe,guicontext->usbhxcfe,&stats,0);

	sprintf(tempstr,"%d (%d p/s)",(int)stats.totalpacketsent,(int)stats.packetsent);
	stats.packetsent=0;
	window->strout_packetsent->value((const char*)tempstr);				
			
	window->valout_synclost->value(stats.synclost);

	if(stats.totaldataout<(1024*1024))
	{			
		sprintf(tempstr,"%d bytes",(int)stats.totaldataout);
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
				
				
	datathroughput=stats.dataout * 2;
	sprintf(tempstr,"%d bytes/second",(int)datathroughput);
	window->strout_datathroughput->value((const char*)tempstr);
				
	period=0;	
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
				
	sprintf(tempstr,"%d ms",(int)period*2);
	window->strout_maxsettletime->value((const char*)tempstr);
				
	sprintf(tempstr,"%d ms",(int)period);
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

	Fl::repeat_timeout(0.50, tick_usb, v);
}

void usbifcfg_window_datachanged(Fl_Widget * w,void *)
{
	usbhxcfecfg_window *usbcfgw;
	Fl_Widget* tw;

	tw=w;
	do
	{
		tw=tw->parent();
		usbcfgw=(usbhxcfecfg_window *)tw->user_data();
	}while(!usbcfgw);

	if(usbcfgw->chk_twistedcable->value())
	{
		if(usbcfgw->rbt_ds0->value())
			guicontext->driveid=3;
		if(usbcfgw->rbt_ds1->value())
			guicontext->driveid=2;
		if(usbcfgw->rbt_ds2->value())
			guicontext->driveid=1;
		if(usbcfgw->rbt_ds3->value())
			guicontext->driveid=0;
	}
	else
	{
		if(usbcfgw->rbt_ds3->value())
			guicontext->driveid=3;
		if(usbcfgw->rbt_ds2->value())
			guicontext->driveid=2;
		if(usbcfgw->rbt_ds1->value())
			guicontext->driveid=1;
		if(usbcfgw->rbt_ds0->value())
			guicontext->driveid=0;
	}

	if(usbcfgw->chk_disabledrive->value())
		guicontext->driveid=guicontext->driveid | 0x4 ;
	else
		guicontext->driveid=guicontext->driveid & ~0x4 ;

	libusbhxcfe_setInterfaceMode(guicontext->hxcfe,guicontext->usbhxcfe,guicontext->interfacemode,guicontext->doublestep,guicontext->driveid);
	libusbhxcfe_setUSBBufferSize(guicontext->hxcfe,guicontext->usbhxcfe, (int)usbcfgw->slider_usbpacket_size->value() );
}

void ifcfg2_window_datachanged(Fl_Widget * w,void *)
{
	usbhxcfecfg_window *usbcfgw;
	Fl_Widget* tw;

	tw=w;
	do
	{
		tw=tw->parent();
		usbcfgw=(usbhxcfecfg_window *)tw->user_data();
	}while(!usbcfgw);

	if(!usbcfgw->chk_autoifmode->value())
	{
		guicontext->autoselectmode = 0x00;
	}
	else
	{
		guicontext->autoselectmode = 0xFF;
	}

	if(!usbcfgw->chk_doublestep->value())
	{
		guicontext->doublestep = 0x00;
	}
	else
	{
		guicontext->doublestep = 0xFF;
	}

}

void resetusbstat_bt(Fl_Button *w,void *)
{
	USBStats stats;

	libusbhxcfe_getStats(guicontext->hxcfe,guicontext->usbhxcfe,&stats,1);
}