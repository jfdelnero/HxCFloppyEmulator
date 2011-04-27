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
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <direct.h>

#include <time.h>

#include <shlobj.h>

#include "resource.h"
#include "mainrouts.h"

#include "Dialog_CreateFileSystem.h"

#include "hxc_floppy_emulator.h"
#include "internal_floppy.h"
#include "floppy_loader.h"
#include "./usb_floppyemulator/usb_hxcfloppyemulator.h"

#include "../../common/plugins/amigadosfs_loader/amigadosfs_loader.h"
#include "../../common/plugins/fat12floppy_loader/fat12floppy_loader.h"


#include "fileselector.h"



#include "mfm_file_writer.h"
#include "hfe_file_writer.h"
#include "afi_file_writer.h"
#include "raw_file_writer.h"
#include "vtrucco_file_writer.h"
#include "imd_file_writer.h"


#include "win32_api.h"

extern HXCFLOPPYEMULATOR * flopemu;
extern guicontext * demo;
extern HWINTERFACE * hwif;

	typedef struct ff_type_
	{
		int id;
		char * name;

	}ff_type;


	typedef struct batchconverterparams_
	{
		HXCFLOPPYEMULATOR * flopemu;
		HWND windowshwd;
		char sourcedir[1024];
		char destdir[1024];
		char **filelist;
		int fileformat;
		unsigned long numberoffileconverted;
		int abort;
	}batchconverterparams;

	enum
	{
		FF_HFE,
		FF_MFM,
		FF_AFI,
		FF_VTR,
		FF_RAW,
		FF_IMD
	};

	ff_type ff_type_list[]=
	{
		{ FF_HFE,"HFE - SDCard HxC Floppy Emulator file format"},
		{ FF_MFM,"MFM - MFM/FM track file format"},
		{ FF_AFI,"AFI - Advanced file image format"},
		{ FF_VTR,"VTR - VTrucco Floppy Emulator file format"},
		{ FF_RAW,"RAW - RAW sectors file format"},
		{ FF_IMD,"IMD - IMD sectors file format"},
		{ -1,""}			
	};


