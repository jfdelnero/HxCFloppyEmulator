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
// File : Dialog_RAWFileSettings.c
// Contains: Floppy Emulator Project
//
// Written by:	DEL NERO Jean Francois
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////
#include <WINDOWSX.H>
#include <windows.h>
#include <commctrl.h>

#include <stdio.h>
#include <time.h>

#include <shlobj.h>

#include "resource.h"
#include "mainrouts.h"

#include "Dialog_config.h"
#include "Dialog_stats.h"
#include "Dialog_CreateFileSystem.h"
#include "Dialog_RAWFileSettings.h"
#include "Dialog_BatchConvert.h"
#include "Dialog_floppydump.h"


#include "hxc_floppy_emulator.h"
#include "internal_floppy.h"
#include "floppy_loader.h"
#include "./usb_floppyemulator/usb_hxcfloppyemulator.h"

#include "fileselector.h"

#include "afi_file_writer.h"
#include "mfm_file_writer.h"
#include "hfe_file_writer.h"
#include "raw_file_writer.h"
#include "vtrucco_file_writer.h"
#include "cpcdsk_file_writer.h"
#include "imd_file_writer.h"


#include "../../common/plugins/raw_loader/raw_loader.h"
#include "loader.h"


extern HWINTERFACE * hwif;
extern guicontext * demo;
extern HXCFLOPPYEMULATOR * flopemu;
extern FLOPPY * thefloppydisk;

HBRUSH Dlg_OnCtlColor(HWND hwnd, HDC hdc, HWND hwndChild, int type)
{
   SetBkMode(hdc, TRANSPARENT);
   return (HBRUSH)GetStockObject(HOLLOW_BRUSH);
}


	void changefond(HWND hwndDlg,int icd_code)
	{
		HANDLE hFont;
		HWND  hwnd;
		LOGFONT  logFont;

		hwnd =GetDlgItem(hwndDlg, icd_code);
		hFont = (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0);

		GetObject(hFont, sizeof logFont, &logFont);
		logFont.lfWeight = FW_BOLD;
		hFont = CreateFontIndirect(&logFont);

		SendMessage(hwnd, WM_SETFONT, (WPARAM)hFont, 0);
	}

