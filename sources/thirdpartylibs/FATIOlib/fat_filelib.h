#ifndef __FAT_FILELIB_H__
#define __FAT_FILELIB_H__

#include "fat_opts.h"
#include "fat_access.h"
#include "fat_list.h"

//-----------------------------------------------------------------------------
// Defines
//-----------------------------------------------------------------------------
#ifndef SEEK_CUR
    #define SEEK_CUR    1
#endif

#ifndef SEEK_END
    #define SEEK_END    2
#endif

#ifndef SEEK_SET
    #define SEEK_SET    0
#endif

#ifndef EOF
    #define EOF         (-1)
#endif

//-----------------------------------------------------------------------------
// Structures
//-----------------------------------------------------------------------------
struct sFL_FILE;

struct cluster_lookup
{
    uint32 ClusterIdx;
    uint32 CurrentCluster;
};

typedef struct sFL_FILE
{
    uint32                  parentcluster;
    uint32                  startcluster;
    uint32                  bytenum;
    uint32                  filelength;
    int                     filelength_changed;
    char                    path[FATFS_MAX_LONG_FILENAME];
    char                    filename[FATFS_MAX_LONG_FILENAME];
    uint8                   shortfilename[11];

#ifdef FAT_CLUSTER_CACHE_ENTRIES
    uint32                  cluster_cache_idx[FAT_CLUSTER_CACHE_ENTRIES];
    uint32                  cluster_cache_data[FAT_CLUSTER_CACHE_ENTRIES];
#endif

    // Cluster Lookup
    struct cluster_lookup   last_fat_lookup;

    // Read/Write sector buffer
    uint8                   file_data_sector[MAX_FAT_SECTOR_SIZE];
    uint32                  file_data_address; 
    int                     file_data_dirty;

    // File fopen flags
    uint8                   flags;
#define FILE_READ           (1 << 0)
#define FILE_WRITE          (1 << 1)
#define FILE_APPEND         (1 << 2)
#define FILE_BINARY         (1 << 3)
#define FILE_ERASE          (1 << 4)
#define FILE_CREATE         (1 << 5)

    struct fat_node         list_node;
} FL_FILE;

//-----------------------------------------------------------------------------
// Prototypes
//-----------------------------------------------------------------------------

// External
void                fiol_init(void);
void                fiol_attach_locks(struct fatfs *fs, void (*lock)(void), void (*unlock)(void));
int                 fiol_attach_media(fn_diskio_read rd, fn_diskio_write wr);
void                fiol_shutdown(void);

// Standard API
void*               fiol_fopen(const char *path, const char *modifiers);
void                fiol_fclose(void *file);
int                 fiol_fflush(void *file);
int                 fiol_fgetc(void *file);
char *              fiol_fgets(char *s, int n, void *f);
int                 fiol_fputc(int c, void *file);
int                 fiol_fputs(const char * str, void *file);
int                 fiol_fwrite(const void * data, int size, int count, void *file );
int                 fiol_fread(void * data, int size, int count, void *file );
int                 fiol_fseek(void *file , long offset , int origin );
int                 fiol_fgetpos(void *file , uint32 * position);
long                fiol_ftell(void *f);
int                 fiol_feof(void *f);
int                 fiol_remove(const char * filename);    

// Equivelant dirent.h 
typedef struct fs_dir_list_status    FL_DIR;
typedef struct fs_dir_ent            fl_dirent;

FL_DIR*             fiol_opendir(const char* path, FL_DIR *dir);
int                 fiol_readdir(FL_DIR *dirls, fl_dirent *entry);
int                 fiol_closedir(FL_DIR* dir);

// Extensions
void                fiol_listdirectory(const char *path);
int                 fiol_createdirectory(const char *path);
int                 fiol_is_dir(const char *path);

// Test hooks
#ifdef FATFS_INC_TEST_HOOKS
struct fatfs*       fiol_get_fs(void);
#endif

// Disk size functions
int fiol_getFreeSpace(void);
int fiol_getTotalSpace(void);

//-----------------------------------------------------------------------------
// Stdio file I/O names
//-----------------------------------------------------------------------------
#ifdef USE_FILELIB_STDIO_COMPAT_NAMES

#define FILE            FL_FILE

#define fopen(a,b)      fiol_fopen(a, b)
#define fclose(a)       fiol_fclose(a)
#define fflush(a)       fiol_fflush(a)
#define fgetc(a)        fiol_fgetc(a)
#define fgets(a,b,c)    fiol_fgets(a, b, c)
#define fputc(a,b)      fiol_fputc(a, b)
#define fputs(a,b)      fiol_fputs(a, b)
#define fwrite(a,b,c,d) fiol_fwrite(a, b, c, d)
#define fread(a,b,c,d)  fiol_fread(a, b, c, d)
#define fseek(a,b,c)    fiol_fseek(a, b, c)
#define fgetpos(a,b)    fiol_fgetpos(a, b)
#define ftell(a)        fiol_ftell(a)
#define feof(a)         fiol_feof(a)
#define remove(a)       fiol_remove(a)
#define mkdir(a)        fiol_createdirectory(a)
#define rmdir(a)        0

#endif

#endif
