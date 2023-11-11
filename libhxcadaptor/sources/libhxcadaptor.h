#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32
#define DIR_SEPARATOR "\\"
#define DIR_SEPARATOR_CHAR '\\'
#if defined(_MSC_VER) && _MSC_VER < 1900
int snprintf(char *outBuf, size_t size, const char *format, ...);
#endif
#else
#define DIR_SEPARATOR "/"
#define DIR_SEPARATOR_CHAR '/'
#endif

#define FILEFOUND_NAMESIZE 256

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
	char filename[FILEFOUND_NAMESIZE];
	int size;
}filefoundinfo;

int hxc_setevent( HXCFE* floppycontext, unsigned char id );
uintptr_t hxc_createevent( HXCFE* floppycontext, unsigned char id );
int hxc_waitevent( HXCFE* floppycontext, int id, int timeout );
void hxc_pause( int ms );
int hxc_createthread( HXCFE* floppycontext, void* hwcontext, THREADFUNCTION thread, int priority );

void * hxc_createcriticalsection( HXCFE* floppycontext, unsigned char id );
void hxc_entercriticalsection( HXCFE* floppycontext, unsigned char id );
void hxc_leavecriticalsection( HXCFE* floppycontext, unsigned char id );
void hxc_destroycriticalsection( HXCFE* floppycontext, unsigned char id );

/////////////// String functions ///////////////

#ifndef WIN32
//void strlwr(char *string)
#endif
char * hxc_strupper( char * str );
char * hxc_strlower( char * str );
char * hxc_dyn_strcat(char * deststr,char * srcstr);
char * hxc_dyn_sprintfcat(char * deststr,char * srcstr, ...);

/////////////// File functions ////////////////

int hxc_open ( const char *filename, int flags, ... );

FILE *hxc_fopen ( const char *filename, const char *mode );
int hxc_fread( void * ptr, size_t size, FILE *f );
char * hxc_fgets( char * str, int num, FILE *f );
int hxc_fclose( FILE * f );
int hxc_fgetsize( FILE * f );
#ifndef stat
#include <sys/stat.h>
#endif
int hxc_stat( const char *filename, struct stat *buf );

void* hxc_find_first_file( char *folder, char *file, filefoundinfo* fileinfo );
int hxc_find_next_file( void* handleff, char *folder, char *file, filefoundinfo* fileinfo );
int hxc_find_close( void* handle );

int  hxc_mkdir( char * folder );

char * hxc_getcurrentdirectory( char *currentdirectory, int buffersize );

enum
{
	SYS_PATH_TYPE = 0,
	UNIX_PATH_TYPE,
	WINDOWS_PATH_TYPE,
};

char * hxc_getfilenamebase( char * fullpath, char * filenamebase, int type );
char * hxc_getfilenameext( char * fullpath, char * filenameext, int type );
int hxc_getfilenamewext( char * fullpath, char * filenamewext, int type );
int hxc_getpathfolder( char * fullpath, char * folder, int type );
int hxc_checkfileext( char * path, char *ext, int type );
int hxc_getfilesize( char * path );

typedef struct HXCRAMFILE_
{
	uint8_t * ramfile;
	int32_t ramfile_size;
}HXCRAMFILE;

FILE * hxc_ram_fopen(char* fn, char * mode, HXCRAMFILE * rf);
int hxc_ram_fwrite(void * buffer,int size,int mul,FILE * file,HXCRAMFILE * rf);
int hxc_ram_fclose(FILE *f,HXCRAMFILE * rf);

/////////////// Network functions ////////////////

void * network_connect(char * address,unsigned short port);
int network_read(void * network_connection, unsigned char * buffer, int size,int timeout);
int network_read2(void * network_connection, unsigned char * buffer, int size,int timeout);
int network_write(void * network_connection, unsigned char * buffer, int size,int timeout);
int network_close(void * network_connection);

#ifdef __cplusplus
}
#endif
