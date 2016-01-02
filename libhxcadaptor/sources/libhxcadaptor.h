#ifdef __cplusplus
extern "C" {
#endif

/////////////// Thread functions ////////////////

typedef int (*THREADFUNCTION) (void* floppyemulator,void* hwemulator);

typedef struct threadinit_
{
	THREADFUNCTION thread;
	HXCFE * hxcfloppyemulatorcontext;
	void * hwcontext;
}threadinit;

typedef struct filefoundinfo_
{
	int isdirectory;
	char filename[256];
	int size;
}filefoundinfo;

int hxc_setevent(HXCFE* floppycontext,unsigned char id);
unsigned long hxc_createevent(HXCFE* floppycontext,unsigned char id);
int hxc_waitevent(HXCFE* floppycontext,int id,int timeout);
void hxc_pause(int ms);
int hxc_createthread(HXCFE* floppycontext,void* hwcontext,THREADFUNCTION thread,int priority);

unsigned long hxc_createcriticalsection(HXCFE* floppycontext,unsigned char id);
void hxc_entercriticalsection(HXCFE* floppycontext,unsigned char id);
void hxc_leavecriticalsection(HXCFE* floppycontext,unsigned char id);
void hxc_destroycriticalsection(HXCFE* floppycontext,unsigned char id);

/////////////// String functions ///////////////

#ifndef WIN32
//void strlwr(char *string)
#endif
char * hxc_strupper(char * str);
char * hxc_strlower(char * str);

/////////////// File functions ////////////////

int hxc_open (const char *filename, int flags, ...);

FILE *hxc_fopen (const char *filename, const char *mode);
int hxc_fread(void * ptr, size_t size, FILE *f);
int hxc_fclose(FILE * f);

#ifndef stat
#include <sys/stat.h>
#endif
int hxc_stat( const char *filename, struct stat *buf);

int hxc_find_first_file(char *folder,char *file,filefoundinfo* fileinfo);
int hxc_find_next_file(int handleff,char *folder,char *file,filefoundinfo* fileinfo);
int hxc_find_close(int handle);

int  hxc_mkdir(char * folder);

char * hxc_getcurrentdirectory(char *currentdirectory,int buffersize);

char * hxc_getfilenamebase(char * fullpath,char * filenamebase);
char * hxc_getfilenameext(char * fullpath,char * filenameext);
int hxc_getfilenamewext(char * fullpath,char * filenamewext);
int hxc_getpathfolder(char * fullpath,char * folder);
int hxc_checkfileext(char * path,char *ext);
int hxc_getfilesize(char * path);

#ifdef __cplusplus
}
#endif
