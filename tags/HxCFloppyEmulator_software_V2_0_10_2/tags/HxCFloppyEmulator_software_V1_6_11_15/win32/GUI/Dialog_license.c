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
// File : Dialog_logs.c
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

#include "Dialog_license.h"
#include "licensetxt.h"


////////////////////////////////////////////////////////////////////////// 
//
//  Gestion Boite de dialogue Log
//  
//////////////////////////////////////////////////////////////////////////
BOOL CALLBACK DialogLicense(
						 HWND  hwndDlg,	// handle of dialog box
						 UINT  message,	// message
						 WPARAM  wParam,	// first message parameter
						 LPARAM  lParam 	// second message parameter
						 )
{
	static char nbinstance=0;
	int wmId;//, wmEvent;
	
	wmId    = LOWORD(wParam);
	switch (message) 
	{
		
		case WM_COMMAND:
			switch (wmId)
			{
			
				case ID_CLOSE:
					nbinstance=0;
					DestroyWindow(hwndDlg);
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
			}
			
			SetDlgItemText( hwndDlg, IDC_LICENSE,licensetxt); 
			break;
			
			
		case WM_CLOSE:
			nbinstance=0;
			DestroyWindow(hwndDlg);
			break;
						
		default:
			return FALSE;
			
	}
	
	return TRUE;
}


