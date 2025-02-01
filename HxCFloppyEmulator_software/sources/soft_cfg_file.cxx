#ifdef WIN32
#include <windows.h>
#include <commctrl.h>
#endif

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <stdint.h>

#include "soft_cfg_file.h"

#include "libhxcfe.h"

#include "usb_hxcfloppyemulator.h"

#include "main.h"

#include "libhxcadaptor.h"

#define TMP_PATH_SIZE 1024

typedef struct laststate_
{
	unsigned int usb_packet_size;
	unsigned int drive_select_source;
	unsigned int interface_mode;
	unsigned int autoselectmode;
	unsigned int twistedcable;
	unsigned int double_step;
}laststate;

extern s_gui_context * guicontext;
unsigned char cfg_file_buffer[4*1024];

int load_last_cfg()
{
	FILE * f;
	laststate * lastst;
	char *tmp_ptr;
	char executablepath[TMP_PATH_SIZE];

	lastst=(laststate *)&cfg_file_buffer;

	memset(cfg_file_buffer,0,TMP_PATH_SIZE);

	hxc_getcurrentdirectory(executablepath,TMP_PATH_SIZE);

	tmp_ptr = hxc_getfilenamebase(executablepath,NULL,SYS_PATH_TYPE);
	snprintf( tmp_ptr, TMP_PATH_SIZE - ( 11 + (tmp_ptr - (char*)&executablepath)),"config.dat");

	f=hxc_fopen(executablepath,"rb");
	if(f)
	{
		if(!fread(lastst,sizeof(cfg_file_buffer),1,f))
		{
			memset(lastst,0,sizeof(cfg_file_buffer));
		}
		hxc_fclose(f);

//      hwif->usbstats.packetsize=lastst->usb_packet_size;
//      hwif->drive_select_source=lastst->drive_select_source;
//      hwif->interface_mode=lastst->interface_mode;
//      hwif->double_step=lastst->double_step;
		guicontext->autoselectmode=lastst->autoselectmode;
		guicontext->twistedcable=lastst->twistedcable;
	}
	else
	{
		//hwif->usbstats.packetsize=1792;
	}
	return 0;
};


int save_cfg()
{
	FILE * f;
	laststate * lastst;
	char executablepath[TMP_PATH_SIZE];
	char * tmp_ptr;

	lastst=(laststate *)&cfg_file_buffer;

	memset(&cfg_file_buffer,0,sizeof(cfg_file_buffer));

	hxc_getcurrentdirectory(executablepath,TMP_PATH_SIZE);

	tmp_ptr = hxc_getfilenamebase(executablepath,NULL,SYS_PATH_TYPE);
	snprintf( tmp_ptr, TMP_PATH_SIZE - ( 11 + (tmp_ptr - (char*)&executablepath)),"config.dat");

	f=hxc_fopen(executablepath,"wb");
	if(f)
	{
	//  lastst->usb_packet_size=hwif->usbstats.packetsize;
	//  lastst->drive_select_source=hwif->drive_select_source;
	//  lastst->interface_mode=hwif->interface_mode;
	//  lastst->autoselectmode=gui_context->autoselectmode;
	//  lastst->twistedcable=gui_context->twistedcable;
	//  lastst->double_step=hwif->double_step;
		fwrite(lastst,sizeof(cfg_file_buffer),1,f);
		hxc_fclose(f);
	}
	return 0;
};