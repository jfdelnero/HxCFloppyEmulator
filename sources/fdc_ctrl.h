
#ifndef _HXCFE_FDCCTRL_
typedef struct _HXCFE_FDCCTRL
{
	HXCFE* floppycontext;
	HXCFE_FLOPPY * loadedfp;
	HXCFE_SECTORACCESS * ss_ctx;
}HXCFE_FDCCTRL;
#define _HXCFE_FDCCTRL_
#endif
