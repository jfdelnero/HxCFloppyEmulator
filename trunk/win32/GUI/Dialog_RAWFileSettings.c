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

#include <windows.h>
#include <commctrl.h>

#include <stdio.h>
#include <time.h>

#include "resource.h"
#include "mainrouts.h"

#include "Dialog_RAWFileSettings.h"

#include "hxc_floppy_emulator.h"
#include "internal_floppy.h"
#include "floppy_loader.h"
#include "./usb_floppyemulator/usb_hxcfloppyemulator.h"


#include "../../common/plugins/raw_loader/raw_loader.h"

#include "loader.h"
#include "fileselector.h"


extern HWINTERFACE * hwif;
extern guicontext * demo;
extern HXCFLOPPYEMULATOR * flopemu;

cfgrawfile rawfileconfig;



	track_type track_type_list[]=
	{
		{ FM_TRACK_TYPE,"FM"},
		{ FMIBM_TRACK_TYPE,"IBM FM"},
		{ MFM_TRACK_TYPE,"MFM"},
		{ MFMIBM_TRACK_TYPE,"IBM MFM"},
		//{ GCR_TRACK_TYPE,"GCR"},
		{ -1,""}
	};

	sectorsize_type sectorsize_type_list[]=
	{
		{ SECTORSIZE_128,"128"},
		{ SECTORSIZE_256,"256"},
		{ SECTORSIZE_512,"512"},
		{ SECTORSIZE_1024,"1024"},
		{ SECTORSIZE_2048,"2048"},
		{ SECTORSIZE_4096,"4096"},
		{ SECTORSIZE_8192,"8192"},
		{ SECTORSIZE_16384,"16384"},		
		{ -1,""}			
	};



