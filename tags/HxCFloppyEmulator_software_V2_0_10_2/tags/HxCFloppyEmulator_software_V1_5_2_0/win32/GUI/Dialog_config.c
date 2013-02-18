/*
//
// Copyright (C) 2006, 2007, 2008, 2009 Jean-François DEL NERO
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
// File : Dialog_config.c
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

#include "Dialog_config.h"

#include "hxc_floppy_emulator.h"
#include "internal_floppy.h"
#include "floppy_loader.h"
#include "./usb_floppyemulator/usb_hxcfloppyemulator.h"

#include "plateforms.h"

#include "fileselector.h"

#include "soft_cfg_file.h"


extern HWINTERFACE * hwif;
extern guicontext * demo;

extern platform platformlist[];


unsigned char filebuffer[8*1024];

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

typedef struct sdhxcfecfgfile_
{
	char signature[16]; //"HXCFECFGV1.0"
	unsigned char step_sound;     //0x00 -> off 0xFF->on
	unsigned char ihm_sound;     //0x00 -> off 0xFF->on
	unsigned char back_light_tmr; //0x00 always off, 0xFF always on, other -> on x second
	unsigned char standby_tmr;    //0xFF disable, other -> on x second
	unsigned char disable_drive_select;
	unsigned char buzzer_duty_cycle; // 0x00 <> 0x80
	unsigned char number_of_slot;     
	unsigned char slot_index;
	unsigned short update_cnt;
	unsigned char load_last_floppy;
	unsigned char buzzer_step_duration;  // 0xD8 <> 0xFF
    unsigned char lcd_scroll_speed;
}sdhxcfecfgfile;




////////////////////////////////////////////////////////////////////////// 
//
//  Gestion Boite de dialogue Settings
//  
//////////////////////////////////////////////////////////////////////////
BOOL CALLBACK DialogSettings(
						  HWND  hwndDlg,	// handle of dialog box
						  UINT  message,	// message
						  WPARAM  wParam,	// first message parameter
						  LPARAM  lParam 	// second message parameter
						  )
{
	static char nbinstance=0;
	int wmId, wmEvent;
	char tempstr[512];
	char filename[1024];
	int i,j;
	FILE * cfg_file;
	sdhxcfecfgfile * filecfg;
	cfgfile ifcfg;
	int backlight_tmr,standby_tmr,step_sound,ui_sound,lcd_scroll;
	wmId    = LOWORD(wParam); 
	wmEvent = HIWORD(wParam); 
	
	switch (message) 
	{
		
	case WM_COMMAND:
		
		switch (wmEvent)
		{
		case BN_CLICKED:
			
			
			if(SendDlgItemMessage(hwndDlg,IDC_AUTO,BM_GETCHECK,0,0))
				demo->autoselectmode=1;
			else
				demo->autoselectmode=0;

			if(SendDlgItemMessage(hwndDlg,IDC_DOUBLESTEP,BM_GETCHECK,0,0))
				hwif->double_step=0xFF;
			else
				hwif->double_step=0x00;
			
			if(!SendDlgItemMessage(hwndDlg,IDC_AUTO,BM_GETCHECK,0,0))
			{
				EnableWindow(GetDlgItem(hwndDlg, IDC_COMBO1),TRUE);
			}
			else
			{
				EnableWindow(GetDlgItem(hwndDlg, IDC_COMBO1),FALSE);
			}
			
			i=SendDlgItemMessage(hwndDlg, IDC_COMBO1, CB_GETCURSEL, 0, 0);
			SendDlgItemMessage(hwndDlg,IDC_COMBO1,  CB_GETLBTEXT, (WORD)i, (LONG)tempstr);
			break;
			
			
		}
		switch (wmId)
		{
			
		case IDC_COMBO1:
			if( CBN_SELCHANGE == HIWORD(wParam) )
			{
				i=SendDlgItemMessage(hwndDlg, IDC_COMBO1, CB_GETCURSEL, 0, 0);
				SendDlgItemMessage(hwndDlg,IDC_COMBO1,  CB_GETLBTEXT, (WORD)i, (LONG)tempstr);	
			}
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


	case IDLOAD_HXCFECFG:
			sprintf(filename,"HXCSDFE.CFG");
			if(fileselector(hwndDlg,0,0,filename,"Read SD HxCFE settings file","SD HxCFE settings file\0*.cfg\0\0","*.cfg",0,3))
			{
				cfg_file=fopen(filename,"rb");
				if(cfg_file)
				{
					fread(filebuffer,8*1024,1,cfg_file);
					filecfg=(sdhxcfecfgfile *)filebuffer;

					if(strncmp(filebuffer,"HXCFECFGV1.0",16))
					{
						MessageBox(hwndDlg,"Bad file!","Error!",MB_OK);
						memset(filebuffer,0,8192);

					}
					else
					{
						
						if(filecfg->step_sound)
							SendDlgItemMessage(hwndDlg,IDC_AUDIO_STEP,BM_SETCHECK,BST_CHECKED,0);
						else
							SendDlgItemMessage(hwndDlg,IDC_AUDIO_STEP,BM_SETCHECK,BST_UNCHECKED,0);

						if(filecfg->ihm_sound)
						{	
							SendDlgItemMessage(hwndDlg,IDC_SLIDER_UI_SOUND, TBM_SETPOS, TRUE,filecfg->buzzer_duty_cycle);
						}
						else
						{
							SendDlgItemMessage(hwndDlg,IDC_SLIDER_UI_SOUND, TBM_SETPOS, TRUE,0);
						}

						if(filecfg->disable_drive_select)
							SendDlgItemMessage(hwndDlg,IDC_DISABLE_DISKB_SELECTOR,BM_SETCHECK,BST_CHECKED,0);
						else
							SendDlgItemMessage(hwndDlg,IDC_DISABLE_DISKB_SELECTOR,BM_SETCHECK,BST_UNCHECKED,0);

						if(filecfg->load_last_floppy)
							SendDlgItemMessage(hwndDlg,IDC_LOADLASTFLOPPY,BM_SETCHECK,BST_UNCHECKED,0);
						else
							SendDlgItemMessage(hwndDlg,IDC_LOADLASTFLOPPY,BM_SETCHECK,BST_CHECKED,0);

						if(filecfg->number_of_slot)
							SendDlgItemMessage(hwndDlg,IDC_AUTOBOOTMODE,BM_SETCHECK,BST_CHECKED,0);
						else
							SendDlgItemMessage(hwndDlg,IDC_AUTOBOOTMODE,BM_SETCHECK,BST_UNCHECKED,0);


						backlight_tmr=filecfg->back_light_tmr;
						SendDlgItemMessage(hwndDlg,IDC_SLIDER_LCDBACKLIGHT, TBM_SETPOS, TRUE,(int)backlight_tmr);

						standby_tmr=filecfg->standby_tmr;
						SendDlgItemMessage(hwndDlg,IDC_SLIDER_STANDBY, TBM_SETPOS, TRUE,(int)standby_tmr);


						SendDlgItemMessage(hwndDlg,IDC_SLIDER_STEP_SOUND, TBM_SETPOS, TRUE,255-(int)filecfg->buzzer_step_duration);


						SendDlgItemMessage(hwndDlg,IDC_SLIDER_SCROLLTXTSPEED, TBM_SETPOS, TRUE,255-(int)filecfg->lcd_scroll_speed);

				



					}

					fclose(cfg_file);

				}
			}
            break;

		case IDSAVE_HXCFECFG:
			sprintf(filename,"HXCSDFE.CFG");
			if(fileselector(hwndDlg,1,0,filename,"Write SD HxCFE settings file","SD HxCFE settings file\0*.cfg\0\0","*.cfg",0,3))
			{
				cfg_file=fopen(filename,"wb");
				if(cfg_file)
				{

					filecfg=(sdhxcfecfgfile *)filebuffer;

					memset(filebuffer,0,8192);

					sprintf(filecfg->signature,"HXCFECFGV1.0");

					if((unsigned char)SendDlgItemMessage(hwndDlg,IDC_SLIDER_UI_SOUND, TBM_GETPOS, 0,0))
					{
						filecfg->ihm_sound=0xFF;
						filecfg->buzzer_duty_cycle=(unsigned char)SendDlgItemMessage(hwndDlg,IDC_SLIDER_UI_SOUND, TBM_GETPOS, 0,0);
					}
					else
					{
						filecfg->ihm_sound=0x00;
						filecfg->buzzer_duty_cycle=0x80;
					}

					if(SendDlgItemMessage(hwndDlg,IDC_DISABLE_DISKB_SELECTOR,BM_GETCHECK,BST_CHECKED,0))
						filecfg->disable_drive_select=0xFF;
					else
						filecfg->disable_drive_select=0x00;
					
					if(SendDlgItemMessage(hwndDlg,IDC_LOADLASTFLOPPY,BM_GETCHECK,BST_CHECKED,0))
						filecfg->load_last_floppy=0x00;
					else
						filecfg->load_last_floppy=0xFF;

					if(SendDlgItemMessage(hwndDlg,IDC_AUTOBOOTMODE,BM_GETCHECK,BST_CHECKED,0))
						filecfg->number_of_slot=0x01;
					else
						filecfg->number_of_slot=0x00;

					filecfg->back_light_tmr=(unsigned char)SendDlgItemMessage(hwndDlg,IDC_SLIDER_LCDBACKLIGHT, TBM_GETPOS, 0,0);
					filecfg->standby_tmr=(unsigned char)SendDlgItemMessage(hwndDlg,IDC_SLIDER_STANDBY, TBM_GETPOS, 0,0);
					
					filecfg->buzzer_step_duration=0xFF-(unsigned char)SendDlgItemMessage(hwndDlg,IDC_SLIDER_STEP_SOUND, TBM_GETPOS, 0,0);
					if(filecfg->buzzer_step_duration==0xFF)
						filecfg->step_sound=0x00;
					else
						filecfg->step_sound=0xFF;

					filecfg->lcd_scroll_speed=64+((255-64)) - (unsigned char)SendDlgItemMessage(hwndDlg,IDC_SLIDER_SCROLLTXTSPEED, TBM_GETPOS, 0,0);
					
					fwrite(filebuffer,8*1024,1,cfg_file);
					fclose(cfg_file);

				}
			}
            break;
			
		case IDOK:
			KillTimer(hwndDlg,34);
			nbinstance=0;
			save_cfg();
			DestroyWindow(hwndDlg);
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

				backlight_tmr=20;
				standby_tmr=20;
				lcd_scroll=150;
				step_sound=0xD8;
				ui_sound=0x60;

				SendDlgItemMessage(hwndDlg,IDC_AUDIO_STEP,BM_SETCHECK,BST_CHECKED,0);
				SendDlgItemMessage(hwndDlg,IDC_LOADLASTFLOPPY,BM_SETCHECK,BST_CHECKED,0);
				
				sprintf(tempstr,"%ds",backlight_tmr);
				SetDlgItemText(hwndDlg,IDC_EDIT1,tempstr);
				sprintf(tempstr,"%ds",standby_tmr);
				SetDlgItemText(hwndDlg,IDC_EDIT2,tempstr);
				
				SendDlgItemMessage(hwndDlg,IDC_SLIDER_LCDBACKLIGHT, TBM_SETRANGE, TRUE, MAKELONG(0, 255));
				SendDlgItemMessage(hwndDlg,IDC_SLIDER_LCDBACKLIGHT, TBM_SETPOS, TRUE,(int)backlight_tmr);

				SendDlgItemMessage(hwndDlg,IDC_SLIDER_STANDBY, TBM_SETRANGE, TRUE, MAKELONG(0, 255));
				SendDlgItemMessage(hwndDlg,IDC_SLIDER_STANDBY, TBM_SETPOS, TRUE,(int)standby_tmr);

				SendDlgItemMessage(hwndDlg,IDC_SLIDER_STEP_SOUND, TBM_SETRANGE, TRUE, MAKELONG(0x00, 0xFF-0xD8));
				SendDlgItemMessage(hwndDlg,IDC_SLIDER_STEP_SOUND, TBM_SETPOS, TRUE,(int)(0xFF-step_sound));

				SendDlgItemMessage(hwndDlg,IDC_SLIDER_UI_SOUND, TBM_SETRANGE, TRUE, MAKELONG(0x00, 128));
				SendDlgItemMessage(hwndDlg,IDC_SLIDER_UI_SOUND, TBM_SETPOS, TRUE,(int)(ui_sound));


				SendDlgItemMessage(hwndDlg,IDC_SLIDER_SCROLLTXTSPEED, TBM_SETRANGE, TRUE, MAKELONG(0, (255-64)));
				SendDlgItemMessage(hwndDlg,IDC_SLIDER_SCROLLTXTSPEED, TBM_SETPOS, TRUE,(int)0xFF-lcd_scroll);

//				SetTimer(hwndDlg,36,100,NULL);

								
				if(demo->autoselectmode)
				{
					SendDlgItemMessage(hwndDlg,IDC_AUTO,BM_SETCHECK,BST_CHECKED,0);
					EnableWindow(GetDlgItem(hwndDlg, IDC_COMBO1),FALSE);
				}

				if(hwif->double_step)
				{
					SendDlgItemMessage(hwndDlg,IDC_DOUBLESTEP,BM_SETCHECK,BST_CHECKED,0);
				}
				
				i=0;
				do
				{
					SendDlgItemMessage(hwndDlg,IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)platformlist[i].name);
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
					SendDlgItemMessage(hwndDlg,IDC_COMBO1,  CB_GETLBTEXT, (WORD)i, (LONG)tempstr);
					
					if(!strcmp(tempstr,platformlist[j].name))
					{
						SendDlgItemMessage(hwndDlg, IDC_COMBO1, CB_SETCURSEL, i, 0);
					
					}
					i++;
				}while(platformlist[i].id!=-1);
				nbinstance=1;
				
				
			}
			SetTimer(hwndDlg,34,500,NULL);
			break;
			
		case WM_CLOSE:
			KillTimer(hwndDlg,34);
			nbinstance=0;
			DestroyWindow(hwndDlg);
			break;
			
			
		case WM_TIMER:
			if(demo->autoselectmode)
			{
				SendDlgItemMessage(hwndDlg,IDC_AUTO,BM_SETCHECK,BST_CHECKED,0);
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
				
				EnableWindow(GetDlgItem(hwndDlg, IDC_COMBO1),FALSE);
				
				i=0;
				do
				{
					SendDlgItemMessage(hwndDlg,IDC_COMBO1,  CB_GETLBTEXT, (WORD)i, (LONG)tempstr);
					
					if(!strcmp(tempstr,platformlist[j].name))
					{
						SendDlgItemMessage(hwndDlg, IDC_COMBO1, CB_SETCURSEL, i, 0);
					}
					i++;
				}while(platformlist[i].id!=-1);
	
			}
			else
			{
				EnableWindow(GetDlgItem(hwndDlg, IDC_COMBO1),TRUE);
				i=0;
				do
				{
					i++;
				}while(platformlist[i].id!=-1 && (platformlist[i].id!=hwif->interface_mode));
				
				j=SendDlgItemMessage(hwndDlg, IDC_COMBO1, CB_GETCURSEL, 0, 0);
				SendDlgItemMessage(hwndDlg,IDC_COMBO1,  CB_GETLBTEXT, (WORD)j, (LONG)tempstr);

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
					SendDlgItemMessage(hwndDlg,IDC_DOUBLESTEP,BM_SETCHECK,BST_CHECKED,0);
				}
				else
				{
					SendDlgItemMessage(hwndDlg,IDC_DOUBLESTEP,BM_SETCHECK,BST_UNCHECKED,0);
				}


			backlight_tmr=SendDlgItemMessage(hwndDlg,IDC_SLIDER_LCDBACKLIGHT, TBM_GETPOS, 0,0);
			standby_tmr=SendDlgItemMessage(hwndDlg,IDC_SLIDER_STANDBY, TBM_GETPOS, 0,0);

			if(backlight_tmr==0xFF)
			{
				sprintf(tempstr,"On");
			}
			else
			{
				if(!backlight_tmr)
					sprintf(tempstr,"Off");
				else
					sprintf(tempstr,"%ds",backlight_tmr);

			}
			SetDlgItemText(hwndDlg,IDC_EDIT1,tempstr);

			

			if(standby_tmr==0xFF)
			{
				sprintf(tempstr,"On");
			}
			else
			{
				if(!standby_tmr)
					sprintf(tempstr,"Off");
				else
					sprintf(tempstr,"%ds",standby_tmr);

			}
			SetDlgItemText(hwndDlg,IDC_EDIT2,tempstr);
			


			break;
			
		default:
			return FALSE;
			
	}
	
	return TRUE;
}
