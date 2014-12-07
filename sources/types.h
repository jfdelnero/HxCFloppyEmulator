#include <stdint.h>

#ifndef FALSE
#define FALSE 0x00
#endif

#ifndef TRUE
#define TRUE  0xFF
#endif

/////////////////////////////////////////////////////////////////////////

#ifdef BIGENDIAN_HOST

// big endian
#define BIGENDIAN_WORD(wValue) (wValue)
#define BIGENDIAN_DWORD(dwValue) (dwValue)

// little endian

#define LITTLEENDIAN_WORD(wValue) ( ((wValue<<8)&0xFF00) | ((wValue>>8)&0xFF) )
#define LITTLEENDIAN_DWORD(dwValue) ( ((dwValue<<24)&0xFF000000) | ((dwValue<<16)&0xFF0000) | ((dwValue>>16)&0xFF00) | ((dwValue>>24)&0xFF) )

#else

//LITTLE ENDIAN HOST

// big endian
#define BIGENDIAN_WORD(wValue) ( ((wValue<<8)&0xFF00) | ((wValue>>8)&0xFF) )
#define BIGENDIAN_DWORD(dwValue) ( ((dwValue<<24)&0xFF000000) | ((dwValue<<16)&0xFF0000) | ((dwValue>>16)&0xFF00) | ((dwValue>>24)&0xFF) )

// little endian

#define LITTLEENDIAN_WORD (wValue) (wValue)
#define LITTLEENDIAN_DWORD(dwValue) (dwValue)

#endif