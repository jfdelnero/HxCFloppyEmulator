/////////////// Thread functions ////////////////

typedef int (*THREADFUNCTION) (void* floppyemulator,void* hwemulator);

typedef struct threadinit_
{
	THREADFUNCTION thread;
	HXCFLOPPYEMULATOR * hxcfloppyemulatorcontext;
	void * hwcontext;
}threadinit;

typedef struct filefoundinfo_
{
	int isdirectory;
	char filename[256];
	int size;
}filefoundinfo;

int hxc_setevent(HXCFLOPPYEMULATOR* floppycontext,unsigned char id);
unsigned long hxc_createevent(HXCFLOPPYEMULATOR* floppycontext,unsigned char id);
int hxc_waitevent(HXCFLOPPYEMULATOR* floppycontext,int id,int timeout);
void hxc_pause(int ms);
int hxc_createthread(HXCFLOPPYEMULATOR* floppycontext,void* hwcontext,THREADFUNCTION thread,int priority);

/////////////// String functions ///////////////

#ifndef WIN32
//void strlwr(char *string)
#endif
char * strupper(char * str);
char * strlower(char * str);

/////////////// File functions ////////////////

int hxc_open (const char *filename, int flags, ...);

FILE *hxc_fopen (const char *filename, const char *mode);
int hxc_fclose(FILE * f);

#ifndef stat
#include <sys/stat.h>
#endif
int hxc_stat( const char *filename, struct stat *buf);

long find_first_file(char *folder,char *file,filefoundinfo* fileinfo);
long find_next_file(long handleff,char *folder,char *file,filefoundinfo* fileinfo);
long find_close(long handle);
