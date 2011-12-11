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
// File : Dialog_floppydump.c
// Contains: Floppy Emulator Project
//
// Written by:	DEL NERO Jean Francois
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>

#include "resource.h"
#include "mainrouts.h"
#include "Dialog_config.h"
#include "hxc_floppy_emulator.h"
#include "fdrawcmd.h"

extern HXCFLOPPYEMULATOR * flopemu;

static BITMAPINFO * bmapinfo;
unsigned int xsize,ysize;
static RECT myRect;
unsigned char * mapfloppybuffer;

typedef struct floppydumperparams_
{
	HXCFLOPPYEMULATOR * flopemu;
		
	FLOPPY * floppydisk;

	HWND windowshwd;
	int drive;
	int number_of_track;
	int number_of_side;
	int double_step;
	int status;
	int retry;
}floppydumperparams;

typedef struct trackmode_
{
	int index;
	unsigned int bitrate;
	unsigned char bitrate_code;
	unsigned char encoding_mode; // 0 Fm other MFM
}trackmode;


static floppydumperparams fdparams;

trackmode tm[]=
{
	{0,500000,0,0xFF},
	{1,300000,1,0xFF},
	{2,250000,2,0xFF},

	{3,500000,0,0x00},
	{4,300000,1,0x00},
	{5,250000,2,0x00},

	{7,1000000,3,0x00},
	{6,1000000,3,0xFF}

};


