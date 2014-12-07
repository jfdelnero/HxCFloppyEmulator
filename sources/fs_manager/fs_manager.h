
#ifndef _HXCFE_FSMNG_

typedef struct _HXCFE_FSMNG
{
	HXCFE * hxcfe;
	int32_t fs_selected;

	// Mounted Floppy disk
	HXCFE_FLOPPY *fp;

	HXCFE_FDCCTRL * fdc;

	// mounted disk image geometry
	int32_t sectorpertrack;
	int32_t sidepertrack;
	int32_t trackperdisk;
	int32_t sectorsize;

	void * handletable[128];
	void * dirhandletable[128];
	int32_t    dirindex[128];

	void  * device;
	void  * volume;
	void  * internal;

}HXCFE_FSMNG;

#define _HXCFE_FSMNG_

#endif
