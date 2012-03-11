
#include <windows.h>
#include <commctrl.h>

#include <stdio.h>
#include <time.h>

#include <sys/stat.h>

#include "mainrouts.h"

#include "soft_cfg_file.h"

#include "libhxcfe.h"
#include "./usb_floppyemulator/usb_hxcfloppyemulator.h"

typedef struct laststate_
{
	unsigned int usb_packet_size;
	unsigned int drive_select_source;
	unsigned int interface_mode;
	unsigned int autoselectmode;
	unsigned int twistedcable;
	unsigned int double_step;
}laststate;


extern HWINTERFACE * hwif;
//extern guicontext * demo;
extern guicontext * gui_context;
unsigned char cfg_file_buffer[4*1024];

int load_last_cfg()
{
	FILE * f;
	int i;
	laststate * lastst;
	char executablepath[512];

	lastst=(laststate *)&cfg_file_buffer;

	memset(cfg_file_buffer,0,sizeof(cfg_file_buffer));
	
	GetModuleFileName(NULL,executablepath,512);

	i=strlen(executablepath);
	while(i && executablepath[i]!='\\')
	{
		i--;
	}
	if(executablepath[i]=='\\')
	{
		i++;
	}
	sprintf(&executablepath[i],"config.dat");


	f=fopen(executablepath,"rb");
	if(f)
	{
		fread(lastst,sizeof(cfg_file_buffer),1,f);
		fclose(f);

		hwif->usbstats.packetsize=lastst->usb_packet_size;
		hwif->drive_select_source=lastst->drive_select_source;
		hwif->interface_mode=lastst->interface_mode;
		hwif->double_step=lastst->double_step;
		gui_context->autoselectmode=lastst->autoselectmode;
		gui_context->twistedcable=lastst->twistedcable;
	}
	else
	{
		hwif->usbstats.packetsize=1792;
	}
	return 0;
};


int save_cfg()
{
	FILE * f;
	int i;
	laststate * lastst;
	char executablepath[512];

	lastst=(laststate *)&cfg_file_buffer;

	memset(&cfg_file_buffer,0,sizeof(laststate));

	GetModuleFileName(NULL,executablepath,512);

	i=strlen(executablepath);
	while(i && executablepath[i]!='\\')
	{
		i--;
	}
	if(executablepath[i]=='\\')
	{
		i++;
	}
	sprintf(&executablepath[i],"config.dat");


	f=fopen(executablepath,"wb");
	if(f)
	{
		lastst->usb_packet_size=hwif->usbstats.packetsize;
		lastst->drive_select_source=hwif->drive_select_source;
		lastst->interface_mode=hwif->interface_mode;
		lastst->autoselectmode=gui_context->autoselectmode;
		lastst->twistedcable=gui_context->twistedcable;
		lastst->double_step=hwif->double_step;
		fwrite(lastst,sizeof(cfg_file_buffer),1,f);
		fclose(f);
	}
	return 0;
};