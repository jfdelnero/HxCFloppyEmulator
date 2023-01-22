#ifndef __WINDOWS_TYPES__
#define __WINDOWS_TYPES__

#define WINAPI

#define MAX_NUM_DEVICES 50
#include <sys/time.h>

typedef unsigned int			DWORD;
typedef unsigned int			ULONG;
typedef unsigned short			USHORT;
typedef unsigned short			SHORT;
typedef unsigned char			UCHAR;
typedef unsigned short			WORD;
typedef unsigned char			BYTE;
typedef BYTE					*LPBYTE;
typedef unsigned int			BOOL;
typedef unsigned char			BOOLEAN;
typedef unsigned char			CHAR;
typedef BOOL					*LPBOOL;
typedef UCHAR					*PUCHAR;
typedef const char				*LPCSTR;
typedef char					*PCHAR;
typedef void					*PVOID;
typedef void					*HANDLE;
typedef unsigned int			LONG;
typedef int						INT;
typedef unsigned int			UINT;
typedef char					*LPSTR;
typedef char					*LPTSTR;
typedef const char				*LPCTSTR;
typedef DWORD					*LPDWORD;
typedef WORD					*LPWORD;
typedef ULONG					*PULONG;
typedef LONG					*LPLONG;
typedef PVOID					LPVOID;
typedef void					VOID;
typedef unsigned long long int	ULONGLONG;

typedef struct _OVERLAPPED {
	DWORD Internal;
	DWORD InternalHigh;
	DWORD Offset;
	DWORD OffsetHigh;
	HANDLE hEvent;
} OVERLAPPED, *LPOVERLAPPED;

typedef struct _SECURITY_ATTRIBUTES {
	DWORD nLength;
	LPVOID lpSecurityDescriptor;
	BOOL bInheritHandle;
} SECURITY_ATTRIBUTES , *LPSECURITY_ATTRIBUTES;

typedef struct timeval SYSTEMTIME;
typedef struct timeval FILETIME;
#ifndef TRUE
#define TRUE	1
#endif
#ifndef FALSE
#define FALSE	0
#endif

//
// Modem Status Flags
//
#define MS_CTS_ON           ((DWORD)0x0010)
#define MS_DSR_ON           ((DWORD)0x0020)
#define MS_RING_ON          ((DWORD)0x0040)
#define MS_RLSD_ON          ((DWORD)0x0080)

//
// Error Flags
//
#define CE_RXOVER           0x0001  // Receive Queue overflow
#define CE_OVERRUN          0x0002  // Receive Overrun Error
#define CE_RXPARITY         0x0004  // Receive Parity Error
#define CE_FRAME            0x0008  // Receive Framing error
#define CE_BREAK            0x0010  // Break Detected
#define CE_TXFULL           0x0100  // TX Queue is full
#define CE_PTO              0x0200  // LPTx Timeout
#define CE_IOE              0x0400  // LPTx I/O Error
#define CE_DNS              0x0800  // LPTx Device not selected
#define CE_OOP              0x1000  // LPTx Out-Of-Paper
#define CE_MODE             0x8000  // Requested mode unsupported

//
// Events
//
#define EV_RXCHAR           0x0001  // Any Character received
#define EV_RXFLAG           0x0002  // Received certain character
#define EV_TXEMPTY          0x0004  // Transmit Queue Empty
#define EV_CTS              0x0008  // CTS changed state
#define EV_DSR              0x0010  // DSR changed state
#define EV_RLSD             0x0020  // RLSD changed state
#define EV_BREAK            0x0040  // BREAK received
#define EV_ERR              0x0080  // Line status error occurred
#define EV_RING             0x0100  // Ring signal detected
#define EV_PERR             0x0200  // Printer error occured
#define EV_RX80FULL         0x0400  // Receive buffer is 80 percent full
#define EV_EVENT1           0x0800  // Provider specific event 1
#define EV_EVENT2           0x1000  // Provider specific event 2

//
// Escape Functions
//
#define SETXOFF             1       // Simulate XOFF received
#define SETXON              2       // Simulate XON received
#define SETRTS              3       // Set RTS high
#define CLRRTS              4       // Set RTS low
#define SETDTR              5       // Set DTR high
#define CLRDTR              6       // Set DTR low
#define RESETDEV            7       // Reset device if possible
#define SETBREAK            8       // Set the device break line.
#define CLRBREAK            9       // Clear the device break line.

//
// PURGE function flags.
//
#define PURGE_TXABORT       0x0001  // Kill the pending/current writes to the comm port.
#define PURGE_RXABORT       0x0002  // Kill the pending/current reads to the comm port.
#define PURGE_TXCLEAR       0x0004  // Kill the transmit queue if there.
#define PURGE_RXCLEAR       0x0008  // Kill the typeahead buffer if there.

#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE 0xFFFFFFFF
#endif

#endif
