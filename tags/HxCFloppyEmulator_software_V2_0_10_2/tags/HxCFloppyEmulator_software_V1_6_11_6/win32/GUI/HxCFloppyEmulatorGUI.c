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
// File : HxCFloppyEmu.c
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

#include <mmsystem.h>
//#include "mod32.h"
#include <math.h>

#include "resource.h"
#include "mainrouts.h"

#include "hxc_floppy_emulator.h"
#include "internal_floppy.h"
#include "floppy_loader.h"
#include "./usb_floppyemulator/usb_hxcfloppyemulator.h"

#include "afi_file_writer.h"
#include "mfm_file_writer.h"
#include "hfe_file_writer.h"
#include "extended_hfe_file_writer.h"
#include "raw_file_writer.h"
#include "vtrucco_file_writer.h"
#include "cpcdsk_file_writer.h"
#include "imd_file_writer.h"

#include "win32_api.h"

#include "version.h"

#include "fileselector.h"
#include "HxCFloppyEmulatorGUI.h"

#include "../../common/plugins/raw_loader/raw_loader.h"

#include <ftd2xx.h>

#include "Dialog_config.h"
#include "Dialog_stats.h"
#include "Dialog_logs.h"
#include "Dialog_about.h"
#include "Dialog_RAWFileSettings.h"
#include "Dialog_CreateFileSystem.h"
#include "Dialog_BatchConvert.h"
#include "Dialog_MainDialog.h"
#include "Dialog_sdhxcfe_filecfg.h"
#include "Dialog_floppydump.h"

#include "loader.h"

#include "soft_cfg_file.h"

extern logfifo logsfifo;
extern cfgrawfile rawfileconfig;
guicontext * demo;

HWINTERFACE * hwif;
HXCFLOPPYEMULATOR * flopemu;
FLOPPY * thefloppydisk;
CRITICAL_SECTION log_cs;

ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX); 
	wcex.style			= 0;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, (LPCTSTR)IDI_ICON2);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_BTNFACE);
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= WINDOW_CLASS;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_ICON2);
	wcex.hIcon		    = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_ICON2);
	wcex.cbWndExtra		= 0;
	return RegisterClassEx(&wcex);
}


BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;
	int TailleX,TailleY;
	HWND hDlg_MainDialog;

	TailleX=341;
	TailleY=392+GetSystemMetrics(SM_CYCAPTION)+GetSystemMetrics(SM_CYMENU)+GetSystemMetrics(SM_CYBORDER)-3;
	
	hWnd=CreateWindowEx(WS_EX_LEFT,"hxc2001_Class",NOMFENETRE,WS_MINIMIZEBOX|WS_OVERLAPPED|WS_SYSMENU,16,16,TailleX,TailleY,NULL,NULL,hInstance,NULL);
	memset(&logsfifo,0,sizeof(logfifo));

	if (!hWnd)
	{
		return FALSE;
	}
	
	SetMenu(hWnd,LoadMenu(hInstance,(const char *)IDR_MENU1));
	
	// Affichage de la fenetre principale
	
	ShowWindow(hWnd,SW_SHOWNORMAL);
	CheckMenuItem(GetMenu(hWnd),ID_DRIVE_DS0,MF_BYCOMMAND | MF_CHECKED );	
		

	demo=(guicontext*)malloc(sizeof(guicontext));
	memset(demo,0,sizeof(guicontext));
	demo->autoselectmode=1;


	SetPriorityClass(GetCurrentProcess(),ABOVE_NORMAL_PRIORITY_CLASS);

	InitializeCriticalSection(&log_cs);

	flopemu=(HXCFLOPPYEMULATOR*)malloc(sizeof(HXCFLOPPYEMULATOR));
	memset(flopemu,0,sizeof(HXCFLOPPYEMULATOR));
	flopemu->hxc_printf=&CUI_affiche;
	initHxCFloppyEmulator(flopemu);
	
	thefloppydisk=0;

	hwif=(HWINTERFACE *)malloc(sizeof(HWINTERFACE));
	memset(hwif,0,sizeof(HWINTERFACE));

	load_last_cfg();

	HW_CPLDFloppyEmulator_init(flopemu,hwif);
	//////////////////////////////////////////////////////////////////
	
	load_last_cfg();

	hDlg_MainDialog=CreateDialog(hInstance,(const char*)IDD_DIALOG_MAINDIALOG,hWnd,&DialogMainDialog);
	
	ShowWindow(hDlg_MainDialog,SW_SHOWNORMAL);
	DragAcceptFiles(hDlg_MainDialog,TRUE);
	UpdateWindow(hWnd);

   return TRUE;
}