int draganddropconvert(HXCFLOPPYEMULATOR* floppycontext,char ** filelist,char * destfolder,int output_file_format,batchconverterparams * params)
{
	int i,j,filenb,ret;
	char *destinationfile,*tempstr;
	FLOPPY * thefloppydisk;
	filenb=0;
	while(filelist[filenb])
	{		
		thefloppydisk=(FLOPPY*)malloc(sizeof(FLOPPY));
		memset(thefloppydisk,0,sizeof(FLOPPY));

		ret=floppy_load(floppycontext,thefloppydisk,filelist[filenb]);
		if(ret!=LOADER_NOERROR)
		{
			switch(ret)
			{
				case LOADER_UNSUPPORTEDFILE:
					printf("Load error!: Image file not yet supported!\n");
				break;
				case LOADER_FILECORRUPT:
					printf("Load error!: File corrupted ? Read error ?\n");
				break;
				case LOADER_ACCESSERROR:
					printf("Load error!:  Read file error!\n");
				break;
				default:
					printf("Load error! error %d\n",ret);
				break;
			}
		}
		else
		{

			j=strlen(filelist[filenb]);
			while(filelist[filenb][j]!='\\' && j)
			{
				j--;
			}
			if(filelist[filenb][j]=='\\') j++;
			destinationfile=malloc(strlen(&filelist[filenb][j])+strlen(destfolder)+2+99);
			sprintf(destinationfile,"%s\\%s",destfolder,&filelist[filenb][j]);
			i=strlen(destinationfile);
			
			do
			{
				i--;
			}while(i && destinationfile[i]!='.');

			if(i)
			{
				destinationfile[i]='_';
			}
			ret=1;
			switch(output_file_format)
			{
				case FF_HFE:
					strcat(destinationfile,".hfe");
					if(demo->autoselectmode)
						ret=write_HFE_file(flopemu,thefloppydisk,destinationfile,-1,hwif->double_step);
					else
						ret=write_HFE_file(flopemu,thefloppydisk,destinationfile,hwif->interface_mode,hwif->double_step);
				break;
				case FF_AFI:
					strcat(destinationfile,".afi");
					ret=write_AFI_file(flopemu,thefloppydisk,destinationfile);
				break;

				case FF_MFM:
					strcat(destinationfile,".mfm");
					ret=write_MFM_file(flopemu,thefloppydisk,destinationfile);
				break;

				case FF_RAW:
					strcat(destinationfile,".img");
					ret=write_RAW_file(flopemu,thefloppydisk,destinationfile);
				break;
				case FF_VTR:
					strcat(destinationfile,".vtr");
					if(demo->autoselectmode)
						ret=write_HFE_file(flopemu,thefloppydisk,destinationfile,-1,hwif->double_step);
					else
						ret=write_HFE_file(flopemu,thefloppydisk,destinationfile,hwif->interface_mode,hwif->double_step);
				break;
				case FF_IMD:
					strcat(destinationfile,".imd");
					ret=write_IMD_file(flopemu,thefloppydisk,destinationfile);
				break;


			}
						
						
			floppy_unload(floppycontext,thefloppydisk);


			tempstr=(char*)malloc(1024);
						
			i=strlen(destinationfile);
			do
			{
				i--;
			}while(i && destinationfile[i]!='\\');

			if(!ret)
			{
				sprintf(tempstr,"%s created",&destinationfile[i]);
				SetDlgItemText(params->windowshwd,IDC_CONVERTSTATUS,tempstr);
				params->numberoffileconverted++;
			}
			else
			{
				sprintf(tempstr,"Error cannot create %s",&destinationfile[i]);
				SetDlgItemText(params->windowshwd,IDC_CONVERTSTATUS,tempstr);
			}

			free(destinationfile);
			free(tempstr);
				

		}
		
		free(thefloppydisk);

		free(filelist[filenb]);
		filenb++;
	}	

	return 0;
}
int browse_and_convert_directory(HXCFLOPPYEMULATOR* floppycontext,char * folder,char * destfolder,char * file,int output_file_format,batchconverterparams * params)
{
	long hfindfile;
	filefoundinfo FindFileData;
	int bbool;
	int ret,i;
	char * destinationfolder;
	char * destinationfile;
	FLOPPY * thefloppydisk;
	unsigned char * fullpath;
	unsigned char * tempstr;


	hfindfile=find_first_file(folder,file, &FindFileData); 
	if(hfindfile!=-1)
	{
		bbool=1;
		while(hfindfile!=-1 && bbool && !params->abort)
		{
			if(!params->abort)
			{


			if(FindFileData.isdirectory)
			{
				if(strcmp(".",FindFileData.filename)!=0 && strcmp("..",FindFileData.filename)!=0)
				{
					destinationfolder=malloc(strlen(FindFileData.filename)+strlen(destfolder)+2);
					sprintf(destinationfolder,"%s\\%s",destfolder,FindFileData.filename);

					printf("Creating directory %s\n",destinationfolder);
					mkdir(destinationfolder);

					fullpath=malloc(strlen(FindFileData.filename)+strlen(folder)+2+9);
		            sprintf(fullpath,"%s\\%s",folder,FindFileData.filename);

					floppycontext->hxc_printf(MSG_INFO_1,"Entering directory %s",FindFileData.filename);

					tempstr=(char*)malloc(1024);
					sprintf(tempstr,"Entering directory %s",FindFileData.filename);
					SetDlgItemText(params->windowshwd,IDC_CONVERTSTATUS,tempstr);

					if(browse_and_convert_directory(floppycontext,fullpath,destinationfolder,file,output_file_format,params))
					{
						free(destinationfolder);
						free(fullpath);
						free(tempstr);
						find_close(hfindfile);
						return 1;
					}
					free(destinationfolder);
					free(fullpath);
					floppycontext->hxc_printf(MSG_INFO_1,"Leaving directory %s",FindFileData.filename);
					
					sprintf(tempstr,"Leaving directory %s",FindFileData.filename);
					SetDlgItemText(params->windowshwd,IDC_CONVERTSTATUS,tempstr);
					free(tempstr);
					
				}
			}
			else
			{			
				floppycontext->hxc_printf(MSG_INFO_1,"converting file %s, %dB",FindFileData.filename,FindFileData.size);
				if(FindFileData.size)
				{

					fullpath=malloc(strlen(FindFileData.filename)+strlen(folder)+2+9);
		            sprintf(fullpath,"%s\\%s",folder,FindFileData.filename);

					thefloppydisk=(FLOPPY*)malloc(sizeof(FLOPPY));
					memset(thefloppydisk,0,sizeof(FLOPPY));

					ret=floppy_load(floppycontext,thefloppydisk,fullpath);
		            free(fullpath);
					if(ret!=LOADER_NOERROR)
					{
						switch(ret)
						{
							case LOADER_UNSUPPORTEDFILE:
								printf("Load error!: Image file not yet supported!\n");
							break;
							case LOADER_FILECORRUPT:
								printf("Load error!: File corrupted ? Read error ?\n");
							break;
							case LOADER_ACCESSERROR:
								printf("Load error!:  Read file error!\n");
							break;
							default:
								printf("Load error! error %d\n",ret);
							break;
						}

					}
					else
					{
						destinationfile=malloc(strlen(FindFileData.filename)+strlen(destfolder)+2+99);
					    sprintf(destinationfile,"%s\\%s",destfolder,FindFileData.filename);
						i=strlen(destinationfile);
						do
						{
							i--;
						}while(i && destinationfile[i]!='.');

						if(i)
						{
                          destinationfile[i]='_';
						}
					
					    //printf("Creating file %s\n",destinationfile);

						ret=1;
						switch(output_file_format)
						{
						case FF_HFE:
							strcat(destinationfile,".hfe");
							if(demo->autoselectmode)
							ret=write_HFE_file(flopemu,thefloppydisk,destinationfile,-1,hwif->double_step);
							else
							ret=write_HFE_file(flopemu,thefloppydisk,destinationfile,hwif->interface_mode,hwif->double_step);
							break;

						case FF_AFI:
							strcat(destinationfile,".afi");
							ret=write_AFI_file(flopemu,thefloppydisk,destinationfile);
							break;

						case FF_MFM:
							strcat(destinationfile,".mfm");
							ret=write_MFM_file(flopemu,thefloppydisk,destinationfile);
							break;

						case FF_RAW:
							strcat(destinationfile,".img");
							ret=write_RAW_file(flopemu,thefloppydisk,destinationfile);
							break;

						case FF_VTR:
							strcat(destinationfile,".vtr");
							if(demo->autoselectmode)
							ret=write_vtrucco_file(flopemu,thefloppydisk,destinationfile,-1);
							else
							ret=write_vtrucco_file(flopemu,thefloppydisk,destinationfile,hwif->interface_mode);

							break;
						case FF_IMD:
							strcat(destinationfile,".imd");
							ret=write_IMD_file(flopemu,thefloppydisk,destinationfile);
							break;


						}
						
						
						floppy_unload(floppycontext,thefloppydisk);


						tempstr=(char*)malloc(1024);
						
						i=strlen(destinationfile);
						do
						{
							i--;
						}while(i && destinationfile[i]!='\\');

						if(!ret)
						{
							sprintf(tempstr,"%s created",&destinationfile[i]);
							SetDlgItemText(params->windowshwd,IDC_CONVERTSTATUS,tempstr);
							params->numberoffileconverted++;
						}
						else
						{
							sprintf(tempstr,"Error cannot create %s",&destinationfile[i]);
							SetDlgItemText(params->windowshwd,IDC_CONVERTSTATUS,tempstr);
						}

						free(destinationfile);
						free(tempstr);
				

					}
					free(thefloppydisk);

				}	
			}
			
			bbool=find_next_file(hfindfile,folder,file,&FindFileData);	
			}
		}
		
	}
	else printf("Error FindFirstFile\n");
	
	find_close(hfindfile);
	
	return 0;
}




