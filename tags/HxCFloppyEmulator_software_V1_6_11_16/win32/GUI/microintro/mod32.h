#include <windows.h>

#ifndef __include_mod32_h
#define __include_mod32_h
/*

#ifndef EXTERN_C
#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C
#endif
#endif
*/
//EXTERN_C 
void WINAPI CLOSEMODSYS(void);
//EXTERN_C 
void WINAPI InitModule(char *pfile,char *pbuffer,unsigned long sizebuffer,char *offsetdata);
//EXTERN_C 
void WINAPI GiveMeSamples(char * pbuffer,unsigned long bufize);

#endif
