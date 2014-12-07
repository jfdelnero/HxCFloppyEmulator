

typedef int (*fn_diskio_read) (uint32_t sector, unsigned char *buffer, uint32_t sector_count);
typedef int (*fn_diskio_write)(uint32_t sector, unsigned char *buffer, uint32_t sector_count);

typedef struct _cpmfs
{
	fn_diskio_read  rdfn;
	fn_diskio_write wrfn;
}cpmfs;

typedef struct _cpmfs_dir
{
	int sector;
}cpmfs_dir;

typedef struct _cpmfs_entry
{
	int filename[8+3+1];
	int size;
	int is_dir;
}cpmfs_entry;

int     libcpmfs_init(cpmfs * fs);
int     libcpmfs_attach_media(cpmfs * fs, fn_diskio_read rd, fn_diskio_write wr);
int     libcpmfs_deinit(cpmfs * fs);

int     libcpmfs_mountImage(cpmfs * fs);
int     libcpmfs_unmountImage(cpmfs * fs);

void  * libcpmfs_fopen(cpmfs * fs, const char *path, const char *modifiers);
int     libcpmfs_fwrite(cpmfs * fs,const void * data, int size, int count, void *file );
int     libcpmfs_ftell(cpmfs * fs, void *file);
int     libcpmfs_fseek(cpmfs * fs, void *file, int32_t offset, int origin );
int     libcpmfs_fread(cpmfs * fs, void * data, int size, int count, void *file );
int     libcpmfs_feof(cpmfs * fs,  void *file);
int     libcpmfs_fclose(cpmfs * fs,void *file);

cpmfs_dir* libcpmfs_opendir(cpmfs * fs, const char* path, cpmfs_dir *dir);
int     libcpmfs_readdir(cpmfs * fs, cpmfs_dir* dir, cpmfs_entry *entry);
int     libcpmfs_closedir(cpmfs * fs,cpmfs_dir* dir);
int     libcpmfs_createdirectory(cpmfs * fs,const char *path);
int     libcpmfs_remove(cpmfs * fs,const char * filename);