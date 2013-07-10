/* Signed.  */

/* There is some amount of overlap with <sys/types.h> as known by inet code */
#ifndef __int8_t_defined
#define __int8_t_defined
typedef signed char		int8_t;
typedef short int		int16_t;
typedef int			int32_t;
#endif

/* Unsigned.  */
typedef unsigned char		uint8_t;
typedef unsigned short int	uint16_t;
#ifndef __uint32_t_defined
typedef unsigned int		uint32_t;
# define __uint32_t_defined
#endif


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