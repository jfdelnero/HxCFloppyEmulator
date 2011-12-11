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
// File : Dialog_RAWFileSettings.c
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

#include <shlobj.h>

#include "resource.h"
#include "mainrouts.h"

#include "Dialog_CreateFileSystem.h"

#include "hxc_floppy_emulator.h"
#include "./usb_floppyemulator/usb_hxcfloppyemulator.h"

#include "../../common/plugins/amigadosfs_loader/amigadosfs_loader.h"
#include "../../common/plugins/fat12floppy_loader/fat12floppy_loader.h"


#include "fileselector.h"


extern HWINTERFACE * hwif;
extern guicontext * demo;
extern HXCFLOPPYEMULATOR * flopemu;
extern FLOPPY * thefloppydisk;

	typedef struct fs_type_
	{
		int id;
		char * name;
		char * format_code;
	}fs_type;


	enum
	{
		FS_720KB_ATARI_FAT12=0,
		FS_902KB_ATARI_FAT12,
		FS_360KB_ATARI_FAT12,
		FS_880KB_AMIGADOS,
			
		FS_5P25_300RPM_160KB_MSDOS_FAT12,
		FS_5P25_360RPM_160KB_MSDOS_FAT12,

		FS_5P25_300RPM_180KB_MSDOS_FAT12,
		FS_5P25_360RPM_180KB_MSDOS_FAT12,

		FS_5P25_SS_300RPM_320KB_MSDOS_FAT12,
		FS_5P25_SS_360RPM_320KB_MSDOS_FAT12,

		FS_5P25_DS_300RPM_320KB_MSDOS_FAT12,
		FS_5P25_DS_360RPM_320KB_MSDOS_FAT12,

		FS_5P25_DS_300RPM_360KB_MSDOS_FAT12,
		FS_5P25_DS_360RPM_360KB_MSDOS_FAT12,

		FS_3P5_DS_300RPM_640KB_MSDOS_FAT12,

		FS_720KB_MSDOS_FAT12,

		FS_5P25_300RPM_1200KB_MSDOS_FAT12,

		FS_1_44MB_MSDOS_FAT12,
		FS_1_68MB_MSDOS_FAT12,
		FS_2_88MB_MSDOS_FAT12,
		FS_3_38MB_MSDOS_FAT12,
		FS_4_23MB_ATARI_FAT12,
		FS_6_78MB_MSDOS_FAT12,
		FS_16MB_MSDOS_FAT12
	};



	fs_type fs_type_list[]=
	{
		{ FS_5P25_300RPM_160KB_MSDOS_FAT12,   "5\"25 & 8\" 160KB SSDD 300RPM FAT12",".fat160a"},
		{ FS_5P25_360RPM_160KB_MSDOS_FAT12,   "5\"25 & 8\" 160KB SSDD 360RPM FAT12",".fat160b"},

		{ FS_5P25_300RPM_180KB_MSDOS_FAT12,   "5\"25       180KB SSDD 300RPM FAT12",".fat180a"},
		{ FS_5P25_360RPM_180KB_MSDOS_FAT12,   "5\"25       180KB SSDD 360RPM FAT12",".fat180b"},

		{ FS_5P25_SS_300RPM_320KB_MSDOS_FAT12,"5\"25       320KB SSDD 300RPM FAT12",".fat320ssa"},
		{ FS_5P25_SS_360RPM_320KB_MSDOS_FAT12,"5\"25       320KB SSDD 360RPM FAT12",".fat320ssb"},

		{ FS_5P25_DS_300RPM_320KB_MSDOS_FAT12,"5\"25       320KB DSDD 300RPM FAT12",".fat320dsa"},
		{ FS_5P25_DS_360RPM_320KB_MSDOS_FAT12,"5\"25       320KB DSDD 360RPM FAT12",".fat320dsb"},

		{ FS_5P25_DS_300RPM_360KB_MSDOS_FAT12,"5\"25 & 8\" 360KB DSDD 300RPM FAT12",".fat360a"},
		{ FS_5P25_DS_360RPM_360KB_MSDOS_FAT12,"5\"25 & 8\" 360KB DSDD 360RPM FAT12",".fat360b"},

		{ FS_3P5_DS_300RPM_640KB_MSDOS_FAT12, "3\"5        640KB DSDD FAT12",".fat640"},
		
		{ FS_720KB_MSDOS_FAT12,				  "3\"5        720KB DSDD FAT12 ",".fat720"},

		{ FS_5P25_300RPM_1200KB_MSDOS_FAT12,  "5\"25       1.2MB DSHD FAT12",".fat1200"},

		{ FS_1_44MB_MSDOS_FAT12,              "3\"5        1.44MB DSHD FAT12",".fat1440"},
		{ FS_1_68MB_MSDOS_FAT12,              "3\"5        1.68MB DSHD FAT12",".fat1680"},
		{ FS_2_88MB_MSDOS_FAT12,              "3\"5        2.88MB DSED FAT12",".fat2880"},
		{ FS_3_38MB_MSDOS_FAT12,              "3\"5        3.38MB DSHD FAT12",".fat3381"},
		
		{ FS_6_78MB_MSDOS_FAT12,              "3\"5        6.78MB DSHD FAT12",".fat6789"},

		{ FS_360KB_ATARI_FAT12,               "3\"5        360KB SSDD Atari FAT12",".fatst360"},
		{ FS_720KB_ATARI_FAT12,               "3\"5        720KB DSDD Atari FAT12",".fatst"},
		{ FS_902KB_ATARI_FAT12,               "3\"5        902KB DSDD Atari FAT12",".fatst902"},
		{ FS_4_23MB_ATARI_FAT12,              "3\"5        4.23MB DSDD Atari FAT12",".fatmonsterst"},

		{ FS_880KB_AMIGADOS,                  "3\"5        880KB DSDD AmigaDOS",0},
		

		{ -1,""}			
	};



