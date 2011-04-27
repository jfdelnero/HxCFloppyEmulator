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
#include "mainrouts.h"

#include "Dialog_logs.h"

#include "hxc_floppy_emulator.h"

#include "fileselector.h"

extern guicontext * demo;


logfifo logsfifo;
extern CRITICAL_SECTION log_cs;

char * logs_buffer;

int CUI_affiche(int MSGTYPE,char * chaine, ...)
{
	FILE * debugfile;
	char test_temp[LOGSTRINGSIZE];
	
	va_list marker;
	va_start( marker, chaine );     
	
	EnterCriticalSection(&log_cs);
	if(!logsfifo.fifotab[logsfifo.in&(LOGFIFOSIZE-1)])
	{
		logsfifo.fifotab[logsfifo.in&(LOGFIFOSIZE-1)]=malloc(LOGSTRINGSIZE);
		memset(logsfifo.fifotab[logsfifo.in&(LOGFIFOSIZE-1)],0,LOGSTRINGSIZE);
		_vsnprintf(test_temp,LOGSTRINGSIZE,chaine,marker);
		
		switch(MSGTYPE)
		{
			
		case MSG_INFO_0:
			_snprintf(logsfifo.fifotab[logsfifo.in&(LOGFIFOSIZE-1)],LOGSTRINGSIZE,"INFO 0 : %s",test_temp);
			break;
			
		case MSG_INFO_1:
			_snprintf(logsfifo.fifotab[logsfifo.in&(LOGFIFOSIZE-1)],LOGSTRINGSIZE,"INFO 1 : %s",test_temp);
			break;
			
		case MSG_WARNING:
			_snprintf(logsfifo.fifotab[logsfifo.in&(LOGFIFOSIZE-1)],LOGSTRINGSIZE,"WARNING: %s",test_temp);
			break;
			
		case MSG_ERROR:
			_snprintf(logsfifo.fifotab[logsfifo.in&(LOGFIFOSIZE-1)],LOGSTRINGSIZE,"ERROR  : %s",test_temp);
			break;
			
		case MSG_DEBUG:
			_snprintf(logsfifo.fifotab[logsfifo.in&(LOGFIFOSIZE-1)],LOGSTRINGSIZE,"DEBUG  : %s",test_temp);
			break;
			
		default:
			break;
		}
		
		
		logsfifo.in=logsfifo.in+1;
	}
	else
	{
		memset(logsfifo.fifotab[logsfifo.in&(LOGFIFOSIZE-1)],0,LOGSTRINGSIZE);
		_vsnprintf(test_temp,LOGSTRINGSIZE,chaine,marker);
		
		switch(MSGTYPE)
		{
			
		case MSG_INFO_0:
			_snprintf(logsfifo.fifotab[logsfifo.in&(LOGFIFOSIZE-1)],LOGSTRINGSIZE,"INFO 0 : %s",test_temp);
			break;
			
		case MSG_INFO_1:
			_snprintf(logsfifo.fifotab[logsfifo.in&(LOGFIFOSIZE-1)],LOGSTRINGSIZE,"INFO 1 : %s",test_temp);
			break;
			
		case MSG_WARNING:
			_snprintf(logsfifo.fifotab[logsfifo.in&(LOGFIFOSIZE-1)],LOGSTRINGSIZE,"WARNING: %s",test_temp);
			break;
			
		case MSG_ERROR:
			_snprintf(logsfifo.fifotab[logsfifo.in&(LOGFIFOSIZE-1)],LOGSTRINGSIZE,"ERROR  : %s",test_temp);
			break;
			
		case MSG_DEBUG:
			_snprintf(logsfifo.fifotab[logsfifo.in&(LOGFIFOSIZE-1)],LOGSTRINGSIZE,"DEBUG  : %s",test_temp);
			break;
			
		default:
			break;
		}
		
		logsfifo.in=(logsfifo.in+1)&(LOGFIFOSIZE-1);
		logsfifo.out=(logsfifo.out+1)&(LOGFIFOSIZE-1);
		
		
		
	}
	LeaveCriticalSection(&log_cs);
	
	
	if(demo->logfile)
	{
		debugfile=fopen(demo->logfile,"a");
		if(debugfile)
		{
			switch(MSGTYPE)
			{
			case MSG_DEBUG:
				fprintf(debugfile,"DEBUG: ");
				break;
			default:
				break;
			}
			vfprintf(debugfile,chaine,marker);
			fprintf(debugfile,"\n");
			
			fclose(debugfile);		
		}
	}
	
	
	va_end( marker ); 
	
    return 0;
}



