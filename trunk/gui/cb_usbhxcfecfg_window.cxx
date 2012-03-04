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
	#include "hxc_floppy_emulator.h"
	#include "../usb_floppyemulator/usb_hxcfloppyemulator.h"
}

#include "loader.h"

extern HXCFLOPPYEMULATOR * flopemu;
extern HWINTERFACE * hwif;


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
	usbhxcfecfg_window *window;
	
	window=(usbhxcfecfg_window *)v;
	
	sprintf(tempstr,"%d (%d p/s)",hwif->usbstats.totalpacketsent,hwif->usbstats.packetsent);
	hwif->usbstats.packetsent=0;
	window->strout_packetsent->value((const char*)tempstr);				
			
	window->valout_synclost->value(hwif->usbstats.synclost);

	if(hwif->usbstats.totaldataout<(1024*1024))
	{			
		sprintf(tempstr,"%d bytes",hwif->usbstats.totaldataout);
	}
	else
	{
		if(hwif->usbstats.totaldataout<(1024*1024*1024))
		{
			sprintf(tempstr,"%4.2f MB",(float)(hwif->usbstats.totaldataout)/(float)(1024*1024));
		}
		else
		{
			sprintf(tempstr,"%4.2f GB",(float)(hwif->usbstats.totaldataout)/(float)(1024*1024*1024));
		}				
	}
	window->strout_datasent->value((const char*)tempstr);
				
				
	datathroughput=hwif->usbstats.dataout;
	sprintf(tempstr,"%d bytes/second",hwif->usbstats.dataout);
	window->strout_datathroughput->value((const char*)tempstr);
	hwif->usbstats.dataout=0;
				
				
	if(hwif->usbstats.packetsize)
	{
		packetpersecond=(float)datathroughput/(float)hwif->usbstats.packetsize;
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


	if(hwif)
	{
		switch(hwif->status)
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

		switch(hwif->drive_select_source&0x3)
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
			
		if(hwif->drive_select_source>3)
		{
			window->chk_disabledrive->set();
		}
		else
		{
			window->chk_disabledrive->clear();
		}
/*
		if(demo->autoselectmode)
		{
			SendDlgItemMessage(hwndDlg,IDC_AUTO_USB,BM_SETCHECK,BST_CHECKED,0);
			i=0;
			j=0;
			do
			{
				if(hwif->interface_mode==platformlist[i].id)
				{
					j=i;
				}
				i++;
			}while(platformlist[i].id!=-1);
				
			EnableWindow(GetDlgItem(hwndDlg, IDC_COMBO_USBIFMODE),FALSE);
				
			if(!demo->twistedcable)
			{
				SendDlgItemMessage(hwndDlg,IDC_TWISTED,BM_SETCHECK,BST_UNCHECKED,0);
				SetDlgItemText(hwndDlg,IDC_RADIO_DS0,platformlist[j].selectline0_name);
				SetDlgItemText(hwndDlg,IDC_RADIO_DS1,platformlist[j].selectline1_name);
				SetDlgItemText(hwndDlg,IDC_RADIO_DS2,platformlist[j].selectline2_name);
				SetDlgItemText(hwndDlg,IDC_RADIO_DS3,platformlist[j].selectline3_name);
			}
			else
			{
				SendDlgItemMessage(hwndDlg,IDC_TWISTED,BM_SETCHECK,BST_CHECKED,0);
				SetDlgItemText(hwndDlg,IDC_RADIO_DS0,platformlist[j].selectline3_name);
				SetDlgItemText(hwndDlg,IDC_RADIO_DS1,platformlist[j].selectline2_name);
				SetDlgItemText(hwndDlg,IDC_RADIO_DS2,platformlist[j].selectline1_name);
				SetDlgItemText(hwndDlg,IDC_RADIO_DS3,platformlist[j].selectline0_name);
			}

			i=0;
			do
			{
				SendDlgItemMessage(hwndDlg,IDC_COMBO_USBIFMODE,  CB_GETLBTEXT, (WORD)i, (LONG)tempstr);
				
				if(!strcmp(tempstr,platformlist[j].name))
				{
					SendDlgItemMessage(hwndDlg, IDC_COMBO_USBIFMODE, CB_SETCURSEL, i, 0);
				}
				i++;
			}while(platformlist[i].id!=-1);
	
		}
		else
		{
			SendDlgItemMessage(hwndDlg,IDC_AUTO_USB,BM_SETCHECK,BST_UNCHECKED,0);
			EnableWindow(GetDlgItem(hwndDlg, IDC_COMBO_USBIFMODE),TRUE);
			i=0;
			do
			{
				i++;
			}while(platformlist[i].id!=-1 && (platformlist[i].id!=hwif->interface_mode));
				
			if(!demo->twistedcable)
			{
				SendDlgItemMessage(hwndDlg,IDC_TWISTED,BM_SETCHECK,BST_UNCHECKED,0);
				SetDlgItemText(hwndDlg,IDC_RADIO_DS0,platformlist[i].selectline0_name);
				SetDlgItemText(hwndDlg,IDC_RADIO_DS1,platformlist[i].selectline1_name);
				SetDlgItemText(hwndDlg,IDC_RADIO_DS2,platformlist[i].selectline2_name);
				SetDlgItemText(hwndDlg,IDC_RADIO_DS3,platformlist[i].selectline3_name);
			}
			else
			{
				SendDlgItemMessage(hwndDlg,IDC_TWISTED,BM_SETCHECK,BST_CHECKED,0);
				SetDlgItemText(hwndDlg,IDC_RADIO_DS0,platformlist[i].selectline3_name);
				SetDlgItemText(hwndDlg,IDC_RADIO_DS1,platformlist[i].selectline2_name);
				SetDlgItemText(hwndDlg,IDC_RADIO_DS2,platformlist[i].selectline1_name);
				SetDlgItemText(hwndDlg,IDC_RADIO_DS3,platformlist[i].selectline0_name);
			}

			j=SendDlgItemMessage(hwndDlg, IDC_COMBO_USBIFMODE, CB_GETCURSEL, 0, 0);
			SendDlgItemMessage(hwndDlg,IDC_COMBO_USBIFMODE,  CB_GETLBTEXT, (WORD)j, (LONG)tempstr);

			i=0;
			do
			{
				if(!strcmp(tempstr,platformlist[i].name))
				{		
					hwif->interface_mode=platformlist[i].id;
				}
				i++;
			}while(platformlist[i].id!=-1);
				
		}

		*/
		if(hwif->double_step)
		{
			window->chk_doublestep->set();
		}
		else
		{
			window->chk_doublestep->clear();
		}

	}
	else
	{
		window->strout_usbhfestatus->value((const char*)"Hardware-software interface not initialized");
	}

	Fl::repeat_timeout(0.50, tick_usb, v);
}