DWORD WINAPI BatchConverterThreadProc( LPVOID lpParameter)
{
	char * tempstr;
	batchconverterparams * params;

	params=(batchconverterparams*)lpParameter;
	SetThreadPriority(GetCurrentThread(),IDLE_PRIORITY_CLASS);
	
	params->numberoffileconverted=0;
	browse_and_convert_directory(params->flopemu,params->sourcedir,params->destdir,"*.*",params->fileformat,params);

	tempstr=(char*)malloc(1024);
	sprintf(tempstr,"%d files converted!",params->numberoffileconverted);
	SetDlgItemText(params->windowshwd,IDC_CONVERTSTATUS,tempstr);
	free(tempstr);
	EnableWindow(GetDlgItem(params->windowshwd, IDBATCHCONV),TRUE);


	return 0;
	
}

DWORD WINAPI DragandDropConvertThreadProc( LPVOID lpParameter)
{
	char * tempstr;
	batchconverterparams * params;

	params=(batchconverterparams*)lpParameter;
	SetThreadPriority(GetCurrentThread(),IDLE_PRIORITY_CLASS);
	
	params->numberoffileconverted=0;
	draganddropconvert(params->flopemu,params->filelist,params->destdir,params->fileformat,params);

	tempstr=(char*)malloc(1024);
	sprintf(tempstr,"%d files converted!",params->numberoffileconverted);
	SetDlgItemText(params->windowshwd,IDC_CONVERTSTATUS,tempstr);


	EnableWindow(GetDlgItem(params->windowshwd, IDBATCHCONV),TRUE);
	DragAcceptFiles(params->windowshwd,TRUE);

	free(params->filelist);
	free(params);
	
	return 0;
}