////////////////////////////////////////////////////////////////////////// 
//
//  Gestion Boite de dialogue Log
//  
//////////////////////////////////////////////////////////////////////////
BOOL CALLBACK DialogLogs(
						 HWND  hwndDlg,	// handle of dialog box
						 UINT  message,	// message
						 WPARAM  wParam,	// first message parameter
						 LPARAM  lParam 	// second message parameter
						 )
{
	static char nbinstance=0;
	int wmId;//, wmEvent;
	char * logfile,*logfile2;

	char filename[1024];
	int i;
	int line;
	int linecount;
	static int old_index;
	HWND h;
	
	wmId    = LOWORD(wParam);
	switch (message) 
	{
		
	case WM_COMMAND:
		switch (wmId)
		{
			
		case ID_SETLOGFILE:
			
			logfile=demo->logfile;
			memset(filename,0,sizeof(filename));
			if(fileselector(hwndDlg,1,0,filename,"log file","*.log","log",0,0))
			{
				logfile2=malloc(strlen(filename)+1);
				memcpy(logfile2,filename,strlen(filename)+1);
				demo->logfile=logfile2;
				if(logfile) free(logfile);
			}
			
			break;
		case ID_CLOSE:
			nbinstance=0;
			DestroyWindow(hwndDlg);
			break;
			
		default:;
		}
		break;
		
		
		case WM_INITDIALOG:
			old_index=0;
			if(nbinstance!=0)
			{
				DestroyWindow(hwndDlg);
			}
			else
			{
				nbinstance=1;
				logs_buffer=malloc(LOGFIFOSIZE*LOGSTRINGSIZE);
				if(logs_buffer)
				{
					memset(logs_buffer,0,LOGFIFOSIZE*LOGSTRINGSIZE);
					SetTimer(hwndDlg,37,100,NULL);
				}
				else
				{
					nbinstance=0;
					DestroyWindow(hwndDlg);
				}
			}
			

			break;
			
			
		case WM_CLOSE:
			KillTimer(hwndDlg,37);
			if(logs_buffer) free(logs_buffer);
			logs_buffer=0;
			nbinstance=0;
			DestroyWindow(hwndDlg);
			break;
			
			
		case WM_TIMER:
			if(logsfifo.in!=old_index)
			{

				EnterCriticalSection(&log_cs);

				old_index=logsfifo.in;
			
				if(logs_buffer)
				{
					logs_buffer[0]=0;
					i=logsfifo.out;
					do
					{	
						if(logsfifo.fifotab[i&(LOGFIFOSIZE-1)])
						{
							strcat(logs_buffer,logsfifo.fifotab[i&(LOGFIFOSIZE-1)]);
							strcat(logs_buffer,"\r\n");
						}
						i=(i+1)&(LOGFIFOSIZE-1);
					}while(logsfifo.in!=i);
					
					SetDlgItemText( hwndDlg, IDC_LOGS/*GetDlgItem(hwndDlg, IDC_LOGS)*/,logs_buffer);  
					
					h=(HWND)GetDlgItem(hwndDlg, IDC_LOGS);
					line = SendMessage(h, EM_GETFIRSTVISIBLELINE, 0, 0);
					linecount = SendMessage(h, EM_GETLINECOUNT, 0, 0);
					SendMessage(h, EM_LINESCROLL, 0, (linecount - line - 2));
				}


				LeaveCriticalSection(&log_cs);
			}
			
			break;
			
		default:
			return FALSE;
			
	}
	
	return TRUE;
}