////////////////////////////////////////////////////////////////////////// 
//
//  Gestion Boite de dialogue Settings
//  
//////////////////////////////////////////////////////////////////////////
BOOL CALLBACK DialogMainDialog(
						  HWND  hwndDlg,	// handle of dialog box
						  UINT  message,	// message
						  WPARAM  wParam,	// first message parameter
						  LPARAM  lParam 	// second message parameter
						  )
{
	char filename[1024];
	char tempstr[1024];
	char tempstr2[1024];
	int extpos;
	unsigned int j,i;
	static unsigned int txtindex;
	int wmId, wmEvent;

	wmId    = LOWORD(wParam); 
	wmEvent = HIWORD(wParam); 

	switch (message) 
	{
		

	case WM_DROPFILES:
		DragQueryFile((HDROP)wParam, 0, (char*)filename, 1024);
		sprintf(demo->buffertext,"    Loading floppy   ");
		sprintf(demo->bufferfilename,"");
		loadfloppy(filename,0,0);
		break;	

	case WM_CTLCOLORSTATIC:
      // //  NOTE: HANDLE_WM_CTLCOLORSTATIC is defined in <WINDOWSX.H>
		if(
			GetDlgItem(hwndDlg,IDC_EDIT_STATUS)==(HWND)lParam ||
			GetDlgItem(hwndDlg,IDC_SLIDER_TRACKPOS)==(HWND)lParam ||
			GetDlgItem(hwndDlg,IDC_EDIT_TRACKPOS)==(HWND)lParam			
			)
		{
		return FALSE;
		}
      return HANDLE_WM_CTLCOLORSTATIC(hwndDlg, wParam, lParam, Dlg_OnCtlColor);
      break;





	case WM_COMMAND:
		switch (wmId)
		{
			
		case IDC_BUTTON_LOAD:
			
			sprintf(filename,"*.*");
			i=0;
			if(fileselector(hwndDlg,0,0,filename,"Load image file","All disk image types\0*.*\0\0","*.*",0,1))
			{
				sprintf(demo->buffertext,"    Loading floppy   ");
				
				CheckMenuItem(GetMenu(hwndDlg),ID_DRIVE_SHUGARTMODE,MF_UNCHECKED|MF_BYCOMMAND);
				CheckMenuItem(GetMenu(hwndDlg),ID_DRIVE_IBMPCMODE,MF_UNCHECKED| MF_BYCOMMAND);
				sprintf(demo->bufferfilename,"");
				loadfloppy(filename,0,0);
			}
			break;
			
		case IDC_BUTTON_LOADRAW:
			CreateDialog(GetModuleHandle(NULL),(const char*)IDD_RAWFILECONFIG,GetActiveWindow(),&DialogRAWFileSettings);
			break;
			
		case IDC_BUTTON_CREATEFLOPPY:
			CreateDialog(GetModuleHandle(NULL),(const char*)IDD_CREATEFILESYSTEM,GetActiveWindow(),&DialogCreateFileSystem);
			break;
			
		case IDC_BUTTON_CONVERT:
			CreateDialog(GetModuleHandle(NULL),(const char*)IDD_BATCHCONVERT,GetActiveWindow(),&DialogBatchConvert);
			break;
			
		case IDC_BUTTON_SETTINGS:
			CreateDialog(GetModuleHandle(NULL),(const char*)IDD_DIALOG_CONFIG,GetActiveWindow(),&DialogSettings);
			break;
			
		case IDC_BUTTON_USBCONFIG:
			CreateDialog(GetModuleHandle(NULL),(const char*)IDD_DIALOG_STATS,GetActiveWindow(),&DialogStats);
			break;

		case IDC_BUTTON_FLOPPYDUMP:
			CreateDialog(GetModuleHandle(NULL),(const char*)IDD_DIALOG_FLOPPYDUMP,GetActiveWindow(),&DialogFloppyDump);
			break;

		case IDC_BUTTON_EXPORT:
			if(thefloppydisk)
			{
				sprintf(filename,"%s",demo->bufferfilename);
				i=0;
				while(filename[i]!=0)
				{
					if(filename[i]=='.')filename[i]='_';
					i++;
				}
				//strcat(filename,".mfm");
				if(fileselector(hwndDlg,1,0,filename,"Export disk/Save As",
					"HFE file (SDCard HxC Floppy Emulator file format)\0*.hfe\0VTR file (VTrucco Floppy Emulator file format)\0*.vtr\0MFM file (MFM/FM track file format)\0*.mfm\0AFI file (Advanced File image format)\0*.afi\0IMG file (RAW Sector file format)\0*.img\0CPC DSK file\0*.dsk\0IMD file\0*.imd\0\0",
					"*.hfe",&extpos,3)
					)
				{
					switch(extpos)
					{
					case 1:
						write_HFE_file(flopemu,thefloppydisk,filename,hwif->interface_mode,hwif->double_step);
						break;
						
					case 2:
						write_vtrucco_file(flopemu,thefloppydisk,filename,hwif->interface_mode);
						break;
						
					case 3:
						write_MFM_file(flopemu,thefloppydisk,filename);
						break;
						
					case 4:
						write_AFI_file(flopemu,thefloppydisk,filename);
						break;
						
					case 5:
						write_RAW_file(flopemu,thefloppydisk,filename);
						break;
				
					case 6:
						write_CPCDSK_file(flopemu,thefloppydisk,filename);
						break;

					case 7:
						write_IMD_file(flopemu,thefloppydisk,filename);
						break;


					}
				}
			}
			else
			{
				MessageBox(hwndDlg,"No floppy loaded !\nPlease drag and drop your disk file into the window","Error",MB_OK|MB_ICONHAND);
			}
			break;
			
			break;
			
		default:;
			break;
		}
		
		break;
		
		
		
		case WM_INITDIALOG:			
			txtindex=0;
			SetDlgItemText(hwndDlg,IDC_EDIT_STATUS,"No disk loaded.");
			SetTimer(hwndDlg,36,100,NULL);

			changefond(hwndDlg,IDC_EDIT_STATUS);
			changefond(hwndDlg,IDC_EDIT_TRACKPOS);
			changefond(hwndDlg,IDC_STATIC_MAIN01);
			changefond(hwndDlg,IDC_STATIC_MAIN02);
			changefond(hwndDlg,IDC_STATIC_MAIN03);
			changefond(hwndDlg,IDC_STATIC_MAIN04);
			changefond(hwndDlg,IDC_STATIC_MAIN05);
			changefond(hwndDlg,IDC_STATIC_MAIN06);
			changefond(hwndDlg,IDC_STATIC_MAIN07);
			changefond(hwndDlg,IDC_STATIC_MAIN08);

			/*
			changefond(hwndDlg,IDC_BUTTON_LOAD);
			changefond(hwndDlg,IDC_BUTTON_LOADRAW);
			changefond(hwndDlg,IDC_BUTTON_CONVERT);
			changefond(hwndDlg,IDC_BUTTON_CREATEFLOPPY);
			changefond(hwndDlg,IDC_BUTTON_EXPORT);
			changefond(hwndDlg,IDC_BUTTON_SDCONFIG);
			changefond(hwndDlg,IDC_BUTTON_USBCONFIG);*/
			break;
			
		case WM_CLOSE:
			DestroyWindow(hwndDlg);
			break;
			
		case WM_TIMER:
			if(strlen(demo->bufferfilename))
			{
				sprintf(tempstr,"%s - %d track(s) %d side(s)     ",demo->bufferfilename,thefloppydisk->floppyNumberOfTrack,thefloppydisk->floppyNumberOfSide);
				if(txtindex>=strlen(tempstr))txtindex=0;
				
				i=0;
				j=txtindex;
				do
				{
					tempstr2[i]=tempstr[j];
					i++;
					j++;
					if(j>=strlen(tempstr)) j=0;
				}while(i<strlen(tempstr));
				tempstr2[i]=0;
				memcpy(&tempstr2[i],tempstr2,strlen(tempstr2));
				
				SetDlgItemText(hwndDlg,IDC_EDIT_STATUS,tempstr2);
				
				txtindex++;
			}
			else
			{
				if(demo->loadstatus!=LOADER_NOERROR)
				{
					switch(demo->loadstatus)
					{
					case LOADER_UNSUPPORTEDFILE:
						SetDlgItemText(hwndDlg,IDC_EDIT_STATUS,"Load error! Image file not supported!");
						break;
					case LOADER_FILECORRUPT:
						SetDlgItemText(hwndDlg,IDC_EDIT_STATUS,"Load error! File corrupted/Read error ?");
						break;
					case LOADER_ACCESSERROR:
						SetDlgItemText(hwndDlg,IDC_EDIT_STATUS,"Load error! Read file error!");
						break;
					default:
						sprintf(tempstr2,"Load error! error %d",demo->loadstatus);
						SetDlgItemText(hwndDlg,IDC_EDIT_STATUS,tempstr2);
						break;
					}
				}
			}
			
			if(thefloppydisk)
			{
				if(hwif)
				{
					sprintf(tempstr,"Track %d/%d",hwif->current_track,thefloppydisk->floppyNumberOfTrack);
					SetDlgItemText(hwndDlg,IDC_EDIT_TRACKPOS,tempstr);
					
					SendDlgItemMessage(hwndDlg,IDC_SLIDER_TRACKPOS, TBM_SETRANGE, TRUE, MAKELONG(0, thefloppydisk->floppyNumberOfTrack));
					SendDlgItemMessage(hwndDlg,IDC_SLIDER_TRACKPOS, TBM_SETPOS, TRUE,(int)hwif->current_track);
				}
			}
			break;
			
			
		default:
			return FALSE;
			
	}
	
	return TRUE;
}

////////////////////////////////////////////////////////////////////////// 
////////////////////////////////////////////////////////////////////////// 
////////////////////////////////////////////////////////////////////////// 
////////////////////////////////////////////////////////////////////////// 