////////////////////////////////////////////////////////////////////////// 
//
//  Gestion Boite de dialogue Settings
//  
//////////////////////////////////////////////////////////////////////////
BOOL CALLBACK DialogCreateFileSystem(
						  HWND  hwndDlg,	// handle of dialog box
						  UINT  message,	// message
						  WPARAM  wParam,	// first message parameter
						  LPARAM  lParam 	// second message parameter
						  )
{
	static char nbinstance=0;
	int wmId, wmEvent;
	int i,t,ret;
	BROWSEINFO binf;
	LPITEMIDLIST Item;
	char foldername[MAX_PATH];
	wmId    = LOWORD(wParam); 
	wmEvent = HIWORD(wParam); 



	switch (message) 
	{
		
	case WM_COMMAND:
		
		switch (wmEvent)
		{
		case BN_CLICKED:
			
			break;
			
		case EN_CHANGE: //-> appellé a chaque modification d'une boite edit

			break;
		}

		switch (wmId)
		{
			
		case IDBUILDFS:
			i=0;
				memset(&binf,0,sizeof(BROWSEINFO));
				binf.hwndOwner=hwndDlg;
				binf.pszDisplayName="Select a source folder";
				binf.lpszTitle="Select the root directory of the floppy";
				binf.pszDisplayName=foldername;
				Item=SHBrowseForFolder(&binf);
				if(Item)
				{
					if(SHGetPathFromIDList(Item,foldername))
					{
						t=SendDlgItemMessage(hwndDlg, IDC_FSTYPE, CB_GETCURSEL, 0, 0);
						
						sprintf(demo->buffertext,"    Loading floppy   ");
						sprintf(demo->bufferfilename,"");	

						if(thefloppydisk)
						{
							hxcfe_floppyUnload(flopemu,thefloppydisk);
						}
						else
						{
							thefloppydisk=(FLOPPY *)malloc(sizeof(FLOPPY));
							memset(thefloppydisk,0,sizeof(FLOPPY));
						}

					/*	i=0;
						while((fs_type_list[i].id!=t) && (fs_type_list[i].id!=-1))
						{
							i++;
						}*/

						i=t;

						if(fs_type_list[i].id!=-1)
						{

							if(fs_type_list[i].format_code)
							{
								//ret=FAT12FLOPPY_libLoad_DiskFile(flopemu,thefloppydisk,foldername,fs_type_list[i].format_code);
							}
							else
							{
								//ret=AMIGADOSFSDK_libLoad_DiskFile(flopemu,thefloppydisk,foldername,0);
							}

						}
						else
						{
							ret=HXCFE_UNSUPPORTEDFILE;
						}

						if(ret!=HXCFE_NOERROR)
						{
							switch(ret)
							{
								case HXCFE_UNSUPPORTEDFILE:
									sprintf(demo->buffertext,"      Load error!\n\n\n\n    Image file not\n    yet supported!");
									break;
								case HXCFE_FILECORRUPTED:
									sprintf(demo->buffertext,"      Load error!\n\n\n\n   File corrupted ?\n    Read error ?");
									break;
								case HXCFE_ACCESSERROR:
									sprintf(demo->buffertext,"      Load error!\n  Read file error!");
									break;
								default:
									sprintf(demo->buffertext,"      Load error!\n      error %d",ret);
									break;
							}
							free(thefloppydisk);
							thefloppydisk=0;

						}
						else
						{	
							InjectFloppyImg(flopemu,thefloppydisk,hwif);
							
							if(foldername)
							{
								i=strlen(foldername);
								while(i!=0 && foldername[i]!='\\')
								{
									i--;
								}
								if(foldername[i]=='\\') i++;
								sprintf(demo->bufferfilename,"%s",&foldername[i]);
							}
							else
							{
								sprintf(demo->bufferfilename,"Empty Floppy");
							}

							sprintf(demo->buffertext,"      Load ok!\n Track: %d\n Sector: %d\n Side: %d\n",thefloppydisk->floppyNumberOfTrack,thefloppydisk->floppySectorPerTrack,thefloppydisk->floppyNumberOfSide);
						}
						demo->loadstatus=ret;
					}
				}

				nbinstance=0;
				DestroyWindow(hwndDlg);
			break;
			
		case IDCANCELRAWFILE:
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

				i=0;
				do
				{
					SendDlgItemMessage(hwndDlg,IDC_FSTYPE, CB_ADDSTRING, 0, (LPARAM)fs_type_list[i].name);
					i++;
				}while(fs_type_list[i].id!=-1);

				SendDlgItemMessage(hwndDlg, IDC_FSTYPE, CB_SETCURSEL,0, 0);		
			}
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

////////////////////////////////////////////////////////////////////////// 
////////////////////////////////////////////////////////////////////////// 
////////////////////////////////////////////////////////////////////////// 
////////////////////////////////////////////////////////////////////////// 