int getfilename(char * cmdline,char * filename)
{
	int i;
	int fin_ligne;
	int debut_ligne;
	
	i=strlen(cmdline);
	if(i)
	{
		if(cmdline[i-1]=='"')
		{
			fin_ligne=i-1;
			i--;
			while(i && cmdline[i-1]!='"' )
			{
				i--;
			}
			debut_ligne=i;
			memset(filename,0,(fin_ligne-debut_ligne)+2);
			memcpy(filename,&cmdline[debut_ligne],(fin_ligne-debut_ligne));
			
		}
		else
		{
			while(i && cmdline[i-1]!=' ' )
			{
				i--;
			}
			sprintf((char*)filename,"%s",&cmdline[i]);
		}
		
	}
	
	return i;
}


int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	HWND hWnd;
	char filename[1024];
	unsigned int i;
	unsigned char c;

	MSG msg;
	HACCEL hAccelTable;


	MyRegisterClass(hInstance);


	if (((hWnd = FindWindow(NULL, NOMFENETRE)) == NULL) || !getfilename(lpCmdLine,filename))
	{

		rawfileconfig.bitrate=250000;
		rawfileconfig.gap3=50;
		rawfileconfig.numberoftrack=80;
		rawfileconfig.sectorpertrack=9;
		rawfileconfig.rpm=300;
		rawfileconfig.sectorsize=2;//SECTORSIZE_512;
		rawfileconfig.tracktype=MFMIBM_TRACK_TYPE;
		rawfileconfig.sidecfg=TWOSIDESFLOPPY;
		rawfileconfig.interleave=1;
		rawfileconfig.firstidsector=1;
		rawfileconfig.fillvalue=0xF6;
		rawfileconfig.autogap3=0xFF;

		// Perform application initialization:
		if (!InitInstance (hInstance, nCmdShow)) 
		{
			return FALSE;
		}

		hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDI_ICON2);

		///////////////////////////////////////

		if(getfilename(lpCmdLine,filename))
		{
			sprintf(demo->buffertext,"    Loading floppy   ");
			loadfloppy(filename,0,0);
		}

		///////////////////////////////////////
		// Main message loop:
		while (GetMessage(&msg, NULL, 0, 0)) 
		{
			if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) 
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}

		return msg.wParam;
	}
	else
	{
			if(getfilename(lpCmdLine,filename))
			{
				strcat(filename,"%");

				i=0;
				do
				{
					c=filename[i];
					SendMessage(hWnd,WM_USER+1,666,c);
					i++;
				}while(c!='%');	
			}
	}
	return 0;
	// Fin
}

