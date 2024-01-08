/*
//
// Copyright (C) 2006-2024 Jean-François DEL NERO
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
// Written by: Jean-François DEL NERO
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

	int start_track;
	int end_track;

	int start_side;
	int end_side;

	int number_of_track;
	int number_of_side;
	int double_step;
	int status;
	int retry;

	int stop;
}floppydumperparams;

typedef struct trackmode_
{
	int index;
	unsigned int bitrate;
	unsigned char bitrate_code;
	unsigned char encoding_mode; // 0 Fm other MFM
	Fl_Check_Button * bt;
}trackmode;

trackmode tm[]=
{
	{0,500000,0,0xFF,NULL},
	{1,300000,1,0xFF,NULL},
	{2,250000,2,0xFF,NULL},

	{3,500000,0,0x00,NULL},
	{4,300000,1,0x00,NULL},
	{5,250000,2,0x00,NULL},

	{7,1000000,3,0x00,NULL},
	{6,1000000,3,0xFF,NULL},

	{-1,0,0,0,NULL}
};

floppydumperparams fdp;

void tick_dump(void *w) {
	floppy_dump_window *window;

	window=(floppy_dump_window *)w;

	if(window->window->shown())
	{
		window->window->make_current();
		fl_draw_image(guicontext->mapfloppybuffer, window->layout_area->x(), window->layout_area->y(), window->layout_area->w(), window->layout_area->h(), 3, 0);
	}

	Fl::repeat_timeout(0.02, tick_dump, w);
}

#ifdef WIN32
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
#endif

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

	snprintf(drv_name,sizeof(drv_name),"\\\\.\\fdraw%d",drive);

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

#define MAX_SECTOR_NB 512

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
	int rotationtime,bitrate;
	int total_size;
	int number_of_bad_sector;
	int numberofsector_read;
	unsigned char * sector_data;
	int sectordata_size;

	params=(floppydumperparams*)hw_context;

	hxcfe=params->flopemu;

	params->status = 1;

	xsize = params->windowshwd->layout_area->w();
	ysize = params->windowshwd->layout_area->h();

	tm[0].bt = params->windowshwd->MFM500;
	tm[1].bt = params->windowshwd->MFM300;
	tm[2].bt = params->windowshwd->MFM250;
	tm[3].bt = params->windowshwd->FM250;
	tm[4].bt = params->windowshwd->FM150;
	tm[5].bt = params->windowshwd->FM125;
	tm[6].bt = params->windowshwd->FM500;
	tm[7].bt = params->windowshwd->MFM1000;

	CUI_affiche(MSG_DEBUG,"Starting Floppy dump...");

	tempstr=(char*)malloc(MAX_TMP_STR_SIZE);
	if(!tempstr)
		return -1;

	if(checkversion())
	{
		h=opendevice(params->drive);
		if(h)
		{
			total_size=0;
			numberofsector_read=0;
			number_of_bad_sector=0;

			sr = (FD_SCAN_RESULT*)malloc(sizeof(FD_ID_HEADER)*MAX_SECTOR_NB + 1);
			if(!sr)
			{
				free(tempstr);
				closedevice(h);
				return -1;
			}

			fb = hxcfe_initFloppy(hxcfe,params->start_track + params->number_of_track,params->end_side+1);
			if(!fb)
			{
				snprintf(tempstr,MAX_TMP_STR_SIZE,"Internal error ! Can't generate the virtual floppy");
				Fl::lock();
				params->windowshwd->global_status->value(tempstr);
				Fl::unlock();

				CUI_affiche(MSG_DEBUG,"Leaving Floppy dump");
				closedevice(h);
				free(tempstr);

				params->status=0;

				return 0;
			}

			hxcfe_setSectorFill(fb,0xF6);
			hxcfe_setIndexPosition(fb,0,-2500,0);
			hxcfe_setIndexLength(fb,0,2500);

			seek(h,0, 0);

			snprintf(tempstr,MAX_TMP_STR_SIZE,"Checking drive RPM...");

			params->windowshwd->global_status->value(tempstr);

			rotationtime=200000;

			if (!DeviceIoControl(h, IOCTL_FD_GET_TRACK_TIME, 0, 0, &rotationtime, sizeof(rotationtime), &ret, NULL))
			{
				snprintf(tempstr,MAX_TMP_STR_SIZE,"Error during RPM checking :%ld ",GetLastError());
				Fl::lock();
				params->windowshwd->global_status->value(tempstr);
				Fl::unlock();

				CUI_affiche(MSG_DEBUG,"Leaving Floppy dump: IOCTL_FD_GET_TRACK_TIME error %d...",GetLastError());

				closedevice(h);

				free(tempstr);

				params->status=0;

				return 0;
			}

			DeviceIoControl(h, IOCTL_FD_MOTOR_OFF, 0, 0, 0, 0, &ret, NULL);

			Sleep(500);

			if(rotationtime < 50000)
			{
				snprintf(tempstr,MAX_TMP_STR_SIZE,"Bad RPM value ! (%d ms period)",rotationtime);
				Fl::lock();
				params->windowshwd->global_status->value(tempstr);
				Fl::unlock();

				CUI_affiche(MSG_DEBUG,"Leaving Floppy dump");
				closedevice(h);
				free(tempstr);

				params->status=0;

				return 0;
			}

			rpm=(unsigned short)(60000/(rotationtime/1000));

			CUI_affiche(MSG_DEBUG,"Drive RPM: %d",rpm);
			snprintf(tempstr,MAX_TMP_STR_SIZE,"Drive RPM: %d",rpm);

			Fl::lock();
			params->windowshwd->global_status->value(tempstr);
			Fl::unlock();

			if(rpm>280 && rpm<320)
				rpm=300;

			if(rpm>340 && rpm<380)
				rpm=360;

			bitrate=500000;

			snprintf(tempstr,MAX_TMP_STR_SIZE,"Starting reading disk...");
			Fl::lock();
			params->windowshwd->global_status->value(tempstr);
			Fl::unlock();

			memset(guicontext->mapfloppybuffer,0x00,xsize*ysize*4);

			o = 0;
			p = 0;

			for(i = fdp.start_track;i <= fdp.end_track;i++)
			{
				if(i<100)
				{
					for(j=params->start_side;j<=params->end_side;j++)
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
							guicontext->mapfloppybuffer[o++] = 0x55;
							guicontext->mapfloppybuffer[o++] = 0x55;
							guicontext->mapfloppybuffer[o] = 0x55;

							if(i&1)
								guicontext->mapfloppybuffer[o]=0xA5;

							o++;
							l--;
						}
					}
				}
			}

			for(i = fdp.start_track;i <= fdp.end_track && !fdp.stop;i++)
			{
				for(j = fdp.start_side;j < fdp.start_side + params->number_of_side && !fdp.stop;j++)
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

						trackformat = IBMFORMAT_DD;

						memset(sr,0,sizeof(FD_ID_HEADER)*MAX_SECTOR_NB + 1);

						if(tm[m].bt->value())
						{
							if(tm[m].encoding_mode)
							{
								sp.flags=FD_OPTION_MFM;
								trackformat = IBMFORMAT_DD;
							}
							else
							{
								sp.flags=0x00;
								trackformat = IBMFORMAT_SD;
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
						}

						l++;
					}while(l<8 && !sr->count);

					CUI_affiche(MSG_DEBUG,"Track %d side %d: %d sectors found : ",i,j,sr->count);

					seek(h,i*params->double_step, j);

					if(sr->count)
					{
						if( sr->Header[0].cyl==sr->Header[sr->count-1].cyl &&
							sr->Header[0].head==sr->Header[sr->count-1].head &&
							sr->Header[0].sector==sr->Header[sr->count-1].sector &&
							sr->Header[0].size==sr->Header[sr->count-1].size
							)
						{
							sr->count--;
						}
					}

					o=i*99*3;

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
								guicontext->mapfloppybuffer[o++] = 0xFF;
								guicontext->mapfloppybuffer[o++] = 0xFF;
								guicontext->mapfloppybuffer[o++] = 0xFF;
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
							rwp.flags = FD_OPTION_MFM;
						}
						else
						{
							rwp.flags = 0x00;
						}

						rwp.cyl = sr->Header[k].cyl;
						rwp.phead = j;
						rwp.head = sr->Header[k].head;
						rwp.sector = sr->Header[k].sector;
						rwp.size = sr->Header[k].size;
						rwp.eot = sr->Header[k].sector+1;
						rwp.gap = 10;

						if((128<<sr->Header[k].size)>128)
						{
							rwp.datalen=255;
						}
						else
						{
							rwp.datalen=128;
						}

						sectordata_size=(128<<(sr->Header[k].size&0x7));
						sector_data = (unsigned char*)malloc(sectordata_size);
						if(sector_data)
						{
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
						}
						else
						{
							retry = 0;
						}

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

						snprintf(tempstr,MAX_TMP_STR_SIZE,"%d Sector(s) Read, %d bytes, %d Bad sector(s)",numberofsector_read,total_size,number_of_bad_sector);

						Fl::lock();
						params->windowshwd->current_status->value(tempstr);
						Fl::unlock();
					}

					hxcfe_popTrack(fb);

					snprintf(tempstr,MAX_TMP_STR_SIZE,"Track %d side %d: %d sectors, %dkbits/s, %s",i,j,sr->count,bitrate/1000,((trackformat==ISOFORMAT_SD) || (trackformat==IBMFORMAT_SD) )?"FM":"MFM");
					Fl::lock();
					params->windowshwd->global_status->value(tempstr);
					Fl::unlock();
				}
			}

			closedevice(h);

			params->floppydisk=hxcfe_getFloppy(fb);
			load_floppy(params->floppydisk,"Floppy Dump Image");

			Fl::lock();
			if(!fdp.stop)
				snprintf(tempstr,MAX_TMP_STR_SIZE,"Done !");
			else
				snprintf(tempstr,MAX_TMP_STR_SIZE,"Stopped !");

			params->windowshwd->global_status->value(tempstr);
			snprintf(tempstr,MAX_TMP_STR_SIZE,"%d Sector(s) Read, %d bytes, %d Bad sector(s)",numberofsector_read,total_size,number_of_bad_sector);
			params->windowshwd->current_status->value(tempstr);
			Fl::unlock();

			CUI_affiche(MSG_DEBUG,"Done ! %d sectors read, %d bad sector(s), %d bytes read",numberofsector_read,number_of_bad_sector,total_size);

			free(tempstr);

			CUI_affiche(MSG_DEBUG,"Leaving Floppy dump...");

			params->status=0;
			return 0;
		}
	}

	snprintf(tempstr,MAX_TMP_STR_SIZE,"Error while opening fdrawcmd, see: https://simonowen.com/fdrawcmd");

	Fl::lock();
	params->windowshwd->global_status->value(tempstr);
	Fl::unlock();

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
	Fl_Group *gp;

	gp=((Fl_Group*)(bt->parent()));
	dw=((Fl_Window*)(gp->parent()));
	fdw=(floppy_dump_window *)dw->user_data();

	if(fdp.status)
		return;

	memset((void*)&fdp,0,sizeof(floppydumperparams));
	fdp.windowshwd=fdw;

	if(fdw->sel_drive_b->value())
	{
		fdp.drive=1;
	}

	fdp.number_of_side = 0;
	fdp.start_side = 0;
	fdp.end_side = 0;

	if(fdw->side_0->value() && fdw->side_1->value())
	{
		fdp.start_side = 0;
		fdp.end_side = 1;
		fdp.number_of_side = 2;
	}
	else
	{
		if(fdw->side_0->value())
		{
			fdp.start_side = 0;
			fdp.end_side = 0;
			fdp.number_of_side = 1;
		}

		if(fdw->side_1->value())
		{
			fdp.start_side = 1;
			fdp.end_side = 1;
			fdp.number_of_side = 1;
		}
	}

	fdp.double_step=1;
	if(fdw->double_step->value())
	{
		fdp.double_step=2;
	}

	fdp.retry=(int)fdw->number_of_retry->value();

	fdp.start_track = (int)fdw->start_track->value();
	fdp.end_track = (int)fdw->end_track->value();

	if( fdp.start_track > 84 )
		fdp.start_track = 84;

	if( fdp.end_track > 84 )
		fdp.end_track = 84;

	if(fdp.end_track < fdp.start_track)
		fdp.end_track = fdp.start_track;

	fdp.number_of_track = ( fdp.end_track - fdp.start_track ) + 1;

	fdp.flopemu=guicontext->hxcfe;

	snprintf(tempstr,sizeof(tempstr),"%d Sector(s) Read, %d bytes, %d Bad sector(s)",0,0,0);
	fdw->current_status->value(tempstr);

	fdp.stop = 0;

	hxc_createthread(guicontext->hxcfe,&fdp,&DumpThreadProc,0);
}

void floppy_dump_stop(Fl_Button*, void* w)
{
	fdp.stop = 1;
}

