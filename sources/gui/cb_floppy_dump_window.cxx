/*
//
// Copyright (C) 2006-2016 Jean-François DEL NERO
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
// File : cb_floppy_dump_window.cxx
// Contains: Floppy dump window
//
// Written by:	DEL NERO Jean Francois
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include "floppy_dump_window.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>

#include "fl_includes.h"

#include "libhxcfe.h"
#include "usb_hxcfloppyemulator.h"
#include "libhxcadaptor.h"

#ifdef WIN32
extern "C"
{
	#include "thirdpartylibs/fdrawcmd/fdrawcmd.h"
}
#endif


#include "loader.h"
#include "main.h"
#include "log_gui.h"

extern s_gui_context * guicontext;

typedef struct floppydumperparams_
{
	HXCFE * flopemu;

	HXCFE_FLOPPY * floppydisk;

	floppy_dump_window *windowshwd;
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

floppydumperparams fdp;


void tick_dump(void *w) {
	floppy_dump_window *window;

	window=(floppy_dump_window *)w;

	if(window->window->shown())
	{
		window->window->make_current();
		fl_draw_image(guicontext->mapfloppybuffer, 8, 210, 460, 200, 3, 0);
	}

	Fl::repeat_timeout(0.02, tick_dump, w);
}

static int checkversion(void)
{
#ifdef WIN32
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
#else
    return 0;
#endif

}

#ifdef WIN32
static void closedevice(HANDLE h)
{

	DWORD ret;

	if (h == INVALID_HANDLE_VALUE)
		return;

	if (!DeviceIoControl(h, IOCTL_FD_UNLOCK_FDC, 0,0, 0, 0, &ret, NULL)) {
		CUI_affiche(MSG_DEBUG,"IOCTL_FD_UNLOCK_FDC failed err=%d\n", GetLastError());
	}

	CloseHandle(h);

}
#endif

#ifdef WIN32
HANDLE opendevice(int drive)
{

	HANDLE h;
	DWORD ret;
	BYTE b;
	FD_SPECIFY_PARAMS specparams;

	char drv_name[512];

	sprintf(drv_name,"\\\\.\\fdraw%d",drive);

	h = CreateFile(drv_name, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (h == INVALID_HANDLE_VALUE)
		return 0;

	specparams.srt_hut = 0x0F;
	specparams.hlt_nd = 0xFE;
	if (!DeviceIoControl(h, IOCTL_FDCMD_SPECIFY, &specparams, sizeof(FD_SPECIFY_PARAMS), NULL, 0, &ret, NULL)) {
		CUI_affiche(MSG_DEBUG,"IOCTL_FDCMD_SPECIFY failed err=%d\n", GetLastError());
		closedevice(h);
		return 0;
	}

	b = 2; // 250Kbps
	if (!DeviceIoControl(h, IOCTL_FD_SET_DATA_RATE, &b, sizeof b, NULL, 0, &ret, NULL)) {
		CUI_affiche(MSG_DEBUG,"IOCTL_FD_SET_DATA_RATE=%d failed err=%d\n", b, GetLastError());
		closedevice(h);
		return 0;
	}

	b = 0;
	if (!DeviceIoControl(h, IOCTL_FD_SET_DISK_CHECK, &b, sizeof b, NULL, 0, &ret, NULL)) {
		CUI_affiche(MSG_DEBUG,"IOCTL_FD_SET_DISK_CHECK=%d failed err=%d\n", b, GetLastError());
		closedevice(h);
		return 0;
	}

	if (!DeviceIoControl(h, IOCTL_FD_LOCK_FDC, 0,0, 0, 0, &ret, NULL)) {
		CUI_affiche(MSG_DEBUG,"IOCTL_FD_LOCK_FDC=%d failed err=%d\n", b, GetLastError());
		closedevice(h);
		return 0;
	}

	return h;
}
#endif

#ifdef WIN32
static int seek(HANDLE h,int cyl, int head)
{

	FD_SEEK_PARAMS sp;
	DWORD ret;

	sp.cyl = cyl;
	sp.head = head;
	if (!DeviceIoControl(h, IOCTL_FDCMD_SEEK, &sp, sizeof sp, NULL, 0, &ret, NULL)) {
		CUI_affiche(MSG_DEBUG,"IOCTL_FDCMD_SEEK failed cyl=%d, err=%d\n", sp.cyl, GetLastError());
		return 0;
	}
	return 1;
}
#endif

int DumpThreadProc(void* floppycontext,void* hw_context)//( LPVOID lpParameter)
{
#ifdef WIN32
	int xsize,ysize;

	unsigned int k,m,n;
	unsigned long ret;
	int i,j;
	int l,o,p;
	FD_SCAN_PARAMS sp;
	FD_SCAN_RESULT *sr;
	FD_READ_WRITE_PARAMS rwp;
	FD_CMD_RESULT cmdr;
	HXCFE *hxcfe;
	HXCFE_FLPGEN* fb;
	floppydumperparams * params;
	char * tempstr;
	unsigned char b;
	int retry;
	HANDLE h;
	unsigned char trackformat;
	unsigned short rpm;
	unsigned long rotationtime,bitrate;
	unsigned long total_size,numberofsector_read,number_of_bad_sector;
	unsigned char * sector_data;
	int sectordata_size;

	params=(floppydumperparams*)hw_context;

	hxcfe=params->flopemu;

	xsize=460;
	ysize=200;

	CUI_affiche(MSG_DEBUG,"Starting Floppy dump...");

	tempstr=(char*)malloc(1024);

	if(checkversion())
	{
		h=opendevice(params->drive);
		if(h)
		{

			total_size=0;
			numberofsector_read=0;
			number_of_bad_sector=0;

			sr=(FD_SCAN_RESULT*)malloc(sizeof(FD_ID_HEADER)*256 + 1);

			fb=hxcfe_initFloppy(hxcfe,params->number_of_track,params->number_of_side);
			hxcfe_setSectorFill(fb,0xF6);
			hxcfe_setIndexPosition(fb,-2500,0);
			hxcfe_setIndexLength(fb,2500);

			seek(h,0, 0);

			sprintf(tempstr,"Checking drive RPM...");
			params->windowshwd->global_status->value(tempstr);

			rotationtime=200000;
			if (!DeviceIoControl(h, IOCTL_FD_GET_TRACK_TIME, 0, 0, &rotationtime, sizeof(rotationtime), &ret, NULL))
			{
				sprintf(tempstr,"Error during RPM checking :%d ",GetLastError());
				params->windowshwd->global_status->value(tempstr);

				CUI_affiche(MSG_DEBUG,"Leaving Floppy dump: IOCTL_FD_GET_TRACK_TIME error %d...",GetLastError());
				closedevice(h);
				free(tempstr);

				params->status=0;
				return 0;
			}

			DeviceIoControl(h, IOCTL_FD_MOTOR_OFF, 0, 0, 0, 0, &ret, NULL);

			Sleep(500);

			rpm=(unsigned short)(60000/(rotationtime/1000));

			CUI_affiche(MSG_DEBUG,"Drive RPM: %d",rpm);
			sprintf(tempstr,"Drive RPM: %d",rpm);
			params->windowshwd->global_status->value(tempstr);

			if(rpm>280 && rpm<320) rpm=300;
			if(rpm>340 && rpm<380) rpm=360;

			bitrate=500000;

			sprintf(tempstr,"Starting reading disk...");
			params->windowshwd->global_status->value(tempstr);

			for(i=0;i<params->number_of_track;i++)
			{

				if(i<100)
				{
					for(j=0;j<params->number_of_side;j++)
					{

						l=0;
						o=i*xsize*3;
						if(j)
						{
							o=o+xsize*3*100;
						}

						l=xsize;
						while(l)
						{
							guicontext->mapfloppybuffer[o++]=0x55;
							guicontext->mapfloppybuffer[o++]=0x55;
							guicontext->mapfloppybuffer[o]=0x55;
							if(i&1)
								guicontext->mapfloppybuffer[o]=0xA5;
							o++;

							//o++;
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
							CUI_affiche(MSG_DEBUG,"IOCTL_FD_SET_DATA_RATE=%d failed err=%d\n", b, GetLastError());
							closedevice(h);
							params->status=0;
							return 0;
						}

						if (!DeviceIoControl(h, IOCTL_FD_SCAN_TRACK, &sp, sizeof sp, sr, sizeof(FD_ID_HEADER)*256 + 1, &ret, NULL))
						{
							CUI_affiche(MSG_DEBUG,"IOCTL_FD_SCAN_TRACK error %d ...",GetLastError());
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


					CUI_affiche(MSG_DEBUG,"Track %d side %d: %d sectors found : ",i,j,sr->count);

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
						o=i*xsize*3;
						if(j)
						{
							o=o+xsize*3*100;
						}
						for(k=0;k<sr->count;k++)
						{
							l=4<<(sr->Header[k].size);
							while(l)
							{
								guicontext->mapfloppybuffer[o++]=0xFF;
								guicontext->mapfloppybuffer[o++]=0xFF;
								guicontext->mapfloppybuffer[o++]=0xFF;
								//o++;
								l--;
							}
							o=o+6;
						}

						l=0;
						o=i*xsize*3;
						if(j)
						{
							o=o+xsize*3*100;
						}
					}

					hxcfe_pushTrack (fb,rpm,i,j,trackformat);
					hxcfe_setTrackBitrate(fb,bitrate);


					for(k=0;k<sr->count;k++)
					{
						CUI_affiche(MSG_DEBUG,"Sector %.2d, Track ID: %.3d, Head ID:%d, Size: %d bytes, Bitrate:%dkbits/s, %s",sr->Header[k].sector,sr->Header[k].cyl,sr->Header[k].head,128<<sr->Header[k].size,bitrate/1000,trackformat==IBMFORMAT_SD?"FM":"MFM");

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
						sector_data=(unsigned char*)malloc(sectordata_size);
						memset((unsigned char*)sector_data,0,sectordata_size);

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
									guicontext->mapfloppybuffer[o++]=0xFF;
									guicontext->mapfloppybuffer[o++]=0xFF;
									guicontext->mapfloppybuffer[o++]=0x00;
									//o++;
									l--;
								}
								o=o+6;
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
							CUI_affiche(MSG_DEBUG,"Read Error ! ST0: %.2x, ST1: %.2x, ST2: %.2x",cmdr.st0,cmdr.st1,cmdr.st2);
							number_of_bad_sector++;

							o=p;
							l=4<<(sr->Header[k].size);
							while(l)
							{
								guicontext->mapfloppybuffer[o++]=0xFF;
								guicontext->mapfloppybuffer[o++]=0x00;
								guicontext->mapfloppybuffer[o++]=0x00;
								//o++;
								l--;
							}
							o=o+6;

						}
						else
						{
							if(i<100)
							{

								o=p;
								l=4<<(sr->Header[k].size);
								while(l)
								{
									guicontext->mapfloppybuffer[o++]=0x00;
									guicontext->mapfloppybuffer[o++]=0xFF;
									guicontext->mapfloppybuffer[o++]=0x00;
									//o++;
									l--;
								}
								o=o+6;
							}
						}


						total_size=total_size+sectordata_size;
						numberofsector_read++;

						sprintf(tempstr,"%d Sector(s) Read, %d bytes, %d Bad sector(s)",numberofsector_read,total_size,number_of_bad_sector);
						params->windowshwd->current_status->value(tempstr);

					}

					hxcfe_popTrack(fb);

					sprintf(tempstr,"Track %d side %d: %d sectors, %dkbits/s, %s",i,j,sr->count,bitrate/1000,((trackformat==ISOFORMAT_SD) || (trackformat==IBMFORMAT_SD) )?"FM":"MFM");
					params->windowshwd->global_status->value(tempstr);
				}
			}

			closedevice(h);

			params->floppydisk=hxcfe_getFloppy(fb);
			load_floppy(params->floppydisk,"Floppy Dump Image");

			sprintf(tempstr,"Done !");
			params->windowshwd->global_status->value(tempstr);
			sprintf(tempstr,"%d Sector(s) Read, %d bytes, %d Bad sector(s)",numberofsector_read,total_size,number_of_bad_sector);
			params->windowshwd->current_status->value(tempstr);

			CUI_affiche(MSG_DEBUG,"Done ! %d sectors read, %d bad sector(s), %d bytes read",numberofsector_read,number_of_bad_sector,total_size);

			free(tempstr);

			CUI_affiche(MSG_DEBUG,"Leaving Floppy dump...");

			params->status=0;
			return 0;
		}
	}

	sprintf(tempstr,"Error while opening fdrawcmd, see: http://simonowen.com/fdrawcmd");
	params->windowshwd->global_status->value(tempstr);

	free(tempstr);

	CUI_affiche(MSG_DEBUG,"Leaving Floppy dump...");

	params->status=0;
#endif
	return 0;
}


void floppy_dump_window_bt_read(Fl_Button* bt, void*)
{
	char tempstr[512];
	floppy_dump_window *fdw;
	Fl_Window *dw;

	dw=((Fl_Window*)(bt->parent()));
	fdw=(floppy_dump_window *)dw->user_data();

	memset((void*)&fdp,0,sizeof(floppydumperparams));
	fdp.windowshwd=fdw;

	if(fdw->sel_drive_b->value())
	{
		fdp.drive=1;
	}

	fdp.number_of_side=1;
	if(fdw->double_sided->value())
	{
		fdp.number_of_side=2;
	}

	fdp.double_step=1;
	if(fdw->double_step->value())
	{
		fdp.double_step=2;
	}

	fdp.retry=(int)fdw->number_of_retry->value();
	fdp.number_of_track=(int)fdw->number_of_track->value();

	fdp.flopemu=guicontext->hxcfe;

	sprintf(tempstr,"%d Sector(s) Read, %d bytes, %d Bad sector(s)",0,0,0);
	fdw->current_status->value(tempstr);

	//fdw->deactivate();
	hxc_createthread(guicontext->hxcfe,&fdp,&DumpThreadProc,0);
}

void floppy_dump_ok(Fl_Button*, void* w)
{
	floppy_dump_window *fdw;

	fdw=(floppy_dump_window *)w;

	fdw->window->hide();
}

