void init_cpm(HXCFE_FSMNG * fsmng);
int32_t cpm_mountImage(HXCFE_FSMNG * fsmng, HXCFE_FLOPPY *floppy);
int32_t cpm_umountImage(HXCFE_FSMNG * fsmng);
int32_t cpm_getFreeSpace(HXCFE_FSMNG * fsmng);
int32_t cpm_getTotalSpace(HXCFE_FSMNG * fsmng);
int32_t cpm_openFile(HXCFE_FSMNG * fsmng, char * filename);
int32_t cpm_createFile(HXCFE_FSMNG * fsmng, char * filename);
int32_t cpm_writeFile(HXCFE_FSMNG * fsmng,int32_t filehandle,unsigned char * buffer,int32_t size);
int32_t cpm_readFile( HXCFE_FSMNG * fsmng,int32_t filehandle,unsigned char * buffer,int32_t size);
int32_t cpm_deleteFile(HXCFE_FSMNG * fsmng, char * filename);
int32_t cpm_closeFile( HXCFE_FSMNG * fsmng,int32_t filehandle);
int32_t cpm_createDir( HXCFE_FSMNG * fsmng,char * foldername);
int32_t cpm_removeDir( HXCFE_FSMNG * fsmng,char * foldername);

int32_t cpm_ftell( HXCFE_FSMNG * fsmng,int32_t filehandle);
int32_t cpm_fseek( HXCFE_FSMNG * fsmng,int32_t filehandle,int32_t offset,int32_t origin);

int32_t cpm_openDir(HXCFE_FSMNG * fsmng, char * path);
int32_t cpm_readDir(HXCFE_FSMNG * fsmng,int32_t dirhandle,HXCFE_FSENTRY * dirent);
int32_t cpm_closeDir(HXCFE_FSMNG * fsmng, int32_t dirhandle);
