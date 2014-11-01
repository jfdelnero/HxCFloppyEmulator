
#ifndef _HXCFE_FSMNG_

typedef struct _HXCFE_FSMNG
{
	HXCFE * hxcfe;
	int fs_selected;

	// Mounted Floppy disk
	HXCFE_FLOPPY *fp;

	HXCFE_FDCCTRL * fdc;

	// mounted disk image geometry
	int sectorpertrack;
	int sidepertrack;
	int trackperdisk;
	int sectorsize;

	void * handletable[128];
	void * dirhandletable[128];
	int    dirindex[128];

	void  * device;
	void  * volume;
	void  * internal;

}HXCFE_FSMNG;

#define _HXCFE_FSMNG_

#endif