////////////////////////////////////////////////////////////////////////// 
//
//  Gestion Boite de dialogue Settings
//  
//////////////////////////////////////////////////////////////////////////
BOOL CALLBACK DialogRAWFileSettings(
						  HWND  hwndDlg,	// handle of dialog box
						  UINT  message,	// message
						  WPARAM  wParam,	// first message parameter
						  LPARAM  lParam 	// second message parameter
						  )
{
	static char nbinstance=0;
	int wmId, wmEvent,totalsector,totalsize;
	char filename[1024];
	int i,t;
	cfgrawfile temp_rawfileconfig;
	static int old_bitrate,new_bitrate,old_rpm,new_rpm;
	FILE* fpf_file;
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
			

		case ID_MAKEEMPTYFLOPPY:
		case IDLOADRAWFILE:
			sprintf(filename,"*.*");
			i=0;


			if(wmId==IDLOADRAWFILE)
			{
				if(!fileselector(hwndDlg,0,0,filename,"Load image file","All raw disk image types\0*.*\0\0","*.*",0,1))
				{
						break;
				}
			}

		
				rawfileconfig.numberoftrack=GetDlgItemInt(hwndDlg,IDC_NUMBEROFTRACK,NULL,0);
				rawfileconfig.sectorpertrack=GetDlgItemInt(hwndDlg,IDC_SECTORPERTRACK,NULL,0);
				rawfileconfig.bitrate=GetDlgItemInt(hwndDlg,IDC_BITRATE,NULL,0);
				rawfileconfig.rpm=GetDlgItemInt(hwndDlg,IDC_RPM,NULL,0);
				rawfileconfig.gap3=GetDlgItemInt(hwndDlg,IDC_GAP3LEN,NULL,0);
				rawfileconfig.firstidsector=GetDlgItemInt(hwndDlg,IDC_SECTORIDSTART,NULL,0);

				t=SendDlgItemMessage(hwndDlg, IDC_SECTORSIZE, CB_GETCURSEL, 0, 0);
				if(t>255) t=255;
				rawfileconfig.sectorsize=(unsigned char)t;


				t=SendDlgItemMessage(hwndDlg, IDC_TRACKTYPE, CB_GETCURSEL, 0, 0);
				if(t>255) t=255;
				rawfileconfig.tracktype=t;

				rawfileconfig.interleave=GetDlgItemInt(hwndDlg,IDC_INTERLEAVE,NULL,0);
				
				rawfileconfig.skew=GetDlgItemInt(hwndDlg,IDC_SKEW,NULL,0);

				rawfileconfig.sidecfg=0;

				if(SendDlgItemMessage(hwndDlg,IDC_SIDE1ENABLE,BM_GETCHECK,0,0))
				{
					rawfileconfig.sidecfg=rawfileconfig.sidecfg|TWOSIDESFLOPPY;
				}

				if(SendDlgItemMessage(hwndDlg,IDC_REVERSESIDE,BM_GETCHECK,0,0))
				{
					rawfileconfig.sidecfg=rawfileconfig.sidecfg|SIDE_INVERTED;
				}

				if(SendDlgItemMessage(hwndDlg,IDC_SIDE0FIRST,BM_GETCHECK,0,0))
				{
					rawfileconfig.sidecfg=rawfileconfig.sidecfg|SIDE0_FIRST;
				}

				
				sprintf(demo->buffertext,"    Loading floppy   ");
		
				sprintf(demo->bufferfilename,"");	
				if(wmId==IDLOADRAWFILE)
				{
					loadfloppy(filename,0,&rawfileconfig);
				}
				else
				{
					loadfloppy(0,0,&rawfileconfig);
				}


				nbinstance=0;
				KillTimer(hwndDlg,34);

				DestroyWindow(hwndDlg);
			break;
			
		case IDCANCELRAWFILE:
			nbinstance=0;
			KillTimer(hwndDlg,34);
			DestroyWindow(hwndDlg);
			break;
			
		case IDSAVECONFIG:
			sprintf(filename,"floppy_profile.fpf");
				if(fileselector(hwndDlg,1,0,filename,"Save floppy profile","Floppy profile settings\0*.fpf\0\0","*.fpf",0,2))
				{
					rawfileconfig.numberoftrack=GetDlgItemInt(hwndDlg,IDC_NUMBEROFTRACK,NULL,0);
					rawfileconfig.sectorpertrack=GetDlgItemInt(hwndDlg,IDC_SECTORPERTRACK,NULL,0);
					rawfileconfig.bitrate=GetDlgItemInt(hwndDlg,IDC_BITRATE,NULL,0);
					rawfileconfig.rpm=GetDlgItemInt(hwndDlg,IDC_RPM,NULL,0);
					rawfileconfig.gap3=GetDlgItemInt(hwndDlg,IDC_GAP3LEN,NULL,0);
					rawfileconfig.firstidsector=GetDlgItemInt(hwndDlg,IDC_SECTORIDSTART,NULL,0);

					t=SendDlgItemMessage(hwndDlg, IDC_SECTORSIZE, CB_GETCURSEL, 0, 0);
					if(t>255) t=255;
					rawfileconfig.sectorsize=(unsigned char)t;


					t=SendDlgItemMessage(hwndDlg, IDC_TRACKTYPE, CB_GETCURSEL, 0, 0);
					if(t>255) t=255;
					rawfileconfig.tracktype=t;

					rawfileconfig.interleave=GetDlgItemInt(hwndDlg,IDC_INTERLEAVE,NULL,0);
					
					rawfileconfig.skew=GetDlgItemInt(hwndDlg,IDC_SKEW,NULL,0);

					rawfileconfig.sidecfg=0;

					if(SendDlgItemMessage(hwndDlg,IDC_SIDE1ENABLE,BM_GETCHECK,0,0))
					{
						rawfileconfig.sidecfg=rawfileconfig.sidecfg|TWOSIDESFLOPPY;
					}

					if(SendDlgItemMessage(hwndDlg,IDC_REVERSESIDE,BM_GETCHECK,0,0))
					{
						rawfileconfig.sidecfg=rawfileconfig.sidecfg|SIDE_INVERTED;
					}

					if(SendDlgItemMessage(hwndDlg,IDC_SIDE0FIRST,BM_GETCHECK,0,0))
					{
						rawfileconfig.sidecfg=rawfileconfig.sidecfg|SIDE0_FIRST;
					}

					fpf_file=fopen(filename,"wb");
					if(fpf_file)
					{
						fprintf(fpf_file,"FPF_V0.1");
						fwrite(&rawfileconfig,sizeof(cfgrawfile),1,fpf_file);
						fclose(fpf_file);
					}
					else
						MessageBox(hwndDlg,"Cannot create the file !","Error",MB_OK|MB_ICONERROR);
				

				}
			break;

			
		case IDLOADCONFIG:
			sprintf(filename,"*.fpf");
				if(fileselector(hwndDlg,0,0,filename,"Load floppy profile","Floppy profile settings\0*.fpf\0\0","*.fpf",0,2))
				{
					fpf_file=fopen(filename,"rb");
					if(fpf_file)
					{
						memset(filename,0,9);
						fread(filename,8,1,fpf_file);
						if(!strcmp(filename,"FPF_V0.1"))
						{
							rawfileconfig.firstidsector=1;
							fseek(fpf_file,8,SEEK_SET);
							fread(&rawfileconfig,sizeof(cfgrawfile),1,fpf_file);
							fclose(fpf_file);


							SetDlgItemInt(hwndDlg,IDC_NUMBEROFTRACK,rawfileconfig.numberoftrack,0);
							SetDlgItemInt(hwndDlg,IDC_RPM,rawfileconfig.rpm,0);
							SetDlgItemInt(hwndDlg,IDC_BITRATE,rawfileconfig.bitrate,0);
							SetDlgItemInt(hwndDlg,IDC_GAP3LEN,rawfileconfig.gap3,0);
							SetDlgItemInt(hwndDlg,IDC_SECTORPERTRACK,rawfileconfig.sectorpertrack,0);
							SetDlgItemInt(hwndDlg,IDC_INTERLEAVE,rawfileconfig.interleave,0);
							SetDlgItemInt(hwndDlg,IDC_SKEW,rawfileconfig.skew,0);
							SetDlgItemInt(hwndDlg,IDC_SECTORIDSTART,rawfileconfig.firstidsector,0);
							
							old_bitrate=rawfileconfig.bitrate;
							old_rpm=rawfileconfig.rpm;
							SendDlgItemMessage(hwndDlg,IDC_SIDE1ENABLE,BM_SETCHECK,rawfileconfig.sidecfg&TWOSIDESFLOPPY?BST_CHECKED:BST_UNCHECKED,0);
							SendDlgItemMessage(hwndDlg,IDC_REVERSESIDE,BM_SETCHECK,rawfileconfig.sidecfg&SIDE_INVERTED?BST_CHECKED:BST_UNCHECKED,0);
							SendDlgItemMessage(hwndDlg,IDC_SIDE0FIRST ,BM_SETCHECK,rawfileconfig.sidecfg&SIDE0_FIRST?BST_CHECKED:BST_UNCHECKED,0);
							SendDlgItemMessage(hwndDlg, IDC_TRACKTYPE, CB_SETCURSEL, rawfileconfig.tracktype, 0);
							SendDlgItemMessage(hwndDlg, IDC_SECTORSIZE, CB_SETCURSEL, rawfileconfig.sectorsize, 0);
						}
						else
						{
							MessageBox(hwndDlg,"this is not a floppy profile file !","Error",MB_OK|MB_ICONERROR);
							fclose(fpf_file);
						}
					}
					else
						MessageBox(hwndDlg,"Cannot open the file !","Error",MB_OK|MB_ICONERROR);
				}
			break;

		default:;
		}
		
		
		break;
		
		case WM_TIMER:
			
			temp_rawfileconfig.bitrate=GetDlgItemInt(hwndDlg,IDC_BITRATE,NULL,0);
			temp_rawfileconfig.rpm=GetDlgItemInt(hwndDlg,IDC_RPM,NULL,0);
			temp_rawfileconfig.gap3=GetDlgItemInt(hwndDlg,IDC_GAP3LEN,NULL,0);
			temp_rawfileconfig.interleave=GetDlgItemInt(hwndDlg,IDC_INTERLEAVE,NULL,0);
			temp_rawfileconfig.skew=GetDlgItemInt(hwndDlg,IDC_SKEW,NULL,0);
			temp_rawfileconfig.numberoftrack=GetDlgItemInt(hwndDlg,IDC_NUMBEROFTRACK,NULL,0);
			temp_rawfileconfig.sectorpertrack=GetDlgItemInt(hwndDlg,IDC_SECTORPERTRACK,NULL,0);
			temp_rawfileconfig.firstidsector=GetDlgItemInt(hwndDlg,IDC_SECTORIDSTART,NULL,0);

			t=SendDlgItemMessage(hwndDlg, IDC_SECTORSIZE, CB_GETCURSEL, 0, 0);
			if(t>255) t=255;
			temp_rawfileconfig.sectorsize=(unsigned char)t;
			t=SendDlgItemMessage(hwndDlg, IDC_TRACKTYPE, CB_GETCURSEL, 0, 0);
			if(t>255) t=255;
			temp_rawfileconfig.tracktype=t;



			totalsector=1;
			if(SendDlgItemMessage(hwndDlg,IDC_SIDE1ENABLE,BM_GETCHECK,0,0))
			{
				totalsector++;
			}

			totalsector=totalsector*GetDlgItemInt(hwndDlg,IDC_SECTORPERTRACK,NULL,0);
			totalsector=totalsector*GetDlgItemInt(hwndDlg,IDC_NUMBEROFTRACK,NULL,0);

			switch(SendDlgItemMessage(hwndDlg, IDC_SECTORSIZE, CB_GETCURSEL, 0, 0))
			{
			case SECTORSIZE_128:
				totalsize=totalsector*128;
				break;
			case SECTORSIZE_256:
				totalsize=totalsector*256;
				break;
			case SECTORSIZE_512:
				totalsize=totalsector*512;
				break;
			case SECTORSIZE_1024:
				totalsize=totalsector*1024;
				break;
			case SECTORSIZE_2048:
				totalsize=totalsector*2048;
				break;
			case SECTORSIZE_4096:
				totalsize=totalsector*4096;
				break;
			case SECTORSIZE_8192:
				totalsize=totalsector*8192;
				break;
			case SECTORSIZE_16384:
				totalsize=totalsector*16384;
				break;
			default:
				totalsize=0;
				break;
			}
	

			if(RAW_libIsValidFormat(flopemu,&temp_rawfileconfig)==LOADER_ISVALID &&
			   (temp_rawfileconfig.bitrate<=1000000) && (temp_rawfileconfig.bitrate>=50000) &&
			   (temp_rawfileconfig.rpm<=600) && (temp_rawfileconfig.rpm>=50) &&
			   (GetDlgItemInt(hwndDlg,IDC_NUMBEROFTRACK,NULL,0)<=256) && (GetDlgItemInt(hwndDlg,IDC_NUMBEROFTRACK,NULL,0)>=1) &&
			   (GetDlgItemInt(hwndDlg,IDC_SECTORPERTRACK,NULL,0)<=255) && (GetDlgItemInt(hwndDlg,IDC_SECTORPERTRACK,NULL,0)>=1)
			   )
			{
				EnableWindow(GetDlgItem(hwndDlg, IDLOADRAWFILE),TRUE);
				EnableWindow(GetDlgItem(hwndDlg, IDSAVECONFIG),TRUE);
				EnableWindow(GetDlgItem(hwndDlg, ID_MAKEEMPTYFLOPPY),TRUE);	
			}
			else
			{
				EnableWindow(GetDlgItem(hwndDlg, IDLOADRAWFILE),FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDSAVECONFIG),FALSE);
				EnableWindow(GetDlgItem(hwndDlg, ID_MAKEEMPTYFLOPPY),FALSE);
			}


			//new_bitrate=(int)(float)(((float)temp_rawfileconfig.rpm*(float)old_bitrate)/(float)old_rpm);
			//if(	new_bitrate !=	old_bitrate)
			//SetDlgItemInt(hwndDlg,IDC_BITRATE,new_bitrate,0);

			//old_bitrate=new_bitrate;
			//old_rpm=temp_rawfileconfig.rpm;



			SetDlgItemInt(hwndDlg,IDC_NBOFSECTORS,totalsector,0);
			SetDlgItemInt(hwndDlg,IDC_TOTALSIZE,totalsize,0);
		break;

		case WM_INITDIALOG:
			if(nbinstance!=0)
			{
				DestroyWindow(hwndDlg);
			}
			else
			{
				
				nbinstance=1;
				SetDlgItemInt(hwndDlg,IDC_NUMBEROFTRACK,rawfileconfig.numberoftrack,0);
				SetDlgItemInt(hwndDlg,IDC_RPM,rawfileconfig.rpm,0);
				SetDlgItemInt(hwndDlg,IDC_BITRATE,rawfileconfig.bitrate,0);
				SetDlgItemInt(hwndDlg,IDC_GAP3LEN,rawfileconfig.gap3,0);
				SetDlgItemInt(hwndDlg,IDC_SECTORPERTRACK,rawfileconfig.sectorpertrack,0);
				SetDlgItemInt(hwndDlg,IDC_INTERLEAVE,rawfileconfig.interleave,0);
				SetDlgItemInt(hwndDlg,IDC_SKEW,rawfileconfig.skew,0);
				SetDlgItemInt(hwndDlg,IDC_SECTORIDSTART,rawfileconfig.firstidsector,0);
				
				old_bitrate=rawfileconfig.bitrate;
				old_rpm=rawfileconfig.rpm;

				SendDlgItemMessage(hwndDlg,IDC_SIDE1ENABLE,BM_SETCHECK,rawfileconfig.sidecfg&TWOSIDESFLOPPY?BST_CHECKED:BST_UNCHECKED,0);
				SendDlgItemMessage(hwndDlg,IDC_REVERSESIDE,BM_SETCHECK,rawfileconfig.sidecfg&SIDE_INVERTED?BST_CHECKED:BST_UNCHECKED,0);
				SendDlgItemMessage(hwndDlg,IDC_SIDE0FIRST ,BM_SETCHECK,rawfileconfig.sidecfg&SIDE0_FIRST?BST_CHECKED:BST_UNCHECKED,0);
						

				i=0;
				do
				{
					SendDlgItemMessage(hwndDlg,IDC_TRACKTYPE, CB_ADDSTRING, 0, (LPARAM)track_type_list[i].name);
					i++;
				}while(track_type_list[i].id!=-1);

				SendDlgItemMessage(hwndDlg, IDC_TRACKTYPE, CB_SETCURSEL, rawfileconfig.tracktype, 0);

				i=0;
				do
				{
					SendDlgItemMessage(hwndDlg,IDC_SECTORSIZE, CB_ADDSTRING, 0, (LPARAM)sectorsize_type_list[i].name);
					i++;
				}while(sectorsize_type_list[i].id!=-1);

				SendDlgItemMessage(hwndDlg, IDC_SECTORSIZE, CB_SETCURSEL, rawfileconfig.sectorsize, 0);
			

				SetTimer(hwndDlg,34,250,NULL);
			}
			break;
			
		case WM_CLOSE:
			nbinstance=0;
			KillTimer(hwndDlg,34);
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
