#include <windows.h>
#include "uswait.h"


ULONG uswCalibrate()
{
	ULONG hi,lo,t1,t2;
	ULONGLONG y1;

//SetPriorityClass(GetCurrentProcess(),REALTIME_PRIORITY_CLASS);
//SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

__asm {
		rdtsc
		mov ds:[hi],edx
		mov ds:[lo],eax
	}
	y1=GetTickCount();
	do
	{
	}while(GetTickCount()-y1<1000);

	__asm {
		rdtsc
		mov ebx,hi
		mov ecx,lo
		sub edx,ebx
		sub eax,ecx
		mov t2,edx
		mov t1,eax
	}
    R1=t1;

 return R1;
}


/*__forceinline*/ void waitus(ULONG useconde)
{
	ULONG hi,lo,t1,t3;
//SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
	t3=(R1/1000000)*useconde;
	
	__asm {
		rdtsc
		mov ds:[hi],edx
		mov ds:[lo],eax
	boucle:
		rdtsc
		mov ecx,lo
		sub eax,ecx
		mov t1,eax
		cmp eax,ds:[t3]
		jnae boucle
	}


//SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
}


unsigned long gettick()
{
	ULONG hi,lo;
//SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
	__asm {
		rdtsc
		mov ds:[hi],edx
		mov ds:[lo],eax
	}


return lo;

//SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
}