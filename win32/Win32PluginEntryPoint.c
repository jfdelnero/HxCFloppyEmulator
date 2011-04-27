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
// File : Win32PluginEntryPoint.c
// Contains: Common plugin entry points
//
// Written by:	DEL NERO Jean Francois
//
// Change History (most recent first):
///////////////////////////////////////////////////////////////////////////////////

#include <windows.h>


#include "HxCFloppy.h"
#include "HxCFloppyHard.h"
#include "HxCFloppyEmulator.h"
#include "pluginsheaders.h"

//#define DLL_EXPORTS 1

//#ifdef DLL_EXPORTS
#define DLL_API __declspec(dllexport)
//#else
//#define DLL_API __declspec(dllimport)
//#endif


BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    return TRUE;
}

DLL_API int __cdecl IsValidDiskFile(HXCFLOPPYEMULATOR* floppycontext,char * imgfile)
{

	return libIsValidDiskFile(floppycontext,imgfile);
}


DLL_API int __cdecl LoadDiskFile(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * floppy,char * imgfile)
{

	return libLoad_DiskFile(floppycontext,floppy,imgfile);
}


DLL_API int __cdecl UnLoadDiskFile(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * floppy)
{
	return libUnLoadDiskFile(floppycontext,floppy);
}


DLL_API int __cdecl GetFloppyPropretiesInt(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * floppy,char * name,int value1,int value2)
{
	return libGetFloppyPropretiesInt(floppycontext,floppy,name,value1,value2);
}


DLL_API int __cdecl GetTrackData(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * floppy,unsigned int track,unsigned int head,int bufferposition,unsigned int buffersize, unsigned char * rawdata)
{
	return libGetTrackData(floppycontext,floppy,track,head,bufferposition,buffersize, rawdata);
}

DLL_API int __cdecl GetIndexData(HXCFLOPPYEMULATOR* floppycontext,FLOPPY * floppy,unsigned int track,unsigned int head,int bufferposition,unsigned int buffersize, unsigned char * rawdata)
{
	return libGetIndexData(floppycontext,floppy,track,head,bufferposition,buffersize, rawdata);
}