////////////////////////////////////////////////////////////////////////// 
//
//  Fonction de gestion des évenements de la fenetre Mere
//  (Appelle par windows lorsqu'un evenement arrive à la fenetre.)
//
//////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{	
	int wmId, wmEvent,i,extpos;
	char filename[1024];
	static char filename2[1024];

	
	switch (message) // switch selon le message
	{
		
	case WM_CREATE:
		sprintf(filename2,"");
		break;
		
	case WM_DROPFILES:
		DragQueryFile((HDROP)wParam, 0, (char*)filename, 1024);
		sprintf(demo->buffertext,"    Loading floppy   ");
		sprintf(demo->bufferfilename,"");
		loadfloppy(filename,0,0);
		break;	
		
	case WM_COMMAND:	//Action sur un menu
		wmId    = LOWORD(wParam); 
		wmEvent = HIWORD(wParam); 
		switch(wmId)
		{
		case ID_DRIVE_SETTINGS:
			CreateDialog(GetModuleHandle(NULL),(const char*)IDD_DIALOG_CONFIG,GetActiveWindow(),&DialogSettings);
			break;

		case ID_ABOUT_HXCFLOPPYEMULATOR:
			CreateDialog(GetModuleHandle(NULL),(const char*)ID_DIALOG_ABOUT,GetActiveWindow(),&DialogAbout);
			break;	

		case ID_INFOS_USBSTATS:
			CreateDialog(GetModuleHandle(NULL),(const char*)IDD_DIALOG_STATS,GetActiveWindow(),&DialogStats);
			break;	

		case ID_DISK_EXPORT:
			if(thefloppydisk)
			{
				sprintf(filename,"%s",demo->bufferfilename);
				i=0;
				while(filename[i]!=0)
				{
					if(filename[i]=='.')filename[i]='_';
					i++;
				}

				if(fileselector(hWnd,1,0,filename,"Export disk/Save As",
					"HFE file (SDCard HxC Floppy Emulator file format)\0*.hfe\0VTR file (VTrucco Floppy Emulator file format)\0*.vtr\0MFM file (MFM/FM track file format)\0*.mfm\0AFI file (Advanced File image format)\0*.afi\0IMG file (RAW Sector file format)\0*.img\0CPC DSK file\0*.dsk\0IMD file\0*.imd\0HFE file (Rev 2 - Experimental)\0*.hfe\0\0",
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

					case 8:
						write_EXTHFE_file(flopemu,thefloppydisk,filename,hwif->interface_mode,hwif->double_step);
						break;

					}
				}
			}
			else
			{
				MessageBox(hWnd,"No floppy loaded !\nPlease drag and drop your disk file into the window","Error",MB_OK|MB_ICONHAND);
			}

			break;

		case ID_INFOS_SETLOGFILE:
			CreateDialog(GetModuleHandle(NULL),(const char*)ID_DIALOG_LOGS,GetActiveWindow(),&DialogLogs);
			break;

		case ID_LOAD_LOAD:
			sprintf(filename,"*.*");
			i=0;
			if(fileselector(hWnd,0,0,filename,"Load image file","All disk image types\0*.*\0\0","*.*",0,1))
			{
				sprintf(demo->buffertext,"    Loading floppy   ");
				sprintf(demo->bufferfilename,"");
				loadfloppy(filename,0,0);
			}
			break;

		case ID_IMAGES_LOADRAWFILE:
			CreateDialog(GetModuleHandle(NULL),(const char*)IDD_RAWFILECONFIG,GetActiveWindow(),&DialogRAWFileSettings);
			break;

		case ID_IMAGES_CREATEFILESYSTEM:
			CreateDialog(GetModuleHandle(NULL),(const char*)IDD_CREATEFILESYSTEM,GetActiveWindow(),&DialogCreateFileSystem);
			break;

		case ID_IMAGES_CONVERT:
			CreateDialog(GetModuleHandle(NULL),(const char*)IDD_BATCHCONVERT,GetActiveWindow(),&DialogBatchConvert);
			break;

		case ID_FLOPPYIMAGE_DUMPAFLOPPYDISK:
			CreateDialog(GetModuleHandle(NULL),(const char*)IDD_DIALOG_FLOPPYDUMP,GetActiveWindow(),&DialogFloppyDump);
			break;
		}
		break;

		case WM_DESTROY:
			PostQuitMessage(0);//fin
			break;
			
		case WM_USER: //Message venant de l'icone de la barre des taches	
			break;
			
		case WM_USER+1: 
			filename2[strlen(filename2)]=lParam&0xff;
			if((lParam&0xff)=='%')
			{
				filename2[strlen(filename2)-1]=0;
				sprintf(demo->buffertext,"    Loading floppy   ");
				
				if(loadfloppy(filename2,0,0)!=LOADER_NOERROR)
				{
					sprintf(demo->buffertext,"      Load error!    ");
				}
				else
				{		
					//sprintf(demo->buffertext,"      Load ok!\n Track: %d\n Sector: %d\n Side: %d\n",thefloppy.number_of_track,thefloppy.number_of_sector_per_track,thefloppy.number_of_side);
				}
				memset(filename2,0,1024);
				SetFocus(hWnd);
			}
			break;
			


		case WM_CLOSE: //message de fermeture
			PostQuitMessage(0);
			break;
			
		case WM_MOUSEMOVE:
			break;
			
		case MM_WOM_OPEN:
			break;
			
		case MM_WOM_CLOSE:
			break;
			
		case MM_WOM_DONE:
			break;
			
		case WM_KEYDOWN:			
			break;
			
		default: // traitement par defaut de l'evenement (gerer par windows)
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

