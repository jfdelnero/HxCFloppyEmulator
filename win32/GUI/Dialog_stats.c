/*
//
// Copyright (C) 2006, 2007, 2008, 2009, 2010, 2011 Jean-François DEL NERO
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
// File : Dialog_stats.c
// Contains: Floppy Emulator Project
//
// Written by:	DEL NERO Jean Francois
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <commctrl.h>

#include <stdio.h>
#include <time.h>

#include "resource.h"
#include "mainrouts.h"

#include "Dialog_stats.h"

#include "hxc_floppy_emulator.h"
#include "internal_floppy.h"
#include "floppy_loader.h"
#include "./usb_floppyemulator/usb_hxcfloppyemulator.h"

#include "soft_cfg_file.h"
#include "fileselector.h"
#include "plateforms.h"

extern HWINTERFACE * hwif;
extern guicontext * demo;



platform platformlist[]=
{
	{ AMIGA_DD_FLOPPYMODE,"Amiga","DS0","DS1","DS2","MTRON"},
	{ AMIGA_HD_FLOPPYMODE,"Amiga HD","DS0","DS1","DS2","MTRON"},
	{ ATARIST_DD_FLOPPYMODE,"Atari ST","D0SEL","D1SEL","-","MTRON"},
	{ ATARIST_HD_FLOPPYMODE,"Atari ST HD","D0SEL","D1SEL","-","MTRON"},
	{ IBMPC_DD_FLOPPYMODE,"IBM PC 720kB","MOTEA","DRVSB","DRVSA","MOTEB"},
	{ IBMPC_HD_FLOPPYMODE,"IBM PC 1.44MB","MOTEA","DRVSB","DRVSA","MOTEB"},
	{ IBMPC_ED_FLOPPYMODE,"IBM PC 2.88MB","MOTEA","DRVSB","DRVSA","MOTEB"},
	{ S950_DD_FLOPPYMODE,"AKAI S900/S950 DD","DS0","DS1","DS2","MTRON"},
	{ S950_HD_FLOPPYMODE,"AKAI S950 HD","DS0","DS1","DS2","MTRON"},
	{ CPC_DD_FLOPPYMODE,"Amstrad CPC","Drive Select 0","Drive Select 1","-","MOTOR ON"},
	{ MSX2_DD_FLOPPYMODE,"MSX 2","DS0","DS1","DS2","MTRON"},
	{ GENERIC_SHUGART_DD_FLOPPYMODE,"Generic Shugart","DS0","DS1","DS2","MTRON"},
	{ EMU_SHUGART_FLOPPYMODE,"Emu Shugart","DS0","DS1","DS2","MTRON"},
	{ C64_DD_FLOPPYMODE,"C64 1541","NA","NA","NA","NA"},
	{ -1,"?","DS0","DS1","DS2","MTRON"}
};

	typedef struct cfgfile_
	{
		unsigned char version;
		unsigned int  usbpacketsize;
		unsigned char driveid;
		unsigned char twistedcable;
		unsigned char disabledrive;
		unsigned char automode;
		unsigned char idmode;
		unsigned char double_step;
	}cfgfile;



////////////////////////////////////////////////////////////////////////// 
//
//  Gestion Boite de dialogue USB Stats
//  
//////////////////////////////////////////////////////////////////////////
BOOL CALLBACK DialogStats(
						  HWND  hwndDlg,	// handle of dialog box
						  UINT  message,	// message
						  WPARAM  wParam,	// first message parameter
						  LPARAM  lParam 	// second message parameter
						  )
{
	static char nbinstance=0;
	char filename[1024];
	FILE * cfg_file;
	char tempstr[512];
	cfgfile ifcfg;
	unsigned long packetsize;
	unsigned long datathroughput;
	unsigned long period;
	float packetpersecond;
	int priority,i,j;
	int wmId, wmEvent;
	
	wmId    = LOWORD(wParam);
	wmEvent = HIWORD(wParam); 

	switch (message) 
	{
		
	case WM_COMMAND:



		switch (wmEvent)
		{
		case BN_CLICKED:
			
			if(SendDlgItemMessage(hwndDlg,IDC_RADIO_DS0,BM_GETCHECK,0,0))
				hwif->drive_select_source=0;
			if(SendDlgItemMessage(hwndDlg,IDC_RADIO_DS1,BM_GETCHECK,0,0))
				hwif->drive_select_source=1;
			if(SendDlgItemMessage(hwndDlg,IDC_RADIO_DS2,BM_GETCHECK,0,0))
				hwif->drive_select_source=2;
			if(SendDlgItemMessage(hwndDlg,IDC_RADIO_DS3,BM_GETCHECK,0,0))
				hwif->drive_select_source=3;
			if(SendDlgItemMessage(hwndDlg,IDC_DISABLE,BM_GETCHECK,0,0))
				hwif->drive_select_source=hwif->drive_select_source|0x4;
			
			
			if(SendDlgItemMessage(hwndDlg,IDC_AUTO_USB,BM_GETCHECK,0,0))
				demo->autoselectmode=1;
			else
				demo->autoselectmode=0;


			if(SendDlgItemMessage(hwndDlg,IDC_DOUBLESTEP_USB,BM_GETCHECK,0,0))
				hwif->double_step=0xFF;
			else
				hwif->double_step=0x00;

			
			if(SendDlgItemMessage(hwndDlg,IDC_TWISTED,BM_GETCHECK,0,0))
				demo->twistedcable=1;
			else
				demo->twistedcable=0;
			
			
			
			if(!SendDlgItemMessage(hwndDlg,IDC_AUTO_USB,BM_GETCHECK,0,0))
			{
				EnableWindow(GetDlgItem(hwndDlg, IDC_COMBO_USBIFMODE),TRUE);
			}
			else
			{
				EnableWindow(GetDlgItem(hwndDlg, IDC_COMBO_USBIFMODE),FALSE);
			}
			
			i=SendDlgItemMessage(hwndDlg, IDC_COMBO_USBIFMODE, CB_GETCURSEL, 0, 0);
			SendDlgItemMessage(hwndDlg,IDC_COMBO_USBIFMODE,  CB_GETLBTEXT, (WORD)i, (LONG)tempstr);
			i=0;
			do
			{
				if(!strcmp(tempstr,platformlist[i].name))
				{
					if(!SendDlgItemMessage(hwndDlg,IDC_TWISTED,BM_GETCHECK,0,0))
					{
						SetDlgItemText(hwndDlg,IDC_RADIO_DS0,platformlist[i].selectline0_name);
						SetDlgItemText(hwndDlg,IDC_RADIO_DS1,platformlist[i].selectline1_name);
						SetDlgItemText(hwndDlg,IDC_RADIO_DS2,platformlist[i].selectline2_name);
						SetDlgItemText(hwndDlg,IDC_RADIO_DS3,platformlist[i].selectline3_name);
					}
					else
					{
						SetDlgItemText(hwndDlg,IDC_RADIO_DS0,platformlist[i].selectline3_name);
						SetDlgItemText(hwndDlg,IDC_RADIO_DS1,platformlist[i].selectline2_name);
						SetDlgItemText(hwndDlg,IDC_RADIO_DS2,platformlist[i].selectline1_name);
						SetDlgItemText(hwndDlg,IDC_RADIO_DS3,platformlist[i].selectline0_name);
					}
				}
				i++;
			}while(platformlist[i].id!=-1);
			break;
			
			
		}

		switch (wmId)
		{
		case IDC_COMBO_USBIFMODE:
			if( CBN_SELCHANGE == HIWORD(wParam) )
			{
				i=SendDlgItemMessage(hwndDlg, IDC_COMBO_USBIFMODE, CB_GETCURSEL, 0, 0);
				SendDlgItemMessage(hwndDlg,IDC_COMBO_USBIFMODE,  CB_GETLBTEXT, (WORD)i, (LONG)tempstr);
				i=0;
				do
				{
					if(!strcmp(tempstr,platformlist[i].name))
					{
						if(!SendDlgItemMessage(hwndDlg,IDC_TWISTED,BM_GETCHECK,0,0))
						{
							SetDlgItemText(hwndDlg,IDC_RADIO_DS0,platformlist[i].selectline0_name);
							SetDlgItemText(hwndDlg,IDC_RADIO_DS1,platformlist[i].selectline1_name);
							SetDlgItemText(hwndDlg,IDC_RADIO_DS2,platformlist[i].selectline2_name);
							SetDlgItemText(hwndDlg,IDC_RADIO_DS3,platformlist[i].selectline3_name);
						}
						else
						{
							SetDlgItemText(hwndDlg,IDC_RADIO_DS0,platformlist[i].selectline3_name);
							SetDlgItemText(hwndDlg,IDC_RADIO_DS1,platformlist[i].selectline2_name);
							SetDlgItemText(hwndDlg,IDC_RADIO_DS2,platformlist[i].selectline1_name);
							SetDlgItemText(hwndDlg,IDC_RADIO_DS3,platformlist[i].selectline0_name);
						}
						
						hwif->interface_mode=platformlist[i].id;
						
					}
					i++;
				}while(platformlist[i].id!=-1);
				
			}
            break;


		case IDOK:
			KillTimer(hwndDlg,34);
			nbinstance=0;
			save_cfg();
			DestroyWindow(hwndDlg);
			break;

		case IDLOAD:
			sprintf(filename,"hxcfloppyemulator.cfg");
			if(fileselector(hwndDlg,0,0,filename,"Read drive settings file","Drive settings file\0*.cfg\0\0","*.cfg",0,0))
			{
				cfg_file=fopen(filename,"rb");
				if(cfg_file)
				{
					fread(&ifcfg,sizeof(cfgfile),1,cfg_file);

			
					hwif->drive_select_source=ifcfg.driveid&3;
					if(ifcfg.disabledrive&1)
					{
						hwif->drive_select_source=hwif->drive_select_source|0x4;
					}
					hwif->interface_mode=ifcfg.idmode;
					demo->twistedcable=ifcfg.twistedcable&1;
					hwif->usbstats.packetsize=ifcfg.usbpacketsize;
					demo->autoselectmode=ifcfg.automode&1;
					hwif->double_step=ifcfg.double_step;

					fclose(cfg_file);

				}
			}
            break;
		case IDSAVE:
			sprintf(filename,"hxcfloppyemulator.cfg");
			if(fileselector(hwndDlg,1,0,filename,"Write drive settings file","Drive settings file\0*.cfg\0\0","*.cfg",0,0))
			{
				cfg_file=fopen(filename,"wb");
				if(cfg_file)
				{
					ifcfg.version=0;
					ifcfg.disabledrive=(hwif->drive_select_source)>>2;
					ifcfg.driveid=hwif->drive_select_source&3;
					ifcfg.idmode=hwif->interface_mode;
					ifcfg.twistedcable=demo->twistedcable;
					ifcfg.usbpacketsize=hwif->usbstats.packetsize;
					ifcfg.automode=demo->autoselectmode;
					ifcfg.double_step=hwif->double_step;
					fwrite(&ifcfg,sizeof(cfgfile),1,cfg_file);
					fclose(cfg_file);

				}
			}
            break;


		case IDC_BUTTON_RESET:
			hwif->usbstats.dataout=0;
			hwif->usbstats.packetsent=0;
			hwif->usbstats.synclost=0;
			hwif->usbstats.totaldataout=0;
			hwif->usbstats.totalpacketsent=0;
			break;
			
		case IDC_BUTTON1:
			break;
			
		default:;
		}
		break;
		
		
		case WM_INITDIALOG:
			if(nbinstance!=0)
			{
				DestroyWindow(hwndDlg);
			}
			else
			{
				nbinstance=1;
				SendDlgItemMessage(hwndDlg,IDC_SLIDER1, TBM_SETRANGE, TRUE, MAKELONG((512/128), (4*1024)/128));
				SendDlgItemMessage(hwndDlg,IDC_SLIDER1, TBM_SETPOS, (WPARAM) TRUE, (LPARAM) hwif->usbstats.packetsize/128);
				
				SendDlgItemMessage(hwndDlg,IDC_PRIORITYSLIDER, TBM_SETRANGE, TRUE, MAKELONG(0, 5));
								
				priority=0;
				switch(GetPriorityClass(GetCurrentProcess()))
				{
					
				case IDLE_PRIORITY_CLASS:
					priority=0;
					break;
					
				case BELOW_NORMAL_PRIORITY_CLASS:
					priority=1;
					break;
					
				case NORMAL_PRIORITY_CLASS:
					priority=2;
					break;
					
				case ABOVE_NORMAL_PRIORITY_CLASS:
					priority=3;
					break;
				case HIGH_PRIORITY_CLASS:
					priority=4;
					break;
				case REALTIME_PRIORITY_CLASS:
					priority=5;
					break;
				}
				
				
				SendDlgItemMessage(hwndDlg,IDC_PRIORITYSLIDER, TBM_SETPOS, (WPARAM) TRUE, (LPARAM)priority);
				
				
				hwif->usbstats.dataout=0;
				hwif->usbstats.packetsent=0;

				switch(hwif->drive_select_source&0x3)
				{
					case 0:
						SendDlgItemMessage(hwndDlg,IDC_RADIO_DS0,BM_SETCHECK,BST_CHECKED,0);
						break;
					case 1:
						SendDlgItemMessage(hwndDlg,IDC_RADIO_DS1,BM_SETCHECK,BST_CHECKED,0);
						break;
					case 2:
						SendDlgItemMessage(hwndDlg,IDC_RADIO_DS2,BM_SETCHECK,BST_CHECKED,0);
						break;
					case 3:
						SendDlgItemMessage(hwndDlg,IDC_RADIO_DS3,BM_SETCHECK,BST_CHECKED,0);
						break;
				}
				
				if(hwif->drive_select_source>3)
				{
					SendDlgItemMessage(hwndDlg,IDC_DISABLE,BM_SETCHECK,BST_CHECKED,0);
				}



				if(demo->autoselectmode)
				{
					SendDlgItemMessage(hwndDlg,IDC_AUTO_USB,BM_SETCHECK,BST_CHECKED,0);
					EnableWindow(GetDlgItem(hwndDlg, IDC_COMBO_USBIFMODE),FALSE);
				}

				if(hwif->double_step)
				{
					SendDlgItemMessage(hwndDlg,IDC_DOUBLESTEP_USB,BM_SETCHECK,BST_CHECKED,0);
				}
				
				i=0;
				do
				{
					SendDlgItemMessage(hwndDlg,IDC_COMBO_USBIFMODE, CB_ADDSTRING, 0, (LPARAM)platformlist[i].name);
					i++;
				}while(platformlist[i].id!=-1);



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
				
				i=0;
				do
				{
					SendDlgItemMessage(hwndDlg,IDC_COMBO_USBIFMODE,  CB_GETLBTEXT, (WORD)i, (LONG)tempstr);
					
					if(!strcmp(tempstr,platformlist[j].name))
					{
						SendDlgItemMessage(hwndDlg, IDC_COMBO_USBIFMODE, CB_SETCURSEL, i, 0);
						if(!SendDlgItemMessage(hwndDlg,IDC_TWISTED,BM_GETCHECK,0,0))
						{
							SetDlgItemText(hwndDlg,IDC_RADIO_DS0,platformlist[j].selectline0_name);
							SetDlgItemText(hwndDlg,IDC_RADIO_DS1,platformlist[j].selectline1_name);
							SetDlgItemText(hwndDlg,IDC_RADIO_DS2,platformlist[j].selectline2_name);
							SetDlgItemText(hwndDlg,IDC_RADIO_DS3,platformlist[j].selectline3_name);
						}
						else
						{
							SetDlgItemText(hwndDlg,IDC_RADIO_DS0,platformlist[j].selectline3_name);
							SetDlgItemText(hwndDlg,IDC_RADIO_DS1,platformlist[j].selectline2_name);
							SetDlgItemText(hwndDlg,IDC_RADIO_DS2,platformlist[j].selectline1_name);
							SetDlgItemText(hwndDlg,IDC_RADIO_DS3,platformlist[j].selectline0_name);
						}
					}
					i++;
				}while(platformlist[i].id!=-1);



			}
			SetTimer(hwndDlg,34,1000,NULL);
			break;
			
		case  WM_NOTIFY:
			switch (((LPNMHDR)lParam)->code)
			{
				
			case  NM_RELEASEDCAPTURE:
				break;
				
			case NM_CUSTOMDRAW:
				
				packetsize=SendDlgItemMessage(hwndDlg,IDC_SLIDER1,TBM_GETPOS, 0, 0)*128;
				SetDlgItemInt(hwndDlg,IDC_PACKETSIZE,packetsize,0);
				hwif->usbstats.packetsize=packetsize;
				
				priority=0;
				switch(SendDlgItemMessage(hwndDlg,IDC_PRIORITYSLIDER,TBM_GETPOS, 0, 0))
				{
					
				case 0:
					SetPriorityClass(GetCurrentProcess(),IDLE_PRIORITY_CLASS);
					break;
					
				case 1: 
					SetPriorityClass(GetCurrentProcess(),BELOW_NORMAL_PRIORITY_CLASS);
					break;
					
				case 2:
					SetPriorityClass(GetCurrentProcess(),NORMAL_PRIORITY_CLASS);
					break;
					
				case 3:
					SetPriorityClass(GetCurrentProcess(),ABOVE_NORMAL_PRIORITY_CLASS);
					break;
				case 4:
					SetPriorityClass(GetCurrentProcess(),HIGH_PRIORITY_CLASS);
					break;
				case 5:
					SetPriorityClass(GetCurrentProcess(),REALTIME_PRIORITY_CLASS);
					break;
				}
				break;
			}
			
			break;
			case WM_CLOSE:
				KillTimer(hwndDlg,34);
				nbinstance=0;
				DestroyWindow(hwndDlg);
				break;
				
				
			case WM_TIMER:
				
				sprintf(tempstr,"%d (%d p/s)",hwif->usbstats.totalpacketsent,hwif->usbstats.packetsent);
				hwif->usbstats.packetsent=0;
				SetDlgItemText(hwndDlg,IDC_NBOFPACKET,tempstr);
				
				
				SetDlgItemInt(hwndDlg,IDC_NBOFSYNCLOST,hwif->usbstats.synclost,0);
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
				SetDlgItemText(hwndDlg,IDC_DATASENT,tempstr);
				
				
				datathroughput=hwif->usbstats.dataout;
				sprintf(tempstr,"%d bytes/second",hwif->usbstats.dataout);
				SetDlgItemText(hwndDlg,IDC_DATATHROUGHPUT,tempstr);
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
				SetDlgItemText(hwndDlg,IDC_MAXSETTLE,tempstr);
				
				sprintf(tempstr,"%d ms",period);
				SetDlgItemText(hwndDlg,IDC_MINSETTLE,tempstr);


				if(hwif)
				{
					switch(hwif->status)
					{
						case STATUS_ERROR:
							SetDlgItemText(hwndDlg,IDC_USBHXCFE_STATUS,"FTDI D2XX Driver not installed!");
							break;

						case STATUS_LOOKINGFOR:
							SetDlgItemText(hwndDlg,IDC_USBHXCFE_STATUS,"USB HxC Floppy Emulator not detected!");
							break;

						case STATUS_ONLINE:
							SetDlgItemText(hwndDlg,IDC_USBHXCFE_STATUS,"USB HxC Floppy Emulator ready!");
							break;

						default:
							SetDlgItemText(hwndDlg,IDC_USBHXCFE_STATUS,"Unknow status !");
							break;
					
					}


					switch(hwif->drive_select_source&0x3)
					{
						case 0:
							SendDlgItemMessage(hwndDlg,IDC_RADIO_DS0,BM_SETCHECK,BST_CHECKED,0);
							SendDlgItemMessage(hwndDlg,IDC_RADIO_DS1,BM_SETCHECK,BST_UNCHECKED,0);
							SendDlgItemMessage(hwndDlg,IDC_RADIO_DS2,BM_SETCHECK,BST_UNCHECKED,0);
							SendDlgItemMessage(hwndDlg,IDC_RADIO_DS3,BM_SETCHECK,BST_UNCHECKED,0);
							break;
						case 1:
							SendDlgItemMessage(hwndDlg,IDC_RADIO_DS0,BM_SETCHECK,BST_UNCHECKED,0);
							SendDlgItemMessage(hwndDlg,IDC_RADIO_DS1,BM_SETCHECK,BST_CHECKED,0);
							SendDlgItemMessage(hwndDlg,IDC_RADIO_DS2,BM_SETCHECK,BST_UNCHECKED,0);
							SendDlgItemMessage(hwndDlg,IDC_RADIO_DS3,BM_SETCHECK,BST_UNCHECKED,0);
							break;
						case 2:
							SendDlgItemMessage(hwndDlg,IDC_RADIO_DS0,BM_SETCHECK,BST_UNCHECKED,0);
							SendDlgItemMessage(hwndDlg,IDC_RADIO_DS1,BM_SETCHECK,BST_UNCHECKED,0);
							SendDlgItemMessage(hwndDlg,IDC_RADIO_DS2,BM_SETCHECK,BST_CHECKED,0);
							SendDlgItemMessage(hwndDlg,IDC_RADIO_DS3,BM_SETCHECK,BST_UNCHECKED,0);
							break;
						case 3:
							SendDlgItemMessage(hwndDlg,IDC_RADIO_DS0,BM_SETCHECK,BST_UNCHECKED,0);
							SendDlgItemMessage(hwndDlg,IDC_RADIO_DS1,BM_SETCHECK,BST_UNCHECKED,0);
							SendDlgItemMessage(hwndDlg,IDC_RADIO_DS2,BM_SETCHECK,BST_UNCHECKED,0);
							SendDlgItemMessage(hwndDlg,IDC_RADIO_DS3,BM_SETCHECK,BST_CHECKED,0);
							break;
					}
				
					if(hwif->drive_select_source>3)
					{
						SendDlgItemMessage(hwndDlg,IDC_DISABLE,BM_SETCHECK,BST_CHECKED,0);
					}
					else
					{
						SendDlgItemMessage(hwndDlg,IDC_DISABLE,BM_SETCHECK,BST_UNCHECKED,0);
					}



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


				if(hwif->double_step)
				{
					SendDlgItemMessage(hwndDlg,IDC_DOUBLESTEP_USB,BM_SETCHECK,BST_CHECKED,0);
				}
				else
				{
					SendDlgItemMessage(hwndDlg,IDC_DOUBLESTEP_USB,BM_SETCHECK,BST_UNCHECKED,0);
				}

				}
				else
				{
					SetDlgItemText(hwndDlg,IDC_USBHXCFE_STATUS,"Hardware-software interface not initialized");
				}


				break;
				
			default:
				return FALSE;
				
	}
	
	return TRUE;
}

