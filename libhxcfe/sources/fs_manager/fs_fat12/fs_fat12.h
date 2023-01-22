void init_fat12(HXCFE_FSMNG * fsmng);
int32_t fat12_mountImage(HXCFE_FSMNG * fsmng, HXCFE_FLOPPY *floppy);
int32_t fat12_umountImage(HXCFE_FSMNG * fsmng);
int32_t fat12_getFreeSpace(HXCFE_FSMNG * fsmng);
int32_t fat12_getTotalSpace(HXCFE_FSMNG * fsmng);
int32_t fat12_openFile(HXCFE_FSMNG * fsmng, char * filename);
int32_t fat12_createFile(HXCFE_FSMNG * fsmng, char * filename);
int32_t fat12_writeFile(HXCFE_FSMNG * fsmng,int32_t filehandle,unsigned char * buffer,int32_t size);
int32_t fat12_readFile( HXCFE_FSMNG * fsmng,int32_t filehandle,unsigned char * buffer,int32_t size);
int32_t fat12_deleteFile(HXCFE_FSMNG * fsmng, char * filename);
int32_t fat12_closeFile( HXCFE_FSMNG * fsmng,int32_t filehandle);
int32_t fat12_createDir( HXCFE_FSMNG * fsmng,char * foldername);
int32_t fat12_removeDir( HXCFE_FSMNG * fsmng,char * foldername);

int32_t fat12_ftell( HXCFE_FSMNG * fsmng,int32_t filehandle);
int32_t fat12_fseek( HXCFE_FSMNG * fsmng,int32_t filehandle,int32_t offset,int32_t origin);

int32_t fat12_openDir(HXCFE_FSMNG * fsmng, char * path);
int32_t fat12_readDir(HXCFE_FSMNG * fsmng,int32_t dirhandle,HXCFE_FSENTRY * dirent);
int32_t fat12_closeDir(HXCFE_FSMNG * fsmng, int32_t dirhandle);
