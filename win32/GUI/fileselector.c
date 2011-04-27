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
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include "fileselector.h"
#include "soft_cfg_file.h"


extern unsigned char cfg_file_buffer[4*1024];

int fileselector(HWND hWnd,char rw,char multi,char * files,char * title,char* selector,char * defext,unsigned int * ext_def,unsigned char initialdir)
{
	OPENFILENAME sfile;
	int i;

	memset(&sfile,sizeof(sfile),0);
	sfile.lStructSize=sizeof(OPENFILENAME);
	sfile.hwndOwner=hWnd;
	sfile.hInstance=GetModuleHandle(NULL);
	sfile.lpstrCustomFilter = NULL;
	sfile.nFilterIndex      = 0;
	sfile.lpstrFileTitle    = NULL;
	if(initialdir)
	{
		sfile.lpstrInitialDir   = &cfg_file_buffer[512*initialdir];
	}
	else
	{
		sfile.lpstrInitialDir   = 0;
	}
	
	sfile.Flags=OFN_PATHMUSTEXIST|OFN_LONGNAMES|OFN_EXPLORER;
	if(rw!=0) sfile.Flags=sfile.Flags|OFN_OVERWRITEPROMPT;
	if(multi) sfile.Flags=sfile.Flags|OFN_ALLOWMULTISELECT;
	sfile.lpstrDefExt       = defext;
	sfile.nMaxFile=1024;
	sfile.lpstrFilter=selector;
	sfile.lpstrTitle=title;

	sfile.lpstrFile=(char*)files;	
	if(rw==0)
	{
		if(GetOpenFileName(&sfile))
		{	
			if(ext_def) *ext_def=sfile.nFilterIndex;
			
			if(initialdir)
			{
				i=strlen(sfile.lpstrFile);
				while(i && sfile.lpstrFile[i]!='\\')
				{
					i--;
				}
				if(sfile.lpstrFile[i]=='\\')
				{
					i++;
				}
				cfg_file_buffer[(512*initialdir)+i]=0;
				memcpy(&cfg_file_buffer[512*initialdir],sfile.lpstrFile,i);
				save_cfg();
			}
			return 1;
		}
		else
		{
			return 0;
		}
	}
	else 
	{
		
		if(GetSaveFileName(&sfile))
		{
			if(ext_def) *ext_def=sfile.nFilterIndex;
			if(initialdir)
			{
				i=strlen(sfile.lpstrFile);
				while(i && sfile.lpstrFile[i]!='\\')
				{
					i--;
				}
				if(sfile.lpstrFile[i]=='\\')
				{
					i++;
				}
				cfg_file_buffer[(512*initialdir)+i]=0;
				memcpy(&cfg_file_buffer[512*initialdir],sfile.lpstrFile,i);
				save_cfg();
			}
			return 1;
		}
		else
		{
			return 0;
		}
		
	}
	
}