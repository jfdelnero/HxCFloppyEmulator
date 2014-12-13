
typedef int32_t (*fn_diskio_read) (uint32_t sector, unsigned char *buffer, uint32_t sector_count);
typedef int32_t (*fn_diskio_write)(uint32_t sector, unsigned char *buffer, uint32_t sector_count);

typedef struct _cpmfs
{
	fn_diskio_read  rdfn;
	fn_diskio_write wrfn;
}cpmfs;

typedef struct _cpmfs_dir
{
	int32_t sector;
}cpmfs_dir;

typedef struct _cpmfs_entry
{
	int32_t filename[8+3+1];
	int32_t size;
	int32_t is_dir;
}cpmfs_entry;

int32_t     libcpmfs_init(cpmfs * fs);
int32_t     libcpmfs_attach_media(cpmfs * fs, fn_diskio_read rd, fn_diskio_write wr);
int32_t     libcpmfs_deinit(cpmfs * fs);

int32_t     libcpmfs_mountImage(cpmfs * fs);
int32_t     libcpmfs_unmountImage(cpmfs * fs);

void  * libcpmfs_fopen(cpmfs * fs, const char *path, const char *modifiers);
int32_t     libcpmfs_fwrite(cpmfs * fs,const void * data, int32_t size, int32_t count, void *file );
int32_t     libcpmfs_ftell(cpmfs * fs, void *file);
int32_t     libcpmfs_fseek(cpmfs * fs, void *file, int32_t offset, int32_t origin );
int32_t     libcpmfs_fread(cpmfs * fs, void * data, int32_t size, int32_t count, void *file );
int32_t     libcpmfs_feof(cpmfs * fs,  void *file);
int32_t     libcpmfs_fclose(cpmfs * fs,void *file);

cpmfs_dir* libcpmfs_opendir(cpmfs * fs, const char* path, cpmfs_dir *dir);
int32_t     libcpmfs_readdir(cpmfs * fs, cpmfs_dir* dir, cpmfs_entry *entry);
int32_t     libcpmfs_closedir(cpmfs * fs,cpmfs_dir* dir);
int32_t     libcpmfs_createdirectory(cpmfs * fs,const char *path);
int32_t     libcpmfs_remove(cpmfs * fs,const char * filename);
