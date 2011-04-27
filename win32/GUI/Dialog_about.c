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
// File : Dialog_about.c
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

#include "mod32.h"
#include "microintro.h"
#include "themod.h"

#include "resource.h"
#include "mainrouts.h"


#include "Dialog_about.h"
#include "Dialog_license.h"


////////////////////////////////////////////////////////////////////////// 
//
//  Gestion Boite de dialogue apropos (Timer 50hz + Wave +msgs...)
//  
//////////////////////////////////////////////////////////////////////////
BOOL CALLBACK DialogAbout(
    HWND  hwndDlg,	// handle of dialog box
    UINT  message,	// message
    WPARAM  wParam,	// first message parameter
    LPARAM  lParam 	// second message parameter
   )
{
static char nbinstance=0;
static HWAVEOUT shwd;
static WAVEFORMATEX pwfx;
static WAVEHDR pwhOut1;
static WAVEHDR pwhOut2;
static char buffer2[1024*16];
static char buffer1[1024*16];
static RECT myRect;
static HDC hdc;	
static BITMAPINFO * bmapinfo;
static uintro_context * ui_context;
int wmId;//, wmEvent;
unsigned short xsize,ysize;

	wmId    = LOWORD(wParam);
	switch (message) 
	{
		case	MM_WOM_OPEN:
		break;
	
		case	MM_WOM_CLOSE:
		break;
	
		case	MM_WOM_DONE:
			GiveMeSamples(((struct wavehdr_tag *)lParam)->lpData,sizeof(buffer2));
			waveOutWrite(shwd,(struct wavehdr_tag *)lParam,sizeof(pwhOut2));
		break;

		case WM_COMMAND:
			switch (wmId)
			{
				case ID_OK:
					KillTimer(hwndDlg,33);
					waveOutReset(shwd);

					waveOutBreakLoop(shwd);
					waveOutBreakLoop(shwd);
  
					waveOutUnprepareHeader(shwd,&pwhOut1,sizeof(WAVEHDR));
					waveOutUnprepareHeader(shwd,&pwhOut2,sizeof(WAVEHDR));
					CLOSEMODSYS();

					waveOutClose(shwd);
					nbinstance=0;

					DestroyWindow(hwndDlg);
					break;
				case IDC_BUTTONLICENSE:
					CreateDialog(GetModuleHandle(NULL),(const char*)ID_DIALOG_LICENSE,GetActiveWindow(),&DialogLicense);
					break;
				case IDC_BUTTON1:
					ShellExecute(NULL, "open", "http://hxc2001.free.fr/floppy_drive_emulator", NULL, NULL, SW_SHOWNORMAL);
					break;
				case IDC_RELEASENOTES:
					ShellExecute(NULL, "open", "http://hxc2001.free.fr/floppy_drive_emulator/hxcfloppyemulator_soft_release_notes.txt", NULL, NULL, SW_SHOWNORMAL);
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
				xsize=180;
				ysize=110;

				ui_context=uintro_init(xsize,ysize);

				nbinstance=1;
				bmapinfo = (BITMAPINFO *) malloc((sizeof(BITMAPINFO) + sizeof(RGBQUAD) * 255));
				memset(bmapinfo,0,(sizeof(BITMAPINFO) + sizeof(RGBQUAD) * 255));
				bmapinfo->bmiHeader.biSize=sizeof(bmapinfo->bmiHeader);
				bmapinfo->bmiHeader.biWidth=xsize;
				bmapinfo->bmiHeader.biHeight=-ysize;
				bmapinfo->bmiHeader.biPlanes=1;
				bmapinfo->bmiHeader.biBitCount=32;
				bmapinfo->bmiHeader.biCompression=BI_RGB;

				if(waveOutGetNumDevs()!=0)
				{
					InitModule(NULL,buffer1,sizeof(buffer1),(char*)&themod);
					pwfx.wFormatTag=1;
					pwfx.nChannels=2;
					pwfx.nSamplesPerSec=44100;//SAMPLERATE;
					pwfx.nAvgBytesPerSec=pwfx.nSamplesPerSec*4;
					pwfx.nBlockAlign=4;
					pwfx.wBitsPerSample=16;
					pwfx.cbSize=0;

					waveOutOpen(&shwd,WAVE_MAPPER,&pwfx,(unsigned long)hwndDlg,0,CALLBACK_WINDOW);
					pwhOut1.lpData=(char*)buffer1;
					pwhOut1.dwBufferLength=sizeof(buffer1);
					pwhOut1.dwFlags=0;
					pwhOut1.dwLoops=0;
		
					pwhOut2.lpData=(char*)buffer2;
					pwhOut2.dwBufferLength=sizeof(buffer2);
					pwhOut2.dwFlags=0;
					pwhOut2.dwLoops=0;
	
					waveOutPrepareHeader(shwd, &pwhOut1, sizeof(pwhOut1));
					waveOutPrepareHeader(shwd, &pwhOut2, sizeof(pwhOut2));
					
					waveOutWrite(shwd,&pwhOut1,sizeof(pwhOut1));
					waveOutWrite(shwd,&pwhOut2,sizeof(pwhOut2));
				}
			
			SetTimer(hwndDlg,33,20,NULL);
		}
		break;

		case WM_CLOSE:
			
			KillTimer(hwndDlg,33);

			waveOutReset(shwd);

			waveOutBreakLoop(shwd);
			waveOutBreakLoop(shwd);
  
			waveOutUnprepareHeader(shwd,&pwhOut1,sizeof(WAVEHDR));
			waveOutUnprepareHeader(shwd,&pwhOut2,sizeof(WAVEHDR));
			
			CLOSEMODSYS();

			waveOutClose(shwd);
			uintro_deinit(ui_context);

			nbinstance=0;
			DestroyWindow(hwndDlg);
		break;
		
		case WM_TIMER:
				
			uintro_getnextframe(ui_context);

			hdc=GetDC(hwndDlg);
			GetClientRect(hwndDlg,&myRect);
			StretchDIBits(hdc,180,18,ui_context->xsize,ui_context->ysize,0,0,ui_context->xsize,ui_context->ysize,ui_context->framebuffer,bmapinfo,0,SRCCOPY);
			ReleaseDC(hwndDlg,hdc);
		
		break;

		default:
		return FALSE;
		
	}

	return TRUE;
}
