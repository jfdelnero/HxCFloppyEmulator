#ifdef IPF_SUPPORT 

#include <windows.h>
#include "thirdpartylibs/capslib/Comtype.h"

typedef SDWORD (__cdecl* CAPSINIT)(void);
typedef SDWORD (__cdecl* CAPSADDIMAGE)(void);
typedef SDWORD (__cdecl* CAPSLOCKIMAGEMEMORY)(SDWORD,PUBYTE,UDWORD,UDWORD);
typedef SDWORD (__cdecl* CAPSUNLOCKIMAGE)(SDWORD);
typedef SDWORD (__cdecl* CAPSLOADIMAGE)(SDWORD,UDWORD);
typedef SDWORD (__cdecl* CAPSGETIMAGEINFO)(PCAPSIMAGEINFO,SDWORD);
typedef SDWORD (__cdecl* CAPSLOCKTRACK)(PCAPSTRACKINFO,SDWORD,UDWORD,UDWORD,UDWORD);
typedef SDWORD (__cdecl* CAPSUNLOCKTRACK)(SDWORD id, UDWORD cylinder, UDWORD head);
typedef SDWORD (__cdecl* CAPSUNLOCKALLTRACKS)(SDWORD);
typedef SDWORD (__cdecl* CAPSGETVERSIONINFO)(PCAPSVERSIONINFO,UDWORD);
typedef SDWORD (__cdecl* CAPSREMIMAGE)(SDWORD id);

CAPSINIT pCAPSInit=0;
CAPSADDIMAGE pCAPSAddImage=0;
CAPSLOCKIMAGEMEMORY pCAPSLockImageMemory=0;
CAPSUNLOCKIMAGE pCAPSUnlockImage=0;
CAPSLOADIMAGE pCAPSLoadImage=0;
CAPSGETIMAGEINFO pCAPSGetImageInfo=0;
CAPSLOCKTRACK pCAPSLockTrack=0;
CAPSUNLOCKTRACK pCAPSUnlockTrack=0;
CAPSUNLOCKALLTRACKS pCAPSUnlockAllTracks=0;
CAPSGETVERSIONINFO pCAPSGetVersionInfo=0;
CAPSREMIMAGE pCAPSRemImage=0;

int init_caps_lib()
{
	HMODULE h;
	uint32_t ret;
	if(pCAPSInit && \
	   pCAPSAddImage &&  \
	   pCAPSLockImageMemory && \
	   pCAPSUnlockImage && \
	   pCAPSLoadImage && \
	   pCAPSGetImageInfo && \
	   pCAPSLockTrack && \
	   pCAPSUnlockTrack && \
	   pCAPSUnlockAllTracks && \
	   pCAPSGetVersionInfo && \
	   pCAPSRemImage)
	{
		return 1;
	}
	else
	{
		h = LoadLibrary ("CAPSImg.dll");
		if(h)
		{
			ret=(uint32_t )GetProcAddress (h, "CAPSInit");
			pCAPSInit = (CAPSINIT)ret;
			if(!ret){ FreeLibrary(h); return 0;};

			ret=(uint32_t )GetProcAddress (h, "CAPSAddImage");
			pCAPSAddImage = (CAPSADDIMAGE)ret;
			if(!ret){ FreeLibrary(h); return 0;};

			ret=(uint32_t )GetProcAddress (h, "CAPSLockImageMemory");
			pCAPSLockImageMemory = (CAPSLOCKIMAGEMEMORY)ret;
			if(!ret){ FreeLibrary(h); return 0;};

			ret=(uint32_t )GetProcAddress (h, "CAPSUnlockImage");
			pCAPSUnlockImage = (CAPSUNLOCKIMAGE)ret;
			if(!ret){ FreeLibrary(h); return 0;};

			ret=(uint32_t )GetProcAddress (h, "CAPSLoadImage");
			pCAPSLoadImage = (CAPSLOADIMAGE)ret;
			if(!ret){ FreeLibrary(h); return 0;};

			ret=(uint32_t )GetProcAddress (h, "CAPSGetImageInfo");
			pCAPSGetImageInfo = (CAPSGETIMAGEINFO)ret;
			if(!ret){ FreeLibrary(h); return 0;};

			ret=(uint32_t )GetProcAddress (h, "CAPSLockTrack");
			pCAPSLockTrack = (CAPSLOCKTRACK)ret;
			if(!ret){ FreeLibrary(h); return 0;};

			ret=(uint32_t )GetProcAddress (h, "CAPSUnlockTrack");
			pCAPSUnlockTrack = (CAPSUNLOCKTRACK)ret;
			if(!ret){ FreeLibrary(h); return 0;};

			ret=(uint32_t )GetProcAddress (h, "CAPSUnlockAllTracks");
			pCAPSUnlockAllTracks = (CAPSUNLOCKALLTRACKS)ret;
			if(!ret){ FreeLibrary(h); return 0;};

			ret=(uint32_t )GetProcAddress (h, "CAPSGetVersionInfo");
			pCAPSGetVersionInfo = (CAPSGETVERSIONINFO)ret;
			if(!ret){ FreeLibrary(h); return 0;};

			ret=(uint32_t )GetProcAddress (h, "CAPSRemImage");
			pCAPSRemImage = (CAPSREMIMAGE)ret;
			if(!ret){ FreeLibrary(h); return 0;};

			return 1;
		}

	}


	return 0;
};

#endif
