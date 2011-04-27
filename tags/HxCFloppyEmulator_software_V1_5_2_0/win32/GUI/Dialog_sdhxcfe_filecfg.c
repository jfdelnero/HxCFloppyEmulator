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

#include "Dialog_sdhxcfe_filecfg.h"

#include "hxc_floppy_emulator.h"
#include "internal_floppy.h"
#include "floppy_loader.h"

#include "fileselector.h"

unsigned char filebuffer[8*1024];

////////////////////////////////////////////////////////////////////////// 
//
//  Gestion Boite de dialogue Settings
//  
//////////////////////////////////////////////////////////////////////////
BOOL CALLBACK DialogSDHxCFESettings(
						  HWND  hwndDlg,	// handle of dialog box
						  UINT  message,	// message
						  WPARAM  wParam,	// first message parameter
						  LPARAM  lParam 	// second message parameter
						  )
{
	static char nbinstance=0;
	int wmId, wmEvent;
	char tempstr[512];
	char filename[512];
	int backlight_tmr,standby_tmr,step_sound,lcd_scroll;
	FILE * cfg_file;
	sdhxcfecfgfile * filecfg;
	wmId    = LOWORD(wParam); 
	wmEvent = HIWORD(wParam); 
	
	switch (message) 
	{
		
	case WM_COMMAND:
		
		switch (wmEvent)
		{
		
		case BN_CLICKED:		
            break;
		}
		
		switch (wmId)
		{
		
		case IDLOAD:
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
							SendDlgItemMessage(hwndDlg,IDC_AUDIO_UI,BM_SETCHECK,BST_CHECKED,0);
						else
							SendDlgItemMessage(hwndDlg,IDC_AUDIO_UI,BM_SETCHECK,BST_UNCHECKED,0);

						if(filecfg->disable_drive_select)
							SendDlgItemMessage(hwndDlg,IDC_DISABLE_DISKB_SELECTOR,BM_SETCHECK,BST_CHECKED,0);
						else
							SendDlgItemMessage(hwndDlg,IDC_DISABLE_DISKB_SELECTOR,BM_SETCHECK,BST_UNCHECKED,0);

						if(filecfg->load_last_floppy)
							SendDlgItemMessage(hwndDlg,IDC_LOADLASTFLOPPY,BM_SETCHECK,BST_UNCHECKED,0);
						else
							SendDlgItemMessage(hwndDlg,IDC_LOADLASTFLOPPY,BM_SETCHECK,BST_CHECKED,0);


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

		case IDSAVE:
			sprintf(filename,"HXCSDFE.CFG");
			if(fileselector(hwndDlg,1,0,filename,"Write SD HxCFE settings file","SD HxCFE settings file\0*.cfg\0\0","*.cfg",0,3))
			{
				cfg_file=fopen(filename,"wb");
				if(cfg_file)
				{

					filecfg=(sdhxcfecfgfile *)filebuffer;

					memset(filebuffer,0,8192);

					sprintf(filecfg->signature,"HXCFECFGV1.0");

					if(SendDlgItemMessage(hwndDlg,IDC_AUDIO_UI,BM_GETCHECK,BST_CHECKED,0))
						filecfg->ihm_sound=0xFF;
					else
						filecfg->ihm_sound=0x00;

					if(SendDlgItemMessage(hwndDlg,IDC_DISABLE_DISKB_SELECTOR,BM_GETCHECK,BST_CHECKED,0))
						filecfg->disable_drive_select=0xFF;
					else
						filecfg->disable_drive_select=0x00;
					
					if(SendDlgItemMessage(hwndDlg,IDC_LOADLASTFLOPPY,BM_GETCHECK,BST_CHECKED,0))
						filecfg->load_last_floppy=0x00;
					else
						filecfg->load_last_floppy=0xFF;


					filecfg->buzzer_duty_cycle=0x60;

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
			nbinstance=0;
			DestroyWindow(hwndDlg);
			break;
			
		default:;
			break;
		}
		
		
		break;
		
		
		case WM_INITDIALOG:
			backlight_tmr=20;
			standby_tmr=20;
			lcd_scroll=150;
			step_sound=0xD8;

			SendDlgItemMessage(hwndDlg,IDC_AUDIO_UI,BM_SETCHECK,BST_CHECKED,0);
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

			SendDlgItemMessage(hwndDlg,IDC_SLIDER_SCROLLTXTSPEED, TBM_SETRANGE, TRUE, MAKELONG(0, (255-64)));
			SendDlgItemMessage(hwndDlg,IDC_SLIDER_SCROLLTXTSPEED, TBM_SETPOS, TRUE,(int)0xFF-lcd_scroll);

			SetTimer(hwndDlg,36,100,NULL);


			break;
			
		case WM_CLOSE:
			nbinstance=0;
			DestroyWindow(hwndDlg);
			break;
			
			
		case WM_TIMER:
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