////////////////////////////////////////////////////////////////////////// 
//
//  Gestion Boite de dialogue Settings
//  
//////////////////////////////////////////////////////////////////////////
BOOL CALLBACK DialogBatchConvert(
						  HWND  hwndDlg,	// handle of dialog box
						  UINT  message,	// message
						  WPARAM  wParam,	// first message parameter
						  LPARAM  lParam 	// second message parameter
						  )
{
	static char nbinstance=0;
	int wmId, wmEvent;
	DWORD sit;
	int i,nboffile;
	BROWSEINFO binf;
	LPITEMIDLIST Item;
	char foldername[MAX_PATH];
	char filename[1024];
	char ** filelist;

	static batchconverterparams * threadparams;

	wmId    = LOWORD(wParam); 
	wmEvent = HIWORD(wParam); 



	switch (message) 
	{
		
	case WM_DROPFILES:
		nboffile=DragQueryFile((HDROP)wParam, -1, NULL, 0);
		if(nboffile)
		{
			filelist=malloc(sizeof(char*)*(nboffile+1));
			memset(filelist,0,sizeof(char*)*(nboffile+1));
			for(i=0;i<nboffile;i++)
			{
				DragQueryFile((HDROP)wParam, i, filename, 1024);
				filelist[i]=malloc(strlen(filename)+1);
				sprintf(filelist[i],"%s",filename);
			}

			threadparams=(batchconverterparams*)malloc(sizeof(batchconverterparams));
			memset(threadparams,0,sizeof(batchconverterparams));
			threadparams->abort=0;
			threadparams->windowshwd=hwndDlg;
			threadparams->fileformat=SendDlgItemMessage(hwndDlg, IDC_FFTYPE, CB_GETCURSEL, 0, 0);
			threadparams->flopemu=flopemu;
			GetDlgItemText(hwndDlg,IDC_DESTDIR,(char*)threadparams->destdir,1024);
			threadparams->filelist=filelist;
			if(strlen(threadparams->destdir))
			{
				EnableWindow(GetDlgItem(hwndDlg, IDBATCHCONV),FALSE);
				//BatchConverterThreadProc(threadparams);
				DragAcceptFiles(hwndDlg,FALSE);
				CreateThread(NULL,0,&DragandDropConvertThreadProc,threadparams,0,&sit);
			}
		}
		break;

	case WM_COMMAND:
		switch (wmId)
		{
		case IDSELECTSOURCEDIR:
				memset(&binf,0,sizeof(BROWSEINFO));
				binf.hwndOwner=hwndDlg;
				binf.pszDisplayName="Select a source folder";
				binf.lpszTitle="Select a folder to convert";
				binf.pszDisplayName=foldername;
				Item=SHBrowseForFolder(&binf);
				if(Item)
				{
					if(SHGetPathFromIDList(Item,foldername))
					{
						SetDlgItemText(hwndDlg,IDC_SOURCEDIR,foldername);						
					}
				}
			break;

		case IDSELECTDESTINATIONDIR:
				memset(&binf,0,sizeof(BROWSEINFO));
				binf.hwndOwner=hwndDlg;
				binf.pszDisplayName="Select a target folder";
				binf.lpszTitle="Select the target folder";
				binf.pszDisplayName=foldername;
				Item=SHBrowseForFolder(&binf);
				if(Item)
				{
					if(SHGetPathFromIDList(Item,foldername))
					{					
						SetDlgItemText(hwndDlg,IDC_DESTDIR,foldername);
					}
				}
			break;

		case IDBATCHCONV:
				threadparams=(batchconverterparams*)malloc(sizeof(batchconverterparams));
				memset(threadparams,0,sizeof(batchconverterparams));
				threadparams->abort=0;
				threadparams->windowshwd=hwndDlg;
				threadparams->fileformat=SendDlgItemMessage(hwndDlg, IDC_FFTYPE, CB_GETCURSEL, 0, 0);
				threadparams->flopemu=flopemu;
				GetDlgItemText(hwndDlg,IDC_DESTDIR,(char*)threadparams->destdir,1024);
				GetDlgItemText(hwndDlg,IDC_SOURCEDIR,(char*)&threadparams->sourcedir,1024);
				if(strlen(threadparams->sourcedir) && strlen(threadparams->destdir))
				{
					EnableWindow(GetDlgItem(hwndDlg, IDBATCHCONV),FALSE);
					//BatchConverterThreadProc(threadparams);
					CreateThread(NULL,0,&BatchConverterThreadProc,threadparams,0,&sit);
				}
			break;
			
		case IDCANCELRAWFILE:
			if(threadparams)
			threadparams->abort=1;
			nbinstance=0;
			DestroyWindow(hwndDlg);
			break;

		default:
			break;

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
				threadparams=0;
		
				i=0;
				do
				{
					SendDlgItemMessage(hwndDlg,IDC_FFTYPE, CB_ADDSTRING, 0, (LPARAM)ff_type_list[i].name);
					i++;
				}while(ff_type_list[i].id!=-1);

				SendDlgItemMessage(hwndDlg, IDC_FFTYPE, CB_SETCURSEL,0, 0);	
					DragAcceptFiles(hwndDlg,TRUE);
			}
			break;
			
		case WM_CLOSE:
			nbinstance=0;
			KillTimer(hwndDlg,34);
			DestroyWindow(hwndDlg);
			break;
			
		default:
			return FALSE;
			break;
			
	}
	
	return TRUE;
}

////////////////////////////////////////////////////////////////////////// 
////////////////////////////////////////////////////////////////////////// 
////////////////////////////////////////////////////////////////////////// 
////////////////////////////////////////////////////////////////////////// 