static int checkversion(void)
{
	DWORD version = 0;
	DWORD ret;
	HANDLE h;

	h = CreateFile("\\\\.\\fdrawcmd", GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (h == INVALID_HANDLE_VALUE)
		return 0;
	DeviceIoControl(h, IOCTL_FDRAWCMD_GET_VERSION, NULL, 0, &version, sizeof version, &ret, NULL);
	CloseHandle(h);
	if (!version) {
		return 0;
	}
	if (HIWORD(version) != HIWORD(FDRAWCMD_VERSION)) {
		return 0;
	}
	return version;
}

static void closedevice(HANDLE h)
{
	DWORD ret;
	
	if (h == INVALID_HANDLE_VALUE)
		return;

	if (!DeviceIoControl(h, IOCTL_FD_UNLOCK_FDC, 0,0, 0, 0, &ret, NULL)) {
		printf("IOCTL_FD_UNLOCK_FDC failed err=%d\n", GetLastError());
	}

	CloseHandle(h);
}

HANDLE opendevice(int drive)
{
	HANDLE h;
	DWORD ret;
	BYTE b;

	char drv_name[512];

	sprintf(drv_name,"\\\\.\\fdraw%d",drive);

	h = CreateFile(drv_name, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (h == INVALID_HANDLE_VALUE)
		return 0;


	b = 2; // 250Kbps
	if (!DeviceIoControl(h, IOCTL_FD_SET_DATA_RATE, &b, sizeof b, NULL, 0, &ret, NULL)) {
		printf("IOCTL_FD_SET_DATA_RATE=%d failed err=%d\n", b, GetLastError());
		closedevice(h);
		return 0;
	}

	b = 0;
	if (!DeviceIoControl(h, IOCTL_FD_SET_DISK_CHECK, &b, sizeof b, NULL, 0, &ret, NULL)) {
		printf("IOCTL_FD_SET_DISK_CHECK=%d failed err=%d\n", b, GetLastError());
		closedevice(h);
		return 0;
	}

	if (!DeviceIoControl(h, IOCTL_FD_LOCK_FDC, 0,0, 0, 0, &ret, NULL)) {
		printf("IOCTL_FD_LOCK_FDC=%d failed err=%d\n", b, GetLastError());
		closedevice(h);
		return 0;
	}

	return h;
}

static int seek(HANDLE h,int cyl, int head)
{
	FD_SEEK_PARAMS sp;
	DWORD ret;

	sp.cyl = cyl;
	sp.head = head;
	if (!DeviceIoControl(h, IOCTL_FDCMD_SEEK, &sp, sizeof sp, NULL, 0, &ret, NULL)) {
		printf("IOCTL_FDCMD_SEEK failed cyl=%d, err=%d\n", sp.cyl, GetLastError());
		return 0;
	}
	return 1;
}

void splashscreen(HWND  hwndDlg,unsigned char * buffer)
{
	HDC hdc;	

	hdc=GetDC(hwndDlg);
	GetClientRect(hwndDlg,&myRect);
	StretchDIBits(hdc,16,210,xsize,ysize,0,0,xsize,ysize,buffer,bmapinfo,0,SRCCOPY);
	ReleaseDC(hwndDlg,hdc);
}

DWORD WINAPI DumpThreadProc( LPVOID lpParameter)
{
	unsigned int k,m,n,ret;
	int i,j;
	int l,o,p;
	FD_SCAN_PARAMS sp;
	FD_SCAN_RESULT *sr;
	FD_READ_WRITE_PARAMS rwp;
	FD_CMD_RESULT cmdr;
	HXCFLOPPYEMULATOR *hxcfe;
	FBuilder* fb;
	floppydumperparams * params;
	unsigned char * tempstr;
	unsigned char b;
	int retry;
	HANDLE h;
	unsigned char trackformat;
	unsigned short rpm;
	unsigned long rotationtime,bitrate;
	unsigned long total_size,numberofsector_read,number_of_bad_sector;
	unsigned char * sector_data;
	int sectordata_size;
	
	params=(floppydumperparams*)lpParameter;
	
	hxcfe=params->flopemu;

	hxcfe->hxc_printf(MSG_DEBUG,"Starting Floppy dump...");
	
	tempstr=(char*)malloc(1024);
	
	if(checkversion())
	{
		h=opendevice(params->drive);
		if(h)
		{

			total_size=0;
			numberofsector_read=0;
			number_of_bad_sector=0;

			sr=malloc(sizeof(FD_ID_HEADER)*256 + 1);
						
			fb=hxcfe_initFloppy(hxcfe,params->number_of_track,params->number_of_side);
			hxcfe_setSectorFill(fb,0xF6);
			hxcfe_setIndexPosition(fb,-2500,0);
			hxcfe_setIndexLength(fb,2500);

			seek(h,0, 0);

			sprintf(tempstr,"Checking drive RPM...");
			SetDlgItemText(params->windowshwd,IDC_EDIT1,tempstr);
					
			rotationtime=200000;
			if (!DeviceIoControl(h, IOCTL_FD_GET_TRACK_TIME, 0, 0, &rotationtime, sizeof(rotationtime), &ret, NULL))
			{
				sprintf(tempstr,"Error during RPM checking :%d ",GetLastError());
				SetDlgItemText(params->windowshwd,IDC_EDIT1,tempstr);
				hxcfe->hxc_printf(MSG_DEBUG,"Leaving Floppy dump: IOCTL_FD_GET_TRACK_TIME error %d...",GetLastError());
				closedevice(h);
				free(tempstr);

				params->status=0;
				return 0;
			}
				
			rpm=(unsigned short)(60000/(rotationtime/1000));
			
			hxcfe->hxc_printf(MSG_DEBUG,"Drive RPM: %d",rpm);
			sprintf(tempstr,"Drive RPM: %d",rpm);
			SetDlgItemText(params->windowshwd,IDC_EDIT1,tempstr);
			
			if(rpm>280 && rpm<320) rpm=300;
			if(rpm>340 && rpm<380) rpm=360;
			
			bitrate=500000;
			
			sprintf(tempstr,"Starting reading disk...");
			SetDlgItemText(params->windowshwd,IDC_EDIT1,tempstr);

			for(i=0;i<params->number_of_track;i++)
			{

				if(i<100)
				{
					for(j=0;j<params->number_of_side;j++)
					{

						l=0;
						o=i*xsize*4;
						if(j)
						{
							o=o+xsize*4*100;
						}
				
						l=xsize;
						while(l)
						{
							mapfloppybuffer[o++]=0x55;
							mapfloppybuffer[o++]=0x55;
							mapfloppybuffer[o]=0x55;
							if(i&1)
								mapfloppybuffer[o]=0xA5;
							o++;

							o++;	
							l--;
						}
					}
				}
			}

			for(i=0;i<params->number_of_track;i++)
			{
				
				for(j=0;j<params->number_of_side;j++)
				{
					
					seek(h,i*params->double_step, j);

										
					l=0;
					do
					{
						m=0;
						while(tm[m].index!=l)
						{
							m++;
						}
						
						memset(sr,0,sizeof(FD_ID_HEADER)*256 + 1);
						
						if(tm[m].encoding_mode)
						{
							sp.flags=FD_OPTION_MFM;
							trackformat=IBMFORMAT_DD;
						}
						else
						{
							sp.flags=0x00;
							trackformat=IBMFORMAT_SD;
						}
						
						sp.head=j;
						
						bitrate=tm[m].bitrate;
						b = tm[m].bitrate_code;
						if (!DeviceIoControl(h, IOCTL_FD_SET_DATA_RATE, &b, sizeof b, NULL, 0, &ret, NULL)) 
						{
							printf("IOCTL_FD_SET_DATA_RATE=%d failed err=%d\n", b, GetLastError());
							closedevice(h);
							params->status=0;
							return 0;
						}
						
						if (!DeviceIoControl(h, IOCTL_FD_SCAN_TRACK, &sp, sizeof sp, sr, sizeof(FD_ID_HEADER)*256 + 1, &ret, NULL))
						{
							hxcfe->hxc_printf(MSG_DEBUG,"IOCTL_FD_SCAN_TRACK error %d ...",GetLastError());
						}
						
						if(sr->count)
						{
							n=0;
							while(tm[n].index)
							{
								n++;
							}
							
							k=tm[n].index;
							tm[n].index=tm[m].index;		
							tm[m].index=k;
						}
						else
						{
							DeviceIoControl(h, IOCTL_FD_RESET, NULL, 0, NULL, 0, &ret, NULL);
						}
						
						l++;
					}while(l<8 && !sr->count);
					
					
					hxcfe->hxc_printf(MSG_DEBUG,"Track %d side %d: %d sectors found : ",i,j,sr->count);
					
					seek(h,i*params->double_step, j);
					
					if( sr->Header[0].cyl==sr->Header[sr->count-1].cyl && 
						sr->Header[0].head==sr->Header[sr->count-1].head && 
						sr->Header[0].sector==sr->Header[sr->count-1].sector && 
						sr->Header[0].size==sr->Header[sr->count-1].size 
						)
					{
						sr->count--;
					}

					if(i<100)
					{
						l=0;
						o=i*xsize*4;
						if(j)
						{
							o=o+xsize*4*100;
						}
						for(k=0;k<sr->count;k++)
						{
							l=4<<(sr->Header[k].size);
							while(l)
							{
								mapfloppybuffer[o++]=0xFF;
								mapfloppybuffer[o++]=0xFF;
								mapfloppybuffer[o++]=0xFF;
								o++;	
								l--;
							}
							o=o+8;
						}

						l=0;
						o=i*xsize*4;
						if(j)
						{
							o=o+xsize*4*100;
						}
					}

					hxcfe_pushTrack (fb,rpm,i,j,trackformat);
					hxcfe_setTrackBitrate(fb,bitrate);


					for(k=0;k<sr->count;k++)
					{
						hxcfe->hxc_printf(MSG_DEBUG,"Sector %.2d, Track ID: %.3d, Head ID:%d, Size: %d bytes, Bitrate:%dkbits/s, %s",sr->Header[k].sector,sr->Header[k].cyl,sr->Header[k].head,128<<sr->Header[k].size,bitrate/1000,trackformat==IBMFORMAT_SD?"FM":"MFM");

						if(tm[m].encoding_mode)
						{
							rwp.flags=FD_OPTION_MFM;
						}
						else
						{
							rwp.flags=0x00;
						}
						rwp.cyl=sr->Header[k].cyl;
						rwp.phead=j;
						rwp.head=sr->Header[k].head;
						rwp.sector=sr->Header[k].sector;
						rwp.size=sr->Header[k].size;
						rwp.eot=sr->Header[k].sector+1;
						rwp.gap=10;
						if((128<<sr->Header[k].size)>128)
						{
							rwp.datalen=255;
						}
						else
						{
							rwp.datalen=128;
						}
						
						sectordata_size=(128<<(sr->Header[k].size&0x7));
						sector_data=malloc(sectordata_size);
						memset(sector_data,0,sectordata_size);

						p=o;
						retry=params->retry;
						do
						{
							retry--;

							if(i<100)
							{
								o=p;
								l=4<<(sr->Header[k].size);
								while(l)
								{
									mapfloppybuffer[o++]=0x00;
									mapfloppybuffer[o++]=0xFF;
									mapfloppybuffer[o++]=0xFF;
									o++;	
									l--;
								}
								o=o+8;
							}

						}while(!DeviceIoControl(h, IOCTL_FDCMD_READ_DATA, &rwp, sizeof rwp,sector_data, sectordata_size, &ret, NULL) && retry);

						hxcfe_setSectorBitrate(fb,bitrate);
						hxcfe_setSectorGap3(fb,255);
						hxcfe_setSectorEncoding(fb,trackformat);
						hxcfe_addSector(fb,
										sr->Header[k].sector,
										sr->Header[k].head,
										sr->Header[k].cyl,
										sector_data,sectordata_size);

						free(sector_data);

						if(!retry)
						{	
							DeviceIoControl(h, IOCTL_FD_GET_RESULT,0, 0, &cmdr, sizeof(FD_CMD_RESULT), &ret, NULL);
							hxcfe->hxc_printf(MSG_DEBUG,"Read Error ! ST0: %.2x, ST1: %.2x, ST2: %.2x",cmdr.st0,cmdr.st1,cmdr.st2);
							number_of_bad_sector++;

							o=p;
							l=4<<(sr->Header[k].size);
							while(l)
							{
								mapfloppybuffer[o++]=0x00;
								mapfloppybuffer[o++]=0x00;
								mapfloppybuffer[o++]=0xFF;
								o++;	
								l--;
							}
							o=o+8;

						}
						else
						{
							if(i<100)
							{

								o=p;
								l=4<<(sr->Header[k].size);
								while(l)
								{
									mapfloppybuffer[o++]=0x00;
									mapfloppybuffer[o++]=0xFF;
									mapfloppybuffer[o++]=0x00;
									o++;	
									l--;
								}
								o=o+8;
							}
						}
					
						
						total_size=total_size+sectordata_size;
						numberofsector_read++;

						sprintf(tempstr,"%d Sector(s) Read, %d bytes, %d Bad sector(s)",numberofsector_read,total_size,number_of_bad_sector);
						SetDlgItemText(params->windowshwd,IDC_EDIT2,tempstr);

					}
				
					hxcfe_popTrack(fb);

					sprintf(tempstr,"Track %d side %d: %d sectors, %dkbits/s, %s",i,j,sr->count,bitrate/1000,trackformat==ISOFORMAT_SD?"FM":"MFM");
					SetDlgItemText(params->windowshwd,IDC_EDIT1,tempstr);

				}
			}

			closedevice(h);

			params->floppydisk=hxcfe_getFloppy(fb);

			sprintf(tempstr,"Done !");
			SetDlgItemText(params->windowshwd,IDC_EDIT1,tempstr);
			sprintf(tempstr,"%d Sector(s) Read, %d bytes, %d Bad sector(s)",numberofsector_read,total_size,number_of_bad_sector);
			SetDlgItemText(params->windowshwd,IDC_EDIT2,tempstr);

			hxcfe->hxc_printf(MSG_DEBUG,"Done ! %d sectors read, %d bad sector(s), %d bytes read",numberofsector_read,number_of_bad_sector,total_size);

			free(tempstr);

			hxcfe->hxc_printf(MSG_DEBUG,"Leaving Floppy dump...");

			params->status=0;
			return 0;
		}
	}

	sprintf(tempstr,"Error while opening fdrawcmd, see: http://simonowen.com/fdrawcmd");
	SetDlgItemText(params->windowshwd,IDC_EDIT1,tempstr);

	free(tempstr);

	hxcfe->hxc_printf(MSG_DEBUG,"Leaving Floppy dump...");

	params->status=0;
	return 0;	
}


////////////////////////////////////////////////////////////////////////// 
//
//  
//////////////////////////////////////////////////////////////////////////
BOOL CALLBACK DialogFloppyDump(
						  HWND  hwndDlg,	// handle of dialog box
						  UINT  message,	// message
						  WPARAM  wParam,	// first message parameter
						  LPARAM  lParam 	// second message parameter
						  )
{
	static char nbinstance=0;
	char tempstr[128];
	int wmId, wmEvent;
	DWORD sit;

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

			case IDOK:
				if(!fdparams.status)
				{
					//KillTimer(hwndDlg,34);
					nbinstance=0;
					DestroyWindow(hwndDlg);
				}
			break;

			
			case IDLOAD_READDISK:
				if(!fdparams.status)
				{
					fdparams.floppydisk=(FLOPPY*)malloc(sizeof(FLOPPY));
					memset(fdparams.floppydisk,0,sizeof(FLOPPY));

					fdparams.windowshwd=hwndDlg;
					fdparams.flopemu=flopemu;
					fdparams.double_step=1;
					fdparams.number_of_side=1;
					fdparams.number_of_track=GetDlgItemInt(hwndDlg,IDC_NUMBEROFTRACK_DUMP,NULL,0);
					fdparams.retry=GetDlgItemInt(hwndDlg,IDC_RETRY,NULL,0);
					fdparams.status=1;

					if(SendDlgItemMessage(hwndDlg,IDC_RADIO1,BM_GETCHECK,0,0))
						fdparams.drive=0;
					if(SendDlgItemMessage(hwndDlg,IDC_RADIO2,BM_GETCHECK,0,0))
						fdparams.drive=1;

					if(SendDlgItemMessage(hwndDlg,IDC_DOUBLESIDE_DUMP,BM_GETCHECK,0,0))
					{
						fdparams.number_of_side=2;
					}

					if(SendDlgItemMessage(hwndDlg,IDC_DOUBLESTEP_DUMP,BM_GETCHECK,0,0))
					{
						fdparams.double_step=2;
					}

					sprintf(tempstr,"%d Sector(s) Read, %d bytes, %d Bad sector(s)",0,0,0);
					SetDlgItemText(hwndDlg,IDC_EDIT2,tempstr);

					CreateThread(NULL,0,&DumpThreadProc,&fdparams,0,&sit);

				}
			break;

			
			default:;
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
				xsize=460;
				ysize=200;

				mapfloppybuffer=malloc(xsize*ysize*4);
				memset(mapfloppybuffer,0,xsize*ysize*4);

				bmapinfo = (BITMAPINFO *) malloc((sizeof(BITMAPINFO) + sizeof(RGBQUAD) * 255));
				memset(bmapinfo,0,(sizeof(BITMAPINFO) + sizeof(RGBQUAD) * 255));
				bmapinfo->bmiHeader.biSize=sizeof(bmapinfo->bmiHeader);
				bmapinfo->bmiHeader.biWidth=xsize;
				bmapinfo->bmiHeader.biHeight=-(long)(ysize);
				bmapinfo->bmiHeader.biPlanes=1;
				bmapinfo->bmiHeader.biBitCount=32;
				bmapinfo->bmiHeader.biCompression=BI_RGB;

				SendDlgItemMessage(hwndDlg,IDC_DOUBLESIDE_DUMP,BM_SETCHECK,BST_CHECKED,0);
				SendDlgItemMessage(hwndDlg,IDC_RADIO1,BM_SETCHECK,BST_CHECKED,0);
				SetDlgItemInt(hwndDlg,IDC_NUMBEROFTRACK_DUMP,80,0);
				SetDlgItemInt(hwndDlg,IDC_RETRY,10,0);
				memset(&fdparams,0,sizeof(fdparams));
				sprintf(tempstr,"%d Sector(s) Read, %d bytes, %d Bad sector(s)",0,0,0);
				SetDlgItemText(hwndDlg,IDC_EDIT2,tempstr);
				
				splashscreen(hwndDlg,mapfloppybuffer);

				SetTimer(hwndDlg,33,250,NULL);
			}
			break;
			
		case WM_CLOSE:
			if(!fdparams.status)
			{
				KillTimer(hwndDlg,33);
				free(mapfloppybuffer);
				nbinstance=0;

				DestroyWindow(hwndDlg);
			}
			break;
			
		case WM_TIMER:
			splashscreen(hwndDlg,mapfloppybuffer);
		break;
			
		
			
		default:
			return FALSE;
			
	}
	
	return TRUE;
}
